// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidAlgorithms/MaxEnt.h"
#include "MantidAPI/EqualBinSizesValidator.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/TextAxis.h"
#include "MantidAlgorithms/MaxEnt/MaxentEntropyNegativeValues.h"
#include "MantidAlgorithms/MaxEnt/MaxentEntropyPositiveValues.h"
#include "MantidAlgorithms/MaxEnt/MaxentSpaceComplex.h"
#include "MantidAlgorithms/MaxEnt/MaxentSpaceReal.h"
#include "MantidAlgorithms/MaxEnt/MaxentTransformFourier.h"
#include "MantidAlgorithms/MaxEnt/MaxentTransformMultiFourier.h"
#include "MantidDataObjects/TableWorkspace.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidDataObjects/WorkspaceCreation.h"
#include "MantidHistogramData/Histogram.h"
#include "MantidHistogramData/HistogramBuilder.h"
#include "MantidHistogramData/LinearGenerator.h"
#include "MantidKernel/BoundedValidator.h"
#include "MantidKernel/ListValidator.h"
#include "MantidKernel/UnitFactory.h"
#include "MantidKernel/VectorHelper.h"
#include <algorithm>
#include <gsl/gsl_linalg.h>
#include <numeric>

namespace Mantid {
namespace Algorithms {

using Mantid::HistogramData::LinearGenerator;
using Mantid::HistogramData::Points;

using namespace API;
using namespace Kernel;
using namespace Mantid::DataObjects;
using namespace Mantid::HistogramData;

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
// A threshold for small singular values
const double THRESHOLD = 1E-6;

/** removes zeros from converged results
 * @param ws :: [input] The input workspace with zeros
 * @param itCount [input] The number of iterations this alg used for each
 * spectrum
 * @param yLabel :: [input] y-label to use for returned ws
 * @return : ws cut down in lenght to maxIt
 */
MatrixWorkspace_sptr removeZeros(MatrixWorkspace_sptr &ws,
                                 const std::vector<size_t> &itCount,
                                 const std::string &yLabel) {

  ws->setYUnitLabel(yLabel);
  ws->getAxis(0)->unit() = UnitFactory::Instance().create("Label");
  Unit_sptr unit = ws->getAxis(0)->unit();
  boost::shared_ptr<Units::Label> label =
      boost::dynamic_pointer_cast<Units::Label>(unit);
  label->setLabel("Number of Iterations", "");

  const size_t nspec = ws->getNumberHistograms();
  if (itCount.empty()) {
    return ws; // In case, we don't have any spectra
  }
  for (size_t spec = 0; spec < nspec; spec++) {
    auto &dataX = ws->dataX(spec);
    dataX.resize(itCount[spec]);
    auto &dataY = ws->dataY(spec);
    dataY.resize(itCount[spec]);
    auto &dataE = ws->dataE(spec);
    dataE.resize(itCount[spec]);
  }
  return ws;
}
} // namespace

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
         "It currently works for the case where data and image are related by a"
         " 1D Fourier transform.";
}

//----------------------------------------------------------------------------------------------
/** Initialize the algorithm's properties.
 */
