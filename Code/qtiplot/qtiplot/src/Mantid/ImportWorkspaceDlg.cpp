
#include <QtGui>
#include <qfiledialog.h>

#include "ImportWorkspaceDlg.h"

ImportWorkspaceDlg::ImportWorkspaceDlg(QWidget *parent, int num) : QDialog(parent), numHists(num)
{
	label = new QLabel(tr("Set Histogram Range to Load (Max Number = " + QString::number(numHists) + "):"));
	
	labelLow = new QLabel(tr("From:"));
	lineLow = new QLineEdit;
	lineLow->setText("0");
	labelLow->setBuddy(lineLow);
	
	labelHigh = new QLabel(tr("To:"));
	lineHigh = new QLineEdit;
	lineHigh->setText(QString::number(numHists));
	labelHigh->setBuddy(lineHigh);
	
	okButton = new QPushButton(tr("OK"));
	okButton->setDefault(true);
	cancelButton = new QPushButton(tr("Cancel"));

	connect(cancelButton, SIGNAL(clicked()), this, SLOT(reject()));
	connect(okButton, SIGNAL(clicked()), this, SLOT(okClicked()));
	
	//Set the appearance
	QHBoxLayout *topRowLayout = new QHBoxLayout;
	topRowLayout->addWidget(label);
	
	QHBoxLayout *middleRowLayout = new QHBoxLayout;
	middleRowLayout->addWidget(labelLow);
	middleRowLayout->addWidget(lineLow);
	middleRowLayout->addWidget(labelHigh);
	middleRowLayout->addWidget(lineHigh);
	
	QHBoxLayout *bottomRowLayout = new QHBoxLayout;
	bottomRowLayout->addStretch();
	bottomRowLayout->addWidget(cancelButton);
	bottomRowLayout->addWidget(okButton);
	
	QVBoxLayout *mainLayout = new QVBoxLayout;
	mainLayout->addLayout(topRowLayout);
	mainLayout->addLayout(middleRowLayout);
	mainLayout->addLayout(bottomRowLayout);
	
	setLayout(mainLayout);
	setWindowTitle(tr("Set Histogram Range"));
	setFixedHeight(sizeHint().height());
}

ImportWorkspaceDlg::~ImportWorkspaceDlg()
{
	
}

void ImportWorkspaceDlg::okClicked()
{
	if (!lineLow->text().isNull() && !lineLow->text().isEmpty() && !lineHigh->text().isNull() && !lineHigh->text().isEmpty())
	{
		//Check range is valid
		bool ok;
		int low = lineLow->text().toInt(&ok, 10); 
		
		if (!ok || low < 0 || low > numHists)
		{
			QMessageBox::warning(this, tr("Mantid"),
                   		tr("Lower limit is not valid - please change it.\n")
                    		, QMessageBox::Ok, QMessageBox::Ok);
			return;
		}
		
		int high = lineHigh->text().toInt(&ok, 10); 
		
		if (!ok || high < 0 || high > numHists)
		{
			QMessageBox::warning(this, tr("Mantid"),
                   		tr("Upper limit is not valid - please change it.\n")
                    		, QMessageBox::Ok, QMessageBox::Ok);
			return;
		}
		
		if (high < low)
		{
			lowerLimit = high;
			upperLimit = low;
		}
		else
		{
			lowerLimit = low;
			upperLimit = high;
		}
		
		accept();
	}
}

