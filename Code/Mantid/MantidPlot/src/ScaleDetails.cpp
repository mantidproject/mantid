//---------------------------
// Includes
//--------------------------

#include "ScaleDetails.h"
#include "ApplicationWindow.h"
#include "DoubleSpinBox.h"
#include <qwt_scale_widget.h>
//#include <qwt_plot.h>
#include "qwt_compat.h"
#include "Plot.h"
#include "plot2D/ScaleEngine.h"

#include <QWidget>
#include <QSpinBox>
#include <QCheckBox>
#include <QComboBox>
#include <QGroupBox>
#include <QRadioButton>
#include <QLabel>
#include <QDateTimeEdit>
#include <QTimeEdit>
#include <QLayout>
#include <QDate>
#include <QList>
#include <QListWidget>
#include <QVector>
#include <QTextEdit>
#include <QPushButton>
#include <QGridLayout>
#include <QMessageBox>
#include <TextFormatButtons.h>
#include <ColorButton.h>
#include <QFontDialog>

ScaleDetails::ScaleDetails(ApplicationWindow* app, Graph* graph, int mappedaxis, QWidget *parent) : QWidget(parent)
{
  d_app = app;
  d_graph = graph;
  m_mappedaxis = mappedaxis;
  m_initialised = false;
  m_modified = false;

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

  connect(radStep, SIGNAL(clicked()), this, SLOT(radiosSwitched()));
  connect(radMajor, SIGNAL(clicked()), this, SLOT(radiosSwitched()));

  initWidgets();
}

ScaleDetails::~ScaleDetails()
{

}

