#include "lapisimageviewer.h"
#include <QtWidgets/QApplication>

int main(int argc, char *argv[])
{
	QApplication a(argc, argv);
	LapisImageViewer w;
	w.show();
	return a.exec();
}
