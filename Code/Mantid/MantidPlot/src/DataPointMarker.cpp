#include "DataPointMarker.h"

DataPointMarker::DataPointMarker (Plot *plot)
  : QObject(plot), PlotEnrichement(),  m_positionX(), m_positionY(), d_x_right(0.0),d_y_bottom (0.0)
  {
  }

// d_plot(plot),


void DataPointMarker::setMarkerPlotPos(double x, double y)
{
  if (!plot())
	return;
  
  plot()->updateLayout();

  m_positionX = x;
  m_positionY = y;

  QPainter * painter;

  int posX = plot()->transform(QwtPlot::xBottom, m_positionX);
  int posY = plot()->transform(QwtPlot::yLeft, m_positionY);

  symbol().draw(painter, posX, posY);

  /*
  const QwtScaleMap &xMap = xMap;
  const QwtScaleMap &yMap = yMap;

  QRect r;
  
  draw(painter, xMap, yMap, r);
  */
}


void DataPointMarker::setXValue(double x)
{
  m_positionX = x;
  xPlotPosOfDataPoint();
}

void DataPointMarker::setYValue(double y)
{
  m_positionY = y;
  yPlotPosOfDataPoint();
}

double DataPointMarker::xPlotPosOfDataPoint()
{
  double xPlotPosOfDataPoint = m_positionX;
  return xPlotPosOfDataPoint;
}

double DataPointMarker::yPlotPosOfDataPoint()
{
  double yPlotPosOfDataPoint = m_positionY;
  return yPlotPosOfDataPoint;
}

int DataPointMarker::xPaintPosOfDataPoint()
{
  int xPaintPosOfDataPoint = plot()->transform(QwtPlot::xBottom, m_positionX);
  return xPaintPosOfDataPoint;
}

int DataPointMarker::yPaintPosOfDataPoint()
{
  int yPaintPosOfDataPoint = plot()->transform(QwtPlot::yLeft, m_positionY);
  return yPaintPosOfDataPoint;
}

/*
void DataPointMarker::draw(QPainter *p, 
    const QwtScaleMap &xMap, const QwtScaleMap &yMap,
    const QRect &) const
{
  const int posX = xMap.transform(m_positionX);
  const int posY = yMap.transform(m_positionY);
      
  p->save();
  symbol().draw(p, posX, posY);

  p->restore();
  
  return;
}


void DataPointMarker::setBoundingRect(double left, double top, double right, double bottom)
{
    if (xValue() == left && yValue() == top && d_x_right == right && d_y_bottom == bottom)
        return;

    setXValue(left);
    setYValue(top);
    d_x_right = right;
    d_y_bottom = bottom;

    if (!plot())
        return;

    plot()->updateLayout();

    QRect r = this->rect();
    d_pos = r.topLeft();
    d_size = r.size();
}

void DataPointMarker::setRect(int x, int y, int w, int h)
{
    if (d_pos == QPoint(x, y) && d_size == QSize(w, h))
        return;

    d_pos = QPoint(x, y);
    d_size = QSize(w, h);
    updateBoundingRect();
}

QRect DataPointMarker::rect() const
{
    const QwtScaleMap &xMap = plot()->canvasMap(xAxis());
    const QwtScaleMap &yMap = plot()->canvasMap(yAxis());

    const int x0 = xMap.transform(xValue());
    const int y0 = yMap.transform(yValue());
    const int x1 = xMap.transform(d_x_right);
    const int y1 = yMap.transform(d_y_bottom);

    return QRect(x0, y0, abs(x1 - x0), abs(y1 - y0));
}

QwtDoubleRect DataPointMarker::boundingRect() const
{
    return QwtDoubleRect(xValue(), yValue(), qAbs(d_x_right - xValue()), qAbs(d_y_bottom - yValue()));
}


void DataPointMarker::updateBoundingRect()
{
    if (!plot())
        return;

    setXValue(plot()->invTransform(xAxis(), d_pos.x()));
    d_x_right = plot()->invTransform(xAxis(), d_pos.x() + d_size.width());

    setYValue(plot()->invTransform(yAxis(), d_pos.y()));
    d_y_bottom = plot()->invTransform(yAxis(), d_pos.y() + d_size.height());
}
*/


/*
void DataPointMarker::setOrigin(const QPoint& p)
{
    d_pos = p;

    if (!plot())
        return;

    setXValue(plot()->invTransform(xAxis(), p.x()));
    setYValue(plot()->invTransform(yAxis(), p.y()));

    d_size = size();
    updateBoundingRect();
}

void DataPointMarker::setLabel(const QwtText &)
{

}
*/