void ScaleDetails::initWidgets()
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
      ScaleDraw *sclDraw = dynamic_cast<ScaleDraw *>(d_plot->axisScaleDraw(m_mappedaxis));
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
      ScaleDraw *sclDraw = dynamic_cast<ScaleDraw *>(d_plot->axisScaleDraw(m_mappedaxis));
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
      {
        dspnBreakStart->setValue(sc_engine->axisBreakLeft());
      }
      else
      {
        dspnBreakStart->setValue(start + 0.25 * range);
      }

      if (sc_engine->axisBreakRight() < DBL_MAX)
      {
        dspnBreakEnd->setValue(sc_engine->axisBreakRight());
      }
      else
      {
        dspnBreakEnd->setValue(start + 0.75 * range);
      }
      grpAxesBreaks->setChecked(sc_engine->hasBreak());

      spnBreakPosition->setValue(sc_engine->breakPosition());
      spnBreakWidth->setValue(sc_engine->breakWidth());
      dspnStepBeforeBreak->setValue(sc_engine->stepBeforeBreak());
      dspnStepAfterBreak->setValue(sc_engine->stepAfterBreak());

      QwtScaleTransformation::Type scale_type = sc_engine->type();
      cmbMinorTicksBeforeBreak->clear();
      if (scale_type == QwtScaleTransformation::Log10)
      {
        cmbMinorTicksBeforeBreak->addItems(QStringList() << "0" << "2" << "4" << "8");
      }
      else
      {
        cmbMinorTicksBeforeBreak->addItems(QStringList() << "0" << "1" << "4" << "9" << "14" << "19");
      }
      cmbMinorTicksBeforeBreak->setEditText(QString::number(sc_engine->minTicksBeforeBreak()));

      cmbMinorTicksAfterBreak->setEditText(QString::number(sc_engine->minTicksAfterBreak()));
      chkLog10AfterBreak->setChecked(sc_engine->log10ScaleAfterBreak());
      chkBreakDecoration->setChecked(sc_engine->hasBreakDecoration());
      chkInvert->setChecked(sc_engine->testAttribute(QwtScaleEngine::Inverted));
      cmbScaleType->setCurrentItem(scale_type);
      cmbMinorValue->clear();
      if (scale_type == QwtScaleTransformation::Log10)
      {
        cmbMinorValue->addItems(QStringList() << "0" << "2" << "4" << "8");
      }
      else
      {
        cmbMinorValue->addItems(QStringList() << "0" << "1" << "4" << "9" << "14" << "19");
      }
      cmbMinorValue->setEditText(QString::number(d_plot->axisMaxMinor(m_mappedaxis)));

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

    checkstep();

    connect(grpAxesBreaks,SIGNAL(clicked()), this, SLOT(setModified()));
    connect(chkInvert,SIGNAL(clicked()), this, SLOT(setModified()));
    connect(chkLog10AfterBreak,SIGNAL(clicked()), this, SLOT(setModified()));
    connect(chkBreakDecoration,SIGNAL(clicked()), this, SLOT(setModified()));
    connect(radStep,SIGNAL(clicked()), this, SLOT(setModified()));
    connect(radMajor,SIGNAL(clicked()), this, SLOT(setModified()));
    connect(cmbMinorTicksBeforeBreak,SIGNAL(currentIndexChanged(int)), this, SLOT(setModified()));
    connect(cmbMinorTicksAfterBreak,SIGNAL(currentIndexChanged(int)), this, SLOT(setModified()));
    connect(cmbMinorValue,SIGNAL(currentIndexChanged(int)), this, SLOT(setModified()));
    connect(cmbUnit,SIGNAL(currentIndexChanged(int)), this, SLOT(setModified()));
    connect(cmbScaleType,SIGNAL(currentIndexChanged(int)), this, SLOT(setModified()));
    connect(dspnEnd, SIGNAL(valueChanged(double)), this, SLOT(setModified()));
    connect(dspnStart, SIGNAL(valueChanged(double)), this, SLOT(setModified()));
    connect(dspnStep, SIGNAL(valueChanged(double)), this, SLOT(setModified()));
    connect(dspnBreakStart, SIGNAL(valueChanged(double)), this, SLOT(setModified()));
    connect(dspnStepBeforeBreak, SIGNAL(valueChanged(double)), this, SLOT(setModified()));
    connect(dspnStepAfterBreak, SIGNAL(valueChanged(double)), this, SLOT(setModified()));
    connect(dspnBreakEnd, SIGNAL(valueChanged(double)), this, SLOT(setModified()));
    connect(spnMajorValue, SIGNAL(valueChanged(int)), this, SLOT(setModified()));
    connect(spnBreakPosition, SIGNAL(valueChanged(int)), this, SLOT(setModified()));
    connect(spnBreakWidth, SIGNAL(valueChanged(int)), this, SLOT(setModified()));
    connect(dteStartDateTime, SIGNAL(dateTimeChanged(QDateTime)), this, SLOT(setModified()));
    connect(dteStartDateTime, SIGNAL(dateChanged(QDate)), this, SLOT(setModified()));
    connect(dteStartDateTime, SIGNAL(timeChanged(QTime)), this, SLOT(setModified()));
    connect(dteEndDateTime, SIGNAL(dateTimeChanged(QDateTime)), this, SLOT(setModified()));
    connect(dteEndDateTime, SIGNAL(dateChanged(QDate)), this, SLOT(setModified()));
    connect(dteEndDateTime, SIGNAL(timeChanged(QTime)), this, SLOT(setModified()));
    connect(timStartTime, SIGNAL(dateTimeChanged(QDateTime)), this, SLOT(setModified()));
    connect(timStartTime, SIGNAL(dateChanged(QDate)), this, SLOT(setModified()));
    connect(timStartTime, SIGNAL(timeChanged(QTime)), this, SLOT(setModified()));
    connect(timEndTime, SIGNAL(dateTimeChanged(QDateTime)), this, SLOT(setModified()));
    connect(timEndTime, SIGNAL(dateChanged(QDate)), this, SLOT(setModified()));
    connect(timEndTime, SIGNAL(timeChanged(QTime)), this, SLOT(setModified()));

    m_initialised = true;
  }
}

