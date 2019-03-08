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
#include "MantidKernel/ListValidator.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidAPI/Algorithm.tcc"

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
  return "";
}

void ReflectometryBackgroundSubtraction::calculateAverageSpectrumBackground(
    API::MatrixWorkspace_sptr inputWS, std::vector<specnum_t> spectraList) {

auto alg = this->createChildAlgorithm("GroupDetectors");
  alg->setProperty("InputWorkspace", inputWS);
  alg->setProperty("SpectraList", spectraList);
  alg->setProperty("Behaviour", "Average");
  alg->execute();
  API::MatrixWorkspace_sptr outputWS = alg->getProperty("OutputWorkspace");

  bool subtract = getProperty("SubtractBackground");
  if (subtract == true) {
    auto subtract = createChildAlgorithm("Minus");
    subtract->setProperty("LHSWorkspace", inputWS);
    subtract->setProperty("RHSWorkspace", outputWS);
    subtract->setProperty("AllowDifferentNumberSpectra", true);
    subtract->execute();
    outputWS = subtract->getProperty("OutputWorkspace");
  }

  setProperty("OutputWorkspace", outputWS);
}

std::vector<double> ReflectometryBackgroundSubtraction::findSpectrumRanges(
    API::MatrixWorkspace_sptr inputWS, std::vector<specnum_t> spectraList) {
	
  std::vector<double> spectrumRanges;
  spectrumRanges.push_back(spectraList[0]);
  auto prevSpec = spectrumRanges[0];
   for (auto index = 1; index < spectraList.size() -1; ++index) {
      auto spec = spectraList[index+1];
      auto range = spec - prevSpec;
	  //check if start of new range
      if (range > 1) {
        spectrumRanges.push_back(prevSpec);
        spectrumRanges.push_back(spec); 
	  }  
	  prevSpec = spec;
	  }
   spectrumRanges.push_back(spectraList.back());
  return spectrumRanges;
}

void ReflectometryBackgroundSubtraction::calculatePolynomialBackground(
    API::MatrixWorkspace_sptr inputWS, std::vector<double> spectrumRanges) {
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
  poly->setProperty("XRanges", spectrumRanges); 
  poly->setProperty("CostFunction", getPropertyValue("CostFunction"));
  poly->execute();
  API::MatrixWorkspace_sptr bgd = poly->getProperty("OutputWorkspace");

  // transpose again to get the background
  transpose->setProperty("InputWorkspace", bgd);
  transpose->execute();
  API::MatrixWorkspace_sptr outputWS =
      transpose->getProperty("OutputWorkspace");

  // subtract the background from the input workspace
  bool subtract = getProperty("SubtractBackground");
  if (subtract == true) {
    outputWS = minus(inputWS, outputWS);
  }

setProperty("OutputWorkspace", outputWS);
  
}

//----------------------------------------------------------------------------------------------
/** Initialize the algorithm's properties.
 */
void ReflectometryBackgroundSubtraction::init() {

  // Input workspace
  declareWorkspaceInputProperties<API::MatrixWorkspace,
                                  API::IndexType::SpectrumNum |
                                      API::IndexType::WorkspaceIndex>(
      "InputWorkspace", "An input workspace",
      boost::make_shared<API::CommonBinsValidator>());

  std::vector<std::string> backgroundTypes = {"Per Detector Average",
                                              "Polynomial"};
  declareProperty("SubtractionMethod", "Per Detector Average",
                  boost::make_shared<StringListValidator>(backgroundTypes),
                  "The type of background reduction to perform.",
                  Direction::Input);

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
                                                "SubtractionMethod",
                                                IS_EQUAL_TO, "Polynomial"));
  setPropertySettings("CostFunction", make_unique<EnabledWhenProperty>(
                                          "SubtractionMethod",
                                          IS_EQUAL_TO, "Polynomial"));

  declareProperty(make_unique<PropertyWithValue<bool>>("SubtractBackground",
                                                       true, Direction::Input),
                  "If true then the background is subtracted from the InputWorkspace before output");

  // Output workspace
  declareProperty(make_unique<WorkspaceProperty<>>("OutputWorkspace", "",
                                                   Direction::Output,
                                                   PropertyMode::Optional),
                  "The output Workspace containing either the background or the InputWorkspace with the background removed.");
}

//----------------------------------------------------------------------------------------------
/** Execute the algorithm.
 */
void ReflectometryBackgroundSubtraction::exec() {
  API::MatrixWorkspace_sptr inputWS;
  Indexing::SpectrumIndexSet indexSet;
  std::tie(inputWS, indexSet) =
      getWorkspaceAndIndices<API::MatrixWorkspace>("InputWorkspace");
  const std::string backgroundType = getProperty("SubtractionMethod");

  // Set default outputWorkspace name to be inputWorkspace Name
  const std::string wsName = inputWS->getName();
  if (isDefault("OutputWorkspace")) {
    setPropertyValue("OutputWorkspace" , wsName);
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
    auto spectrumRanges = findSpectrumRanges(inputWS, spectraList);
    calculatePolynomialBackground(inputWS, spectrumRanges);
  }
}

std::map<std::string, std::string>
ReflectometryBackgroundSubtraction::validateInputs() {

  std::map<std::string, std::string> errors;

  API::MatrixWorkspace_const_sptr inputWS;
  Indexing::SpectrumIndexSet indexSet;
  std::tie(inputWS, indexSet) =
      getWorkspaceAndIndices<API::MatrixWorkspace>("InputWorkspace");
  const std::string backgroundType = getProperty("SubtractionMethod");

  if (inputWS) {
    if (backgroundType == "Polynomial" && indexSet.size() == 1) {
      errors["InputWorkspaceIndexSet"] =
          "Input workspace index set must contain a more than one spectra for polynomial background subtraction";
    }
  }
  return errors;
  }
} // namespace Algorithms
} // namespace Mantid

