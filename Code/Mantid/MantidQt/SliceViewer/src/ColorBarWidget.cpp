#include "MantidQtSliceViewer/ColorBarWidget.h"
#include <qwt_scale_widget.h>
#include "qwt_scale_div.h"
#include <qwt_scale_map.h>

ColorBarWidget::ColorBarWidget(QWidget *parent)
: QWidget(parent)
{
  ui.setupUi(this);

  // Default values.
  m_colorMap = new QwtLinearColorMap(Qt::blue, Qt::red);
  m_min = 0;
  m_max = 10;

  // Create and add the color bar
  m_colorBar = new QwtScaleWidget();
  ui.verticalLayout->insertWidget(2,m_colorBar, 1,0 );

  this->update();
}


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


/** Set the range of values in the overall data (limits to selections)
 *
 * @param min
 * @param max
 */
void ColorBarWidget::setDataRange(double min, double max)
{
  m_rangeMin = min;
  m_rangeMax = max;
  ui.valMin->setMinimum( m_rangeMin );
  ui.valMin->setMaximum( m_rangeMax );
  ui.valMax->setMinimum( m_rangeMin );
  ui.valMax->setMaximum( m_rangeMax );
}

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

/** Set the color bar to use log scale
 *
 * @param log :: true to use log scale
 */
void ColorBarWidget::setLog(bool log)
{
  m_log = log;
  update();
}



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
