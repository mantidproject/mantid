/*WIKI* 


This algorithm is intended to automatically find all the peaks in a dataset and subtract them, leaving just the residual 'background'. 

====ChildAlgorithms used====
The [[FindPeaks]] algorithm is used to identify the peaks in the data.


*WIKI*/
#include "MantidAlgorithms/StripPeaks.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/PhysicalConstants.h"
#include "MantidAPI/TableRow.h"
#include "MantidKernel/BoundedValidator.h"
#include "MantidKernel/ListValidator.h"

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
    "The name of the input workspace." );
  declareProperty(new WorkspaceProperty<>("OutputWorkspace","",Direction::Output),
    "The name to use for the output workspace." );

  auto min = boost::make_shared<BoundedValidator<int> >();
  min->setLower(1);
  // The estimated width of a peak in terms of number of channels
  declareProperty("FWHM", 7, min,
    "The number of points covered, on average, by the fwhm of a peak (default 7).\nPassed through to [[FindPeaks]]." );
  // The tolerance allowed in meeting the conditions
  declareProperty("Tolerance",4,min,
    "A measure of the strictness desired in meeting the condition on peak candidates,\n"
    "Mariscotti recommends 2 (default 4)");
  
  declareProperty(new ArrayProperty<double>("PeakPositions"),
    "Optional: enter a comma-separated list of the expected X-position of the centre of the peaks. Only peaks near these positions will be fitted." );

  declareProperty("PeakPositionTolerance", 0.01,
      "Tolerance on the found peaks' positions against the input peak positions. Non-positive value indicates that this option is turned off.");

  std::vector<std::string> bkgdtypes;
  bkgdtypes.push_back("Linear");
  bkgdtypes.push_back("Quadratic");
  declareProperty("BackgroundType", "Linear", boost::make_shared<StringListValidator>(bkgdtypes),
      "Type of Background. Present choices include 'Linear' and 'Quadratic'");

  declareProperty("HighBackground", true,
      "Flag to indicate that the peaks are relatively weak comparing to background.");

  auto mustBePositive = boost::make_shared<BoundedValidator<int> >();
  mustBePositive->setLower(0);
  declareProperty("WorkspaceIndex",EMPTY_INT(),mustBePositive,
    "If set, will remove peaks only in the given spectrum of the workspace. Otherwise, all spectra will be searched.");

  auto mustBePositiveDbl = boost::make_shared<BoundedValidator<double> >();
  mustBePositiveDbl->setLower(0.);
  declareProperty("MaximumChisq",100.,mustBePositiveDbl,
    "The maximum chisq value for fits to remove the peak. Default 100.");
}

