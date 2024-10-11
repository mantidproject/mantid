// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidAlgorithms/GeneratePythonFitScript.h"

#include "MantidAPI/ADSValidator.h"
#include "MantidAPI/CostFunctionFactory.h"
#include "MantidAPI/FileProperty.h"
#include "MantidAPI/FuncMinimizerFactory.h"
#include "MantidAPI/FunctionProperty.h"
#include "MantidAPI/MultiDomainFunction.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/BoundedValidator.h"
#include "MantidKernel/ConfigService.h"
#include "MantidKernel/ListValidator.h"
#include "MantidKernel/StartsWithValidator.h"

#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/detail/classification.hpp>
#include <boost/algorithm/string/split.hpp>

#include <fstream>
#include <streambuf>
#include <string>

using namespace Mantid::API;
using namespace Mantid::Kernel;

namespace {

template <typename T> std::string joinVector(std::vector<T> const &vec, std::string const &delimiter = ", ") {
  std::stringstream ss;
  std::copy(vec.cbegin(), vec.cend(), std::ostream_iterator<T>(ss, delimiter.c_str()));
  auto const str = ss.str();
  return str.substr(0, str.size() - delimiter.size());
}

std::string constructInputListEntry(std::string const &workspaceName, std::size_t const &workspaceIndex,
                                    double const startX, double const endX) {
  return "(\"" + workspaceName + "\", " + std::to_string(workspaceIndex) + ", " + std::to_string(startX) + ", " +
         std::to_string(endX) + ")";
}

std::string constructInputList(std::vector<std::string> const &inputWorkspaces,
                               std::vector<std::size_t> const &workspaceIndices, std::vector<double> const &startXs,
                               std::vector<double> const &endXs) {
  std::vector<std::string> entries;
  entries.reserve(inputWorkspaces.size());
  for (auto i = 0u; i < inputWorkspaces.size(); ++i)
    entries.emplace_back(constructInputListEntry(inputWorkspaces[i], workspaceIndices[i], startXs[i], endXs[i]));

  return "[\n    " + joinVector(entries, ",\n    ") + "\n]";
}

std::vector<std::string> splitStringBy(std::string const &str, std::string const &delimiter) {
  std::vector<std::string> subStrings;
  boost::split(subStrings, str, boost::is_any_of(delimiter));
  subStrings.erase(std::remove_if(subStrings.begin(), subStrings.end(),
                                  [](std::string const &subString) { return subString.empty(); }),
                   subStrings.end());
  return subStrings;
}

void replaceAll(std::string &str, std::string const &remove, std::string const &insert) {
  std::string::size_type pos = 0;
  while ((pos = str.find(remove, pos)) != std::string::npos) {
    str.replace(pos, remove.size(), insert);
    pos++;
  }
}

std::string getFileContents(std::string const &filename) {
  auto const directory = ConfigService::Instance().getString("python.templates.directory");

  std::ifstream filestream(directory + "/" + filename);
  if (!filestream) {
    filestream.close();
    throw std::runtime_error("Error occured when attempting to load file: " + filename);
  }

  std::string fileText((std::istreambuf_iterator<char>(filestream)), std::istreambuf_iterator<char>());
  filestream.close();
  return fileText;
}

} // namespace