void MaxEnt::init() {

  // X values in input workspace must be (almost) equally spaced
  const double warningLevel = 0.01;
  const double errorLevel = 0.5;
  declareProperty(
      std::make_unique<WorkspaceProperty<>>(
          "InputWorkspace", "", Direction::Input,
          boost::make_shared<EqualBinSizesValidator>(errorLevel, warningLevel)),
      "An input workspace.");

  declareProperty("ComplexData", false,
                  "If true, the input data is assumed to be complex and the "
                  "input workspace is expected to have an even number of "
                  "histograms (2N). Spectrum numbers S and S+N are assumed to "
                  "be the real and imaginary part of the complex signal "
                  "respectively.");

  declareProperty("ComplexImage", true,
                  "If true, the algorithm will use complex images for the "
                  "calculations. This is the recommended option when there is "
                  "no prior knowledge about the image. If the image is known "
                  "to be real, this option can be set to false and the "
                  "algorithm will only consider the real part for "
                  "calculations.");

  declareProperty("PositiveImage", false,
                  "If true, the reconstructed image is only allowed to take "
                  "positive values. It can take negative values otherwise. "
                  "This option defines the entropy formula that will be used "
                  "for the calculations (see next section for more details).");

  declareProperty("AutoShift", false,
                  "Automatically calculate and apply phase shift. Zero on the "
                  "X axis is assumed to be in the first bin. If it is not, "
                  "setting this property will automatically correct for this.");

  auto mustBePositive = boost::make_shared<BoundedValidator<size_t>>();
  mustBePositive->setLower(0);
  declareProperty(std::make_unique<PropertyWithValue<size_t>>(
                      "ResolutionFactor", 1, mustBePositive, Direction::Input),
                  "An integer number indicating the factor by which the number "
                  "of points will be increased in the image and reconstructed "
                  "data");

  auto mustBeNonNegative = boost::make_shared<BoundedValidator<double>>();
  mustBeNonNegative->setLower(1E-12);
  declareProperty(
      std::make_unique<PropertyWithValue<double>>("A", 0.4, mustBeNonNegative,
                                                  Direction::Input),
      "A maximum entropy constant. This algorithm was first developed for the "
      "ISIS muon group where the default 0.4 was found to give good "
      "reconstructions. "
      "In general the user will need to experiment with this value. Choosing a "
      "small value may lead to unphysical spiky reconstructions and choosing "
      "an increasingly large "
      "value the reconstruction will start to resamble that of a direct "
      "fourier "
      "transform reconstruction. However, where the data contain a "
      "zero Fourier data point with a small error the "
      "reconstruction will be insensitive to the choice "
      "of this property (and increasing so the more well determined "
      "this data point is).");

  declareProperty(
      std::make_unique<PropertyWithValue<double>>(
          "ChiTargetOverN", 1.0, mustBeNonNegative, Direction::Input),
      "Target value of Chi-square divided by the number of data points (N)");

  declareProperty(std::make_unique<PropertyWithValue<double>>(
                      "ChiEps", 0.001, mustBeNonNegative, Direction::Input),
                  "Required precision for Chi-square");

  declareProperty(
      std::make_unique<PropertyWithValue<double>>(
          "DistancePenalty", 0.1, mustBeNonNegative, Direction::Input),
      "Distance penalty applied to the current image at each iteration.");

  declareProperty(
      std::make_unique<PropertyWithValue<double>>(
          "MaxAngle", 0.001, mustBeNonNegative, Direction::Input),
      "Maximum degree of non-parallelism between S (the entropy) and C "
      "(chi-squared). These needs to be parallel. Chosing a smaller "
      "shouldn't change the output. However, if you find this is the "
      "case please let the Mantid team know since this indicates that "
      "the default value of this proporty may need changing or "
      "other changes to this implementation are required.");

  mustBePositive = boost::make_shared<BoundedValidator<size_t>>();
  mustBePositive->setLower(1);
  declareProperty(std::make_unique<PropertyWithValue<size_t>>(
                      "MaxIterations", 20000, mustBePositive, Direction::Input),
                  "Maximum number of iterations.");

  declareProperty(
      std::make_unique<PropertyWithValue<size_t>>(
          "AlphaChopIterations", 500, mustBePositive, Direction::Input),
      "Maximum number of iterations in alpha chop.");
  declareProperty(
      std::make_unique<WorkspaceProperty<>>(
          "DataLinearAdj", "", Direction::Input, PropertyMode::Optional,
          boost::make_shared<EqualBinSizesValidator>(errorLevel, warningLevel)),
      "Adjusts the calculated data by multiplying each value by the "
      "corresponding Y value of this workspace. "
      "The data in this workspace is complex in the same manner as complex "
      "input data.");
  declareProperty(
      std::make_unique<WorkspaceProperty<>>(
          "DataConstAdj", "", Direction::Input, PropertyMode::Optional,
          boost::make_shared<EqualBinSizesValidator>(errorLevel, warningLevel)),
      "Adjusts the calculated data by adding to each value the corresponding Y "
      "value of this workspace. "
      "If DataLinearAdj is also specified, this addition is done after its "
      "multiplication. "
      "See equation in documentation for how DataLinearAdj and DataConstAdj "
      "are applied. "
      "The data in this workspace is complex in the same manner as complex "
      "input data.");
  declareProperty(
      "PerSpectrumReconstruction", true,
      "Reconstruction is done independently on each spectrum. "
      "If false, all the spectra use one image and the reconstructions "
      "differ only through their adjustments. "
      "ComplexData must be set true, when this is false.");

  declareProperty(
      std::make_unique<WorkspaceProperty<>>("EvolChi", "", Direction::Output),
      "Output workspace containing the evolution of Chi-sq.");
  declareProperty(
      std::make_unique<WorkspaceProperty<>>("EvolAngle", "", Direction::Output),
      "Output workspace containing the evolution of "
      "non-paralellism between S and C.");
  declareProperty(std::make_unique<WorkspaceProperty<>>("ReconstructedImage",
                                                        "", Direction::Output),
                  "The output workspace containing the reconstructed image.");
  declareProperty(std::make_unique<WorkspaceProperty<>>("ReconstructedData", "",
                                                        Direction::Output),
                  "The output workspace containing the reconstructed data.");
}

