#include "BinDialog.h"

///< Qt Includes
#include <QString>
#include <QGroupBox>
#include <QGridLayout>
#include <QIntValidator>
#include <QPushButton>
#include <QLabel>
/**
 *Constructor
 */
BinDialog::BinDialog(QWidget* parent):QDialog(parent)
{
	//Set the tile of the window
	setWindowTitle (QString("Select Bin Operation"));

	//Create a group for the radio button to select bin map type
	QGroupBox *groupBox = new QGroupBox();
	mSingleBinRBtn = new QRadioButton(tr("Single Bin"));
	mIntegralRBtn=new QRadioButton(tr("Integral Value"));
	
	//Input value for SingleBin options
	QValidator *validator = new QIntValidator(this);
	mSingleBinValue = new QLineEdit();
	mSingleBinValue->setValidator(validator);
	//Input value for Integral option
	mIntegralMinValue = new QLineEdit();
	mIntegralMinValue->setValidator(validator);
	mIntegralMaxValue = new QLineEdit();
	mIntegralMaxValue->setValidator(validator);
	//Make Integral as default
	mIntegralRBtn->setChecked(true);
	//Create a grid layout
	QGridLayout  *gridbox = new QGridLayout;
	gridbox->addWidget(mSingleBinRBtn,0,0);
	gridbox->addWidget(new QLabel("Value:"),1,0);
	gridbox->addWidget(mSingleBinValue,1,1);
	gridbox->addWidget(mIntegralRBtn,2,0);
	gridbox->addWidget(new QLabel("Min Bin:"),3,0);
	gridbox->addWidget(mIntegralMinValue,3,1);
	gridbox->addWidget(new QLabel("Max Bin:"),4,0);
	gridbox->addWidget(mIntegralMaxValue,4,1);
	groupBox->setLayout(gridbox);

	//create a frame for Ok and Cancel btn
	QFrame* okcancelFrame=new QFrame();
	QPushButton* okButton=new QPushButton("Ok");
	QPushButton* cancelButton=new QPushButton("Cancel");
	QHBoxLayout* okcancelLayout=new QHBoxLayout;
	okcancelLayout->addWidget(okButton);
	okcancelLayout->addWidget(cancelButton);
	okcancelFrame->setLayout(okcancelLayout);
	QVBoxLayout *dialogLayout=new QVBoxLayout();
	dialogLayout->addWidget(groupBox);
	dialogLayout->addWidget(okcancelFrame);
	this->setLayout(dialogLayout);
	connect(cancelButton, SIGNAL(clicked()), this, SLOT(reject()));	
	connect(okButton,SIGNAL(clicked()),this,SLOT(btnOKClicked()));
}

/**
 * Destructor
 */
BinDialog::~BinDialog()
{
}

void BinDialog::setSingleBinNumber(int binNum)
{
	QString strBinNum;
	mSingleBinValue->setText(strBinNum.setNum(binNum));
}

void BinDialog::setIntegralMinMax(int minBin,int maxBin)
{
	QString strBinNum;
	mIntegralMinValue->setText(strBinNum.setNum(minBin));
	mIntegralMaxValue->setText(strBinNum.setNum(maxBin));
}

void BinDialog::btnOKClicked()
{
	if(mSingleBinRBtn->isChecked()) // Single Bin value
	{
		emit SingleBinNumber( mSingleBinValue->displayText().toInt() );
	}
	else if(mIntegralRBtn->isChecked()) // Integral Value Selected
	{
		emit IntegralMinMax( mIntegralMinValue->displayText().toInt(),mIntegralMaxValue->displayText().toInt());
	}
	accept();
}