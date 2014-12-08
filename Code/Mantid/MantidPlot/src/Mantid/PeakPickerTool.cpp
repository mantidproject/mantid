#include "PeakPickerTool.h"
#include "MantidMatrixCurve.h"
#include "MantidUI.h"
#include "MantidQtMantidWidgets/FitPropertyBrowser.h"
#include "MantidQtMantidWidgets/MuonFitPropertyBrowser.h"
#include "../FunctionCurve.h"
#include "MantidQtMantidWidgets/PropertyHandler.h"


#include "MantidAPI/CompositeFunction.h"
#include "MantidAPI/IPeakFunction.h"
#include "MantidAPI/FunctionFactory.h"
#include "MantidKernel/Exception.h"
#include "MantidKernel/Logger.h"

#include "qwt_painter.h"
#include <QPainter>
#include <QList>
#include <QToolBar>
#include <QAction>
#include <QMenu>
#include <QInputDialog>
#include <QMessageBox>

#include <iostream>

namespace
{
  /// static logger
  Mantid::Kernel::Logger g_log("PeakPickerTool");
}

PeakPickerTool::PeakPickerTool(Graph *graph, MantidQt::MantidWidgets::FitPropertyBrowser *fitPropertyBrowser, MantidUI *mantidUI, bool showFitPropertyBrowser) :
QwtPlotPicker(graph->plotWidget()->canvas()),
PlotToolInterface(graph),
m_fitPropertyBrowser(fitPropertyBrowser),
m_mantidUI(mantidUI),
m_wsName(),m_spec(),m_init(false),
m_width_set(true),m_width(0),m_addingPeak(false),m_resetting(false),m_shouldBeNormalised(false)
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
      MantidMatrixCurve* mcurve = dynamic_cast<MantidMatrixCurve*>(curve);
      if (mcurve)
      {
        m_wsName = mcurve->workspaceName();
        m_spec = mcurve->workspaceIndex();
        m_shouldBeNormalised = mcurve->isDistribution() && mcurve->isNormalizable();
      }
      else
      {
        return;
      }
    }
  }
  else
  {
    return;
  }
  m_fitPropertyBrowser->normaliseData(m_shouldBeNormalised);
  m_fitPropertyBrowser->getHandler()->removeAllPlots();
  m_fitPropertyBrowser->setWorkspaceName(m_wsName);
  m_fitPropertyBrowser->setWorkspaceIndex(m_spec);
  connect(m_fitPropertyBrowser,SIGNAL(currentChanged()),this,SLOT(currentChanged()));
  connect(m_fitPropertyBrowser,SIGNAL(workspaceIndexChanged(int)),this,SLOT(workspaceIndexChanged(int)));
  connect(m_fitPropertyBrowser,SIGNAL(workspaceNameChanged(const QString&)),
                  this,SLOT(workspaceNameChanged(const QString&)));
  connect(m_fitPropertyBrowser,SIGNAL(functionRemoved()),this,SLOT(functionRemoved()));
  connect(m_fitPropertyBrowser,SIGNAL(functionCleared()),this,SLOT(functionCleared()));
  connect(m_fitPropertyBrowser,SIGNAL(algorithmFinished(const QString&)),
                  this,SLOT(algorithmFinished(const QString&)));
  connect(m_fitPropertyBrowser,SIGNAL(startXChanged(double)),this,SLOT(startXChanged(double)));
  connect(m_fitPropertyBrowser,SIGNAL(endXChanged(double)),this,SLOT(endXChanged(double)));
  connect(m_fitPropertyBrowser,SIGNAL(parameterChanged(const Mantid::API::IFunction*)),
                  this,SLOT(parameterChanged(const Mantid::API::IFunction*)));
  connect(m_fitPropertyBrowser,SIGNAL(plotGuess()),this,SLOT(plotGuess()));
  connect(m_fitPropertyBrowser,SIGNAL(plotCurrentGuess()),this,SLOT(plotCurrentGuess()));
  connect(m_fitPropertyBrowser,SIGNAL(removeGuess()),this,SLOT(removeGuess()));
  connect(m_fitPropertyBrowser,SIGNAL(removeCurrentGuess()),this,SLOT(removeCurrentGuess()));
  connect(m_fitPropertyBrowser,SIGNAL(removePlotSignal(MantidQt::MantidWidgets::PropertyHandler*)),
          this,SLOT(removePlot(MantidQt::MantidWidgets::PropertyHandler*)));
  connect(m_fitPropertyBrowser,SIGNAL(removeFitCurves()),this,SLOT(removeFitCurves()));

  // When fit browser destroyed, disable oneself in the parent graph
  connect(m_fitPropertyBrowser, SIGNAL(destroyed()), graph, SLOT(disableTools()));

  //Show the fitPropertyBrowser if it isn't already.
  if (showFitPropertyBrowser) m_fitPropertyBrowser->show();
  connect(this,SIGNAL(isOn(bool)),m_fitPropertyBrowser,SLOT(setPeakToolOn(bool)));
  emit isOn(true);

  auto cf = m_fitPropertyBrowser->compositeFunction();
  if (m_fitPropertyBrowser->count() == 0 || (m_fitPropertyBrowser->count() == 1 && m_fitPropertyBrowser->isAutoBack()))
  {
    m_init = true;

    QwtScaleMap xMap = d_graph->plotWidget()->canvasMap(QwtPlot::xBottom);
    double s1 = xMap.s1(), s2 = xMap.s2();
    double ds = fabs(s2-s1)*0.05; 
    xMin(s1 + ds);
    xMax(s2 - ds);

    m_changingXMin = false;
    m_changingXMax = false;
    m_fitPropertyBrowser->setStartX(xMin());
    m_fitPropertyBrowser->setEndX(xMax());
    if (m_fitPropertyBrowser->isAutoBack())
    {
      m_fitPropertyBrowser->addAutoBackground();
    }
  }
  else
  {
    m_init = true;
    xMin(m_fitPropertyBrowser->startX());
    xMax(m_fitPropertyBrowser->endX());
    m_changingXMin = false;
    m_changingXMax = false;
    for(size_t i=0;i<cf->nFunctions();i++)
    {
      auto pf = dynamic_cast<Mantid::API::IPeakFunction*>(cf->getFunction(i).get());
      if (pf)
      {
        m_width = pf->fwhm();
        if (m_width != 0.) break;
      }
    }
  }
  attach(d_graph->plotWidget());
  d_graph->plotWidget()->replot();

  connect(d_graph,SIGNAL(curveRemoved()),this,SLOT(curveRemoved()));
  connect(d_graph,SIGNAL(modifiedGraph()),this,SLOT(modifiedGraph()));

  try
  {// if it's a MatrixWorkspace in the ADS
    m_ws = boost::dynamic_pointer_cast<Mantid::API::MatrixWorkspace>(
      Mantid::API::AnalysisDataService::Instance().retrieve(
        m_wsName.toStdString()
      ));
  }
  catch(...)
  {// or it can be a TableWorkspace
    m_ws = boost::dynamic_pointer_cast<Mantid::API::MatrixWorkspace>(
      m_fitPropertyBrowser->createMatrixFromTableWorkspace()
    );
  }
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
        if (!isWidthSet())
        {
          double c = centre();
          double w = d_graph->plotWidget()->invTransform(2,pnt.x()) - c;
          setWidth(2*fabs(w));
          m_fitPropertyBrowser->updateParameters();
          emit peakChanged();
        }
        else if (resetting())
        {
          double c = d_graph->plotWidget()->invTransform(2,pnt.x());
          int yAxis = QwtPlot::yLeft;
          double h = d_graph->plotWidget()->invTransform(yAxis,pnt.y());
          setPeak(c,h);
          m_fitPropertyBrowser->updateParameters();
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
          m_fitPropertyBrowser->setStartX(xMin());
          m_fitPropertyBrowser->setEndX(xMax());
        }
        else if (changingXMin())
        {
          double x = d_graph->plotWidget()->invTransform(2,pnt.x());
          xMin(x);
          m_fitPropertyBrowser->setStartX(xMin());
        }
        else if (changingXMax())
        {
          double x = d_graph->plotWidget()->invTransform(2,pnt.x());
          xMax(x);
          m_fitPropertyBrowser->setEndX(xMax());
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
            m_fitPropertyBrowser->setStartX(xMin());
            m_fitPropertyBrowser->setEndX(xMax());
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
            MantidQt::MantidWidgets::PropertyHandler* handler = clickedOnCentreMarker(x,fabs(x1-x));
            if (clickedOnXMax(x,fabs(x1-x)))
            {// begin changing xMax
              changingXMax(true);
              d_graph->plotWidget()->canvas()->setCursor(Qt::SizeHorCursor);
              d_graph->plotWidget()->replot();
              m_fitPropertyBrowser->setStartX(xMin());
            }
            if (clickedOnXMin(x,fabs(x1-x)))
            {// begin changing xMin
              changingXMin(true);
              d_graph->plotWidget()->canvas()->setCursor(Qt::SizeHorCursor);
              d_graph->plotWidget()->replot();
              m_fitPropertyBrowser->setEndX(xMax());
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
              m_fitPropertyBrowser->setCurrentFunction(handler);
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
      if ((m_changingXMin || m_changingXMax) && m_fitPropertyBrowser->isAutoBack())
      {
        m_fitPropertyBrowser->refitAutoBackground();
      }
      resetting(false);
      changingXMin(false);
      changingXMax(false);
      m_addingPeak = false;
      m_fitPropertyBrowser->setStartX(xMin());
      m_fitPropertyBrowser->setEndX(xMax());
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

void PeakPickerTool::draw(QPainter *p, const QwtScaleMap &xMap, const QwtScaleMap &yMap, const QRect &) const
{
  try
  {
    MantidQt::MantidWidgets::PropertyHandler* h = m_fitPropertyBrowser->getHandler();
    if ( !h ) return;
    QList<MantidQt::MantidWidgets::PropertyHandler*> peaks = h->getPeakList();
    foreach(MantidQt::MantidWidgets::PropertyHandler* peak,peaks)
    {
      double c = peak->centre();
      if (c >= xMap.s1() && c <= xMap.s2())
      {
        int ic = xMap.transform(c);
        if (peak == m_fitPropertyBrowser->currentHandler())
        {// draw current peak
          double width = peak->fwhm();
          QPen pen;
          pen.setColor(QColor(255,0,0));
          pen.setStyle(Qt::DashLine);
          p->setPen(pen);
          int x1 = xMap.transform(c - width/2);
          int x2 = xMap.transform(c + width/2);
          QwtPainter::drawLine(p, x1, int(yMap.p1()), x1,int(yMap.p2()));
          QwtPainter::drawLine(p, x2, int(yMap.p1()), x2,int(yMap.p2()));

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
          QwtPainter::drawLine(p, ic, int(yMap.p1()), ic, int(yMap.p2()));
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
  QwtPainter::drawLine(p, x1, int(yMap.p1()), x1,int(yMap.p2()));
  QwtPainter::drawLine(p, x2, int(yMap.p1()), x2,int(yMap.p2()));

  pen.setColor(QColor(0,0,255));
  pen.setStyle(Qt::SolidLine);
  p->setPen(pen);
  QwtPainter::drawLine(p, x1, int(yMap.p1()), x1+3,int(yMap.p1()));
  QwtPainter::drawLine(p, x1, int(yMap.p2()), x1+3,int(yMap.p2()));

  QwtPainter::drawLine(p, x2, int(yMap.p1()), x2-3,int(yMap.p1()));
  QwtPainter::drawLine(p, x2, int(yMap.p2()), x2-3,int(yMap.p2()));

}

// Add a new peak with centre c and height h. 
void PeakPickerTool::addPeak(double c,double h)
{
  std::string fnName = m_fitPropertyBrowser->defaultPeakType();
  MantidQt::MantidWidgets::PropertyHandler* handler = m_fitPropertyBrowser->addFunction(fnName);
  if (!handler || !handler->pfun()) return;
  handler->setCentre(c);
  double width = handler->fwhm();
  if (width == 0)
  {
    handler->setFwhm(m_width);
  }
  if (handler->fwhm() > 0.)
  {
    handler->calcBase();
  }
  handler->setHeight(h);
}

// Give new centre and height to the current peak
void PeakPickerTool::setPeak(double c,double h)
{
  MantidQt::MantidWidgets::PropertyHandler* handler = m_fitPropertyBrowser->currentHandler();
  if ( ! handler ) return;
  handler->setCentre(c);
  handler->calcBase();
  handler->setHeight(h);
}

// Return the centre of the currently selected peak
double PeakPickerTool::centre()const
{
  MantidQt::MantidWidgets::PropertyHandler* h = m_fitPropertyBrowser->currentHandler();
  return h? h->centre(): 0;
}

// Return the width of the currently selected peak
double PeakPickerTool::width()const
{
  MantidQt::MantidWidgets::PropertyHandler* h = m_fitPropertyBrowser->currentHandler();
  return h? h->fwhm(): 0;
}

// Return the height of the currently selected peak
double PeakPickerTool::height()const
{
  MantidQt::MantidWidgets::PropertyHandler* h = m_fitPropertyBrowser->currentHandler();
  return h? h->fwhm(): 0;
}

// Change the width of the currently selected peak
void PeakPickerTool::setWidth(double x)
{
  MantidQt::MantidWidgets::PropertyHandler* h = m_fitPropertyBrowser->currentHandler();
  if (!h || !h->pfun()) return;
  m_width = x;
  h->setFwhm(x);
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
  MantidQt::MantidWidgets::PropertyHandler* h = m_fitPropertyBrowser->currentHandler();
  if (!h) return false;
  double c = h->centre();
  double w = h->fwhm()/2;
  return (fabs(x - c - w) <= dx) || (fabs(x - c + w) <= dx);
}

// Check if x is near a peak centre marker (+-dx). If true returns the peak's address or 0 otherwise.
MantidQt::MantidWidgets::PropertyHandler* PeakPickerTool::clickedOnCentreMarker(double x,double dx)const
{
  QList<MantidQt::MantidWidgets::PropertyHandler*> peaks = m_fitPropertyBrowser->getHandler()->getPeakList();
  foreach(MantidQt::MantidWidgets::PropertyHandler* peak,peaks)
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
 */
void PeakPickerTool::currentChanged()
{
  d_graph->plotWidget()->replot();
}

/**
 * Slot. Reacts on the function deletion in the Fit Browser.
 */
void PeakPickerTool::functionRemoved()
{
  d_graph->plotWidget()->replot();
}

/**
 * Slot. Called when the Fit algorithm finishes. 
 * @param out :: The name of the output workspace with the results of the fit.
 */
void PeakPickerTool::algorithmFinished(const QString& out)
{
  // Remove old curves first
  removeFitCurves();

  // If style needs to be changed from default, signal pair second will be true and change to line.
  auto * curve = new MantidMatrixCurve("",out,graph(),1,MantidMatrixCurve::Spectrum, false, m_shouldBeNormalised, Graph::Line);
  m_curveNames.append(curve->title().text());
  if (m_fitPropertyBrowser->plotDiff())
  {
    curve = new MantidMatrixCurve("",out,graph(),2,MantidMatrixCurve::Spectrum,false,m_shouldBeNormalised);
    m_curveNames.append(curve->title().text());
  }
  if(m_fitPropertyBrowser->plotCompositeMembers())
  {
    using namespace Mantid::API;
    try
    {
      auto wkspace = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(out.toStdString());
      const size_t nhist = wkspace->getNumberHistograms();
      for(size_t i = 3; i < nhist; ++i) // first 3 are data,sum,diff
      {
        curve = new MantidMatrixCurve("",out,graph(),static_cast<int>(i),MantidMatrixCurve::Spectrum,false,m_shouldBeNormalised);
        m_curveNames.append(curve->title().text());
      }
    }
    catch(Mantid::Kernel::Exception::NotFoundError&)
    {
      g_log.warning() << "PeakPicker cannot find output workspace '" + out.toStdString() + "'" << std::endl;
    }
  }
  
  graph()->replot();
}

/**
 * Slot. Called when the workspace index is changed in the FitBrowser
 * @param i :: The new workspace index.
 */
void PeakPickerTool::workspaceIndexChanged(int i)
{
  if (i != m_spec)
  {
    m_fitPropertyBrowser->setWorkspaceIndex(m_spec);
  }
}

/**
  * Slot. Called when the workspace name is changed in the FitBrowser.
  * It doesn't allow changing the workspace name unless it is a name of
  * the workspace group containing m_wsName
  * @param wsName :: The new workspace name.
  */
void PeakPickerTool::workspaceNameChanged(const QString& wsName)
{
  if (wsName != m_wsName)
  {
    Mantid::API::Workspace_sptr ws;
 
    if (Mantid::API::AnalysisDataService::Instance().doesExist(wsName.toStdString()))
    {
       m_wsName = wsName;
       m_fitPropertyBrowser->setWorkspaceName(m_wsName);
    }
  }
}

/**
 * Slot. Called when the startX changed in the FitBrowser
 * @param sX :: The new startX
 */
void PeakPickerTool::startXChanged(double sX)
{
  xMin(sX);
  graph()->replot();
}

/**
 * Slot. Called when the endX changed in the FitBrowser
 * @param eX :: The new endX
 */
void PeakPickerTool::endXChanged(double eX)
{
  xMax(eX);
  graph()->replot();
}

/**
 * Slot. Called in response to parameterChanged signal from FitBrowser
 * @param f :: The pointer to the function with the changed parameter
 */
void PeakPickerTool::parameterChanged(const Mantid::API::IFunction* f)
{
  MantidQt::MantidWidgets::PropertyHandler* theHandler = m_fitPropertyBrowser->getHandler();
  MantidQt::MantidWidgets::PropertyHandler* h = theHandler->findHandler(f);
  if (!h) return;
  replot(h);
  if (h != theHandler && theHandler->hasPlot())
  {
    replot(theHandler);
  }
  graph()->replot();
}

void PeakPickerTool::replot(MantidQt::MantidWidgets::PropertyHandler* h) const
{
  if (h->hasPlot())
  {
    FunctionCurve* fc = 0;
    int indexForFC = -1;
    for (int i = 0; i < d_graph->curves(); i++)
    {
      fc = dynamic_cast<FunctionCurve*>(d_graph->curve(i));
      if (fc)
      {
        if (fc->getIFunctionIdentifier() == h->ifun().get())
        {
          indexForFC = i;
          break;
        }
      }
    }

    if (indexForFC >= 0)
    {
      QStringList formulas = fc->formulas();
      formulas[1] = QString::fromStdString(h->ifun()->asString());
      fc->setFormulas(formulas);
      //fc->loadData();
      auto ws = boost::dynamic_pointer_cast<const Mantid::API::MatrixWorkspace>(m_fitPropertyBrowser->getWorkspace());
      fc->loadMantidData(ws,m_fitPropertyBrowser->workspaceIndex());
    }
  }
}


/**
 * Adds commands specific to the tool to a context menu
 * @param menu :: A reference to the context menu
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

  if (m_fitPropertyBrowser->count()>0)
  {
    if (m_fitPropertyBrowser->getHandler()->hasPlot())
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

    MantidQt::MantidWidgets::PropertyHandler* h = m_fitPropertyBrowser->currentHandler();
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

  action = new QAction("Get Parameters", this);
  connect(action, SIGNAL(triggered()), this, SLOT(getParameters()));
  menu.addAction(action);
  
  menu.addSeparator();

  if (m_fitPropertyBrowser->isFitEnabled())
  {
    action = new QAction("Fit",this);
    connect(action,SIGNAL(triggered()),this,SLOT(fit()));
    menu.addAction(action);
  }

  if (m_fitPropertyBrowser->isUndoEnabled())
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
  int i = m_fitPropertyBrowser->registeredPeaks().indexOf(QString::fromStdString(m_fitPropertyBrowser->defaultPeakType()));
  bool ok = false;
  QString fnName = 
    QInputDialog::getItem(d_graph, "MantidPlot - Fit", "Select peak type", m_fitPropertyBrowser->registeredPeaks(),i,false,&ok);
  if (ok)
  {
    m_fitPropertyBrowser->setDefaultPeakType(fnName.toStdString());
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
  m_fitPropertyBrowser->updateParameters();
}

/**
 * Slot. Deletes the current peak
 */
void PeakPickerTool::deletePeak()
{
  MantidQt::MantidWidgets::PropertyHandler* h = m_fitPropertyBrowser->currentHandler();
  if (!h) return;
  h->removeFunction();
  functionRemoved();
}

/**
 * Slot. Start the fit
 */
void PeakPickerTool::fit()
{
  m_fitPropertyBrowser->fit();
}

/**
 * Slot. Add a background function
 */
void PeakPickerTool::addBackground()
{
  bool ok = false;
  QString fnName = 
    QInputDialog::getItem(d_graph, 
         "MantidPlot - Fit", "Select background type", 
         m_fitPropertyBrowser->registeredBackgrounds(),
         m_fitPropertyBrowser->registeredBackgrounds().indexOf("LinearBackground"),
         false,&ok);
  if (ok)
  {
    if (fnName == "LinearBackground")
    {
      m_fitPropertyBrowser->setAutoBackgroundName(fnName);
      m_fitPropertyBrowser->addAutoBackground();
    }
    else
    {
      m_fitPropertyBrowser->addFunction(fnName.toStdString());
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
    QInputDialog::getItem(d_graph, 
         "MantidPlot - Fit", "Select function type", 
         m_fitPropertyBrowser->registeredOthers(),0,
         false,&ok);
  if (ok)
  {
    m_fitPropertyBrowser->addFunction(fnName.toStdString());
  }
}

/**
 * Slot. Undo the fit
 */
void PeakPickerTool::undoFit()
{
  m_fitPropertyBrowser->undoFit();
}

/**
 * Slot. Clear all functions
 */
void PeakPickerTool::clear()
{
  m_fitPropertyBrowser->clear();
}

/** Set the tool tip text
 * @param txt :: The tip text
 */
void PeakPickerTool::setToolTip(const QString& txt)
{
  d_graph->setToolTip(txt);
  m_fitPropertyBrowser->setTip(txt);
}

/**
 * Slot. Plot the initial guess for the function
 */
void PeakPickerTool::plotGuess()
{
  MantidQt::MantidWidgets::PropertyHandler* h = m_fitPropertyBrowser->getHandler();
  plotFitFunction(h);
  h->hasPlot() = true;
  d_graph->replot();
}

void PeakPickerTool::plotCurrentGuess()
{
  MantidQt::MantidWidgets::PropertyHandler* h = m_fitPropertyBrowser->currentHandler();
  plotFitFunction(h);
  h->hasPlot() = true;
  d_graph->replot();
}

/**
 * Plot function
 */
void PeakPickerTool::plotFitFunction(MantidQt::MantidWidgets::PropertyHandler* h)
{
  if (h)
  {
    // check to see if function is already plotted?
    bool alreadyPlotted = false;
    FunctionCurve* fc;
    for (int i = 0; i < d_graph->curves(); i++)
    {
      fc = dynamic_cast<FunctionCurve*>(d_graph->curve(i));
      if (fc)
        if (fc->getIFunctionIdentifier() == h->ifun().get())
        {
          alreadyPlotted = true;
          break;
        }
    }
  
    // plot current function guess
    if (!alreadyPlotted)
    {
      fc = new FunctionCurve(
        h->ifun().get(), QString::fromStdString(m_fitPropertyBrowser->workspaceName()),
        //QString::fromStdString(m_fitPropertyBrowser->groupMember()),//m_browser->workspaceName()),
        m_fitPropertyBrowser->workspaceIndex(),
        h->functionName());
      fc->setRange(m_fitPropertyBrowser->startX(),m_fitPropertyBrowser->endX());
      auto ws = boost::dynamic_pointer_cast<const Mantid::API::MatrixWorkspace>(m_fitPropertyBrowser->getWorkspace());
      fc->loadMantidData(ws,m_fitPropertyBrowser->workspaceIndex());
      // Graph now owns m_curve. Use m_curve->removeMe() to remove (and delete) from Graph
      d_graph->insertCurve(fc);
      connect(fc,SIGNAL(forgetMe()),h,SLOT(plotRemoved()));
      if (h == m_fitPropertyBrowser->getHandler())
      {
        m_fitPropertyBrowser->setTextPlotGuess("Remove guess");
      }
    }
  }
}


/**
 * Slot. Remove the plot of the i-th function
 */
void PeakPickerTool::removeGuess()
{
  MantidQt::MantidWidgets::PropertyHandler* h = m_fitPropertyBrowser->getHandler();
  removePlot(h);
  h->hasPlot() = false;
  d_graph->replot();
}

void PeakPickerTool::removePlot(MantidQt::MantidWidgets::PropertyHandler* h)
{
  // check to see if function is already plotted?
  FunctionCurve* fc = 0;
  int indexForFC = -1;
  for (int i = 0; i < d_graph->curves(); i++)
  {
    fc = dynamic_cast<FunctionCurve*>(d_graph->curve(i));
    if (fc)
    {
      if (fc->getIFunctionIdentifier() == h->ifun().get())
      {
        indexForFC = i;
        break;
      }
    }
  }

  if (indexForFC >= 0)
  {
    fc->removeMe();
    if (h == m_fitPropertyBrowser->getHandler())
    {
      m_fitPropertyBrowser->setTextPlotGuess("Plot guess");
    }
  }
}

/**
 * Slot. Remove the plot of the i-th function
 */
void PeakPickerTool::removeCurrentGuess()
{
  MantidQt::MantidWidgets::PropertyHandler* h = m_fitPropertyBrowser->currentHandler();
  if (h)
  {
    removePlot(h);
    h->hasPlot() = false;
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
  m_fitPropertyBrowser->setStartX(xMin());
  m_fitPropertyBrowser->setEndX(xMax());
  if (m_fitPropertyBrowser->isAutoBack())
  {
    m_fitPropertyBrowser->addAutoBackground();
  }
  d_graph->replot();
}


/**
* Checks whether there is a parameters file attached to the plot. 
* If so, it opens up that parameters table.
*/
void PeakPickerTool::getParameters()
{
  QString parameterWs = m_wsName + "_Parameters";
  if (Mantid::API::AnalysisDataService::Instance().doesExist(parameterWs.toStdString() ) )
  {
    m_mantidUI->importWorkspace(parameterWs);
  }
  else
  {
    QMessageBox::information(m_fitPropertyBrowser, "Mantid - Warning", "No parameter file with the name \"" + parameterWs + "\" found.");
  }
}

void PeakPickerTool::modifiedGraph()
{
}

void PeakPickerTool::removeFitCurves()
{
  QStringListIterator itr(m_curveNames);
  while(itr.hasNext())
  {
    graph()->removeCurve(itr.next());
  }
  m_curveNames.clear();
}