//----------------------------------------------------------------------------------------------
/** Validate the input properties.
 */
std::map<std::string, std::string> MaxEnt::validateInputs() {

  std::map<std::string, std::string> result;

  MatrixWorkspace_sptr inWS = getProperty("InputWorkspace");

  size_t nHistograms = 0;
  if (inWS) {
    // If the input signal is complex, we expect an even number of histograms
    // in the input workspace

    nHistograms = inWS->getNumberHistograms();
    bool complex = getProperty("ComplexData");
    if (complex && (nHistograms % 2))
      result["InputWorkspace"] = "The number of histograms in the input "
                                 "workspace must be even for complex data";
    if (!complex)
      nHistograms *= 2; // Double number of real histograms to compare with
                        // adjustments, which are always complex.
  }

  // Check linear adjustments, we expect and even number of histograms
  // and if any, they must be sufficient for all spectra in input workspace,
  // if per spectrum reconstruction is done.
  MatrixWorkspace_sptr linAdj = getProperty("DataLinearAdj");
  size_t nAHistograms = 0;
  if (linAdj)
    nAHistograms = linAdj->getNumberHistograms();
  if (nAHistograms % 2)
    result["DataLinearAdj"] =
        "The number of histograms in the linear "
        "adjustments workspace must be even, because they are complex data";
  else if (nAHistograms > 0 && nAHistograms < nHistograms)
    result["DataLinearAdj"] =
        "The number of histograms in the linear "
        "adjustments workspace is insufficient for the input workspace";

  // Check constant adjustments, we expect and even number of histograms
  // and if any, they must be sufficient for all spectra in input workspace,
  // if per spectrum reconstruction is done.
  MatrixWorkspace_sptr constAdj = getProperty("DataConstAdj");
  nAHistograms = 0;
  if (constAdj)
    nAHistograms = constAdj->getNumberHistograms();
  if (nAHistograms % 2)
    result["DataConstAdj"] =
        "The number of histograms in the constant "
        "adjustments workspace must be even, because they are complex data";
  else if (nAHistograms > 0 && nAHistograms < nHistograms)
    result["DataConstAdj"] =
        "The number of histograms in the constant "
        "adjustments workspace is insufficient for the input workspace";

  return result;
}

//----------------------------------------------------------------------------------------------
/** Execute the algorithm.
 */
