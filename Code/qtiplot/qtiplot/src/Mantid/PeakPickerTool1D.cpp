#include "PeakPickerTool1D.h"
#include "MantidCurve.h"
#include "../ApplicationWindow.h"
#include "qwt_painter.h"
#include <qpainter.h>
#include <qlist.h>
#include <qtoolbar.h>
#include <qaction.h>
#include <iostream>

PeakPickerTool1D::PeakPickerTool1D(Graph *graph, ApplicationWindow *app) :
QwtPlotPicker(graph->plotWidget()->canvas()),
PlotToolInterface(graph),
d_app(app),m_range(0),m_wsName(),m_spec()
{
  d_graph->plotWidget()->canvas()->setCursor(Qt::pointingHandCursor);

  if (d_graph->plotWidget()->curves().size() > 0)
  {
    // Can we use a different curve? (not the first one). RT.
    PlotCurve* curve = dynamic_cast<PlotCurve*>(d_graph->plotWidget()->curves().begin().value());
    if (!curve) return;
    DataCurve* dcurve = dynamic_cast<DataCurve*>(curve);
    if (dcurve)
    {
      m_wsName = dcurve->table()->name().section('-',0,0);
      m_spec = dcurve->table()->colName(0).section('_',1,1).mid(2).toInt();
    }
    else
    {
      MantidCurve* mcurve = dynamic_cast<MantidCurve*>(curve);
      if (mcurve)
      {
        m_wsName = mcurve->title().text().section('-',0,0);
        m_spec = mcurve->title().text().section('-',2,2).toInt();
      }
    }
  }
}

PeakPickerTool1D::~PeakPickerTool1D()
{
  if (m_range) m_range->detach();
  delete m_range;
	d_graph->plotWidget()->canvas()->unsetCursor();
  d_graph->plotWidget()->replot();
}

/**
     Event filter. Returning true means event processed, false let it go 
     down the processing chain.
 */
bool PeakPickerTool1D::eventFilter(QObject *obj, QEvent *event)
{
  //std::cerr<<"event "<<event->type()<<'\n';
  switch(event->type()) {
    case QEvent::MouseButtonDblClick:  
      return true;

    case QEvent::MouseMove:
      {
        if (m_range)
        {
          QPoint pnt = ((QMouseEvent*)event)->pos();
          Qt::KeyboardModifiers mod = ((QMouseEvent*)event)->modifiers();
          if (!m_range->isWidthSet())
          {
            double c = m_range->centre();
            double w = d_graph->plotWidget()->invTransform(2,pnt.x()) - c;
            m_range->setWidth(2*fabs(w));
            d_graph->plotWidget()->replot();
          }
          else if (m_range->resetting())
          {
            double c = d_graph->plotWidget()->invTransform(2,pnt.x());
            double h = d_graph->plotWidget()->invTransform(0,pnt.y());
            m_range->reset(c,h);
            d_graph->plotWidget()->replot();
          }
        }
        break;
      }

    case QEvent::MouseButtonPress:
      {
        Qt::KeyboardModifiers mod = ((QMouseEvent*)event)->modifiers();
        QPoint p = ((QMouseEvent*)event)->pos();
        if (((QMouseEvent*)event)->button() == Qt::LeftButton )
        {
          if ( mod.testFlag(Qt::ShiftModifier) || !m_range)
          {
            if (!m_range)// init the tool
            {
              m_range = new PeakRangeMarker1D();
              m_range->attach(d_graph->plotWidget());
            }
            // x - axis is #2, y - is #0
            double c = d_graph->plotWidget()->invTransform(2,p.x());
            double h = d_graph->plotWidget()->invTransform(0,p.y());
            m_range->add(c,h);
            d_graph->plotWidget()->replot();
          }
          else
          {
            if (m_range)
            {
              m_range->widthIsSet();
              double x = d_graph->plotWidget()->invTransform(2,p.x());
              double x1 = d_graph->plotWidget()->invTransform(2,p.x()+3);
              int ic = m_range->clickedOnCentreMarker(x,fabs(x1-x));
              if (m_range->clickedOnWidthMarker(x,fabs(x1-x)))
              {// begin changing width
                m_range->widthIsSet(false);
                d_graph->plotWidget()->canvas()->setCursor(Qt::SizeHorCursor);
                d_graph->plotWidget()->replot();
              }
              else if (ic >= 0)
              {// select current, begin changing centre and height
                m_range->setCurrent(ic);
                d_graph->plotWidget()->replot();
                m_range->resetting(true);
              }
            }
          }
        }
        return true;
      }

    case QEvent::MouseButtonRelease:
      d_graph->plotWidget()->canvas()->setCursor(Qt::pointingHandCursor);
      if (m_range)
      {
        m_range->widthIsSet();
        m_range->resetting(false);
      }
      break;

    case QEvent::KeyPress:
      break;
    case QEvent::KeyRelease:
      break;
    default:
      break;
  }
  return QwtPlotPicker::eventFilter(obj, event);
}

void PeakPickerTool1D::windowStateChanged( Qt::WindowStates oldState, Qt::WindowStates newState )
{
  if (newState.testFlag(Qt::WindowActive))
    d_app->enableMantidPeakFit(true);
  else
    d_app->enableMantidPeakFit(false);
}

//--------------------------------------------------------
//      PeakRangeMarker1D methods
//--------------------------------------------------------

void PeakRangeMarker1D::draw(QPainter *p, 
                  const QwtScaleMap &xMap, const QwtScaleMap &yMap,
                  const QRect &) const
{
  for(int i=0;i<m_params.size();i++)
  {
    double c = m_params[i].centre;
    if (c >= xMap.s1() && c <= xMap.s2())
    {
      int ic = xMap.transform(c);
      if (i == m_current)
      {
        double width = m_params[i].width;
        QPen pen;
        pen.setColor(QColor(255,0,0));
        pen.setStyle(Qt::DashLine);
        p->setPen(pen);
        int x1 = xMap.transform(c - width/2);
        int x2 = xMap.transform(c + width/2);
        QwtPainter::drawLine(p, x1, xMap.p1(), x1,xMap.p2());
        QwtPainter::drawLine(p, x2, xMap.p1(), x2,xMap.p2());

        pen.setStyle(Qt::SolidLine);
        p->setPen(pen);
        int ih = yMap.transform(height());
        QwtPainter::drawLine(p, ic, xMap.p1(), ic, ih);
      }
      else
      {
        p->setPen(QPen(QColor(0,0,0)));
        QwtPainter::drawLine(p, ic, xMap.p1(), ic, xMap.p1() + 10);
      }
    }
  }
}
void PeakRangeMarker1D::add(double c,double h)
{
  int idx = -1;
  for(int i=0;i<m_params.size();i++)
    if (m_params[i].centre == c)
    {
      idx = i;
      break;
    }
    if (idx < 0)
    {
      m_params.append(PeakParams(c,h,m_width));
      m_current = m_params.size() - 1;
    }
    else
      m_current = idx;
}

