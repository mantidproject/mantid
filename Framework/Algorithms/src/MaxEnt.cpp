#include "MantidAlgorithms/MaxEnt.h"

#include "MantidKernel/BoundedValidator.h"

namespace Mantid {
namespace Algorithms {

using Mantid::Kernel::Direction;
using Mantid::API::WorkspaceProperty;

using namespace API;
using namespace Kernel;

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(MaxEnt)

//----------------------------------------------------------------------------------------------
/** Constructor
 */
MaxEnt::MaxEnt() {}

//----------------------------------------------------------------------------------------------
/** Destructor
 */
MaxEnt::~MaxEnt() {}

//----------------------------------------------------------------------------------------------

/// Algorithm's name for identification. @see Algorithm::name
const std::string MaxEnt::name() const { return "MaxEnt"; }

/// Algorithm's version for identification. @see Algorithm::version
int MaxEnt::version() const { return 1; }

/// Algorithm's category for identification. @see Algorithm::category
const std::string MaxEnt::category() const { return "FFT"; }

/// Algorithm's summary for use in the GUI and help. @see Algorithm::summary
const std::string MaxEnt::summary() const {
  return "Runs Maximum Entropy method on an input workspace";
}

//----------------------------------------------------------------------------------------------
/** Initialize the algorithm's properties.
 */
void MaxEnt::init() {

  declareProperty(
      new WorkspaceProperty<>("InputWorkspace", "", Direction::Input),
      "An input workspace.");

  auto mustBeNonNegative = boost::make_shared<BoundedValidator<double>>();
  mustBeNonNegative->setLower(1E-12);
  declareProperty(new PropertyWithValue<double>(
                      "Background", 0.4, mustBeNonNegative, Direction::Input),
                  "Default level above which the image is significant");

  declareProperty(new PropertyWithValue<double>(
                      "ChiTarget", 100.0, mustBeNonNegative, Direction::Input),
                  "Target value of Chi-square");

  declareProperty(new PropertyWithValue<double>(
                      "ChiEps", 0.001, mustBeNonNegative, Direction::Input),
                  "Required precision for Chi-square");

  declareProperty(new PropertyWithValue<double>("DistancePenalty", 0.1,
                                                mustBeNonNegative,
                                                Direction::Input),
                  "Distance penalty applied to the current image");

  declareProperty(new PropertyWithValue<double>(
                      "MaxAngle", 0.05, mustBeNonNegative, Direction::Input),
                  "Maximum degree of non-parallelism between S and C");

  auto mustBePositive = boost::make_shared<BoundedValidator<size_t>>();
  mustBePositive->setLower(0);
  declareProperty(new PropertyWithValue<size_t>(
                      "MaxIterations", 20000, mustBePositive, Direction::Input),
                  "Maximum number of iterations");

  declareProperty(new PropertyWithValue<size_t>("AlphaChopIterations", 500,
                                                mustBePositive,
                                                Direction::Input),
                  "Maximum number of iterations in alpha chop");

  declareProperty(new WorkspaceProperty<>("EvolChi", "", Direction::Output),
                  "Output workspace containing the evolution of Chi-sq");
  declareProperty(new WorkspaceProperty<>("EvolAngle", "", Direction::Output),
                  "Output workspace containing the evolution of "
                  "non-paralellism between S and C");
  declareProperty(
      new WorkspaceProperty<>("ReconstructedImage", "", Direction::Output),
      "The output workspace containing the reconstructed image.");
  declareProperty(
      new WorkspaceProperty<>("ReconstructedData", "", Direction::Output),
      "The output workspace containing the reconstructed data.");
}

//----------------------------------------------------------------------------------------------
/** Validate the input properties.
*/
std::map<std::string, std::string> MaxEnt::validateInputs() {

  std::map<std::string, std::string> result;

  // X values in input workspace must be equally spaced
  MatrixWorkspace_sptr inWS = getProperty("InputWorkspace");
  const MantidVec &X = inWS->readX(0);
  const double dx = X[1] - X[0];
  for (size_t i = 1; i < X.size() - 2; i++) {
    if (std::abs(dx - X[i + 1] + X[i]) / dx > 1e-7) {
      result["InputWorkspace"] =
          "X axis must be linear (all bins must have the same width)";
    }
  }

  return result;
}

//----------------------------------------------------------------------------------------------
/** Execute the algorithm.
 */
void MaxEnt::exec() {

  // Read input workspace
  MatrixWorkspace_sptr inWS = getProperty("InputWorkspace");

  // Background (default level, sky background, etc)
  double background = getProperty("DefaultLevel");
  // Chi target
  double chiTarget = getProperty("ChiTarget");
  // Required precision for Chi arget
  double chiEps = getProperty("ChiEps");
  // Maximum degree of non-parallelism between S and C
  double angle = getProperty("MaxAngle");
  // Distance penalty for current image
  double distEps = getProperty("DistancePenalty");
  // Maximum number of iterations
  size_t niter = getProperty("MaxIterations");
  // Maximum number of iterations in alpha chop
  size_t alphaIter = getProperty("AlphaChopIterations");

  // Number of spectra
  size_t nspec = inWS->getNumberHistograms();
  // Number of data points
  size_t npoints = inWS->blocksize();

  // Create output workspaces
  MatrixWorkspace_sptr outImageWS = WorkspaceFactory::Instance().create(inWS);
  MatrixWorkspace_sptr outDataWS = WorkspaceFactory::Instance().create(inWS);
  // Create evol workspaces
  MatrixWorkspace_sptr outEvolChi =
      WorkspaceFactory::Instance().create(inWS, nspec, niter, niter);
  MatrixWorkspace_sptr outEvolTest =
      WorkspaceFactory::Instance().create(inWS, nspec, niter, niter);

  // Start distribution (flat background)
  std::vector<double> image(npoints, background * 1.01);

  for (size_t s = 0; s < nspec; s++) {

    // Progress
    Progress progress(this, 0, 1, niter);

    // Run maxent algorithm
    for (size_t it = 0; it < niter; it++) {

      // Check for canceling the algorithm
      if (!(it % 1000)) {
        interruption_point();
      }

      progress.report();

    } // iterations

    // Populate the output workspaces

    // Populate workspaces recording the evolution of Chi and Test

  } // Next spectrum

  setProperty("EvolChi", outEvolChi);
  setProperty("EvolAngle", outEvolTest);
  setProperty("ReconstructedImage", outImageWS);
  setProperty("ReconstructedData", outDataWS);
}

} // namespace Algorithms
} // namespace Mantid