void MaxEnt::exec() {

  // MaxEnt parameters
  // Complex data?
  const bool complexData = getProperty("ComplexData");
  // Complex image?
  const bool complexImage = getProperty("ComplexImage");
  // Image must be positive?
  const bool positiveImage = getProperty("PositiveImage");
  // Autoshift
  const bool autoShift = getProperty("AutoShift");
  // Increase the number of points in the image by this factor
  const size_t resolutionFactor = getProperty("ResolutionFactor");
  // Background (default level, sky background, etc)
  const double background = getProperty("A");
  // Chi target
  const double ChiTargetOverN = getProperty("ChiTargetOverN");
  // Required precision for Chi arget
  const double chiEps = getProperty("ChiEps");
  // Maximum degree of non-parallelism between S and C
  const double angle = getProperty("MaxAngle");
  // Distance penalty for current image
  const double distEps = getProperty("DistancePenalty");
  // Maximum number of iterations
  const size_t nIter = getProperty("MaxIterations");
  // Maximum number of iterations in alpha chop
  const size_t alphaIter = getProperty("AlphaChopIterations");
  // Number of spectra and datapoints
  // Read input workspace
  MatrixWorkspace_const_sptr inWS = getProperty("InputWorkspace");
  // Number of spectra
  size_t nHist = inWS->getNumberHistograms();
  // Number of data points - assumed to be constant between spectra or
  // this will throw an exception
  size_t npoints = inWS->blocksize() * resolutionFactor;
  // Number of X bins
  const size_t npointsX = inWS->isHistogramData() ? npoints + 1 : npoints;
  // Linear adjustment of calculated data
  MatrixWorkspace_const_sptr dataLinearAdj = getProperty("DataLinearAdj");
  // Constant adjustment of calculated data
  MatrixWorkspace_const_sptr dataConstAdj = getProperty("DataConstAdj");
  // Add spectra in reconstruction if false
  const bool perSpectrumReconstruction =
      getProperty("PerSpectrumReconstruction");

  // For now have the requirement that data must have non-zero
  // (and positive!) errors
  for (size_t s = 0; s < nHist; s++) {
    const auto &errors = inWS->e(s);
    if (std::any_of(errors.cbegin(), errors.cend(),
                    [](const auto error) { return error <= 0.; })) {
      throw std::invalid_argument("Input data must have all errors non-zero.");
    }
  }

  // Is our data space real or complex?
  MaxentSpace_sptr dataSpace;
  if (complexData) {
    dataSpace = boost::make_shared<MaxentSpaceComplex>();
  } else {
    dataSpace = boost::make_shared<MaxentSpaceReal>();
  }
  // Is our image space real or complex?
  MaxentSpace_sptr imageSpace;
  if (complexImage) {
    imageSpace = boost::make_shared<MaxentSpaceComplex>();
  } else {
    imageSpace = boost::make_shared<MaxentSpaceReal>();
  }
  // The type of transform. Currently a 1D Fourier Transform or Multiple ID
  // Fourier transform
  MaxentTransform_sptr transform;
  if (perSpectrumReconstruction) {
    transform =
        boost::make_shared<MaxentTransformFourier>(dataSpace, imageSpace);
  } else {
    auto complexDataSpace = boost::make_shared<MaxentSpaceComplex>();
    transform = boost::make_shared<MaxentTransformMultiFourier>(
        complexDataSpace, imageSpace, nHist / 2);
  }

  // The type of entropy we are going to use (depends on the type of image,
  // positive only, or positive and/or negative)
  MaxentEntropy_sptr entropy;
  if (positiveImage) {
    entropy = boost::make_shared<MaxentEntropyPositiveValues>();
  } else {
    entropy = boost::make_shared<MaxentEntropyNegativeValues>();
  }

  // Entropy and transform is all we need to set up a calculator
  MaxentCalculator maxentCalculator = MaxentCalculator(entropy, transform);

  // Output workspaces
  MatrixWorkspace_sptr outImageWS;
  MatrixWorkspace_sptr outDataWS;
  MatrixWorkspace_sptr outEvolChi;
  MatrixWorkspace_sptr outEvolTest;

  size_t nDataSpec = complexData ? nHist / 2 : nHist;
  size_t nImageSpec = nDataSpec;
  size_t nSpecConcat = 1;
  if (!perSpectrumReconstruction) {
    nSpecConcat = nImageSpec;
    nImageSpec = 1;
  }
  outImageWS = create<MatrixWorkspace>(*inWS, 2 * nImageSpec, Points(npoints));
  for (size_t i = 0; i < outImageWS->getNumberHistograms(); ++i)
    outImageWS->getSpectrum(i).setDetectorID(static_cast<detid_t>(i + 1));
  HistogramBuilder builder;
  builder.setX(npointsX);
  builder.setY(npoints);
  builder.setDistribution(inWS->isDistribution());
  outDataWS = create<MatrixWorkspace>(*inWS, 2 * nDataSpec, builder.build());

  for (size_t i = 0; i < outDataWS->getNumberHistograms(); ++i)
    outDataWS->getSpectrum(i).setDetectorID(static_cast<detid_t>(i + 1));
  outEvolChi = create<MatrixWorkspace>(*inWS, nImageSpec, Points(nIter));
  outEvolTest = create<MatrixWorkspace>(*inWS, nImageSpec, Points(nIter));

  npoints = complexImage ? npoints * 2 : npoints;
  std::vector<size_t> iterationCounts;
  iterationCounts.reserve(nImageSpec);
  outEvolChi->setPoints(0, Points(nIter, LinearGenerator(0.0, 1.0)));

  size_t dataLength = complexData ? 2 * inWS->y(0).size() : inWS->y(0).size();
  dataLength *= nSpecConcat;

  for (size_t spec = 0; spec < nImageSpec; spec++) {

    // Start distribution (flat background)
    std::vector<double> image(npoints, background);

    std::vector<double> data(dataLength, 0.0);
    std::vector<double> errors(dataLength, 0.0);
    if (complexData) {
      data = toComplex(inWS, spec, false,
                       !perSpectrumReconstruction); // 3rd arg false -> data
      errors = toComplex(inWS, spec, true,
                         !perSpectrumReconstruction); // 3rd arg true -> errors
    } else {
      if (!perSpectrumReconstruction) {
        throw std::invalid_argument(
            "ComplexData must be true, if PerSpectrumReconstruction is false.");
      } else {
        data = inWS->y(spec).rawData();
        errors = inWS->e(spec).rawData();
      }
    }

    std::vector<double> linearAdjustments;
    std::vector<double> constAdjustments;
    if (dataLinearAdj) {
      linearAdjustments =
          toComplex(dataLinearAdj, spec, false, !perSpectrumReconstruction);
    }
    if (dataConstAdj) {
      constAdjustments =
          toComplex(dataConstAdj, spec, false, !perSpectrumReconstruction);
    }

    // To record the algorithm's progress
    std::vector<double> evolChi(nIter, 0.);
    std::vector<double> evolTest(nIter, 0.);

    // Progress
    Progress progress(this, 0.0, 1.0, nIter);

    // Run maxent algorithm
    bool converged = false;
    for (size_t it = 0; it < nIter; it++) {

      // Iterates one step towards the solution. This means calculating
      // quadratic coefficients, search directions, angle and chi-sq
      maxentCalculator.iterate(data, errors, image, background,
                               linearAdjustments, constAdjustments);

      // Calculate delta to construct new image (SB eq. 25)
      double currChisq = maxentCalculator.getChisq();
      auto coeffs = maxentCalculator.getQuadraticCoefficients();
      auto delta = move(coeffs, ChiTargetOverN / currChisq, chiEps, alphaIter);

      // Apply distance penalty (SB eq. 33)
      delta = applyDistancePenalty(delta, coeffs, image, background, distEps);

      // Update image
      auto dirs = maxentCalculator.getSearchDirections();
      image = updateImage(image, delta, dirs);

      // Record the evolution of Chi-square and angle(S,C)
      double currAngle = maxentCalculator.getAngle();
      evolChi[it] = currChisq;
      evolTest[it] = currAngle;

      // Stop condition for convergence, solution found
      if ((std::abs(currChisq / ChiTargetOverN - 1.) < chiEps) &&
          (currAngle < angle)) {

        // it + 1 iterations have been done because we count from zero
        g_log.information()
            << "Converged after " << it + 1 << " iterations" << std::endl;
        iterationCounts.push_back(it + 1);
        converged = true;
        break;
      }

      // Check for canceling the algorithm
      if (!(it % 1000)) {
        interruption_point();
      }

      progress.report();

    } // Next Iteration

    // If we didn't converge, we still need to record the number of iterations
    if (!converged) {
      iterationCounts.push_back(nIter);
    }

    // Get calculated data
    auto solData = maxentCalculator.getReconstructedData();
    auto solImage = maxentCalculator.getImage();

    // Populate the output workspaces
    populateDataWS(inWS, spec, nDataSpec, solData, !perSpectrumReconstruction,
                   complexData, outDataWS);
    populateImageWS(inWS, spec, nImageSpec, solImage, complexImage, outImageWS,
                    autoShift);

    // Populate workspaces recording the evolution of Chi and Test
    // X values
    outEvolChi->setSharedX(spec, outEvolChi->sharedX(0));
    outEvolTest->setSharedX(spec, outEvolChi->sharedX(0));

    // Y values
    outEvolChi->setCounts(spec, std::move(evolChi));
    outEvolTest->setCounts(spec, std::move(evolTest));
    // No errors

  } // Next spectrum
  setProperty("EvolChi",
              removeZeros(outEvolChi, iterationCounts, "Chi squared"));
  setProperty("EvolAngle",
              removeZeros(outEvolTest, iterationCounts, "Maximum Angle"));
  setProperty("ReconstructedImage", outImageWS);
  setProperty("ReconstructedData", outDataWS);
}

