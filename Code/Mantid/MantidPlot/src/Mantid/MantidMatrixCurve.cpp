#include "MantidMatrixCurve.h"

#include <qpainter.h>
#include <qwt_symbol.h>

#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/AnalysisDataService.h"

#include "MantidQtAPI/QwtWorkspaceSpectrumData.h"
#include "MantidQtAPI/QwtWorkspaceBinData.h"

#include "../Graph.h"
#include "../ApplicationWindow.h"
#include "../MultiLayer.h"
#include "ErrorBarSettings.h"
#include "MantidKernel/ReadLock.h"


using namespace Mantid::API;
using namespace MantidQt::API;
using Mantid::Kernel::ReadLock;

namespace
{
  /// static logger
  Mantid::Kernel::Logger g_log("MantidMatrixCurve");
}


/**
 *  @param name :: The curve's name - shown in the legend
 *  @param wsName :: The workspace name.
 *  @param g :: The Graph widget which will display the curve
 *  @param index :: The index of the spectrum or bin in the workspace
 *  @param indexType :: Enum indicating whether the index is a spectrum/bin index
 *  @param err :: True if the errors are to be plotted
 *  @param distr :: True if it is a distribution
 *  @param style :: CurveType style to use
 *..@throw Mantid::Kernel::Exception::NotFoundError if the workspace cannot be found
 *  @throw std::invalid_argument if the index is out of range for the given workspace
 */
MantidMatrixCurve::MantidMatrixCurve(const QString&,const QString& wsName,Graph* g,int index, IndexDir indexType,
                                     bool err,bool distr, Graph::CurveType style)
  :MantidCurve(err),
  m_wsName(wsName),
  m_index(index), m_indexType(indexType)
{
  if(!g)
  {
    throw std::invalid_argument("MantidMatrixCurve::MantidMatrixCurve - NULL graph pointer not allowed");
  }
  init(g,distr,style);
}

/**
 *  @param wsName :: The workspace name.
 *  @param g :: The Graph widget which will display the curve
 *  @param index :: The index of the spectrum or bin in the workspace
 *  @param indexType :: Enum indicating whether the index is a spectrum/bin index
 *  @param err :: True if the errors are to be plotted
 *  @param distr :: True if it is a distribution
 *  @param style :: CurveType style to use
 *  @throw std::invalid_argument if the index is out of range for the given workspace
 */
MantidMatrixCurve::MantidMatrixCurve(const QString& wsName,Graph* g,int index, IndexDir indexType, bool err,bool distr, Graph::CurveType style)
  :MantidCurve(err),
  m_wsName(wsName),
  m_index(index), m_indexType(indexType)
{
  init(g,distr,style);
}


MantidMatrixCurve::MantidMatrixCurve(const MantidMatrixCurve& c)
  :MantidCurve(createCopyName(c.title().text()), c.m_drawErrorBars, c.m_drawAllErrorBars),
  m_wsName(c.m_wsName),
  m_index(c.m_index), m_indexType(c.m_indexType),
  m_xUnits(c.m_xUnits),
  m_yUnits(c.m_yUnits)
{
  setData(c.data());
  observePostDelete();
  connect( this, SIGNAL(resetData(const QString&)), this, SLOT(dataReset(const QString&)) );
  observeAfterReplace();
  observeADSClear();
}

/**
 *  @param g :: The Graph widget which will display the curve
 *  @param distr :: True for a distribution
 *  @param style :: The curve type to use
 */
