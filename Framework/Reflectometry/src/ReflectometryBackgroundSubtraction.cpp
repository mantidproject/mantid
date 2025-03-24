// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidReflectometry/ReflectometryBackgroundSubtraction.h"
#include "MantidAPI/Algorithm.hxx"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidKernel/ArrayLengthValidator.h"
#include "MantidKernel/BoundedValidator.h"
#include "MantidKernel/EnabledWhenProperty.h"
#include "MantidKernel/ListValidator.h"
#include "MantidKernel/Strings.h"

using namespace Mantid::API;
using namespace Mantid::Kernel;

namespace Mantid::Reflectometry {

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(ReflectometryBackgroundSubtraction)

/// Algorithm's name for identification. @see Algorithm::name
const std::string ReflectometryBackgroundSubtraction::name() const { return "ReflectometryBackgroundSubtraction"; }

/// Algorithm's version for identification. @see Algorithm::version
int ReflectometryBackgroundSubtraction::version() const { return 1; }

/// Algorithm's category for identification. @see Algorithm::category
const std::string ReflectometryBackgroundSubtraction::category() const { return "Reflectometry;Reflectometry\\ISIS"; }

/// Algorithm's summary for use in the GUI and help. @see Algorithm::summary
const std::string ReflectometryBackgroundSubtraction::summary() const {
  return "Calculates and subtracts the background from a given workspace.";
}

/** Calculates the background by finding the average of the given spectra using
 * the child algorithm GroupDetectors. The background is then subtracted from
 * the input workspace.
 *
 * @param inputWS :: the input workspace
 * @param spectraList :: a vector containing all of the spectra in the
 * background
 */
void ReflectometryBackgroundSubtraction::calculateAverageSpectrumBackground(const MatrixWorkspace_sptr &inputWS,
                                                                            const std::vector<specnum_t> &spectraList) {

  auto alg = this->createChildAlgorithm("GroupDetectors");
  alg->setProperty("InputWorkspace", inputWS);
  alg->setProperty("SpectraList", spectraList);
  alg->setProperty("Behaviour", "Average");
  alg->execute();
  MatrixWorkspace_sptr outputWS = alg->getProperty("OutputWorkspace");

  auto subtract = createChildAlgorithm("Minus");
  subtract->setProperty("LHSWorkspace", inputWS);
  subtract->setProperty("RHSWorkspace", outputWS);
  subtract->setProperty("AllowDifferentNumberSpectra", true);
  subtract->execute();
  outputWS = subtract->getProperty("OutputWorkspace");

  setProperty("OutputWorkspace", outputWS);
}

/** Returns the ranges of spectra in the given list.
 *
 * @param spectraList :: a vector containing a list of spectra
 * @return a vector containing a list of ranges of the given spectraList.
 */
std::vector<double> ReflectometryBackgroundSubtraction::findSpectrumRanges(const std::vector<specnum_t> &spectraList) {

  std::vector<double> spectrumRanges;
  spectrumRanges.emplace_back(spectraList[0]);
  auto prevSpec = spectrumRanges[0];
  for (size_t index = 0; index < spectraList.size() - 1; ++index) {
    auto spec = spectraList[index + 1];
    auto range = spec - prevSpec;
    // check if start of new range
    if (range > 1) {
      spectrumRanges.emplace_back(prevSpec);
      spectrumRanges.emplace_back(spec);
    }
    prevSpec = spec;
  }
  spectrumRanges.emplace_back(spectraList.back());
  return spectrumRanges;
}

/** Calculates the background by fitting a polynomial to each TOF. This is done
 * using the child algorithm CalculatePolynomialBackground. The background is
 * then subtracted from the input workspace.
 *
 * @param inputWS :: the input workspace
 * @param spectrumRanges :: a vector containing the ranges of spectra containing
 * the background
 */
void ReflectometryBackgroundSubtraction::calculatePolynomialBackground(MatrixWorkspace_sptr inputWS,
                                                                       const std::vector<double> &spectrumRanges) {

  // if the input workspace is an event workspace it must be converted to a
  // Matrix workspace as cannot transpose an event workspace

  DataObjects::EventWorkspace_const_sptr eventW = std::dynamic_pointer_cast<const DataObjects::EventWorkspace>(inputWS);
  if (eventW) {
    auto convert = createChildAlgorithm("ConvertToMatrixWorkspace");
    convert->setProperty("InputWorkspace", inputWS);
    convert->execute();
    inputWS = convert->getProperty("OutputWorkspace");
  }

  // To use CalculatePolynomialBackground to fit a polynomial to each TOF we
  // require the spectrum numbers in the x axis. So transpose is used to give
  // spectrum numbers in the horizontal axis and TOF channels in the vertical
  // axis.
  auto transpose = createChildAlgorithm("Transpose");
  transpose->setProperty("InputWorkspace", inputWS);
  transpose->execute();
  MatrixWorkspace_sptr transposedWS = transpose->getProperty("OutputWorkspace");

  auto poly = createChildAlgorithm("CalculatePolynomialBackground");
  poly->initialize();
  poly->setProperty("InputWorkspace", transposedWS);
  poly->setProperty("Degree", getPropertyValue("DegreeOfPolynomial"));
  poly->setProperty("XRanges", spectrumRanges);
  poly->setProperty("CostFunction", getPropertyValue("CostFunction"));
  poly->setProperty("Minimizer", "Levenberg-Marquardt");
  poly->execute();
  MatrixWorkspace_sptr bgd = poly->getProperty("OutputWorkspace");

  // the background must be transposed again to get it in the same form as the
  // input workspace
  transpose->setProperty("InputWorkspace", bgd);
  transpose->execute();
  MatrixWorkspace_sptr outputWS = transpose->getProperty("OutputWorkspace");

  outputWS = minus(inputWS, outputWS);

  setProperty("OutputWorkspace", outputWS);
}

/** Calculates the background by finding an average of the number of pixels each
 * side of the peak. This is done using the python LRSubtractAverageBackground.
 * The background is then subtracted from the input workspace.
 *
 * @param inputWS :: the input workspace
 * @param indexList :: the ranges of the background region
 */
void ReflectometryBackgroundSubtraction::calculatePixelBackground(const MatrixWorkspace_sptr &inputWS,
                                                                  const std::vector<double> &indexList) {

  const std::vector<int> backgroundRange{static_cast<int>(indexList.front()), static_cast<int>(indexList.back())};

  auto peakRangeProp = dynamic_cast<IndexProperty *>(getPointerToProperty("PeakRange"));
  Indexing::SpectrumIndexSet peakRangeIndexSet = *peakRangeProp;
  const std::vector<int> peakRange{static_cast<int>(peakRangeIndexSet[0]),
                                   static_cast<int>(peakRangeIndexSet[peakRangeIndexSet.size() - 1])};

  auto LRBgd = createChildAlgorithm("LRSubtractAverageBackground");
  LRBgd->initialize();
  LRBgd->setProperty("InputWorkspace", inputWS);
  LRBgd->setProperty("PeakRange", Strings::toString(peakRange));
  LRBgd->setProperty("BackgroundRange", Strings::toString(backgroundRange));
  LRBgd->setProperty("SumPeak", getPropertyValue("SumPeak"));
  // the low resolution range is 0 as it is assumed to be linear detector, this
  // will need to change if ISIS reflectometry get a 2D detector
  LRBgd->setProperty("LowResolutionRange", "0,0");
  LRBgd->setProperty("TypeOfDetector", "LinearDetector");
  LRBgd->setProperty("ErrorWeighting", true);
  LRBgd->execute();

  Workspace_sptr outputWS = LRBgd->getProperty("OutputWorkspace");
  setProperty("OutputWorkspace", outputWS);
}

//----------------------------------------------------------------------------------------------
/** Initialize the algorithm's properties.
 */
void ReflectometryBackgroundSubtraction::init() {

  // Input workspace
  auto inputWSProp = std::make_unique<WorkspaceProperty<MatrixWorkspace>>(
      "InputWorkspace", "An input workspace", Direction::Input, std::make_shared<CommonBinsValidator>());
  const auto &inputWSPropRef = *inputWSProp;
  declareProperty(std::move(inputWSProp), "An input workspace.");

  auto inputIndexType =
      std::make_unique<IndexTypeProperty>("InputWorkspaceIndexType", static_cast<int>(IndexType::SpectrumNum) |
                                                                         static_cast<int>(IndexType::WorkspaceIndex));
  const auto &inputIndexTypeRef = *inputIndexType;
  declareProperty(std::move(inputIndexType), "The type of indices in the optional index set; For optimal "
                                             "performance WorkspaceIndex should be preferred;");

  declareProperty(std::make_unique<IndexProperty>("ProcessingInstructions", inputWSPropRef, inputIndexTypeRef),
                  "An optional set of spectra containing the background. If "
                  "not set all spectra will be processed. The indices in this "
                  "list can be workspace indices or spectrum numbers, "
                  "depending on the selection made for the index type; Indices "
                  "are entered as a comma-separated list of values, and/or "
                  "ranges.");

  std::vector<std::string> backgroundTypes = {"PerDetectorAverage", "Polynomial", "AveragePixelFit"};
  declareProperty("BackgroundCalculationMethod", "PerDetectorAverage",
                  std::make_shared<StringListValidator>(backgroundTypes),
                  "The type of background reduction to perform.", Direction::Input);

  // polynomial properties
  auto nonnegativeInt = std::make_shared<BoundedValidator<int>>();
  nonnegativeInt->setLower(0);
  declareProperty("DegreeOfPolynomial", 0, nonnegativeInt, "Degree of the fitted polynomial.");
  std::array<std::string, 2> costFuncOpts{{"Least squares", "Unweighted least squares"}};
  declareProperty("CostFunction", "Least squares", std::make_shared<ListValidator<std::string>>(costFuncOpts),
                  "The cost function to be passed to the Fit algorithm.");

  setPropertySettings("DegreeOfPolynomial",
                      std::make_unique<EnabledWhenProperty>("BackgroundCalculationMethod", IS_EQUAL_TO, "Polynomial"));
  setPropertySettings("CostFunction",
                      std::make_unique<EnabledWhenProperty>("BackgroundCalculationMethod", IS_EQUAL_TO, "Polynomial"));

  // Average pixel properties
  declareProperty(std::make_unique<IndexProperty>("PeakRange", inputWSPropRef, inputIndexTypeRef),
                  "A set of spectra defining the reflectivity peak. If not set all spectra "
                  "will be processed. The indices in this list can be workspace indices or "
                  "spectrum numbers, depending on the InputWorkspaceIndexType");

  declareProperty("SumPeak", false, "If True, the resulting peak will be summed");

  setPropertySettings("PeakRange", std::make_unique<EnabledWhenProperty>("BackgroundCalculationMethod", IS_EQUAL_TO,
                                                                         "AveragePixelFit"));

  setPropertySettings(
      "SumPeak", std::make_unique<EnabledWhenProperty>("BackgroundCalculationMethod", IS_EQUAL_TO, "AveragePixelFit"));

  // Output workspace
  declareProperty(
      std::make_unique<WorkspaceProperty<>>("OutputWorkspace", "", Direction::Output, PropertyMode::Optional),
      "The output workspace containing the InputWorkspace with the "
      "background removed.");
}

//----------------------------------------------------------------------------------------------
/** Execute the algorithm.
 */
void ReflectometryBackgroundSubtraction::exec() {
  MatrixWorkspace_sptr inputWS = getProperty("InputWorkspace");
  auto indexProp = dynamic_cast<IndexProperty *>(getPointerToProperty("ProcessingInstructions"));
  Indexing::SpectrumIndexSet indexSet = *indexProp;
  const std::string backgroundType = getProperty("BackgroundCalculationMethod");

  // Set default outputWorkspace name to be inputWorkspace Name
  const std::string wsName = inputWS->getName();
  if (isDefault("OutputWorkspace")) {
    setPropertyValue("OutputWorkspace", wsName);
  }

  std::vector<double> indexList;
  std::vector<specnum_t> spectraList;
  for (auto index : indexSet) {
    auto &spec = inputWS->getSpectrum(index);
    spectraList.emplace_back(spec.getSpectrumNo());
    indexList.emplace_back(static_cast<double>(index));
  }

  if (backgroundType == "PerDetectorAverage") {
    calculateAverageSpectrumBackground(inputWS, spectraList);
  }

  if (backgroundType == "Polynomial") {
    auto range = static_cast<int>(spectraList.back() - spectraList.front());
    int degree = getProperty("degreeOfPolynomial");
    if (range < degree) {
      throw std::invalid_argument("Cannot fit polynomial, number of data points in region < "
                                  "the number of fitting parameters: " +
                                  std::to_string(range + 1) + " < " + std::to_string(degree + 1));
    }
    auto spectrumRanges = findSpectrumRanges(spectraList);
    calculatePolynomialBackground(inputWS, spectrumRanges);
  }

  if (backgroundType == "AveragePixelFit") {
    calculatePixelBackground(inputWS, indexList);
  }
}

std::map<std::string, std::string> ReflectometryBackgroundSubtraction::validateInputs() {

  std::map<std::string, std::string> errors;

  MatrixWorkspace_const_sptr inputWS = getProperty("InputWorkspace");
  auto indexProp = dynamic_cast<IndexProperty *>(getPointerToProperty("ProcessingInstructions"));
  Indexing::SpectrumIndexSet indexSet;
  try {
    indexSet = *indexProp;
  } catch (std::runtime_error const &e) {
    // This can except unhandled when the input workspace is the wrong type. So just add an error instead.
    errors["InputWorkspace"] = e.what();
  }
  const std::string backgroundType = getProperty("BackgroundCalculationMethod");

  if (inputWS) {
    if (backgroundType == "Polynomial" && indexSet.size() == 1) {
      errors["ProcessingInstructions"] = "Input workspace index set must "
                                         "contain a more than one spectra for "
                                         "polynomial background subtraction";
    }

    if (backgroundType == "AveragePixelFit") {
      if (indexSet.size() == 1) {
        errors["ProcessingInstructions"] = "Input workspace index set must "
                                           "contain a more than one spectra for "
                                           "AveragePixelFit background subtraction";
      }

      auto peakRangeProp = dynamic_cast<IndexProperty *>(getPointerToProperty("PeakRange"));
      Indexing::SpectrumIndexSet peakRangeSet = *peakRangeProp;
      if (!peakRangeSet.isContiguous()) {
        errors["PeakRange"] = "PeakRange must be a single range";
      }
    }
  }
  return errors;
}
} // namespace Mantid::Reflectometry
