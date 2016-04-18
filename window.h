#ifndef WINDOW_H
#define WINDOW_H

#include <qwt/qwt_thermo.h>
#include <qwt/qwt_knob.h>
#include <qpushbutton.h>
#include <qwt/qwt_plot.h>
#include <qwt/qwt_plot_curve.h>

#include <QBoxLayout>

#include "adcreader.h"

// class definition 'Window'
class Window : public QWidget
{
	// must include the Q_OBJECT macro for for the Qt signals/slots framework to work with this class
	Q_OBJECT

public:
	Window(); // default constructor - called when a Window is declared without arguments

	~Window();

	void timerEvent( QTimerEvent * );

public slots:
	void units();
	void filter();
	 

// internal variables for the window class
private:
	QPushButton   button1;
	QPushButton   button2;
	QwtPlot      *plot;
	QwtPlotCurve *curve;

	// layout elements from Qt itself http://qt-project.org/doc/qt-4.8/classes.html
	QVBoxLayout  *vLayout;  // vertical layout
	QHBoxLayout  *hLayout;  // horizontal layout

	static const int plotDataSize = 1000;

	// data arrays for the plot
	double xData[plotDataSize];
	double yData[plotDataSize];

	double gain;
	int count;
	int flagf =0;
	int flagu = 0;
	
	
	ADCreader *adcreader;
};

#endif // WINDOW_H
