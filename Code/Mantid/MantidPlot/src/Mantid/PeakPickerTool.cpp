#include "PeakPickerTool.h"
#include "MantidCurve.h"
#include "MantidUI.h"
#include "FitPropertyBrowser.h"
#include "../FunctionCurve.h"
#include "PropertyHandler.h"

#include "MantidAPI/CompositeFunction.h"
#include "MantidAPI/IPeakFunction.h"
#include "MantidAPI/FunctionFactory.h"

#include "qwt_painter.h"
#include <QPainter>
#include <QList>
#include <QToolBar>
#include <QAction>
#include <QMenu>
#include <QInputDialog>

#include <iostream>

PeakPickerTool::PeakPickerTool(Graph *graph, MantidUI *mantidUI) :
QwtPlotPicker(graph->plotWidget()->canvas()),
PlotToolInterface(graph),
m_mantidUI(mantidUI),m_wsName(),m_spec(),m_init(false),//m_current(0),
m_width_set(true),m_width(0),m_addingPeak(false),m_resetting(false)
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
        m_wsName = mcurve->workspaceName();
        m_spec = mcurve->workspaceIndex();
      }
    }
  }
  fitBrowser()->getHandler()->removeAllPlots();
  fitBrowser()->setWorkspaceName(m_wsName);
  fitBrowser()->setWorkspaceIndex(m_spec);
  connect(fitBrowser(),SIGNAL(currentChanged()),this,SLOT(currentChanged()));
  connect(fitBrowser(),SIGNAL(workspaceIndexChanged(int)),this,SLOT(workspaceIndexChanged(int)));
  connect(fitBrowser(),SIGNAL(workspaceNameChanged(const QString&)),
                  this,SLOT(workspaceNameChanged(const QString&)));
  connect(fitBrowser(),SIGNAL(functionRemoved()),this,SLOT(functionRemoved()));
  connect(fitBrowser(),SIGNAL(functionCleared()),this,SLOT(functionCleared()));
  connect(fitBrowser(),SIGNAL(algorithmFinished(const QString&)),
                  this,SLOT(algorithmFinished(const QString&)));
  connect(fitBrowser(),SIGNAL(startXChanged(double)),this,SLOT(startXChanged(double)));
  connect(fitBrowser(),SIGNAL(endXChanged(double)),this,SLOT(endXChanged(double)));
  connect(fitBrowser(),SIGNAL(parameterChanged(const Mantid::API::IFitFunction*)),
                  this,SLOT(parameterChanged(const Mantid::API::IFitFunction*)));
  connect(fitBrowser(),SIGNAL(plotGuess()),this,SLOT(plotGuess()));
  connect(fitBrowser(),SIGNAL(plotCurrentGuess()),this,SLOT(plotCurrentGuess()));
  connect(fitBrowser(),SIGNAL(removeGuess()),this,SLOT(removeGuess()));
  connect(fitBrowser(),SIGNAL(removeCurrentGuess()),this,SLOT(removeCurrentGuess()));

  m_mantidUI->showFitPropertyBrowser();
  connect(this,SIGNAL(isOn(bool)),fitBrowser(),SLOT(setPeakToolOn(bool)));
  emit isOn(true);

  Mantid::API::CompositeFunction* cf = fitBrowser()->compositeFunction();
  if (fitBrowser()->count() == 0 || (fitBrowser()->count() == 1 && fitBrowser()->isAutoBack()))
  {
    m_init = true;
    QwtScaleMap xMap = d_graph->plotWidget()->canvasMap(QwtPlot::xBottom);
    double s1 = xMap.s1(), s2 = xMap.s2();
    double ds = fabs(s2-s1)*0.05;
    xMin(s1 + ds);
    xMax(s2 - ds);
    m_changingXMin = false;
    m_changingXMax = false;
    fitBrowser()->setStartX(xMin());
    fitBrowser()->setEndX(xMax());
    if (fitBrowser()->isAutoBack())
    {
      fitBrowser()->addAutoBackground();
    }
  }
  else
  {
    m_init = true;
    xMin(fitBrowser()->startX());
    xMax(fitBrowser()->endX());
    m_changingXMin = false;
    m_changingXMax = false;
    for(int i=0;i<cf->nFunctions();i++)
    {
      Mantid::API::IPeakFunction* pf = dynamic_cast<Mantid::API::IPeakFunction*>(cf->getFunction(i));
      if (pf)
      {
        m_width = pf->width();
        if (m_width != 0.) break;
      }
    }
  }
  attach(d_graph->plotWidget());
  d_graph->plotWidget()->replot();

  connect(d_graph,SIGNAL(curveRemoved()),this,SLOT(curveRemoved()));
  connect(d_graph,SIGNAL(modifiedGraph()),this,SLOT(modifiedGraph()));

  m_ws = boost::dynamic_pointer_cast<Mantid::API::MatrixWorkspace>(
    Mantid::API::AnalysisDataService::Instance().retrieve(
      m_wsName.toStdString()
    ));
}

