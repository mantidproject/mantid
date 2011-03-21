#include "MantidCurve.h"
#include <MantidAPI/MatrixWorkspace.h>
#include <MantidAPI/AnalysisDataService.h>

#include "../Graph.h"
#include "../ApplicationWindow.h"
#include "../MultiLayer.h"

#include <qpainter.h>
#include <stdexcept>
#include <limits>
#include <typeinfo>
#include <algorithm>

using namespace Mantid::API;

/**
 *  @param name :: The curve's name - shown in the legend
 *  @param wsName :: The workspace name.
 *  @param g :: The Graph widget which will display the curve
 *  @param index :: The index of the spectrum or bin in the workspace
 *  @param err :: True if the errors are to be plotted
 *..@throw Mantid::Kernel::Exception::NotFoundError if the workspace cannot be found
 *  @throw std::invalid_argument if the index is out of range for the given workspace
 */
MantidCurve::MantidCurve(const QString& name,const QString& wsName,Graph* g,const QString& type,int index,bool err)
  :PlotCurve(name), WorkspaceObserver(),m_drawErrorBars(err),m_drawAllErrorBars(false),m_wsName(wsName),m_index(index)
{
  (void) type; //avoid compiler warning

  MatrixWorkspace_const_sptr ws = boost::dynamic_pointer_cast<MatrixWorkspace>(
              AnalysisDataService::Instance().retrieve(wsName.toStdString()) );

  if ( !ws || index >= ws->getNumberHistograms() )
  {
    std::stringstream ss;
    ss << index << " is an invalid spectrum index for workspace " << ws->getName()
       << " - not plotted";
    throw std::invalid_argument(ss.str());
  }
  init(ws,g,index);
  observeDelete();
  connect( this, SIGNAL(resetData(const QString&)), this, SLOT(dataReset(const QString&)) );
  observeAfterReplace();
  observeADSClear();
}

/**
 *  @param wsName :: The workspace name.
 *  @param g :: The Graph widget which will display the curve
 *  @param index :: The index of the spectrum or bin in the workspace
 *  @param err :: True if the errors are to be plotted
 *  @throw std::invalid_argument if the index is out of range for the given workspace
 */
MantidCurve::MantidCurve(const QString& wsName,Graph* g,const QString& type,int index,bool err)
  :PlotCurve(), WorkspaceObserver(), m_drawErrorBars(err),m_drawAllErrorBars(false),m_wsName(wsName),m_index(index)
{
  (void) type; //avoid compiler warning

  MatrixWorkspace_const_sptr ws = boost::dynamic_pointer_cast<MatrixWorkspace>(
              AnalysisDataService::Instance().retrieve(wsName.toStdString()) );

  if ( !ws || index >= ws->getNumberHistograms() )
  {
    std::stringstream ss;
    ss << index << " is an invalid spectrum index for workspace " << ws->getName()
       << " - not plotted";
    throw std::invalid_argument(ss.str());
  }
  // If there's only one spectrum in the workspace, title is simply workspace name
  if (ws->getNumberHistograms() == 1) this->setTitle(wsName);
  else this->setTitle(createCurveName(ws,wsName,index));
  init(ws,g,index);
  observeDelete();
  connect( this, SIGNAL(resetData(const QString&)), this, SLOT(dataReset(const QString&)) );
  observeAfterReplace();
  observeADSClear();
}

MantidCurve::MantidCurve(const MantidCurve& c)
  :PlotCurve(createCopyName(c.title().text())),
  WorkspaceObserver(),
  m_drawErrorBars(c.m_drawErrorBars),
  m_drawAllErrorBars(c.m_drawAllErrorBars),
  m_wsName(c.m_wsName),
  m_index(c.m_index)
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
 *  @param index :: The index of the spectrum or bin in the workspace
 */