//----------------------------------------------------------------------------------------------

/** Returns a given spectrum or sum of spectra as a complex number
 * @param inWS :: [input] The input workspace containing all the spectra
 * @param spec :: [input] The spectrum of interest
 * @param errors :: [input] If true, returns the errors, otherwise returns the
 * counts
 * @param concatSpec :: [input] If true, use concatenation of all spectra
 * (ignoring spec)
 * @return : Spectrum 'spec' as a complex vector
 */
std::vector<double> MaxEnt::toComplex(API::MatrixWorkspace_const_sptr &inWS,
                                      size_t spec, bool errors,
                                      bool concatSpec) {
  const size_t numBins = inWS->y(0).size();
  size_t nSpec = inWS->getNumberHistograms() / 2;
  std::vector<double> result;
  result.reserve(2 * numBins);

  if (inWS->getNumberHistograms() % 2)
    throw std::invalid_argument(
        "Cannot convert input workspace to complex data");

  size_t nSpecOfInterest = (concatSpec ? nSpec : 1);
  size_t firstSpecOfInterest = (concatSpec ? 0 : spec);

  for (size_t s = firstSpecOfInterest;
       s < firstSpecOfInterest + nSpecOfInterest; s++) {
    if (!errors) {
      for (size_t i = 0; i < numBins; i++) {
        result.emplace_back(inWS->y(s)[i]);
        result.emplace_back(inWS->y(s + nSpec)[i]);
      }
    } else {
      for (size_t i = 0; i < numBins; i++) {
        result.emplace_back(inWS->e(s)[i]);
        result.emplace_back(inWS->e(s + nSpec)[i]);
      }
    }
  }

  return result;
}

