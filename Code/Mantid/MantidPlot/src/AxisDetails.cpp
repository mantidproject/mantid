//---------------------------
// Includes
//--------------------------

#include "AxisDetails.h"
#include "ApplicationWindow.h"
#include "DoubleSpinBox.h"
//#include <qwt_scale_widget.h>
//#include <qwt_plot.h>
#include "qwt_compat.h"
#include "Plot.h"
#include "plot2D/ScaleEngine.h"

#include <QWidget>
#include <QSpinBox>
#include <QCheckBox>
#include <QRadioButton>
#include <QGroupBox>
#include <QComboBox>
#include <QLabel>
#include <QDateTimeEdit>
#include <QTimeEdit>
#include <QLayout>

#include <QDate>
#include <QList>
#include <QListWidget>
#include <QVector>

AxisAxisDetails::AxisAxisDetails(QWidget *parent) :
    QWidget(parent)
{

  QHBoxLayout * topLayout = new QHBoxLayout();

  boxShowAxis = new QCheckBox(tr("Show"));
  boxShowAxis->setChecked(true);
  topLayout->addWidget(boxShowAxis);

  labelBox = new QGroupBox(tr("Title"));
  topLayout->addWidget(labelBox);

  QVBoxLayout *labelBoxLayout = new QVBoxLayout(labelBox);
  labelBoxLayout->setSpacing(2);

  boxTitle = new QTextEdit();
  boxTitle->setTextFormat(Qt::PlainText);
  QFontMetrics metrics(this->font());
  boxTitle->setMaximumHeight(3 * metrics.height());
  labelBoxLayout->addWidget(boxTitle);

  QHBoxLayout *hl = new QHBoxLayout();
  hl->setMargin(0);
  hl->setSpacing(2);
  buttonLabelFont = new QPushButton(tr("&Font"));
  hl->addWidget(buttonLabelFont);

  formatButtons = new TextFormatButtons(boxTitle, TextFormatButtons::AxisLabel);
  hl->addWidget(formatButtons);
  hl->addStretch();

  boxTitle->setMaximumWidth(buttonLabelFont->width() + formatButtons->width());
  labelBoxLayout->addLayout(hl);

  QHBoxLayout * bottomLayout = new QHBoxLayout();

  QGroupBox *leftBox = new QGroupBox(QString());
  bottomLayout->addWidget(leftBox);
  QGridLayout * leftBoxLayout = new QGridLayout(leftBox);

  leftBoxLayout->addWidget(new QLabel(tr("Type")), 0, 0);

  boxAxisType = new QComboBox();
  boxAxisType->addItem(tr("Numeric"));
  boxAxisType->addItem(tr("Text from table"));
  boxAxisType->addItem(tr("Day of the week"));
  boxAxisType->addItem(tr("Month"));
  boxAxisType->addItem(tr("Time"));
  boxAxisType->addItem(tr("Date"));
  boxAxisType->addItem(tr("Column Headings"));
  leftBoxLayout->addWidget(boxAxisType, 0, 1);

  leftBoxLayout->addWidget(new QLabel(tr("Font")), 1, 0);

  btnAxesFont = new QPushButton();
  btnAxesFont->setText(tr("Axis &Font"));
  leftBoxLayout->addWidget(btnAxesFont, 1, 1);

  leftBoxLayout->addWidget(new QLabel(tr("Color")), 2, 0);
  boxAxisColor = new ColorButton();
  leftBoxLayout->addWidget(boxAxisColor, 2, 1);

  leftBoxLayout->addWidget(new QLabel(tr("Major Ticks")), 3, 0);

  boxMajorTicksType = new QComboBox();
  boxMajorTicksType->addItem(tr("None"));
  boxMajorTicksType->addItem(tr("Out"));
  boxMajorTicksType->addItem(tr("In & Out"));
  boxMajorTicksType->addItem(tr("In"));
  leftBoxLayout->addWidget(boxMajorTicksType, 3, 1);

  leftBoxLayout->addWidget(new QLabel(tr("Minor Ticks")), 4, 0);

  boxMinorTicksType = new QComboBox();
  boxMinorTicksType->addItem(tr("None"));
  boxMinorTicksType->addItem(tr("Out"));
  boxMinorTicksType->addItem(tr("In & Out"));
  boxMinorTicksType->addItem(tr("In"));
  leftBoxLayout->addWidget(boxMinorTicksType, 4, 1);

  leftBoxLayout->addWidget(new QLabel(tr("Stand-off")), 5, 0);
  boxBaseline = new QSpinBox();
  boxBaseline->setRange(0, 1000);
  leftBoxLayout->addWidget(boxBaseline);

  boxShowLabels = new QGroupBox(tr("Show Labels"));
  boxShowLabels->setCheckable(true);
  boxShowLabels->setChecked(true);

  bottomLayout->addWidget(boxShowLabels);
  QGridLayout *rightBoxLayout = new QGridLayout(boxShowLabels);

  label1 = new QLabel(tr("Column"));
  rightBoxLayout->addWidget(label1, 0, 0);

  boxColName = new QComboBox();
  rightBoxLayout->addWidget(boxColName, 0, 1);

  labelTable = new QLabel(tr("Table"));
  rightBoxLayout->addWidget(labelTable, 1, 0);

  boxTableName = new QComboBox();
  boxTableName->insertStringList(tablesList);
  boxColName->insertStringList(d_app->columnsList(Table::All));
  rightBoxLayout->addWidget(boxTableName, 1, 1);

  label2 = new QLabel(tr("Format"));
  rightBoxLayout->addWidget(label2, 2, 0);

  boxFormat = new QComboBox();
  boxFormat->setDuplicatesEnabled(false);
  rightBoxLayout->addWidget(boxFormat, 2, 1);

  label3 = new QLabel(tr("Precision"));
  rightBoxLayout->addWidget(label3, 3, 0);
  boxPrecision = new QSpinBox();
  boxPrecision->setRange(0, 10);
  rightBoxLayout->addWidget(boxPrecision, 3, 1);

  rightBoxLayout->addWidget(new QLabel(tr("Angle")), 4, 0);

  boxAngle = new QSpinBox();
  boxAngle->setRange(-90, 90);
  boxAngle->setSingleStep(5);
  rightBoxLayout->addWidget(boxAngle, 4, 1);

  rightBoxLayout->addWidget(new QLabel(tr("Color")), 5, 0);
  boxAxisNumColor = new ColorButton();
  rightBoxLayout->addWidget(boxAxisNumColor, 5, 1);

  boxShowFormula = new QCheckBox(tr("For&mula"));
  rightBoxLayout->addWidget(boxShowFormula, 6, 0);

  boxFormula = new QTextEdit();
  boxFormula->setTextFormat(Qt::PlainText);
  boxFormula->setMaximumHeight(3 * metrics.height());
  boxFormula->hide();
  rightBoxLayout->addWidget(boxFormula, 6, 1);
  rightBoxLayout->setRowStretch(7, 1);

  QVBoxLayout * rightLayout = new QVBoxLayout(this);
  rightLayout->addLayout(topLayout);
  rightLayout->addLayout(bottomLayout);
  rightLayout->addStretch(1);

}
AxisAxisDetails::~AxisAxisDetails()
{

}


