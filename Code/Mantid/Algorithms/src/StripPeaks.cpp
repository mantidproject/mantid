#include "MantidAlgorithms/StripPeaks.h"
#include "MantidKernel/PhysicalConstants.h"
#include "MantidAPI/TableRow.h"

namespace Mantid
{
namespace Algorithms
{

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(StripPeaks)

using namespace Kernel;
using namespace API;

// Get a reference to the logger.
Logger& StripPeaks::g_log = Logger::get("StripPeaks");

StripPeaks::StripPeaks() : API::Algorithm() {}

void StripPeaks::init()
{
  declareProperty(new WorkspaceProperty<>("InputWorkspace","",Direction::Input));
  declareProperty(new WorkspaceProperty<>("OutputWorkspace","",Direction::Output));
}

void StripPeaks::exec()
{
  // Retrieve the input workspace
  MatrixWorkspace_sptr inputWS = getProperty("InputWorkspace");

  // Call FindPeaks as a sub-algorithm
  ITableWorkspace_sptr peakslist = this->findPeaks(inputWS);

  MatrixWorkspace_sptr outputWS;
  if ( peakslist->rowCount() > 0 )
  {
    // Remove the peaks that were successfully fitted
    outputWS = this->removePeaks(inputWS, peakslist);
  }
  else
  {
    // No peaks found!!! - just assign the input workspace pointer to the output one
    g_log.warning("No peaks to remove!");
    outputWS = inputWS;
  }

  // Set the output workspace
  setProperty("OutputWorkspace",outputWS);
}

/** Calls FindPeaks as a sub-algorithm.
 *  @param WS The workspace to search
 */
API::ITableWorkspace_sptr StripPeaks::findPeaks(API::MatrixWorkspace_sptr WS)
{
  g_log.information("Calling FindPeaks as a sub-algorithm");

  API::IAlgorithm_sptr findpeaks = createSubAlgorithm("FindPeaks");
  findpeaks->setProperty("InputWorkspace", WS);

  // Now execute the sub-algorithm. Catch and log any error
  try
  {
    findpeaks->execute();
  }
  catch (std::runtime_error& err)
  {
    g_log.error("Unable to successfully run FindPeaks sub-algorithm");
    throw;
  }

  if ( ! findpeaks->isExecuted() ) g_log.error("Unable to successfully run FindPeaks sub-algorithm");

  return findpeaks->getProperty("PeaksList");
}

/** If a peak was successfully fitted, it is subtracted from the data.
 *  THIS METHOD ASSUMES THAT THE FITTING WAS DONE TO A GAUSSIAN FUNCTION.
 *  Note that the error value is left unchanged.
 *  @param input     The input workspace
 *  @param peakslist The succesfully fitted peaks
 *  @return A workspace containing the peak-subtracted data
 */
API::MatrixWorkspace_sptr StripPeaks::removePeaks(API::MatrixWorkspace_const_sptr input, API::ITableWorkspace_sptr peakslist)
{
  g_log.information("Subtracting peaks");
  // Create an output workspace - same size as input one
  MatrixWorkspace_sptr outputWS = WorkspaceFactory::Instance().create(input);
  // Copy the data over from the input to the output workspace
  const int hists = input->getNumberHistograms();
  for (int k = 0; k < hists; ++k)
  {
    outputWS->dataX(k) = input->readX(k);
    outputWS->dataY(k) = input->readY(k);
    outputWS->dataE(k) = input->readE(k);
  }

  const bool isHistogramData = outputWS->isHistogramData();
  // Loop over the list of peaks
  for (int i = 0; i < peakslist->rowCount(); ++i)
  {
    g_log.debug() << "Subtracting peak from spectrum " << peakslist->getRef<int>("spectrum",i) << std::endl;
    // Get references to the data
    const std::vector<double> &X = outputWS->readX(peakslist->getRef<int>("spectrum",i));
    std::vector<double> &Y = outputWS->dataY(peakslist->getRef<int>("spectrum",i));
    // Get back the gaussian parameters
    const double height = peakslist->getRef<double>("height",i);
    const double centre = peakslist->getRef<double>("centre",i);
    const double width = peakslist->getRef<double>("width",i);
    // Loop over the spectrum elements
    const int spectrumLength = Y.size();
    for (int j = 0; j < spectrumLength; ++j)
    {
      // If this is histogram data, we want to use the bin's central value
      double x = (isHistogramData ? 0.5*(X[j]+X[j+1]) : x = X[j]);
      // Skip if not anywhere near this peak
      if ( x < centre-3.0*width ) continue;
      if ( x > centre+3.0*width ) break;
      // Calculate the value of the Gaussian function at this point
      const double funcVal = height*exp(-0.5*pow((x-centre)/width,2));
      // Subtract the calculated value from the data
      Y[j] -= funcVal;
    }
  }

  return outputWS;
}

} // namespace Algorithms
} // namespace Mantid
