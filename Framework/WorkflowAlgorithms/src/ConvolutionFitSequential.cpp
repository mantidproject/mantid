#include "MantidWorkflowAlgorithms/ConvolutionFitSequential.h"

#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/Axis.h"
#include "MantidAPI/FunctionDomain1D.h"
#include "MantidAPI/FunctionFactory.h"
#include "MantidAPI/IFunction.h"
#include "MantidAPI/ITableWorkspace.h"
#include "MantidAPI/Progress.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidAPI/WorkspaceGroup.h"

#include "MantidKernel/MandatoryValidator.h"
#include "MantidKernel/BoundedValidator.h"
#include "MantidKernel/ListValidator.h"
#include "MantidKernel/StringContainsValidator.h"
#include "MantidKernel/VectorHelper.h"

#include <cmath>

namespace {
Mantid::Kernel::Logger g_log("ConvolutionFitSequential");
}

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
void ConvolutionFitSequential::init() {
  declareProperty(
      make_unique<WorkspaceProperty<>>("InputWorkspace", "", Direction::Input),
      "The input workspace for the fit.");

  auto scv = boost::make_shared<StringContainsValidator>();
  auto requires = std::vector<std::string>{"Convolution", "Resolution"};
  scv->setRequiredStrings(requires);

  declareProperty("Function", "", scv,
                  "The function that describes the parameters of the fit.",
                  Direction::Input);

  std::vector<std::string> backType{"Fixed Flat", "Fit Flat", "Fit Linear"};

  declareProperty("BackgroundType", "Fixed Flat",
                  boost::make_shared<StringListValidator>(backType),
                  "The Type of background used in the fitting",
                  Direction::Input);

  declareProperty(
      "StartX", EMPTY_DBL(), boost::make_shared<MandatoryValidator<double>>(),
      "The start of the range for the fit function.", Direction::Input);

  declareProperty(
      "EndX", EMPTY_DBL(), boost::make_shared<MandatoryValidator<double>>(),
      "The end of the range for the fit function.", Direction::Input);

  auto boundedV = boost::make_shared<BoundedValidator<int>>();
  boundedV->setLower(0);

  declareProperty("SpecMin", 0, boundedV, "The first spectrum to be used in "
                                          "the fit. Spectra values can not be "
                                          "negative",
                  Direction::Input);

  declareProperty("SpecMax", 0, boundedV, "The final spectrum to be used in "
                                          "the fit. Spectra values can not be "
                                          "negative",
                  Direction::Input);

  declareProperty("Convolve", true,
                  "If true, the fit is treated as a convolution workspace.",
                  Direction::Input);

  declareProperty("Minimizer", "Levenberg-Marquardt",
                  boost::make_shared<MandatoryValidator<std::string>>(),
                  "Minimizer to use for fitting. Minimizers available are "
                  "'Levenberg-Marquardt', 'Simplex', 'FABADA',\n"
                  "'Conjugate gradient (Fletcher-Reeves imp.)', 'Conjugate "
                  "gradient (Polak-Ribiere imp.)' and 'BFGS'");

  declareProperty("MaxIterations", 500, boundedV,
                  "The maximum number of iterations permitted",
                  Direction::Input);
  declareProperty("PeakRadius", 0,
                  "A value of the peak radius the peak functions should use. A "
                  "peak radius defines an interval on the x axis around the "
                  "centre of the peak where its values are calculated. Values "
                  "outside the interval are not calculated and assumed zeros."
                  "Numerically the radius is a whole number of peak widths "
                  "(FWHM) that fit into the interval on each side from the "
                  "centre. The default value of 0 means the whole x axis.");

  declareProperty(make_unique<WorkspaceProperty<>>("OutputWorkspace", "",
                                                   Direction::Output),
                  "The OutputWorkspace containing the results of the fit.");
}

//----------------------------------------------------------------------------------------------
/** Execute the algorithm.
 */
