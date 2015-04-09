#include "MantidQtCustomInterfaces/MultiDatasetFit/MDFDatasetPlotData.h"

#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidKernel/Exception.h"

#include <qwt_plot_curve.h>

namespace MantidQt
{
namespace CustomInterfaces
{
namespace MDF
{
  
/// Constructor.
/// @param wsName :: Name of a MatrixWorkspace with the data for fitting.
/// @param wsIndex :: Workspace index of a spectrum in wsName to plot.
/// @param outputWSName :: Name of the Fit's output workspace containing at least 3 spectra:
///    #0 - original data (the same as in wsName[wsIndex]), #1 - calculated data, #3 - difference.
///    If empty - ignore this workspace.
DatasetPlotData::DatasetPlotData(const QString& wsName, int wsIndex, const QString& outputWSName):
  m_dataCurve(new QwtPlotCurve(wsName + QString(" (%1)").arg(wsIndex))),
  m_calcCurve(NULL),
  m_diffCurve(NULL)
{
  // get the data workspace
  auto ws = Mantid::API::AnalysisDataService::Instance().retrieveWS<Mantid::API::MatrixWorkspace>( wsName.toStdString() );
  if ( !ws )
  {
    QString mess = QString("Workspace %1 either doesn't exist or isn't a MatrixWorkspace").arg(wsName);
    throw std::runtime_error( mess.toStdString() );
  }
  // check that the index is in range
  if ( static_cast<size_t>(wsIndex) >= ws->getNumberHistograms() )
  {
    QString mess = QString("Spectrum %1 doesn't exist in workspace %2").arg(wsIndex).arg(wsName);
    throw std::runtime_error( mess.toStdString() );
  }

  // get the data workspace
  Mantid::API::MatrixWorkspace_sptr outputWS;
  if ( !outputWSName.isEmpty() )
  {
    std::string stdOutputWSName = outputWSName.toStdString();
    if ( Mantid::API::AnalysisDataService::Instance().doesExist(stdOutputWSName) )
    {
      try
      {
        outputWS = Mantid::API::AnalysisDataService::Instance().retrieveWS<Mantid::API::MatrixWorkspace>( stdOutputWSName );
      }
      catch ( Mantid::Kernel::Exception::NotFoundError& )
      {
        QString mess = QString("Workspace %1 either doesn't exist or isn't a MatrixWorkspace").arg(outputWSName);
        throw std::runtime_error( mess.toStdString() );
      }
    }
  }

  // create the curves
  setData( ws.get(), wsIndex, outputWS.get() );
}

/// Destructor.
DatasetPlotData::~DatasetPlotData()
{
  m_dataCurve->detach();
  delete m_dataCurve;
  if ( m_calcCurve )
  {
    m_calcCurve->detach();
    delete m_calcCurve;
  }
  if ( m_diffCurve )
  {
    m_diffCurve->detach();
    delete m_diffCurve;
  }
}

/// Set the data to the curves.
/// @param ws :: A Fit's input workspace.
/// @param wsIndex :: Workspace index of a spectrum to costruct the plot data for.
/// @param outputWS :: The output workspace from Fit containing the calculated spectrum.
void DatasetPlotData::setData(const Mantid::API::MatrixWorkspace *ws, int wsIndex, const Mantid::API::MatrixWorkspace *outputWS)
{
  bool haveFitCurves = outputWS && outputWS->getNumberHistograms() >= 3;
  std::vector<double> xValues = ws->readX(wsIndex);
  if ( ws->isHistogramData() )
  {
    auto xend = xValues.end() - 1;
    for(auto x = xValues.begin(); x != xend; ++x)
    {
      *x = (*x + *(x+1))/2;
    }
    xValues.pop_back();
  }
  m_dataCurve->setData( xValues.data(), ws->readY(wsIndex).data(), static_cast<int>(xValues.size()) );

  if ( haveFitCurves )
  {
    auto xBegin = std::lower_bound(xValues.begin(),xValues.end(), outputWS->readX(1).front());
    if ( xBegin == xValues.end() ) return;
    int i0 = static_cast<int>(std::distance(xValues.begin(),xBegin));
    int n = static_cast<int>(outputWS->readY(1).size());
    if ( i0 + n > static_cast<int>(xValues.size()) ) return;
    m_calcCurve = new QwtPlotCurve("calc");
    m_calcCurve->setData( xValues.data() + i0, outputWS->readY(1).data(), n );
    QPen penCalc("red");
    m_calcCurve->setPen(penCalc);
    m_diffCurve = new QwtPlotCurve("diff");
    m_diffCurve->setData( xValues.data() + i0, outputWS->readY(2).data(), n );
    QPen penDiff("green");
    m_diffCurve->setPen(penDiff);
  }
}

/// Show the curves on a plot.
void DatasetPlotData::show(QwtPlot *plot)
{
  m_dataCurve->attach(plot);
  if ( m_calcCurve )
  {
    m_calcCurve->attach(plot);
  }
  if ( m_diffCurve )
  {
    m_diffCurve->attach(plot);
  }
}

/// Hide the curves from any plot.
void DatasetPlotData::hide()
{
  m_dataCurve->detach();
  if ( m_calcCurve )
  {
    m_calcCurve->detach();
  }
  if ( m_diffCurve )
  {
    m_diffCurve->detach();
  }
}

/// Get the bounding rect including all plotted data.
QwtDoubleRect DatasetPlotData::boundingRect() const
{
  QwtDoubleRect rect = m_dataCurve->boundingRect();
  if ( m_calcCurve )
  {
    rect = rect.united( m_calcCurve->boundingRect() );
  }
  if ( m_diffCurve )
  {
    rect = rect.united( m_diffCurve->boundingRect() );
  }
  return rect;
}

} // MDF
} // CustomInterfaces
} // MantidQt