ScaleAxisDetails::ScaleAxisDetails(ApplicationWindow* app, Graph* graph,
    int mappedaxis, QWidget *parent) :
    QWidget(parent)
{
  d_app = app;
  d_graph = graph;
  m_mappedaxis = mappedaxis;
  m_initialised = false;
  QGroupBox * middleBox = new QGroupBox(QString());
  QGridLayout * middleLayout = new QGridLayout(middleBox);

  middleLayout->addWidget(new QLabel(tr("From")), 0, 0);
  dspnStart = new DoubleSpinBox();
  dspnStart->setLocale(d_app->locale());
  dspnStart->setDecimals(d_app->d_decimal_digits);
  middleLayout->addWidget(dspnStart, 0, 1);

  dteStartDateTime = new QDateTimeEdit();
  dteStartDateTime->setCalendarPopup(true);
  middleLayout->addWidget(dteStartDateTime, 0, 1);
  dteStartDateTime->hide();

  timStartTime = new QTimeEdit();
  middleLayout->addWidget(timStartTime, 0, 1);
  timStartTime->hide();

  middleLayout->addWidget(new QLabel(tr("To")), 1, 0);
  dspnEnd = new DoubleSpinBox();
  dspnEnd->setLocale(d_app->locale());
  dspnEnd->setDecimals(d_app->d_decimal_digits);
  middleLayout->addWidget(dspnEnd, 1, 1);

  dteEndDateTime = new QDateTimeEdit();
  dteEndDateTime->setCalendarPopup(true);
  middleLayout->addWidget(dteEndDateTime, 1, 1);
  dteEndDateTime->hide();

  timEndTime = new QTimeEdit();
  middleLayout->addWidget(timEndTime, 1, 1);
  timEndTime->hide();

  lblScaleTypeLabel = new QLabel(tr("Type"));
  cmbScaleType = new QComboBox();
  cmbScaleType->addItem(tr("linear"));
  cmbScaleType->addItem(tr("logarithmic"));
  middleLayout->addWidget(lblScaleTypeLabel, 2, 0);
  middleLayout->addWidget(cmbScaleType, 2, 1);

  chkInvert = new QCheckBox();
  chkInvert->setText(tr("Inverted"));
  chkInvert->setChecked(false);
  middleLayout->addWidget(chkInvert, 3, 1);
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
  cmbMinorTicksBeforeBreak->addItems(
      QStringList() << "0" << "1" << "4" << "9" << "14" << "19");
  breaksLayout->addWidget(cmbMinorTicksBeforeBreak, 3, 3);

  breaksLayout->addWidget(new QLabel(tr("Minor Ticks After")), 4, 2);
  cmbMinorTicksAfterBreak = new QComboBox();
  cmbMinorTicksAfterBreak->setEditable(true);
  cmbMinorTicksAfterBreak->addItems(
      QStringList() << "0" << "1" << "4" << "9" << "14" << "19");
  breaksLayout->addWidget(cmbMinorTicksAfterBreak, 4, 3);

  QGroupBox *rightBox = new QGroupBox(QString());
  QGridLayout *rightLayout = new QGridLayout(rightBox);

  QWidget * stepWidget = new QWidget();
  QHBoxLayout * stepWidgetLayout = new QHBoxLayout(stepWidget);
  stepWidgetLayout->setMargin(0);

  radStep = new QRadioButton(tr("Step"));
  radStep->setChecked(true);
  rightLayout->addWidget(radStep, 0, 0);

  dspnStep = new DoubleSpinBox();
  dspnStep->setMinimum(0.0);
  dspnStep->setLocale(d_app->locale());
  dspnStep->setDecimals(d_app->d_decimal_digits);
  stepWidgetLayout->addWidget(dspnStep);

  cmbUnit = new QComboBox();
  cmbUnit->hide();
  stepWidgetLayout->addWidget(cmbUnit);

  rightLayout->addWidget(stepWidget, 0, 1);

  radMajor = new QRadioButton(tr("Major Ticks"));
  rightLayout->addWidget(radMajor, 1, 0);

  spnMajorValue = new QSpinBox();
  spnMajorValue->setDisabled(true);
  rightLayout->addWidget(spnMajorValue, 1, 1);

  lblMinorBox = new QLabel(tr("Minor Ticks"));
  rightLayout->addWidget(lblMinorBox, 2, 0);

  cmbMinorValue = new QComboBox();
  cmbMinorValue->setEditable(true);
  cmbMinorValue->addItems(
      QStringList() << "0" << "1" << "4" << "9" << "14" << "19");
  rightLayout->addWidget(cmbMinorValue, 2, 1);

  rightLayout->setRowStretch(3, 1);

  QHBoxLayout* hl = new QHBoxLayout();
  hl->addWidget(middleBox);
  hl->addWidget(rightBox);

  QVBoxLayout* vl = new QVBoxLayout(this);
  vl->addLayout(hl);
  vl->addWidget(grpAxesBreaks);

  //// these bypass Apply()
  //connect(chkInvert,SIGNAL(clicked()), this, SLOT(updatePlot()));
  //connect(cmbScaleType,SIGNAL(activated(int)), this, SLOT(updateMinorTicksList(int)));
  //connect(dspnEnd, SIGNAL(valueChanged(double)), this, SLOT(endvalueChanged(double)));
  //connect(dspnStart, SIGNAL(valueChanged(double)), this, SLOT(startvalueChanged(double)));
  ////
  connect(radStep, SIGNAL(clicked()), this, SLOT(radiosSwitched()));
  connect(radMajor, SIGNAL(clicked()), this, SLOT(radiosSwitched()));
  initWidgets();
}
ScaleAxisDetails::~ScaleAxisDetails()
{

}