void ConvolutionFitSequential::exec() {
  // Initialise variables with properties
  MatrixWorkspace_sptr inputWs = getProperty("InputWorkspace");
  const std::string function = getProperty("Function");
  const std::string backType =
      convertBackToShort(getProperty("backgroundType"));
  const double startX = getProperty("StartX");
  const double endX = getProperty("EndX");
  const int specMin = getProperty("SpecMin");
  const int specMax = getProperty("Specmax");
  const bool convolve = getProperty("Convolve");
  const int maxIter = getProperty("MaxIterations");
  const std::string minimizer = getProperty("Minimizer");
  const int peakRadius = getProperty("PeakRadius");

  // Inspect function to obtain fit Type and background
  const auto functionValues = findValuesFromFunction(function);
  const auto LorentzNum = functionValues[0];
  const auto funcName = functionValues[1];

  // Check if a delta function is being used
  auto delta = false;
  std::string usingDelta = "false";
  auto pos = function.find("Delta");
  if (pos != std::string::npos) {
    delta = true;
    usingDelta = "true";
  }

  // Log information to result log
  m_log.information("Input files: " + inputWs->getName());
  m_log.information("Fit type: Delta=" + usingDelta + "; Lorentzians=" +
                    LorentzNum);
  m_log.information("Background type: " + backType);

  // Output workspace name
  auto outputWsName = inputWs->getName();
  pos = outputWsName.rfind('_');
  if (pos != std::string::npos) {
    outputWsName = outputWsName.substr(0, pos + 1);
  }
  outputWsName += "conv_";
  if (delta) {
    outputWsName += "Delta";
  }
  if (LorentzNum != "0") {
    outputWsName += LorentzNum + "L";
  } else {
    outputWsName += convertFuncToShort(funcName);
  }
  outputWsName += backType + "_s";
  outputWsName += std::to_string(specMin);
  outputWsName += "_to_";
  outputWsName += std::to_string(specMax);

  // Convert input workspace to get Q axis
  const std::string tempFitWsName = "__convfit_fit_ws";
  convertInputToElasticQ(inputWs, tempFitWsName);

  Progress plotPeakStringProg(this, 0.0, 0.05, specMax - specMin);
  // Construct plotpeak string
  std::string plotPeakInput;
  for (int i = specMin; i < specMax + 1; i++) {
    auto nextWs = tempFitWsName + ",i";
    nextWs += std::to_string(i);
    plotPeakInput += nextWs + ";";
    plotPeakStringProg.report("Constructing PlotPeak name");
  }

  // passWSIndex
  auto passIndex = false;
  if (funcName.find("Diff") != std::string::npos ||
      funcName.find("Stretched") != std::string::npos) {
    passIndex = true;
  }

  // Run PlotPeaksByLogValue
  auto plotPeaks = createChildAlgorithm("PlotPeakByLogValue", 0.05, 0.90, true);
  plotPeaks->setProperty("Input", plotPeakInput);
  plotPeaks->setProperty("OutputWorkspace", outputWsName);
  plotPeaks->setProperty("Function", function);
  plotPeaks->setProperty("StartX", startX);
  plotPeaks->setProperty("EndX", endX);
  plotPeaks->setProperty("FitType", "Sequential");
  plotPeaks->setProperty("CreateOutput", true);
  plotPeaks->setProperty("OutputCompositeMembers", true);
  plotPeaks->setProperty("ConvolveMembers", convolve);
  plotPeaks->setProperty("MaxIterations", maxIter);
  plotPeaks->setProperty("Minimizer", minimizer);
  plotPeaks->setProperty("PassWSIndexToFunction", passIndex);
  plotPeaks->setProperty("PeakRadius", peakRadius);
  plotPeaks->executeAsChildAlg();
  ITableWorkspace_sptr outputWs = plotPeaks->getProperty("OutputWorkspace");

  // Delete workspaces
  Progress deleteProgress(this, 0.90, 0.91, 2);
  auto deleter = createChildAlgorithm("DeleteWorkspace", -1.0, -1.0, false);
  deleter->setProperty("WorkSpace",
                       outputWsName + "_NormalisedCovarianceMatrices");
  deleter->executeAsChildAlg();
  deleteProgress.report("Deleting PlotPeak Output");

  deleter->setProperty("WorkSpace", outputWsName + "_Parameters");
  deleter->executeAsChildAlg();
  deleteProgress.report("Deleting PlotPeak Output");

  const auto paramTableName = outputWsName + "_Parameters";
  AnalysisDataService::Instance().add(paramTableName, outputWs);

  // Construct output workspace
  const auto resultWsName = outputWsName + "_Result";

  Progress workflowProg(this, 0.91, 0.94, 4);
  auto paramNames = std::vector<std::string>();
  if (funcName == "DeltaFunction") {
    paramNames.emplace_back("Height");
  } else {
    auto func = FunctionFactory::Instance().createFunction(funcName);
    if (delta) {
      paramNames.emplace_back("Height");
    }
    for (size_t i = 0; i < func->nParams(); i++) {
      paramNames.push_back(func->parameterName(i));
      workflowProg.report("Finding parameters to process");
    }
    if (funcName == "Lorentzian") {
      // remove peak centre
      size_t pos = find(paramNames.begin(), paramNames.end(), "PeakCentre") -
                   paramNames.begin();
      paramNames.erase(paramNames.begin() + pos);
      paramNames.emplace_back("EISF");
    }
  }

  // Run calcEISF if Delta
  if (delta) {
    calculateEISF(outputWs);
  }

  // Construct comma separated list for ProcessIndirectFitParameters
  std::string paramNamesList;
  const size_t maxNames = paramNames.size();
  for (size_t i = 0; i < maxNames; i++) {
    paramNamesList += paramNames.at(i);
    if (i != (maxNames - 1)) {
      paramNamesList += ",";
    }
    workflowProg.report("Constructing indirectFitParams input");
  }

  // Run ProcessIndirectFitParameters
  auto pifp =
      createChildAlgorithm("ProcessIndirectFitParameters", 0.94, 0.96, true);
  pifp->setProperty("InputWorkspace", outputWs);
  pifp->setProperty("ColumnX", "axis-1");
  pifp->setProperty("XAxisUnit", "MomentumTransfer");
  pifp->setProperty("ParameterNames", paramNamesList);
  pifp->setProperty("OutputWorkspace", resultWsName);
  pifp->executeAsChildAlg();

  MatrixWorkspace_sptr resultWs = pifp->getProperty("OutputWorkspace");
  AnalysisDataService::Instance().addOrReplace(resultWsName, resultWs);

  // Handle sample logs
  auto logCopier = createChildAlgorithm("CopyLogs", -1.0, -1.0, false);
  logCopier->setProperty("InputWorkspace", inputWs);
  logCopier->setProperty("OutputWorkspace", resultWs);
  logCopier->executeAsChildAlg();

  resultWs = logCopier->getProperty("OutputWorkspace");

  // Create Sample Log
  auto sampleLogStrings = std::map<std::string, std::string>();
  sampleLogStrings["sample_filename"] = inputWs->getName();
  sampleLogStrings["convolve_members"] = (convolve == 1) ? "true" : "false";
  sampleLogStrings["fit_program"] = "ConvFit";
  sampleLogStrings["background"] = backType;
  sampleLogStrings["delta_function"] = usingDelta;
  auto sampleLogNumeric = std::map<std::string, std::string>();
  sampleLogNumeric["lorentzians"] =
      boost::lexical_cast<std::string>(LorentzNum);

  Progress logAdderProg(this, 0.96, 0.97, 6);
  // Add String Logs
  auto logAdder = createChildAlgorithm("AddSampleLog", -1.0, -1.0, false);
  for (auto &sampleLogString : sampleLogStrings) {
    logAdder->setProperty("Workspace", resultWs);
    logAdder->setProperty("LogName", sampleLogString.first);
    logAdder->setProperty("LogText", sampleLogString.second);
    logAdder->setProperty("LogType", "String");
    logAdder->executeAsChildAlg();
    logAdderProg.report("Add text logs");
  }

  // Add Numeric Logs
  for (auto &logItem : sampleLogNumeric) {
    logAdder->setProperty("Workspace", resultWs);
    logAdder->setProperty("LogName", logItem.first);
    logAdder->setProperty("LogText", logItem.second);
    logAdder->setProperty("LogType", "Number");
    logAdder->executeAsChildAlg();
    logAdderProg.report("Adding Numerical logs");
  }

  // Copy Logs to GroupWorkspace
  logCopier = createChildAlgorithm("CopyLogs", 0.97, 0.98, false);
  logCopier->setProperty("InputWorkspace", resultWs);
  std::string groupName = outputWsName + "_Workspaces";
  logCopier->setProperty("OutputWorkspace", groupName);
  logCopier->executeAsChildAlg();

  // Rename Workspaces in group
  WorkspaceGroup_sptr groupWs =
      AnalysisDataService::Instance().retrieveWS<WorkspaceGroup>(outputWsName +
                                                                 "_Workspaces");
  const auto groupWsNames = groupWs->getNames();
  auto renamer = createChildAlgorithm("RenameWorkspace", -1.0, -1.0, false);
  Progress renamerProg(this, 0.98, 1.0, specMax + 1);
  for (int i = specMin; i < specMax + 1; i++) {
    renamer->setProperty("InputWorkspace", groupWsNames.at(i - specMin));
    auto outName = outputWsName + "_";
    outName += std::to_string(i);
    outName += "_Workspace";
    renamer->setProperty("OutputWorkspace", outName);
    renamer->executeAsChildAlg();
    renamerProg.report("Renaming group workspaces");
  }

  setProperty("OutputWorkspace", resultWs);
}