void MantidCurve::init(boost::shared_ptr<const Mantid::API::MatrixWorkspace> workspace,Graph* g,int index)
{
  //we need to censor the data if there is a log scale because it can't deal with negative values, only the y-axis has been found to be problem so far
  const bool log = g->isLog(QwtPlot::yLeft);
  MantidQwtData data(workspace,index, log);
  setData(data);

  int lineWidth = 1;
  MultiLayer* ml = (MultiLayer*)(g->parent()->parent()->parent());
  if (ml && ml->applicationWindow()->applyCurveStyleToMantid)
  {
    QwtPlotCurve::CurveStyle qwtStyle;
    switch(ml->applicationWindow()->defaultCurveStyle)
    {
    case Graph::Line:    qwtStyle = QwtPlotCurve::Lines; break;
    case Graph::Scatter: qwtStyle = QwtPlotCurve::Dots; break;
    case 15: qwtStyle = QwtPlotCurve::Steps; break;  // should be Graph::HorizontalSteps but it doesn't work
    default:  qwtStyle = QwtPlotCurve::Lines; break;
    }
    setStyle(qwtStyle);
    lineWidth = ml->applicationWindow()->defaultCurveLineWidth;
  }
  else if (workspace->isHistogramData() && !workspace->isDistribution())
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
}


MantidCurve::~MantidCurve()
{
}

void MantidCurve::setData(const QwtData &data)
{
  if (!dynamic_cast<const MantidQwtData*>(&data)) 
    throw std::runtime_error("Only MantidQwtData can be set to a MantidCurve");
  PlotCurve::setData(data);
}

