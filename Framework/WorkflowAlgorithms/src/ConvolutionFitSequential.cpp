#include "MantidWorkflowAlgorithms/ConvolutionFitSequential.h"

#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/Axis.h"
#include "MantidAPI/CompositeFunction.h"
#include "MantidAPI/FunctionDomain1D.h"
#include "MantidAPI/FunctionFactory.h"
#include "MantidAPI/ITableWorkspace.h"
#include "MantidAPI/NumericAxis.h"
#include "MantidAPI/Progress.h"
#include "MantidAPI/Run.h"
#include "MantidAPI/TextAxis.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidAPI/WorkspaceGroup.h"

#include "MantidKernel/BoundedValidator.h"
#include "MantidKernel/ListValidator.h"
#include "MantidKernel/MandatoryValidator.h"
#include "MantidKernel/StringContainsValidator.h"
#include "MantidKernel/VectorHelper.h"

#include <cmath>

namespace {
Mantid::Kernel::Logger g_log("ConvolutionFitSequential");

using namespace Mantid::API;

MatrixWorkspace_sptr convertSpectrumAxis(MatrixWorkspace_sptr inputWorkspace,
                                         const std::string &outputName) {
  auto convSpec =
      AlgorithmManager::Instance().createUnmanaged("ConvertSpectrumAxis");
  convSpec->setAlwaysStoreInADS(true);
  convSpec->setProperty("InputWorkSpace", inputWorkspace);
  convSpec->setProperty("OutputWorkSpace", outputName);
  convSpec->setProperty("Target", "ElasticQ");
  convSpec->setProperty("EMode", "Indirect");
  convSpec->execute();
  return convSpec->getProperty("OutputWorkspace");
}

MatrixWorkspace_sptr cloneWorkspace(MatrixWorkspace_sptr inputWorkspace,
                                    const std::string &outputName) {
  auto cloneWs = AlgorithmManager::Instance().createUnmanaged("CloneWorkspace");
  cloneWs->setAlwaysStoreInADS(true);
  cloneWs->setProperty("InputWorkspace", inputWorkspace);
  cloneWs->setProperty("OutputWorkspace", outputName);
  cloneWs->execute();
  return cloneWs->getProperty("OutputWorkspace");
}

MatrixWorkspace_sptr convertInputToElasticQ(MatrixWorkspace_sptr inputWorkspace,
                                            const std::string &outputName) {
  auto axis = inputWorkspace->getAxis(1);
  if (axis->isSpectra())
    return convertSpectrumAxis(inputWorkspace, outputName);
  else if (axis->isNumeric()) {
    if (axis->unit()->unitID() != "MomentumTransfer")
      throw std::runtime_error("Input must have axis values of Q");
    return cloneWorkspace(inputWorkspace, outputName);
  } else
    throw std::runtime_error(
        "Input workspace must have either spectra or numeric axis.");
}

IFunction_sptr lastFunction(IFunction_sptr function) {
  const auto composite =
      boost::dynamic_pointer_cast<CompositeFunction>(function);

  if (composite) {
    if (composite->nFunctions() > 0)
      return composite->getFunction(composite->nFunctions() - 1);
    return nullptr;
  }
  return function;
}

std::size_t numberOfFunctions(CompositeFunction_sptr composite,
                              const std::string &functionName) {
  std::size_t count = 0;
  for (auto i = 0u; i < composite->nFunctions(); ++i) {
    if (composite->getFunction(i)->name() == functionName)
      count += 1;
  }
  return count;
}

std::size_t numberOfFunctions(IFunction_sptr function,
                              const std::string &functionName) {
  const auto composite =
      boost::dynamic_pointer_cast<CompositeFunction>(function);
  if (composite)
    return numberOfFunctions(composite, functionName);
  return function->name() == functionName ? 1 : 0;
}

bool containsFunction(CompositeFunction_sptr composite,
                      const std::string &functionName) {
  for (auto i = 0u; i < composite->nFunctions(); ++i) {
    if (composite->getFunction(i)->name() == functionName)
      return true;
  }
  return false;
}

bool containsFunction(IFunction_sptr function,
                      const std::string &functionName) {
  const auto composite =
      boost::dynamic_pointer_cast<CompositeFunction>(function);
  if (composite)
    return containsFunction(composite, functionName);
  return function->name() == functionName;
}
} // namespace

