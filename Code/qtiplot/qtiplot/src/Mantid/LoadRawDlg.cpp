
#include <QtGui>
#include <qfiledialog.h>

#include "LoadRawDlg.h"
#include "InputHistory.h"

loadRawDlg::loadRawDlg(QWidget *parent) : QDialog(parent), fileName(""), workspaceName("")
{
	label = new QLabel(tr("Select Raw File to Load:"));
	lineFile = new QLineEdit;
	lineFile->setReadOnly(true);
	label->setBuddy(lineFile);
    QString propValue = InputHistory::Instance().algorithmProperty("LoadRaw","Filename");
    if (!propValue.isEmpty())
    {
        directory = InputHistory::Instance().getDirectoryFromFilePath(propValue);
        lineFile->setText(propValue);
        lineFile->setSelection(0,propValue.length());
    }
	
	label2 = new QLabel(tr("Enter Name for Workspace:"));
	lineName = new QLineEdit;
	label2->setBuddy(lineName);
    propValue = InputHistory::Instance().algorithmProperty("LoadRaw","OutputWorkspace");
    if (!propValue.isEmpty())
    {
        lineName->setText(propValue);
    }
	
	browseButton = new QPushButton(tr("Browse"));
	loadButton = new QPushButton(tr("Load"));
	cancelButton = new QPushButton(tr("Cancel"));

	connect(browseButton, SIGNAL(clicked()), this, SLOT(browseClicked()));
	connect(cancelButton, SIGNAL(clicked()), this, SLOT(close()));
	connect(loadButton, SIGNAL(clicked()), this, SLOT(loadClicked()));
	
	//Set the appearance
	QHBoxLayout *topRowLayout = new QHBoxLayout;
	topRowLayout->addWidget(label);
	topRowLayout->addWidget(lineFile);
	topRowLayout->addWidget(browseButton);
	
	QHBoxLayout *middleRowLayout = new QHBoxLayout;
	middleRowLayout->addWidget(label2);
	middleRowLayout->addWidget(lineName);
	
	QHBoxLayout *bottomRowLayout = new QHBoxLayout;
	bottomRowLayout->addStretch();
	bottomRowLayout->addWidget(cancelButton);
	bottomRowLayout->addWidget(loadButton);
	
	QGridLayout *paramsLayout = new QGridLayout;
    QLabel *minSpLabel = new QLabel("Starting spectrum");
    minSpLineEdit = new QLineEdit;
    propValue = InputHistory::Instance().algorithmProperty("LoadRaw","spectrum_min");
    if (!propValue.isEmpty())
    {
        minSpLineEdit->setText(propValue);
    }
    paramsLayout->addWidget(minSpLabel,0,0);
    paramsLayout->addWidget(minSpLineEdit,0,1);

    QLabel *maxSpLabel = new QLabel("Ending spectrum");
    maxSpLineEdit = new QLineEdit;
    propValue = InputHistory::Instance().algorithmProperty("LoadRaw","spectrum_max");
    if (!propValue.isEmpty())
    {
        maxSpLineEdit->setText(propValue);
    }
    paramsLayout->addWidget(maxSpLabel,1,0);
    paramsLayout->addWidget(maxSpLineEdit,1,1);

    QLabel *listSpLabel = new QLabel("Spectrum list");
    listSpLineEdit = new QLineEdit;
    propValue = InputHistory::Instance().algorithmProperty("LoadRaw","spectrum_list");
    if (!propValue.isEmpty())
    {
        listSpLineEdit->setText(propValue);
    }
    paramsLayout->addWidget(listSpLabel,2,0);
    paramsLayout->addWidget(listSpLineEdit,2,1);

    QLabel *cacheLabel = new QLabel("Cache on local drive");
    cacheCBox = new QComboBox;
    cacheCBox->insertItem(0,"Never");
    cacheCBox->insertItem(0,"Always");
    cacheCBox->insertItem(0,"If slow");
    cacheCBox->setCurrentIndex(0);
    propValue = InputHistory::Instance().algorithmProperty("LoadRaw","Cache");
    if (!propValue.isEmpty())
    {
        int i = cacheCBox->findText(propValue);
        if (i >= 0)
            cacheCBox->setCurrentIndex(i);
    }
    paramsLayout->addWidget(cacheLabel,3,0);
    paramsLayout->addWidget(cacheCBox,3,1);


    mainLayout = new QVBoxLayout;
	mainLayout->addLayout(topRowLayout);
	mainLayout->addLayout(middleRowLayout);
	mainLayout->addLayout(paramsLayout);
	mainLayout->addLayout(bottomRowLayout);
	
	setLayout(mainLayout);
	setWindowTitle(tr("Load Raw File"));
	setFixedHeight(sizeHint().height());
}

loadRawDlg::~loadRawDlg()
{
	
}

void loadRawDlg::browseClicked()
{
	QString s( QFileDialog::getOpenFileName(this, tr("Select Raw File"), directory, tr("Raw File (*.RAW)") ) );
	if ( s.isEmpty() )  return;
	lineFile->setText(s);

	// Suggest a name for the workspace
	int i = s.lastIndexOf('\\');
	if (i < 0) i = s.lastIndexOf('/');
	if (i < 0) i = 0;
	int j = s.lastIndexOf('.');
	if (j < 0) j = s.length();
	lineName->setText(s.mid(i+1,j - i - 1));
	lineName->setSelection(0,lineName->text().length());
	lineName->setFocus(Qt::OtherFocusReason);
	directory = s.remove(i,s.length()-i);
}

void loadRawDlg::loadClicked()
{
	if (!lineFile->text().isNull() && !lineFile->text().isEmpty() && !lineName->text().isNull() && !lineName->text().isEmpty())
	{
		fileName = lineFile->text();
		workspaceName = lineName->text();
        spectrum_min = minSpLineEdit->text();
        spectrum_max = maxSpLineEdit->text();
        spectrum_list = listSpLineEdit->text();
        cache_option = cacheCBox->currentText();
		close();
	}
}

