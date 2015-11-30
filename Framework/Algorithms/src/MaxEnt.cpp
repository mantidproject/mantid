#include "MantidAlgorithms/MaxEnt.h"

#include "MantidKernel/BoundedValidator.h"

#include <boost/shared_array.hpp>
#include <gsl/gsl_fft_complex.h>

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

    auto data = inWS->readY(s);
    auto error = inWS->readE(s);

    // To record the algorithm's progress
    std::vector<double> evolChi(niter, 0.);
    std::vector<double> evolTest(niter, 0.);

    // Store image in successive iterations
    std::vector<double> newImage = image;

    // Progress
    Progress progress(this, 0, 1, niter);

    // Run maxent algorithm
    for (size_t it = 0; it < niter; it++) {

      // Calculate search directions and quadratic model coefficients (SB eq. 21
      // and 24)
      SearchDirections dirs =
          calculateSearchDirections(data, error, newImage, background);

      // Calculate beta to contruct new image (SB eq. 25)

      // Apply distance penalty (SB eq. 33)

      // Calculate the new image

      // Calculate the new Chi-square

      // Record the evolution of Chi-square and angle(S,C)

      // Stop condition

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

//----------------------------------------------------------------------------------------------

/**
* Transforms from solution-space to data-space
* @param input :: [input] An input vector in image space
* @return :: The input vector transformed to data space
*/
std::vector<double> MaxEnt::opus(const std::vector<double> &input) {

  /* Performs backward Fourier transform */

  size_t n = input.size();

  /* Prepare the data */
  boost::shared_array<double> result(new double[2 * n]);
  for (size_t i = 0; i < n; i++) {
    result[2 * i] = input[i]; // Real part
    result[2 * i + 1] = 0.;   // Imaginary part
  }

  /* Backward FT */
  gsl_fft_complex_wavetable *wavetable = gsl_fft_complex_wavetable_alloc(n);
  gsl_fft_complex_workspace *workspace = gsl_fft_complex_workspace_alloc(n);
  gsl_fft_complex_inverse(result.get(), 1, n, wavetable, workspace);
  gsl_fft_complex_wavetable_free(wavetable);
  gsl_fft_complex_workspace_free(workspace);

  std::vector<double> output(n);
  for (size_t i = 0; i < n; i++) {
    output[i] = result[2 * i];
  }

  return output;
}

//----------------------------------------------------------------------------------------------

/**
* Transforms from data-space to solution-space
* @param input :: [input] An input vector in data space
* @return :: The input vector converted to image space
*/
std::vector<double> MaxEnt::tropus(const std::vector<double> &input) {

  /* Performs forward Fourier transform */

  size_t n = input.size();

  /* Prepare the data */
  boost::shared_array<double> result(new double[n * 2]);
  for (size_t i = 0; i < n; i++) {
    result[2 * i] = input[i]; // even indexes filled with the real part
    result[2 * i + 1] = 0.;   // odd indexes filled with the imaginary part
  }

  /*  Fourier transofrm */
  gsl_fft_complex_wavetable *wavetable = gsl_fft_complex_wavetable_alloc(n);
  gsl_fft_complex_workspace *workspace = gsl_fft_complex_workspace_alloc(n);
  gsl_fft_complex_forward(result.get(), 1, n, wavetable, workspace);
  gsl_fft_complex_wavetable_free(wavetable);
  gsl_fft_complex_workspace_free(workspace);

  /* Get the data */
  std::vector<double> output(n);
  for (size_t i = 0; i < n; i++) {
    output[i] = result[2 * i];
  }

  return output;
}

/** Calculate the search directions and quadratic coefficients as described in
* section 3.6 in SB
* @param data :: [input] The experimental input data
* @param error :: [input] The experimental input errors
* @param image :: [input] The current image
* @param background :: [input] The default or 'sky' background
* @return :: Search directions as a SearchDirections object
*/
SearchDirections MaxEnt::calculateSearchDirections(
    const std::vector<double> &data, const std::vector<double> &error,
    const std::vector<double> &image, double background) {

  // Two search directions
  const size_t dim = 2;

  // Some checks
  if ((data.size() != error.size()) || (data.size() != image.size())) {

    throw std::invalid_argument("Can't compute quadratic coefficients");
  }

  size_t npoints = data.size();

  // Calculate data from start image
  std::vector<double> dataC = opus(image);
  // Calculate Chi-square
  double chiSq = getChiSq(data, error, dataC);

  // Gradient of C (Chi)
  std::vector<double> cgrad = getCGrad(data, error, dataC);
  cgrad = tropus(cgrad);
  // Gradient of S (Entropy)
  std::vector<double> sgrad = getSGrad(image, background);

  SearchDirections dirs(dim, npoints);

  dirs.chisq = chiSq;

  double cnorm = 0.;
  double snorm = 0.;
  double csnorm = 0.;

  // Here we calculate:
  // SB. eq 22 -> |grad S|, |grad C|
  // SB. eq 37 -> test
  for (size_t i = 0; i < npoints; i++) {
    cnorm += cgrad[i] * cgrad[i] * image[i] * image[i];
    snorm += sgrad[i] * sgrad[i] * image[i] * image[i];
    csnorm += cgrad[i] * sgrad[i] * image[i] * image[i];
  }
  cnorm = sqrt(cnorm);
  snorm = sqrt(snorm);

  dirs.angle = sqrt(0.5 * (1. - csnorm / snorm / cnorm));
  // csnorm could be greater than snorm * cnorm due to rounding issues
  // so check for nan
  if (dirs.angle != dirs.angle)
    dirs.angle = 0.;

  // Calculate the search directions

  // Temporary vectors (image space)
  std::vector<double> xi0(npoints, 0.);
  std::vector<double> xi1(npoints, 0.);

  for (size_t i = 0; i < npoints; i++) {
    xi0[i] = fabs(image[i]) * cgrad[i] / cnorm;
    xi1[i] = fabs(image[i]) * sgrad[i] / snorm;
    // xi1[i] = image[i] * (sgrad[i] / snorm - cgrad[i] / cnorm);
  }

  // Temporary vectors (data space)
  std::vector<double> eta0 = opus(xi0);
  std::vector<double> eta1 = opus(xi1);

  // Store the search directions
  dirs.xi.setRow(0, xi0);
  dirs.xi.setRow(1, xi1);
  dirs.eta.setRow(0, eta0);
  dirs.eta.setRow(1, eta1);

  // Now compute the quadratic coefficients SB. eq 24

  // First compute s1, c1
  for (size_t k = 0; k < dim; k++) {
    dirs.c1[k][0] = dirs.s1[k][0] = 0.;
    for (size_t i = 0; i < npoints; i++) {
      dirs.s1[k][0] += dirs.xi[k][i] * sgrad[i];
      dirs.c1[k][0] += dirs.xi[k][i] * cgrad[i];
    }
    // Note: the factor chiSQ has to go either here or in ChiNow
    dirs.c1[k][0] /= chiSq;
  }

  // Then s2, c2
  for (size_t k = 0; k < dim; k++) {
    for (size_t l = 0; l < k + 1; l++) {
      dirs.s2[k][l] = 0.;
      dirs.c2[k][l] = 0.;
      for (size_t i = 0; i < npoints; i++) {
        dirs.c2[k][l] += dirs.eta[k][i] * dirs.eta[l][i] / error[i] / error[i];
        dirs.s2[k][l] -= dirs.xi[k][i] * dirs.xi[l][i] / fabs(image[i]);
      }
      // Note: the factor chiSQ has to go either here or in ChiNow
      dirs.c2[k][l] *= 2.0 / chiSq;
      dirs.s2[k][l] *= 1.0 / background;
    }
  }
  // Symmetrise s2, c2: reflect accross the diagonal
  for (size_t k = 0; k < dim; k++) {
    for (size_t l = k + 1; l < dim; l++) {
      dirs.s2[k][l] = dirs.s2[l][k];
      dirs.c2[k][l] = dirs.c2[l][k];
    }
  }

  return dirs;
}

/** Calculates Chi-square
* @param data :: [input] Data measured during the experiment
* @param errors :: [input] Associated errors
* @param dataCalc :: [input] Data calculated from image
* @return :: The calculated Chi-square
*/
double MaxEnt::getChiSq(const std::vector<double> &data,
                        const std::vector<double> &errors,
                        const std::vector<double> &dataCalc) {

  if ((data.size() != errors.size()) || (data.size() != dataCalc.size())) {
    throw std::invalid_argument("Cannot compute Chi square");
  }

  size_t npoints = data.size();

  // Calculate
  // ChiSq = sum_i [ data_i - dataCalc_i ]^2 / [ error_i ]^2
  double chiSq = 0;
  for (size_t i = 0; i < npoints; i++) {
    if (errors[i]) {
      double term = (data[i] - dataCalc[i]) / errors[i];
      chiSq += term * term;
    }
  }

  return chiSq;
}
/** Calculates the gradient of C (Chi term)
* @param data :: [input] Data measured during the experiment
* @param errors :: [input] Associated errors
* @param dataCalc :: [input] Data calculated from image
* @return :: The calculated gradient of C
*/
std::vector<double> MaxEnt::getCGrad(const std::vector<double> &data,
                                     const std::vector<double> &errors,
                                     const std::vector<double> &dataCalc) {

  if ((data.size() != errors.size()) || (data.size() != dataCalc.size())) {
    throw std::invalid_argument("Cannot compute gradient of Chi");
  }

  size_t npoints = data.size();

  // Calculate gradient of Chi
  // CGrad_i = -2 * [ data_i - dataCalc_i ] / [ error_i ]^2
  std::vector<double> cgrad(npoints, 0.);
  for (size_t i = 0; i < npoints; i++) {
    if (errors[i])
      cgrad[i] = -2. * (data[i] - dataCalc[i]) / errors[i] / errors[i];
  }

  return cgrad;
}

/** Calculates the gradient of S (Entropy)
* @param image :: [input] The current image
* @param background :: [input] The background
* @return :: The calculated gradient of S
*/
std::vector<double> MaxEnt::getSGrad(const std::vector<double> &image,
                                     double background) {

#define S(x) (-log(x + std::sqrt(x * x + 1)))
  //#define S(x) (-log(x))

  size_t npoints = image.size();

  // Calculate gradient of S
  std::vector<double> sgrad(npoints, 0.);
  for (size_t i = 0; i < npoints; i++) {
    sgrad[i] = S(image[i] / background);
  }

  return sgrad;

#undef S
}

} // namespace Algorithms
} // namespace Mantid
