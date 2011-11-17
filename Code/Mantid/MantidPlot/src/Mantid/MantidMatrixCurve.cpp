#include "MantidMatrixCurve.h"
#include "MantidQwtMatrixWorkspaceData.h"

#include <qpainter.h>
#include <qwt_symbol.h>

#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/AnalysisDataService.h"

#include "../Graph.h"
#include "../ApplicationWindow.h"
#include "../MultiLayer.h"


using namespace Mantid::API;
using namespace MantidQt::API;

/**
 *  @param name :: The curve's name - shown in the legend
 *  @param wsName :: The workspace name.
 *  @param g :: The Graph widget which will display the curve
 *  @param index :: The index of the spectrum or bin in the workspace
 *  @param err :: True if the errors are to be plotted
 *  @param distr :: True if it is a distribution
 *  @param sytle :: CurveType style to use
 *..@throw Mantid::Kernel::Exception::NotFoundError if the workspace cannot be found
 *  @throw std::invalid_argument if the index is out of range for the given workspace
 */
MantidMatrixCurve::MantidMatrixCurve(const QString& name,const QString& wsName,Graph* g,int index,bool err,bool distr, Graph::CurveType style)
  :MantidCurve(),
  m_drawErrorBars(err),
  m_drawAllErrorBars(false),
  m_wsName(wsName),
  m_index(index)
{
  init(g,distr,style);
}

/**
 *  @param wsName :: The workspace name.
 *  @param g :: The Graph widget which will display the curve
 *  @param index :: The index of the spectrum or bin in the workspace
 *  @param err :: True if the errors are to be plotted
 *  @param distr :: True if it is a distribution
 *  @param sytle :: CurveType style to use
 *  @throw std::invalid_argument if the index is out of range for the given workspace
 */
MantidMatrixCurve::MantidMatrixCurve(const QString& wsName,Graph* g,int index,bool err,bool distr, Graph::CurveType style)
  :MantidCurve(),
  m_drawErrorBars(err),
  m_drawAllErrorBars(false),
  m_wsName(wsName),
  m_index(index)
{
  init(g,distr,style);
}


MantidMatrixCurve::MantidMatrixCurve(const MantidMatrixCurve& c)
  :MantidCurve(createCopyName(c.title().text())),
  m_drawErrorBars(c.m_drawErrorBars),
  m_drawAllErrorBars(c.m_drawAllErrorBars),
  m_wsName(c.m_wsName),
  m_index(c.m_index),
  m_xUnits(c.m_xUnits),
  m_yUnits(c.m_yUnits)
{
  setData(c.data());
  observeDelete();
  connect( this, SIGNAL(resetData(const QString&)), this, SLOT(dataReset(const QString&)) );
  observeAfterReplace();
  observeADSClear();
}

/**
 *  @param workspace :: The source workspace for the curve's data
 *  @param g :: The Graph widget which will display the curve
 *  @param distrib :: True for a distribution
 *  @param style :: The curve type to use
 */