/**
 * Check function to establish if it is for one lorentzian or Two
 * @param subFunction The unchecked substring of the function
 * @return true if the function is two lorentzian false if one lorentzian
 */
bool ConvolutionFitSequential::checkForTwoLorentz(
    const std::string &subFunction) {
  auto pos = subFunction.rfind("Lorentzian");
  return pos != std::string::npos;
}

/**
 * Finds specific values embedded in the function supplied to the algorithm
 * @param function The full function string
 * @return all values of interest from the function (0 - fitType,  1 -
 * functionName)
 */
std::vector<std::string>
ConvolutionFitSequential::findValuesFromFunction(const std::string &function) {
  std::vector<std::string> result;
  std::string fitType;
  std::string functionName;
  auto startPos = function.rfind("name=");
  if (startPos != std::string::npos) {
    fitType = function.substr(startPos, function.size());
    auto nextPos = fitType.find_first_of(',');
    fitType = fitType.substr(5, nextPos - 5);
    functionName = fitType;
    if (fitType == "Lorentzian") {
      std::string newSub = function.substr(0, startPos);
      bool isTwoL = checkForTwoLorentz(newSub);
      if (isTwoL) {
        fitType = "2";
      } else {
        fitType = "1";
      }
    } else {
      fitType = "0";
    }
    result.push_back(fitType);
  }

  result.push_back(functionName);
  return result;
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
  for (size_t i = 0; i < totalColumns; i++) {
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
 * Converts the input workspaces to spectrum axis to ElasticQ and adds it to the
 * ADS to be used by PlotPeakBylogValue
 * @param inputWs - The MatrixWorkspace to be converted
 * @param wsName - The desired name of the output workspace
 */
void ConvolutionFitSequential::convertInputToElasticQ(
    API::MatrixWorkspace_sptr &inputWs, const std::string &wsName) {
  auto axis = inputWs->getAxis(1);
  if (axis->isSpectra()) {
    auto convSpec = createChildAlgorithm("ConvertSpectrumAxis");
    // Store in ADS to allow use by PlotPeakByLogValue
    convSpec->setAlwaysStoreInADS(true);
    convSpec->setProperty("InputWorkSpace", inputWs);
    convSpec->setProperty("OutputWorkSpace", wsName);
    convSpec->setProperty("Target", "ElasticQ");
    convSpec->setProperty("EMode", "Indirect");
    convSpec->executeAsChildAlg();
  } else if (axis->isNumeric()) {
    // Check that units are Momentum Transfer
    if (axis->unit()->unitID() != "MomentumTransfer") {
      throw std::runtime_error("Input must have axis values of Q");
    }
    auto cloneWs = createChildAlgorithm("CloneWorkspace");
    // Store in ADS to allow use by PlotPeakByLogValue
    cloneWs->setAlwaysStoreInADS(true);
    cloneWs->setProperty("InputWorkspace", inputWs);
    cloneWs->setProperty("OutputWorkspace", wsName);
    cloneWs->executeAsChildAlg();
  } else {
    throw std::runtime_error(
        "Input workspace must have either spectra or numeric axis.");
  }
}

/**
 * Calculates the EISF if the fit includes a Delta function
 * @param tableWs - The TableWorkspace to append the EISF calculation to
 */
void ConvolutionFitSequential::calculateEISF(
    API::ITableWorkspace_sptr &tableWs) {
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
        0, (ampName.size() - std::string("Amplitude_Err").size()));
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

/**
 * Converts the user input for background into short hand for use in the
 * workspace naming
 * @param original - The original user input to the function
 * @return The short hand of the users input
 */
std::string
ConvolutionFitSequential::convertBackToShort(const std::string &original) {
  auto result = original.substr(0, 3);
  const auto pos = original.find(' ');
  if (pos != std::string::npos) {
    result += original.at(pos + 1);
  }
  return result;
}

/**
 * Converts the user input for function into short hand for use in the workspace
 * naming
 * @param original - The original user input to the function
 * @return The short hand of the users input
 */
std::string
ConvolutionFitSequential::convertFuncToShort(const std::string &original) {
  std::string result;
  if (original != "DeltaFunction") {
    if (original.at(0) == 'E') {
      result += "E";
    } else if (original.at(0) == 'I') {
      result += "I";
    } else {
      return "SFT";
    }
    const auto pos = original.find("Circle");
    if (pos != std::string::npos) {
      result += "DC";
    } else {
      result += "DS";
    }
  }
  return result;
}

} // namespace Algorithms
} // namespace Mantid