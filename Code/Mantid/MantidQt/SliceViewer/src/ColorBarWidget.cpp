#include "MantidQtAPI/MantidColorMap.h"
#include "MantidQtSliceViewer/ColorBarWidget.h"
#include "MantidQtSliceViewer/QScienceSpinBox.h"
#include "qwt_scale_div.h"
#include <iosfwd>
#include <iostream>
#include <qwt_scale_map.h>
#include <qwt_scale_widget.h>
#include <QKeyEvent>
#include <qwt_plot.h>
#include <qwt_scale_engine.h>

namespace MantidQt
{
namespace SliceViewer
{

//-------------------------------------------------------------------------------------------------
/** Constructor */
ColorBarWidget::ColorBarWidget(QWidget *parent)
: QWidget(parent)
{
  ui.setupUi(this);

  // Default values.
  m_min = 0;
  m_max = 1000;
  m_showTooltip = false;
  m_log = false;
  m_colorMap.changeScaleType( GraphOptions::Linear );

  // Create and add the color bar
  m_colorBar = new QwtScaleWidgetExtended();
  m_colorBar->setToolTip("");
  m_colorBar->setColorBarEnabled(true);
  m_colorBar->setColorBarWidth(20);
  m_colorBar->setAlignment(QwtScaleDraw::RightScale);
  m_colorBar->setLabelAlignment( Qt::AlignRight | Qt::AlignVCenter);
  //m_colorBar->setCursor(Qt::OpenHandCursor);
  ui.verticalLayout->insertWidget(2,m_colorBar, 1,0 );

  // Hook up signals
  QObject::connect(ui.checkLog, SIGNAL(stateChanged(int)), this, SLOT(changedLogState(int)));
  QObject::connect(ui.valMin, SIGNAL(editingFinished()), this, SLOT(changedMinimum()));
  QObject::connect(ui.valMax, SIGNAL(editingFinished()), this, SLOT(changedMaximum()));
  QObject::connect(ui.valMin, SIGNAL(valueChangedFromArrows()), this, SLOT(changedMinimum()));
  QObject::connect(ui.valMax, SIGNAL(valueChangedFromArrows()), this, SLOT(changedMaximum()));
  QObject::connect(m_colorBar, SIGNAL(mouseMoved(QPoint, double)), this, SLOT(colorBarMouseMoved(QPoint, double)));

  // Initial view
  this->updateColorMap();
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

/// @return then min/max range currently viewed
QwtDoubleInterval ColorBarWidget::getViewRange() const
{ return QwtDoubleInterval(m_min, m_max); }

/// @return the color map in use (ref)
MantidColorMap & ColorBarWidget::getColorMap()
{ return m_colorMap;
}

//-------------------------------------------------------------------------------------------------
/** Turn "rendering mode" on/off, where GUI widgets are hidden
 * for the purposes of rendering an image.
 *
 * @param rendering :: true if you are going to render
 */
void ColorBarWidget::setRenderMode(bool rendering)
{
  bool visible = !rendering;
  this->ui.valMin->setVisible(visible);
  this->ui.valMax->setVisible(visible);
  this->ui.checkLog->setVisible(visible);

}


//-------------------------------------------------------------------------------------------------
/** Send a double-clicked event but only when clicking the color bar */
void ColorBarWidget::mouseDoubleClickEvent(QMouseEvent * event)
{
  if (m_colorBar->rect().contains(event->x(), event->y()))
    emit colorBarDoubleClicked();
}

//-------------------------------------------------------------------------------------------------
/// Event called after resizing
void ColorBarWidget::resizeEvent(QResizeEvent * event)
{
  updateColorMap();
  QWidget::resizeEvent(event);
}


//-------------------------------------------------------------------------------------------------
/** Adjust the steps of the spin boxes for log/linear mode */
void ColorBarWidget::setSpinBoxesSteps()
{
  // Large maximum value
  ui.valMin->setMaximum( +1e100 );
  ui.valMax->setMaximum( +1e100 );

  double step = 1.1;
  if (m_log)
  {
    // Logarithmic color scale: move by logarithmic steps
    double logRange;
    double temp_min = m_min;
    if (temp_min <= 0)
    {
      // Try to guess at a valid min range if 0 for log scale
      logRange = log10(m_max);
      if (logRange >= 3) temp_min = 1;
      else if (logRange >= 0) temp_min = 1e-3;
      // Default to 1/10000 of the max
      else temp_min = pow(10., double(int(logRange))-4.);
    }
    logRange = log10(m_max) - log10(temp_min);
    if (logRange > 6) logRange = 6;
    step = pow(10., logRange/100.);

    // Small positive value for the minimum
    ui.valMin->setMinimum( 1e-99 );
    ui.valMax->setMinimum( 1e-99 );
    // Limit the current min/max to positive values
    if (m_min < temp_min) m_min = temp_min;
    if (m_max < temp_min) m_max = temp_min;
  }
  else
  {
    // --- Linear scale ----
    // Round step that is between 1/100 to 1/1000)
    int exponent = int(log10(m_max)) - 2;
    step = pow(10., double(exponent));

    // Large negative value for the minimum
    ui.valMin->setMinimum( -1e100 );
    ui.valMax->setMinimum( -1e100 );
  }

  ui.valMin->setSingleStep(step);
  ui.valMax->setSingleStep(step);
  int dec = 2;
  ui.valMin->setDecimals(dec);
  ui.valMax->setDecimals(dec);

  updateMinMaxGUI();
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
  updateMinMaxGUI();
}

/** Set the range of values viewed in the color bar
 *
 * @param min :: min value = start of the color map
 */
void ColorBarWidget::setMinimum(double min)
{
  m_min = min;
  updateMinMaxGUI();
}

/** Set the range of values viewed in the color bar
 *
 * @param min :: min value = start of the color map
 */
void ColorBarWidget::setMaximum(double max)
{
  m_max = max;
 updateMinMaxGUI();
}

void ColorBarWidget::setViewRange(QwtDoubleInterval range)
{ this->setViewRange(range.minValue(), range.maxValue()); }

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
  m_colorMap.changeScaleType( m_log ? GraphOptions::Log10 : GraphOptions::Linear );
  ui.checkLog->setChecked( m_log );
  ui.valMin->setLogSteps( m_log );
  ui.valMax->setLogSteps( m_log );
  setSpinBoxesSteps();
  updateColorMap();
}

//-------------------------------------------------------------------------------------------------
/** SLOT called when minValue changes */
void ColorBarWidget::changedMinimum()
{
  m_min = ui.valMin->value();
  if (m_min > m_max)
  {
    m_max = m_min+0.001;
    ui.valMax->setValue( m_max );
  }
  updateColorMap();
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
    ui.valMin->setValue( m_min );
  }
  updateColorMap();
  emit changedColorRange(m_min,m_max,m_log);
}

//-------------------------------------------------------------------------------------------------
/** SLOT called when the mouse moves over the color bar*/
void ColorBarWidget::colorBarMouseMoved(QPoint globalPos, double fraction)
{
  if (m_showTooltip)
  {
    double val = 0;
    if (m_log)
      val = pow(10., fraction * (log10(m_max)-log10(m_min)) + log10(m_min));
    else
      val = fraction * (m_max-m_min) + m_min;
    QString tooltip = QString::number(val,'g', 4);
    QToolTip::showText(globalPos, tooltip, m_colorBar);
  }
}


//-------------------------------------------------------------------------------------------------
/** Update the widget when the color map is changed */
void ColorBarWidget::updateColorMap()
{
  // The color bar alway shows the same range. Doesn't matter since the ticks don't show up
  QwtDoubleInterval range(1.0, 100.0);
  m_colorBar->setColorBarEnabled(true);
  m_colorBar->setColorMap( range, m_colorMap);
  m_colorBar->setColorBarWidth(15);
  m_colorBar->setEnabled(true);

  // Try to limit the number of steps based on the height of the color bar
  int maxMajorSteps = m_colorBar->height()/15; // 15 pixels per div looked about right
  //std::cout << "maxMajorSteps" << maxMajorSteps << std::endl;
  if (maxMajorSteps > 10) maxMajorSteps = 10;

  // Show the scale on the right
  double minValue = m_min;
  double maxValue = m_max;
  GraphOptions::ScaleType type = m_colorMap.getScaleType();
  if( type == GraphOptions::Linear )
  {
    QwtLinearScaleEngine linScaler;
    m_colorBar->setScaleDiv(linScaler.transformation(), linScaler.divideScale(minValue, maxValue, maxMajorSteps, 5));
    m_colorBar->setColorMap(QwtDoubleInterval(minValue, maxValue),m_colorMap);
  }
  else
 {
    QwtLog10ScaleEngine logScaler;
    m_colorBar->setScaleDiv(logScaler.transformation(), logScaler.divideScale(minValue, maxValue, maxMajorSteps, 5));
    m_colorBar->setColorMap(QwtDoubleInterval(minValue, maxValue), m_colorMap);
  }

}

//-------------------------------------------------------------------------------------------------
/** Updatet the widget when changing min/max*/
void ColorBarWidget::updateMinMaxGUI()
{
  ui.valMin->setValue( m_min );
  ui.valMax->setValue( m_max );
}



ColorBarWidget::~ColorBarWidget()
{
}

} //namespace
}
