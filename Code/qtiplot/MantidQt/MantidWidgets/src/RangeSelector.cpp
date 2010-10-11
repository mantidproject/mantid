#include "MantidQtMantidWidgets/RangeSelector.h"

#include <qwt_plot_picker.h>

#include <QEvent>
#include <QMouseEvent>

using namespace MantidQt::MantidWidgets;

RangeSelector::RangeSelector(QwtPlot* plot) : QwtPlotPicker(plot->canvas()), m_canvas(plot->canvas()), m_plot(plot)
{
  m_canvas->installEventFilter(this);

  m_canvas->setCursor(Qt::PointingHandCursor); ///< @todo Make this an option at some point

  m_mrkMin = new QwtPlotMarker();
  m_mrkMax = new QwtPlotMarker();

  m_mrkMin->setLineStyle(QwtPlotMarker::VLine);
  m_mrkMin->attach(m_plot);
  m_mrkMin->setYValue(0.0);

  m_mrkMax->setLineStyle(QwtPlotMarker::VLine);
  m_mrkMax->attach(m_plot);
  m_mrkMax->setYValue(1.0);

  connect(this, SIGNAL(xMinValueChanged(double)), this, SLOT(xMinChanged(double)));
  connect(this, SIGNAL(xMaxValueChanged(double)), this, SLOT(xMaxChanged(double)));

  m_xMinChanging = false;
  m_xMaxChanging = false;

  setXMin(100); // known starting values
  setXMax(200);

  /// Setup pen with default values
  /// @todo Add constructor options for pen settings and other functions to change it
  m_pen = new QPen();
  m_pen->setColor(Qt::blue);
  m_pen->setStyle(Qt::DashDotLine);

  // Apply pen to marker objects
  m_mrkMin->setLinePen(*m_pen);
  m_mrkMax->setLinePen(*m_pen);
}

bool RangeSelector::eventFilter(QObject* obj, QEvent* evn)
{
  switch ( evn->type() )
  {
  case QEvent::MouseButtonPress: // User has started moving something (perhaps)
    {
    QPoint p = ((QMouseEvent*)evn)->pos();
    double x = m_plot->invTransform(QwtPlot::xBottom, p.x());
    double dx = m_plot->invTransform(QwtPlot::xBottom, p.x()+3);
    if ( inRange(x) )
    {
      if ( changingXMin(x, dx) )
      {
        m_xMinChanging = true;
        m_canvas->setCursor(Qt::SizeHorCursor);
        setXMin(x);
        m_plot->replot();
        return true;
      }
      else if ( changingXMax(x, dx) )
      {
        m_xMaxChanging = true;
        m_canvas->setCursor(Qt::SizeHorCursor);
        setXMax(x);
        m_plot->replot();
        return true;
      }
      else
      {
        return false;
      }
    }
    break;
    }
  case QEvent::MouseMove: // User is in the process of moving something (perhaps)
    {
    if ( m_xMinChanging || m_xMaxChanging )
    {
    QPoint p = ((QMouseEvent*)evn)->pos();
    double x = m_plot->invTransform(QwtPlot::xBottom, p.x());
    if ( inRange(x) )
    {
      if ( m_xMinChanging )
      {
        setXMin(x);
        if ( x > m_xMax )
          setXMax(x);
      }
      else
      {
        setXMax(x);
        if ( x < m_xMin )
          setXMin(x);
      }
    }
    else
    {
      m_canvas->setCursor(Qt::PointingHandCursor);
      m_xMinChanging = false;
      m_xMaxChanging = false;
    }
    m_plot->replot();
    return true;
    }
    else
    {
    return false;
    }
    break;
    }
  case QEvent::MouseButtonRelease: // User has finished moving something (perhaps)
    {
    if ( m_xMinChanging || m_xMaxChanging )
    {
    m_canvas->setCursor(Qt::PointingHandCursor);
    QPoint p = ((QMouseEvent*)evn)->pos();
    double x = m_plot->invTransform(QwtPlot::xBottom, p.x());
    if ( inRange(x) )
    {
    if ( m_xMinChanging )
      setXMin(x);
    else
      setXMax(x);
    }
    m_plot->replot();
    m_xMinChanging = false;
    m_xMaxChanging = false;
    return true;
    }
    else
    {
    return false;
    }
    break;
    }
  default:
    return false;
  }
}

void RangeSelector::setRange(double min, double max)
{
  m_lower = min;
  m_higher = max;
  verify();
}

void RangeSelector::xMinChanged(double val)
{
  m_mrkMin->setValue(val, 0.0);
  m_plot->replot();
}

void RangeSelector::xMaxChanged(double val)
{
  m_mrkMax->setValue(val, 1.0);
  m_plot->replot();
}

void RangeSelector::setMinimum(double val)
{
  setXMin(val);
}
void RangeSelector::setMaximum(double val)
{
  setXMax(val);
}

void RangeSelector::reapply()
{
  m_mrkMin->attach(m_plot);
  m_mrkMax->attach(m_plot);
}

void RangeSelector::setXMin(double val)
{
  m_xMin = val;
  emit xMinValueChanged(val);
}

void RangeSelector::setXMax(double val)
{
  m_xMax = val;
  emit xMaxValueChanged(val);
}

bool RangeSelector::changingXMin(double x, double dx)
{
  return ( fabs(x - m_xMin) <= fabs(dx-x) );
}

bool RangeSelector::changingXMax(double x, double dx)
{
  return ( fabs(x - m_xMax) <= fabs(dx-x) );
}

void RangeSelector::verify()
{
  if ( m_xMin < m_lower || m_xMin > m_higher )
    setXMin(m_lower);
  if ( m_xMax < m_lower || m_xMax > m_higher )
    setXMax(m_higher);

  if ( m_xMin > m_xMax )
  {
    double tmp = m_xMin;
    setXMin(m_xMax);
    setXMax(tmp);
  }
}

bool RangeSelector::inRange(double x)
{
  if ( x < m_lower || x > m_higher )
    return false;
  else
    return true;
}