void MantidMatrixCurve::init(Graph* g,bool distr,Graph::CurveType style)
{
  // Will throw if name not found but return NULL ptr if the type is incorrect
  MatrixWorkspace_const_sptr workspace = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(m_wsName.toStdString() );

  if ( !workspace ) // The respective *Data classes will check for index validity
  {
    std::stringstream ss;
    ss << "Workspace named '" << m_wsName.toStdString()
       << "' found but it is not a MatrixWorkspace. ID='"
       << AnalysisDataService::Instance().retrieve(m_wsName.toStdString() )->id() << "'";
    throw std::invalid_argument(ss.str());
  }

  // Set the curve name if it the non-naming constructor was called
  if ( this->title().isEmpty() )
  {
    // If there's only one spectrum in the workspace, title is simply workspace name
    if (workspace->getNumberHistograms() == 1) this->setTitle(m_wsName);
    else this->setTitle(createCurveName(workspace));
  }

  Mantid::API::MatrixWorkspace_const_sptr matrixWS = boost::dynamic_pointer_cast<const Mantid::API::MatrixWorkspace>(workspace);
  //we need to censor the data if there is a log scale because it can't deal with negative values, only the y-axis has been found to be problem so far
  const bool log = g->isLog(QwtPlot::yLeft);

  // Y units are the same for both spectrum and bin plots, e.g. counts
  m_yUnits.reset(new Mantid::Kernel::Units::Label(matrixWS->YUnit(), matrixWS->YUnitLabel()));

  if(m_indexType == Spectrum) // Spectrum plot
  {
    QwtWorkspaceSpectrumData data(*matrixWS,m_index, log,distr);
    setData(data);

    // For spectrum plots, X axis are actual X axis, e.g. TOF
    m_xUnits = matrixWS->getAxis(0)->unit();
  }
  else // Bin plot
  {
    QwtWorkspaceBinData data(*matrixWS, m_index,log);
    setData(data);

    // For bin plots, X axis are "spectra axis", e.g. spectra numbers
    m_xUnits = matrixWS->getAxis(1)->unit();
  }

  if (!m_xUnits)
  {
    m_xUnits.reset(new Mantid::Kernel::Units::Empty());
  }

  int lineWidth = 1;
  MultiLayer* ml = dynamic_cast<MultiLayer*>(g->parent()->parent()->parent());
  if (style == Graph::Unspecified || (ml && ml->applicationWindow()->applyCurveStyleToMantid) )
  {
    applyStyleChoice(style, ml, lineWidth);
  }
  else if (matrixWS->isHistogramData() && !matrixWS->isDistribution())
  {
    setStyle(QwtPlotCurve::Steps);
    setCurveAttribute(Inverted,true);// this is the Steps style modifier that makes horizontal steps
  }
  else
  {
    setStyle(QwtPlotCurve::Lines);
  }
  g->insertCurve(this,lineWidth);

  // set the option to draw all error bars from the global settings
  if ( hasErrorBars() )
  {
    setErrorBars( true, g->multiLayer()->applicationWindow()->drawAllErrors );
  }
  // Initialise error bar colour to match curve colour
  m_errorSettings->m_color = pen().color();
  m_errorSettings->setWidth(pen().widthF());

  connect(g,SIGNAL(axisScaleChanged(int,bool)),this,SLOT(axisScaleChanged(int,bool)));
  observePostDelete();
 	connect( this, SIGNAL(resetData(const QString&)), this, SLOT(dataReset(const QString&)) );
 	observeAfterReplace();
 	observeADSClear();
}


MantidMatrixCurve::~MantidMatrixCurve()
{
  observePostDelete(false);
  observeAfterReplace(false);
  observeADSClear(false);
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
}

void MantidMatrixCurve::setData(const QwtData &data)
{
  if (!dynamic_cast<const MantidQwtWorkspaceData*>(&data))
    throw std::runtime_error("Only MantidQwtWorkspaceData can be set to a MantidMatrixCurve");
  PlotCurve::setData(data);
}

QwtDoubleRect MantidMatrixCurve::boundingRect() const
{
  return MantidCurve::boundingRect();
}

void MantidMatrixCurve::draw(QPainter *p, 
          const QwtScaleMap &xMap, const QwtScaleMap &yMap,
          const QRect &rect) const
{
  PlotCurve::draw(p,xMap,yMap,rect);

  if (m_drawErrorBars)// drawing error bars
  {
    const MantidQwtWorkspaceData* d = dynamic_cast<const MantidQwtWorkspaceData*>(&data());
    if (!d)
    {
      throw std::runtime_error("Only MantidQwtWorkspaceData can be set to a MantidMatrixCurve");
    }
    p->translate(d_x_offset,-d_y_offset); // For waterfall plots (will be zero otherwise)
                                          // Don't really know why you'd want errors on a waterfall plot, but just in case...
    doDraw(p,xMap,yMap,rect,d);
  }
}