PeakPickerTool::~PeakPickerTool()
{
  disconnect(d_graph,SIGNAL(curveRemoved()),this,SLOT(curveRemoved()));
  detach();
  d_graph->plotWidget()->canvas()->unsetCursor();
  d_graph->plotWidget()->replot();
  emit isOn(false);
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
        QPoint p = ((QMouseEvent*)event)->pos();
        double x = d_graph->plotWidget()->invTransform(2,p.x());
        double x1 = d_graph->plotWidget()->invTransform(2,p.x()+3);
        double dx = fabs(x-x1);
        if (clickedOnXMin(x,dx) || clickedOnXMax(x,dx))
        {
          xMin(x-dx);
          xMax(x+dx);
          d_graph->plotWidget()->replot();
        }
        return true;
      }

    case QEvent::MouseMove:
      {
        QPoint pnt = ((QMouseEvent*)event)->pos();
        Qt::KeyboardModifiers mod = ((QMouseEvent*)event)->modifiers();
        if (!isWidthSet())
        {
          double c = centre();
          double w = d_graph->plotWidget()->invTransform(2,pnt.x()) - c;
          setWidth(2*fabs(w));
          fitBrowser()->updateParameters();
          emit peakChanged();
        }
        else if (resetting())
        {
          double c = d_graph->plotWidget()->invTransform(2,pnt.x());
          int yAxis = QwtPlot::yLeft;
          double h = d_graph->plotWidget()->invTransform(yAxis,pnt.y());
          setPeak(c,h);
          fitBrowser()->updateParameters();
          emit peakChanged();
        }
        else if (changingXMin() && changingXMax())
        {// modify xMin and xMax at the same time 
          setToolTip("");
          double x = d_graph->plotWidget()->invTransform(2,pnt.x());
          double x0 = (xMin() + xMax())/2;
          double xmin,xmax;
          if (x >= x0)
          {
            xmax = x;
            xmin = x0*2-x;
          }
          else
          {
            xmax = x0*2-x;
            xmin = x;
          }
          xMin(xmin);
          xMax(xmax);
          fitBrowser()->setStartX(xMin());
          fitBrowser()->setEndX(xMax());
        }
        else if (changingXMin())
        {
          double x = d_graph->plotWidget()->invTransform(2,pnt.x());
          xMin(x);
          fitBrowser()->setStartX(xMin());
        }
        else if (changingXMax())
        {
          double x = d_graph->plotWidget()->invTransform(2,pnt.x());
          xMax(x);
          fitBrowser()->setEndX(xMax());
        }
        d_graph->plotWidget()->replot();
        //}
        break;
      }

    case QEvent::MouseButtonPress:
      {
        Qt::KeyboardModifiers mod = ((QMouseEvent*)event)->modifiers();
        QPoint p = ((QMouseEvent*)event)->pos();
        if (((QMouseEvent*)event)->button() == Qt::LeftButton )
        {
          if ( ! m_init )
          {// Create the marker
            m_init = true;
            attach(d_graph->plotWidget());
            double x = d_graph->plotWidget()->invTransform(2,p.x());
            // when PeakRangeMarker is created chngingxMin() and changingXMax are both true
            xMin(x);
            xMax(x);
            fitBrowser()->setStartX(xMin());
            fitBrowser()->setEndX(xMax());
            d_graph->plotWidget()->replot();
          }
          else if (m_addingPeak)
          {
            addPeakAt(p.x(),p.y());
            m_addingPeak = false;
            d_graph->plotWidget()->canvas()->setCursor(Qt::pointingHandCursor);
            setToolTip("");
          }
          else if ( mod.testFlag(Qt::ShiftModifier) )// Shift button was pressed
          {
            // Add a new peak
            addPeakAt(p.x(),p.y());
          }
          else // No shift button
          {
            widthIsSet();
            double x = d_graph->plotWidget()->invTransform(2,p.x());
            double x1 = d_graph->plotWidget()->invTransform(2,p.x()+3);
            PropertyHandler* handler = clickedOnCentreMarker(x,fabs(x1-x));
            if (clickedOnXMax(x,fabs(x1-x)))
            {// begin changing xMax
              changingXMax(true);
              d_graph->plotWidget()->canvas()->setCursor(Qt::SizeHorCursor);
              d_graph->plotWidget()->replot();
              fitBrowser()->setStartX(xMin());
            }
            if (clickedOnXMin(x,fabs(x1-x)))
            {// begin changing xMin
              changingXMin(true);
              d_graph->plotWidget()->canvas()->setCursor(Qt::SizeHorCursor);
              d_graph->plotWidget()->replot();
              fitBrowser()->setEndX(xMax());
            }
            if (clickedOnWidthMarker(x,fabs(x1-x)))
            {// begin changing width
              widthIsSet(false);
              d_graph->plotWidget()->canvas()->setCursor(Qt::SizeHorCursor);
              d_graph->plotWidget()->replot();
              emit peakChanged();
            }
            else if (handler)
            {// select current, begin changing centre and height
              fitBrowser()->setCurrentFunction(handler);
              d_graph->plotWidget()->replot();
              resetting(true);
              emit peakChanged();
            }
          }
        }
        return true;
      }

      // Mouse button up - stop all changes
    case QEvent::MouseButtonRelease:
      d_graph->plotWidget()->canvas()->setCursor(Qt::pointingHandCursor);
      widthIsSet();
      if ((m_changingXMin || m_changingXMax) && fitBrowser()->isAutoBack())
      {
        fitBrowser()->refitAutoBackground();
      }
      resetting(false);
      changingXMin(false);
      changingXMax(false);
      m_addingPeak = false;
      fitBrowser()->setStartX(xMin());
      fitBrowser()->setEndX(xMax());
      //if (current() >= 0 && m_width == 0.)
      //{
      //  setToolTip("Click and drag the red line to set the peak width");
      //}
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

