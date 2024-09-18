// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidMuon/ConvertFitFunctionForMuonTFAsymmetry.h"

#include "MantidAPI/ADSValidator.h"
#include "MantidAPI/CompositeFunction.h"
#include "MantidAPI/FunctionFactory.h"
#include "MantidAPI/FunctionProperty.h"
#include "MantidAPI/ITableWorkspace.h"
#include "MantidAPI/MultiDomainFunction.h"
#include "MantidAPI/TableRow.h"

#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/Run.h"

#include "MantidAPI/AnalysisDataService.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/CompositeValidator.h"
#include "MantidKernel/ListValidator.h"
#include "MantidKernel/MandatoryValidator.h"
#include "MantidKernel/PhysicalConstants.h"

#include <vector>

namespace {
constexpr double MICROSECONDS_PER_SECOND{1000000.0};
constexpr double MUON_LIFETIME_MICROSECONDS{Mantid::PhysicalConstants::MuonLifetime * MICROSECONDS_PER_SECOND};
const std::string INSERT_FUNCTION{"f0.f1.f1."};

std::string trimTie(const std::string &stringTie) {
  auto index = stringTie.find_first_of(".");
  std::string domain = stringTie.substr(0, index);
  std::string userFunc = stringTie.substr(9 + index, std::string::npos);
  return domain + userFunc;
}

std::string rmInsertFunction(const std::string &originalTie) {
  auto stringTie = originalTie;
  // check the tie only exists in the user function (f)
  auto seperator = stringTie.find_first_of("=");
  // the wrapped name added 9 characters
  auto LHName = stringTie.substr(0, seperator);
  LHName = trimTie(LHName);
  // this one includes = sign
  auto RHName = stringTie.substr(seperator, std::string::npos);
  RHName = trimTie(RHName);

  return LHName + RHName;
}

int findName(const std::vector<std::string> &colNames, const char *name) {
  for (size_t j = 0; j < colNames.size(); j++) {
    if (colNames[j] == name) {
      return static_cast<int>(j);
    }
  }
  return -1;
}
} // namespace
namespace Mantid::Muon {

using namespace Kernel;
using namespace API;
using std::size_t;

// Register the class into the algorithm factory
DECLARE_ALGORITHM(ConvertFitFunctionForMuonTFAsymmetry)

/** Initialisation method. Declares properties to be used in algorithm.
 *
 */
void ConvertFitFunctionForMuonTFAsymmetry::init() {
  declareProperty(std::make_unique<FunctionProperty>("InputFunction"), "The fitting function to be converted.");
  // table of name, norms
  // if construct -> read relevant norms into sorted list
  declareProperty(std::make_unique<API::WorkspaceProperty<API::ITableWorkspace>>(
                      "NormalizationTable", "", Direction::Input, API::PropertyMode::Optional),
                  "Name of the table containing the normalizations for the asymmetries.");
  // list of workspaces
  declareProperty(
      std::make_unique<Kernel::ArrayProperty<std::string>>("WorkspaceList", std::make_shared<API::ADSValidator>()),
      "An ordered list of workspaces (to get the initial values "
      "for the normalizations).");

  std::vector<std::string> allowedModes{"Construct", "Extract"};
  auto modeVal = std::make_shared<Kernel::CompositeValidator>();
  modeVal->add(std::make_shared<Kernel::StringListValidator>(allowedModes));
  modeVal->add(std::make_shared<Kernel::MandatoryValidator<std::string>>());
  declareProperty("Mode", "Construct", modeVal,
                  "Mode to run in. Construct will convert the"
                  "input function into one suitable for calculating the"
                  " TF Asymmetry. Extract will find the original user function"
                  " from a function that is suitable for TF Asymmetry calculations.");

  declareProperty(std::make_unique<FunctionProperty>("OutputFunction", Direction::Output),
                  "The converted fitting function.");

  declareProperty("CopyTies", true,
                  "Set to true to copy over ties from input function"
                  "(default is true).");
}

/*
 * Validate the input parameters
 * @returns map with keys corresponding to properties with errors and values
 * containing the error messages.
 */
std::map<std::string, std::string> ConvertFitFunctionForMuonTFAsymmetry::validateInputs() {
  // create the map
  std::map<std::string, std::string> result;
  // check norm table is correct
  API::ITableWorkspace_const_sptr tabWS = getProperty("NormalizationTable");
  if (tabWS) {
    if (tabWS->columnCount() == 0) {
      result["NormalizationTable"] = "Please provide a non-empty NormalizationTable.";
    }

    // NormalizationTable should have three columns: (norm, name, method)
    if (tabWS->columnCount() != 3) {
      result["NormalizationTable"] = "NormalizationTable must have three columns";
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
      result["NormalizationTable"] = "NormalizationTable needs norm column";
    }
    if (wsNamesCount == 0) {
      result["NormalizationTable"] = "NormalizationTable needs a name column";
    }
    if (normCount > 1) {
      result["NormalizationTable"] = "NormalizationTable has " + std::to_string(normCount) + " norm columns";
    }
    if (wsNamesCount > 1) {
      result["NormalizationTable"] = "NormalizationTable has " + std::to_string(wsNamesCount) + " name columns";
    }
  } else {
    const std::vector<std::string> wsNames = getProperty("WorkspaceList");
    for (std::string wsName : wsNames) {
      API::MatrixWorkspace_const_sptr ws =
          API::AnalysisDataService::Instance().retrieveWS<API::MatrixWorkspace>(wsName);
      const Mantid::API::Run &run = ws->run();
      if (!run.hasProperty("analysis_asymmetry_norm")) {
        result["NormalizationTable"] = "NormalizationTable has not been "
                                       "included and no sample logs for "
                                       "nrmalization.";
      }
    }
  }
  // Check units, should be microseconds
  return result;
}

/** Executes the algorithm
 *
 */
void ConvertFitFunctionForMuonTFAsymmetry::exec() {
  IFunction_sptr inputFitFunction = getProperty("InputFunction");
  auto mode = getPropertyValue("Mode");
  if (mode == "Construct") {
    std::vector<double> norms = getNorms();
    auto outputFitFunction = getTFAsymmFitFunction(inputFitFunction, norms);
    setOutput(outputFitFunction);
  } else {
    try {
      auto outputFitFunction = extractFromTFAsymmFitFunction(inputFitFunction);
      setOutput(outputFitFunction);
    } catch (...) {
      throw std::runtime_error("The input function was not of the form N*(1+f)+A*exp(-lambda*t)");
    }
  }
}

void ConvertFitFunctionForMuonTFAsymmetry::setOutput(const Mantid::API::IFunction_sptr &function) {
  IFunction_sptr outputFitFunction = function;
  const std::vector<std::string> wsNames = getProperty("WorkspaceList");
  if (wsNames.size() == 1) {
    // if single domain func, strip off multi domain
    auto TFFunc = std::dynamic_pointer_cast<CompositeFunction>(function);
    outputFitFunction = TFFunc->getFunction(0);
  }
  setProperty("OutputFunction", outputFitFunction);
}
/** Extracts the user's original function f from the normalization function
 * N(1+f)+expDecay
 * and adds in the ties
 * @param original :: [input] normalization function
 * @return :: user function
 */
Mantid::API::IFunction_sptr
ConvertFitFunctionForMuonTFAsymmetry::extractFromTFAsymmFitFunction(const Mantid::API::IFunction_sptr &original) {

  auto multi = std::make_shared<MultiDomainFunction>();
  IFunction_sptr tmp = original;

  size_t numDomains = original->getNumberDomains();
  for (size_t j = 0; j < numDomains; j++) {
    auto TFFunc = std::dynamic_pointer_cast<CompositeFunction>(original);
    if (numDomains > 1) {
      // get correct domain
      tmp = TFFunc->getFunction(j);
      multi->setDomainIndex(j, j);
    }
    IFunction_sptr userFunc = extractUserFunction(tmp);
    multi->addFunction(userFunc);
  }
  // if multi data set we need to do the ties manually
  bool copyTies = getProperty("CopyTies");
  if (numDomains > 1 && copyTies) {
    auto originalNames = original->getParameterNames();
    for (const auto &parName : originalNames) {
      const auto index = original->parameterIndex(parName);
      auto originalTie = original->getTie(index);
      if (originalTie) {
        const auto stringTie = originalTie->asString();
        // check the tie only exists in the user function (f)
        auto start = stringTie.find_first_of(".") + 1;
        const auto end = stringTie.find_first_of("=");
        // the wrapped name added 9 characters
        const auto LHName = stringTie.substr(start, 9);
        // need to do in 2 steps
        auto RHName = stringTie.substr(end, std::string::npos);

        start = RHName.find_first_of(".") + 1;
        RHName = RHName.substr(start, 9);
        if (LHName == INSERT_FUNCTION && LHName == RHName) {
          // get new tie
          const auto newTie = rmInsertFunction(stringTie);
          multi->addTies(newTie);
        }
      }
    }
  }

  return std::dynamic_pointer_cast<IFunction>(multi);
}
/** Extracts the user's original function f from the normalization function
 * N(1+f)+expDecay
 * @param original :: [input] normalization function
 * @return :: user function
 */

IFunction_sptr ConvertFitFunctionForMuonTFAsymmetry::extractUserFunction(const IFunction_sptr &TFFuncIn) {
  // N(1+g) + exp
  auto TFFunc = std::dynamic_pointer_cast<CompositeFunction>(TFFuncIn);
  if (TFFunc == nullptr) {
    throw std::runtime_error("Input function is not of the correct form");
  }
  // getFunction(0) -> N(1+g)

  TFFunc = std::dynamic_pointer_cast<CompositeFunction>(TFFunc->getFunction(0));
  if (TFFunc == nullptr) {
    throw std::runtime_error("Input function is not of the correct form");
  }
  // getFunction(1) -> 1+g

  TFFunc = std::dynamic_pointer_cast<CompositeFunction>(TFFunc->getFunction(1));
  if (TFFunc == nullptr) {
    throw std::runtime_error("Input function is not of the correct form");
  }
  // getFunction(1) -> g
  return TFFunc->getFunction(1);
}

/** Get the nomralisation constants from the table
 * the order is the same as the workspace list
 * @return :: vector of normals
 */
std::vector<double> ConvertFitFunctionForMuonTFAsymmetry::getNorms() {
  API::ITableWorkspace_sptr table = getProperty("NormalizationTable");
  const std::vector<std::string> wsNames = getProperty("WorkspaceList");
  std::vector<double> norms(wsNames.size(), 0);

  if (table) {
    auto colNames = table->getColumnNames();
    auto wsNamesIndex = findName(colNames, "name");
    auto normIndex = findName(colNames, "norm");

    for (size_t row = 0; row < table->rowCount(); row++) {
      for (size_t wsPosition = 0; wsPosition < wsNames.size(); wsPosition++) {
        std::string wsName = wsNames[wsPosition];
        std::replace(wsName.begin(), wsName.end(), ' ', ';');
        if (table->String(row, wsNamesIndex) == wsName) {
          norms[wsPosition] = table->Double(row, normIndex);
        }
      }
    }
  } else {
    for (size_t wsPosition = 0; wsPosition < wsNames.size(); wsPosition++) {
      std::string wsName = wsNames[wsPosition];
      API::MatrixWorkspace_const_sptr ws =
          API::AnalysisDataService::Instance().retrieveWS<API::MatrixWorkspace>(wsName);
      const Mantid::API::Run &run = ws->run();
      norms[wsPosition] = std::stod(run.getProperty("analysis_asymmetry_norm")->value());
    }
  }

  return norms;
}
/** Gets the fitting function for TFAsymmetry fit
 * @param original :: The user function f
 * @param norms :: vector of normalization constants
 * @returns :: The normalization function N(1+f) +ExpDecay
 */
Mantid::API::IFunction_sptr
ConvertFitFunctionForMuonTFAsymmetry::getTFAsymmFitFunction(const Mantid::API::IFunction_sptr &original,
                                                            const std::vector<double> &norms) {
  auto multi = std::make_shared<MultiDomainFunction>();
  auto tmp = std::dynamic_pointer_cast<MultiDomainFunction>(original);
  size_t numDomains = original->getNumberDomains();
  for (size_t j = 0; j < numDomains; j++) {
    IFunction_sptr userFunc;
    auto constant = FunctionFactory::Instance().createInitialized("name = FlatBackground, A0 = 1.0, ties=(A0=1)");
    if (numDomains == 1) {
      userFunc = original;
    } else {
      userFunc = tmp->getFunction(j);
      multi->setDomainIndex(j, j);
    }
    auto inBrace = std::make_shared<CompositeFunction>();
    inBrace->addFunction(constant);
    inBrace->addFunction(userFunc);
    auto norm = FunctionFactory::Instance().createInitialized("name = FlatBackground, A0 "
                                                              "=" +
                                                              std::to_string(norms[j]));
    auto product =
        std::dynamic_pointer_cast<CompositeFunction>(FunctionFactory::Instance().createFunction("ProductFunction"));
    product->addFunction(norm);
    product->addFunction(inBrace);
    auto composite =
        std::dynamic_pointer_cast<CompositeFunction>(FunctionFactory::Instance().createFunction("CompositeFunction"));
    constant = FunctionFactory::Instance().createInitialized(
        "name = ExpDecayMuon, A = 0.0, Lambda = -" + std::to_string(MUON_LIFETIME_MICROSECONDS) +
        ",ties = (A = 0.0, Lambda = -" + std::to_string(MUON_LIFETIME_MICROSECONDS) + ")");
    composite->addFunction(product);
    composite->addFunction(constant);
    multi->addFunction(composite);
  }
  // if multi data set we need to do the ties manually
  bool copyTies = getProperty("CopyTies");
  if (numDomains > 1 && copyTies) {
    auto originalNames = original->getParameterNames();
    for (const auto &parName : originalNames) {
      auto index = original->parameterIndex(parName);
      auto originalTie = original->getTie(index);
      if (originalTie) {
        auto stringTie = originalTie->asString();
        // change name to reflect new postion
        auto insertPosition = stringTie.find_first_of(".");
        stringTie.insert(insertPosition + 1, INSERT_FUNCTION);
        // need to change the other side of =
        insertPosition = stringTie.find_first_of("=");
        insertPosition = stringTie.find_first_of(".", insertPosition);
        stringTie.insert(insertPosition + 1, INSERT_FUNCTION);
        multi->addTies(stringTie);
      }
    }
  }

  return std::dynamic_pointer_cast<IFunction>(multi);
}
} // namespace Mantid::Muon
