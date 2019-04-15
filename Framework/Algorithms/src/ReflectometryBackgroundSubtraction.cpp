// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +

#include "MantidAlgorithms/ReflectometryBackgroundSubtraction.h"
#include "MantidAPI/Algorithm.tcc"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidKernel/ArrayLengthValidator.h"
#include "MantidKernel/BoundedValidator.h"
#include "MantidKernel/EnabledWhenProperty.h"
#include "MantidKernel/ListValidator.h"

using namespace Mantid::API;
using namespace Mantid::Kernel;

namespace Mantid {
namespace Algorithms {

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(ReflectometryBackgroundSubtraction)

/// Algorithm's name for identification. @see Algorithm::name
const std::string ReflectometryBackgroundSubtraction::name() const {
  return "ReflectometryBackgroundSubtraction";
}

/// Algorithm's version for identification. @see Algorithm::version
int ReflectometryBackgroundSubtraction::version() const { return 1; }

/// Algorithm's category for identification. @see Algorithm::category
const std::string ReflectometryBackgroundSubtraction::category() const {
  return "Reflectometry;Reflectometry\\ISIS";
}

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
void ReflectometryBackgroundSubtraction::calculateAverageSpectrumBackground(
    MatrixWorkspace_sptr inputWS, std::vector<specnum_t> spectraList) {

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
std::vector<double> ReflectometryBackgroundSubtraction::findSpectrumRanges(
    std::vector<specnum_t> spectraList) {

  std::vector<double> spectrumRanges;
  spectrumRanges.push_back(spectraList[0]);
  auto prevSpec = spectrumRanges[0];
  for (size_t index = 0; index < spectraList.size() - 1; ++index) {
    auto spec = spectraList[index + 1];
    auto range = spec - prevSpec;
    // check if start of new range
    if (range > 1) {
      spectrumRanges.push_back(prevSpec);
      spectrumRanges.push_back(spec);
    }
    prevSpec = spec;
  }
  spectrumRanges.push_back(spectraList.back());
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
void ReflectometryBackgroundSubtraction::calculatePolynomialBackground(
    MatrixWorkspace_sptr inputWS, std::vector<double> spectrumRanges) {

  // if the input workspace is an event workspace it must be converted to a
  // Matrix workspace as cannot transpose an event workspace

  DataObjects::EventWorkspace_const_sptr eventW =
      boost::dynamic_pointer_cast<const DataObjects::EventWorkspace>(inputWS);
  MatrixWorkspace_sptr outputWorkspace;
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
 * side of the peak. This is done using the child algorithm
 * LRSubtractAverageBackground. The background is then subtracted from the input
 * workspace.
 *
 * @param inputWS :: the input workspace
 */
void ReflectometryBackgroundSubtraction::subtractPixelBackground(
    MatrixWorkspace_sptr inputWS, std::vector<double> spectrumRanges) {

  MatrixWorkspace_sptr outputWS = inputWS->clone();
  AnalysisDataService::Instance().addOrReplace(
      getPropertyValue("OutputWorkspace"), outputWS);

  auto LRBgd = createChildAlgorithm("LRSubtractAverageBackground");
  LRBgd->initialize();
  LRBgd->setProperty("InputWorkspace", outputWS);
  LRBgd->setProperty("LowResolutionRange", getPropertyValue("YRange"));
  LRBgd->setProperty("PeakRange", getPropertyValue("PeakRange"));
  LRBgd->setProperty("BackgroundRange", getPropertyValue("BackgroundRange"));
  LRBgd->setProperty("SumPeak", getPropertyValue("SumPeak"));
  LRBgd->execute();
}

//----------------------------------------------------------------------------------------------
/** Initialize the algorithm's properties.
 */
void ReflectometryBackgroundSubtraction::init() {

  // Input workspace
  declareWorkspaceInputProperties<
      MatrixWorkspace, IndexType::SpectrumNum | IndexType::WorkspaceIndex>(
      "InputWorkspace", "An input workspace",
      boost::make_shared<CommonBinsValidator>());

  std::vector<std::string> backgroundTypes = {
      "Per Detector Average", "Polynomial", "Average Pixel Fit"};
  declareProperty("BackgroundCalculationMethod", "Per Detector Average",
                  boost::make_shared<StringListValidator>(backgroundTypes),
                  "The type of background reduction to perform.",
                  Direction::Input);

  // polynomial properties
  auto nonnegativeInt = boost::make_shared<BoundedValidator<int>>();
  nonnegativeInt->setLower(0);
  declareProperty("DegreeOfPolynomial", 0, nonnegativeInt,
                  "Degree of the fitted polynomial.");
  std::array<std::string, 2> costFuncOpts{
      {"Least squares", "Unweighted least squares"}};
  declareProperty("CostFunction", "Least squares",
                  boost::make_shared<ListValidator<std::string>>(costFuncOpts),
                  "The cost function to be passed to the Fit algorithm.");

  setPropertySettings("DegreeOfPolynomial", make_unique<EnabledWhenProperty>(
                                                "BackgroundCalculationMethod",
                                                IS_EQUAL_TO, "Polynomial"));
  setPropertySettings("CostFunction", make_unique<EnabledWhenProperty>(
                                          "BackgroundCalculationMethod",
                                          IS_EQUAL_TO, "Polynomial"));

  // Average pixel properties

  auto lengthArray = boost::make_shared<ArrayLengthValidator<int>>(2);
  declareProperty(
      make_unique<ArrayProperty<int>>("PeakRange", "0,0", lengthArray),
      "Pixel range defining the reflectivity peak");
  declareProperty(
      make_unique<ArrayProperty<int>>("BackgroundRange", "0,0", lengthArray),
      "Pixel range defining the outer range of the background on each side of "
      "the peak");
  declareProperty(
      make_unique<ArrayProperty<int>>("YRange", "0,0", lengthArray),
      "Pixel range defining the low-resolution axis to integrate over");
  declareProperty("SumPeak", false,
                  "If True, the resulting peak will be summed");

  setPropertySettings("YRange", make_unique<EnabledWhenProperty>(
                                    "BackgroundCalculationMethod", IS_EQUAL_TO,
                                    "Average Pixel Fit"));

  // Output workspace
  declareProperty(make_unique<WorkspaceProperty<>>("OutputWorkspace", "",
                                                   Direction::Output,
                                                   PropertyMode::Optional),
                  "The output Workspace containing either the background or "
                  "the InputWorkspace with the background removed.");
}

//----------------------------------------------------------------------------------------------
/** Execute the algorithm.
 */
void ReflectometryBackgroundSubtraction::exec() {
  MatrixWorkspace_sptr inputWS;
  Indexing::SpectrumIndexSet indexSet;
  std::tie(inputWS, indexSet) =
      getWorkspaceAndIndices<MatrixWorkspace>("InputWorkspace");
  const std::string backgroundType = getProperty("BackgroundCalculationMethod");

  // Set default outputWorkspace name to be inputWorkspace Name
  const std::string wsName = inputWS->getName();
  if (isDefault("OutputWorkspace")) {
    setPropertyValue("OutputWorkspace", wsName);
  }

  std::vector<specnum_t> spectraList;
  for (auto index : indexSet) {
    auto &spec = inputWS->getSpectrum(index);
    spectraList.push_back(spec.getSpectrumNo());
  }

  if (backgroundType == "Per Detector Average") {
    calculateAverageSpectrumBackground(inputWS, spectraList);
  }

  if (backgroundType == "Polynomial") {
    auto spectrumRanges = findSpectrumRanges(spectraList);
    calculatePolynomialBackground(inputWS, spectrumRanges);
  }

  if (backgroundType == "Average Pixel Fit") {
    auto spectrumRanges = findSpectrumRanges(spectraList);
    subtractPixelBackground(inputWS, spectrumRanges);
  }
}

std::map<std::string, std::string>
ReflectometryBackgroundSubtraction::validateInputs() {

  std::map<std::string, std::string> errors;

  MatrixWorkspace_const_sptr inputWS;
  Indexing::SpectrumIndexSet indexSet;
  std::tie(inputWS, indexSet) =
      getWorkspaceAndIndices<MatrixWorkspace>("InputWorkspace");
  const std::string backgroundType = getProperty("BackgroundCalculationMethod");

  if (inputWS) {
    if (backgroundType == "Polynomial" && indexSet.size() == 1) {
      errors["InputWorkspaceIndexSet"] = "Input workspace index set must "
                                         "contain a more than one spectra for "
                                         "polynomial background subtraction";
    }

    if (backgroundType == "Average Pixel Fit" && indexSet.size() == 1) {
      errors["InputWorkspaceIndexSet"] = "Input workspace index set must "
                                         "contain a more than one spectra for "
                                         "polynomial background subtraction";
    }
  }
  return errors;
}
} // namespace Algorithms
} // namespace Mantid
