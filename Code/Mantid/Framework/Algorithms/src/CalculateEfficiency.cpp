//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAlgorithms/CalculateEfficiency.h"
#include "MantidAPI/WorkspaceValidators.h"
#include "MantidAPI/SpectraDetectorMap.h"
#include <vector>

namespace Mantid
{
namespace Algorithms
{

// Register the class into the algorithm factory
DECLARE_ALGORITHM(CalculateEfficiency)

/// Sets documentation strings for this algorithm
void CalculateEfficiency::initDocs()
{
  this->setWikiSummary("Calculates the detector efficiency for a SANS instrument. ");
  this->setOptionalMessage("Calculates the detector efficiency for a SANS instrument.");
}


using namespace Kernel;
using namespace API;
using namespace Geometry;

/** Initialization method.
 *
 */
void CalculateEfficiency::init()
{
  declareProperty(
    new WorkspaceProperty<>("InputWorkspace","",Direction::Input, new CommonBinsValidator<>),
                            "The workspace containing the flood data" );
  declareProperty(
    new WorkspaceProperty<>("OutputWorkspace","",Direction::Output),
    "The name of the workspace to be created as the output of the algorithm" );

  BoundedValidator<double> *positiveDouble = new BoundedValidator<double>();
  positiveDouble->setLower(0);
  declareProperty("MinEfficiency", EMPTY_DBL(), positiveDouble,
      "Minimum efficiency for a pixel to be considered (default: no minimum).");
  declareProperty("MaxEfficiency", EMPTY_DBL(), positiveDouble->clone(),
      "Maximum efficiency for a pixel to be considered (default: no maximum).");

}

/** Executes the algorithm
 *
 */
void CalculateEfficiency::exec()
{

  // Minimum efficiency. Pixels with lower efficiency will be masked
  double min_eff = getProperty("MinEfficiency");
  // Maximum efficiency. Pixels with higher efficiency will be masked
  double max_eff = getProperty("MaxEfficiency");

  // Get the input workspace
  MatrixWorkspace_sptr inputWS = getProperty("InputWorkspace");

  // Now create the output workspace
  MatrixWorkspace_sptr outputWS = getProperty("OutputWorkspace");

  Progress progress(this,0.0,1.0,5);

  // Sum up all the wavelength bins
  IAlgorithm_sptr childAlg = createSubAlgorithm("Integration");
  childAlg->setProperty<MatrixWorkspace_sptr>("InputWorkspace", inputWS);
  childAlg->executeAsSubAlg();

  MatrixWorkspace_sptr rebinnedWS = childAlg->getProperty("OutputWorkspace");

  outputWS = WorkspaceFactory::Instance().create(rebinnedWS);
  for (int i=0; i<(int)rebinnedWS->getNumberHistograms(); i++)
  {
    outputWS->dataX(i) = rebinnedWS->readX(i);
  }
  setProperty("OutputWorkspace",outputWS);

  progress.report();

  double sum = 0.0;
  double err = 0.0;
  int npixels = 0;

  // Loop over spectra and sum all the counts to get normalization
  // Skip monitors and masked detectors
  sumUnmaskedDetectors(rebinnedWS, sum, err, npixels);

  progress.report();

  // Normalize each detector pixel by the sum we just found to get the
  // relative efficiency. If the minimum and maximum efficiencies are
  // provided, the pixels falling outside this range will be marked
  // as 'masked' in both the input and output workspace.
  // We mask detectors in the input workspace so that we can resum the
  // counts to find a new normalization factor that takes into account
  // the newly masked detectors.
  normalizeDetectors(rebinnedWS, outputWS, sum, err, npixels, min_eff, max_eff);

  progress.report();

  if ( !isEmpty(min_eff) || !isEmpty(max_eff) )
  {
    // Recompute the normalization, excluding the pixels that were outside
    // the acceptable efficiency range.
    sumUnmaskedDetectors(rebinnedWS, sum, err, npixels);

    progress.report();

    // Now that we have a normalization factor that excludes bad pixels,
    // recompute the relative efficiency.
    // We pass EMPTY_DBL() to avoid masking pixels that might end up high or low
    // after the new normalization.
    normalizeDetectors(rebinnedWS, outputWS, sum, err, npixels, EMPTY_DBL(), EMPTY_DBL());
  }

  return;
}

/*
 *  Sum up all the unmasked detector pixels.
 *
 * @param rebinnedWS: workspace where all the wavelength bins have been grouped together
 * @param sum: sum of all the unmasked detector pixels (counts)
 * @param error: error on sum (counts)
 * @param nPixels: number of unmasked detector pixels that contributed to sum
 */
void CalculateEfficiency::sumUnmaskedDetectors(MatrixWorkspace_sptr rebinnedWS,
    double& sum, double& error, int& nPixels)
{
  // Number of spectra
    const size_t numberOfSpectra = rebinnedWS->getNumberHistograms();
    sum = 0.0;
    error = 0.0;
    nPixels = 0;

    for (int i = 0; i < numberOfSpectra; i++)
    {
      // Get the detector object for this spectrum
      IDetector_const_sptr det = rebinnedWS->getDetector(i);
      // If this detector is masked, skip to the next one
      if ( det->isMasked() ) continue;
      // If this detector is a monitor, skip to the next one
      if ( det->isMonitor() ) continue;

      // Retrieve the spectrum into a vector
      const MantidVec& YValues = rebinnedWS->readY(i);
      const MantidVec& YErrors = rebinnedWS->readE(i);

      sum += YValues[0];
      error += YErrors[0]*YErrors[0];
      nPixels++;
    }

    error = std::sqrt(error);
}

/*
 * Normalize each detector to produce the relative detector efficiency.
 * Pixels that fall outside those efficiency limits will be masked in both
 * the input and output workspaces.
 *
 * @param rebinnedWS: input workspace
 * @param outputWS: output workspace containing the relative efficiency
 * @param sum: sum of all the unmasked detector pixels (counts)
 * @param error: error on sum (counts)
 * @param nPixels: number of unmasked detector pixels that contributed to sum
 */

void CalculateEfficiency::normalizeDetectors(MatrixWorkspace_sptr rebinnedWS,
    MatrixWorkspace_sptr outputWS, double sum, double error, int nPixels,
    double min_eff, double max_eff)
{
    // Number of spectra
    const size_t numberOfSpectra = rebinnedWS->getNumberHistograms();

    // Empty vector to store the pixels that outside the acceptable efficiency range
    std::vector<int> dets_to_mask;

    for (int i = 0; i < numberOfSpectra; i++)
    {
      // Get the detector object for this spectrum
      IDetector_const_sptr det = rebinnedWS->getDetector(i);
      // If this detector is masked, skip to the next one
      if ( det->isMasked() ) continue;
      // If this detector is a monitor, skip to the next one
      if ( det->isMonitor() ) continue;

      // Retrieve the spectrum into a vector
      const MantidVec& YIn = rebinnedWS->readY(i);
      const MantidVec& EIn = rebinnedWS->readE(i);
      MantidVec& YOut = outputWS->dataY(i);
      MantidVec& EOut = outputWS->dataE(i);

      // Normalize counts to get relative efficiency
      YOut[0] = nPixels/sum * YIn[0];
      const double err_sum = YIn[0]/sum*error;
      EOut[0] = nPixels/std::abs(sum) * std::sqrt(EIn[0]*EIn[0] + err_sum*err_sum);

      // Mask this detector if the signal is outside the acceptable band
      if ( !isEmpty(min_eff) && YOut[0] < min_eff ) dets_to_mask.push_back(i);
      if ( !isEmpty(max_eff) && YOut[0] > max_eff ) dets_to_mask.push_back(i);

    }

    // If we identified pixels to be masked, mask them now
    if ( dets_to_mask.size()>0 )
    {
      // Mask detectors that were found to be outside the acceptable efficiency band
      IAlgorithm_sptr mask = createSubAlgorithm("MaskDetectors");
      try
      {
        // First we mask detectors in the output workspace
        mask->setProperty<MatrixWorkspace_sptr>("Workspace", outputWS);
        mask->setProperty< std::vector<specid_t> >("SpectraList", dets_to_mask);
        mask->execute();

        // Then we mask the same detectors in the input workspace
        mask->setProperty<MatrixWorkspace_sptr>("Workspace", rebinnedWS);
        mask->setProperty< std::vector<specid_t> >("SpectraList", dets_to_mask);
        mask->execute();
      } catch (std::invalid_argument& err)
      {
        std::stringstream e;
        e << "Invalid argument to MaskDetectors sub-algorithm: " << err.what();
        g_log.error(e.str());
      } catch (std::runtime_error& err)
      {
        std::stringstream e;
        e << "Unable to successfully run MaskDetectors sub-algorithm: " << err.what();
        g_log.error(e.str());
      }
    }
}

} // namespace Algorithms
} // namespace Mantid