void StripPeaks::exec()
{
  // Retrieve the input workspace
  MatrixWorkspace_sptr inputWS = getProperty("InputWorkspace");
  m_maxChiSq = getProperty("MaximumChisq");

  // Call FindPeaks as a Child Algorithm
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

/** Calls FindPeaks as a Child Algorithm.
 *  @param WS :: The workspace to search
 *  @return list of found peaks
 */
API::ITableWorkspace_sptr StripPeaks::findPeaks(API::MatrixWorkspace_sptr WS)
{
  g_log.debug("Calling FindPeaks as a Child Algorithm");

  bool showlog = true;
  API::IAlgorithm_sptr findpeaks = createChildAlgorithm("FindPeaks",0.0, 0.2, showlog);
  findpeaks->setProperty("InputWorkspace", WS);
  findpeaks->setProperty<int>("FWHM",getProperty("FWHM"));
  findpeaks->setProperty<int>("Tolerance",getProperty("Tolerance"));
  // FindPeaks will do the checking on the validity of WorkspaceIndex
  findpeaks->setProperty<int>("WorkspaceIndex",getProperty("WorkspaceIndex"));

  //Get the specified peak positions, which is optional
  findpeaks->setProperty<std::vector<double> >("PeakPositions", getProperty("PeakPositions"));
  findpeaks->setProperty<std::string>("BackgroundType", getProperty("BackgroundType"));
  findpeaks->setProperty<bool>("HighBackground", getProperty("HighBackground"));
  findpeaks->setProperty<double>("PeakPositionTolerance", getProperty("PeakPositionTolerance"));
  findpeaks->setProperty<bool>("RawPeakParameters", true);

  int fwhm = getProperty("FWHM");
  int tolerance = getProperty("Tolerance");
  std::string backgroundtype = getProperty("BackgroundType");
  bool highbackground = getProperty("HighBackground");
  std::vector<double> peakpositions = getProperty("PeakPositions");
  g_log.debug() << "StripPeaks() calls FindPeaks(): FWHM            = " << fwhm << std::endl;
  g_log.debug() << "StripPeaks() calls FindPeaks(): Tolerance       = " << tolerance << std::endl;
  g_log.debug() << "StripPeaks() calls FindPeaks(): HighBackground  = " << highbackground<< std::endl;
  g_log.debug() << "StripPeaks() calls FindPeaks(): BackgroundType  = " << backgroundtype << std::endl;
  g_log.debug() << "StripPeaks() calls FindPeaks(): Peak positions: " << std::endl;
  for (size_t i = 0; i < peakpositions.size(); ++i)
      g_log.debug() << peakpositions[i] << std::endl;

  findpeaks->executeAsChildAlg();
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
  g_log.debug("Subtracting peaks");
  // Create an output workspace - same size as input one
  MatrixWorkspace_sptr outputWS = WorkspaceFactory::Instance().create(input);
  // Copy the data over from the input to the output workspace
  const size_t hists = input->getNumberHistograms();
  //progress 0.2 to 0.3 in this loop
  double prg=0.2;
  for (size_t k = 0; k < hists; ++k)
  {
    outputWS->dataX(k) = input->readX(k);
    outputWS->dataY(k) = input->readY(k);
    outputWS->dataE(k) = input->readE(k);
    prg+=(0.1/static_cast<double>(hists));
    progress(prg);
  }

  const bool isHistogramData = outputWS->isHistogramData();
  //progress from 0.3 to 1.0 here 
  prg=0.3;
  // Loop over the list of peaks
  for (size_t i = 0; i < peakslist->rowCount(); ++i)
  {
    // Get references to the data
    const MantidVec &X = outputWS->readX(peakslist->getRef<int>("spectrum",i));
    MantidVec &Y = outputWS->dataY(peakslist->getRef<int>("spectrum",i));
    // Get back the gaussian parameters
    const double height = peakslist->getRef<double>("f0.Height",i);
    const double centre = peakslist->getRef<double>("f0.PeakCentre",i);
    const double width = peakslist->getRef<double>("f0.Sigma",i);
    const double chisq = peakslist->getRef<double>("chi2", i);
    // These are some heuristic rules to discard bad fits.
    // Hope to be able to remove them when we have better fitting routine
    if ( height < 0 ) {
      g_log.error() << "Find Peak with Negative Height" << std::endl;
      continue;              // Height must be positive
    }
    if ( chisq > m_maxChiSq)
    {
      if (chisq != 1.e10)
        g_log.error() << "StripPeaks():  Peak Index = " << i << " @ " << centre 
           << "  Error: Peak fit with too high of chisq " << chisq << " > " << m_maxChiSq << "\n";
      continue;
    }

    g_log.debug() << "Subtracting peak " << i << " from spectrum " << peakslist->getRef<int>("spectrum",i)
                  << " at x = " << centre << " h = " << height << " s = " << width << " chi2 = " << chisq << "\n";

    // Loop over the spectrum elements
    const int spectrumLength = static_cast<int>(Y.size());
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
    prg+=(0.7/static_cast<double>(peakslist->rowCount()));
    progress(prg);
  }

  return outputWS;
}

} // namespace Algorithms
} // namespace Mantid
