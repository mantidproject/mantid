// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------

#include "MantidMuon/CalculateMuonAsymmetry.h"
#include "MantidMuon/MuonAsymmetryHelper.h"

#include "MantidAPI/ADSValidator.h"
#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/FuncMinimizerFactory.h"
#include "MantidAPI/FunctionProperty.h"
#include "MantidAPI/IFuncMinimizer.h"
#include "MantidAPI/IFunction.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/MultiDomainFunction.h"
#include "MantidAPI/Run.h"

#include "MantidKernel/ArrayOrderedPairsValidator.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/BoundedValidator.h"
#include "MantidKernel/ListValidator.h"
#include "MantidKernel/StartsWithValidator.h"

#include "MantidMuon/MuonAlgorithmHelper.h"

#include <cmath>
#include <numeric>
#include <vector>

namespace Mantid::Algorithms {

using namespace Kernel;
using std::size_t;

// Register the class into the algorithm factory
DECLARE_ALGORITHM(CalculateMuonAsymmetry)

/** Initialisation method. Declares properties to be used in algorithm.
 *
 */
void CalculateMuonAsymmetry::init() {
  // norm table to update
  declareProperty(std::make_unique<API::WorkspaceProperty<API::ITableWorkspace>>(
                      "NormalizationTable", "", Direction::Input, API::PropertyMode::Optional),
                  "Name of the table containing the normalizations for the asymmetries.");
  // list of uNonrm workspaces to fit to
  declareProperty(std::make_unique<Kernel::ArrayProperty<std::string>>("UnNormalizedWorkspaceList",
                                                                       std::make_shared<API::ADSValidator>()),
                  "An ordered list of workspaces (to get the initial values "
                  "for the normalizations).");
  // list of workspaces to output renormalized result to
  declareProperty(std::make_unique<Kernel::ArrayProperty<std::string>>("ReNormalizedWorkspaceList",
                                                                       std::make_shared<API::ADSValidator>()),
                  "An ordered list of workspaces (to get the initial values "
                  "for the normalizations).");

  declareProperty("OutputFitWorkspace", "fit", "The name of the output fit workspace.");

  declareProperty("StartX", 0.1, "The lower limit for calculating the asymmetry (an X value).");
  declareProperty("EndX", 15.0, "The upper limit for calculating the asymmetry  (an X value).");
  declareProperty(
      std::make_unique<ArrayProperty<double>>("Exclude", std::make_shared<ArrayOrderedPairsValidator<double>>()),
      "A list of pairs of real numbers, defining the regions to "
      "exclude from the fit for all spectra.");

  declareProperty(std::make_unique<API::FunctionProperty>("InputFunction"), "The fitting function to be converted.");

  std::vector<std::string> minimizerOptions = API::FuncMinimizerFactory::Instance().getKeys();
  Kernel::IValidator_sptr minimizerValidator = std::make_shared<Kernel::StartsWithValidator>(minimizerOptions);
  declareProperty("Minimizer", "Levenberg-MarquardtMD", minimizerValidator, "Minimizer to use for fitting.");
  auto mustBePositive = std::make_shared<Kernel::BoundedValidator<int>>();
  mustBePositive->setLower(0);
  declareProperty("MaxIterations", 500, mustBePositive->clone(),
                  "Stop after this number of iterations if a good fit is not found");
  declareProperty("OutputStatus", "", Kernel::Direction::Output);
  declareProperty("ChiSquared", 0.0, Kernel::Direction::Output);
  declareProperty(std::make_unique<API::FunctionProperty>("OutputFunction", Kernel::Direction::Output),
                  "The fitting function after fit.");
  declareProperty("EnableDoublePulse", false, "Controls whether to perform a double pulse or single pulse fit.");
  declareProperty("PulseOffset", 0.0, "The time offset between the two pulses");
  declareProperty("FirstPulseWeight", 0.5,
                  "Weighting of first pulse (w_1)."
                  "The second pulse weighting (w_1) is set as w_2 = 1 - w_1.");
}
/*
 * Validate the input parameters
 * @returns map with keys corresponding to properties with errors and values
 * containing the error messages.
 */
std::map<std::string, std::string> CalculateMuonAsymmetry::validateInputs() {
  // create the map
  std::map<std::string, std::string> validationOutput;
  // check start and end times
  double startX = getProperty("StartX");
  double endX = getProperty("EndX");
  if (startX > endX) {
    validationOutput["StartX"] = "Start time is after the end time.";
  } else if (startX == endX) {
    validationOutput["StartX"] = "Start and end times are equal, there is no "
                                 "data to apply the algorithm to.";
  }
  // check inputs
  std::vector<std::string> unnormWS = getProperty("UnNormalizedWorkspaceList");
  std::vector<std::string> normWS = getProperty("ReNormalizedWorkspaceList");
  if (normWS.size() != unnormWS.size()) {
    validationOutput["ReNormalizedWorkspaceList"] = "The ReNormalizedWorkspaceList and UnNormalizedWorkspaceList must "
                                                    "contain the same number of workspaces.";
  }
  API::IFunction_sptr tmp = getProperty("InputFunction");
  auto function = std::dynamic_pointer_cast<API::CompositeFunction>(tmp);
  if (function == nullptr) {
    validationOutput["InputFunction"] = "The fitting function is not the correct type";
  } else if (function->getNumberDomains() != normWS.size()) {
    validationOutput["InputFunction"] = "The Fitting function does not have "
                                        "the same number of domains as the "
                                        "number of domains to fit.";
  }

  // check norm table is correct -> move this to helper when move muon algs to
  // muon folder
  API::ITableWorkspace_const_sptr tabWS = getProperty("NormalizationTable");

  if (tabWS) {
    if (tabWS->columnCount() == 0) {
      validationOutput["NormalizationTable"] = "Please provide a non-empty NormalizationTable.";
    }
    // NormalizationTable should have three columns: (norm, name, method)
    if (tabWS->columnCount() != 3) {
      validationOutput["NormalizationTable"] = "NormalizationTable must have three columns";
    }
    auto columnNames = tabWS->getColumnNames();
    int normCount = 0;
    int wsNamesCount = 0;
    for (const std::string &columnName : columnNames) {

      if (columnName == "norm") {
        normCount += 1;
      }

      if (columnName == "name") {
        wsNamesCount += 1;
      }
    }
    if (normCount == 0) {
      validationOutput["NormalizationTable"] = "NormalizationTable needs norm column";
    }
    if (wsNamesCount == 0) {
      validationOutput["NormalizationTable"] = "NormalizationTable needs a name column";
    }
    if (normCount > 1) {
      validationOutput["NormalizationTable"] = "NormalizationTable has " + std::to_string(normCount) + " norm columns";
    }
    if (wsNamesCount > 1) {
      validationOutput["NormalizationTable"] =
          "NormalizationTable has " + std::to_string(wsNamesCount) + " name columns";
    }
  }

  return validationOutput;
}
/** Executes the algorithm
 *
 */

void CalculateMuonAsymmetry::exec() {
  const std::vector<std::string> wsNamesUnNorm = getProperty("UnNormalizedWorkspaceList");
  std::vector<std::string> wsNames = getProperty("reNormalizedWorkspaceList");

  // get new norm
  std::vector<double> norms = getNormConstants(wsNamesUnNorm); // this will do the fit
  auto containsZeros = std::any_of(norms.begin(), norms.end(), [](double value) { return value == 0.0; });
  if (containsZeros) {

    setProperty("OutputStatus", "Aborted, a normalization constant was zero");
    g_log.error("Got a zero for the normalization, aborting algorithm.");
    return;
  }
  // update the ws to new norm
  for (size_t j = 0; j < wsNames.size(); j++) {
    API::MatrixWorkspace_sptr ws =
        API::AnalysisDataService::Instance().retrieveWS<API::MatrixWorkspace>(wsNamesUnNorm[j]);
    API::MatrixWorkspace_sptr normWS =
        API::AnalysisDataService::Instance().retrieveWS<API::MatrixWorkspace>(wsNames[j]);

    normalizeWorkspace(normWS, ws, 0, norms[j]);
    API::AnalysisDataService::Instance().addOrReplace(normWS->getName(), normWS);

    MuonAlgorithmHelper::addSampleLog(normWS, "analysis_asymmetry_norm", std::to_string(norms[j]));
  }
  addNormalizedFits(wsNames.size(), norms);
  // update table with new norm
  std::vector<std::string> methods(wsNames.size(), "Calculated");
  API::ITableWorkspace_sptr table = getProperty("NormalizationTable");
  if (table) {

    updateNormalizationTable(table, wsNames, norms, methods);
  }
}

/**
 * Adds a normalised fit as the last spectra in the fitted workspace.
 * @param numberOfFits ::  The number of simultaneous fits performed
 * @param norms :: a vector of normalisation values
 * @return normalization constants
 */
void CalculateMuonAsymmetry::addNormalizedFits(size_t numberOfFits, const std::vector<double> &norms) {
  for (size_t j = 0; j < numberOfFits; j++) {
    API::Workspace_sptr fitWorkspace = getProperty("OutputWorkspace");
    API::MatrixWorkspace_sptr fitWorkspaceActual;
    if (fitWorkspace->isGroup()) {
      fitWorkspaceActual = std::dynamic_pointer_cast<API::MatrixWorkspace>(
          std::static_pointer_cast<API::WorkspaceGroup>(fitWorkspace)->getItem(0));
    } else {
      fitWorkspaceActual = std::dynamic_pointer_cast<API::MatrixWorkspace>(fitWorkspace);
    }
    API::IAlgorithm_sptr extractSpectra = createChildAlgorithm("ExtractSingleSpectrum");
    API::IAlgorithm_sptr appendSpectra = createChildAlgorithm("AppendSpectra");

    extractSpectra->setProperty("InputWorkspace", fitWorkspaceActual);
    extractSpectra->setProperty("WorkspaceIndex", 1);
    extractSpectra->execute();
    API::MatrixWorkspace_sptr unnormalisedFit = extractSpectra->getProperty("OutputWorkspace");

    normalizeWorkspace(unnormalisedFit, fitWorkspaceActual, 1, norms[j]);

    appendSpectra->setProperty("InputWorkspace1", fitWorkspaceActual);
    appendSpectra->setProperty("InputWorkspace2", unnormalisedFit);
    appendSpectra->execute();
    API::MatrixWorkspace_sptr appendedFitWorkspace = appendSpectra->getProperty("OutputWorkspace");

    if (fitWorkspace->isGroup()) {
      std::string workspaceName = fitWorkspaceActual->getName();
      auto fitWorkspaceGroupPointer = std::static_pointer_cast<API::WorkspaceGroup>(fitWorkspace);
      fitWorkspaceGroupPointer->removeItem(0);
      API::AnalysisDataService::Instance().addOrReplace(workspaceName, appendedFitWorkspace);
      fitWorkspaceGroupPointer->addWorkspace(appendedFitWorkspace);
    } else {
      setProperty("OutputWorkspace", appendedFitWorkspace);
    }
  }
}

/**
 * Calculate normalization constant after the exponential decay has been removed
 * to a linear fitting function
 * @param wsNames ::  names of workspaces to fit to
 * @return normalization constants
 */
std::vector<double> CalculateMuonAsymmetry::getNormConstants(const std::vector<std::string> &wsNames) {
  std::vector<double> norms;
  double startX = getProperty("StartX");
  double endX = getProperty("EndX");
  std::string exclude = getProperty("Exclude");
  int maxIterations = getProperty("MaxIterations");
  auto minimizer = getProperty("Minimizer");
  API::IAlgorithm_sptr fit;
  auto doublePulseEnabled = getProperty("EnableDoublePulse");
  if (doublePulseEnabled) {
    double pulseOffset = getProperty("PulseOffset");
    double firstPulseWeight = getProperty("FirstPulseWeight");
    fit = createChildAlgorithm("DoublePulseFit");
    fit->initialize();
    fit->setProperty("PulseOffset", pulseOffset);
    fit->setProperty("FirstPulseWeight", firstPulseWeight);
    fit->setProperty("SecondPulseWeight", 1.0 - firstPulseWeight);
  } else {
    fit = createChildAlgorithm("Fit");
    fit->initialize();
  }

  API::IFunction_sptr function = getProperty("InputFunction");

  fit->setProperty("Function", function);

  fit->setProperty("MaxIterations", maxIterations);

  fit->setPropertyValue("Minimizer", minimizer);

  fit->setProperty("CreateOutput", true);

  std::string output = getPropertyValue("OutputFitWorkspace");

  fit->setProperty("Output", output);

  fit->setProperty("InputWorkspace", wsNames[0]);
  fit->setProperty("StartX", startX);
  fit->setProperty("EndX", endX);
  fit->setProperty("Exclude", exclude);
  fit->setProperty("WorkspaceIndex", 0);

  if (wsNames.size() > 1) {
    for (size_t j = 1; j < wsNames.size(); j++) {
      std::string suffix = boost::lexical_cast<std::string>(j);

      fit->setPropertyValue("InputWorkspace_" + suffix, wsNames[j]);
      fit->setProperty("WorkspaceIndex_" + suffix, 0);
      fit->setProperty("StartX_" + suffix, startX);
      fit->setProperty("EndX_" + suffix, endX);
    }
  }

  fit->execute();
  auto status = fit->getPropertyValue("OutputStatus");
  setProperty("OutputStatus", status);
  double chi2 = std::stod(fit->getPropertyValue("OutputChi2overDoF"));
  setProperty("ChiSquared", chi2);
  API::IFunction_sptr tmp = fit->getProperty("Function");
  setProperty("OutputFunction", tmp);

  API::ITableWorkspace_sptr parameterTable = fit->getProperty("OutputParameters");
  API::Workspace_sptr outputWorkspace;
  if (wsNames.size() > 1) {
    API::WorkspaceGroup_sptr outputWorkspaceGroup = fit->getProperty("OutputWorkspace");
    outputWorkspace = outputWorkspaceGroup;
  } else {
    API::MatrixWorkspace_sptr outputWorkspaceMatrix = fit->getProperty("OutputWorkspace");
    outputWorkspace = outputWorkspaceMatrix;
  }

  API::ITableWorkspace_sptr outputNormalisedCovarianceMatrix = fit->getProperty("OutputNormalisedCovarianceMatrix");

  declareProperty(
      std::make_unique<API::WorkspaceProperty<API::ITableWorkspace>>("OutputParameters", "", Kernel::Direction::Output),
      "The name of the TableWorkspace in which to store the "
      "final fit parameters");
  setPropertyValue("OutputParameters", output + "_Parameters");

  declareProperty(
      std::make_unique<API::WorkspaceProperty<API::Workspace>>("OutputWorkspace", "", Kernel::Direction::Output),
      "The name of the matrix in which to store the "
      "final fit results");
  if (outputWorkspace->isGroup()) {
    setPropertyValue("OutputWorkspace", output + "_Workspaces");
  } else {
    setPropertyValue("OutputWorkspace", output + "_Workspace");
  }

  declareProperty(std::make_unique<API::WorkspaceProperty<API::ITableWorkspace>>("OutputNormalisedCovarianceMatrix", "",
                                                                                 Kernel::Direction::Output),
                  "The name of the TableWorkspace in which to store the final covariance "
                  "matrix");
  setPropertyValue("OutputNormalisedCovarianceMatrix", output + "_NormalisedCovarianceMatrix");

  setProperty("OutputParameters", parameterTable);
  setProperty("OutputWorkspace", outputWorkspace);
  setProperty("OutputNormalisedCovarianceMatrix", outputNormalisedCovarianceMatrix);

  const auto wrongFunctionFormError =
      "The fitting function is not of the expected form. Try using ConvertFitFunctionForMuonTFAsymmetry";

  if (wsNames.size() == 1) {
    // N(1+g) + exp
    auto TFFunc = std::dynamic_pointer_cast<API::CompositeFunction>(tmp);
    if (TFFunc == nullptr) {
      throw std::invalid_argument(wrongFunctionFormError);
    }
    norms.emplace_back(getNormValue(TFFunc));
  } else {
    auto result = std::dynamic_pointer_cast<API::MultiDomainFunction>(tmp);
    if (result == nullptr) {
      throw std::invalid_argument(wrongFunctionFormError);
    }
    for (size_t j = 0; j < wsNames.size(); j++) {
      // get domain
      auto TFFunc = std::dynamic_pointer_cast<API::CompositeFunction>(result->getFunction(j));
      if (TFFunc == nullptr) {
        throw std::invalid_argument(wrongFunctionFormError);
      }
      // N(1+g) + exp
      norms.emplace_back(getNormValue(TFFunc));
    }
  }
  return norms;
}
/**
 * Gets the normalization from a fitting function
 * @param func ::  fittef function
 * @return normalization constant
 */
double CalculateMuonAsymmetry::getNormValue(API::CompositeFunction_sptr &func) {

  // getFunction(0) -> N(1+g)
  auto TFFunc = std::dynamic_pointer_cast<API::CompositeFunction>(func->getFunction(0));

  // getFunction(0) -> N
  auto flat = TFFunc->getFunction(0);

  return flat->getParameter("A0");
}
/**
 * Normalize a single spectrum workpace based on a specified index from another
 * workspace and a N0 value. The unormalized from here is N0(1+f) where f is the
 * desired normalized function.
 * @param normalizedWorkspace :: workspace to store normalised data
 * @param unormalizedWorkspace :: workspace to normalise from
 * @param workspaceIndex :: index of raw data in unormalizedWorkspace
 * @param N0 :: normalization value
 * @return normalization constant
 */
void CalculateMuonAsymmetry::normalizeWorkspace(const API::MatrixWorkspace_sptr &normalizedWorkspace,
                                                const API::MatrixWorkspace_const_sptr &unnormalizedWorkspace,
                                                size_t workspaceIndex, double N0) {
  normalizedWorkspace->mutableY(0) = unnormalizedWorkspace->y(workspaceIndex) / N0;
  normalizedWorkspace->mutableY(0) -= 1.0;
  normalizedWorkspace->mutableE(0) = unnormalizedWorkspace->e(workspaceIndex) / N0;
}
} // namespace Mantid::Algorithms
