#include "lapisimageviewer.h"

LapisImageViewer::LapisImageViewer(QWidget *parent)
	: QMainWindow(parent)
{
	ui.setupUi(this);

	connect(ui.LoadOneImageButton, SIGNAL(clicked()), this, SLOT(onLoadOneImagePushed()));

	connect(ui.LoadOneImageSpinBox, SIGNAL(valueChanged(int)), this, SLOT(onLoadOneImagePushed()));
	connect(ui.LoadOneImageSpinBox, SIGNAL(valueChanged(int)), this, SLOT(onSpinBoxChanged(int)));
	connect(ui.ImageSlider, SIGNAL(valueChanged(int)), this, SLOT(onSliderChanged(int)));

	connect(ui.CheckImageDirectoryButton, SIGNAL(clicked()), this, SLOT(onCheckImageDirectoryPushed()));
	connect(ui.GoToMaxButton, SIGNAL(clicked()), this, SLOT(onGoToMaxImagePushed()));
	connect(ui.GoToMinButton, SIGNAL(clicked()), this, SLOT(onGoToMinImagePushed()));

	connect(ui.BrowseImageDirectoryButton, SIGNAL(clicked()), this, SLOT(onBrowseForImageDirectoryPushed()));
	connect(ui.BrowseIqmFileButton, SIGNAL(clicked()), this, SLOT(onBrowseForIqmFilePushed()));

	m_bIQMLoaded = false;
	m_Iqm = new double[3601];
	m_IqmOrdered = new double[3601];
	m_IqmOrderedIndex = new int[3601];

	m_nBrightness = new int[3601];
	m_nxShift = new int[3601];
	m_nyShift = new int[3601];
	m_bGoodData = new bool[3601];
	m_nCCIndex = new int[3601];

	onCheckImageDirectoryPushed();
	onLoadIQMData(); // maybe don't wanna do this is if IQM file doesn't exist...

}

LapisImageViewer::~LapisImageViewer()
{

}

void LapisImageViewer::onLoadOneImagePushed()
{
	QString strPath = "E:\\LapisTestTwo\\20160914\\A1_N_300m_276fps_1237\\";
	strPath = ui.BrowseImageDirectoryLineEdit->text();
	strPath += "\\";

	int nImNum = ui.LoadOneImageSpinBox->value();
	int nIqmIndex;
	if ( ui.IqmOrderRadioButton->isChecked() ) {
		// want to adjust the nImNum to reflect the IQM ordered set index
		nImNum;
		//printf("Requested Ind: %d\t", nImNum);
		//nImNum = m_IqmOrderedIndex[nImNum + 1];
		if ( m_IqmOrderedIndex[nImNum] > 0 )
			nImNum = m_IqmOrderedIndex[nImNum];
		else
			printf("Error in ordered indices...\n");
		ui.IqmIndexLabel->setText(QString::number(nImNum));
		//printf("Corresponding IQM Ind: %d\n", nImNum);

	} //else {

	// update gui with metadata
	QString metaData = "Index: " + QString::number(nImNum) + "\tCCIndex: " + QString::number(m_nCCIndex[nImNum]) + "\txShift: " + QString::number(m_nxShift[nImNum]) + "\tyShift: " + QString::number(m_nyShift[nImNum]) + "\tGood Data? " + QString::number(m_bGoodData[nImNum]) + "\tBrightness: " + QString::number(m_nBrightness[nImNum]);
	ui.MetadataLineEdit->setText(metaData);

	if ( nImNum < 10 )
		strPath += "000" + QString::number(nImNum) + ".tif";
	else if ( nImNum < 100 )
		strPath += "00" + QString::number(nImNum) + ".tif";
	else if ( nImNum < 1000 )
		strPath += "0" + QString::number(nImNum) + ".tif";
	else
		strPath += QString::number(nImNum) + ".tif";

	cv::Mat matIm = cv::imread(strPath.toStdString(), cv::IMREAD_UNCHANGED);

	if ( ui.MedianFilterCheckBox->isChecked() ) {
		cv::medianBlur(matIm, matIm, 3);
	}

	double dCurIqm;
	if ( ui.CalculateIqmCheckBox->isChecked() ) {
		StdIQM metric;
		dCurIqm = metric.GetMetric(matIm, false) * LAPIS_IQM_SCALING_VALUE;
		ui.IQMLabel->setText(QString::number(dCurIqm, 'f', 6));
	}
	else {
		if ( !m_bIQMLoaded )
			onLoadIQMData();
		dCurIqm = m_Iqm[nImNum];
		ui.IQMLabel->setText(QString::number(dCurIqm));
	}

	double min, max;
	cv::minMaxLoc(matIm, &min, &max);
	cv::Mat matDisp = cv::Mat(matIm.size(), CV_8UC1);
	matIm.convertTo(matIm, CV_64FC1);
	matIm = (matIm - min) / (max - min) * 256;
	matIm.convertTo(matDisp, CV_8UC1);
	//matDisp = (matIm - min)/(max-min) * 256;

	// putting iqm value on image
	cv::putText(matDisp, QString::number(dCurIqm).toStdString().c_str(), cv::Point(0, 30), cv::HersheyFonts::FONT_HERSHEY_PLAIN, 2.0, cv::Scalar(255, 255, 255, 1), 3.0); // white
	cv::putText(matDisp, QString::number(dCurIqm).toStdString().c_str(), cv::Point(0, 30), cv::HersheyFonts::FONT_HERSHEY_PLAIN, 2.0, cv::Scalar(0, 0, 0, 0), 2.0); // black

	cv::namedWindow("Image to Display");

	cv::imshow("Image to Display", matDisp);
	//cv::waitKey(0);


}

