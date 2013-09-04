//---------------------------
// Includes
//--------------------------

#include "AxisDetails.h"

#include <QWidget>

AxisAxisDetails::AxisAxisDetails(QWidget *parent) :
    QWidget(parent)
{

}
AxisAxisDetails::~AxisAxisDetails()
{

}
ScaleAxisDetails::ScaleAxisDetails(QWidget *parent) :
    QWidget(parent)
{
  QGroupBox * middleBox = new QGroupBox(QString());
  QGridLayout * middleLayout = new QGridLayout(middleBox);

  middleLayout->addWidget(new QLabel(tr( "From" )), 0, 0);
  dspnStart = new DoubleSpinBox();
  dspnStart->setLocale(d_app->locale());
  dspnStart->setDecimals(d_app->d_decimal_digits);
  middleLayout->addWidget( dspnStart, 0, 1 );

  dteStartDateTime = new QDateTimeEdit();
  dteStartDateTime->setCalendarPopup(true);
  middleLayout->addWidget( dteStartDateTime, 0, 1 );
  dteStartDateTime->hide();

  timStartTime = new QTimeEdit();
  middleLayout->addWidget(timStartTime, 0, 1 );
  timStartTime->hide();

  middleLayout->addWidget(new QLabel(tr( "To" )), 1, 0);
  dspnEnd = new DoubleSpinBox();
  dspnEnd->setLocale(d_app->locale());
  dspnEnd->setDecimals(d_app->d_decimal_digits);
  middleLayout->addWidget( dspnEnd, 1, 1);

  dteEndDateTime = new QDateTimeEdit();
  dteEndDateTime->setCalendarPopup(true);
  middleLayout->addWidget(dteEndDateTime, 1, 1);
  dteEndDateTime->hide();

  timEndTime = new QTimeEdit();
  middleLayout->addWidget(timEndTime, 1, 1);
  timEndTime->hide();

  lblScaleTypeLabel = new QLabel(tr( "Type" ));
  cmbScaleType = new QComboBox();
  cmbScaleType->addItem(tr( "linear" ) );
  cmbScaleType->addItem(tr( "logarithmic" ) );
  middleLayout->addWidget( lblScaleTypeLabel, 2, 0);
  middleLayout->addWidget( cmbScaleType, 2, 1);

  chkInvert = new QCheckBox();
  chkInvert->setText( tr( "Inverted" ) );
  chkInvert->setChecked(false);
  middleLayout->addWidget( chkInvert, 3, 1 );
  middleLayout->setRowStretch(4, 1);

  grpAxesBreaks = new QGroupBox(tr("Show Axis &Break"));
  grpAxesBreaks->setCheckable(true);
  grpAxesBreaks->setChecked(false);

  QGridLayout * breaksLayout = new QGridLayout(grpAxesBreaks);
  chkBreakDecoration = new QCheckBox(tr("Draw Break &Decoration"));
  breaksLayout->addWidget(chkBreakDecoration, 0, 1);

  breaksLayout->addWidget(new QLabel(tr("From")), 1, 0);
  dspnBreakStart = new DoubleSpinBox();
  dspnBreakStart->setLocale(d_app->locale());
  dspnBreakStart->setDecimals(d_app->d_decimal_digits);
  breaksLayout->addWidget(dspnBreakStart, 1, 1);

  breaksLayout->addWidget(new QLabel(tr("To")), 2, 0);
  dspnBreakEnd = new DoubleSpinBox();
  dspnBreakEnd->setLocale(d_app->locale());
  dspnBreakEnd->setDecimals(d_app->d_decimal_digits);
  breaksLayout->addWidget(dspnBreakEnd, 2, 1);

  breaksLayout->addWidget(new QLabel(tr("Position")), 3, 0);
  spnBreakPosition = new QSpinBox();
  spnBreakPosition->setSuffix(" (" + tr("% of Axis Length") + ")");
  breaksLayout->addWidget(spnBreakPosition, 3, 1);

  breaksLayout->addWidget(new QLabel(tr("Width")), 4, 0);
  spnBreakWidth = new QSpinBox();
  spnBreakWidth->setSuffix(" (" + tr("pixels") + ")");
  breaksLayout->addWidget(spnBreakWidth, 4, 1);

  chkLog10AfterBreak = new QCheckBox(tr("&Log10 Scale After Break"));
  breaksLayout->addWidget(chkLog10AfterBreak, 0, 3);

  breaksLayout->addWidget(new QLabel(tr("Step Before Break")), 1, 2);
  dspnStepBeforeBreak = new DoubleSpinBox();
  dspnStepBeforeBreak->setMinimum(0.0);
  dspnStepBeforeBreak->setSpecialValueText(tr("Guess"));
  dspnStepBeforeBreak->setLocale(d_app->locale());
  dspnStepBeforeBreak->setDecimals(d_app->d_decimal_digits);
  breaksLayout->addWidget(dspnStepBeforeBreak, 1, 3);

  breaksLayout->addWidget(new QLabel(tr("Step After Break")), 2, 2);
  dspnStepAfterBreak = new DoubleSpinBox();
  dspnStepAfterBreak->setMinimum(0.0);
  dspnStepAfterBreak->setSpecialValueText(tr("Guess"));
  dspnStepAfterBreak->setLocale(d_app->locale());
  dspnStepAfterBreak->setDecimals(d_app->d_decimal_digits);
  breaksLayout->addWidget(dspnStepAfterBreak, 2, 3);

  breaksLayout->addWidget(new QLabel(tr("Minor Ticks Before")), 3, 2);
  cmbMinorTicksBeforeBreak = new QComboBox();
  cmbMinorTicksBeforeBreak->setEditable(true);
  cmbMinorTicksBeforeBreak->addItems(QStringList()<<"0"<<"1"<<"4"<<"9"<<"14"<<"19");
  breaksLayout->addWidget(cmbMinorTicksBeforeBreak, 3, 3);

  breaksLayout->addWidget(new QLabel(tr("Minor Ticks After")), 4, 2);
  cmbMinorTicksAfterBreak  = new QComboBox();
  cmbMinorTicksAfterBreak->setEditable(true);
  cmbMinorTicksAfterBreak->addItems(QStringList()<<"0"<<"1"<<"4"<<"9"<<"14"<<"19");
  breaksLayout->addWidget(cmbMinorTicksAfterBreak, 4, 3);

  QGroupBox *rightBox = new QGroupBox(QString());
  QGridLayout *rightLayout = new QGridLayout(rightBox);

  QWidget * stepWidget = new QWidget();
  QHBoxLayout * stepWidgetLayout = new QHBoxLayout( stepWidget );
  stepWidgetLayout->setMargin(0);

  chkStep = new QCheckBox(tr("Step"));
  chkStep->setChecked(true);
  rightLayout->addWidget( chkStep, 0, 0 );

  dspnStep = new DoubleSpinBox();
  dspnStep->setMinimum(0.0);
  dspnStep->setLocale(d_app->locale());
  dspnStep->setDecimals(d_app->d_decimal_digits);
  stepWidgetLayout->addWidget(dspnStep);

  cmbUnit = new QComboBox();
  cmbUnit->hide();
  stepWidgetLayout->addWidget( cmbUnit );

  rightLayout->addWidget( stepWidget, 0, 1 );

  chkMajor = new QCheckBox();
  chkMajor->setText( tr( "Major Ticks" ) );
  rightLayout->addWidget( chkMajor, 1, 0);

  spnMajorValue = new QSpinBox();
  spnMajorValue->setDisabled(true);
  rightLayout->addWidget( spnMajorValue, 1, 1);

  lblMinorBox = new QLabel( tr( "Minor Ticks" ));
  rightLayout->addWidget( lblMinorBox, 2, 0);

  cmbMinorValue = new QComboBox();
  cmbMinorValue->setEditable(true);
  cmbMinorValue->addItems(QStringList()<<"0"<<"1"<<"4"<<"9"<<"14"<<"19");
  rightLayout->addWidget( cmbMinorValue, 2, 1);

  rightLayout->setRowStretch( 3, 1 );

  QHBoxLayout* hl = new QHBoxLayout();
  hl->addWidget(middleBox);
  hl->addWidget(rightBox);

  QVBoxLayout* vl = new QVBoxLayout(this);
  vl->addLayout(hl);
  vl->addWidget(grpAxesBreaks);

  //// this is wrong and shouldn't happen i'll have to see what update plot is doing
  //connect(btnInvert,SIGNAL(clicked()), this, SLOT(updatePlot()));
  ////
  ////these need moved to the new object
  connect(boxScaleType,SIGNAL(activated(int)), this, SLOT(updateMinorTicksList(int)));
    //btnstep and btn major need changed to radiobuttons
    connect(btnStep,SIGNAL(clicked()), this, SLOT(stepEnabled()));
    connect(btnMajor,SIGNAL(clicked()), this, SLOT(stepDisabled()));
    //
  connect(boxEnd, SIGNAL(valueChanged(double)), this, SLOT(endvalueChanged(double)));
  connect(boxStart, SIGNAL(valueChanged(double)), this, SLOT(startvalueChanged(double)));
  ////

  //QVBoxLayout *main_layout = new QVBoxLayout(this);
  //main_layout->addLayout(vl);
}
ScaleAxisDetails::~ScaleAxisDetails()
{

}