void PeakPickerTool::windowStateChanged( Qt::WindowStates, Qt::WindowStates newState )
{
  (void) newState;
}

void PeakPickerTool::functionCleared()
{
  d_graph->plotWidget()->replot();
}

FitPropertyBrowser* PeakPickerTool::fitBrowser()const
{
  return m_mantidUI->fitFunctionBrowser();
}

void PeakPickerTool::draw(QPainter *p, const QwtScaleMap &xMap, const QwtScaleMap &yMap, const QRect &) const
{
  try
  {
    PropertyHandler* h = fitBrowser()->getHandler();
    if ( !h ) return;
    QList<PropertyHandler*> peaks = h->getPeakList();
    foreach(PropertyHandler* peak,peaks)
    {
      double c = peak->centre();
      if (c >= xMap.s1() && c <= xMap.s2())
      {
        int ic = xMap.transform(c);
        if (peak == fitBrowser()->currentHandler())
        {// draw current peak
          double width = peak->width();
          QPen pen;
          pen.setColor(QColor(255,0,0));
          pen.setStyle(Qt::DashLine);
          p->setPen(pen);
          int x1 = xMap.transform(c - width/2);
          int x2 = xMap.transform(c + width/2);
          QwtPainter::drawLine(p, x1, yMap.p1(), x1,yMap.p2());
          QwtPainter::drawLine(p, x2, yMap.p1(), x2,yMap.p2());

          pen.setStyle(Qt::SolidLine);
          p->setPen(pen);
          int ih = yMap.transform(peak->height()+peak->base());
          int ib = yMap.transform(peak->base());
          QwtPainter::drawLine(p, ic, ib, ic, ih);
          QwtPainter::drawLine(p, x1, ib, x2, ib);
        }
        else
        {
          p->setPen(QPen(QColor(200,200,200)));
          QwtPainter::drawLine(p, ic, yMap.p1(), ic, yMap.p2());
        }
      }
    }
  }
  catch(...)
  {
    // Do nothing
  }
  QPen pen;
  pen.setColor(QColor(0,0,255));
  pen.setStyle(Qt::DashLine);
  p->setPen(pen);
  int x1 = xMap.transform(xMin());
  int x2 = xMap.transform(xMax());
  QwtPainter::drawLine(p, x1, yMap.p1(), x1,yMap.p2());
  QwtPainter::drawLine(p, x2, yMap.p1(), x2,yMap.p2());

  pen.setColor(QColor(0,0,255));
  pen.setStyle(Qt::SolidLine);
  p->setPen(pen);
  QwtPainter::drawLine(p, x1, yMap.p1(), x1+3,yMap.p1());
  QwtPainter::drawLine(p, x1, yMap.p2(), x1+3,yMap.p2());

  QwtPainter::drawLine(p, x2, yMap.p1(), x2-3,yMap.p1());
  QwtPainter::drawLine(p, x2, yMap.p2(), x2-3,yMap.p2());

}

