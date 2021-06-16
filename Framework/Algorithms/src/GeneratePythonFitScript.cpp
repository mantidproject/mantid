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
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/BoundedValidator.h"
#include "MantidKernel/ListValidator.h"
#include "MantidKernel/StartsWithValidator.h"

#include <fstream>

using namespace Mantid::API;
using namespace Mantid::Kernel;

namespace {

std::string const SEQUENTIAL_SCRIPT =
    "# Perform a sequential fit\n"
    "output_workspaces, parameter_tables, normalised_matrices = [], [], []\n"
    "for input_workspace, domain_data in input_data.items():\n"
    "    fit_output = Fit(Function=function, InputWorkspace=input_workspace, WorkspaceIndex=domain_data[0],\n"
    "                     StartX=domain_data[1], EndX=domain_data[2], MaxIterations=max_iterations,\n"
    "                     Minimizer=minimizer, CostFunction=cost_function, EvaluationType=evaluation_type,\n"
    "                     CreateOutput=True)\n"
    "\n"
    "    output_workspaces.append(fit_output.OutputWorkspace)\n"
    "    parameter_tables.append(fit_output.OutputParameters)\n"
    "    normalised_matrices.append(fit_output.OutputNormalisedCovarianceMatrix)\n"
    "\n"
    "    # Use the parameters in the previous function as the start parameters of the next fit\n"
    "    function = fit_output.Function\n"
    "\n"
    "# Group the output workspaces from the sequential fit\n"
    "GroupWorkspaces(InputWorkspaces=output_workspaces, OutputWorkspace=\"Sequential_Fit_Workspaces\")\n"
    "GroupWorkspaces(InputWorkspaces=parameter_tables, OutputWorkspace=\"Sequential_Fit_Parameters\")\n"
    "GroupWorkspaces(InputWorkspaces=normalised_matrices, "
    "OutputWorkspace=\"Sequential_Fit_NormalisedCovarianceMatrices\")\n"
    "\n"
    "# Plot the results of the sequential fit\n"
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

  declareProperty(
      std::make_unique<FunctionProperty>("Function", Direction::Input),
      "The function to use for the fitting. This should be a single domain function if the Python script will be for "
      "sequential fitting, or a MultiDomainFunction if the Python script is for simultaneous fitting.");

  declareProperty("MaxIterations", 500, mustBePositive->clone(),
                  "The MaxIterations to be passed to the Fit algorithm in the Python script.",
                  Kernel::Direction::Input);

  auto const minimizerOptions = FuncMinimizerFactory::Instance().getKeys();
  auto const minimizerValidator = std::make_shared<StartsWithValidator>(minimizerOptions);
  declareProperty("Minimizer", "Levenberg-Marquardt", minimizerValidator,
                  "The Minimizer to be passed to the Fit algorithm in the Python script.");

  auto const costFunctionOptions = CostFunctionFactory::Instance().getKeys();
  auto const costFunctionValidator = std::make_shared<Kernel::ListValidator<std::string>>(costFunctionOptions);
  declareProperty("CostFunction", "Least squares", costFunctionValidator,
                  "The CostFunction to be passed to the Fit algorithm in the Python script.", Kernel::Direction::Input);

  std::array<std::string, 2> evaluationTypes = {{"CentrePoint", "Histogram"}};
  declareProperty(
      "EvaluationType", "CentrePoint", Kernel::IValidator_sptr(new Kernel::ListValidator<std::string>(evaluationTypes)),
      "The EvaluationType to be passed to the Fit algorithm in the Python script.", Kernel::Direction::Input);

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
  auto const filepath = getPropertyValue("Filepath");

  std::map<std::string, std::string> errors;
  if (workspaceIndices.size() != inputWorkspaces.size())
    errors["WorkspaceIndices"] = "The number of workspace indices must be equal to the number of input workspaces.";
  if (startXs.size() != inputWorkspaces.size())
    errors["StartXs"] = "The number of Start Xs must be equal to the number of input workspaces.";
  if (endXs.size() != inputWorkspaces.size())
    errors["EndXs"] = "The number of End Xs must be equal to the number of input workspaces.";

  return errors;
}

void GeneratePythonFitScript::exec() {
  std::string generatedScript;
  generatedScript += generateVariableSetupCode();
  generatedScript += SEQUENTIAL_SCRIPT;

  auto const filepath = getPropertyValue("Filepath");
  if (!filepath.empty())
    savePythonScript(filepath, generatedScript);

  setProperty("ScriptText", generatedScript);
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

  IFunction_const_sptr function = getProperty("Function");

  std::string code = "# A python script generated to perform a sequential fit\n";
  code += "from mantid.simpleapi import *\n";
  code += "import matplotlib.pyplot as plt\n\n";

  code += "# Dictionary { workspace_name: (workspace_index, start_x, end_x) }\n";
  code += constructInputDictionary(inputWorkspaces, workspaceIndices, startXs, endXs);
  code += "\n\n";

  code += "# Fit function as a string\n";
  code += "function = \"" + function->asString() + "\"\n\n";

  code += "# Fitting options\n";
  code += "max_iterations = " + std::to_string(maxIterations) + "\n";
  code += "minimizer = \"" + minimizer + "\"\n";
  code += "cost_function = \"" + costFunction + "\"\n";
  code += "evaluation_type = \"" + evaluationType + "\"\n\n";

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
