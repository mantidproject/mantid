#include "PeakPickerTool.h"
#include "MantidCurve.h"
#include "MantidUI.h"
#include "FitPropertyBrowser.h"
#include "../FunctionCurve.h"

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
m_mantidUI(mantidUI),m_wsName(),m_spec(),m_init(false),m_current(-1),
m_width_set(true),m_width(0),m_addingPeak(false),m_resetting(false),
m_defaultPeakName("Gaussian")
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
  fitBrowser()->setWorkspaceName(m_wsName);
  fitBrowser()->setWorkspaceIndex(m_spec);
  connect(fitBrowser(),SIGNAL(indexChanged(int)),this,SLOT(indexChanged(int)));
  connect(fitBrowser(),SIGNAL(workspaceIndexChanged(int)),this,SLOT(workspaceIndexChanged(int)));
  connect(fitBrowser(),SIGNAL(workspaceNameChanged(const QString&)),this,SLOT(workspaceNameChanged(const QString&)));
  connect(fitBrowser(),SIGNAL(functionRemoved(int)),this,SLOT(functionRemoved(int)));
  connect(fitBrowser(),SIGNAL(functionCleared()),this,SLOT(functionCleared()));
  connect(fitBrowser(),SIGNAL(algorithmFinished(const QString&)),this,SLOT(algorithmFinished(const QString&)));
  connect(fitBrowser(),SIGNAL(startXChanged(double)),this,SLOT(startXChanged(double)));
  connect(fitBrowser(),SIGNAL(endXChanged(double)),this,SLOT(endXChanged(double)));
  connect(fitBrowser(),SIGNAL(parameterChanged(int)),this,SLOT(parameterChanged(int)));
  connect(fitBrowser(),SIGNAL(plotGuess(int)),this,SLOT(plotGuess(int)));

  m_mantidUI->showFitPropertyBrowser();
  connect(this,SIGNAL(isOn(bool)),fitBrowser(),SLOT(setPeakToolOn(bool)));
  
  emit isOn(true);
  if (fitBrowser()->count() == 0)
  {
    setToolTip("Click and drag to set the fitting region");
    m_changingXMin = true;
    m_changingXMax = true;
  }
  else
  {
    m_init = true;
    xMin(fitBrowser()->startX());
    xMax(fitBrowser()->endX());
    m_changingXMin = false;
    m_changingXMax = false;
    setCurrent(fitBrowser()->index());
    attach(d_graph->plotWidget());
    d_graph->plotWidget()->replot();
  }
}

