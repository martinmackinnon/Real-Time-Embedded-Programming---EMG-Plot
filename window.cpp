#include "window.h"
#include "adcreader.h"
#include "bcm2835.h"
#include <cmath>  
#include <stdio.h>
Window::Window() 

{   //Set up Button2
    button1.setText("Filter On");
    connect(&button1, SIGNAL(released()), SLOT(filter()));

    //Set up Button1
    //button1 = new QPushButton;
    button2.setText("Change Units to mV");
    connect(&button2, SIGNAL(released()), SLOT(units()));

	// set up the initial plot data
	for( int index=0; index<plotDataSize; ++index )
	{
		xData[index] = index;
		yData[index] = 0;
	}

	curve = new QwtPlotCurve;
	plot = new QwtPlot;

	// make a plot curve from the data and attach it to the plot
	curve->setSamples(xData, yData, plotDataSize);
	curve->attach(plot);
	plot->setTitle("EMG Plot");
	plot->replot();
	plot->show();


    // set up the layout
	vLayout = new QVBoxLayout;
	vLayout->addWidget(&button1);
   	vLayout->addWidget(&button2);


	hLayout = new QHBoxLayout;
	hLayout->addLayout(vLayout);
	hLayout->addWidget(plot);

	setLayout(hLayout);

    // Starting the thread
	adcreader = new ADCreader();
	adcreader->start();
	
}

Window::~Window() {
	// tells the thread to no longer run its endless loop
	adcreader->quit();
	// wait until the run method has terminated
	adcreader->wait();
	delete adcreader;
}

void Window::timerEvent( QTimerEvent * )
{   //When x-axis title is set outside of this timer event, an error occurs
    //in the thread with the 'writeReg()' function - we realise this is far from the best
    //place to put it! However, it is most efficient place for it which does not cause error
    plot->setAxisTitle(QwtPlot::xBottom, "Sample Number");
	double inVal = 0;
	while(adcreader->hasSample())
	{
	inVal = (adcreader->getSample());
    	

	// add the new input to the plot
	memmove( yData, yData+1, (plotDataSize-1) * sizeof(double) );
	if(flagu==0)
	{
		inVal = (inVal/65535)*3.3;
		plot->setAxisTitle(QwtPlot::yLeft, "Voltage (V)");
	}

	else
	{
	        inVal = ((inVal/65535)*3.3)*1000;
		plot->setAxisTitle(QwtPlot::yLeft, "Voltage (mV)");
	}
        yData[plotDataSize-1] =   -inVal;
        curve->setSamples(xData, yData, plotDataSize);

	
        
        }
        plot->setTitle("EMG Plot");
        plot->replot();
	
}

void Window::filter(){
	if (flagf == 0){
	   button1.setText("Filter Signal");
	   flagf = 1;
	}
	else
	{
	   button1.setText("Unfilter Signal");
	   flagf = 0;
	}
}


void Window::units(){
	 if (flagu == 0){
		 button2.setText("Change Units to V");
		 flagu = 1;
	 }
	 else {
		 button2.setText("Change Units to mV");
         flagu = 0;
    }

}