namespace Mantid::Algorithms {

DECLARE_ALGORITHM(GeneratePythonFitScript)

std::string const GeneratePythonFitScript::name() const { return "GeneratePythonFitScript"; }

int GeneratePythonFitScript::version() const { return 1; }

std::string const GeneratePythonFitScript::category() const { return "Utility\\Python"; }

std::string const GeneratePythonFitScript::summary() const {
  return "An algorithm to generate a Python script file for performing a sequential or simultaneous fit.";
}

std::vector<std::string> const GeneratePythonFitScript::seeAlso() const { return {"Fit", "GeneratePythonScript"}; }

void GeneratePythonFitScript::init() {
  auto mustBePositive = std::make_shared<BoundedValidator<int>>();
  mustBePositive->setLower(0);

  declareProperty(std::make_unique<ArrayProperty<std::string>>("InputWorkspaces", std::make_shared<ADSValidator>(),
                                                               Direction::Input),
                  "A list of workspace names to be fitted. The workspace name at index i in the list corresponds with "
                  "the 'WorkspaceIndices', 'StartXs' and 'EndXs' properties.");
  declareProperty(std::make_unique<ArrayProperty<std::size_t>>("WorkspaceIndices", Direction::Input),
                  "A list of workspace indices to be fitted. The workspace index at index i in the list will "
                  "correspond to the input workspace at index i.");

  declareProperty(std::make_unique<ArrayProperty<double>>("StartXs", Direction::Input),
                  "A list of start X's to be used for the fitting. The Start X at index i will correspond to the input "
                  "workspace at index i.");

  declareProperty(std::make_unique<ArrayProperty<double>>("EndXs", Direction::Input),
                  "A list of end X's to be used for the fitting. The End X at index i will correspond to the input "
                  "workspace at index i.");

  auto const fittingTypes = std::vector<std::string>{"Sequential", "Simultaneous"};
  auto const fittingTypesValidator = std::make_shared<ListValidator<std::string>>(fittingTypes);
  declareProperty("FittingType", "Sequential", fittingTypesValidator,
                  "The type of fitting to generate a python script for (Sequential or Simultaneous).",
                  Direction::Input);

  declareProperty(
      std::make_unique<FunctionProperty>("Function", Direction::Input),
      "The function to use for the fitting. This should be a single domain function if the Python script will be for "
      "sequential fitting, or a MultiDomainFunction if the Python script is for simultaneous fitting.");

  declareProperty("MaxIterations", 500, mustBePositive->clone(),
                  "The MaxIterations to be passed to the Fit algorithm in the Python script.", Direction::Input);

  auto const minimizerOptions = FuncMinimizerFactory::Instance().getKeys();
  auto const minimizerValidator = std::make_shared<ListValidator<std::string>>(minimizerOptions);
  declareProperty("Minimizer", "Levenberg-Marquardt", minimizerValidator,
                  "The Minimizer to be passed to the Fit algorithm in the Python script.", Direction::Input);

  auto const costFunctionOptions = CostFunctionFactory::Instance().getKeys();
  auto const costFunctionValidator = std::make_shared<ListValidator<std::string>>(costFunctionOptions);
  declareProperty("CostFunction", "Least squares", costFunctionValidator,
                  "The CostFunction to be passed to the Fit algorithm in the Python script.", Direction::Input);

  std::array<std::string, 2> evaluationTypes = {{"CentrePoint", "Histogram"}};
  declareProperty("EvaluationType", "CentrePoint", IValidator_sptr(new ListValidator<std::string>(evaluationTypes)),
                  "The EvaluationType to be passed to the Fit algorithm in the Python script.", Direction::Input);

  declareProperty("OutputBaseName", "Output_Fit",
                  "The OutputBaseName is the base output name to use for the resulting Fit workspaces.");

  declareProperty(
      "PlotOutput", true,
      "If true, code used for plotting the results of a fit will be generated and added to the python script.");

  std::vector<std::string> extensions{".py"};
  declareProperty(std::make_unique<FileProperty>("Filepath", "", FileProperty::OptionalSave, extensions),
                  "The name of the Python fit script which will be generated and saved in the selected location.");

  declareProperty("ScriptText", "", Direction::Output);
}

std::map<std::string, std::string> GeneratePythonFitScript::validateInputs() {
  std::vector<std::string> const inputWorkspaces = getProperty("InputWorkspaces");
  std::vector<std::size_t> const workspaceIndices = getProperty("WorkspaceIndices");
  std::vector<double> const startXs = getProperty("StartXs");
  std::vector<double> const endXs = getProperty("EndXs");
  auto const fittingType = getPropertyValue("FittingType");
  IFunction_sptr function = getProperty("Function");
  std::string const outputBaseName = getProperty("OutputBaseName");

  std::map<std::string, std::string> errors;
  if (workspaceIndices.size() != inputWorkspaces.size())
    errors["WorkspaceIndices"] = "The number of workspace indices must be equal to the number of input workspaces.";
  if (startXs.size() != inputWorkspaces.size())
    errors["StartXs"] = "The number of Start Xs must be equal to the number of input workspaces.";
  if (endXs.size() != inputWorkspaces.size())
    errors["EndXs"] = "The number of End Xs must be equal to the number of input workspaces.";
  if (fittingType == "Sequential") {
    if (std::dynamic_pointer_cast<MultiDomainFunction>(function))
      errors["Function"] = "The Function cannot be a MultiDomainFunction when in Sequential fit mode.";
  }
  if (fittingType == "Simultaneous") {
    if (getNumberOfDomainsInFunction(function) != inputWorkspaces.size())
      errors["Function"] = "The Function provided does not have the same number of domains as there are input "
                           "workspaces. This is a requirement for Simultaneous fitting.";
  }
  if (outputBaseName.empty())
    errors["OutputBaseName"] = "The OutputBaseName is empty, please provide a base name for the output fit.";

  return errors;
}

void GeneratePythonFitScript::exec() {
  auto const fittingType = getPropertyValue("FittingType");
  auto const generatedScript = generateFitScript(fittingType);

  auto const filepath = getPropertyValue("Filepath");
  if (!filepath.empty())
    savePythonScript(filepath, generatedScript);

  setProperty("ScriptText", generatedScript);
}

std::size_t GeneratePythonFitScript::getNumberOfDomainsInFunction(IFunction_sptr const &function) const {
  if (!function)
    return 0u;
  if (auto const multiDomainFunction = std::dynamic_pointer_cast<MultiDomainFunction>(function))
    return multiDomainFunction->getNumberDomains();
  return 1u;
}

std::string GeneratePythonFitScript::generateFitScript(std::string const &fittingType) const {
  std::string generatedScript;
  if (fittingType == "Sequential") {
    generatedScript += generateVariableSetupCode("GeneratePythonFitScript_SequentialVariableSetup.py.in");
    generatedScript += "\n";
    generatedScript += getFileContents("GeneratePythonFitScript_SequentialFit.py.in");
  } else if (fittingType == "Simultaneous") {
    generatedScript += generateVariableSetupCode("GeneratePythonFitScript_SimultaneousVariableSetup.py.in");
    generatedScript += "\n";
    generatedScript += generateSimultaneousFitCode();
  }

  bool plotOutput = getProperty("PlotOutput");
  if (plotOutput) {
    generatedScript += "\n";
    std::vector<double> const startXs = getProperty("StartXs");
    if (startXs.size() == 1u) {
      generatedScript += getFileContents("GeneratePythonFitScript_PlottingSingleOutput.py.in");
    } else {
      generatedScript += getFileContents("GeneratePythonFitScript_PlottingMultiOutput.py.in");
    }
  }

  return generatedScript;
}

std::string GeneratePythonFitScript::generateVariableSetupCode(std::string const &filename) const {
  std::string code = getFileContents(filename);

  std::vector<std::string> const inputWorkspaces = getProperty("InputWorkspaces");
  std::vector<std::size_t> const workspaceIndices = getProperty("WorkspaceIndices");
  std::vector<double> const startXs = getProperty("StartXs");
  std::vector<double> const endXs = getProperty("EndXs");

  int const maxIterations = getProperty("MaxIterations");
  std::string const minimizer = getProperty("Minimizer");
  std::string const costFunction = getProperty("CostFunction");
  std::string const evaluationType = getProperty("EvaluationType");
  std::string const outputBaseName = getProperty("OutputBaseName");

  replaceAll(code, "{{input_list}}", constructInputList(inputWorkspaces, workspaceIndices, startXs, endXs));
  replaceAll(code, "{{function_string}}", generateFunctionString());
  replaceAll(code, "{{max_iterations}}", std::to_string(maxIterations));
  replaceAll(code, "{{minimizer}}", minimizer);
  replaceAll(code, "{{cost_function}}", costFunction);
  replaceAll(code, "{{evaluation_type}}", evaluationType);
  replaceAll(code, "{{output_base_name}}", outputBaseName);
  return code;
}

std::string GeneratePythonFitScript::generateSimultaneousFitCode() const {
  std::string code = getFileContents("GeneratePythonFitScript_SimultaneousFit.py.in");
  std::string const line = getFileContents("GeneratePythonFitScript_SimultaneousFitDomainLine.py.in");

  std::vector<std::string> const inputWorkspaces = getProperty("InputWorkspaces");
  std::string domainLines;
  for (auto i = 1u; i < inputWorkspaces.size(); ++i) {
    std::string snippet = line;
    replaceAll(snippet, "{{i}}", std::to_string(i));
    domainLines += snippet;
  }

  replaceAll(code, "{{other_domains}}", domainLines);
  return code;
}

std::string GeneratePythonFitScript::generateFunctionString() const {
  IFunction_const_sptr function = getProperty("Function");
  auto const functionSplit = splitStringBy(function->asString(), ";");

  std::string code = "\\\n    \"";
  code += joinVector(functionSplit, ";\" \\\n    \"");
  code += "\"";
  return code;
}

void GeneratePythonFitScript::savePythonScript(std::string const &filepath, std::string const &contents) const {
  std::ofstream file(filepath.c_str(), std::ofstream::trunc);
  file << contents;
  file.flush();
  file.close();
}

} // namespace Mantid::Algorithms