PeakPickerTool::~PeakPickerTool()
{
  QMap<int,FunctionCurve*>::iterator it = m_guessCurves.begin();
  for(;it!=m_guessCurves.end();it++)
  {
    d_graph->removeCurve(it.value());
  }
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
        //if (m_range)
        //{
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
        //}
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
          double h = d_graph->plotWidget()->invTransform(0,pnt.y());
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
            int ic = clickedOnCentreMarker(x,fabs(x1-x));
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
            else if (ic >= 0)
            {// select current, begin changing centre and height
              //setCurrent(ic);
              fitBrowser()->selectFunction(ic);
              //d_graph->plotWidget()->replot();
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
      resetting(false);
      changingXMin(false);
      changingXMax(false);
      m_addingPeak = false;
      fitBrowser()->setStartX(xMin());
      fitBrowser()->setEndX(xMax());
      if (current() >= 0 && m_width == 0.)
      {
        setToolTip("Click and drag the red line to set the peak width");
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
    if (fitBrowser()->compositeFunction())
    {
      for(int iFun=0;iFun < fitBrowser()->compositeFunction()->nFunctions();iFun++)
      {
        Mantid::API::IPeakFunction* pf = 
          dynamic_cast<Mantid::API::IPeakFunction*>(fitBrowser()->compositeFunction()->getFunction(iFun));
        if (!pf) continue;
        double c = pf->centre();
        if (c >= xMap.s1() && c <= xMap.s2())
        {
          int ic = xMap.transform(c);
          if (iFun == fitBrowser()->index())
          {
            double width = pf->width();
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
            int ih = yMap.transform(pf->height());
            QwtPainter::drawLine(p, ic, yMap.p2(), ic, ih);
          }
          else
          {
            p->setPen(QPen(QColor(200,200,200)));
            QwtPainter::drawLine(p, ic, yMap.p1(), ic, yMap.p2());
          }
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
  std::string fnName = fitBrowser()->isPeak()? 
    fitBrowser()->defaultFunctionType() : m_defaultPeakName;
  fitBrowser()->addFunction(fnName);
  fitBrowser()->setCentre(c);
  fitBrowser()->setHeight(h);
  fitBrowser()->setWidth(m_width);
  setCurrent(fitBrowser()->index());
}

// Give new centre and height to the current peak
void PeakPickerTool::setPeak(double c,double h)
{
  if ( ! fitBrowser()->isPeak() )
  {
    addPeak(c,h);
    return;
  }
  fitBrowser()->setCentre(c);
  fitBrowser()->setHeight(h);
  setCurrent( fitBrowser()->index() );
}

// Return the centre of the currently selected peak
double PeakPickerTool::centre()const
{
  return m_current>=0?fitBrowser()->centre():0;
}

// Return the width of the currently selected peak
double PeakPickerTool::width()const
{
  return m_current>=0?fitBrowser()->width():0;
}

// Return the height of the currently selected peak
double PeakPickerTool::height()const
{
  return m_current>=0?fitBrowser()->height():0;
}

// Change the width of the currently selected peak
void PeakPickerTool::setWidth(double x)
{
    m_width = x;
  if (m_current>=0)
    fitBrowser()->setWidth(x);
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
  if (m_current < 0) return false;
  double c = centre();
  double w = width()/2;
  return (fabs(x - c - w) <= dx) || (fabs(x - c + w) <= dx);
}

// Check if x is near a peak centre marker (+-dx). If true returns the peak's index or -1 otherwise.
int PeakPickerTool::clickedOnCentreMarker(double x,double dx)
{
  for(int i=0;i<fitBrowser()->count();i++)
  {
    Mantid::API::IPeakFunction* pf = fitBrowser()->peakFunction(i);
    if (pf && fabs(x - pf->centre()) <= dx)
    {
      return i;
    }
  }
  return -1;
}

// Change current peak
void PeakPickerTool::setCurrent(int i)
{
  m_current = i;
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

/** Set the default peak function
 */
void PeakPickerTool::setDefaultPeakName(const std::string& fnName)
{
  boost::shared_ptr<Mantid::API::IPeakFunction> f = 
    boost::dynamic_pointer_cast<Mantid::API::IPeakFunction>(
       Mantid::API::FunctionFactory::Instance().create(fnName)
    );
  if (f)
  {
    m_defaultPeakName = fnName;
  }
}

/**
 * Slot. Reacts on the index change in the Fit Browser.
 * @param i The new function index.
 */
void PeakPickerTool::indexChanged(int i)
{
  int j = fitBrowser()->isPeak() ? i : -1;
  setCurrent(j);
  d_graph->plotWidget()->replot();
}

/**
 * Slot. Reacts on the function deletion in the Fit Browser.
 * @param i The index of the deleted function.
 */
void PeakPickerTool::functionRemoved(int i)
{
  QMap<int,FunctionCurve*>::iterator it = m_guessCurves.find(i);
  if (it != m_guessCurves.end())
  {
    d_graph->removeCurve(it.value());
    m_guessCurves.erase(it);
  }
  int j = fitBrowser()->isPeak() ? fitBrowser()->index() : -1;
  setCurrent(j);
  d_graph->plotWidget()->replot();
}

/**
 * Slot. Called when the Fit algorithm finishes. 
 * @param out The name of the output workspace with the results of the fit.
 */
void PeakPickerTool::algorithmFinished(const QString& out)
{
  QString curveFitName = workspaceName()+QString("-fit-")+QString::number(spec());
  QString curveResName = workspaceName()+QString("-res-")+QString::number(spec());

  graph()->removeCurve(curveFitName);
  graph()->removeCurve(curveResName);

  MantidCurve* c1 = new MantidCurve(curveFitName,out,graph(),"spectra",1,false);
  MantidCurve* c2 = new MantidCurve(curveResName,out,graph(),"spectra",2,false);

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
 * Slot. Called when the workspace name is changed in the FitBrowser
 * @param wsName The new workspace name.
 */
void PeakPickerTool::workspaceNameChanged(const QString& wsName)
{
  if (wsName != m_wsName)
  {
    fitBrowser()->setWorkspaceName(m_wsName);
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
 * @param i The index of the function with the changed parameter
 */
void PeakPickerTool::parameterChanged(int i)
{
  QMap<int,FunctionCurve*>::iterator it = m_guessCurves.find(i);
  if (it != m_guessCurves.end())
  {
    if (it.value()->title().text() != fitBrowser()->functionName(i))
    {
      removeGuess(i);
    }
    else
    {
      QStringList formulas = it.value()->formulas();
      formulas[1] = QString::fromStdString(*fitBrowser()->function(i));
      it.value()->setFormulas(formulas);
      it.value()->loadData();
    }
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
  menu.addSeparator();

  if (fitBrowser()->count()>0)
  {
    if (current()>=0)
    {
      if (hasGuessPlotted(current()))
      {
        action = new QAction("Remove guess",this);
        connect(action,SIGNAL(triggered()),this,SLOT(removeCurrentGuess()));
        menu.addAction(action);
      }
      else
      {
        action = new QAction("Plot guess",this);
        connect(action,SIGNAL(triggered()),this,SLOT(plotCurrentGuess()));
        menu.addAction(action);
      }

      menu.addSeparator();

      action = new QAction("Remove peak",this);
      connect(action,SIGNAL(triggered()),this,SLOT(deletePeak()));
      menu.addAction(action);

    }
    action = new QAction("Remove function...",this);
    connect(action,SIGNAL(triggered()),this,SLOT(deleteFunction()));
    menu.addAction(action);
  }

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
  int i = fitBrowser()->registeredPeaks().indexOf(QString::fromStdString(m_defaultPeakName));
  bool ok = false;
  QString fnName = 
    QInputDialog::getItem(m_mantidUI->appWindow(), "MantidPlot - Fit", "Select peak type", fitBrowser()->registeredPeaks(),i,false,&ok);
  if (ok)
  {
    m_defaultPeakName = fnName.toStdString();
    fitBrowser()->setDefaultFunctionType(m_defaultPeakName);
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
  if (current() < 0) return;
  fitBrowser()->removeFunction(current());
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
    QInputDialog::getItem(m_mantidUI->appWindow(), "MantidPlot - Fit", "Select background type", 
         fitBrowser()->registeredBackgrounds(),0,false,&ok);
  if (ok)
  {
    fitBrowser()->addFunction(fnName.toStdString());
  }
}

/**
 * Slot. Delete peak or background
 */
void PeakPickerTool::deleteFunction()
{
  QStringList names;
  for(int i=0;i<fitBrowser()->count();i++)
  {
    names << fitBrowser()->functionName(i);
  }
  bool ok;
  QString fnName = 
    QInputDialog::getItem(m_mantidUI->appWindow(), "MantidPlot - Fit", 
    "Select function to delete", names,0,false,&ok);
  if (ok)
  {
    QRegExp rx("^f(\\d+)-");
    int i = rx.indexIn(fnName);
    int j = rx.cap(1).toInt();
    fitBrowser()->removeFunction(j);
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
 * Slot. Plot the initial guess for the i-th function
 */
void PeakPickerTool::plotGuess(int i)
{
  if (i >= 0)
  {
    removeGuess(i);

    FunctionCurve* curve = new FunctionCurve(fitBrowser()->compositeFunction()->getFunction(i),
      QString::fromStdString(fitBrowser()->workspaceName()),
      fitBrowser()->workspaceIndex(),fitBrowser()->functionName());

    curve->setRange(fitBrowser()->startX(), fitBrowser()->endX());
    curve->loadData();

    d_graph->insertCurve(curve);
    d_graph->replot();

    m_guessCurves[i] = curve;
  }
}

/**
 * Slot. Remove the plot of the i-th function
 */
void PeakPickerTool::removeGuess(int i)
{
  QMap<int,FunctionCurve*>::iterator oldCurve = m_guessCurves.find(i);
  if (oldCurve != m_guessCurves.end())
  {
    d_graph->removeCurve(oldCurve.value());
    m_guessCurves.erase(oldCurve);
  }
}

/**
 * Slot. Plot the initial guess for the currently selected function
 */
void PeakPickerTool::plotCurrentGuess()
{
  plotGuess(current());
}

/**
 * Slot. Remove the plot of the current function
 */
void PeakPickerTool::removeCurrentGuess()
{
  removeGuess(current());
}

bool PeakPickerTool::hasGuessPlotted(int i)
{
  return m_guessCurves.find(i) != m_guessCurves.end();
}