QwtDoubleRect MantidCurve::boundingRect() const
{
  if (m_boundingRect.isNull())
  {
    const MantidQwtData* data = mantidData();
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

void MantidCurve::draw(QPainter *p, 
          const QwtScaleMap &xMap, const QwtScaleMap &yMap,
          const QRect &rect) const
{
  PlotCurve::draw(p,xMap,yMap,rect);

  if (m_drawErrorBars)// drawing error bars
  {
    const MantidQwtData* d = dynamic_cast<const MantidQwtData*>(&data());
    if (!d)
      throw std::runtime_error("Only MantidQwtData can be set to a MantidCurve");
    int xi0;
    p->setPen(pen());
    const int dx = 3;
    const int dx2 = 2*dx;
    int x1 = xMap.p1();
    int x2 = xMap.p2();
    for (int i = 0; i < d->esize(); i++)
    {
      const int xi = xMap.transform(d->ex(i));
      if (m_drawAllErrorBars || (xi > x1 && xi < x2 && abs(xi - xi0) > dx2))
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

void MantidCurve::itemChanged()
{
  MantidQwtData* d = dynamic_cast<MantidQwtData*>(&data());
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
QString MantidCurve::createCurveName(const boost::shared_ptr<const Mantid::API::MatrixWorkspace> ws,
                                     const QString& wsName,int index)
{
  QString name = wsName + "-" + QString::fromStdString(ws->getAxis(1)->label(index));
  return name;
}

/** Create the name for a curve which is a copy of another curve.
 *  @param curveName :: The original curve name.
 */
QString MantidCurve::createCopyName(const QString& curveName)
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
void MantidCurve::dataReset(const QString& wsName)
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
    Mantid::Kernel::Logger::get("MantidCurve").information() << "Workspace " << wsNameStd
        << " could not be found - plotted curve(s) deleted\n";
    mws = Mantid::API::MatrixWorkspace_sptr();
  }

  if (!mws) return;
  const MantidQwtData * new_mantidData(NULL);
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
    // Queue this plot to be updated once all MantidQwtData objects for this workspace have been
    emit dataUpdated();
  } catch(std::range_error) {
    // Get here if the new workspace has fewer spectra and the plotted one no longer exists
    Mantid::Kernel::Logger::get("MantidCurve").information() << "Workspace " << wsNameStd
        << " now has fewer spectra - plotted curve(s) deleted\n";
    deleteHandle(wsNameStd,mws);
  }
  delete new_mantidData;
}

void MantidCurve::afterReplaceHandle(const std::string& wsName,const boost::shared_ptr<Mantid::API::Workspace> ws)
{
  (void) ws;

  invalidateBoundingRect();
  emit resetData(QString::fromStdString(wsName));
}
/* This method saves the curve details to a string.
 * Useful for loading/saving mantid project.
*/
QString MantidCurve::saveToString()
{
	QString s;
	s="MantidCurve\t"+m_wsName+"\tsp\t"+QString::number(m_index)+"\t"+QString::number(m_drawErrorBars)+"\n";
	return s;
}

/// Returns the workspace index if a spectrum is plotted and -1 if it is a bin.
int MantidCurve::workspaceIndex()const
{
  if (dynamic_cast<const MantidQwtData*>(mantidData()) != 0)
  {
    return m_index;
  }
  return -1;
}

MantidQwtData* MantidCurve::mantidData()
{
  MantidQwtData* d = dynamic_cast<MantidQwtData*>(&data());
  return d;
}

const MantidQwtData* MantidCurve::mantidData()const
{
  const MantidQwtData* d = dynamic_cast<const MantidQwtData*>(&data());
  return d;
}

void MantidCurve::axisScaleChanged(int axis, bool toLog)
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

//==========================================
//
//  MantdQwtData methods
//
//==========================================

/// Constructor
MantidQwtData::MantidQwtData(Mantid::API::MatrixWorkspace_const_sptr workspace,int specIndex, const bool logScale)
: QObject(),
m_workspace(workspace),
m_spec(specIndex),
m_X(workspace->readX(specIndex)),
m_Y(workspace->readY(specIndex)),
m_E(workspace->readE(specIndex)),
m_isHistogram(workspace->isHistogramData()),
m_binCentres(false),
m_logScale(logScale),
m_minPositive(0)
{}

/// Copy constructor
MantidQwtData::MantidQwtData(const MantidQwtData& data)
: QObject(),
m_workspace(data.m_workspace),
m_spec(data.m_spec),
m_X(data.m_workspace->readX(data.m_spec)),
m_Y(data.m_workspace->readY(data.m_spec)),
m_E(data.m_workspace->readE(data.m_spec)),
m_isHistogram(m_workspace->isHistogramData()),
m_binCentres(data.m_binCentres),
m_logScale(data.m_logScale),
m_minPositive(0)
{}

/** Size of the data set
 */
size_t MantidQwtData::size() const
{
  if (m_binCentres || !m_isHistogram)
  {
    return m_Y.size();
  }

  return m_X.size();
}

/**
Return the x value of data point i
@param i :: Index
@return x X value of data point i
*/
double MantidQwtData::x(size_t i) const
{
  return m_binCentres ? (m_X[i] + m_X[i+1])/2 : m_X[i];
}

/**
Return the y value of data point i
@param i :: Index
@return y Y value of data point i
*/
double MantidQwtData::y(size_t i) const
{
  double tmp = i < m_Y.size() ? m_Y[i] : m_Y[m_Y.size()-1];
  if (m_logScale && tmp <= 0.)
  {
    tmp = m_minPositive;
  }

  return tmp;
}

double MantidQwtData::ex(size_t i) const
{
  return m_isHistogram ? (m_X[i] + m_X[i+1])/2 : m_X[i];
}

double MantidQwtData::e(size_t i) const
{
  return m_E[i];
}

int MantidQwtData::esize() const
{
  return m_E.size();
}

bool MantidQwtData::sameWorkspace(boost::shared_ptr<const Mantid::API::MatrixWorkspace> workspace)const
{
  return workspace.get() == m_workspace.get();
}

void MantidQwtData::setLogScale(bool on)
{
  m_logScale = on;
}

void MantidQwtData::saveLowestPositiveValue(const double v)
{
  if (v > 0) m_minPositive = v;
}

