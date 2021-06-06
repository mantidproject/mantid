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
#include "MantidKernel/ListValidator.h"
#include "MantidKernel/StartsWithValidator.h"

#include <boost/algorithm/string/detail/classification.hpp>
#include <boost/algorithm/string/split.hpp>

#include <fstream>

using namespace Mantid::API;
using namespace Mantid::Kernel;

namespace {

std::string const PLOTTING_CODE =
    "# Plot the results of the fit\n"
    "fig, axes = plt.subplots(nrows=2,\n"
    "                         ncols=len(output_workspaces),\n"
    "                         sharex=True,\n"
    "                         gridspec_kw={\"height_ratios\": [2, 1]},\n"
    "                         subplot_kw={\"projection\": \"mantid\"})\n"
    "\n"
    "for i, workspace in enumerate(output_workspaces):\n"
    "    axes[0, i].errorbar(workspace, \"rs\", wkspIndex=0, label=\"Data\", markersize=2)\n"
    "    axes[0, i].errorbar(workspace, \"b-\", wkspIndex=1, label=\"Fit\")\n"
    "    axes[0, i].set_title(workspace.name())\n"
    "    axes[0, i].set_xlabel(\"\")\n"
    "    axes[0, i].tick_params(axis=\"both\", direction=\"in\")\n"
    "    axes[0, i].legend()\n"
    "\n"
    "    axes[1, i].errorbar(workspace, \"ko\", wkspIndex=2, markersize=2)\n"
    "    axes[1, i].set_ylabel(\"Difference\")\n"
    "    axes[1, i].tick_params(axis=\"both\", direction=\"in\")\n"
    "\n"
    "fig.subplots_adjust(hspace=0)\n"
    "fig.show()\n";

template <typename T> std::string joinVector(std::vector<T> const &vec, std::string const &delimiter = ", ") {
  std::stringstream ss;
  std::copy(vec.cbegin(), vec.cend(), std::ostream_iterator<T>(ss, delimiter.c_str()));
  auto const str = ss.str();
  return str.substr(0, str.size() - delimiter.size());
}

std::string constructInputDictionaryEntry(std::string const &workspaceName, std::size_t const &workspaceIndex,
                                          double const startX, double const endX) {
  return "\"" + workspaceName + "\": (" + std::to_string(workspaceIndex) + ", " + std::to_string(startX) + ", " +
         std::to_string(endX) + ")";
}

std::string constructInputDictionary(std::vector<std::string> const &inputWorkspaces,
                                     std::vector<std::size_t> const &workspaceIndices,
                                     std::vector<double> const &startXs, std::vector<double> const &endXs) {
  std::vector<std::string> entries;
  entries.reserve(inputWorkspaces.size());
  for (auto i = 0u; i < inputWorkspaces.size(); ++i)
    entries.emplace_back(constructInputDictionaryEntry(inputWorkspaces[i], workspaceIndices[i], startXs[i], endXs[i]));

  return "input_data = {\n    " + joinVector(entries, ",\n    ") + "\n}";
}

std::vector<std::string> splitStringBy(std::string const &str, std::string const &delimiter) {
  std::vector<std::string> subStrings;
  boost::split(subStrings, str, boost::is_any_of(delimiter));
  subStrings.erase(std::remove_if(subStrings.begin(), subStrings.end(),
                                  [](std::string const &subString) { return subString.empty(); }),
                   subStrings.end());
  return subStrings;
}

} // namespace

namespace Mantid {
namespace Algorithms {

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
  auto const filepath = getPropertyValue("Filepath");
  IFunction_sptr function = getProperty("Function");

