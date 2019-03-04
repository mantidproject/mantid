// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +

#include "MantidAlgorithms/ReflectometryBackgroundSubtraction.h"
#include "MantidAPI/CommonBinsValidator.h"
#include "MantidAPI/IncreasingAxisValidator.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidKernel/ArrayLengthValidator.h"
#include "MantidKernel/ArrayOrderedPairsValidator.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/BoundedValidator.h"
#include "MantidKernel/DynamicFactory.h"
#include "MantidKernel/EnabledWhenProperty.h"
#include "MantidKernel/EnabledWhenProperty.h"
#include "MantidKernel/ListValidator.h"

using namespace Mantid::API;
using namespace Mantid::Kernel;

namespace Mantid {
namespace Algorithms {

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
  return "";
}

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(ReflectometryBackgroundSubtraction)

void ReflectometryBackgroundSubtraction::calculateAverageSpectrumBackground(
    API::MatrixWorkspace_sptr inputWS) {

  auto alg = createChildAlgorithm("AverageSpectrumBackground");
  alg->initialize();
  alg->setProperty("InputWorkspace", inputWS);
  alg->setProperty("BottomBackgroundRange",
                   getPropertyValue("BottomBackgroundRange"));
  alg->setProperty("TopBackgroundRange",
                   getPropertyValue("TopBackgroundRange"));
  alg->execute();
  API::MatrixWorkspace_sptr output = alg->getProperty("OutputWorkspace");
  setProperty("OutputWorkspace", output);
}

void ReflectometryBackgroundSubtraction::calculatePolynomialBackground(
    API::MatrixWorkspace_sptr inputWS) {
  // if the input workspace is an event workspace it must be converted to a
  // Matrix workspace as cannot transpose a event workspace
  // if it is already a MatrixWorkspace this does nothing
  auto convert = createChildAlgorithm("ConvertToMatrixWorkspace");
  convert->setProperty("InputWorkspace", inputWS);
  convert->execute();
  API::MatrixWorkspace_sptr convertedWS =
      convert->getProperty("OutputWorkspace");

  // transpose the workspace before the background can be found
  auto transpose = createChildAlgorithm("Transpose");
  transpose->setProperty("InputWorkspace", convertedWS);
  transpose->execute();
  API::MatrixWorkspace_sptr transposedWS =
      transpose->getProperty("OutputWorkspace");

  auto poly = createChildAlgorithm("CalculatePolynomialBackground");
  poly->initialize();
  poly->setProperty("InputWorkspace", transposedWS);
  poly->setProperty("Degree", getPropertyValue("DegreeOfPolynomial"));
  poly->setProperty("XRanges", getPropertyValue("XRanges"));
  poly->setProperty("CostFunction", getPropertyValue("CostFunction"));
  poly->execute();
  API::MatrixWorkspace_sptr bgd = poly->getProperty("OutputWorkspace");

  // transpose again to get the background
  transpose->setProperty("InputWorkspace", bgd);
  transpose->execute();
  API::MatrixWorkspace_sptr transposedBgd =
      transpose->getProperty("OutputWorkspace");

  // subtract the background from the input workspace
  auto output = minus(inputWS, transposedBgd);
  setProperty("OutputWorkspace", output);
}

//----------------------------------------------------------------------------------------------
/** Initialize the algorithm's properties.
 */
void ReflectometryBackgroundSubtraction::init() {

  // Input workspace
  declareProperty(make_unique<WorkspaceProperty<MatrixWorkspace>>(
                      "InputWorkspace", "", Direction::Input,
                      boost::make_shared<CommonBinsValidator>()),
                  "An input workspace.");

  std::vector<std::string> backgroundTypes = {"Per Spectra Average",
                                              "Polynomial"};
  declareProperty("TypeOfBackgroundSubtraction", "Per Spectra Average",
                  boost::make_shared<StringListValidator>(backgroundTypes),
                  "The type of background reduction to perform.",
                  Direction::Input);

  declareProperty(make_unique<ArrayProperty<size_t>>("BottomBackgroundRange",
                                                     Direction::Input),
                  "A list of the bottom background ranges.");
  declareProperty(make_unique<ArrayProperty<size_t>>("TopBackgroundRange",
                                                     Direction::Input),
                  "A list of the top background ranges.");

  setPropertyGroup("BottomBackgroundRange",
                   "Average Per Spectra Background Subtraction");
  setPropertyGroup("TopBackgroundRange",
                   "Average Per Spectra Background Subtraction");
  setPropertySettings(
      "BottomBackgroundRange",
      make_unique<EnabledWhenProperty>("TypeOfBackgroundSubtraction",
                                       IS_EQUAL_TO, "Per Spectra Average"));
  setPropertySettings(
      "TopBackgroundRange",
      make_unique<EnabledWhenProperty>("TypeOfBackgroundSubtraction",
                                       IS_EQUAL_TO, "Per Spectra Average"));

  auto increasingAxis = boost::make_shared<IncreasingAxisValidator>();
  auto nonnegativeInt = boost::make_shared<BoundedValidator<int>>();
  nonnegativeInt->setLower(0);
  auto orderedPairs = boost::make_shared<ArrayOrderedPairsValidator<double>>();
  declareProperty("DegreeOfPolynomial", 0, nonnegativeInt,
                  "Degree of the fitted polynomial.");
  declareProperty(make_unique<ArrayProperty<double>>(
                      "XRanges", std::vector<double>(), orderedPairs),
                  "A list of fitting ranges given as pairs of X values.");
  std::array<std::string, 2> costFuncOpts{
      {"Least squares", "Unweighted least squares"}};
  declareProperty("CostFunction", "Least squares",
                  boost::make_shared<ListValidator<std::string>>(costFuncOpts),
                  "The cost function to be passed to the Fit algorithm.");

  setPropertyGroup("DegreeOfPolynomial", "Polynomial Background Subtraction");
  setPropertyGroup("XRanges", "Polynomial Background Subtraction");
  setPropertyGroup("CostFunction", "Polynomial Background Subtraction");

  setPropertySettings("DegreeOfPolynomial", make_unique<EnabledWhenProperty>(
                                                "TypeOfBackgroundSubtraction",
                                                IS_EQUAL_TO, "Polynomial"));
  setPropertySettings(
      "XRanges", make_unique<EnabledWhenProperty>("TypeOfBackgroundSubtraction",
                                                  IS_EQUAL_TO, "Polynomial"));
  setPropertySettings("CostFunction", make_unique<EnabledWhenProperty>(
                                          "TypeOfBackgroundSubtraction",
                                          IS_EQUAL_TO, "Polynomial"));

  // Output workspace
  declareProperty(make_unique<WorkspaceProperty<>>("OutputWorkspace", "",
                                                   Direction::Output,
                                                   PropertyMode::Optional),
                  "A Workspace with the background removed.");
}

//----------------------------------------------------------------------------------------------
/** Execute the algorithm.
 */
void ReflectometryBackgroundSubtraction::exec() {
  API::MatrixWorkspace_sptr inputWS = getProperty("InputWorkspace");
  std::string backgroundType = getProperty("TypeOfBackgroundSubtraction");

  // Set default name for output workspaces
  std::string const wsName = inputWS->getName();
  if (isDefault("OutputWorkspace")) {
    setPropertyValue("OutputWorkspace", wsName + "_Background");
  }

  if (backgroundType == "Per Spectra Average") {
    calculateAverageSpectrumBackground(inputWS);
  }

  if (backgroundType == "Polynomial") {
    calculatePolynomialBackground(inputWS);
  }
}
} // namespace Algorithms
} // namespace Mantid