namespace Mantid {
namespace Algorithms {

using namespace API;
using namespace Kernel;

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(ConvolutionFitSequential)

//----------------------------------------------------------------------------------------------

/// Algorithms name for identification. @see Algorithm::name
const std::string ConvolutionFitSequential::name() const {
  return "ConvolutionFitSequential";
}

/// Algorithm's version for identification. @see Algorithm::version
int ConvolutionFitSequential::version() const { return 1; }

/// Algorithm's category for identification. @see Algorithm::category
const std::string ConvolutionFitSequential::category() const {
  return "Workflow\\MIDAS";
}

/// Algorithm's summary for use in the GUI and help. @see Algorithm::summary
const std::string ConvolutionFitSequential::summary() const {
  return "Performs a sequential fit for a convolution workspace";
}

//----------------------------------------------------------------------------------------------
/** Initialize the algorithm's properties.
 */
void ConvolutionFitSequential::initConcrete() {
  std::vector<std::string> backType{"Fixed Flat", "Fit Flat", "Fit Linear"};

  declareProperty("BackgroundType", "Fixed Flat",
                  boost::make_shared<StringListValidator>(backType),
                  "The Type of background used in the fitting",
                  Direction::Input);
}

IValidator_sptr ConvolutionFitSequential::functionValidator() const {
  auto scv = boost::make_shared<StringContainsValidator>();
  auto requires = std::vector<std::string>{"Convolution", "Resolution"};
  scv->setRequiredStrings(requires);
  return scv;
}

void ConvolutionFitSequential::setup() {
  IFunction_sptr function = getProperty("Function");
  m_deltaUsed = containsFunction(function, "DeltaFunction");
  m_lorentzianCount = numberOfFunctions(function, "Lorentzian");
}

std::vector<std::string>
ConvolutionFitSequential::getFitParameterNames() const {
  std::vector<std::string> parameters;

  if (m_deltaUsed)
    parameters.emplace_back("Height");

  IFunction_sptr function = getProperty("Function");
  auto last = lastFunction(function);
  auto lastParameters = last->getParameterNames();
  if (last->name() == "Lorentzian") {
    auto position =
        find(lastParameters.begin(), lastParameters.end(), "PeakCentre");
    lastParameters.erase(position);
    parameters.emplace_back("EISF");
  }

  parameters.insert(std::end(parameters), std::begin(lastParameters),
                    std::end(lastParameters));
  return parameters;
}

std::vector<MatrixWorkspace_sptr>
ConvolutionFitSequential::getWorkspaces() const {
  auto workspaces = QENSFitSequential::getWorkspaces();

  for (auto i = 0u; i < workspaces.size(); ++i) {
    const std::string tempFitWsName = "__convfit_fit_ws" + std::to_string(i);
    workspaces[i] = convertInputToElasticQ(workspaces[i], tempFitWsName);
  }
  return workspaces;
}

ITableWorkspace_sptr
ConvolutionFitSequential::performFit(const std::string &input) {
  auto parameterWorkspace = QENSFitSequential::performFit(input);
  if (m_deltaUsed)
    calculateEISF(parameterWorkspace);
  return parameterWorkspace;
}

void ConvolutionFitSequential::postExec(API::MatrixWorkspace_sptr result) {
  auto sampleLogStrings = std::map<std::string, std::string>();
  sampleLogStrings["sample_filename"] = getPropertyValue("InputWorkspace");
  sampleLogStrings["convolve_members"] = getPropertyValue("ConvolveMembers");
  sampleLogStrings["fit_program"] = "ConvFit";
  sampleLogStrings["background"] = getPropertyValue("BackgroundType");
  sampleLogStrings["delta_function"] = m_deltaUsed ? "true" : "false";
  auto sampleLogNumeric = std::map<std::string, std::string>();
  sampleLogNumeric["lorentzians"] =
      boost::lexical_cast<std::string>(m_lorentzianCount);

  Progress logAdderProg(this, 0.99, 1.00, 6);
  // Add String Logs
  auto logAdder = createChildAlgorithm("AddSampleLog", -1.0, -1.0, false);
  for (auto &sampleLogString : sampleLogStrings) {
    logAdder->setProperty("Workspace", result);
    logAdder->setProperty("LogName", sampleLogString.first);
    logAdder->setProperty("LogText", sampleLogString.second);
    logAdder->setProperty("LogType", "String");
    logAdder->executeAsChildAlg();
    logAdderProg.report("Add text logs");
  }

  // Add Numeric Logs
  for (auto &logItem : sampleLogNumeric) {
    logAdder->setProperty("Workspace", result);
    logAdder->setProperty("LogName", logItem.first);
    logAdder->setProperty("LogText", logItem.second);
    logAdder->setProperty("LogType", "Number");
    logAdder->executeAsChildAlg();
    logAdderProg.report("Adding Numerical logs");
  }
}

/**
 * Searchs for a given fit parameter within the a vector of columnNames
 * @param suffix - The string to search for within the columnName
 * @param columns - A vector of column names to be searched through
 * @return A vector of all the column names that contained the given suffix
 * string
 */
std::vector<std::string> ConvolutionFitSequential::searchForFitParams(
    const std::string &suffix, const std::vector<std::string> &columns) {
  auto fitParams = std::vector<std::string>();
  const size_t totalColumns = columns.size();
  for (auto i = 0u; i < totalColumns; ++i) {
    auto pos = columns.at(i).rfind(suffix);
    if (pos != std::string::npos) {
      auto endCheck = pos + suffix.size();
      if (endCheck == columns.at(i).size()) {
        fitParams.push_back(columns.at(i));
      }
    }
  }
  return fitParams;
}

/**
 * Squares all the values inside a vector of type double
 * @param target - The vector to be squared
 * @return The vector after being squared
 */
std::vector<double>
ConvolutionFitSequential::squareVector(std::vector<double> target) {
  std::transform(target.begin(), target.end(), target.begin(),
                 VectorHelper::Squares<double>());
  return target;
}

/**
 * Creates a vector of type double using the values of another vector
 * @param original - The original vector to be cloned
 * @return A clone of the original vector
 */
std::vector<double>
ConvolutionFitSequential::cloneVector(const std::vector<double> &original) {
  return std::vector<double>(original.begin(), original.end());
}

/**
 * Calculates the EISF if the fit includes a Delta function
 * @param tableWs - The TableWorkspace to append the EISF calculation to
 */
void ConvolutionFitSequential::calculateEISF(ITableWorkspace_sptr &tableWs) {
  // Get height data from parameter table
  const auto columns = tableWs->getColumnNames();
  const auto height = searchForFitParams("Height", columns).at(0);
  const auto heightErr = searchForFitParams("Height_Err", columns).at(0);
  auto heightY = tableWs->getColumn(height)->numeric_fill<>();
  auto heightE = tableWs->getColumn(heightErr)->numeric_fill<>();

  // Get amplitude column names
  const auto ampNames = searchForFitParams("Amplitude", columns);
  const auto ampErrorNames = searchForFitParams("Amplitude_Err", columns);

  // For each lorentzian, calculate EISF
  size_t maxSize = ampNames.size();
  if (ampErrorNames.size() > maxSize) {
    maxSize = ampErrorNames.size();
  }
  for (size_t i = 0; i < maxSize; i++) {
    // Get amplitude from column in table workspace
    const auto ampName = ampNames.at(i);
    auto ampY = tableWs->getColumn(ampName)->numeric_fill<>();
    const auto ampErrorName = ampErrorNames.at(i);
    auto ampErr = tableWs->getColumn(ampErrorName)->numeric_fill<>();

    // Calculate EISF and EISF error
    // total = heightY + ampY
    auto total = cloneVector(heightY);
    std::transform(total.begin(), total.end(), ampY.begin(), total.begin(),
                   std::plus<double>());
    // eisfY = heightY / total
    auto eisfY = cloneVector(heightY);
    std::transform(eisfY.begin(), eisfY.end(), total.begin(), eisfY.begin(),
                   std::divides<double>());
    // heightE squared
    auto heightESq = cloneVector(heightE);
    heightESq = squareVector(heightESq);
    // ampErr squared
    auto ampErrSq = cloneVector(ampErr);
    ampErrSq = squareVector(ampErrSq);
    // totalErr = heightE squared + ampErr squared
    auto totalErr = cloneVector(heightESq);
    std::transform(totalErr.begin(), totalErr.end(), ampErrSq.begin(),
                   totalErr.begin(), std::plus<double>());
    // heightY squared
    auto heightYSq = cloneVector(heightY);
    heightYSq = squareVector(heightYSq);
    // total Squared
    auto totalSq = cloneVector(total);
    totalSq = squareVector(totalSq);
    // errOverTotalSq = totalErr / total squared
    auto errOverTotalSq = cloneVector(totalErr);
    std::transform(errOverTotalSq.begin(), errOverTotalSq.end(),
                   totalSq.begin(), errOverTotalSq.begin(),
                   std::divides<double>());
    // heightESqOverYSq = heightESq / heightYSq
    auto heightESqOverYSq = cloneVector(heightESq);
    std::transform(heightESqOverYSq.begin(), heightESqOverYSq.end(),
                   heightYSq.begin(), heightESqOverYSq.begin(),
                   std::divides<double>());
    // sqrtESqOverYSq = squareRoot( heightESqOverYSq )
    auto sqrtESqOverYSq = cloneVector(heightESqOverYSq);
    std::transform(sqrtESqOverYSq.begin(), sqrtESqOverYSq.end(),
                   sqrtESqOverYSq.begin(),
                   static_cast<double (*)(double)>(sqrt));
    // eisfYSumRoot = eisfY * sqrtESqOverYSq
    auto eisfYSumRoot = cloneVector(eisfY);
    std::transform(eisfYSumRoot.begin(), eisfYSumRoot.end(),
                   sqrtESqOverYSq.begin(), eisfYSumRoot.begin(),
                   std::multiplies<double>());
    // eisfErr = eisfYSumRoot + errOverTotalSq
    auto eisfErr = cloneVector(eisfYSumRoot);
    std::transform(eisfErr.begin(), eisfErr.end(), errOverTotalSq.begin(),
                   eisfErr.begin(), std::plus<double>());

    // Append the calculated values to the table workspace
    auto columnName =
        ampName.substr(0, (ampName.size() - std::string("Amplitude").size()));
    columnName += "EISF";
    auto errorColumnName = ampErrorName.substr(
        0, (ampErrorName.size() - std::string("Amplitude_Err").size()));
    errorColumnName += "EISF_Err";

    tableWs->addColumn("double", columnName);
    tableWs->addColumn("double", errorColumnName);
    size_t maxEisf = eisfY.size();
    if (eisfErr.size() > maxEisf) {
      maxEisf = eisfErr.size();
    }

    Column_sptr col = tableWs->getColumn(columnName);
    Column_sptr errCol = tableWs->getColumn(errorColumnName);
    for (size_t j = 0; j < maxEisf; j++) {
      col->cell<double>(j) = eisfY.at(j);
      errCol->cell<double>(j) = eisfErr.at(j);
    }
  }
}

/*
 * Extracts the convolution fit members from the specified result group
 * workspace, given the specified input workspace used for the fit, each into a
 * workspace, stored inside a group workspace of the specified name.
 *
 * @param inputWs       The input workspace used in the convolution fit.
 * @param resultGroupWs The result group workspace produced by the convolution
 *                      fit; from which to extract the members.
 * @param outputWsName  The name of the output group workspace to store the
 *                      member workspaces.
 */
void ConvolutionFitSequential::extractMembers(
    MatrixWorkspace_sptr inputWs, WorkspaceGroup_sptr resultGroupWs,
    const std::vector<std::string> &convolvedMembers,
    const std::string &outputWsName) {
  bool convolved = getProperty("Convolve");
  auto extractMembersAlg =
      AlgorithmManager::Instance().create("ExtractQENSMembers");
  extractMembersAlg->setProperty("InputWorkspace", inputWs);
  extractMembersAlg->setProperty("ResultWorkspace", resultGroupWs);
  extractMembersAlg->setProperty("OutputWorkspace", outputWsName);
  extractMembersAlg->setProperty("RenameConvolvedMembers", convolved);
  extractMembersAlg->setProperty("ConvolvedMembers", convolvedMembers);
  extractMembersAlg->execute();
}

} // namespace Algorithms
} // namespace Mantid