  std::map<std::string, std::string> errors;
  if (workspaceIndices.size() != inputWorkspaces.size())
    errors["WorkspaceIndices"] = "The number of workspace indices must be equal to the number of input workspaces.";
  if (startXs.size() != inputWorkspaces.size())
    errors["StartXs"] = "The number of Start Xs must be equal to the number of input workspaces.";
  if (endXs.size() != inputWorkspaces.size())
    errors["EndXs"] = "The number of End Xs must be equal to the number of input workspaces.";
  if (fittingType == "Sequential") {
    if (auto const multiDomainFunction = std::dynamic_pointer_cast<MultiDomainFunction>(function))
      errors["Function"] = "The Function cannot be a MultiDomainFunction when in Sequential fit mode.";
  }
  if (fittingType == "Simultaneous") {
    if (getNumberOfDomainsInFunction(function) != inputWorkspaces.size())
      errors["Function"] = "The Function provided does not have the same number of domains as there is input "
                           "workspaces. This is a requirement for Simultaneous fitting.";
  }

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
  generatedScript += generateVariableSetupCode();
  if (fittingType == "Sequential")
    generatedScript += generateSequentialFitCode();
  else if (fittingType == "Simultaneous")
    generatedScript += generateSimultaneousFitCode();
  generatedScript += PLOTTING_CODE;
  return generatedScript;
}

std::string GeneratePythonFitScript::generateVariableSetupCode() const {
  std::vector<std::string> const inputWorkspaces = getProperty("InputWorkspaces");
  std::vector<std::size_t> const workspaceIndices = getProperty("WorkspaceIndices");
  std::vector<double> const startXs = getProperty("StartXs");
  std::vector<double> const endXs = getProperty("EndXs");

  int const maxIterations = getProperty("MaxIterations");
  std::string const minimizer = getProperty("Minimizer");
  std::string const costFunction = getProperty("CostFunction");
  std::string const evaluationType = getProperty("EvaluationType");

  std::string code = "# A python script generated to perform a sequential or simultaneous fit\n";
  code += "from mantid.simpleapi import *\n";
  code += "import matplotlib.pyplot as plt\n\n";

  code += "# Dictionary { workspace_name: (workspace_index, start_x, end_x) }\n";
  code += constructInputDictionary(inputWorkspaces, workspaceIndices, startXs, endXs);
  code += "\n\n";

  code += "# Fit function as a string\n";
  code += generateFunctionString();

  code += "# Fitting options\n";
  code += "max_iterations = " + std::to_string(maxIterations) + "\n";
  code += "minimizer = \"" + minimizer + "\"\n";
  code += "cost_function = \"" + costFunction + "\"\n";
  code += "evaluation_type = \"" + evaluationType + "\"\n\n";

  return code;
}

std::string GeneratePythonFitScript::generateSequentialFitCode() const {
  std::string code = "# Perform a sequential fit\n";
  code += "output_workspaces, parameter_tables, normalised_matrices = [], [], []\n";
  code += "for input_workspace, domain_data in input_data.items():\n";
  code += "    fit_output = Fit(Function=function, InputWorkspace=input_workspace, WorkspaceIndex=domain_data[0],\n";
  code += "                     StartX=domain_data[1], EndX=domain_data[2], MaxIterations=max_iterations,\n";
  code += "                     Minimizer=minimizer, CostFunction=cost_function, EvaluationType=evaluation_type,\n";
  code += "                     CreateOutput=True)\n";
  code += "\n";
  code += "    output_workspaces.append(fit_output.OutputWorkspace)\n";
  code += "    parameter_tables.append(fit_output.OutputParameters)\n";
  code += "    normalised_matrices.append(fit_output.OutputNormalisedCovarianceMatrix)\n";
  code += "\n";
  code += "    # Use the parameters in the previous function as the start parameters of the next fit\n";
  code += "    function = fit_output.Function\n";
  code += "\n";
  code += "# Group the output workspaces from the sequential fit\n";
  code += "GroupWorkspaces(InputWorkspaces=output_workspaces, OutputWorkspace=\"Sequential_Fit_Workspaces\")\n";
  code += "GroupWorkspaces(InputWorkspaces=parameter_tables, OutputWorkspace=\"Sequential_Fit_Parameters\")\n";
  code += "GroupWorkspaces(InputWorkspaces=normalised_matrices, ";
  code += "OutputWorkspace=\"Sequential_Fit_NormalisedCovarianceMatrices\")\n\n";
  return code;
}

std::string GeneratePythonFitScript::generateSimultaneousFitCode() const {
  std::string code = "# Perform a simultaneous fit\n";
  code += "input_workspaces = list(input_data.keys())\n";
  code += "domain_data = list(input_data.values())\n\n";
  code += "fit_output = Fit(Function=function,\n";

  code += "                 InputWorkspace=input_workspaces[0], WorkspaceIndex=domain_data[0][0], "
          "StartX=domain_data[0][1], EndX=domain_data[0][2],\n";

  std::vector<std::string> const inputWorkspaces = getProperty("InputWorkspaces");
  for (auto i = 1u; i < inputWorkspaces.size(); ++i) {
    code += "                 InputWorkspace_" + std::to_string(i) + "=input_workspaces[" + std::to_string(i) + "], ";
    code += "WorkspaceIndex_" + std::to_string(i) + "=domain_data[" + std::to_string(i) + "][0], ";
    code += "StartX_" + std::to_string(i) + "=domain_data[" + std::to_string(i) + "][1], ";
    code += "EndX_" + std::to_string(i) + "=domain_data[" + std::to_string(i) + "][2],\n";
  }

  code += "                 MaxIterations=max_iterations, Minimizer=minimizer, CostFunction=cost_function,\n";
  code += "                 EvaluationType=evaluation_type, CreateOutput=True)\n\n";

  code += "output_workspaces = fit_output.OutputWorkspace\n\n";
  return code;
}

std::string GeneratePythonFitScript::generateFunctionString() const {
  IFunction_const_sptr function = getProperty("Function");
  auto const functionSplit = splitStringBy(function->asString(), ";");

  std::string code = "function = \\\n\"";
  code += joinVector(functionSplit, ";\" \\\n\"");
  code += "\"\n\n";
  return code;
}

void GeneratePythonFitScript::savePythonScript(std::string const &filepath, std::string const &contents) const {
  std::ofstream file(filepath.c_str(), std::ofstream::trunc);
  file << contents;
  file.flush();
  file.close();
}

} // namespace Algorithms
} // namespace Mantid
