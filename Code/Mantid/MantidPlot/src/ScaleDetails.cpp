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

#include "MantidKernel/Logger.h"

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

namespace
{
  Mantid::Kernel::Logger g_log("ScaleDetails");
}

/** The constructor for a single set of widgets containing parameters for the scale of an axis.
*  @param app :: the containing application window
*  @param graph :: the graph the dialog is settign the options for
*  @param mappedaxis :: the QwtPlot::axis value that corresponds to this axis
*  @param parent :: the QWidget that acts as this widget's parent in the hierachy
*/
ScaleDetails::ScaleDetails(ApplicationWindow* app, Graph* graph, int mappedaxis, QWidget *parent) : QWidget(parent)
{
  m_app = app;
  m_graph = graph;
  m_mappedaxis = mappedaxis;
  m_initialised = false;
  m_modified = false;

  QGroupBox * middleBox = new QGroupBox(QString());
  QGridLayout * middleLayout = new QGridLayout(middleBox);

  m_lblStart = new QLabel(tr("From"));
  middleLayout->addWidget(m_lblStart, 0, 0);
  m_dspnStart = new DoubleSpinBox();
  m_dspnStart->setLocale(m_app->locale());
  m_dspnStart->setDecimals(m_app->d_graphing_digits);
  middleLayout->addWidget(m_dspnStart, 0, 1);
  connect(m_dspnStart, SIGNAL(valueChanged(double)), this, SLOT(recalcStepMin()));

  m_dteStartDateTime = new QDateTimeEdit();
  m_dteStartDateTime->setCalendarPopup(true);
  middleLayout->addWidget(m_dteStartDateTime, 0, 1);
  m_dteStartDateTime->hide();

  m_timStartTime = new QTimeEdit();
  middleLayout->addWidget(m_timStartTime, 0, 1);
  m_timStartTime->hide();

  m_lblEnd = new QLabel(tr("To"));
  middleLayout->addWidget(m_lblEnd, 1, 0);
  m_dspnEnd = new DoubleSpinBox();
  m_dspnEnd->setLocale(m_app->locale());
  m_dspnEnd->setDecimals(m_app->d_graphing_digits);
  middleLayout->addWidget(m_dspnEnd, 1, 1);
  connect(m_dspnStart, SIGNAL(valueChanged(double)), this, SLOT(recalcStepMin()));

  m_dteEndDateTime = new QDateTimeEdit();
  m_dteEndDateTime->setCalendarPopup(true);
  middleLayout->addWidget(m_dteEndDateTime, 1, 1);
  m_dteEndDateTime->hide();

  m_timEndTime = new QTimeEdit();
  middleLayout->addWidget(m_timEndTime, 1, 1);
  m_timEndTime->hide();

  m_lblScaleTypeLabel = new QLabel(tr("Type"));
  m_cmbScaleType = new QComboBox();
  m_cmbScaleType->addItem(tr("linear"));
  m_cmbScaleType->addItem(tr("logarithmic"));
  middleLayout->addWidget(m_lblScaleTypeLabel, 2, 0);
  middleLayout->addWidget(m_cmbScaleType, 2, 1);

  m_chkInvert = new QCheckBox();
  m_chkInvert->setText(tr("Inverted"));
  m_chkInvert->setChecked(false);
  middleLayout->addWidget(m_chkInvert, 3, 1);
  middleLayout->setRowStretch(4, 1);
  //TODO: This is disabled because the code handling it's value is commented out
  //Hence the value has no effect on the plot
  //THis was a UX issue in ticket #8198
  m_chkInvert->setVisible(false);

  m_grpAxesBreaks = new QGroupBox(tr("Show Axis &Break"));
  m_grpAxesBreaks->setCheckable(true);
  m_grpAxesBreaks->setChecked(false);

  QGridLayout * breaksLayout = new QGridLayout(m_grpAxesBreaks);
  m_chkBreakDecoration = new QCheckBox(tr("Draw Break &Decoration"));
  breaksLayout->addWidget(m_chkBreakDecoration, 0, 0, 1, 3);

  breaksLayout->addWidget(new QLabel(tr("From")), 1, 0);
  m_dspnBreakStart = new DoubleSpinBox();
  m_dspnBreakStart->setLocale(m_app->locale());
  m_dspnBreakStart->setDecimals(m_app->d_decimal_digits);
  breaksLayout->addWidget(m_dspnBreakStart, 1, 1);

  breaksLayout->addWidget(new QLabel(tr("To")), 2, 0);
  m_dspnBreakEnd = new DoubleSpinBox();
  m_dspnBreakEnd->setLocale(m_app->locale());
  m_dspnBreakEnd->setDecimals(m_app->d_decimal_digits);
  breaksLayout->addWidget(m_dspnBreakEnd, 2, 1);

  breaksLayout->addWidget(new QLabel(tr("Position")), 3, 0);
  m_spnBreakPosition = new QSpinBox();
  /* m_spnBreakPosition->setSuffix(" (" + tr("% of Axis Length") + ")"); */
  breaksLayout->addWidget(m_spnBreakPosition, 3, 1);
  breaksLayout->addWidget(new QLabel(tr("(% of Axis Length)")), 3, 2);

  breaksLayout->addWidget(new QLabel(tr("Width")), 4, 0);
  m_spnBreakWidth = new QSpinBox();
  /* m_spnBreakWidth->setSuffix(" (" + tr("pixels") + ")"); */
  breaksLayout->addWidget(m_spnBreakWidth, 4, 1);
  breaksLayout->addWidget(new QLabel(tr("(pixels)")), 4, 2);

  m_chkLog10AfterBreak = new QCheckBox(tr("&Log10 Scale After Break"));
  breaksLayout->addWidget(m_chkLog10AfterBreak, 0, 3);

  breaksLayout->addWidget(new QLabel(tr("Step Before Break")), 1, 3);
  m_dspnStepBeforeBreak = new DoubleSpinBox();
  m_dspnStepBeforeBreak->addSpecialTextMapping("Guess", 0.0);
  m_dspnStepBeforeBreak->addSpecialTextMapping("guess", 0.0);
  m_dspnStepBeforeBreak->addSpecialTextMapping("GUESS", 0.0);
  m_dspnStepBeforeBreak->setMinimum(0.0);
  m_dspnStepBeforeBreak->setSpecialValueText(tr("Guess"));
  m_dspnStepBeforeBreak->setLocale(m_app->locale());
  m_dspnStepBeforeBreak->setDecimals(m_app->d_decimal_digits);
  breaksLayout->addWidget(m_dspnStepBeforeBreak, 1, 4);

  breaksLayout->addWidget(new QLabel(tr("Step After Break")), 2, 3);
  m_dspnStepAfterBreak = new DoubleSpinBox();
  m_dspnStepAfterBreak->addSpecialTextMapping("Guess", 0.0);
  m_dspnStepAfterBreak->addSpecialTextMapping("guess", 0.0);
  m_dspnStepAfterBreak->addSpecialTextMapping("GUESS", 0.0);
  m_dspnStepAfterBreak->setMinimum(0.0);
  m_dspnStepAfterBreak->setSpecialValueText(tr("Guess"));
  m_dspnStepAfterBreak->setLocale(m_app->locale());
  m_dspnStepAfterBreak->setDecimals(m_app->d_decimal_digits);
  breaksLayout->addWidget(m_dspnStepAfterBreak, 2, 4);

  breaksLayout->addWidget(new QLabel(tr("Minor Ticks Before")), 3, 3);
  m_cmbMinorTicksBeforeBreak = new QComboBox();
  m_cmbMinorTicksBeforeBreak->setEditable(true);
  m_cmbMinorTicksBeforeBreak->addItems(
    QStringList() << "0" << "1" << "4" << "9" << "14" << "19");
  breaksLayout->addWidget(m_cmbMinorTicksBeforeBreak, 3, 4);

  breaksLayout->addWidget(new QLabel(tr("Minor Ticks After")), 4, 3);
  m_cmbMinorTicksAfterBreak = new QComboBox();
  m_cmbMinorTicksAfterBreak->setEditable(true);
  m_cmbMinorTicksAfterBreak->addItems(
    QStringList() << "0" << "1" << "4" << "9" << "14" << "19");
  breaksLayout->addWidget(m_cmbMinorTicksAfterBreak, 4, 4);

  QGroupBox *rightBox = new QGroupBox(QString());
  QGridLayout *rightLayout = new QGridLayout(rightBox);

  QWidget * stepWidget = new QWidget();
  QHBoxLayout * stepWidgetLayout = new QHBoxLayout(stepWidget);
  stepWidgetLayout->setMargin(0);

  m_radStep = new QRadioButton(tr("Step"));
  m_radStep->setChecked(true);
  rightLayout->addWidget(m_radStep, 0, 0);

  m_dspnStep = new DoubleSpinBox();
  m_dspnStep->setMinimum(0.0);
  m_dspnStep->setLocale(m_app->locale());
  m_dspnStep->setDecimals(m_app->d_decimal_digits);
  stepWidgetLayout->addWidget(m_dspnStep);

  m_cmbUnit = new QComboBox();
  m_cmbUnit->hide();
  stepWidgetLayout->addWidget(m_cmbUnit);

  rightLayout->addWidget(stepWidget, 0, 1);

  m_radMajor = new QRadioButton(tr("Max. Major Ticks"));
  rightLayout->addWidget(m_radMajor, 1, 0);

  m_spnMajorValue = new QSpinBox();
  m_spnMajorValue->setDisabled(true);
  m_spnMajorValue->setToolTip("Maximum number of major ticks which will be added to the axis.\nNote that less ticks may be added to preserve readability.");
  rightLayout->addWidget(m_spnMajorValue, 1, 1);

  m_lblMinorBox = new QLabel(tr("Max. Minor Ticks"));
  rightLayout->addWidget(m_lblMinorBox, 2, 0);

  m_cmbMinorValue = new QComboBox();
  m_cmbMinorValue->setEditable(true);
  m_cmbMinorValue->addItems(
    QStringList() << "0" << "1" << "4" << "9" << "14" << "19");
  m_cmbMinorValue->setToolTip("Maximum number of minor ticks which will be added to the axis.\nNote that less ticks may be added to preserve readability.");
  rightLayout->addWidget(m_cmbMinorValue, 2, 1);

  rightLayout->setRowStretch(3, 1);

  QHBoxLayout* hl = new QHBoxLayout();
  hl->addWidget(middleBox);
  hl->addWidget(rightBox);

  QVBoxLayout* vl = new QVBoxLayout(this);
  vl->addLayout(hl);
  vl->addWidget(m_grpAxesBreaks);

  connect(m_radStep, SIGNAL(clicked()), this, SLOT(radiosSwitched()));
  connect(m_radMajor, SIGNAL(clicked()), this, SLOT(radiosSwitched()));

  initWidgets();
  recalcStepMin();
}

