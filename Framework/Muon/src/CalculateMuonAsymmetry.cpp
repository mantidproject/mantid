// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
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

#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/BoundedValidator.h"
#include "MantidKernel/ListValidator.h"
#include "MantidKernel/StartsWithValidator.h"

#include <cmath>
#include <numeric>
#include <vector>

namespace Mantid {
namespace Algorithms {

using namespace Kernel;
using std::size_t;

// Register the class into the algorithm factory
DECLARE_ALGORITHM(CalculateMuonAsymmetry)

/** Initialisation method. Declares properties to be used in algorithm.
 *
 */
void CalculateMuonAsymmetry::init() {
  // norm table to update
  declareProperty(
      make_unique<API::WorkspaceProperty<API::ITableWorkspace>>(
          "NormalizationTable", "", Direction::Input),
      "Name of the table containing the normalizations for the asymmetries.");
  // list of uNonrm workspaces to fit to
  declareProperty(
      Kernel::make_unique<Kernel::ArrayProperty<std::string>>(
          "UnNormalizedWorkspaceList", boost::make_shared<API::ADSValidator>()),
      "An ordered list of workspaces (to get the initial values "
      "for the normalizations).");
  // list of workspaces to output renormalized result to
  declareProperty(
      Kernel::make_unique<Kernel::ArrayProperty<std::string>>(
          "ReNormalizedWorkspaceList", boost::make_shared<API::ADSValidator>()),
      "An ordered list of workspaces (to get the initial values "
      "for the normalizations).");

  declareProperty("OutputFitWorkspace", "fit",
                  "The name of the output fit workspace.");

  declareProperty(
      "StartX", 0.1,
      "The lower limit for calculating the asymmetry (an X value).");
  declareProperty(
      "EndX", 15.0,
      "The upper limit for calculating the asymmetry  (an X value).");
  declareProperty(make_unique<API::FunctionProperty>("InputFunction"),
                  "The fitting function to be converted.");

  std::vector<std::string> minimizerOptions =
      API::FuncMinimizerFactory::Instance().getKeys();
  Kernel::IValidator_sptr minimizerValidator =
      boost::make_shared<Kernel::StartsWithValidator>(minimizerOptions);
  declareProperty("Minimizer", "Levenberg-MarquardtMD", minimizerValidator,
                  "Minimizer to use for fitting.");
  auto mustBePositive = boost::make_shared<Kernel::BoundedValidator<int>>();
  mustBePositive->setLower(0);
  declareProperty(
      "MaxIterations", 500, mustBePositive->clone(),
      "Stop after this number of iterations if a good fit is not found");
  declareProperty("OutputStatus", "", Kernel::Direction::Output);
  declareProperty("ChiSquared", 0.0, Kernel::Direction::Output);
  declareProperty(make_unique<API::FunctionProperty>("OutputFunction",
                                                     Kernel::Direction::Output),
                  "The fitting function after fit.");
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
    validationOutput["ReNormalizedWorkspaceList"] =
        "The ReNormalizedWorkspaceList and UnNormalizedWorkspaceList must "
        "contain the same number of workspaces.";
  }
  API::IFunction_sptr tmp = getProperty("InputFunction");
  auto function = boost::dynamic_pointer_cast<API::CompositeFunction>(tmp);
  if (function->getNumberDomains() != normWS.size()) {
    validationOutput["InputFunction"] = "The Fitting function does not have "
                                        "the same number of domains as the "
                                        "number of domains to fit.";
  }

  // check norm table is correct -> move this to helper when move muon algs to
  // muon folder
  API::ITableWorkspace_const_sptr tabWS = getProperty("NormalizationTable");

  if (tabWS->columnCount() == 0) {
    validationOutput["NormalizationTable"] =
        "Please provide a non-empty NormalizationTable.";
  }
  // NormalizationTable should have three columns: (norm, name, method)
  if (tabWS->columnCount() != 3) {
    validationOutput["NormalizationTable"] =
        "NormalizationTable must have three columns";
  }
  auto names = tabWS->getColumnNames();
  int normCount = 0;
  int wsNamesCount = 0;
  for (const std::string &name : names) {

    if (name == "norm") {
      normCount += 1;
    }

    if (name == "name") {
      wsNamesCount += 1;
    }
  }
  if (normCount == 0) {
    validationOutput["NormalizationTable"] =
        "NormalizationTable needs norm column";
  }
  if (wsNamesCount == 0) {
    validationOutput["NormalizationTable"] =
        "NormalizationTable needs a name column";
  }
  if (normCount > 1) {
    validationOutput["NormalizationTable"] =
        "NormalizationTable has " + std::to_string(normCount) + " norm columns";
  }
  if (wsNamesCount > 1) {
    validationOutput["NormalizationTable"] = "NormalizationTable has " +
                                             std::to_string(wsNamesCount) +
                                             " name columns";
  }