// loads iqm data from file
void LapisImageViewer::onLoadIQMData()
{

	double dCurIqmVal; double Iqm[3601];
	double dMax = 0, dMin = 0xff;
	int nMaxInd = -1, nMinInd = -1;
	int nInd = 0;

	if ( 1 ) {    /// when we want to read from file
		QString path = ui.BrowseIqmFileLineEdit->text();
		QFile file(path);

		if ( !file.open(QIODevice::ReadOnly) ) {
			printf("Error opening iqm file\n");
			return;
		}

		QTextStream in(&file);
		QString line;// = in.readLine(); // first line is junk data (actually just the name of the columns)
		while ( !in.atEnd() ) {
			line = in.readLine();
			QStringList  fields = line.split("\t");

			// finding min/max
			dCurIqmVal = fields[1].toDouble();
			if ( dCurIqmVal < -0xff ){
				dCurIqmVal = 0;
				//continue;
			}
			Iqm[nInd] = dCurIqmVal;
			if ( dCurIqmVal != 0 ) {
				if ( dCurIqmVal > dMax ) {
					dMax = dCurIqmVal;
					nMaxInd = nInd;
				}
				if ( dCurIqmVal < dMin ) {
					dMin = dCurIqmVal;
					nMinInd = nInd;
				}
			}
			nInd++;
		}
	}
	else { // Option to read all images in directory and calculate all IQM values new

		QString strDirPath = ui.BrowseImageDirectoryLineEdit->text(), strFilePath;
		QDir direct(strDirPath);

		QStringList fils = direct.entryList();
		cv::Mat matIm;
		StdIQM iqm;
		int nTifCount = 0, nIqmCount = 0;
		for ( int i = 0; i < fils.length(); i++ ) {
			if ( fils[i].contains(".tif") && !fils[i].contains("%") ) { // one image is just %04d.tif so we want to leave that one out.

				strFilePath = strDirPath + "\\" + fils[i];

				matIm = cv::imread(strFilePath.toStdString(), cv::IMREAD_UNCHANGED);
				if ( ui.MedianFilterCheckBox->isChecked() ) 
					cv::medianBlur(matIm,matIm,3);

				Iqm[nInd] = iqm.GetMetric(matIm, false);

				// finding min/max
				dCurIqmVal = Iqm[nInd];
				if ( dCurIqmVal > dMax ) {
					dMax = dCurIqmVal;
					nMaxInd = nInd;
				}
				if ( dCurIqmVal < dMin ) {
					dMin = dCurIqmVal;
					nMinInd = nInd;
				}
				nInd++;
			}

			if ( i%100 == 0 )
				printf("IQM calculated for image: %d\n",i);

		}

	}

	memcpy(m_Iqm, Iqm, (nInd - 1)*sizeof(double));

	m_dMaxIqm = dMax*LAPIS_IQM_SCALING_VALUE; m_nMaxIqmInd = nMaxInd;
	m_dMinIqm = dMin*LAPIS_IQM_SCALING_VALUE; m_nMinIqmInd = nMinInd;

	ui.MaxIqmLabel->setText(QString::number(dMax*LAPIS_IQM_SCALING_VALUE));
	ui.MinIqmLabel->setText(QString::number(dMin*LAPIS_IQM_SCALING_VALUE));

	m_Iqm[0] = 0;

	cv::Mat matIqm = cv::Mat(1, nInd - 1, CV_64FC1, m_Iqm);
	matIqm *= LAPIS_IQM_SCALING_VALUE;
	cv::Mat matIqmOrderedIndex;


	cv::sortIdx(matIqm, matIqmOrderedIndex, cv::SortFlags::SORT_ASCENDING);

	memcpy(m_IqmOrderedIndex, matIqmOrderedIndex.data, 3601 * sizeof(int));
	//m_IqmOrderedIndex = (int*)matIqmOrderedIndex.data;

	m_bIQMLoaded = true;


	//////////////////////////////////////////////
	// reading metadata also /////////////////////

	QString path = ui.BrowseImageDirectoryLineEdit->text();
	path += "\\.txt";
	QFile file(path);

	if ( !file.open(QIODevice::ReadOnly) ) {
		printf("Error opening iqm file\n");
		return;
	}

	int nCCIndex[3601];
	int nxShift[3601];
	int nyShift[3601];
	bool bGoodData[3601];
	int nBrightness[3601];
	int nIndex = 0;

	QTextStream in(&file);
	QString line;// = in.readLine(); // first line is junk data
	in.readLine(); // ignoring the first line
	while ( !in.atEnd() ) {
		line = in.readLine();
		QStringList  fields = line.split(",");
		// when reading an S set, the following will be true
		// fields -> 0 = frame number, 1 = frame Index, 2 = Good data, 3 = ccIndex, 4 = xshift, 5 = yshift, 6 = brightness
		// when reading an N set, all but the brightness will be useless info
		// for T set, shift data is not very trustable...

		nIndex = fields[0].toInt();
		if ( fields[2].toInt() < 0 )
			bGoodData[nIndex] = false;
		else
			bGoodData[nIndex] = true;
		nCCIndex[nIndex] = fields[3].toInt();
		nxShift[nIndex] = fields[4].toInt();
		nyShift[nIndex] = fields[5].toInt();
		nBrightness[nIndex] = fields[6].toInt();



	}

	// save local arrays to members
	memcpy(m_nBrightness, nBrightness, 3601 * sizeof(int));
	memcpy(m_nCCIndex, nCCIndex, 3601 * sizeof(int));
	memcpy(m_nyShift, nyShift, 3601 * sizeof(int));
	memcpy(m_nxShift, nxShift, 3601 * sizeof(int));
	memcpy(m_bGoodData, bGoodData, 3601 * sizeof(bool));

}


