#include "PeakPickerTool.h"
#include "MantidCurve.h"
#include "MantidUI.h"
#include "FitPropertyBrowser.h"
#include "MantidAPI/CompositeFunction.h"

#include "qwt_painter.h"
#include <qpainter.h>
#include <qlist.h>
#include <qtoolbar.h>
#include <qaction.h>
#include <iostream>

PeakPickerTool::PeakPickerTool(Graph *graph, MantidUI *mantidUI) :
QwtPlotPicker(graph->plotWidget()->canvas()),
PlotToolInterface(graph),
m_mantidUI(mantidUI),m_range(0),m_wsName(),m_spec()
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

PeakPickerTool::~PeakPickerTool()
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
bool PeakPickerTool::eventFilter(QObject *obj, QEvent *event)
{
  //std::cerr<<"event "<<event->type()<<'\n';
  switch(event->type()) {
    case QEvent::MouseButtonDblClick:  
      {
        if (m_range)
        {
          QPoint p = ((QMouseEvent*)event)->pos();
          double x = d_graph->plotWidget()->invTransform(2,p.x());
          double x1 = d_graph->plotWidget()->invTransform(2,p.x()+3);
          double dx = fabs(x-x1);
          if (m_range->clickedOnXMin(x,dx) || m_range->clickedOnXMax(x,dx))
          {
            m_range->xMin(x-dx);
            m_range->xMax(x+dx);
            d_graph->plotWidget()->replot();
          }
        }
        return true;
      }

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
            emit peakChanged();
          }
          else if (m_range->resetting())
          {
            double c = d_graph->plotWidget()->invTransform(2,pnt.x());
            double h = d_graph->plotWidget()->invTransform(0,pnt.y());
            m_range->reset(c,h);
            emit peakChanged();
          }
          else if (m_range->changingXMin() && m_range->changingXMax())
          {// modify xMin and xMax at the same time 
            double x = d_graph->plotWidget()->invTransform(2,pnt.x());
            double x0 = (m_range->xMin() + m_range->xMax())/2;
            double xMin,xMax;
            if (x >= x0)
            {
              xMax = x;
              xMin = x0*2-x;
            }
            else
            {
              xMax = x0*2-x;
              xMin = x;
            }
            m_range->xMin(xMin);
            m_range->xMax(xMax);
          }
          else if (m_range->changingXMin())
          {
            double x = d_graph->plotWidget()->invTransform(2,pnt.x());
            m_range->xMin(x);
          }
          else if (m_range->changingXMax())
          {
            double x = d_graph->plotWidget()->invTransform(2,pnt.x());
            m_range->xMax(x);
          }
          d_graph->plotWidget()->replot();
        }
        break;
      }

    case QEvent::MouseButtonPress:
      {
        Qt::KeyboardModifiers mod = ((QMouseEvent*)event)->modifiers();
        QPoint p = ((QMouseEvent*)event)->pos();
        if (((QMouseEvent*)event)->button() == Qt::LeftButton )
        {
          // Shift button was pressed
          if ( mod.testFlag(Qt::ShiftModifier) || !m_range)
          {// Create the marker
            if (!m_range)// init the tool
            {
              m_range = new PeakRangeMarker();
              m_range->attach(d_graph->plotWidget());
              double x = d_graph->plotWidget()->invTransform(2,p.x());
              // when PeakRangeMarker is created chngingxMin() and changingXMax are both true
              m_range->xMin(x);
              m_range->xMax(x);
            }
            else
            {// Add a new peak
              // x - axis is #2, y - is #0
              double c = d_graph->plotWidget()->invTransform(2,p.x());
              double h = d_graph->plotWidget()->invTransform(0,p.y());
              m_range->add(c,h);
              emit peakChanged();
            }
            d_graph->plotWidget()->replot();
          }
          else // No shift button
          {
            if (m_range)
            {
              m_range->widthIsSet();
              double x = d_graph->plotWidget()->invTransform(2,p.x());
              double x1 = d_graph->plotWidget()->invTransform(2,p.x()+3);
              int ic = m_range->clickedOnCentreMarker(x,fabs(x1-x));
              if (m_range->clickedOnXMax(x,fabs(x1-x)))
              {// begin changing xMax
                m_range->changingXMax(true);
                d_graph->plotWidget()->canvas()->setCursor(Qt::SizeHorCursor);
                d_graph->plotWidget()->replot();
                emit peakChanged();
              }
              if (m_range->clickedOnXMin(x,fabs(x1-x)))
              {// begin changing xMin
                m_range->changingXMin(true);
                d_graph->plotWidget()->canvas()->setCursor(Qt::SizeHorCursor);
                d_graph->plotWidget()->replot();
                emit peakChanged();
              }
              if (m_range->clickedOnWidthMarker(x,fabs(x1-x)))
              {// begin changing width
                m_range->widthIsSet(false);
                d_graph->plotWidget()->canvas()->setCursor(Qt::SizeHorCursor);
                d_graph->plotWidget()->replot();
                emit peakChanged();
              }
              else if (ic >= 0)
              {// select current, begin changing centre and height
                m_range->setCurrent(ic);
                d_graph->plotWidget()->replot();
                m_range->resetting(true);
                emit peakChanged();
              }
            }
          }
        }
        return true;
      }

      // Mouse button up - stop all changes
    case QEvent::MouseButtonRelease:
      d_graph->plotWidget()->canvas()->setCursor(Qt::pointingHandCursor);
      if (m_range)
      {
        m_range->widthIsSet();
        m_range->resetting(false);
        m_range->changingXMin(false);
        m_range->changingXMax(false);
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

void PeakPickerTool::windowStateChanged( Qt::WindowStates oldState, Qt::WindowStates newState )
{
  if (newState.testFlag(Qt::WindowActive))
    m_mantidUI->appWindow()->enableMantidPeakFit(true);
  else
    m_mantidUI->appWindow()->enableMantidPeakFit(false);
}

void PeakPickerTool::functionChanged()
{
}

FitPropertyBrowser* PeakPickerTool::fitBrowser()
{
  return m_mantidUI->fitFunctionBrowser();
}


//--------------------------------------------------------
//      PeakRangeMarker methods
//--------------------------------------------------------

PeakRangeMarker::PeakRangeMarker()
:m_fnName("Gaussian"),m_width(),m_current(-1),m_width_set(true),m_resetting(false),
  m_changingXMin(true),m_changingXMax(true)
{
  setFunction(Mantid::API::FunctionFactory::Instance().createUnwrapped("CompositeFunction"));
}

void PeakRangeMarker::draw(QPainter *p, 
                  const QwtScaleMap &xMap, const QwtScaleMap &yMap,
                  const QRect &) const
{
  //for(int i=0;i<m_params.size();i++)
  //{
  //  double c = m_params[i].centre;
  //  if (c >= xMap.s1() && c <= xMap.s2())
  //  {
  //    int ic = xMap.transform(c);
  //    if (i == m_current)
  //    {
  //      double width = m_params[i].width;
  //      QPen pen;
  //      pen.setColor(QColor(255,0,0));
  //      pen.setStyle(Qt::DashLine);
  //      p->setPen(pen);
  //      int x1 = xMap.transform(c - width/2);
  //      int x2 = xMap.transform(c + width/2);
  //      QwtPainter::drawLine(p, x1, yMap.p1(), x1,yMap.p2());
  //      QwtPainter::drawLine(p, x2, yMap.p1(), x2,yMap.p2());

  //      pen.setStyle(Qt::SolidLine);
  //      p->setPen(pen);
  //      int ih = yMap.transform(height());
  //      QwtPainter::drawLine(p, ic, yMap.p2(), ic, ih);
  //    }
  //    else
  //    {
  //      p->setPen(QPen(QColor(200,200,200)));
  //      QwtPainter::drawLine(p, ic, yMap.p1(), ic, yMap.p2());
  //    }
  //  }
  //}
  //QPen pen;
  //pen.setColor(QColor(0,0,255));
  //pen.setStyle(Qt::DashLine);
  //p->setPen(pen);
  //int x1 = xMap.transform(xMin());
  //int x2 = xMap.transform(xMax());
  //QwtPainter::drawLine(p, x1, yMap.p1(), x1,yMap.p2());
  //QwtPainter::drawLine(p, x2, yMap.p1(), x2,yMap.p2());

  //pen.setColor(QColor(0,0,255));
  //pen.setStyle(Qt::SolidLine);
  //p->setPen(pen);
  //QwtPainter::drawLine(p, x1, yMap.p1(), x1+3,yMap.p1());
  //QwtPainter::drawLine(p, x1, yMap.p2(), x1+3,yMap.p2());

  //QwtPainter::drawLine(p, x2, yMap.p1(), x2-3,yMap.p1());
  //QwtPainter::drawLine(p, x2, yMap.p2(), x2-3,yMap.p2());

}
void PeakRangeMarker::add(double c,double h)
{
  int idx = -1;
  //for(int i=0;i<m_params.size();i++)
  //{
  //  if (m_params[i].centre == c)
  //  {
  //    idx = i;
  //    break;
  //  }
  //}
  //if (idx < 0)
  //{
  //  PeakParams pp = PeakParams(c,h,m_width);
  //  pp.fnName = m_fnName;
  //  m_params.append(pp);
  //  m_current = m_params.size() - 1;
  //}
  //else
  //  m_current = idx;
}

// Return the centre of the currently selected peak
double PeakRangeMarker::centre()const
{
  return 0.;//m_current>=0?m_params[m_current].centre:0;
}

// Return the width of the currently selected peak
double PeakRangeMarker::width()const
{
  return 0.;//m_current>=0?m_params[m_current].width:m_width;
}

// Return the height of the currently selected peak
double PeakRangeMarker::height()const
{
  return 0.;//m_current>=0?m_params[m_current].height:0;
}

// Check if the width is been set
bool PeakRangeMarker::isWidthSet()const
{
  return m_width_set;
}

// Set the width set flag
void PeakRangeMarker::widthIsSet(bool yes)
{
  m_width_set = yes;
}

// Change the width of the currently selected peak
void PeakRangeMarker::setWidth(double x)
{
  m_width = x;
  //if (m_current>=0)
  //  m_params[m_current].width = x;
}

// Return current function name
std::string PeakRangeMarker::fnName()const
{
  return "";//m_current>=0?m_params[m_current].fnName:m_fnName;
}

// Set new function name
void PeakRangeMarker::fnName(const std::string& name)
{
  if (m_current>=0) 
  {
    //m_params[m_current].fnName = name;
    //m_fnName = name;
  }
}
