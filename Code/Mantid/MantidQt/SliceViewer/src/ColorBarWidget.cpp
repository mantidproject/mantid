#include "MantidQtSliceViewer/ColorBarWidget.h"
#include "MantidQtSliceViewer/QScienceSpinBox.h"
#include "qwt_scale_div.h"
#include <iosfwd>
#include <iostream>
#include <qwt_scale_map.h>
#include <qwt_scale_widget.h>

//-------------------------------------------------------------------------------------------------
/** Constructor */
ColorBarWidget::ColorBarWidget(QWidget *parent)
: QWidget(parent)
{
  ui.setupUi(this);

  // Default values.
  m_colorMap = new QwtLinearColorMap(Qt::blue, Qt::red);
  m_min = 0;
  m_max = 1000;
  m_log = false;
  this->setDataRange(0, 1000);

  // Create and add the color bar
  m_colorBar = new QwtScaleWidget();
  ui.verticalLayout->insertWidget(2,m_colorBar, 1,0 );

  // Hook up signals
  QObject::connect(ui.checkLog, SIGNAL(stateChanged(int)), this, SLOT(changedLogState(int)));
  QObject::connect(ui.valMin, SIGNAL(editingFinished()), this, SLOT(changedMinimum()));
  QObject::connect(ui.valMax, SIGNAL(editingFinished()), this, SLOT(changedMaximum()));
  QObject::connect(ui.valMin, SIGNAL(valueChangedFromArrows()), this, SLOT(changedMinimum()));
  QObject::connect(ui.valMax, SIGNAL(valueChangedFromArrows()), this, SLOT(changedMaximum()));

  // Initial view
  this->update();
}


//-------------------------------------------------------------------------------------------------
/// @return the minimum value of the min of the color scale
double ColorBarWidget::getMinimum() const
{ return m_min; }

/// @return the maximum value of the max of the color scale
double ColorBarWidget::getMaximum() const
{ return m_max; }

/// @return true if the color scale is logarithmic.
bool ColorBarWidget::getLog() const
{ return m_log; }


//-------------------------------------------------------------------------------------------------
/** Change the color map shown
 *
 * @param colorMap
 */
void ColorBarWidget::setColorMap(QwtColorMap * colorMap)
{
  if (m_colorMap) delete m_colorMap;
  m_colorMap = colorMap;
  update();
}

//-------------------------------------------------------------------------------------------------
/** Adjust the steps of the spin boxes for log/linear mode */
void ColorBarWidget::setSpinBoxesSteps()
{
  double step = 1.1;
  if (m_log)
  {
    // Logarithmic color scale: move by logarithmic steps
    double logRange;
    if (m_rangeMin <= 0)
    {
      // Try to guess at a valid min range if 0 for log scale
      logRange = log10(m_rangeMax);
      if (logRange >= 3) m_rangeMin = 1;
      else if (logRange >= 0) m_rangeMin = 1e-3;
      // Default to 1/10000 of the max
      else m_rangeMin = pow(10., double(int(logRange))-4.);
    }
    logRange = log10(m_rangeMax) - log10(m_rangeMin);
    if (logRange > 6) logRange = 6;
    step = pow(10., logRange/100.);
  }
  else
  {
    // Linear scale
    // Round step that is between 1/100 to 1/1000)
    int exponent = int(log10(m_rangeMax)) - 2;
    step = pow(10., double(exponent));
  }

  ui.valMin->setMinimum( m_rangeMin );
  ui.valMin->setMaximum( m_rangeMax );
  ui.valMax->setMinimum( m_rangeMin );
  ui.valMax->setMaximum( m_rangeMax );

  ui.valMin->setSingleStep(step);
  ui.valMax->setSingleStep(step);
  int dec = 2;
  ui.valMin->setDecimals(dec);
  ui.valMax->setDecimals(dec);
}


//-------------------------------------------------------------------------------------------------
/** Set the range of values in the overall data (limits to selections)
 *
 * @param min
 * @param max
 */
void ColorBarWidget::setDataRange(double min, double max)
{
  m_rangeMin = min;
  m_rangeMax = max;
  setSpinBoxesSteps();
}

//-------------------------------------------------------------------------------------------------
/** Set the range of values viewed in the color bar
 *
 * @param min :: min value = start of the color map
 * @param max :: max value = end of the color map
 */
void ColorBarWidget::setViewRange(double min, double max)
{
  m_min = min;
  m_max = max;
  update();
}

//-------------------------------------------------------------------------------------------------
/** SLOT called when clicking the log button */
void ColorBarWidget::changedLogState(int log)
{
  this->setLog(log);
  emit changedColorRange(m_min,m_max,m_log);
}

//-------------------------------------------------------------------------------------------------
/** Set the color bar to use log scale
 *
 * @param log :: true to use log scale
 */
void ColorBarWidget::setLog(bool log)
{
  m_log = log;
  ui.checkLog->setChecked( m_log );
  ui.valMin->setLogSteps( m_log );
  ui.valMax->setLogSteps( m_log );
  setSpinBoxesSteps();
  update();
}

//-------------------------------------------------------------------------------------------------
/** SLOT called when minValue changes */
void ColorBarWidget::changedMinimum()
{
  m_min = ui.valMin->value();
  if (m_min > m_max)
  {
    m_max = m_min+0.001;
    update();
  }
  emit changedColorRange(m_min,m_max,m_log);
}

//-------------------------------------------------------------------------------------------------
/** SLOT called when maxValue changes */
void ColorBarWidget::changedMaximum()
{
  m_max = ui.valMax->value();
  if (m_max < m_min)
  {
    m_min = m_max-0.001;
    update();
  }
  emit changedColorRange(m_min,m_max,m_log);
}


//-------------------------------------------------------------------------------------------------
/** Update the widget when the color map is changed in any way */
void ColorBarWidget::update()
{
  m_colorBar->setColorBarEnabled(true);
  m_colorBar->setColorMap( QwtDoubleInterval(m_min, m_max), *m_colorMap);
  m_colorBar->setColorBarWidth(15);

  QwtScaleDiv scaleDiv;
  scaleDiv.setInterval(m_min, m_max);
  m_colorBar->setScaleDiv(new QwtScaleTransformation(QwtScaleTransformation::Linear), scaleDiv);

  ui.valMin->setValue( m_min );
  ui.valMax->setValue( m_max );

//  QList<double> ticks;
//  ticks.push_back(1);
//  ticks.push_back(2);
//  ticks.push_back(3);
//  ticks.push_back(4);
//  ticks.push_back(5);
//  scaleDiv.setTicks(QwtScaleDiv::MajorTick, ticks);

}


ColorBarWidget::~ColorBarWidget()
{

}
