// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidAlgorithms/GeneratePythonFitScript.h"
#include "MantidAPI/ADSValidator.h"
#include "MantidAPI/AlgorithmHistory.h"
#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/CostFunctionFactory.h"
#include "MantidAPI/FileProperty.h"
#include "MantidAPI/FuncMinimizerFactory.h"
#include "MantidAPI/FunctionProperty.h"
#include "MantidAPI/ScriptBuilder.h"
#include "MantidAPI/Workspace.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/BoundedValidator.h"
#include "MantidKernel/ListValidator.h"
#include "MantidKernel/MantidVersion.h"
#include "MantidKernel/StartsWithValidator.h"
#include "MantidKernel/System.h"

#include <fstream>

using namespace Mantid::API;
using namespace Mantid::Kernel;
using Mantid::Types::Core::DateAndTime;

namespace {

template <typename T> std::string joinVector(std::vector<T> const &vec, std::string const &delimiter = ", ") {
  std::stringstream ss;
  std::copy(vec.cbegin(), vec.cend(), std::ostream_iterator<T>(ss, delimiter.c_str()));
  auto const str = ss.str();
  return str.substr(0, str.size() - delimiter.size());
}

template <typename T> std::string constructPythonList(std::vector<T> const &vec) { return "[" + joinVector(vec) + "]"; }

template <typename T> std::string constructPythonStringList(std::vector<T> const &vec) {
  return "[\"" + joinVector(vec, "\", \"") + "\"]";
}

} // namespace

namespace Mantid {
namespace Algorithms {

DECLARE_ALGORITHM(GeneratePythonFitScript)

std::string const GeneratePythonFitScript::name() const { return "GeneratePythonFitScript"; };

int GeneratePythonFitScript::version() const { return 1; };

std::string const GeneratePythonFitScript::category() const { return "Utility\\Python"; }

std::string const GeneratePythonFitScript::summary() const {
  return "An Algorithm to generate a Python script file for performing a sequential or simultaneous fit.";
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
  declareProperty(std::make_unique<FileProperty>("Filename", "", FileProperty::Save, extensions),
                  "The name of the Python fit script which will be generated and saved in the selected location.");
}

std::map<std::string, std::string> GeneratePythonFitScript::validateInputs() {
  std::vector<std::string> const inputWorkspaces = getProperty("InputWorkspaces");
  std::vector<std::size_t> const workspaceIndices = getProperty("WorkspaceIndices");
  std::vector<double> const startXs = getProperty("StartXs");
  std::vector<double> const endXs = getProperty("EndXs");
  auto const filename = getPropertyValue("Filename");

  std::map<std::string, std::string> errors;
  if (filename.empty())
    errors["Filename"] = "A name must be provided for the Python fit script to be generated.";
  if (workspaceIndices.size() != inputWorkspaces.size())
    errors["WorkspaceIndices"] = "The number of workspace indices must correspond to the number of input workspaces.";
  if (startXs.size() != inputWorkspaces.size())
    errors["StartXs"] = "The number of Start Xs must correspond to the number of input workspaces.";
  if (endXs.size() != inputWorkspaces.size())
    errors["EndXs"] = "The number of End Xs must correspond to the number of input workspaces.";

  return errors;
}

void GeneratePythonFitScript::exec() {
  std::vector<std::string> const inputWorkspaces = getProperty("InputWorkspaces");
  std::vector<std::size_t> const workspaceIndices = getProperty("WorkspaceIndices");
  std::vector<double> const startXs = getProperty("StartXs");
  std::vector<double> const endXs = getProperty("EndXs");

  IFunction_const_sptr function = getProperty("Function");

  std::string generatedScript;
  generatedScript += generateVariableSetupCode(inputWorkspaces, workspaceIndices, startXs, endXs, function);
  generatedScript += generateSequentialFittingCode();
  generatedScript += generateCodeForTidyingFitOutput();

  savePythonScript(generatedScript);
}

std::string GeneratePythonFitScript::generateVariableSetupCode(std::vector<std::string> const &inputWorkspaces,
                                                               std::vector<std::size_t> const &workspaceIndices,
                                                               std::vector<double> const &startXs,
                                                               std::vector<double> const &endXs,
                                                               IFunction_const_sptr function) const {
  std::string code = "# A python script generated to perform a sequential fit\n";
  code += "from mantid.simpleapi import *\n";
  code += "\n";
  code += "input_workspaces = " + constructPythonStringList(inputWorkspaces) + "\n";
  code += "workspace_indices = " + constructPythonList(workspaceIndices) + "\n";
  code += "start_xs = " + constructPythonList(startXs) + "\n";
  code += "end_xs = " + constructPythonList(endXs) + "\n\n";
  code += "function = \"" + function->asString() + "\"\n\n";
  return code;
}

std::string GeneratePythonFitScript::generateSequentialFittingCode() const {
  std::string code = "# Perform a sequential fit.\n";
  code += "output_workspaces, parameter_tables, normalised_matrices = [], [], []\n";
  code += "for i in range(len(input_workspaces)):\n";
  code += "    fit_output = Fit(Function=function, InputWorkspace=input_workspaces[i], ";
  code += "WorkspaceIndex=workspace_indices[i], \n                     ";
  code += "StartX=start_xs[i], EndX=end_xs[i], ";
  code += generateFitOptionsString();
  code += "\n";
  code += "    output_workspaces.append(fit_output.OutputWorkspace)\n";
  code += "    parameter_tables.append(fit_output.OutputParameters)\n";
  code += "    normalised_matrices.append(fit_output.OutputNormalisedCovarianceMatrix)\n";
  code += "\n";
  code += "    # Use the parameters in the previous function as the start parameters of the next fit.\n";
  code += "    function = fit_output.Function\n";
  code += "\n";
  return code;
}

std::string GeneratePythonFitScript::generateFitOptionsString() const {
  int const maxIterations = getProperty("MaxIterations");
  std::string const minimizer = getProperty("Minimizer");
  std::string const costFunction = getProperty("CostFunction");
  std::string const evaluationType = getProperty("EvaluationType");

  std::string fitOptionsString = "MaxIterations=" + std::to_string(maxIterations) + ", ";
  fitOptionsString += "Minimizer=\"" + minimizer + "\", \n                     ";
  fitOptionsString += "CostFunction=\"" + costFunction + "\", ";
  fitOptionsString += "EvaluationType=\"" + evaluationType + "\", CreateOutput=True)\n";
  return fitOptionsString;
}

std::string GeneratePythonFitScript::generateCodeForTidyingFitOutput() const {
  std::string code = "# Group the output workspaces from the sequential fit.\n";
  code += "base_name = input_workspaces[0] + \"_Sequential_\"\n";
  code += "GroupWorkspaces(InputWorkspaces=output_workspaces, OutputWorkspace=base_name + \"Workspaces\")\n";
  code += "GroupWorkspaces(InputWorkspaces=parameter_tables, OutputWorkspace=base_name + \"Parameters\")\n";
  code += "GroupWorkspaces(InputWorkspaces=normalised_matrices, OutputWorkspace=base_name + "
          "\"NormalisedCovarianceMatrices\")\n";
  return code;
}

void GeneratePythonFitScript::savePythonScript(std::string const &contents) const {
  auto const filename = getPropertyValue("Filename");

  std::ofstream file(filename.c_str(), std::ofstream::trunc);
  file << contents;
  file.flush();
  file.close();
}

} // namespace Algorithms
} // namespace Mantid