void ScaleAxisDetails::radiosSwitched()
{

}

/**
 slot called when the  To spinbox value changed
 */
/*
 void ScaleAxisDetails::endvalueChanged(double endVal)
 {
 //these bypass apply
 (void) endVal;
 if(d_graph)
 d_graph->changeIntensity( true);
 }

 void ScaleAxisDetails::startvalueChanged(double startVal)
 {
 //is this even doing anything?
 (void) startVal;
 if(d_graph)
 d_graph->changeIntensity( true);
 }
 void ScaleAxisDetails::updateMinorTicksList(int scaleType)
 {
 //is this even doing anything?
 (void) scaleType;
 }
 */

void ScaleAxisDetails::initWidgets()
{
  if (m_initialised)
  {
    return;
  }
  else
  {
    Plot *d_plot = d_graph->plotWidget();
    const QwtScaleDiv *scDiv = d_plot->axisScaleDiv(m_mappedaxis);
    double start = QMIN(scDiv->lBound(), scDiv->hBound());
    double end = QMAX(scDiv->lBound(), scDiv->hBound());
    ScaleDraw::ScaleType type = d_graph->axisType(m_mappedaxis);
    if (type == ScaleDraw::Date)
    {
      ScaleDraw *sclDraw = dynamic_cast<ScaleDraw *>(d_plot->axisScaleDraw(
          m_mappedaxis));
      QDateTime origin = sclDraw->dateTimeOrigin();

      dspnStart->hide();
      timStartTime->hide();
      dteStartDateTime->show();
      dteStartDateTime->setDisplayFormat(sclDraw->format());
      dteStartDateTime->setDateTime(origin.addSecs((int) start));

      dspnEnd->hide();
      timEndTime->hide();
      dteEndDateTime->show();
      dteEndDateTime->setDisplayFormat(sclDraw->format());
      dteEndDateTime->setDateTime(origin.addSecs((int) end));

      cmbUnit->show();
      cmbUnit->insertItem(tr("days"));
      cmbUnit->insertItem(tr("weeks"));
      dspnStep->setValue(d_graph->axisStep(m_mappedaxis) / 86400.0);
      dspnStep->setSingleStep(1);
    }
    else if (type == ScaleDraw::Time)
    {
      ScaleDraw *sclDraw = dynamic_cast<ScaleDraw *>(d_plot->axisScaleDraw(
          m_mappedaxis));
      QTime origin = sclDraw->dateTimeOrigin().time();

      dspnStart->hide();
      dteStartDateTime->hide();
      timStartTime->show();
      timStartTime->setDisplayFormat(sclDraw->format());
      timStartTime->setTime(origin.addMSecs((int) start));

      dspnEnd->hide();
      dteEndDateTime->hide();
      timEndTime->show();
      timEndTime->setDisplayFormat(sclDraw->format());
      timEndTime->setTime(origin.addMSecs((int) end));

      cmbUnit->show();
      cmbUnit->insertItem(tr("millisec."));
      cmbUnit->insertItem(tr("sec."));
      cmbUnit->insertItem(tr("min."));
      cmbUnit->insertItem(tr("hours"));
      cmbUnit->setCurrentIndex(1);
      dspnStep->setValue(d_graph->axisStep(m_mappedaxis) / 1e3);
      dspnStep->setSingleStep(1000);
    }
    else
    {
      dspnStart->show();
      dspnStart->setValue(start);
      timStartTime->hide();
      dteStartDateTime->hide();
      dspnEnd->show();
      dspnEnd->setValue(end);
      timEndTime->hide();
      dteEndDateTime->hide();
      dspnStep->setValue(d_graph->axisStep(m_mappedaxis));
      dspnStep->setSingleStep(0.1);
    }

    double range = fabs(scDiv->range());
    QwtScaleEngine *qwtsc_engine = d_plot->axisScaleEngine(m_mappedaxis);
    ScaleEngine* sc_engine = dynamic_cast<ScaleEngine*>(qwtsc_engine);
    if (sc_engine)
    {
      if (sc_engine->axisBreakLeft() > -DBL_MAX)
        dspnBreakStart->setValue(sc_engine->axisBreakLeft());
      else
        dspnBreakStart->setValue(start + 0.25 * range);

      if (sc_engine->axisBreakRight() < DBL_MAX)
        dspnBreakEnd->setValue(sc_engine->axisBreakRight());
      else
        dspnBreakEnd->setValue(start + 0.75 * range);

      grpAxesBreaks->setChecked(sc_engine->hasBreak());

      spnBreakPosition->setValue(sc_engine->breakPosition());
      spnBreakWidth->setValue(sc_engine->breakWidth());
      dspnStepBeforeBreak->setValue(sc_engine->stepBeforeBreak());
      dspnStepAfterBreak->setValue(sc_engine->stepAfterBreak());

      QwtScaleTransformation::Type scale_type = sc_engine->type();
      cmbMinorTicksBeforeBreak->clear();
      if (scale_type == QwtScaleTransformation::Log10)
        cmbMinorTicksBeforeBreak->addItems(
            QStringList() << "0" << "2" << "4" << "8");
      else
        cmbMinorTicksBeforeBreak->addItems(
            QStringList() << "0" << "1" << "4" << "9" << "14" << "19");
      cmbMinorTicksBeforeBreak->setEditText(
          QString::number(sc_engine->minTicksBeforeBreak()));

      cmbMinorTicksAfterBreak->setEditText(
          QString::number(sc_engine->minTicksAfterBreak()));
      chkLog10AfterBreak->setChecked(sc_engine->log10ScaleAfterBreak());
      chkBreakDecoration->setChecked(sc_engine->hasBreakDecoration());
      chkInvert->setChecked(sc_engine->testAttribute(QwtScaleEngine::Inverted));
      cmbScaleType->setCurrentItem(scale_type);
      cmbMinorValue->clear();
      if (scale_type == QwtScaleTransformation::Log10)
        cmbMinorValue->addItems(QStringList() << "0" << "2" << "4" << "8");
      else
        cmbMinorValue->addItems(
            QStringList() << "0" << "1" << "4" << "9" << "14" << "19");

      cmbMinorValue->setEditText(
          QString::number(d_plot->axisMaxMinor(m_mappedaxis)));

      bool isColorMap = d_graph->isColorBarEnabled(m_mappedaxis);
      grpAxesBreaks->setEnabled(!isColorMap);
      if (isColorMap)
      {
        grpAxesBreaks->setChecked(false);
      }

    }
    else
    {
      grpAxesBreaks->setChecked(false);
      grpAxesBreaks->setEnabled(false);
    }

    QwtValueList lst = scDiv->ticks(QwtScaleDiv::MajorTick);
    spnMajorValue->setValue(lst.count());

    if (d_graph->axisStep(m_mappedaxis) != 0.0)
    {
      radStep->setChecked(true);
      dspnStep->setEnabled(true);
      cmbUnit->setEnabled(true);

      radMajor->setChecked(false);
      spnMajorValue->setEnabled(false);
    }
    else
    {
      radStep->setChecked(false);
      dspnStep->setEnabled(false);
      cmbUnit->setEnabled(false);
      radMajor->setChecked(true);
      spnMajorValue->setEnabled(true);
    }
    m_initialised = true;
  }
}