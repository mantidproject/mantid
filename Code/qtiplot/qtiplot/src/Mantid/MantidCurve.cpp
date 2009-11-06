#include "MantidCurve.h"
#include <MantidAPI/MatrixWorkspace.h>
#include <MantidAPI/AnalysisDataService.h>

#include "../Graph.h"

#include <qpainter.h>
#include <stdexcept>

/**
 *  @param name The curve's name - shown in the legend
 *  @param wsName The workspace name.
 *  @param g The Graph widget which will display the curve
 *  @param type The type of the QwtData: "spectra" for MantidQwtDataSpectra or
 *              "bin" for MantidQwtDataBin
 *  @param index The index of the spectrum or bin in the workspace
 *  @param err True if the errors are to be plotted
 */
MantidCurve::MantidCurve(const QString& name,const QString& wsName,Graph* g,
                         const QString& type,int index,bool err)
  :PlotCurve(name),m_drawErrorBars(err),m_wsName(wsName)
{
  Mantid::API::MatrixWorkspace_sptr ws = 
    boost::dynamic_pointer_cast<Mantid::API::MatrixWorkspace>(
       Mantid::API::AnalysisDataService::Instance().retrieve(wsName.toStdString())
    );
  init(ws,g,type,index);
  observeDelete();
  connect( this, SIGNAL(resetData(const QString&)), this, SLOT(dataReset(const QString&)) );
  observeAfterReplace();
}

/**
 *  @param wsName The workspace name.
 *  @param g The Graph widget which will display the curve
 *  @param type The type of the QwtData: "spectra" for MantidQwtDataSpectra or
 *              "bin" for MantidQwtDataBin
 *  @param index The index of the spectrum or bin in the workspace
 *  @param err True if the errors are to be plotted
 */
MantidCurve::MantidCurve(const QString& wsName,Graph* g,
                         const QString& type,int index,bool err)
 :PlotCurve(),m_drawErrorBars(err),m_wsName(wsName)
{
  Mantid::API::MatrixWorkspace_sptr ws = 
    boost::dynamic_pointer_cast<Mantid::API::MatrixWorkspace>(
       Mantid::API::AnalysisDataService::Instance().retrieve(wsName.toStdString())
    );
  // If there's only one spectrum in the workspace, title is simply workspace name
  if (ws->getNumberHistograms() == 1) this->setTitle(wsName);
  else this->setTitle(createCurveName(wsName,type,index));
  init(ws,g,type,index);
  observeDelete();
  connect( this, SIGNAL(resetData(const QString&)), this, SLOT(dataReset(const QString&)) );
  observeAfterReplace();
}

MantidCurve::MantidCurve(const MantidCurve& c)
  :PlotCurve(createCopyName(c.title().text())),m_drawErrorBars(c.m_drawErrorBars),m_wsName(c.m_wsName)
{
  setData(c.data());
  observeDelete();
  connect( this, SIGNAL(resetData(const QString&)), this, SLOT(dataReset(const QString&)) );
  observeAfterReplace();
}

/**
 *  @param workspace The source workspace for the curve's data
 *  @param g The Graph widget which will display the curve
 *  @param type The type of the QwtData: "spectra" for MantidQwtDataSpectra or
 *              "bin" for MantidQwtDataBin
 *  @param index The index of the spectrum or bin in the workspace
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
    throw std::runtime_error("Unrecognized MuntidCurve type " + type.toStdString());
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
    for (int i = 0; i < dataSize(); i++)
    {
      const int xi = xMap.transform(d->ex(i));
      const double Y = y(i);
      const double E = d->e(i);
      const int ei1 = yMap.transform(Y - E);
      const int ei2 = yMap.transform(Y + E);

      p->setPen(pen());
      p->drawLine(xi,ei1,xi,ei2);
      p->drawLine(xi-5,ei1,xi+5,ei1);
      p->drawLine(xi-5,ei2,xi+5,ei2);
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
  Mantid::API::MatrixWorkspace_sptr mws = 
    boost::dynamic_pointer_cast<Mantid::API::MatrixWorkspace>(
       Mantid::API::AnalysisDataService::Instance().retrieve(wsNameStd)
    );
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
  return m_Y.size();
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
  return m_Y[i];
}

double MantidQwtDataSpectra::ex(size_t i) const
{
  return m_isHistogram ? (m_X[i] + m_X[i+1])/2 : m_X[i];
}

double MantidQwtDataSpectra::e(size_t i) const
{
  return m_E[i];
}

