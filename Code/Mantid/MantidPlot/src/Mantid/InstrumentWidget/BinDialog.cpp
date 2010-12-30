#include "BinDialog.h"

///< Qt Includes
#include <QString>
#include <QGroupBox>
#include <QGridLayout>
#include <QDoubleValidator>
#include <QPushButton>
#include <QLabel>
#include <QLineEdit>
#include <QRadioButton>

/**
 *Constructor
 */
BinDialog::BinDialog(QWidget* parent):QDialog(parent)
{
	//Set the tile of the window
	setWindowTitle (QString("Select X Range"));

	//frame for group box
	QFrame* groupBox=new QFrame();
	//Double Validator
	QValidator *validator = new QDoubleValidator(this);
	//Input value for Integral option
	mIntegralMinValue = new QLineEdit();
	mIntegralMinValue->setValidator(validator);
	mIntegralMaxValue = new QLineEdit();
	mIntegralMaxValue->setValidator(validator);

	//Checkbco
	mEntireRange = new QCheckBox("Use the entire X range", this);
  connect(mEntireRange,SIGNAL(toggled(bool)),this,SLOT(mEntireRange_toggled(bool)));

	//Create a grid layout
	QGridLayout  *gridbox = new QGridLayout;
	gridbox->addWidget(new QLabel("Min X Value:"),0,0);
	gridbox->addWidget(mIntegralMinValue,0,1);
  gridbox->addWidget(new QLabel("Max X Value:"),1,0);
  gridbox->addWidget(mIntegralMaxValue,1,1);
  gridbox->addWidget(mEntireRange,2,1);
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

/** Set the values in the GUI. */
void BinDialog::setIntegralMinMax(double minBin,double maxBin, bool useEverything)
{
	QString strBinNum;
	mIntegralMinValue->setText(strBinNum.setNum(minBin));
	mIntegralMaxValue->setText(strBinNum.setNum(maxBin));
	//And the checkbox
	mEntireRange->setChecked(useEverything);
	this->mEntireRange_toggled(useEverything);
}


/** Called when the OK button is pressed. */
void BinDialog::btnOKClicked()
{
	emit IntegralMinMax( mIntegralMinValue->displayText().toDouble(),mIntegralMaxValue->displayText().toDouble(), mEntireRange->isChecked());
	accept();
}

/** Called when the mEntireRange checkbox state toggles.
 * Disables the textboxes if the checkbox is on.
 * */
void BinDialog::mEntireRange_toggled(bool on)
{
  this->mIntegralMaxValue->setEnabled(!on);
  this->mIntegralMinValue->setEnabled(!on);
}
