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

/** Returns the indexNumbers of the ranges of spectra in the given list.
 *
 * @param indexList :: a vector containing an list of all the index numbers of the histograms
 * @return a vector containing a list of ranges of the given indexList.
 */
std::vector<double> ReflectometryBackgroundSubtraction::findIndexRanges(
    std::vector<double> indexList) {

  std::vector<double> indexRanges;
  indexRanges.push_back(indexList[0]);
  auto prevSpec = indexRanges[0];
  for (auto index = 0; index < indexList.size() - 1; ++index) {
    auto spec = indexList[index + 1];
    auto range = spec - prevSpec;
    // check if start of new range
    if (range > 1) {
      indexRanges.push_back(prevSpec);
      indexRanges.push_back(spec);
    }
    prevSpec = spec;
  }
  indexRanges.push_back(indexList.back());
  return indexRanges;
}

/** Calculates the background by fitting a polynomial to each TOF. This is done
 * using the child algorithm CalculatePolynomialBackground. The background is
 * then subtracted from the input workspace.
 *
 * @param inputWS :: the input workspace
 * @param indexRanges :: a vector containing the index of the ranges of histograms containing
 * the background
 */
void ReflectometryBackgroundSubtraction::calculatePolynomialBackground(
    MatrixWorkspace_sptr inputWS, std::vector<double> indexRanges) {

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
  poly->setProperty("XRanges", indexRanges);
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
 * @param indexRanges :: the ranges of the background region
 */
void ReflectometryBackgroundSubtraction::subtractPixelBackground(
    MatrixWorkspace_sptr inputWS, std::vector<double> indexRanges) {

  // this is needed becuase the algorithm LRSubtractAverageBackground replaces
  // the InputWorkspace with the OutputWorkspace
  MatrixWorkspace_sptr outputWS = inputWS->clone();
  AnalysisDataService::Instance().addOrReplace(
      getPropertyValue("OutputWorkspace"), outputWS);

  const std::string backgroundRange =
      std::to_string(static_cast<int>(indexRanges.front())) + "," +
      std::to_string(static_cast<int>(indexRanges.back()));

  auto LRBgd = createChildAlgorithm("LRSubtractAverageBackground");
  LRBgd->initialize();
  LRBgd->setProperty("InputWorkspace", outputWS);
  LRBgd->setProperty("LowResolutionRange",
                     getPropertyValue("IntegrationRange"));
  LRBgd->setProperty("PeakRange", getPropertyValue("PeakRange"));
  LRBgd->setProperty("BackgroundRange", backgroundRange);
  LRBgd->setProperty("SumPeak", getPropertyValue("SumPeak"));
  LRBgd->setProperty("TypeOfDetector", "LinearDetector");
  LRBgd->execute();

	//Can only run once as doesn't override output!!!!! TODO::fix this
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
      "PerDetectorAverage", "Polynomial", "AveragePixelFit"};
  declareProperty("BackgroundCalculationMethod", "PerDetectorAverage",
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
      make_unique<ArrayProperty<int>>("PeakRange", "147, 163", lengthArray),
      "Pixel range defining the reflectivity peak");
  declareProperty(make_unique<ArrayProperty<int>>("IntegrationRange", "94, 160",
                                                  lengthArray),
                  "Pixel range defining the axis to integrate over");
  declareProperty("SumPeak", false,
                  "If True, the resulting peak will be summed");

  setPropertySettings("PeakRange", make_unique<EnabledWhenProperty>(
                                       "BackgroundCalculationMethod",
                                       IS_EQUAL_TO, "AveragePixelFit"));

  setPropertySettings(
      "IntegrationRange",
      make_unique<EnabledWhenProperty>("BackgroundCalculationMethod",
                                       IS_EQUAL_TO, "AveragePixelFit"));

  setPropertySettings("SumPeak", make_unique<EnabledWhenProperty>(
                                     "BackgroundCalculationMethod", IS_EQUAL_TO,
                                     "AveragePixelFit"));

  // Output workspace
  declareProperty(make_unique<WorkspaceProperty<>>("OutputWorkspace", "",
                                                   Direction::Output,
                                                   PropertyMode::Optional),
                  "The output workspace containing the InputWorkspace with the "
                  "background removed.");
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

  std::vector<double> indexList;
  std::vector<specnum_t> spectraList;
  for (auto index : indexSet) {
    auto &spec = inputWS->getSpectrum(index);
    spectraList.push_back(spec.getSpectrumNo());
    indexList.push_back(static_cast<double>(index));
  }

  if (backgroundType == "PerDetectorAverage") {
    calculateAverageSpectrumBackground(inputWS, spectraList);
  }

  if (backgroundType == "Polynomial") {
    auto indexRanges = findIndexRanges(indexList);
    calculatePolynomialBackground(inputWS, indexRanges);
  }

  if (backgroundType == "AveragePixelFit") {
    auto indexRanges = findIndexRanges(indexList);
    subtractPixelBackground(inputWS, indexRanges);
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
  const std::vector<int> peakRange = getProperty("PeakRange");

  if (inputWS) {
    if (backgroundType == "Polynomial" && indexSet.size() == 1) {
      errors["InputWorkspaceIndexSet"] = "Input workspace index set must "
                                         "contain a more than one spectra for "
                                         "polynomial background subtraction";
    }

    if (backgroundType == "AveragePixelFit" && indexSet.size() == 1) {
      errors["InputWorkspaceIndexSet"] =
          "Input workspace index set must "
          "contain a more than one spectra for "
          "AveragePixelFit background subtraction";
    }
    auto numberOfypixels = inputWS->getNumberHistograms();
    if (backgroundType == "AveragePixelFit" &&
        (peakRange.front() < 0 || peakRange.back() > numberOfypixels - 1)) {
      errors["PeakRange"] = "PeakRange must be contained within the number of pixels";
    }
  }
  return errors;
}
} // namespace Algorithms
} // namespace Mantid