void MantidMatrixCurve::itemChanged()
{
  QwtWorkspaceSpectrumData* d = dynamic_cast<QwtWorkspaceSpectrumData*>(&data());
  if (d && d->m_isHistogram)
  {
    if (style() == Steps) d->m_binCentres = false;
    else
      d->m_binCentres = true;
  }
  PlotCurve::itemChanged();
}


/**
 * Create the name for a curve from the following input:
 * @param ws :: Pointer to workspace
 */
QString MantidMatrixCurve::createCurveName(const boost::shared_ptr<const Mantid::API::MatrixWorkspace> ws)
{
  QString name = m_wsName + "-";
  if(m_indexType == Spectrum) name += QString::fromStdString(ws->getAxis(1)->label(m_index));
  else name += "bin-" + QString::number(m_index);
  return name;
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
    g_log.information() << "Workspace " << wsNameStd
        << " could not be found - plotted curve(s) deleted\n";
    mws = Mantid::API::MatrixWorkspace_sptr();
  }
  if (!mws) return;

  // Acquire a read-lock on the matrix workspace data
  ReadLock _lock(*mws);

  try 
  {
    const auto *newData = mantidData()->copyWithNewSource(*mws);
    setData(*newData);
    delete newData;
    // Queue this plot to be updated once all QwtWorkspaceSpectrumData objects for this workspace have been
    emit dataUpdated();
  } 
  catch(std::range_error &) 
  {
    // Get here if the new workspace has fewer spectra and the plotted one no longer exists
    g_log.information() << "Workspace " << wsNameStd
        << " now has fewer spectra - plotted curve(s) deleted\n";
    postDeleteHandle(wsNameStd);
  }
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
  s = "MantidMatrixCurve\t" + m_wsName + "\t sp \t" + QString::number(m_index) + "\t"
      + QString::number(m_drawErrorBars) + "\t" + QString::number(isDistribution()) + "\t";
  return s;
}

/// Returns the workspace index if a spectrum is plotted and -1 if it is a bin.
int MantidMatrixCurve::workspaceIndex()const
{
  if (dynamic_cast<const QwtWorkspaceSpectrumData*>(mantidData()) != 0)
  {
    return m_index;
  }
  return -1;
}

MantidQwtMatrixWorkspaceData * MantidMatrixCurve::mantidData()
{
  auto * d = dynamic_cast<MantidQwtMatrixWorkspaceData*>(&data());
  return d;
}

const MantidQwtMatrixWorkspaceData * MantidMatrixCurve::mantidData() const
{
  const auto *d = dynamic_cast<const MantidQwtMatrixWorkspaceData*>(&data());
  return d;
}

/// Enables/disables drawing as distribution, ie dividing each y-value by the bin width.
bool MantidMatrixCurve::setDrawAsDistribution(bool on)
{
  if( auto *d = dynamic_cast<QwtWorkspaceSpectrumData*>(&data()))
  {
    return d->setAsDistribution(on);
  }
  else return false;
}

/// Returns whether the curve is plotted as a distribution
bool MantidMatrixCurve::isDistribution() const
{
  if( auto *d = dynamic_cast<const QwtWorkspaceSpectrumData*>(&data()))
  {
    return d->m_isDistribution;
  }
  else return false;
}

bool MantidMatrixCurve::isHistogramData() const
{
  if( auto *d = dynamic_cast<const QwtWorkspaceSpectrumData*>(&data()))
  {
    return d->isHistogram();
  }
  else return false;
}

bool MantidMatrixCurve::isNormalizable() const
{
  if( auto *d = dynamic_cast<const QwtWorkspaceSpectrumData*>(&data()))
  {
    return d->isHistogram() && !d->dataIsNormalized();
  }
  else return false;
}
