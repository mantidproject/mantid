#include "MantidAlgorithms/StripPeaks.h"
#include "MantidKernel/PhysicalConstants.h"
#include "MantidAPI/TableRow.h"

namespace Mantid
{
namespace Algorithms
{

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(StripPeaks)

/// Sets documentation strings for this algorithm
void StripPeaks::initDocs()
{
  this->setWikiSummary("This algorithm attempts to find all the peaks in all spectra of a workspace and subtract them from the data, leaving just the 'background'. ");
  this->setOptionalMessage("This algorithm attempts to find all the peaks in all spectra of a workspace and subtract them from the data, leaving just the 'background'.");
}


using namespace Kernel;
using namespace API;

StripPeaks::StripPeaks() : API::Algorithm() {}

void StripPeaks::init()
{
  declareProperty(
    new WorkspaceProperty<>("InputWorkspace","",Direction::Input),
    "Name of the input workspace" );
  declareProperty(new WorkspaceProperty<>("OutputWorkspace","",Direction::Output),
    "The name of the workspace to be created as the output of the algorithm" );

  BoundedValidator<int> *min = new BoundedValidator<int>();
  min->setLower(1);
  // The estimated width of a peak in terms of number of channels
  declareProperty("FWHM", 7, min,
    "Estimated number of points covered by the fwhm of a peak (default 7)" );
  // The tolerance allowed in meeting the conditions
  declareProperty("Tolerance",4,min->clone(),
    "A measure of the strictness desired in meeting the condition on peak candidates,\n"
    "Mariscotti recommends 2 (default 4)");
  
  declareProperty("PeakPositions", "",
    "Optional: enter a comma-separated list of the expected X-position of the centre of the peaks. Only peaks near these positions will be fitted." );

  BoundedValidator<int> *mustBePositive = new BoundedValidator<int>();
  mustBePositive->setLower(0);
  declareProperty("WorkspaceIndex",EMPTY_INT(),mustBePositive,
    "If set, peaks will only be removed from this spectrum (otherwise from all)");   
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
 *  @param WS :: The workspace to search
 *  @return list of found peaks
 */
API::ITableWorkspace_sptr StripPeaks::findPeaks(API::MatrixWorkspace_sptr WS)
{
  g_log.information("Calling FindPeaks as a sub-algorithm");

  API::IAlgorithm_sptr findpeaks = createSubAlgorithm("FindPeaks",0.0,0.2);
  findpeaks->setProperty("InputWorkspace", WS);
  findpeaks->setProperty<int>("FWHM",getProperty("FWHM"));
  findpeaks->setProperty<int>("Tolerance",getProperty("Tolerance"));
  // FindPeaks will do the checking on the validity of WorkspaceIndex
  findpeaks->setProperty<int>("WorkspaceIndex",getProperty("WorkspaceIndex"));

  //Get the specified peak positions, which is optional
  findpeaks->setProperty<std::string>("PeakPositions", getProperty("PeakPositions"));
  findpeaks->executeAsSubAlg();
  return findpeaks->getProperty("PeaksList");
}

/** If a peak was successfully fitted, it is subtracted from the data.
 *  THIS METHOD ASSUMES THAT THE FITTING WAS DONE TO A GAUSSIAN FUNCTION.
 *  Note that the error value is left unchanged.
 *  @param input ::     The input workspace
 *  @param peakslist :: The succesfully fitted peaks
 *  @return A workspace containing the peak-subtracted data
 */
API::MatrixWorkspace_sptr StripPeaks::removePeaks(API::MatrixWorkspace_const_sptr input, API::ITableWorkspace_sptr peakslist)
{
  g_log.information("Subtracting peaks");
  // Create an output workspace - same size as input one
  MatrixWorkspace_sptr outputWS = WorkspaceFactory::Instance().create(input);
  // Copy the data over from the input to the output workspace
  const size_t hists = input->getNumberHistograms();
  //progress 0.2 to 0.3 in this loop
  double prg=0.2;
  for (int k = 0; k < hists; ++k)
  {
    outputWS->dataX(k) = input->readX(k);
    outputWS->dataY(k) = input->readY(k);
    outputWS->dataE(k) = input->readE(k);
    prg+=(0.1/hists);
    progress(prg);
  }

  const bool isHistogramData = outputWS->isHistogramData();
  //progress from 0.3 to 1.0 here 
  prg=0.3;
  // Loop over the list of peaks
  for (int i = 0; i < peakslist->rowCount(); ++i)
  {
    // Get references to the data
    const MantidVec &X = outputWS->readX(peakslist->getRef<int>("spectrum",i));
    MantidVec &Y = outputWS->dataY(peakslist->getRef<int>("spectrum",i));
    // Get back the gaussian parameters
    const double height = peakslist->getRef<double>("height",i);
    const double centre = peakslist->getRef<double>("centre",i);
    const double width = peakslist->getRef<double>("width",i);
    // These are some heuristic rules to discard bad fits.
    // Hope to be able to remove them when we have better fitting routine
    if ( height < 0 ) continue;              // Height must be positive

    g_log.debug() << "Subtracting peak from spectrum " << peakslist->getRef<int>("spectrum",i) 
                  << " at x = " << centre << "\n";
    // Loop over the spectrum elements
    const int spectrumLength = Y.size();
    for (int j = 0; j < spectrumLength; ++j)
    {
      // If this is histogram data, we want to use the bin's central value
      double x = (isHistogramData ? 0.5*(X[j]+X[j+1]) : X[j]);
      // Skip if not anywhere near this peak
      if ( x < centre-3.0*width ) continue;
      if ( x > centre+3.0*width ) break;
      // Calculate the value of the Gaussian function at this point
      const double funcVal = height*exp(-0.5*pow((x-centre)/width,2));
      // Subtract the calculated value from the data
      Y[j] -= funcVal;
    }
    prg+=(0.7/peakslist->rowCount());
    progress(prg);
  }

  return outputWS;
}

} // namespace Algorithms
} // namespace Mantid
