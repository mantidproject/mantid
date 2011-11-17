#include "MantidMDCurve.h"
#include "MantidQwtIMDWorkspaceData.h"

#include <qpainter.h>
#include <qwt_symbol.h>

#include "MantidAPI/IMDWorkspace.h"
#include "MantidAPI/IMDIterator.h"

#include "MantidAPI/AnalysisDataService.h"

#include "../Graph.h"
#include "../ApplicationWindow.h"
#include "../MultiLayer.h"


using namespace Mantid::API;
using namespace MantidQt::API;

/**
 *  @param wsName :: The workspace name.
 *  @param g :: The Graph widget which will display the curve
 *  @param index :: The index of the spectrum or bin in the workspace
 *  @param err :: True if the errors are to be plotted
 *  @param style :: Graph style
 *  @throw std::invalid_argument if the index is out of range for the given workspace
 */
MantidMDCurve::MantidMDCurve(const QString& wsName,Graph* g,bool err,bool distr,Graph::CurveType style)
  :MantidCurve(wsName),
  m_drawErrorBars(err),
  m_drawAllErrorBars(err),
  m_wsName(wsName)
{
  init(g,distr,style);
}


MantidMDCurve::MantidMDCurve(const MantidMDCurve& c)
  :MantidCurve(createCopyName(c.title().text())),
  m_drawErrorBars(c.m_drawErrorBars),
  m_drawAllErrorBars(c.m_drawAllErrorBars),
  m_wsName(c.m_wsName)
{
  setData(c.data());
  observeDelete();
  connect( this, SIGNAL(resetData(const QString&)), this, SLOT(dataReset(const QString&)) );
  observeAfterReplace();
  observeADSClear();
}

/**
 *  @param g :: The Graph widget which will display the curve
 *  @param distr :: True if this is a distribution
 *  @param style :: The graph style to use
 */
void MantidMDCurve::init(Graph* g, bool distr, Graph::CurveType style)
{
  IMDWorkspace_const_sptr ws = boost::dynamic_pointer_cast<IMDWorkspace>(
              AnalysisDataService::Instance().retrieve(m_wsName.toStdString()) );
  if(!ws)
  {
    std::string message = "Could not extract IMDWorkspace of name: " + m_wsName.toStdString();
    throw std::runtime_error(message);
  }
  if(ws->getNonIntegratedDimensions().size() != 1)
  {
    std::string message = "This plot only applies to Workspaces with a single expanded dimension";
    throw std::invalid_argument(message);
  }

  this->setTitle(m_wsName + "-signal");

  const bool log = g->isLog(QwtPlot::yLeft);
  MantidQwtIMDWorkspaceData data(ws,log);
  setData(data);

  int lineWidth = 1;
  MultiLayer* ml = (MultiLayer*)(g->parent()->parent()->parent());
  if (style == Graph::Unspecified || (ml && ml->applicationWindow()->applyCurveStyleToMantid) )
  {
    if ( style == Graph::Unspecified )
      style = static_cast<Graph::CurveType>(ml->applicationWindow()->defaultCurveStyle);

    QwtPlotCurve::CurveStyle qwtStyle;
    const int symbolSize = ml->applicationWindow()->defaultSymbolSize;
    const QwtSymbol symbol(QwtSymbol::Ellipse,QBrush(Qt::black),QPen(),QSize(symbolSize,symbolSize));
    switch(style)
    {
    case Graph::Line :
      qwtStyle = QwtPlotCurve::Lines;
      break;
    case Graph::Scatter:
      qwtStyle = QwtPlotCurve::NoCurve;
      this->setSymbol(symbol);
      break;
    case Graph::LineSymbols :
      qwtStyle = QwtPlotCurve::Lines;
      this->setSymbol(symbol);
      break;
    case 15:
      qwtStyle = QwtPlotCurve::Steps;
      break;  // should be Graph::HorizontalSteps but it doesn't work
    default:
      qwtStyle = QwtPlotCurve::Lines;
      break;
    }
    setStyle(qwtStyle);
    lineWidth = static_cast<int>(floor(ml->applicationWindow()->defaultCurveLineWidth));
  }
  else
    setStyle(QwtPlotCurve::Lines);
  if (g)
  {
    g->insertCurve(this,lineWidth);
  }
  connect(g,SIGNAL(axisScaleChanged(int,bool)),this,SLOT(axisScaleChanged(int,bool)));
  observeDelete();
  connect( this, SIGNAL(resetData(const QString&)), this, SLOT(dataReset(const QString&)) );
  observeAfterReplace();
  observeADSClear();
}