  return validationOutput;
}
/** Executes the algorithm
 *
 */

void CalculateMuonAsymmetry::exec() {
  const std::vector<std::string> wsNamesUnNorm =
      getProperty("UnNormalizedWorkspaceList");
  std::vector<std::string> wsNames = getProperty("reNormalizedWorkspaceList");

  // get new norm
  std::vector<double> norms =
      getNormConstants(wsNamesUnNorm); // this will do the fit
  auto containsZeros = std::any_of(norms.begin(), norms.end(),
                                   [](double value) { return value == 0.0; });
  if (containsZeros) {

    setProperty("OutputStatus", "Aborted, a normalization constant was zero");
    g_log.error("Got a zero for the normalization, aborting algorithm.");
    return;
  }
  // update the ws to new norm
  for (size_t j = 0; j < wsNames.size(); j++) {
    API::MatrixWorkspace_sptr ws =
        API::AnalysisDataService::Instance().retrieveWS<API::MatrixWorkspace>(
            wsNamesUnNorm[j]);
    API::MatrixWorkspace_sptr normWS =
        API::AnalysisDataService::Instance().retrieveWS<API::MatrixWorkspace>(
            wsNames[j]);

    normWS->mutableY(0) = ws->y(0) / norms[j];
    normWS->mutableY(0) -= 1.0;
    normWS->mutableE(0) = ws->e(0) / norms[j];
  }
  // update table with new norm
  std::vector<std::string> methods(wsNames.size(), "Calculated");
  API::ITableWorkspace_sptr table = getProperty("NormalizationTable");
  updateNormalizationTable(table, wsNames, norms, methods);
}

/**
 * Calculate normalization constant after the exponential decay has been removed
 * to a linear fitting function
 * @param wsNames ::  names of workspaces to fit to
 * @return normalization constants
 */

std::vector<double> CalculateMuonAsymmetry::getNormConstants(
    const std::vector<std::string> wsNames) {
  std::vector<double> norms;

  double startX = getProperty("StartX");
  double endX = getProperty("EndX");
  int maxIterations = getProperty("MaxIterations");
  auto minimizer = getProperty("Minimizer");
  API::IAlgorithm_sptr fit =
      API::AlgorithmManager::Instance().createUnmanaged("Fit");
  fit->initialize();

  API::IFunction_sptr function = getProperty("InputFunction");

  fit->setProperty("Function", function);

  fit->setProperty("MaxIterations", maxIterations);

  fit->setPropertyValue("Minimizer", minimizer);

  std::string output = getPropertyValue("OutputFitWorkspace");

  fit->setProperty("Output", output);

  fit->setProperty("InputWorkspace", wsNames[0]);
  fit->setProperty("StartX", startX);
  fit->setProperty("EndX", endX);
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
  try {
    if (wsNames.size() == 1) {
      // N(1+g) + exp
      auto TFFunc = boost::dynamic_pointer_cast<API::CompositeFunction>(tmp);
      norms.push_back(getNormValue(TFFunc));
    } else {
      auto result = boost::dynamic_pointer_cast<API::MultiDomainFunction>(tmp);
      for (size_t j = 0; j < wsNames.size(); j++) {
        // get domain
        auto TFFunc = boost::dynamic_pointer_cast<API::CompositeFunction>(
            result->getFunction(j));
        // N(1+g) + exp
        TFFunc = boost::dynamic_pointer_cast<API::CompositeFunction>(TFFunc);
        norms.push_back(getNormValue(TFFunc));
      }
    }
  } catch (...) {
    throw std::invalid_argument("The fitting function is not of the expected "
                                "form. Try using "
                                "ConvertFitFunctionForMuonTFAsymmetry");
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
  auto TFFunc =
      boost::dynamic_pointer_cast<API::CompositeFunction>(func->getFunction(0));

  // getFunction(0) -> N
  auto flat = TFFunc->getFunction(0);

  return flat->getParameter("A0");
}
} // namespace Algorithms
} // namespace Mantid
