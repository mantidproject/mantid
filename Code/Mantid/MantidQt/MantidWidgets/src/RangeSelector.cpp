#include "MantidQtMantidWidgets/RangeSelector.h"

#include <qwt_plot_picker.h>

#include <QEvent>
#include <QMouseEvent>

#include "MantidQtMantidWidgets/PreviewPlot.h"

using namespace MantidQt::MantidWidgets;

RangeSelector::RangeSelector(QwtPlot* plot, SelectType type, bool visible, bool infoOnly)
  : QwtPlotPicker(plot->canvas()), m_type(type), m_min(0.0) ,m_max(0.0), m_lower(0.0),
    m_higher(0.0), m_canvas(plot->canvas()), m_plot(plot), m_mrkMin(NULL), m_mrkMax(NULL),
    m_minChanging(false), m_maxChanging(false), m_infoOnly(infoOnly), m_visible(visible),
    m_pen(NULL), m_movCursor()
{
  init();
}

RangeSelector::RangeSelector(PreviewPlot* plot, SelectType type, bool visible, bool infoOnly)
  : QwtPlotPicker(plot->m_uiForm.plot->canvas()), m_type(type), m_min(0.0) ,m_max(0.0), m_lower(0.0),
    m_higher(0.0), m_canvas(plot->m_uiForm.plot->canvas()), m_plot(plot->m_uiForm.plot), m_mrkMin(NULL), m_mrkMax(NULL),
    m_minChanging(false), m_maxChanging(false), m_infoOnly(infoOnly), m_visible(visible),
    m_pen(NULL), m_movCursor()
{
  init();
}

void RangeSelector::init()
{
  m_canvas->installEventFilter(this);

  m_canvas->setCursor(Qt::PointingHandCursor); ///< @todo Make this an option at some point

  m_mrkMin = new QwtPlotMarker();
  m_mrkMax = new QwtPlotMarker();

  QwtPlotMarker::LineStyle lineStyle;

  switch ( m_type )
  {
  case XMINMAX:
  case XSINGLE:
    lineStyle = QwtPlotMarker::VLine;
    m_movCursor = Qt::SizeHorCursor;
    break;
  case YMINMAX:
  case YSINGLE:
    lineStyle = QwtPlotMarker::HLine;
    m_movCursor = Qt::SizeVerCursor;
    break;
  default:
    lineStyle = QwtPlotMarker::Cross;
    break;
  }

  switch ( m_type )
  {
  case XMINMAX:
  case YMINMAX:
    m_mrkMax->setLineStyle(lineStyle);
    m_mrkMax->attach(m_plot);
    m_mrkMax->setYValue(1.0);
  case XSINGLE:
  case YSINGLE:
    m_mrkMin->setLineStyle(lineStyle);
    m_mrkMin->attach(m_plot);
    m_mrkMin->setYValue(0.0);
    break;
  }

  m_minChanging = false;
  m_maxChanging = false;

  setMin(100); // known starting values
  setMax(200);

  /// Setup pen with default values
  m_pen = new QPen();
  m_pen->setColor(Qt::blue);
  m_pen->setStyle(Qt::DashDotLine);

  // Apply pen to marker objects
  m_mrkMin->setLinePen(*m_pen);
  m_mrkMax->setLinePen(*m_pen);
}

