#include "MantidCurve.h"
#include <MantidAPI/MatrixWorkspace.h>
#include <MantidAPI/AnalysisDataService.h>

#include "../Graph.h"

#include <qpainter.h>
#include <stdexcept>

using namespace Mantid::API;

/**
 *  @param name The curve's name - shown in the legend
 *  @param wsName The workspace name.
 *  @param g The Graph widget which will display the curve
 *  @param type The type of the QwtData: "spectra" for MantidQwtDataSpectra or
 *              "bin" for MantidQwtDataBin (N.B. "bin" is not yet implemented)
 *  @param index The index of the spectrum or bin in the workspace
 *  @param err True if the errors are to be plotted
 *..@throw std::runtime_error if an invalid string is given in the type argument
 *..@throw Mantid::Kernel::Exception::NotFoundError if the workspace cannot be found
 *  @throw std::invalid_argument if the index is out of range for the given workspace
 */
MantidCurve::MantidCurve(const QString& name,const QString& wsName,Graph* g,
                         const QString& type,int index,bool err)
  :PlotCurve(name), WorkspaceObserver(),m_drawErrorBars(err),m_wsName(wsName),m_type(type),m_index(index)
{
  MatrixWorkspace_const_sptr ws = boost::dynamic_pointer_cast<MatrixWorkspace>(
              AnalysisDataService::Instance().retrieve(wsName.toStdString()) );

  if ( !ws || index >= ws->getNumberHistograms() )
  {
    std::stringstream ss;
    ss << index << " is an invalid spectrum index for workspace " << ws->getName()
       << " - not plotted";
    throw std::invalid_argument(ss.str());
  }
  init(ws,g,type,index);
  observeDelete();
  connect( this, SIGNAL(resetData(const QString&)), this, SLOT(dataReset(const QString&)) );
  observeAfterReplace();
  observeADSClear();
}

/**
 *  @param wsName The workspace name.
 *  @param g The Graph widget which will display the curve
 *  @param type The type of the QwtData: "spectra" for MantidQwtDataSpectra or
 *              "bin" for MantidQwtDataBin (N.B. "bin" is not yet implemented)
 *  @param index The index of the spectrum or bin in the workspace
 *  @param err True if the errors are to be plotted
 *..@throw std::runtime_error if an invalid string is given in the type argument
 *  @throw std::invalid_argument if the index is out of range for the given workspace
 */
MantidCurve::MantidCurve(const QString& wsName,Graph* g,
                         const QString& type,int index,bool err)
  :PlotCurve(), WorkspaceObserver(), m_drawErrorBars(err),m_wsName(wsName),m_type(type),m_index(index)
{
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
  else this->setTitle(createCurveName(wsName,type,index));
  init(ws,g,type,index);
  observeDelete();
  connect( this, SIGNAL(resetData(const QString&)), this, SLOT(dataReset(const QString&)) );
  observeAfterReplace();
  observeADSClear();
}

MantidCurve::MantidCurve(const MantidCurve& c)
  :PlotCurve(createCopyName(c.title().text())), WorkspaceObserver(), m_drawErrorBars(c.m_drawErrorBars),m_wsName(c.m_wsName),m_type(c.m_type),m_index(c.m_index)
{
  setData(c.data());
  observeDelete();
  connect( this, SIGNAL(resetData(const QString&)), this, SLOT(dataReset(const QString&)) );
  observeAfterReplace();
  observeADSClear();
}

/**
 *  @param workspace The source workspace for the curve's data
 *  @param g The Graph widget which will display the curve
 *  @param type The type of the QwtData: "spectra" for MantidQwtDataSpectra or
 *              "bin" for MantidQwtDataBin (N.B. "bin" is not yet implemented)
 *  @param index The index of the spectrum or bin in the workspace
 *..@throw std::runtime_error if an invalid string is given in the type argument
 */
void MantidCurve::init(boost::shared_ptr<const Mantid::API::MatrixWorkspace> workspace,Graph* g,
                         const QString& type,int index)
{
  if (type == "spectra")
  {
    MantidQwtDataSpectra data(workspace,index);
    setData(data);
  }
  else
    throw std::runtime_error("Unrecognized MantidCurve type " + type.toStdString());
  if (workspace->isHistogramData())
  {
    setStyle(QwtPlotCurve::Steps);
    setCurveAttribute(Inverted,true);// this is the Steps style modifier that makes horizontal steps
  }
  else
    setStyle(QwtPlotCurve::Lines);
  if (g)
  {
    g->insertCurve(this);
  }
}


MantidCurve::~MantidCurve()
{
  //std::cerr<<"MantidCurve deleted...\n";
}

