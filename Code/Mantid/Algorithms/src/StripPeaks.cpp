#include "MantidAlgorithms/StripPeaks.h"
#include "MantidKernel/PhysicalConstants.h"
#include "MantidAPI/WorkspaceValidators.h"
#include "MantidDataObjects/TableRow.h"

namespace Mantid
{
namespace Algorithms
{

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(StripPeaks);

using namespace Kernel;
using namespace API;

// Get a reference to the logger.
Logger& StripPeaks::g_log = Logger::get("StripPeaks");

StripPeaks::StripPeaks() : API::Algorithm(), m_peaks() {}

void StripPeaks::init()
{
  // The validator can be removed when this algorithm is generalised
  declareProperty(new WorkspaceProperty<>("InputWorkspace","",Direction::Input,new WorkspaceUnitValidator<>("dSpacing")));
  declareProperty(new WorkspaceProperty<>("OutputWorkspace","",Direction::Output));

  // Set up the columns for the TableWorkspace holding the peak information
  m_peaks.createColumn("int","spectrum");
  m_peaks.createColumn("double","centre");
  m_peaks.createColumn("double","width");
  m_peaks.createColumn("double","height");
  m_peaks.createColumn("double","background");
  m_peaks.createColumn("int","bad");
}

void StripPeaks::exec()
{
  Workspace_sptr inputWS = getProperty("InputWorkspace");

  Workspace_sptr smoothWS = this->smoothInput(inputWS);
  this->findPeaks(smoothWS);
  smoothWS.reset();

  Workspace_sptr outputWS;
  if ( m_peaks.rowCount() > 0 )
  {
    this->fitPeaks(inputWS);
    outputWS = this->removePeaks(inputWS);
  }
  else
  {
    // No peaks found!!!
    outputWS = inputWS;
  }

  setProperty("OutputWorkspace",outputWS);
}

/** Smooths the data in the input workspace.
 *  The smoothed data will be used by the peak finding algorithm
 *  @param input The input workspace
 *  @return A workspace containing the smoothed data
 *  @throw std::runtime_error if the SmoothData subalgorithm fails
 */
API::Workspace_sptr StripPeaks::smoothInput(API::Workspace_sptr input)
{
  g_log.information("Smoothing the input data");
  Algorithm_sptr smooth = createSubAlgorithm("SmoothData");
  smooth->setProperty("InputWorkspace", input);
  smooth->setProperty("NPoints",5);
  try {
    smooth->execute();
  } catch (std::runtime_error) {
    g_log.error("Unable to successfully run SmoothData sub-algorithm");
    throw;
  }

  if ( ! smooth->isExecuted() )
  {
    g_log.error("Unable to successfully run SmoothData sub-algorithm");
    throw std::runtime_error("Unable to successfully run SmoothData sub-algorithm");
  }
  // Get back the result
  return smooth->getProperty("OutputWorkspace");
}

/** Searches for peaks in the smoothed data.
 *  @param WS The workspace to search
 */
void StripPeaks::findPeaks(API::Workspace_sptr WS)
{
  g_log.information("Searching for peaks");
  // Will in future use peak finding algorithm
  // For now, just hard-code in a few vanadium peaks
  for (int i = 0; i < WS->getNumberHistograms(); ++i)
  {
    double peakPositions[5] = { 2.14, 1.51, 1.23, 0.955, 0.81 };
    double heights[5] = { 3000, 600, 1000, 400, 500 };
    for (int j = 0; j < 5; ++j)
    {
      DataObjects::TableRow t = m_peaks.appendRow();
      t << i << peakPositions[j] << 0.01 << heights[j] << 0.0 << 0;
    }
  }
  return;
}

/** Performs a fit on the candidate peaks found
 *  @param WS The workspace to perform the fits on
 *  @throw std::runtime_error if the fitting sub-algorithm fails
 */
void StripPeaks::fitPeaks(API::Workspace_sptr WS)
{
  g_log.information("Fitting peaks");
  for (int i = 0; i < m_peaks.rowCount(); ++i)
  {
    const int currentSpec = m_peaks.getRef<int>("spectrum",i);
    g_log.information() << "Fitting peak for spectrum " << currentSpec << std::endl;
    Algorithm_sptr fit = createSubAlgorithm("Gaussian");
    fit->setProperty("InputWorkspace", WS);
    fit->setProperty("SpectrumIndex",currentSpec);
    fit->setProperty("bg0",m_peaks.getRef<double>("background",i));
    fit->setProperty("height",m_peaks.getRef<double>("height",i));
    fit->setProperty("peakCentre",m_peaks.getRef<double>("centre",i));
    fit->setProperty("sigma",m_peaks.getRef<double>("width",i));
    try {
      fit->execute();
    } catch (std::runtime_error) {
      g_log.error("Unable to successfully run Gaussian Fit sub-algorithm");
      throw;
    }

    if ( ! fit->isExecuted() )
    {
      g_log.error("Unable to successfully run Gaussian Fit sub-algorithm");
      throw std::runtime_error("Unable to successfully run Gaussian Fit sub-algorithm");
    }

    std::string fitStatus = fit->getProperty("Output Status");
    // These are some heuristic rules to discard bad fits.
    // Hope to be able to remove them when peak finding algorithm is in place
    const double height = fit->getProperty("height");
    if ( height < 0 ) fitStatus.clear();
    const double background = fit->getProperty("bg0");
    if ( background < 0 ) fitStatus.clear();
    const double width = fit->getProperty("sigma");
    if ( std::abs(width) > 0.03 ) fitStatus.clear();

    if ( ! fitStatus.compare("success") )
    {
      DataObjects::TableRow t = m_peaks.getRow(i);
      const double centre = fit->getProperty("peakCentre");
      t << currentSpec << centre << width << height << background;
      continue;
    }

    // Only get to here if fit fails
    g_log.warning("Peak fit failed to converge");
    m_peaks.getRef<int>("bad",i) = 1;
  }

}

/** If a peak was successfully fitted, it is subtracted from the data
 *  @param input The input workspace
 *  @return A workspace containing the peak-subtracted data
 */
API::Workspace_sptr StripPeaks::removePeaks(API::Workspace_sptr input)
{
  g_log.information("Subtracting peaks");
  Workspace_sptr outputWS = WorkspaceFactory::Instance().create(input);
  const int hists = input->getNumberHistograms();
  for (int k = 0; k < hists; ++k)
  {
    outputWS->dataX(k) = input->readX(k);
    outputWS->dataY(k) = input->readY(k);
    outputWS->dataE(k) = input->readE(k);
  }
  const bool isHistogramData = outputWS->isHistogramData();
  for (int i = 0; i < m_peaks.rowCount(); ++i)
  {
    if (m_peaks.getRef<int>("bad",i)) continue;
    g_log.debug() << "Subtracting peak from spectrum " << m_peaks.getRef<int>("spectrum",i) << std::endl;
    const std::vector<double> &X = outputWS->readX(m_peaks.getRef<int>("spectrum",i));
    std::vector<double> &Y = outputWS->dataY(m_peaks.getRef<int>("spectrum",i));
    const double height = m_peaks.getRef<double>("height",i);
    const double centre = m_peaks.getRef<double>("centre",i);
    const double width = m_peaks.getRef<double>("width",i);
    const int spectrumLength = Y.size();
    for (int j = 0; j < spectrumLength; ++j)
    {
      double x;
      if (isHistogramData) x = 0.5*(X[j]+X[j+1]);
        else x = X[j];
      if ( x < centre-3.0*width ) continue;
      if ( x > centre+3.0*width ) break;
      const double funcVal = height*exp(-0.5*pow((x-centre)/width,2));
      Y[j] -= funcVal;
    }
  }

  return outputWS;
}

} // namespace Algorithms
} // namespace Mantid