/** Bisection method to move delta one step closer towards the solution
 * @param coeffs :: [input] The current quadratic coefficients
 * @param ChiTargetOverN :: [input] The requested Chi target over N
 * (data points)
 * @param chiEps :: [input] Precision required for Chi target
 * @param alphaIter :: [input] Maximum number of iterations in the bisection
 * method (alpha chop)
 * @return : The increment length to be added to the current image
 */
std::vector<double> MaxEnt::move(const QuadraticCoefficients &coeffs,
                                 double ChiTargetOverN, double chiEps,
                                 size_t alphaIter) {

  double aMin = 0.; // Minimum alpha
  double aMax = 1.; // Maximum alpha

  // Dimension, number of search directions
  size_t dim = coeffs.c2.size().first;

  std::vector<double> deltaMin(dim, 0); // delta at alpha min
  std::vector<double> deltaMax(dim, 0); // delta at alpha max

  double chiMin = calculateChi(coeffs, aMin, deltaMin); // Chi at alpha min
  double chiMax = calculateChi(coeffs, aMax, deltaMax); // Chi at alpha max

  double dchiMin = chiMin - ChiTargetOverN; // max - target
  double dchiMax = chiMax - ChiTargetOverN; // min - target

  if (dchiMin * dchiMax > 0) {
    // ChiTargetOverN could be outside the range [chiMin, chiMax]

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

  while ((fabs(eps / ChiTargetOverN) > chiEps) && (iter < alphaIter)) {

    double aMid = 0.5 * (aMin + aMax);
    double chiMid = calculateChi(coeffs, aMid, delta);

    eps = chiMid - ChiTargetOverN;

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
  if ((fabs(eps / ChiTargetOverN) > chiEps) || (iter > alphaIter)) {
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
std::vector<double> MaxEnt::solveSVD(DblMatrix &A, const DblMatrix &B) {

  size_t dim = A.size().first;

  auto a = gsl_matrix_view_array(A[0], dim, dim);
  auto b = gsl_vector_const_view_array(B[0], dim);

  std::vector<double> vVec(dim * dim), sVec(dim), wVec(dim), delta(dim);

  auto v = gsl_matrix_view_array(vVec.data(), dim, dim);
  auto s = gsl_vector_view_array(sVec.data(), dim);
  auto w = gsl_vector_view_array(wVec.data(), dim);
  auto x = gsl_vector_view_array(delta.data(), dim);

  // Singular value decomposition
  gsl_linalg_SV_decomp(&a.matrix, &v.matrix, &s.vector, &w.vector);

  // A could be singular or ill-conditioned. We can use SVD to obtain a least
  // squares solution by setting the small (compared to the maximum) singular
  // values to zero

  // Find largest sing value
  double max = *std::max_element(sVec.begin(), sVec.end());

  // Apply a threshold to small singular values
  double threshold = THRESHOLD * max;
  std::transform(sVec.begin(), sVec.end(), sVec.begin(),
                 [&threshold](double el) { return el > threshold ? el : 0.0; });

  // Solve A*x = B
  gsl_linalg_SV_solve(&a.matrix, &v.matrix, &s.vector, &b.vector, &x.vector);

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

  const double pointSum = std::accumulate(
      image.cbegin(), image.cend(), 0.,
      [](const auto sum, const auto point) { return sum + std::abs(point); });

  const size_t dim = coeffs.s2.size().first;

  double dist = 0.;

  for (size_t k = 0; k < dim; k++) {
    double sum = 0.0;
    for (size_t l = 0; l < dim; l++)
      sum -= coeffs.s2[k][l] * delta[l];
    dist += delta[k] * sum;
  }

  if (dist > distEps * pointSum / background) {
    auto newDelta = delta;
    for (size_t k = 0; k < delta.size(); k++) {
      newDelta[k] *= sqrt(distEps * pointSum / dist / background);
    }
    return newDelta;
  }
  return delta;
}

/**
 * Updates the image according to an increment delta
 * @param image : [input] The current image as a vector (can be real or complex)
 * @param delta : [input] The increment delta as a vector (can be real or
 * complex)
 * @param dirs : [input] The search directions
 * @return : The new image
 */
std::vector<double>
MaxEnt::updateImage(const std::vector<double> &image,
                    const std::vector<double> &delta,
                    const std::vector<std::vector<double>> dirs) {

  if (image.empty() || dirs.empty() || (delta.size() != dirs.size())) {
    throw std::runtime_error("Cannot calculate new image");
  }

  std::vector<double> newImage = image;

  // Calculate the new image
  for (size_t i = 0; i < image.size(); i++) {
    for (size_t k = 0; k < delta.size(); k++) {
      newImage[i] += delta[k] * dirs[k][i];
    }
  }
  return newImage;
}

/** Populates the image output workspace
 * @param inWS :: [input] The input workspace
 * @param spec :: [input] The current spectrum being analyzed
 * @param nspec :: [input] The total number of histograms in the input workspace
 * @param result :: [input] The image to be written in the output workspace (can
 * be real or complex vector)
 * @param complex :: [input] True if the result is a complex vector, false
 * otherwise
 * @param outWS :: [input] The output workspace to populate
 * @param autoShift :: [input] Whether or not to correct the phase shift
 */
void MaxEnt::populateImageWS(MatrixWorkspace_const_sptr &inWS, size_t spec,
                             size_t nspec, const std::vector<double> &result,
                             bool complex, MatrixWorkspace_sptr &outWS,
                             bool autoShift) {

  if (complex && result.size() % 2)
    throw std::invalid_argument(
        "Cannot write image results to output workspaces");

  int npoints = complex ? static_cast<int>(result.size() / 2)
                        : static_cast<int>(result.size());
  MantidVec X(npoints);
  MantidVec YR(npoints);
  MantidVec YI(npoints);
  MantidVec E(npoints, 0.);

  auto dataPoints = inWS->points(spec);
  double x0 = dataPoints[0];
  double dx = dataPoints[1] - x0;

  double delta = 1. / dx / npoints;
  const int isOdd = (inWS->y(0).size() % 2) ? 1 : 0;

  double shift = x0 * 2. * M_PI;
  if (!autoShift)
    shift = 0.;

  // X values
  for (int i = 0; i < npoints; i++) {
    X[i] = delta * (-npoints / 2 + i);
  }

  // Y values
  if (complex) {
    for (int i = 0; i < npoints; i++) {
      int j = (npoints / 2 + i + isOdd) % npoints;
      double xShift = X[i] * shift;
      double c = cos(xShift);
      double s = sin(xShift);
      YR[i] = result[2 * j] * c - result[2 * j + 1] * s;
      YI[i] = result[2 * j] * s + result[2 * j + 1] * c;
      YR[i] *= dx;
      YI[i] *= dx;
    }
  } else {
    for (int i = 0; i < npoints; i++) {
      int j = (npoints / 2 + i + isOdd) % npoints;
      double xShift = X[i] * shift;
      double c = cos(xShift);
      double s = sin(xShift);
      YR[i] = result[j] * c;
      YI[i] = result[j] * s;
      YR[i] *= dx;
      YI[i] *= dx;
    }
  }

  // X caption & label
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

  outWS->mutableX(spec) = std::move(X);
  outWS->mutableY(spec) = std::move(YR);
  outWS->mutableE(spec) = std::move(E);
  outWS->setSharedX(nspec + spec, outWS->sharedX(spec));
  outWS->mutableY(nspec + spec) = std::move(YI);
  outWS->setSharedE(nspec + spec, outWS->sharedE(spec));
}

/** Populates the data output workspace
 * @param inWS :: [input] The input workspace
 * @param spec :: [input] The current spectrum being analyzed
 * @param nspec :: [input] The total number of histograms in the input workspace
 * @param result :: [input] The reconstructed data to be written in the output
 * workspace (can be a real or complex vector)
 * @param complex :: [input] True if result is a complex vector, false otherwise
 * @param concatenated :: [input] True if result is concatenated spectra,
 * then all spectra are analyzed and spec must be 0.
 * @param outWS :: [input] The output workspace to populate
 */
void MaxEnt::populateDataWS(MatrixWorkspace_const_sptr &inWS, size_t spec,
                            size_t nspec, const std::vector<double> &result,
                            bool concatenated, bool complex,
                            MatrixWorkspace_sptr &outWS) {

  if (complex && result.size() % 2)
    throw std::invalid_argument(
        "Cannot write data results to output workspaces");
  if (concatenated && !complex)
    throw std::invalid_argument("Concatenated data results must be complex");
  if (concatenated && result.size() % (nspec * 2))
    throw std::invalid_argument(
        "Cannot write complex concatenated data results to output workspaces");
  if (concatenated && spec != 0)
    throw std::invalid_argument("Cannot write concatenated data results to "
                                "output workspaces from non-first spectrum");

  int resultLength = complex ? static_cast<int>(result.size() / 2)
                             : static_cast<int>(result.size());
  size_t spectrumLength = (concatenated ? resultLength / nspec : resultLength);
  size_t spectrumLengthX =
      inWS->isHistogramData() ? spectrumLength + 1 : spectrumLength;
  size_t nSpecAnalyzed = (concatenated ? nspec : 1);

  // Here we assume equal constant binning for all spectra analyzed
  double x0 = inWS->x(spec)[0];
  double dx = inWS->x(spec)[1] - x0;

  // Loop over each spectrum being analyzed - one spectrum unless concatenated
  for (size_t specA = spec; specA < spec + nSpecAnalyzed; specA++) {

    MantidVec X(spectrumLengthX);
    MantidVec YR(spectrumLength);
    MantidVec YI(spectrumLength);
    MantidVec E(spectrumLength, 0.);

    // X values
    for (size_t i = 0; i < spectrumLengthX; i++) {
      X[i] = x0 + static_cast<double>(i) * dx;
    }

    // Y values
    if (complex) {
      if (concatenated) {
        // note the spec=0, so specA starts from 0 in this case.
        for (size_t i = 0; i < spectrumLength; i++) {
          YR[i] = result[2 * i + 2 * specA * spectrumLength];
          YI[i] = result[2 * i + 1 + 2 * specA * spectrumLength];
        }
      } else {
        for (size_t i = 0; i < spectrumLength; i++) {
          YR[i] = result[2 * i];
          YI[i] = result[2 * i + 1];
        }
      }
    } else {
      for (size_t i = 0; i < spectrumLength; i++) {
        YR[i] = result[i];
        YI[i] = 0.;
      }
    }

    outWS->mutableX(specA) = std::move(X);
    outWS->mutableY(specA) = std::move(YR);
    outWS->mutableE(specA) = std::move(E);
    outWS->mutableY(nspec + specA) = std::move(YI);
    outWS->setSharedX(nspec + specA, outWS->sharedX(spec));
    outWS->setSharedE(nspec + specA, outWS->sharedE(spec));
  } // Next spectrum if concatenated
}

} // namespace Algorithms
} // namespace Mantid
