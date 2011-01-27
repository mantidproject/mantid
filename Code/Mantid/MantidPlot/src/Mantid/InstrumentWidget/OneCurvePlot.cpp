#include "OneCurvePlot.h"

#include <qwt_plot_curve.h>
#include <qwt_scale_div.h>
#include <qwt_scale_engine.h>
#include <qwt_plot_canvas.h>

#include <QFontMetrics>
#include <QMouseEvent>

OneCurvePlot::OneCurvePlot(QWidget* parent):
QwtPlot(parent),m_curve(NULL)
{
  setAxisFont(QwtPlot::xBottom, parent->font());
  setAxisFont(QwtPlot::yLeft, parent->font());
  //setMouseTracking(true);
  //canvas()->setMouseTracking(true);
 }

/**
  * Set the scale of the horizontal axis
  * @param from Minimum value
  * @param to Maximum value
  */
void OneCurvePlot::setXScale(double from, double to)
{
  QFontMetrics fm(this->font());
  int n = from != 0.0 ? abs(log10(fabs(from))) : 0;
  int n1 = to != 0.0 ? abs(log10(fabs(to))) : 0;
  if (n1 > n) n = n1;
  n += 4;

  int labelWidth = n * fm.width("0"); // approxiamte width of a tick label in pixels
  int nMajorTicks = this->width() / labelWidth;
  //std::cerr << "ticks: " << labelWidth << ' ' << nMajorTicks << std::endl;
  const QwtScaleDiv div = axisScaleEngine(QwtPlot::xBottom)->divideScale(from,to,nMajorTicks,nMajorTicks);
  setAxisScaleDiv(xBottom,div);
}

/**
  * Set the scale of the vertical axis
  * @param from Minimum value
  * @param to Maximum value
  */
void OneCurvePlot::setYScale(double from, double to)
{
  setAxisScale(QwtPlot::yLeft,from,to);
}

/**
  * Set the data for the curve to display
  * @param x A pointer to x values
  * @param y A pointer to y values
  * @param dataSize The size of the data
  */
void OneCurvePlot::setData(const double* x,const double* y,int dataSize)
{
  if (!m_curve)
  {
    m_curve = new QwtPlotCurve();
    m_curve->attach(this);
  }

  m_curve->setData(x,y,dataSize);
}

/**
  * Hide the curve
  */
void OneCurvePlot::clearCurve()
{
  if (m_curve)
  {
    m_curve->attach(0);
    m_curve = NULL;
  }
}

void OneCurvePlot::resizeEvent(QResizeEvent *e)
{
  QwtPlot::resizeEvent(e);
  recalcAxisDivs();
}

/**
  * Recalculate axis divisions to make sure that tick labels don't overlap
  */
void OneCurvePlot::recalcAxisDivs()
{
  const QwtScaleDiv *div0 = axisScaleDiv(QwtPlot::xBottom);
  double from = div0->lBound();
  double to = div0->hBound();
  setXScale(from,to);
}

void OneCurvePlot::mousePressEvent(QMouseEvent* e)
{
  if (e->buttons() & Qt::RightButton)
  {
    e->accept();
    emit showContextMenu();
  }
}

//void OneCurvePlot::mouseMoveEvent(QMouseEvent* e)
//{
//}

//bool OneCurvePlot::event(QEvent *e)
//{
//  static int i = 0;
//  std::cerr << "event " << i++ << "\n" ;
//  return QwtPlot::event(e);
//}
