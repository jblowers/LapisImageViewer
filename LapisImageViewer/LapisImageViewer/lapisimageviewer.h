#ifndef LAPISIMAGEVIEWER_H
#define LAPISIMAGEVIEWER_H

#include <QtWidgets/QMainWindow>
#include "ui_lapisimageviewer.h"


#include "QString.h"
#include "qfile.h"
#include <QTextStream>
#include "qdir.h"
#include "qfiledialog.h"

#include "opencv2/core/core.hpp"
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/opencv.hpp>


#include "StdIQM.h"


#define LAPIS_IQM_SCALING_VALUE 1000000

class LapisImageViewer : public QMainWindow
{
	Q_OBJECT

public:
	LapisImageViewer(QWidget *parent = 0);
	~LapisImageViewer();


	double *m_Iqm;
	double *m_IqmOrdered;
	int *m_IqmOrderedIndex;

	int *m_nCCIndex;
	int *m_nxShift;
	int *m_nyShift;
	int *m_nBrightness;
	bool *m_bGoodData;


	double m_dMinIqm;
	double m_dMaxIqm;
	int m_nMaxIqmInd;
	int m_nMinIqmInd;

	bool m_bIQMLoaded;

	cv::Mat m_matIqmOrdered;

public slots:

	void onLoadOneImagePushed();

	void onSpinBoxChanged(int nVal);
	void onSliderChanged(int nVal);
	void onCheckImageDirectoryPushed();
	void onLoadIQMData();

	void onGoToMaxImagePushed();
	void onGoToMinImagePushed();

	void onBrowseForIqmFilePushed();
	void onBrowseForImageDirectoryPushed();

signals:






private:
	Ui::LapisImageViewerClass ui;
};

#endif // LAPISIMAGEVIEWER_H