// Add a new peak with centre c and height h. 
void PeakPickerTool::addPeak(double c,double h)
{
  std::string fnName = fitBrowser()->defaultPeakType();
  PropertyHandler* handler = fitBrowser()->addFunction(fnName);
  if (!handler || !handler->pfun()) return;
  handler->setCentre(c);
  double width = handler->width();
  if (width == 0)
  {
    handler->setWidth(m_width);
  }
  if (handler->width() > 0.)
  {
    handler->calcBase();
  }
  handler->setHeight(h);
}

// Give new centre and height to the current peak
void PeakPickerTool::setPeak(double c,double h)
{
  PropertyHandler* handler = fitBrowser()->currentHandler();
  if ( ! handler ) return;
  handler->setCentre(c);
  handler->calcBase();
  handler->setHeight(h);
}

// Return the centre of the currently selected peak
double PeakPickerTool::centre()const
{
  PropertyHandler* h = fitBrowser()->currentHandler();
  return h? h->centre(): 0;
}

// Return the width of the currently selected peak
double PeakPickerTool::width()const
{
  PropertyHandler* h = fitBrowser()->currentHandler();
  return h? h->width(): 0;
}

// Return the height of the currently selected peak
double PeakPickerTool::height()const
{
  PropertyHandler* h = fitBrowser()->currentHandler();
  return h? h->width(): 0;
}

// Change the width of the currently selected peak
void PeakPickerTool::setWidth(double x)
{
  PropertyHandler* h = fitBrowser()->currentHandler();
  if (!h || !h->pfun()) return;
  m_width = x;
  h->setWidth(x);
  double height = h->height() + h->base();
  h->calcBase();
  h->setHeight(height);
  setToolTip("");
}

// Check if x is near the xMin marker (+-dx)
bool PeakPickerTool::clickedOnXMin(double x,double dx)
{
  double c = xMin();
  return (fabs(x - c) <= dx);
}

// Check if x is near the xMax marker (+-dx)
bool PeakPickerTool::clickedOnXMax(double x,double dx)
{
  double c = xMax();
  return (fabs(x - c) <= dx);
}