MantidMDCurve::~MantidMDCurve()
{
}

/**
 * Clone the curve for the use by a particular Graph
 */
MantidMDCurve* MantidMDCurve::clone(const Graph* g)const
{
  MantidMDCurve* mc = new MantidMDCurve(*this);/*
  if (g)
  {
    mc->setDrawAsDistribution(g->isDistribution());
  }*/
  return mc;
}


void MantidMDCurve::setData(const QwtData &data)
{
  if (!dynamic_cast<const MantidQwtIMDWorkspaceData*>(&data)) 
    throw std::runtime_error("Only MantidQwtIMDWorkspaceData can be set to a MantidMDCurve");
  PlotCurve::setData(data);
}

QwtDoubleRect MantidMDCurve::boundingRect() const
{
  if (m_boundingRect.isNull())
  {
    const MantidQwtIMDWorkspaceData* data = mantidData();
    if (data->size() == 0) return QwtDoubleRect(0,0,1,1);
    double y_min = std::numeric_limits<double>::infinity();
    double y_max = -y_min;
    for(size_t i=0;i<data->size();++i)
    {
      double y = data->y(i);
      if (y == std::numeric_limits<double>::infinity() || y != y) continue;
      if (y < y_min && (!mantidData()->logScale() || y > 0.)) y_min = y;
      if (y > y_max) y_max = y;
    }
    double x_min = data->x(0);
    double x_max = data->x(data->size()-1);
    m_boundingRect = QwtDoubleRect(x_min,y_min,x_max-x_min,y_max-y_min);
  }
  return m_boundingRect;
}

void MantidMDCurve::draw(QPainter *p, 
          const QwtScaleMap &xMap, const QwtScaleMap &yMap,
          const QRect &rect) const
{
  PlotCurve::draw(p,xMap,yMap,rect);

  if (m_drawErrorBars)// drawing error bars
  {
    const MantidQwtIMDWorkspaceData* d = dynamic_cast<const MantidQwtIMDWorkspaceData*>(&data());
    if (!d)
      throw std::runtime_error("Only MantidQwtIMDWorkspaceData can be set to a MantidMDCurve");
    int xi0 = 0;
    p->setPen(pen());
    const int dx = 3;
    const int dx2 = 2*dx;
    int x1 = static_cast<int>(floor(xMap.p1()));
    int x2 = static_cast<int>(floor(xMap.p2()));
    for (int i = 0; i < static_cast<int>(d->esize()); i++)
    {
      const int xi = xMap.transform(d->ex(i));
      if (m_drawAllErrorBars || (xi > x1 && xi < x2 && (i == 0 || abs(xi - xi0) > dx2)))
      {
        const double Y = y(i);
        const double E = d->e(i);
        const int ei1 = yMap.transform(Y - E);
        const int ei2 = yMap.transform(Y + E);

        // This call can crash MantidPlot if the error is zero,
        //   so protect against this (line of zero length anyway)
        if (E) p->drawLine(xi,ei1,xi,ei2);
        p->drawLine(xi-dx,ei1,xi+dx,ei1);
        p->drawLine(xi-dx,ei2,xi+dx,ei2);

        xi0 = xi;
      }
    }
  }
}

void MantidMDCurve::itemChanged()
{
  PlotCurve::itemChanged();
}