ScaleDetails::~ScaleDetails()
{
}

/** Initialisation method. Sets up all widgets and variables not done in the constructor.
*
*/
void ScaleDetails::initWidgets()
{
  if (m_initialised)
  {
    return;
  }
  else
  {
    Plot *d_plot = m_graph->plotWidget();
    const QwtScaleDiv *scDiv = d_plot->axisScaleDiv(m_mappedaxis);
    double start = QMIN(scDiv->lBound(), scDiv->hBound());
    double end = QMAX(scDiv->lBound(), scDiv->hBound());
    ScaleDraw::ScaleType type = m_graph->axisType(m_mappedaxis);
    if (type == ScaleDraw::Date)
    {
      ScaleDraw *sclDraw = dynamic_cast<ScaleDraw *>(d_plot->axisScaleDraw(m_mappedaxis));
      QDateTime origin = sclDraw->dateTimeOrigin();

      m_dspnStart->hide();
      m_timStartTime->hide();
      m_dteStartDateTime->show();
      m_dteStartDateTime->setDisplayFormat(sclDraw->format());
      m_dteStartDateTime->setDateTime(origin.addSecs((int) start));

      m_dspnEnd->hide();
      m_timEndTime->hide();
      m_dteEndDateTime->show();
      m_dteEndDateTime->setDisplayFormat(sclDraw->format());
      m_dteEndDateTime->setDateTime(origin.addSecs((int) end));

      m_cmbUnit->show();
      m_cmbUnit->insertItem(tr("days"));
      m_cmbUnit->insertItem(tr("weeks"));
      m_dspnStep->setValue(m_graph->axisStep(m_mappedaxis) / 86400.0);
      m_dspnStep->setSingleStep(1);
    }
    else if (type == ScaleDraw::Time)
    {
      ScaleDraw *sclDraw = dynamic_cast<ScaleDraw *>(d_plot->axisScaleDraw(m_mappedaxis));
      QTime origin = sclDraw->dateTimeOrigin().time();

      m_dspnStart->hide();
      m_dteStartDateTime->hide();
      m_timStartTime->show();
      m_timStartTime->setDisplayFormat(sclDraw->format());
      m_timStartTime->setTime(origin.addMSecs((int) start));

      m_dspnEnd->hide();
      m_dteEndDateTime->hide();
      m_timEndTime->show();
      m_timEndTime->setDisplayFormat(sclDraw->format());
      m_timEndTime->setTime(origin.addMSecs((int) end));

      m_cmbUnit->show();
      m_cmbUnit->insertItem(tr("millisec."));
      m_cmbUnit->insertItem(tr("sec."));
      m_cmbUnit->insertItem(tr("min."));
      m_cmbUnit->insertItem(tr("hours"));
      m_cmbUnit->setCurrentIndex(1);
      m_dspnStep->setValue(m_graph->axisStep(m_mappedaxis) / 1e3);
      m_dspnStep->setSingleStep(1000);
    }
    else
    {
      m_dspnStart->show();
      m_dspnStart->setValue(start);
      m_timStartTime->hide();
      m_dteStartDateTime->hide();
      m_dspnEnd->show();
      m_dspnEnd->setValue(end);
      m_timEndTime->hide();
      m_dteEndDateTime->hide();
      m_dspnStep->setValue(m_graph->axisStep(m_mappedaxis));
      m_dspnStep->setSingleStep(0.1);
    }

    double range = fabs(scDiv->range());
    QwtScaleEngine *qwtsc_engine = d_plot->axisScaleEngine(m_mappedaxis);
    ScaleEngine* sc_engine = dynamic_cast<ScaleEngine*>(qwtsc_engine);
    if (sc_engine)
    {
      if (sc_engine->axisBreakLeft() > -DBL_MAX)
      {
        m_dspnBreakStart->setValue(sc_engine->axisBreakLeft());
      }
      else
      {
        m_dspnBreakStart->setValue(start + 0.25 * range);
      }

      if (sc_engine->axisBreakRight() < DBL_MAX)
      {
        m_dspnBreakEnd->setValue(sc_engine->axisBreakRight());
      }
      else
      {
        m_dspnBreakEnd->setValue(start + 0.75 * range);
      }
      m_grpAxesBreaks->setChecked(sc_engine->hasBreak());

      m_spnBreakPosition->setValue(sc_engine->breakPosition());
      m_spnBreakWidth->setValue(sc_engine->breakWidth());
      m_dspnStepBeforeBreak->setValue(sc_engine->stepBeforeBreak());
      m_dspnStepAfterBreak->setValue(sc_engine->stepAfterBreak());

      QwtScaleTransformation::Type scale_type = sc_engine->type();
      m_cmbMinorTicksBeforeBreak->clear();
      if (scale_type == QwtScaleTransformation::Log10)
      {
        m_cmbMinorTicksBeforeBreak->addItems(QStringList() << "0" << "2" << "4" << "8");
      }
      else
      {
        m_cmbMinorTicksBeforeBreak->addItems(QStringList() << "0" << "1" << "4" << "9" << "14" << "19");
      }
      m_cmbMinorTicksBeforeBreak->setEditText(QString::number(sc_engine->minTicksBeforeBreak()));

      m_cmbMinorTicksAfterBreak->setEditText(QString::number(sc_engine->minTicksAfterBreak()));
      m_chkLog10AfterBreak->setChecked(sc_engine->log10ScaleAfterBreak());
      m_chkBreakDecoration->setChecked(sc_engine->hasBreakDecoration());
      m_chkInvert->setChecked(sc_engine->testAttribute(QwtScaleEngine::Inverted));
      m_cmbScaleType->setCurrentItem(scale_type);
      m_cmbMinorValue->clear();
      if (scale_type == QwtScaleTransformation::Log10)
      {
        m_cmbMinorValue->addItems(QStringList() << "0" << "2" << "4" << "8");
      }
      else
      {
        m_cmbMinorValue->addItems(QStringList() << "0" << "1" << "4" << "9" << "14" << "19");
      }
      m_cmbMinorValue->setEditText(QString::number(d_plot->axisMaxMinor(m_mappedaxis)));

      bool isColorMap = m_graph->isColorBarEnabled(m_mappedaxis);
      m_grpAxesBreaks->setEnabled(!isColorMap);
      if (isColorMap)
      {
        m_grpAxesBreaks->setChecked(false);
      }
    }
    else
    {
      m_grpAxesBreaks->setChecked(false);
      m_grpAxesBreaks->setEnabled(false);
    }

    QwtValueList lst = scDiv->ticks(QwtScaleDiv::MajorTick);
    m_spnMajorValue->setValue(lst.count());

    checkstep();

    connect(m_grpAxesBreaks,SIGNAL(clicked()), this, SLOT(setModified()));
    connect(m_chkInvert,SIGNAL(clicked()), this, SLOT(setModified()));
    connect(m_chkLog10AfterBreak,SIGNAL(clicked()), this, SLOT(setModified()));
    connect(m_chkBreakDecoration,SIGNAL(clicked()), this, SLOT(setModified()));
    connect(m_radStep,SIGNAL(clicked()), this, SLOT(setModified()));
    connect(m_radMajor,SIGNAL(clicked()), this, SLOT(setModified()));
    connect(m_cmbMinorTicksBeforeBreak,SIGNAL(currentIndexChanged(int)), this, SLOT(setModified()));
    connect(m_cmbMinorTicksAfterBreak,SIGNAL(currentIndexChanged(int)), this, SLOT(setModified()));
    connect(m_cmbMinorValue,SIGNAL(currentIndexChanged(int)), this, SLOT(setModified()));
    connect(m_cmbUnit,SIGNAL(currentIndexChanged(int)), this, SLOT(setModified()));
    connect(m_cmbScaleType,SIGNAL(currentIndexChanged(int)), this, SLOT(setModified()));
    connect(m_dspnEnd, SIGNAL(valueChanged(double)), this, SLOT(setModified()));
    connect(m_dspnStart, SIGNAL(valueChanged(double)), this, SLOT(setModified()));
    connect(m_dspnStep, SIGNAL(valueChanged(double)), this, SLOT(setModified()));
    connect(m_dspnBreakStart, SIGNAL(valueChanged(double)), this, SLOT(setModified()));
    connect(m_dspnStepBeforeBreak, SIGNAL(valueChanged(double)), this, SLOT(setModified()));
    connect(m_dspnStepAfterBreak, SIGNAL(valueChanged(double)), this, SLOT(setModified()));
    connect(m_dspnBreakEnd, SIGNAL(valueChanged(double)), this, SLOT(setModified()));
    connect(m_spnMajorValue, SIGNAL(valueChanged(int)), this, SLOT(setModified()));
    connect(m_spnBreakPosition, SIGNAL(valueChanged(int)), this, SLOT(setModified()));
    connect(m_spnBreakWidth, SIGNAL(valueChanged(int)), this, SLOT(setModified()));
    connect(m_dteStartDateTime, SIGNAL(dateTimeChanged(QDateTime)), this, SLOT(setModified()));
    connect(m_dteStartDateTime, SIGNAL(dateChanged(QDate)), this, SLOT(setModified()));
    connect(m_dteStartDateTime, SIGNAL(timeChanged(QTime)), this, SLOT(setModified()));
    connect(m_dteEndDateTime, SIGNAL(dateTimeChanged(QDateTime)), this, SLOT(setModified()));
    connect(m_dteEndDateTime, SIGNAL(dateChanged(QDate)), this, SLOT(setModified()));
    connect(m_dteEndDateTime, SIGNAL(timeChanged(QTime)), this, SLOT(setModified()));
    connect(m_timStartTime, SIGNAL(dateTimeChanged(QDateTime)), this, SLOT(setModified()));
    connect(m_timStartTime, SIGNAL(dateChanged(QDate)), this, SLOT(setModified()));
    connect(m_timStartTime, SIGNAL(timeChanged(QTime)), this, SLOT(setModified()));
    connect(m_timEndTime, SIGNAL(dateTimeChanged(QDateTime)), this, SLOT(setModified()));
    connect(m_timEndTime, SIGNAL(dateChanged(QDate)), this, SLOT(setModified()));
    connect(m_timEndTime, SIGNAL(timeChanged(QTime)), this, SLOT(setModified()));

    m_initialised = true;
  }
}