// Check if x is near a width marker (+-dx)
bool PeakPickerTool::clickedOnWidthMarker(double x,double dx)
{
  PropertyHandler* h = fitBrowser()->currentHandler();
  if (!h) return false;
  double c = h->centre();
  double w = h->width()/2;
  return (fabs(x - c - w) <= dx) || (fabs(x - c + w) <= dx);
}

// Check if x is near a peak centre marker (+-dx). If true returns the peak's address or 0 otherwise.
PropertyHandler* PeakPickerTool::clickedOnCentreMarker(double x,double dx)const
{
  QList<PropertyHandler*> peaks = fitBrowser()->getHandler()->getPeakList();
  foreach(PropertyHandler* peak,peaks)
  {
    if (fabs(x - peak->centre()) <= dx)
    {
      return peak;
    }
  }
  return 0;
}

// Lower fit boundary
void PeakPickerTool::xMin(double x)
{
  m_xMin = x;
  if (x > m_xMax) 
  {
    m_xMax = x;
  }
}

// Upper fit boundary
void PeakPickerTool::xMax(double x) 
{
  m_xMax = x;
  if (x < m_xMin)
  {
    m_xMin = x;
  }
}

/**
 * Slot. Reacts on the index change in the Fit Browser.
 * @param i The new function index.
 */
void PeakPickerTool::currentChanged()
{
  d_graph->plotWidget()->replot();
}

/**
 * Slot. Reacts on the function deletion in the Fit Browser.
 * @param f The address of the deleted function.
 */
void PeakPickerTool::functionRemoved()
{
  d_graph->plotWidget()->replot();
}

/**
 * Slot. Called when the Fit algorithm finishes. 
 * @param out The name of the output workspace with the results of the fit.
 */
void PeakPickerTool::algorithmFinished(const QString& out)
{
  QString axisLabel = QString::fromStdString(m_ws->getAxis(1)->label(spec()));
  QString curveFitName = workspaceName()+"-"+axisLabel+QString("-Calc");
  QString curveResName = workspaceName()+"-"+axisLabel+QString("-Diff");

  graph()->removeCurve(curveFitName);
  graph()->removeCurve(curveResName);

  new MantidCurve(curveFitName,out,graph(),"spectra",1,false);
  new MantidCurve(curveResName,out,graph(),"spectra",2,false);

  graph()->replot();
}

/**
 * Slot. Called when the workspace index is changed in the FitBrowser
 * @param i The new workspace index.
 */
void PeakPickerTool::workspaceIndexChanged(int i)
{
  if (i != m_spec)
  {
    fitBrowser()->setWorkspaceIndex(m_spec);
  }
}

/**
  * Slot. Called when the workspace name is changed in the FitBrowser.
  * It doesn't allow changing the workspace name unless it is a name of
  * the workspace group containing m_wsName
  * @param wsName The new workspace name.
  */
void PeakPickerTool::workspaceNameChanged(const QString& wsName)
{
  if (wsName != m_wsName)
  {
    Mantid::API::Workspace_sptr ws = m_mantidUI->getWorkspace(wsName);
    Mantid::API::WorkspaceGroup_sptr wsg = 
      boost::dynamic_pointer_cast<Mantid::API::WorkspaceGroup>(ws);
    if (wsg)
    {
      std::vector<std::string> names = wsg->getNames();
      for(int i=0;i<static_cast<int>(names.size());++i)
      {
        if (names[i] == m_wsName.toStdString()) 
        {// accept the new name
          return;
        }
      }
      // reject the new name
      fitBrowser()->setWorkspaceName(m_wsName);
    }
    else
    {// reject the new name
      fitBrowser()->setWorkspaceName(m_wsName);
    }
  }
}

/**
 * Slot. Called when the startX changed in the FitBrowser
 * @param sX The new startX
 */
void PeakPickerTool::startXChanged(double sX)
{
  xMin(sX);
  graph()->replot();
}

