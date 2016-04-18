#include <window.h>
#include <adcreader.h>
#include <QApplication>

int main(int argc, char *argv[])
{
	QApplication app(argc, argv);

	// create the window
	Window window;
	window.showMaximized();
   
	   window.startTimer(40);
//	window.startTimer(24);

	// execute the application
	return app.exec();
}
