
#include <QtGui>
#include <qfiledialog.h>

#include "LoadRawDlg.h"

loadRawDlg::loadRawDlg(QWidget *parent) : QDialog(parent), fileName(""), workspaceName("")
{
	label = new QLabel(tr("Select Raw File to Load:"));
	lineFile = new QLineEdit;
	lineFile->setReadOnly(true);
	label->setBuddy(lineFile);
	
	label2 = new QLabel(tr("Enter Name for Workspace:"));
	lineName = new QLineEdit;
	label2->setBuddy(lineName);
	
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
	
	QVBoxLayout *mainLayout = new QVBoxLayout;
	mainLayout->addLayout(topRowLayout);
	mainLayout->addLayout(middleRowLayout);
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
    static QString curDir = "";

	QString s( QFileDialog::getOpenFileName(this, tr("Select Raw File"), curDir, tr("Raw File (*.RAW)") ) );
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
    curDir = s.remove(i,s.length()-i);

}

void loadRawDlg::loadClicked()
{
	if (!lineFile->text().isNull() && !lineFile->text().isEmpty() && !lineName->text().isNull() && !lineName->text().isEmpty())
	{
		fileName = lineFile->text();
		workspaceName = lineName->text();
		close();
	}
}