/**
 * Enabled or disables the scale controls for the axis.
 *
 * @param enabled If the controls should be enabled or disabled
 */
void ScaleDetails::axisEnabled(bool enabled)
{
  //Stuff the is always enabled when the axis is shown
  m_dspnStart->setEnabled(enabled);
  m_dspnEnd->setEnabled(enabled);
  m_cmbScaleType->setEnabled(enabled);
  m_chkInvert->setEnabled(enabled);
  m_radStep->setEnabled(enabled);
  m_radMajor->setEnabled(enabled);
  m_grpAxesBreaks->setEnabled(enabled);
  m_cmbMinorValue->setEnabled(enabled);
  m_lblStart->setEnabled(enabled);
  m_lblEnd->setEnabled(enabled);
  m_lblMinorBox->setEnabled(enabled);
  m_lblScaleTypeLabel->setEnabled(enabled);

  //Stuff that is only enabled when the axis is shown and axis breaks are enabled
  bool enableAxisBreaks = enabled && m_grpAxesBreaks->isChecked();
  m_dspnBreakStart->setEnabled(enableAxisBreaks);
  m_dspnBreakEnd->setEnabled(enableAxisBreaks);
  m_spnBreakPosition->setEnabled(enableAxisBreaks);
  m_spnBreakWidth->setEnabled(enableAxisBreaks);
  m_dspnStepBeforeBreak->setEnabled(enableAxisBreaks);
  m_dspnStepAfterBreak->setEnabled(enableAxisBreaks);
  m_cmbMinorTicksBeforeBreak->setEnabled(enableAxisBreaks);
  m_cmbMinorTicksAfterBreak->setEnabled(enableAxisBreaks);

  bool majorTicks = enabled && m_radMajor->isChecked();
  m_spnMajorValue->setEnabled(majorTicks);

  bool minorTicks = enabled && m_radStep->isChecked();
  m_dspnStep->setEnabled(minorTicks);
}