/** Create the name for a curve which is a copy of another curve.
 *  @param curveName :: The original curve name.
 */
QString MantidMDCurve::createCopyName(const QString& curveName)
{
  int i = curveName.lastIndexOf(" (copy");
  if (i < 0) return curveName+" (copy)";
  int j = curveName.lastIndexOf(")");
  if (j == i + 5) return curveName.mid(0,i)+" (copy2)";
  int k = curveName.mid(i+5,j-i-5).toInt();
  return curveName.mid(0,i)+" (copy"+QString::number(k+1)+")";
}

/**  Resets the data if wsName is the name of this workspace
 *  @param wsName :: The name of a workspace which data has been changed in the data service.
 */
void MantidMDCurve::dataReset(const QString& wsName)
{
  if (m_wsName != wsName) return;
  const std::string wsNameStd = wsName.toStdString();
  Mantid::API::MatrixWorkspace_sptr mws;
  try
  {
    Mantid::API::Workspace_sptr base =  Mantid::API::AnalysisDataService::Instance().retrieve(wsNameStd);
    mws = boost::dynamic_pointer_cast<Mantid::API::MatrixWorkspace>(base);
  }
  catch(std::runtime_error&)
  {
    Mantid::Kernel::Logger::get("MantidMDCurve").information() << "Workspace " << wsNameStd
        << " could not be found - plotted curve(s) deleted\n";
    mws = Mantid::API::MatrixWorkspace_sptr();
  }

  if (!mws) return;
  const MantidQwtIMDWorkspaceData * new_mantidData(NULL);
  try {
    new_mantidData = mantidData()->copy(mws);
    setData(*new_mantidData);
    if (mws->isHistogramData())
    {
      setStyle(QwtPlotCurve::Steps);
      setCurveAttribute(Inverted,true);// this is the Steps style modifier that makes horizontal steps
    }
    else
    {
      setStyle(QwtPlotCurve::Lines);
    }
    // Queue this plot to be updated once all MantidQwtMatrixWorkspaceData objects for this workspace have been
    emit dataUpdated();
  } catch(std::range_error) {
    // Get here if the new workspace has fewer spectra and the plotted one no longer exists
    Mantid::Kernel::Logger::get("MantidMDCurve").information() << "Workspace " << wsNameStd
        << " now has fewer spectra - plotted curve(s) deleted\n";
    deleteHandle(wsNameStd,mws);
  }
  delete new_mantidData;
}

void MantidMDCurve::afterReplaceHandle(const std::string& wsName,const boost::shared_ptr<Mantid::API::Workspace> ws)
{
  (void) ws;

  invalidateBoundingRect();
  emit resetData(QString::fromStdString(wsName));
}
/* This method saves the curve details to a string.
 * Useful for loading/saving mantid project.
*/
QString MantidMDCurve::saveToString()
{
	QString s;
	s="MantidMDCurve\t"+m_wsName+"\t"+QString::number(m_drawErrorBars)+"\n";
	return s;
}


MantidQwtIMDWorkspaceData* MantidMDCurve::mantidData()
{
  MantidQwtIMDWorkspaceData* d = dynamic_cast<MantidQwtIMDWorkspaceData*>(&data());
  return d;
}

const MantidQwtIMDWorkspaceData* MantidMDCurve::mantidData()const
{
  const MantidQwtIMDWorkspaceData* d = dynamic_cast<const MantidQwtIMDWorkspaceData*>(&data());
  return d;
}

void MantidMDCurve::axisScaleChanged(int axis, bool toLog)
{
  if (axis == QwtPlot::yLeft || axis == QwtPlot::yRight)
  {
    mantidData()->setLogScale(toLog);
    // force boundingRect calculation at this moment
    invalidateBoundingRect();
    boundingRect();
    mantidData()->saveLowestPositiveValue(m_boundingRect.y());
  }
}

/// Returns whether the curve has error bars
bool MantidMDCurve::hasErrorBars() const
{
  return m_drawErrorBars;
}