void LapisImageViewer::onBrowseForImageDirectoryPushed()
{
	QString folderName = QFileDialog::getExistingDirectory(this, tr("Open Folder"), ".");
	ui.BrowseImageDirectoryLineEdit->setText(folderName);

	onCheckImageDirectoryPushed();
	onLoadIQMData();
}

void LapisImageViewer::onBrowseForIqmFilePushed()
{
	QString fileName = QFileDialog::getOpenFileName(this, tr("IQM File"), ".txt");
	ui.BrowseIqmFileLineEdit->setText(fileName);

	onLoadIQMData();
}

void LapisImageViewer::onSliderChanged(int nVal)
{
	ui.LoadOneImageSpinBox->setValue(nVal);

}


void LapisImageViewer::onSpinBoxChanged(int nVal)
{
	ui.ImageSlider->setValue(nVal);

}

void LapisImageViewer::onGoToMaxImagePushed()
{
	int nMaxInd = 0;
	if ( ui.IqmOrderRadioButton->isChecked() ) {
		nMaxInd = 3600;
	}
	else
		nMaxInd = m_nMaxIqmInd;
	ui.LoadOneImageSpinBox->setValue(nMaxInd);
}

void LapisImageViewer::onGoToMinImagePushed()
{
	int nMinInd = 0;
	if ( ui.IqmOrderRadioButton->isChecked() ) {
		nMinInd = 1;
	}
	else
		nMinInd = m_nMinIqmInd;
	ui.LoadOneImageSpinBox->setValue(nMinInd);
}



void LapisImageViewer::onCheckImageDirectoryPushed()
{
	// want to go through directory and find how many tif files are present. Potentially also check for the iqm file and put this path into the iqm lineEdit
	QString strPath = ui.BrowseImageDirectoryLineEdit->text();

	QDir direct(strPath);

	QStringList fils = direct.entryList();

	int nTifCount = 0, nIqmCount = 0;
	for ( int i = 0; i < fils.length(); i++ ) {

		if ( fils[i].contains(".tif") && !fils[i].contains("%") ) { // one image is just %04d.tif so we want to leave that one out.
			// tif file
			nTifCount++;

		}
		else if ( fils[i].contains("IQM") ) {
			// iqm file
			nIqmCount++;
			QString strIqmPath = direct.absoluteFilePath(fils[i]);
			ui.BrowseIqmFileLineEdit->setText(strIqmPath);
		}

	}
	ui.ImageSlider->setMaximum(nTifCount - 1);
	ui.LoadOneImageSpinBox->setMaximum(nTifCount - 1);
	ui.MaxImageNumberLabel->setText(QString::number(nTifCount - 1));

}