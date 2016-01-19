#include "MantidAlgorithms/MaxEnt.h"

#include "MantidKernel/BoundedValidator.h"

#include <boost/shared_array.hpp>
#include <gsl/gsl_fft_complex.h>
#include <gsl/gsl_linalg.h>

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
const std::string MaxEnt::category() const { return "Arithmetic\\FFT"; }

/// Algorithm's summary for use in the GUI and help. @see Algorithm::summary
const std::string MaxEnt::summary() const {
  return "Runs Maximum Entropy method on every spectrum of an input workspace. "
         "Note this algorithm is still in development, and its interface is "
         "likely to change. It currently works for the case where the "
         "number of data points equals the number of reconstructed (image) "
         "points and data and image are related by Fourier transform.";
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
  declareProperty(new PropertyWithValue<double>("A", 0.4, mustBeNonNegative,
                                                Direction::Input),
                  "A maximum entropy constant");

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
  double background = getProperty("A");
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
  // Number of X bins
  size_t npointsX = inWS->isHistogramData() ? npoints + 1 : npoints;
  // We need to handle complex data
  npoints *= 2;

  // Create output workspaces
  MatrixWorkspace_sptr outImageWS =
      WorkspaceFactory::Instance().create(inWS, 2 * nspec, npointsX, npoints);
  MatrixWorkspace_sptr outDataWS =
      WorkspaceFactory::Instance().create(inWS, nspec, npointsX, npoints);
  // Create evol workspaces
  MatrixWorkspace_sptr outEvolChi =
      WorkspaceFactory::Instance().create(inWS, nspec, niter, niter);
  MatrixWorkspace_sptr outEvolTest =
      WorkspaceFactory::Instance().create(inWS, nspec, niter, niter);

  // Start distribution (flat background)
  std::vector<double> image(npoints, background);

  for (size_t s = 0; s < nspec; s++) {

    // Read data from the input workspace
    // Only real part, complex part is zero
    std::vector<double> data(npoints, 0.);
    std::vector<double> error(npoints, 0.);
    for (size_t i = 0; i < npoints / 2; i++) {
      data[2 * i] = inWS->readY(s)[i];
      error[2 * i] = inWS->readE(s)[i];
    }

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
      auto beta = move(dirs, chiTarget / dirs.chisq, chiEps, alphaIter);

      // Apply distance penalty (SB eq. 33)
      double sum = 0.;
      for (size_t i = 0; i < image.size(); i++)
        sum += fabs(image[i]);

      double dist = distance(dirs.s2, beta);
      if (dist > distEps * sum / background) {
        for (size_t k = 0; k < beta.size(); k++) {
          beta[k] *= sqrt(sum / dist / background);
        }
      }

      // Calculate the new image
      for (size_t i = 0; i < npoints; i++) {
        for (size_t k = 0; k < beta.size(); k++) {
          newImage[i] = newImage[i] + beta[k] * dirs.xIm[k][i];
        }
      }

      // Calculate the new Chi-square
      auto dataC = transformImageToData(newImage);
      double chiSq = getChiSq(data, error, dataC);

      // Record the evolution of Chi-square and angle(S,C)
      evolChi[it] = chiSq;
      evolTest[it] = dirs.angle;

      // Stop condition, solution found
      if ((std::abs(chiSq / chiTarget - 1.) < chiEps) && (dirs.angle < angle)) {
        break;
      }

      // Check for canceling the algorithm
      if (!(it % 1000)) {
        interruption_point();
      }

      progress.report();

    } // iterations

    // Get calculated data
    std::vector<double> solutionData = transformImageToData(newImage);

    // Populate the output workspaces
    populateOutputWS(inWS, s, nspec, solutionData, newImage, outDataWS,
                     outImageWS);

    // Populate workspaces recording the evolution of Chi and Test
    // X values
    for (size_t it = 0; it < niter; it++) {
      outEvolChi->dataX(s)[it] = static_cast<double>(it);
      outEvolTest->dataX(s)[it] = static_cast<double>(it);
    }
    // Y values
    outEvolChi->dataY(s).assign(evolChi.begin(), evolChi.end());
    outEvolTest->dataY(s).assign(evolTest.begin(), evolTest.end());
    // No errors

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
std::vector<double>
MaxEnt::transformImageToData(const std::vector<double> &input) {

  /* Performs backward Fourier transform */

  size_t n = input.size();

  if (n % 2) {
    throw std::invalid_argument("Cannot transform to data space");
  }

  /* Prepare the data */
  boost::shared_array<double> result(new double[n]);
  for (size_t i = 0; i < n; i++) {
    result[i] = input[i];
  }

  /* Backward FT */
  gsl_fft_complex_wavetable *wavetable = gsl_fft_complex_wavetable_alloc(n / 2);
  gsl_fft_complex_workspace *workspace = gsl_fft_complex_workspace_alloc(n / 2);
  gsl_fft_complex_inverse(result.get(), 1, n / 2, wavetable, workspace);
  gsl_fft_complex_wavetable_free(wavetable);
  gsl_fft_complex_workspace_free(workspace);

  std::vector<double> output(n);
  for (size_t i = 0; i < n; i++) {
    output[i] = result[i];
  }

  return output;
}

/**
* Transforms from data-space to solution-space
* @param input :: [input] An input vector in data space
* @return :: The input vector converted to image space
*/
std::vector<double>
MaxEnt::transformDataToImage(const std::vector<double> &input) {

  /* Performs forward Fourier transform */

  size_t n = input.size();

  if (n % 2) {
    throw std::invalid_argument("Cannot transform to data space");
  }

  /* Prepare the data */
  boost::shared_array<double> result(new double[n]);
  for (size_t i = 0; i < n; i++) {
    result[i] = input[i];
  }

  /*  Fourier transofrm */
  gsl_fft_complex_wavetable *wavetable = gsl_fft_complex_wavetable_alloc(n / 2);
  gsl_fft_complex_workspace *workspace = gsl_fft_complex_workspace_alloc(n / 2);
  gsl_fft_complex_forward(result.get(), 1, n / 2, wavetable, workspace);
  gsl_fft_complex_wavetable_free(wavetable);
  gsl_fft_complex_workspace_free(workspace);

  /* Get the data */
  std::vector<double> output(n);
  for (size_t i = 0; i < n; i++) {
    output[i] = result[i];
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
  std::vector<double> dataC = transformImageToData(image);
  // Calculate Chi-square
  double chiSq = getChiSq(data, error, dataC);

  // Gradient of C (Chi)
  std::vector<double> cgrad = getCGrad(data, error, dataC);
  cgrad = transformDataToImage(cgrad);
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
  std::vector<double> xIm0(npoints, 0.);
  std::vector<double> xIm1(npoints, 0.);

  for (size_t i = 0; i < npoints; i++) {
    xIm0[i] = fabs(image[i]) * cgrad[i] / cnorm;
    xIm1[i] = fabs(image[i]) * sgrad[i] / snorm;
    // xi1[i] = image[i] * (sgrad[i] / snorm - cgrad[i] / cnorm);
  }

  // Temporary vectors (data space)
  std::vector<double> xDat0 = transformImageToData(xIm0);
  std::vector<double> xDat1 = transformImageToData(xIm1);

  // Store the search directions
  dirs.xIm.setRow(0, xIm0);
  dirs.xIm.setRow(1, xIm1);
  dirs.xDat.setRow(0, xDat0);
  dirs.xDat.setRow(1, xDat1);

  // Now compute the quadratic coefficients SB. eq 24

  // First compute s1, c1
  for (size_t k = 0; k < dim; k++) {
    dirs.c1[k][0] = dirs.s1[k][0] = 0.;
    for (size_t i = 0; i < npoints; i++) {
      dirs.s1[k][0] += dirs.xIm[k][i] * sgrad[i];
      dirs.c1[k][0] += dirs.xIm[k][i] * cgrad[i];
    }
    // Note: the factor chiSQ has to go either here or in calculateChi
    dirs.c1[k][0] /= chiSq;
  }

  // Then s2, c2
  for (size_t k = 0; k < dim; k++) {
    for (size_t l = 0; l < k + 1; l++) {
      dirs.s2[k][l] = 0.;
      dirs.c2[k][l] = 0.;
      for (size_t i = 0; i < npoints; i++) {
        if (error[i])
          dirs.c2[k][l] +=
              dirs.xDat[k][i] * dirs.xDat[l][i] / error[i] / error[i];
        dirs.s2[k][l] -= dirs.xIm[k][i] * dirs.xIm[l][i] / fabs(image[i]);
      }
      // Note: the factor chiSQ has to go either here or in calculateChi
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

/** Bisection method to move beta one step closer towards the solution
* @param dirs :: [input] The current quadratic coefficients
* @param chiTarget :: [input] The requested Chi target
* @param chiEps :: [input] Precision required for Chi target
* @param alphaIter :: [input] Maximum number of iterations in the bisection
* method (alpha chop)
*/
std::vector<double> MaxEnt::move(const SearchDirections &dirs, double chiTarget,
                                 double chiEps, size_t alphaIter) {

  double aMin = 0.; // Minimum alpha
  double aMax = 1.; // Maximum alpha

  // Dimension, number of search directions
  size_t dim = dirs.c2.size().first;

  std::vector<double> betaMin(dim, 0); // Beta at alpha min
  std::vector<double> betaMax(dim, 0); // Beta at alpha max

  double chiMin = calculateChi(dirs, aMin, betaMin); // Chi at alpha min
  double chiMax = calculateChi(dirs, aMax, betaMax); // Chi at alpha max

  double dchiMin = chiMin - chiTarget; // Delta = max - target
  double dchiMax = chiMax - chiTarget; // Delta = min - target

  if (dchiMin * dchiMax > 0) {
    // ChiTarget could be outside the range [chiMin, chiMax]

    if (fabs(dchiMin) < fabs(dchiMax)) {
      return betaMin;
    } else {
      return betaMax;
    }
    //    throw std::runtime_error("Error in alpha chop\n");
  }

  // Initial values of eps and iter to start while loop
  double eps = 2. * chiEps;
  size_t iter = 0;

  // Bisection method

  std::vector<double> beta(dim, 0); // Beta at current alpha

  while ((fabs(eps) > chiEps) && (iter < alphaIter)) {

    double aMid = 0.5 * (aMin + aMax);
    double chiMid = calculateChi(dirs, aMid, beta);

    eps = chiMid - chiTarget;

    if (dchiMin * eps > 0) {
      aMin = aMid;
      dchiMin = eps;
    }

    if (dchiMax * eps > 0) {
      aMax = aMid;
      dchiMax = eps;
    }

    iter++;
  }

  // Check if move was successful
  if ((fabs(eps) > chiEps) || (iter > alphaIter)) {

    throw std::runtime_error("Error encountered when calculating solution "
                             "image. No convergence in alpha chop.\n");
  }

  return beta;
}

/** Calculates Chi given the quadratic coefficients and an alpha value by
* solving the matrix equation A*b = B
* @param dirs :: [input] The quadratic coefficients
* @param a :: [input] The alpha value
* @param b :: [output] The solution
* @return :: The calculated chi-square
*/
double MaxEnt::calculateChi(const SearchDirections &dirs, double a,
                            std::vector<double> &b) {

  size_t dim = dirs.c2.size().first;

  double ax = a;
  double bx = 1 - ax;

  Kernel::DblMatrix A(dim, dim);
  Kernel::DblMatrix B(dim, 1);

  // Construct the matrix A and vector B such that Ax=B
  for (size_t k = 0; k < dim; k++) {
    for (size_t l = 0; l < dim; l++) {
      A[k][l] = bx * dirs.c2[k][l] - ax * dirs.s2[k][l];
    }
    B[k][0] = -bx * dirs.c1[k][0] + ax * dirs.s1[k][0];
  }

  // Alternatives I have tried:
  // Gauss-Jordan
  // LU
  // SVD seems to work better

  // Solve using SVD
  b = solveSVD(A, B);

  // Now compute Chi
  double ww = 0.;
  for (size_t k = 0; k < dim; k++) {
    double z = 0.;
    for (size_t l = 0; l < dim; l++) {
      z += dirs.c2[k][l] * b[l];
    }
    ww += b[k] * (dirs.c1[k][0] + 0.5 * z);
  }

  // Return chi
  return ww + 1.;
}

/** Solves A*x = B using SVD
* @param A :: [input] The matrix A
* @param B :: [input] The vector B
* @return :: The solution x
*/
std::vector<double> MaxEnt::solveSVD(const DblMatrix &A, const DblMatrix &B) {

  size_t dim = A.size().first;

  gsl_matrix *a = gsl_matrix_alloc(dim, dim);
  gsl_matrix *v = gsl_matrix_alloc(dim, dim);
  gsl_vector *s = gsl_vector_alloc(dim);
  gsl_vector *w = gsl_vector_alloc(dim);
  gsl_vector *x = gsl_vector_alloc(dim);
  gsl_vector *b = gsl_vector_alloc(dim);

  // Need to copy from DblMatrix to gsl matrix

  for (size_t k = 0; k < dim; k++)
    for (size_t l = 0; l < dim; l++)
      gsl_matrix_set(a, k, l, A[k][l]);
  for (size_t k = 0; k < dim; k++)
    gsl_vector_set(b, k, B[k][0]);

  // Singular value decomposition
  gsl_linalg_SV_decomp(a, v, s, w);

  // A could be singular or ill-conditioned. We can use SVD to obtain a least
  // squares
  // solution by setting the small (compared to the maximum) singular values to
  // zero

  // Find largest sing value
  double max = gsl_vector_get(s, 0);
  for (size_t i = 0; i < dim; i++) {
    if (max < gsl_vector_get(s, i))
      max = gsl_vector_get(s, i);
  }

  // Apply a threshold to small singular values
  const double THRESHOLD = 1E-6;
  double threshold = THRESHOLD * max;

  for (size_t i = 0; i < dim; i++)
    if (gsl_vector_get(s, i) > threshold)
      gsl_vector_set(s, i, gsl_vector_get(s, i));
    else
      gsl_vector_set(s, i, 0);

  // Solve A*x = B
  gsl_linalg_SV_solve(a, v, s, b, x);

  // From gsl_vector to vector
  std::vector<double> beta(dim);
  for (size_t k = 0; k < dim; k++)
    beta[k] = gsl_vector_get(x, k);

  gsl_matrix_free(a);
  gsl_matrix_free(v);
  gsl_vector_free(s);
  gsl_vector_free(w);
  gsl_vector_free(x);
  gsl_vector_free(b);

  return beta;
}

/** Calculates the distance of the current solution
* @param s2 :: [input] The current quadratic coefficient for the entropy S
* @param beta :: [input] The current beta vector
* @return :: The distance
*/
double MaxEnt::distance(const DblMatrix &s2, const std::vector<double> &beta) {

  size_t dim = s2.size().first;

  double dist = 0.;

  for (size_t k = 0; k < dim; k++) {
    double sum = 0.0;
    for (size_t l = 0; l < dim; l++)
      sum -= s2[k][l] * beta[l];
    dist += beta[k] * sum;
  }
  return dist;
}

void MaxEnt::populateOutputWS(const MatrixWorkspace_sptr &inWS, size_t spec,
                              size_t nspec, const std::vector<double> &data,
                              const std::vector<double> &image,
                              MatrixWorkspace_sptr &outData,
                              MatrixWorkspace_sptr &outImage) {

  if (data.size() % 2)
    throw std::invalid_argument("Cannot write results to output workspaces");

  int npoints = static_cast<int>(data.size() / 2);
  int npointsX = inWS->isHistogramData() ? npoints + 1 : npoints;
  MantidVec X(npointsX);
  MantidVec YR(npoints);
  MantidVec YI(npoints);
  MantidVec E(npoints, 0.);

  // Reconstructed data

  for (int i = 0; i < npoints; i++)
    YR[i] = data[2 * i];
  outData->dataX(spec) = inWS->readX(spec);
  outData->dataY(spec).assign(YR.begin(), YR.end());

  // Reconstructed image

  double dx = inWS->readX(spec)[1] - inWS->readX(spec)[0];
  double delta = 1. / dx / npoints;
  int isOdd = (inWS->blocksize() % 2) ? 1 : 0;

  for (int i = 0; i < npoints; i++) {
    int j = (npoints / 2 + i + isOdd) % npoints;
    X[i] = delta * (-npoints / 2 + i);
    YR[i] = image[2 * j] * dx;
    YI[i] = image[2 * j + 1] * dx;
  }
  if (npointsX == npoints + 1)
    X[npoints] = X[npoints - 1] + delta;

  outImage->dataX(spec).assign(X.begin(), X.end());
  outImage->dataY(spec).assign(YR.begin(), YR.end());
  outImage->dataX(nspec + spec).assign(X.begin(), X.end());
  outImage->dataY(nspec + spec).assign(YI.begin(), YI.end());

  // No errors
  outData->dataE(spec).assign(E.begin(), E.end());
  outImage->dataE(spec).assign(E.begin(), E.end());
  outImage->dataE(spec + nspec).assign(E.begin(), E.end());
}

} // namespace Algorithms
} // namespace Mantid