void MantidMatrixCurve::init(Graph* g,bool distr,Graph::CurveType style)
{
  MatrixWorkspace_const_sptr workspace = boost::dynamic_pointer_cast<MatrixWorkspace>(AnalysisDataService::Instance().retrieve(m_wsName.toStdString()) );

  if ( !workspace || m_index >= static_cast<int>(workspace->getNumberHistograms()) )
  {
    std::stringstream ss;
    ss << m_index << " is an invalid spectrum index for workspace " << m_wsName.toStdString() << " - not plotted";
    throw std::invalid_argument(ss.str());
  }

  // Set the curve name if it the non-naming constructor was called
  if ( this->title().isEmpty() )
  {
    // If there's only one spectrum in the workspace, title is simply workspace name
    if (workspace->getNumberHistograms() == 1) this->setTitle(m_wsName);
    else this->setTitle(createCurveName(workspace,m_wsName,m_index));
  }

  Mantid::API::MatrixWorkspace_const_sptr matrixWS = boost::shared_dynamic_cast<const Mantid::API::MatrixWorkspace>(workspace);
  //we need to censor the data if there is a log scale because it can't deal with negative values, only the y-axis has been found to be problem so far
  const bool log = g->isLog(QwtPlot::yLeft);
  MantidQwtMatrixWorkspaceData data(matrixWS,m_index, log,distr);
  setData(data);
  Mantid::API::Axis* ax = matrixWS->getAxis(0);
  if (ax->unit())
  {
    m_xUnits = ax->unit();
  }
  else
  {
    m_xUnits.reset(new Mantid::Kernel::Units::Empty());
  }

  Mantid::API::Axis* ay = matrixWS->getAxis(1);
  if (ay->unit())
  {
    m_yUnits = ay->unit();
  }
  else
  {
    m_yUnits.reset(new Mantid::Kernel::Units::Empty());
  }
  int lineWidth = 1;
  MultiLayer* ml = (MultiLayer*)(g->parent()->parent()->parent());


  if (style == Graph::Unspecified || (ml && ml->applicationWindow()->applyCurveStyleToMantid) )
  {
    if ( style == Graph::Unspecified )
      style = static_cast<Graph::CurveType>(ml->applicationWindow()->defaultCurveStyle);

    QwtPlotCurve::CurveStyle qwtStyle;
    // Get the symbol size from the user preferences and create solid black circle symbol
    // of that size for use if the preferred plot style is scatter or line+symbol
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
  else if (matrixWS->isHistogramData() && !matrixWS->isDistribution())
  {
    setStyle(QwtPlotCurve::Steps);
    setCurveAttribute(Inverted,true);// this is the Steps style modifier that makes horizontal steps
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


MantidMatrixCurve::~MantidMatrixCurve()
{
}

/**
 * Clone the curve for the use by a particular Graph
 */
MantidMatrixCurve* MantidMatrixCurve::clone(const Graph* g)const
{
  MantidMatrixCurve* mc = new MantidMatrixCurve(*this);
  if (g)
  {
    mc->setDrawAsDistribution(g->isDistribution());
  }
  return mc;
}

void MantidMatrixCurve::loadData()
{
  // This should only be called for waterfall plots
  // Calculate the offsets...
  computeWaterfallOffsets();
  MantidQwtMatrixWorkspaceData * data = mantidData();
  // ...and apply them
  data->applyOffsets(d_x_offset,d_y_offset);
  invalidateBoundingRect();
}

void MantidMatrixCurve::setData(const QwtData &data)
{
  if (!dynamic_cast<const MantidQwtMatrixWorkspaceData*>(&data)) 
    throw std::runtime_error("Only MantidQwtMatrixWorkspaceData can be set to a MantidMatrixCurve");
  PlotCurve::setData(data);
}

QwtDoubleRect MantidMatrixCurve::boundingRect() const
{
  if (m_boundingRect.isNull())
  {
    const MantidQwtMatrixWorkspaceData* data = mantidData();
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

void MantidMatrixCurve::draw(QPainter *p, 
          const QwtScaleMap &xMap, const QwtScaleMap &yMap,
          const QRect &rect) const
{
  PlotCurve::draw(p,xMap,yMap,rect);

  if (m_drawErrorBars)// drawing error bars
  {
    const MantidQwtMatrixWorkspaceData* d = dynamic_cast<const MantidQwtMatrixWorkspaceData*>(&data());
    if (!d)
      throw std::runtime_error("Only MantidQwtMatrixWorkspaceData can be set to a MantidMatrixCurve");
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

void MantidMatrixCurve::itemChanged()
{
  MantidQwtMatrixWorkspaceData* d = dynamic_cast<MantidQwtMatrixWorkspaceData*>(&data());
  if (d && d->m_isHistogram)
  {
    if (style() == Steps) d->m_binCentres = false;
    else
      d->m_binCentres = true;
  }
  PlotCurve::itemChanged();
}

/** Create the name for a curve from the following input:
 *  @param wsName :: The workspace name
 *  @param index ::  The spectra (bin) index
 */
QString MantidMatrixCurve::createCurveName(const boost::shared_ptr<const Mantid::API::MatrixWorkspace> ws,
                                     const QString& wsName,int index)
{
  QString name = wsName + "-" + QString::fromStdString(ws->getAxis(1)->label(index));
  return name;
}

/** Create the name for a curve which is a copy of another curve.
 *  @param curveName :: The original curve name.
 */
QString MantidMatrixCurve::createCopyName(const QString& curveName)
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
void MantidMatrixCurve::dataReset(const QString& wsName)
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
    Mantid::Kernel::Logger::get("MantidMatrixCurve").information() << "Workspace " << wsNameStd
        << " could not be found - plotted curve(s) deleted\n";
    mws = Mantid::API::MatrixWorkspace_sptr();
  }

  if (!mws) return;
  const MantidQwtMatrixWorkspaceData * new_mantidData(NULL);
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
    Mantid::Kernel::Logger::get("MantidMatrixCurve").information() << "Workspace " << wsNameStd
        << " now has fewer spectra - plotted curve(s) deleted\n";
    deleteHandle(wsNameStd,mws);
  }
  delete new_mantidData;
}

void MantidMatrixCurve::afterReplaceHandle(const std::string& wsName,const boost::shared_ptr<Mantid::API::Workspace> ws)
{
  (void) ws;

  invalidateBoundingRect();
  emit resetData(QString::fromStdString(wsName));
}
/* This method saves the curve details to a string.
 * Useful for loading/saving mantid project.
*/
QString MantidMatrixCurve::saveToString()
{
	QString s;
	s="MantidMatrixCurve\t"+m_wsName+"\tsp\t"+QString::number(m_index)+"\t"+QString::number(m_drawErrorBars)+"\n";
	return s;
}

/// Returns the workspace index if a spectrum is plotted and -1 if it is a bin.
int MantidMatrixCurve::workspaceIndex()const
{
  if (dynamic_cast<const MantidQwtMatrixWorkspaceData*>(mantidData()) != 0)
  {
    return m_index;
  }
  return -1;
}

MantidQwtMatrixWorkspaceData* MantidMatrixCurve::mantidData()
{
  MantidQwtMatrixWorkspaceData* d = dynamic_cast<MantidQwtMatrixWorkspaceData*>(&data());
  return d;
}

const MantidQwtMatrixWorkspaceData* MantidMatrixCurve::mantidData()const
{
  const MantidQwtMatrixWorkspaceData* d = dynamic_cast<const MantidQwtMatrixWorkspaceData*>(&data());
  return d;
}

void MantidMatrixCurve::axisScaleChanged(int axis, bool toLog)
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

/// Enables/disables drawing as distribution, ie dividing each y-value by the bin width.
bool MantidMatrixCurve::setDrawAsDistribution(bool on)
{
  return mantidData()->setAsDistribution(on);
}

/// Returns whether the curve is plotted as a distribution
bool MantidMatrixCurve::isDistribution() const
{
  return mantidData()->m_isDistribution;
}

/// Returns whether the curve has error bars
bool MantidMatrixCurve::hasErrorBars() const
{
  return m_drawErrorBars;
}