/**
 * Slot. Called when the endX changed in the FitBrowser
 * @param eX The new endX
 */
void PeakPickerTool::endXChanged(double eX)
{
  xMax(eX);
  graph()->replot();
}

/**
 * Slot. Called in response to parameterChanged signal from FitBrowser
 * @param f The pointer to the function with the changed parameter
 */
void PeakPickerTool::parameterChanged(const Mantid::API::IFitFunction* f)
{
  PropertyHandler* theHandler = fitBrowser()->getHandler();
  PropertyHandler* h = theHandler->findHandler(f);
  if (!h) return;
  h->replot();
  if (h != theHandler && theHandler->hasPlot())
  {
    theHandler->replot();
  }
  graph()->replot();
}

/**
 * Adds commands specific to the tool to a context menu
 * @param menu A reference to the context menu
 */
void PeakPickerTool::prepareContextMenu(QMenu& menu)
{
  QAction *action = new QAction("Add peak...",this);
  connect(action,SIGNAL(triggered()),this,SLOT(addPeak()));
  menu.addAction(action);

  action = new QAction("Add background...",this);
  connect(action,SIGNAL(triggered()),this,SLOT(addBackground()));
  menu.addAction(action);

  action = new QAction("Add other function...",this);
  connect(action,SIGNAL(triggered()),this,SLOT(addOther()));
  menu.addAction(action);

  menu.addSeparator();

  if (fitBrowser()->count()>0)
  {
    if (fitBrowser()->getHandler()->hasPlot())
    {
      action = new QAction("Remove guess",this);
      connect(action,SIGNAL(triggered()),this,SLOT(removeGuess()));
      menu.addAction(action);
    }
    else
    {
      action = new QAction("Plot guess",this);
      connect(action,SIGNAL(triggered()),this,SLOT(plotGuess()));
      menu.addAction(action);
    }

    PropertyHandler* h = fitBrowser()->currentHandler();
    if (h && h->pfun())
    {
      if (h->hasPlot())
      {
        action = new QAction("Remove guess for this peak",this);
        connect(action,SIGNAL(triggered()),this,SLOT(removeCurrentGuess()));
        menu.addAction(action);
      }
      else
      {
        action = new QAction("Plot guess for this peak",this);
        connect(action,SIGNAL(triggered()),this,SLOT(plotCurrentGuess()));
        menu.addAction(action);
      }

      menu.addSeparator();

      action = new QAction("Remove peak",this);
      connect(action,SIGNAL(triggered()),this,SLOT(deletePeak()));
      menu.addAction(action);

    }
  }

  action = new QAction("Reset range",this);
  connect(action,SIGNAL(triggered()),this,SLOT(resetRange()));
  menu.addAction(action);

  action = new QAction("Clear all",this);
  connect(action,SIGNAL(triggered()),this,SLOT(clear()));
  menu.addAction(action);

  menu.addSeparator();

  if (fitBrowser()->isFitEnabled())
  {
    action = new QAction("Fit",this);
    connect(action,SIGNAL(triggered()),this,SLOT(fit()));
    menu.addAction(action);
  }

  if (fitBrowser()->isUndoEnabled())
  {
    action = new QAction("Undo fit",this);
    connect(action,SIGNAL(triggered()),this,SLOT(undoFit()));
    menu.addAction(action);
  }

}

/**
 * Slot. Adds a peak
 */
void PeakPickerTool::addPeak()
{
  int i = fitBrowser()->registeredPeaks().indexOf(QString::fromStdString(fitBrowser()->defaultPeakType()));
  bool ok = false;
  QString fnName = 
    QInputDialog::getItem(m_mantidUI->appWindow(), "MantidPlot - Fit", "Select peak type", fitBrowser()->registeredPeaks(),i,false,&ok);
  if (ok)
  {
    fitBrowser()->setDefaultPeakType(fnName.toStdString());
    m_addingPeak = true;
    d_graph->plotWidget()->canvas()->setCursor(Qt::CrossCursor);
    setToolTip("Click to add the peak");
  }
}

