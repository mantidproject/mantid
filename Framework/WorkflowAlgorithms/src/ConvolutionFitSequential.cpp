#include "MantidWorkflowAlgorithms/ConvolutionFitSequential.h"

#include "MantidAPI/AlgorithmManager.h"
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

#include <algorithm>
#include <cmath>

namespace {
Mantid::Kernel::Logger g_log("ConvolutionFitSequential");

using namespace Mantid::API;
using namespace Mantid::Kernel;

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

bool containsFunction(IFunction_sptr function, const std::string &functionName);

bool containsFunction(CompositeFunction_sptr composite,
                      const std::string &functionName) {
  for (auto i = 0u; i < composite->nFunctions(); ++i) {
    if (containsFunction(composite->getFunction(i), functionName))
      return true;
  }
  return false;
}

bool containsFunction(IFunction_sptr function,
                      const std::string &functionName) {
  const auto composite =
      boost::dynamic_pointer_cast<CompositeFunction>(function);
  if (function->name() == functionName)
    return true;
  else if (composite)
    return containsFunction(composite, functionName);
  return false;
}

template <typename T, typename... Ts>
std::vector<T, Ts...> cloneVector(const std::vector<T, Ts...> &vec) {
  return std::vector<double>(vec.begin(), vec.end());
}

template <typename T, typename F, typename... Ts>
std::vector<T, Ts...> combineVectors(const std::vector<T, Ts...> &vec,
                                     const std::vector<T, Ts...> &vec2,
                                     F const &combinator) {
  auto combined = cloneVector(vec);
  std::transform(vec.begin(), vec.end(), vec2.begin(), combined.begin(),
                 combinator);
  return combined;
}

template <typename T, typename... Ts>
std::vector<T, Ts...> divideVectors(const std::vector<T, Ts...> &dividend,
                                    const std::vector<T, Ts...> &divisor) {
  return combineVectors(dividend, divisor, std::divides<T>());
}

template <typename T, typename... Ts>
std::vector<double> addVectors(const std::vector<T, Ts...> &vec,
                               const std::vector<T, Ts...> &vec2) {
  return combineVectors(vec, vec2, std::plus<T>());
}

template <typename T, typename... Ts>
std::vector<double> multiplyVectors(const std::vector<T, Ts...> &vec,
                                    const std::vector<T, Ts...> &vec2) {
  return combineVectors(vec, vec2, std::multiplies<T>());
}

template <typename T, typename... Ts>
std::vector<T, Ts...> squareVector(const std::vector<T, Ts...> &vec) {
  auto target = cloneVector(vec);
  std::transform(target.begin(), target.end(), target.begin(),
                 VectorHelper::Squares<T>());
  return target;
}

template <typename T, typename... Ts>
std::vector<T, Ts...> squareRootVector(const std::vector<T, Ts...> &vec) {
  auto target = cloneVector(vec);
  std::transform(target.begin(), target.end(), target.begin(),
                 static_cast<T (*)(T)>(sqrt));
  return target;
}

IFunction_sptr extractFirstBackground(IFunction_sptr function);

IFunction_sptr extractFirstBackground(CompositeFunction_sptr composite) {
  for (auto i = 0u; i < composite->nFunctions(); ++i) {
    auto background = extractFirstBackground(composite->getFunction(i));
    if (background)
      return background;
  }
  return nullptr;
}

IFunction_sptr extractFirstBackground(IFunction_sptr function) {
  auto composite = boost::dynamic_pointer_cast<CompositeFunction>(function);

  if (composite)
    return extractFirstBackground(composite);
  else if (function->category() == "Background")
    return function;
  return nullptr;
}

std::string extractBackgroundType(IFunction_sptr function) {
  auto background = extractFirstBackground(function);
  if (!background)
    return "None";

  auto backgroundType = background->name();
  auto position = backgroundType.rfind(" Background");

  if (position != std::string::npos)
    backgroundType = backgroundType.substr(0, position);

  if (background->isFixed(0))
    backgroundType = "Fixed " + backgroundType;
  else
    backgroundType = "Fit " + backgroundType;
  return backgroundType;
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

std::map<std::string, std::string> ConvolutionFitSequential::validateInputs() {
  std::map<std::string, std::string> errors;
  IFunction_sptr function = getProperty("Function");
  if (!containsFunction(function, "Convolution") ||
      !containsFunction(function, "Resolution"))
    errors["Function"] = "Function provided does not contain convolution with "
                         "a resolution function.";
  m_deltaUsed = containsFunction(function, "DeltaFunction");
  m_lorentzianCount = numberOfFunctions(function, "Lorentzian");
  return errors;
}

bool ConvolutionFitSequential::throwIfElasticQConversionFails() const {
  return true;
}

bool ConvolutionFitSequential::isFitParameter(const std::string &name) const {
  return name.rfind("Centre") == std::string::npos;
}

ITableWorkspace_sptr
ConvolutionFitSequential::performFit(const std::string &input,
                                     const std::string &output) {
  auto parameterWorkspace = QENSFitSequential::performFit(input, output);
  if (m_deltaUsed)
    calculateEISF(parameterWorkspace);
  return parameterWorkspace;
}

void ConvolutionFitSequential::postExec(API::MatrixWorkspace_sptr result) {
  IFunction_sptr function = getProperty("Function");
  auto logStrings = std::map<std::string, std::string>();
  logStrings["sample_filename"] = getPropertyValue("InputWorkspace");
  logStrings["convolve_members"] = getPropertyValue("ConvolveMembers");
  logStrings["fit_program"] = "ConvFit";
  logStrings["background"] = extractBackgroundType(function);
  logStrings["delta_function"] = m_deltaUsed ? "true" : "false";
  logStrings["fit_mode"] = "Sequential";

  auto logNumbers = std::map<std::string, std::string>();
  logNumbers["lorentzians"] =
      boost::lexical_cast<std::string>(m_lorentzianCount);

  Progress logAdderProg(this, 0.99, 1.00, 6);
  // Add String Logs
  auto logAdder = createChildAlgorithm("AddSampleLog", -1.0, -1.0, false);
  for (auto &sampleLogString : logStrings) {
    logAdder->setProperty("Workspace", result);
    logAdder->setProperty("LogName", sampleLogString.first);
    logAdder->setProperty("LogText", sampleLogString.second);
    logAdder->setProperty("LogType", "String");
    logAdder->executeAsChildAlg();
    logAdderProg.report("Add text logs");
  }

  // Add Numeric Logs
  for (auto &logItem : logNumbers) {
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
  for (auto i = 0u; i < maxSize; i++) {
    // Get amplitude from column in table workspace
    const auto ampName = ampNames.at(i);
    auto ampY = tableWs->getColumn(ampName)->numeric_fill<>();
    const auto ampErrorName = ampErrorNames.at(i);
    auto ampErr = tableWs->getColumn(ampErrorName)->numeric_fill<>();

    // Calculate EISF and EISF error
    auto total = addVectors(heightY, ampY);
    auto eisfY = divideVectors(heightY, total);

    auto heightESq = squareVector(heightE);

    auto ampErrSq = squareVector(ampErr);
    auto totalErr = addVectors(heightESq, ampErr);

    auto heightYSq = squareVector(heightY);
    auto totalSq = squareVector(total);

    auto errOverTotalSq = divideVectors(totalErr, totalSq);
    auto heightESqOverYSq = divideVectors(heightESq, heightYSq);

    auto sqrtESqOverYSq = squareRootVector(heightESqOverYSq);
    auto eisfYSumRoot = multiplyVectors(eisfY, sqrtESqOverYSq);

    auto eisfErr = addVectors(eisfYSumRoot, errOverTotalSq);

    // Append the calculated values to the table workspace
    auto columnName =
        ampName.substr(0, (ampName.size() - std::string("Amplitude").size()));
    columnName += "EISF";
    auto errorColumnName = ampErrorName.substr(
        0, (ampErrorName.size() - std::string("Amplitude_Err").size()));
    errorColumnName += "EISF_Err";

    tableWs->addColumn("double", columnName);
    tableWs->addColumn("double", errorColumnName);
    auto maxEisf = eisfY.size();
    if (eisfErr.size() > maxEisf) {
      maxEisf = eisfErr.size();
    }

    auto col = tableWs->getColumn(columnName);
    auto errCol = tableWs->getColumn(errorColumnName);
    for (auto j = 0u; j < maxEisf; j++) {
      col->cell<double>(j) = eisfY.at(j);
      errCol->cell<double>(j) = eisfErr.at(j);
    }
  }
}

} // namespace Algorithms
} // namespace Mantid