bool ScaleDetails::valid()
{
  //  QMessageBox::warning(this,tr("MantidPlot - Error"), "Invalid option to set the 'From' greater than 'To' for the scale settings.\nOperation aborted! ");
  return m_initialised && d_app && d_graph && !(dspnStart->value() >= dspnEnd->value());
}

void ScaleDetails::apply()
{
  if (m_modified && valid())
  {
    //as the classes are separate now this may cause a problem as ideally i'd get this from the axis tab,
    //but at the moment there's nothing to cause a problem as the only other cases that are used are Date
    //and Time and Mantid doesn't support them in data yet i've been told
    ScaleDraw::ScaleType type = d_graph->axisType(m_mappedaxis);

    double start = 0.0, end = 0.0, step = 0.0, breakLeft = -DBL_MAX, breakRight = DBL_MAX;
    if (type == ScaleDraw::Date)
    {
      ScaleDraw *sclDraw = dynamic_cast<ScaleDraw *>(d_graph->plotWidget()->axisScaleDraw(m_mappedaxis));
      QDateTime origin = sclDraw->dateTimeOrigin();
      start = (double)origin.secsTo(dteStartDateTime->dateTime());
      end = (double)origin.secsTo(dteEndDateTime->dateTime());
    } 
    else if (type == ScaleDraw::Time)
    {
      ScaleDraw *sclDraw = dynamic_cast<ScaleDraw *>(d_graph->plotWidget()->axisScaleDraw(m_mappedaxis));
      QTime origin = sclDraw->dateTimeOrigin().time();
      start = (double)origin.msecsTo(timStartTime->time());
      end = (double)origin.msecsTo(timEndTime->time());
    } 
    else
    {
      start = dspnStart->value();
      end = dspnEnd->value();
    }

    if (radStep->isChecked())
    {
      step = dspnStep->value();
      if (type == ScaleDraw::Time)
      {
        switch (cmbUnit->currentIndex())
        {
        case 0:
          break;
        case 1:
          step *= 1e3;
          break;
        case 2:
          step *= 6e4;
          break;
        case 3:
          step *= 36e5;
          break;
        }
      }
      else if (type == ScaleDraw::Date)
      {
        switch (cmbUnit->currentIndex())
        {
        case 0:
          step *= 86400;
          break;
        case 1:
          step *= 604800;
          break;
        }
      }
    }

    if (grpAxesBreaks->isChecked())
    {
      breakLeft = qMin(dspnBreakStart->value(), dspnBreakEnd->value());
      breakRight = qMax(dspnBreakStart->value(), dspnBreakEnd->value());
    }
    d_graph->setScale(m_mappedaxis, start, end, step, spnMajorValue->value(), cmbMinorValue->currentText().toInt(),
      cmbScaleType->currentIndex(), chkInvert->isChecked(), breakLeft, breakRight, spnBreakPosition->value(),
      dspnStepBeforeBreak->value(),dspnStepAfterBreak->value(), cmbMinorTicksBeforeBreak->currentText().toInt(),
      cmbMinorTicksAfterBreak->currentText().toInt(), chkLog10AfterBreak->isChecked(), spnBreakWidth->value(), chkBreakDecoration->isChecked());
    d_graph->changeIntensity(true);
    d_graph->notifyChanges();
    m_modified = false;
  }
}

void ScaleDetails::setModified()
{
  m_modified = true;
}

void ScaleDetails::radiosSwitched()
{
  if (radStep->isChecked())
  {
    dspnStep->setEnabled(true);
    spnMajorValue->setEnabled(false);
  }
  else if (radStep->isChecked())
  {

    dspnStep->setEnabled(false);
    spnMajorValue->setEnabled(true);
  }
  else
  {
    radStep->setChecked(true);
    dspnStep->setEnabled(true);
    spnMajorValue->setEnabled(false);
  }
}

void ScaleDetails::checkstep()
{
  if (dspnStep->value() != 0.0)
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
}