/**
 * 
 */
void PeakPickerTool::addPeakAt(int x,int y)
{
  // x - axis is #2, y - is #0
  double c = d_graph->plotWidget()->invTransform(2,x);
  double h = d_graph->plotWidget()->invTransform(0,y);
  addPeak(c,h);
  emit peakChanged();
  d_graph->plotWidget()->replot();
  fitBrowser()->updateParameters();
}

/**
 * Slot. Deletes the current peak
 */
void PeakPickerTool::deletePeak()
{
  PropertyHandler* h = fitBrowser()->currentHandler();
  if (!h) return;
  h->removeFunction();
  functionRemoved();
}

/**
 * Slot. Start the fit
 */
void PeakPickerTool::fit()
{
  fitBrowser()->fit();
}

/**
 * Slot. Add a background function
 */
void PeakPickerTool::addBackground()
{
  bool ok = false;
  QString fnName = 
    QInputDialog::getItem(m_mantidUI->appWindow(), 
         "MantidPlot - Fit", "Select background type", 
         fitBrowser()->registeredBackgrounds(),
         fitBrowser()->registeredBackgrounds().indexOf("LinearBackground"),
         false,&ok);
  if (ok)
  {
    if (fnName == "LinearBackground")
    {
      fitBrowser()->setAutoBackgroundName(fnName);
      fitBrowser()->addAutoBackground();
    }
    else
    {
      fitBrowser()->addFunction(fnName.toStdString());
    }
  }
}

/**
 * Slot. Add a function that is neither peak nor background
 */
void PeakPickerTool::addOther()
{
  bool ok = false;
  QString fnName = 
    QInputDialog::getItem(m_mantidUI->appWindow(), 
         "MantidPlot - Fit", "Select function type", 
         fitBrowser()->registeredOthers(),0,
         false,&ok);
  if (ok)
  {
    fitBrowser()->addFunction(fnName.toStdString());
  }
}

/**
 * Slot. Undo the fit
 */
void PeakPickerTool::undoFit()
{
  fitBrowser()->undoFit();
}

/**
 * Slot. Clear all functions
 */
void PeakPickerTool::clear()
{
  fitBrowser()->clear();
}

/** Set the tool tip text
 * @param tst The tip text
 */
void PeakPickerTool::setToolTip(const QString& txt)
{
  d_graph->setToolTip(txt);
  fitBrowser()->setTip(txt);
}

/**
 * Slot. Plot the initial guess for the function
 */
void PeakPickerTool::plotGuess()
{
  fitBrowser()->getHandler()->plot(d_graph);
  d_graph->replot();
}

void PeakPickerTool::plotCurrentGuess()
{
  PropertyHandler* h = fitBrowser()->currentHandler();
  if (h)
  {
    h->plot(d_graph);
    d_graph->replot();
  }
}

/**
 * Slot. Remove the plot of the i-th function
 */
void PeakPickerTool::removeGuess()
{
  fitBrowser()->getHandler()->removePlot();
  d_graph->replot();
}

/**
 * Slot. Remove the plot of the i-th function
 */
void PeakPickerTool::removeCurrentGuess()
{
  PropertyHandler* h = fitBrowser()->currentHandler();
  if (h)
  {
    h->removePlot();
    d_graph->replot();
  }
}

void PeakPickerTool::curveRemoved()
{
  d_graph->replot();
}

void PeakPickerTool::resetRange()
{
  QwtScaleMap xMap = d_graph->plotWidget()->canvasMap(QwtPlot::xBottom);
  double s1 = xMap.s1(), s2 = xMap.s2();
  double ds = fabs(s2-s1)*0.05;
  xMin(s1 + ds);
  xMax(s2 - ds);
  fitBrowser()->setStartX(xMin());
  fitBrowser()->setEndX(xMax());
  if (fitBrowser()->isAutoBack())
  {
    fitBrowser()->addAutoBackground();
  }
  d_graph->replot();
}

void PeakPickerTool::modifiedGraph()
{
}