/** Checks to see if this axis has valid parameters
*
*/
bool ScaleDetails::valid()
{
  if(m_radStep->isChecked() && (m_dspnStep->value() < m_dspnStep->getMinimum()))
    return false;

  return  m_initialised &&
          m_app &&
          m_graph &&
          !(m_dspnStart->value() >= m_dspnEnd->value());
}

/** Applies this axis' parameters to the graph
*
*/
void ScaleDetails::apply()
{
  if (m_modified && valid())
  {
    //as the classes are separate now this may cause a problem as ideally i'd get this from the axis tab,
    //but at the moment there's nothing to cause a problem as the only other cases that are used are Date
    //and Time and Mantid doesn't support them in data yet i've been told
    ScaleDraw::ScaleType type = m_graph->axisType(m_mappedaxis);

    double start = 0.0, end = 0.0, step = 0.0, breakLeft = -DBL_MAX, breakRight = DBL_MAX;
    if (type == ScaleDraw::Date)
    {
      ScaleDraw *sclDraw = dynamic_cast<ScaleDraw *>(m_graph->plotWidget()->axisScaleDraw(m_mappedaxis));
      QDateTime origin = sclDraw->dateTimeOrigin();
      start = (double)origin.secsTo(m_dteStartDateTime->dateTime());
      end = (double)origin.secsTo(m_dteEndDateTime->dateTime());
    } 
    else if (type == ScaleDraw::Time)
    {
      ScaleDraw *sclDraw = dynamic_cast<ScaleDraw *>(m_graph->plotWidget()->axisScaleDraw(m_mappedaxis));
      QTime origin = sclDraw->dateTimeOrigin().time();
      start = (double)origin.msecsTo(m_timStartTime->time());
      end = (double)origin.msecsTo(m_timEndTime->time());
    } 
    else
    {
      start = m_dspnStart->value();
      end = m_dspnEnd->value();
    }

    if (m_radStep->isChecked())
    {
      step = m_dspnStep->value();
      if (type == ScaleDraw::Time)
      {
        switch (m_cmbUnit->currentIndex())
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
        switch (m_cmbUnit->currentIndex())
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

    if (m_grpAxesBreaks->isChecked())
    {
      breakLeft = qMin(m_dspnBreakStart->value(), m_dspnBreakEnd->value());
      breakRight = qMax(m_dspnBreakStart->value(), m_dspnBreakEnd->value());
    }
    m_graph->setScale(m_mappedaxis, start, end, step, m_spnMajorValue->value(), m_cmbMinorValue->currentText().toInt(),
      m_cmbScaleType->currentIndex(), m_chkInvert->isChecked(), breakLeft, breakRight, m_spnBreakPosition->value(),
      m_dspnStepBeforeBreak->value(),m_dspnStepAfterBreak->value(), m_cmbMinorTicksBeforeBreak->currentText().toInt(),
      m_cmbMinorTicksAfterBreak->currentText().toInt(), m_chkLog10AfterBreak->isChecked(), m_spnBreakWidth->value(), m_chkBreakDecoration->isChecked());
    m_graph->changeIntensity(true);
    m_graph->notifyChanges();
    m_modified = false;
  }
}

/** Sets the modifed flag to true so that the changes may be applied.
*
*/
void ScaleDetails::setModified()
{
  m_modified = true;
}

/** enables and disables the appropriate field depending on the current radio button
*
*/
void ScaleDetails::radiosSwitched()
{
  if (m_radStep->isChecked())
  {
    m_dspnStep->setEnabled(true);
    m_spnMajorValue->setEnabled(false);
  }
  else if (m_radMajor->isChecked())
  {
    m_dspnStep->setEnabled(false);
    m_spnMajorValue->setEnabled(true);
  }
  else
  {
    m_radStep->setChecked(true);
    m_dspnStep->setEnabled(true);
    m_spnMajorValue->setEnabled(false);
  }
}

/** Enables or disables widgets corresponding to the current value of the step parameter.
*
*/
void ScaleDetails::checkstep()
{
  if (m_dspnStep->value() != 0.0)
  {
    m_radStep->setChecked(true);
    m_dspnStep->setEnabled(true);
    m_cmbUnit->setEnabled(true);

    m_radMajor->setChecked(false);
    m_spnMajorValue->setEnabled(false);
  }
  else
  {
    m_radStep->setChecked(false);
    m_dspnStep->setEnabled(false);
    m_cmbUnit->setEnabled(false);
    m_radMajor->setChecked(true);
    m_spnMajorValue->setEnabled(true);
  }
}

/**
 * Recalculates the minimum value allowed in step to stop too many labels being rendered
 */
void ScaleDetails::recalcStepMin()
{
  double range = m_dspnEnd->value() - m_dspnStart->value();
  double minStep = range / 20;
  m_dspnStep->setMinimum(minStep);
}