void MantidCurve::setData(const QwtData &data)
{
  if (!dynamic_cast<const MantidQwtData*>(&data)) 
    throw std::runtime_error("Only MantidQwtData can be set to a MantidCurve");
  PlotCurve::setData(data);
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
      if (xi > x1 && xi < x2 && abs(xi - xi0) > dx2)
      {
        const double Y = y(i);
        const double E = d->e(i);
        const int ei1 = yMap.transform(Y - E);
        const int ei2 = yMap.transform(Y + E);

        p->drawLine(xi,ei1,xi,ei2);
        p->drawLine(xi-dx,ei1,xi+dx,ei1);
        p->drawLine(xi-dx,ei2,xi+dx,ei2);

        xi0 = xi;
      }
    }
  }
}

void MantidCurve::itemChanged()
{
  MantidQwtDataSpectra* d = dynamic_cast<MantidQwtDataSpectra*>(&data());
  if (d && d->m_isHistogram)
  {
    if (style() == Steps) d->m_binCentres = false;
    else
      d->m_binCentres = true;
  }
  PlotCurve::itemChanged();
}

/** Create the name for a curve from the following input:
 *  @param wsName The workspace name
 *  @param type   The data type ("spectra", "bin",...)
 *  @param index  The spectra (bin) index
 */
QString MantidCurve::createCurveName(const QString& wsName,const QString& type,int index)
{
  QString name = wsName + "-";
  if (type == "spectra")
    name += "sp-";
  name += QString::number(index);
  return name;
}

/** Create the name for a curve which is a copy of another curve.
 *  @param curveName The original curve name.
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
 *  @param wsName The name of a workspace which data has been changed in the data service.
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
  emit resetData(QString::fromStdString(wsName));
}
/* This method saves the curve details to a string.
 * Useful for loading/saving mantid project.
*/
QString MantidCurve::saveToString()
{
	QString s;
	s="MantidCurve\t"+m_wsName+"\t"+m_type+"\t"+QString::number(m_index)+"\t"+QString::number(m_drawErrorBars)+"\n";
	return s;
}

/// Returns the workspace index if a spectrum is plotted and -1 if it is a bin.
int MantidCurve::workspaceIndex()const
{
  if (dynamic_cast<const MantidQwtDataSpectra*>(mantidData()) != 0)
  {
    return m_index;
  }
  return -1;
}

//==========================================
//
//  MantdQwtData methods
//
//==========================================
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

bool MantidQwtData::sameWorkspace(boost::shared_ptr<const Mantid::API::MatrixWorkspace> workspace)const
{
  return workspace.get() == m_workspace.get();
}

/// Constructor
MantidQwtDataSpectra::MantidQwtDataSpectra(boost::shared_ptr<const Mantid::API::MatrixWorkspace> workspace,int specIndex)
:MantidQwtData(workspace),m_spec(specIndex),
m_X(workspace->readX(specIndex)),
m_Y(workspace->readY(specIndex)),
m_E(workspace->readE(specIndex)),
m_isHistogram(workspace->isHistogramData()),
m_binCentres(false)
{}

/// Copy constructor
MantidQwtDataSpectra::MantidQwtDataSpectra(const MantidQwtDataSpectra& data)
:MantidQwtData(data),m_spec(data.m_spec),
m_X(data.m_workspace->readX(data.m_spec)),
m_Y(data.m_workspace->readY(data.m_spec)),
m_E(data.m_workspace->readE(data.m_spec)),
m_isHistogram(m_workspace->isHistogramData()),
m_binCentres(false)
{}

MantidQwtData::~MantidQwtData()
{
}


/** Size of the data set
 */
size_t MantidQwtDataSpectra::size() const
{
  if (m_binCentres || !m_isHistogram)
  {
    return m_Y.size();
  }

  return m_X.size();
}

/**
Return the x value of data point i
\param i Index
\return x X value of data point i
*/
double MantidQwtDataSpectra::x(size_t i) const
{
  return m_binCentres ? (m_X[i] + m_X[i+1])/2 : m_X[i];
}

/**
Return the y value of data point i
\param i Index
\return y Y value of data point i
*/
double MantidQwtDataSpectra::y(size_t i) const
{
  return i < m_Y.size() ? m_Y[i] : m_Y[m_Y.size()-1];
}

double MantidQwtDataSpectra::ex(size_t i) const
{
  return m_isHistogram ? (m_X[i] + m_X[i+1])/2 : m_X[i];
}

double MantidQwtDataSpectra::e(size_t i) const
{
  return m_E[i];
}

int MantidQwtDataSpectra::esize() const
{
  return m_E.size();
}