bool RangeSelector::eventFilter(QObject* obj, QEvent* evn)
{
  Q_UNUSED(obj);
  // Do not handle the event if the widget is set to be invisible
  if ( !m_visible || m_infoOnly )
  {
    return false;
  }

  switch ( evn->type() )
  {
  case QEvent::MouseButtonPress: // User has started moving something (perhaps)
    {
    QPoint p = ((QMouseEvent*)evn)->pos();
    double x(0.0), dx(0.0);
    switch ( m_type )
    {
    case XMINMAX:
    case XSINGLE:
      x = m_plot->invTransform(QwtPlot::xBottom, p.x());
      dx = m_plot->invTransform(QwtPlot::xBottom, p.x()+3);
      break;
    case YMINMAX:
    case YSINGLE:
      x = m_plot->invTransform(QwtPlot::yLeft, p.y());
      dx = m_plot->invTransform(QwtPlot::yLeft, p.y()+3);
      break;
    }
    if ( inRange(x) )
    {
      if ( changingMin(x, dx) )
      {
        m_minChanging = true;
        m_canvas->setCursor(m_movCursor);
        setMin(x);
        m_plot->replot();
        return true;
      }
      else if ( changingMax(x, dx) )
      {
        m_maxChanging = true;
        m_canvas->setCursor(m_movCursor);
        setMax(x);
        m_plot->replot();
        return true;
      }
      else
      {
        return false;
      }
    }
    else
    {
      return false;
    }
    break;
    }
  case QEvent::MouseMove: // User is in the process of moving something (perhaps)
    {
    if ( m_minChanging || m_maxChanging )
    {
    QPoint p = ((QMouseEvent*)evn)->pos();
    double x(0.0);
    switch ( m_type )
    {
    case XMINMAX:
    case XSINGLE:
      x = m_plot->invTransform(QwtPlot::xBottom, p.x());
      break;
    case YMINMAX:
    case YSINGLE:
      x = m_plot->invTransform(QwtPlot::yLeft, p.y());
      break;
    }
    if ( inRange(x) )
    {
      if ( m_minChanging )
      {
        if ( x <= m_max )
        {
            setMin(x);
        }
        else
        {
            setMax(x);
            m_minChanging = false;
            m_maxChanging = true;
        }
      }
      else
      {
        if ( x >= m_min )
        {
            setMax(x);
        }
        else
        {
            setMin(x);
            m_minChanging = true;
            m_maxChanging = false;
        }
      }
    }
    else
    {
      m_canvas->setCursor(Qt::PointingHandCursor);
      m_minChanging = false;
      m_maxChanging = false;
      emit selectionChangedLazy(m_min, m_max);
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
      if ( m_minChanging || m_maxChanging )
      {
        m_canvas->setCursor(Qt::PointingHandCursor);
        m_minChanging = false;
        m_maxChanging = false;
        emit selectionChangedLazy(m_min, m_max);
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

std::pair<double, double> RangeSelector::getRange()
{
  std::pair<double,double> range(m_lower,m_higher);
  return range;
}

void RangeSelector::setRange(double min, double max)
{
  m_lower = ( min < max ) ? min : max;
  m_higher = ( min < max ) ? max : min;
  verify();
  emit rangeChanged(min, max);
}

void RangeSelector::setRange(std::pair<double,double> range)
{
  this->setRange(range.first, range.second);
}

void RangeSelector::setMinLinePos(double val)
{
  switch ( m_type )
  {
  case XMINMAX:
  case XSINGLE:
    m_mrkMin->setValue(val, 1.0);
    break;
  case YMINMAX:
  case YSINGLE:
    m_mrkMin->setValue(1.0, val);
    break;
  }
  m_plot->replot();
}

void RangeSelector::setMaxLinePos(double val)
{
  switch ( m_type )
  {
  case XMINMAX:
  case XSINGLE:
    m_mrkMax->setValue(val, 1.0);
    break;
  case YMINMAX:
  case YSINGLE:
    m_mrkMax->setValue(1.0, val);
    break;
  }
  m_plot->replot();
}

void RangeSelector::setMinimum(double val)
{
  setMin(val);
}

void RangeSelector::setMaximum(double val)
{
  setMax(val);
}

void RangeSelector::reapply()
{
  m_mrkMin->attach(m_plot);
  m_mrkMax->attach(m_plot);
}

void RangeSelector::detach()
{
  m_mrkMin->attach(NULL);
  m_mrkMax->attach(NULL);
}

void RangeSelector::setColour(QColor colour)
{
  m_pen->setColor(colour);
  switch ( m_type )
  {
  case XMINMAX:
  case YMINMAX:
    m_mrkMax->setLinePen(*m_pen);
  case XSINGLE:
  case YSINGLE:
    m_mrkMin->setLinePen(*m_pen);
    break;
  }
}

void RangeSelector::setInfoOnly(bool state)
{
  m_infoOnly = state;
}

void RangeSelector::setVisible(bool state)
{
  if ( state )
  {
    m_mrkMin->show();
    m_mrkMax->show();
  }
  else
  {
    m_mrkMin->hide();
    m_mrkMax->hide();
  }
  m_plot->replot();
  m_visible = state;
}
/** Changes the start and end of the x-axis range
*  @param min the start of the range
*  @param max the end of the range
*/
void RangeSelector::setMaxMin(const double min, const double max)
{
  if ( min == m_min && max == m_max )
  {
    //this is just to save work, the comparison above may fail if min or max are represented differently in the machine but that wont cause bad result
    return;
  }
  m_min = min;
  m_max = max;
  setMinLinePos(m_min);
  setMaxLinePos(m_max);
  emit selectionChanged(m_min, m_max);
  emit minValueChanged(m_min);
  emit maxValueChanged(m_max);
}
/** Changes only the minium of the x range, the current range maximium
*  must be valid
*  @param val the start of the range
*/
void RangeSelector::setMin(double val)
{
  if ( val != m_min )
  {
    m_min = val;
    setMinLinePos(m_min);
    emit minValueChanged(val);
    emit selectionChanged(val, m_max);
  }
}
/** Changes only the maximium of the x range, the current range minimum
*  must be valid
*  @param val the end of the range
*/
void RangeSelector::setMax(double val)
{
  if ( val != m_max )
  {
    m_max = val;
    setMaxLinePos(m_max);
    emit maxValueChanged(val);
    emit selectionChanged(m_min, val);
  }
}

bool RangeSelector::changingMin(double x, double dx)
{
  return ( fabs(x - m_min) <= fabs(dx-x) );
}

bool RangeSelector::changingMax(double x, double dx)
{
  return ( fabs(x - m_max) <= fabs(dx-x) );
}

void RangeSelector::verify()
{
  double min(m_min);
  double max(m_max);

  if ( min > max )
  {
    std::swap(min,max);
  }

  if ( min < m_lower || min > m_higher )
  {
    min = m_lower;
  }
  if ( max < m_lower || max > m_higher )
  {
    max = m_higher;
  }

  setMaxMin(min, max);
}

bool RangeSelector::inRange(double x)
{
  if ( x < m_lower || x > m_higher )
  {
    return false;
  }
  else
  {
    return true;
  }
}
