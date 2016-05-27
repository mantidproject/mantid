#include "MantidAlgorithms/MaxEnt.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/TextAxis.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidAlgorithms/MaxEnt/MaxentEntropyNegativeValues.h"
#include "MantidAlgorithms/MaxEnt/MaxentEntropyPositiveValues.h"
#include "MantidKernel/BoundedValidator.h"
#include "MantidKernel/ListValidator.h"
#include "MantidKernel/UnitFactory.h"
#include <boost/make_shared.hpp>
#include <boost/shared_array.hpp>
#include <gsl/gsl_linalg.h>

namespace Mantid {
namespace Algorithms {

using Mantid::Kernel::Direction;
using Mantid::API::WorkspaceProperty;

using namespace API;
using namespace Kernel;

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(MaxEnt)

namespace {

// Maps defining the inverse caption and label for the reconstructed image
// Example:
// The input workspaces (X axis) is in (Time, s)
// The output image should be in (Frequency, Hz)

// Defines the new caption
std::map<std::string, std::string> inverseCaption = {{"Time", "Frequency"},
                                                     {"Frequency", "Time"},
                                                     {"d-Spacing", "q"},
                                                     {"q", "d-Spacing"}};
// Defines the new label
std::map<std::string, std::string> inverseLabel = {{"s", "Hz"},
                                                   {"microsecond", "MHz"},
                                                   {"Hz", "s"},
                                                   {"MHz", "microsecond"},
                                                   {"Angstrom", "Angstrom^-1"},
                                                   {"Angstrom^-1", "Angstrom"}};
}

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
         "likely to change. It currently works for the case where data and "
         "image are related by a Fourier transform.";
}

//----------------------------------------------------------------------------------------------
/** Initialize the algorithm's properties.
 */
void MaxEnt::init() {

  declareProperty(
      make_unique<WorkspaceProperty<>>("InputWorkspace", "", Direction::Input),
      "An input workspace.");

  declareProperty("ComplexData", false,
                  "Whether or not the input data are complex. If true, the "
                  "input workspace is expected to have an even number of "
                  "histograms.");

  declareProperty("PositiveImage", false, "If true, the reconstructed image "
                                          "must be positive. It can take "
                                          "negative values otherwise");

  declareProperty("AutoShift", false,
                  "Automatically calculate and apply phase shift. Zero on the "
                  "X axis is assumed to be in the first bin. If it is not, "
                  "setting this property will automatically correct for this.");

  auto mustBePositive = boost::make_shared<BoundedValidator<size_t>>();
  mustBePositive->setLower(0);
  declareProperty(make_unique<PropertyWithValue<size_t>>(
                      "ResolutionFactor", 1, mustBePositive, Direction::Input),
                  "An integer number indicating the factor by which the number "
                  "of points will be increased in the image and reconstructed "
                  "data");

  auto mustBeNonNegative = boost::make_shared<BoundedValidator<double>>();
  mustBeNonNegative->setLower(1E-12);
  declareProperty(make_unique<PropertyWithValue<double>>(
                      "A", 0.4, mustBeNonNegative, Direction::Input),
                  "A maximum entropy constant");

  declareProperty(make_unique<PropertyWithValue<double>>(
                      "ChiTarget", 100.0, mustBeNonNegative, Direction::Input),
                  "Target value of Chi-square");

  declareProperty(make_unique<PropertyWithValue<double>>(
                      "ChiEps", 0.001, mustBeNonNegative, Direction::Input),
                  "Required precision for Chi-square");

  declareProperty(make_unique<PropertyWithValue<double>>("DistancePenalty", 0.1,
                                                         mustBeNonNegative,
                                                         Direction::Input),
                  "Distance penalty applied to the current image");

  declareProperty(make_unique<PropertyWithValue<double>>(
                      "MaxAngle", 0.05, mustBeNonNegative, Direction::Input),
                  "Maximum degree of non-parallelism between S and C");

  mustBePositive = boost::make_shared<BoundedValidator<size_t>>();
  mustBePositive->setLower(1);
  declareProperty(make_unique<PropertyWithValue<size_t>>(
                      "MaxIterations", 20000, mustBePositive, Direction::Input),
                  "Maximum number of iterations");

  declareProperty(make_unique<PropertyWithValue<size_t>>("AlphaChopIterations",
                                                         500, mustBePositive,
                                                         Direction::Input),
                  "Maximum number of iterations in alpha chop");

  declareProperty(
      make_unique<WorkspaceProperty<>>("EvolChi", "", Direction::Output),
      "Output workspace containing the evolution of Chi-sq");
  declareProperty(
      make_unique<WorkspaceProperty<>>("EvolAngle", "", Direction::Output),
      "Output workspace containing the evolution of "
      "non-paralellism between S and C");
  declareProperty(make_unique<WorkspaceProperty<>>("ReconstructedImage", "",
                                                   Direction::Output),
                  "The output workspace containing the reconstructed image.");
  declareProperty(make_unique<WorkspaceProperty<>>("ReconstructedData", "",
                                                   Direction::Output),
                  "The output workspace containing the reconstructed data.");
}

//----------------------------------------------------------------------------------------------
/** Validate the input properties.
*/
std::map<std::string, std::string> MaxEnt::validateInputs() {

  std::map<std::string, std::string> result;

  MatrixWorkspace_sptr inWS = getProperty("InputWorkspace");

  if (inWS) {

    // 1. X values in input workspace must be (almost) equally spaced

    const double warningLevel = 0.01;
    const double errorLevel = 0.5;
    bool printWarning = false;
    // Average spacing
    const MantidVec &X = inWS->readX(0);
    const double dx =
        (X[X.size() - 1] - X[0]) / static_cast<double>(X.size() - 1);
    for (size_t i = 1; i < X.size() - 1; i++) {
      // 1% accuracy exceeded, but data still usable
      if (std::abs(X[i] - X[0] - static_cast<double>(i) * dx) / dx >
          warningLevel) {
        printWarning = true;
        if (std::abs(X[i] - X[0] - static_cast<double>(i) * dx) / dx >
            errorLevel) {
          // 50% accuracy exceeded, data not usable
          printWarning = false;
          result["InputWorkspace"] =
              "X axis must be linear (all bins have same width)";
          break;
        }
      }
    }
    if (printWarning) {
      g_log.warning() << "Bin widths differ by more than " << warningLevel * 100
                      << "% of average\n";
    }

    // 2. If the input signal is complex, we expect an even number of histograms
    // in the input workspace

    size_t nhistograms = inWS->getNumberHistograms();
    bool complex = getProperty("ComplexData");
    if (complex && (nhistograms % 2))
      result["InputWorkspace"] = "The number of histograms in the input "
                                 "workspace must be even for complex data";
  }

  return result;
}

//----------------------------------------------------------------------------------------------
/** Execute the algorithm.
 */
void MaxEnt::exec() {

  // MaxEnt parameters
  // Complex data?
  bool complex = getProperty("ComplexData");
  // Image must be positive?
  bool positiveImage = getProperty("PositiveImage");
  // Autoshift
  bool autoShift = getProperty("AutoShift");
  // Increase the number of points in the image by this factor
  size_t resolutionFactor = getProperty("ResolutionFactor");
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
  // Number of spectra and datapoints
  // Read input workspace
  MatrixWorkspace_sptr inWS = getProperty("InputWorkspace");
  // Number of spectra
  size_t nspec = inWS->getNumberHistograms();
  // Number of data points
  size_t npoints = inWS->blocksize() * resolutionFactor;
  // Number of X bins
  size_t npointsX = inWS->isHistogramData() ? npoints + 1 : npoints;

  // The type of entropy we are going to use (depends on the type of image,
  // positive only, or positive and/or negative)
  MaxentData_sptr maxentData;
  if (positiveImage) {
    maxentData = boost::make_shared<MaxentData>(
        boost::make_shared<MaxentEntropyPositiveValues>());
  } else {
    maxentData = boost::make_shared<MaxentData>(
        boost::make_shared<MaxentEntropyNegativeValues>());
  }

  // Output workspaces
  MatrixWorkspace_sptr outImageWS;
  MatrixWorkspace_sptr outDataWS;
  MatrixWorkspace_sptr outEvolChi;
  MatrixWorkspace_sptr outEvolTest;

  nspec = complex ? nspec / 2 : nspec;
  outImageWS =
      WorkspaceFactory::Instance().create(inWS, 2 * nspec, npointsX, npoints);
  outDataWS =
      WorkspaceFactory::Instance().create(inWS, 2 * nspec, npointsX, npoints);
  outEvolChi = WorkspaceFactory::Instance().create(inWS, nspec, niter, niter);
  outEvolTest = WorkspaceFactory::Instance().create(inWS, nspec, niter, niter);

  npoints *= 2;
  for (size_t s = 0; s < nspec; s++) {

    // Start distribution (flat background)
    std::vector<double> image(npoints, background);

    if (complex) {
      auto dataRe = inWS->readY(s);
      auto dataIm = inWS->readY(s + nspec);
      auto errorsRe = inWS->readE(s);
      auto errorsIm = inWS->readE(s + nspec);
      maxentData->loadComplex(dataRe, dataIm, errorsRe, errorsIm, image,
                              background);
    } else {
      auto data = inWS->readY(s);
      auto error = inWS->readE(s);
      maxentData->loadReal(data, error, image, background);
    }

    // To record the algorithm's progress
    std::vector<double> evolChi(niter, 0.);
    std::vector<double> evolTest(niter, 0.);

    // Progress
    Progress progress(this, 0, 1, niter);

    // Run maxent algorithm
    for (size_t it = 0; it < niter; it++) {

      // Calculate quadratic model coefficients
      // (SB eq. 21 and 24)
      maxentData->calculateQuadraticCoefficients();
      double currAngle = maxentData->getAngle();
      double currChisq = maxentData->getChisq();
      auto coeffs = maxentData->getQuadraticCoefficients();

      // Calculate delta to construct new image (SB eq. 25)
      auto delta = move(coeffs, chiTarget / currChisq, chiEps, alphaIter);

      // Apply distance penalty (SB eq. 33)
      image = maxentData->getImage();
      delta = applyDistancePenalty(delta, coeffs, image, background, distEps);

      // Update image according to 'delta' and calculate the new Chi-square
      maxentData->updateImage(delta);
      currChisq = maxentData->getChisq();

      // Record the evolution of Chi-square and angle(S,C)
      evolChi[it] = currChisq;
      evolTest[it] = currAngle;

      // Stop condition, solution found
      if ((std::abs(currChisq / chiTarget - 1.) < chiEps) &&
          (currAngle < angle)) {
        break;
      }

      // Check for canceling the algorithm
      if (!(it % 1000)) {
        interruption_point();
      }

      progress.report();

    } // iterations

    // Get calculated data
    auto solData = maxentData->getReconstructedData();
    auto solImage = maxentData->getImage();

    // Populate the output workspaces
    populateDataWS(inWS, s, nspec, solData, outDataWS);
    populateImageWS(inWS, s, nspec, solImage, outImageWS, autoShift);

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

/** Bisection method to move delta one step closer towards the solution
* @param coeffs :: [input] The current quadratic coefficients
* @param chiTarget :: [input] The requested Chi target
* @param chiEps :: [input] Precision required for Chi target
* @param alphaIter :: [input] Maximum number of iterations in the bisection
* method (alpha chop)
* @return : The increment length to be added to the current image
*/
std::vector<double> MaxEnt::move(const QuadraticCoefficients &coeffs,
                                 double chiTarget, double chiEps,
                                 size_t alphaIter) {

  double aMin = 0.; // Minimum alpha
  double aMax = 1.; // Maximum alpha

  // Dimension, number of search directions
  size_t dim = coeffs.c2.size().first;

  std::vector<double> deltaMin(dim, 0); // delta at alpha min
  std::vector<double> deltaMax(dim, 0); // delta at alpha max

  double chiMin = calculateChi(coeffs, aMin, deltaMin); // Chi at alpha min
  double chiMax = calculateChi(coeffs, aMax, deltaMax); // Chi at alpha max

  double dchiMin = chiMin - chiTarget; // max - target
  double dchiMax = chiMax - chiTarget; // min - target

  if (dchiMin * dchiMax > 0) {
    // ChiTarget could be outside the range [chiMin, chiMax]

    if (fabs(dchiMin) < fabs(dchiMax)) {
      return deltaMin;
    } else {
      return deltaMax;
    }
    //    throw std::runtime_error("Error in alpha chop\n");
  }

  // Initial values of eps and iter to start while loop
  double eps = 2. * chiEps;
  size_t iter = 0;

  // Bisection method

  std::vector<double> delta(dim, 0); // delta at current alpha

  while ((fabs(eps) > chiEps) && (iter < alphaIter)) {

    double aMid = 0.5 * (aMin + aMax);
    double chiMid = calculateChi(coeffs, aMid, delta);

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

  return delta;
}

/** Calculates Chi given the quadratic coefficients and an alpha value by
* solving the matrix equation A*b = B
* @param coeffs :: [input] The quadratic coefficients
* @param a :: [input] The alpha value
* @param b :: [output] The solution
* @return :: The calculated chi-square
*/
double MaxEnt::calculateChi(const QuadraticCoefficients &coeffs, double a,
                            std::vector<double> &b) {

  size_t dim = coeffs.c2.size().first;

  double ax = a;
  double bx = 1 - ax;

  Kernel::DblMatrix A(dim, dim);
  Kernel::DblMatrix B(dim, 1);

  // Construct the matrix A and vector B such that Ax=B
  for (size_t k = 0; k < dim; k++) {
    for (size_t l = 0; l < dim; l++) {
      A[k][l] = bx * coeffs.c2[k][l] - ax * coeffs.s2[k][l];
    }
    B[k][0] = -bx * coeffs.c1[k][0] + ax * coeffs.s1[k][0];
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
      z += coeffs.c2[k][l] * b[l];
    }
    ww += b[k] * (coeffs.c1[k][0] + 0.5 * z);
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
  std::vector<double> delta(dim);
  for (size_t k = 0; k < dim; k++)
    delta[k] = gsl_vector_get(x, k);

  gsl_matrix_free(a);
  gsl_matrix_free(v);
  gsl_vector_free(s);
  gsl_vector_free(w);
  gsl_vector_free(x);
  gsl_vector_free(b);

  return delta;
}

/** Applies a distance penalty
* @param delta :: [input] The current increment
* @param coeffs :: [input] The quadratic coefficients
* @param image :: [input] The current image
* @param background :: [input] The background
* @param distEps :: [input] The distance constraint
* @return :: The new increment
*/
std::vector<double> MaxEnt::applyDistancePenalty(
    const std::vector<double> &delta, const QuadraticCoefficients &coeffs,
    const std::vector<double> &image, double background, double distEps) {

  double sum = 0.;
  for (double point : image)
    sum += fabs(point);

  size_t dim = coeffs.s2.size().first;

  double dist = 0.;

  for (size_t k = 0; k < dim; k++) {
    double sum = 0.0;
    for (size_t l = 0; l < dim; l++)
      sum -= coeffs.s2[k][l] * delta[l];
    dist += delta[k] * sum;
  }

  auto newDelta = delta;
  if (dist > distEps * sum / background) {
    for (size_t k = 0; k < delta.size(); k++) {
      newDelta[k] *= sqrt(sum / dist / background);
    }
  }
  return newDelta;
}

/** Populates the output workspaces
* @param inWS :: [input] The input workspace
* @param spec :: [input] The current spectrum being analyzed
* @param nspec :: [input] The total number of histograms in the input workspace
* @param result :: [input] The result to be written in the output workspace
* @param outWS :: [input] The output workspace to populate
* @param autoShift :: [input] Whether or not to correct the phase shift
*/
void MaxEnt::populateImageWS(const MatrixWorkspace_sptr &inWS, size_t spec,
                             size_t nspec, const std::vector<double> &result,
                             MatrixWorkspace_sptr &outWS, bool autoShift) {

  if (result.size() % 2)
    throw std::invalid_argument("Cannot write results to output workspaces");

  int npoints = static_cast<int>(result.size() / 2);
  int npointsX = inWS->isHistogramData() ? npoints + 1 : npoints;
  MantidVec X(npointsX);
  MantidVec YR(npoints);
  MantidVec YI(npoints);
  MantidVec E(npoints, 0.);

  double x0 = inWS->readX(spec)[0];
  double dx = inWS->readX(spec)[1] - x0;

  double delta = 1. / dx / npoints;
  int isOdd = (inWS->blocksize() % 2) ? 1 : 0;

  double shift = x0 * 2 * M_PI;
  if (!autoShift)
    shift = 0;

  for (int i = 0; i < npoints; i++) {
    int j = (npoints / 2 + i + isOdd) % npoints;
    X[i] = delta * (-npoints / 2 + i);

    double xShift = X[i] * shift;
    double c = cos(xShift);
    double s = sin(xShift);
    YR[i] = result[2 * j] * c - result[2 * j + 1] * s;
    YI[i] = result[2 * j] * s + result[2 * j + 1] * c;
    YR[i] *= dx;
    YI[i] *= dx;
  }
  if (npointsX == npoints + 1)
    X[npoints] = X[npoints - 1] + delta;

  // Caption & label
  auto inputUnit = inWS->getAxis(0)->unit();
  if (inputUnit) {
    boost::shared_ptr<Kernel::Units::Label> lblUnit =
        boost::dynamic_pointer_cast<Kernel::Units::Label>(
            UnitFactory::Instance().create("Label"));
    if (lblUnit) {

      lblUnit->setLabel(
          inverseCaption[inWS->getAxis(0)->unit()->caption()],
          inverseLabel[inWS->getAxis(0)->unit()->label().ascii()]);
      outWS->getAxis(0)->unit() = lblUnit;
    }
  }

  outWS->dataX(spec).assign(X.begin(), X.end());
  outWS->dataY(spec).assign(YR.begin(), YR.end());
  outWS->dataE(spec).assign(E.begin(), E.end());
  outWS->dataX(nspec + spec).assign(X.begin(), X.end());
  outWS->dataY(nspec + spec).assign(YI.begin(), YI.end());
  outWS->dataE(nspec + spec).assign(E.begin(), E.end());
}

/** Populates the output workspaces
* @param inWS :: [input] The input workspace
* @param spec :: [input] The current spectrum being analyzed
* @param nspec :: [input] The total number of histograms in the input workspace
* @param result :: [input] The result to be written in the output workspace
* @param outWS :: [input] The output workspace to populate
*/
void MaxEnt::populateDataWS(const MatrixWorkspace_sptr &inWS, size_t spec,
                            size_t nspec, const std::vector<double> &result,
                            MatrixWorkspace_sptr &outWS) {

  if (result.size() % 2)
    throw std::invalid_argument("Cannot write results to output workspaces");

  int npoints = static_cast<int>(result.size() / 2);
  int npointsX = inWS->isHistogramData() ? npoints + 1 : npoints;
  MantidVec X(npointsX);
  MantidVec YR(npoints);
  MantidVec YI(npoints);
  MantidVec E(npoints, 0.);

  double x0 = inWS->readX(spec)[0];
  double dx = inWS->readX(spec)[1] - x0;

  for (int i = 0; i < npoints; i++) {
    X[i] = x0 + i * dx;
    YR[i] = result[2 * i];
    YI[i] = result[2 * i + 1];
  }
  if (npointsX == npoints + 1)
    X[npoints] = x0 + npoints * dx;

  outWS->dataX(spec).assign(X.begin(), X.end());
  outWS->dataY(spec).assign(YR.begin(), YR.end());
  outWS->dataE(spec).assign(E.begin(), E.end());
  outWS->dataX(nspec + spec).assign(X.begin(), X.end());
  outWS->dataY(nspec + spec).assign(YI.begin(), YI.end());
  outWS->dataE(nspec + spec).assign(E.begin(), E.end());
}

} // namespace Algorithms
} // namespace Mantid
