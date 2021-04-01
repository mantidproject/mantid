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
  declareProperty(std::make_unique<ArrayProperty<int>>("WorkspaceIndices", mustBePositive->clone(), Direction::Input),
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
  declareProperty(std::make_unique<API::FileProperty>("ScriptName", "", API::FileProperty::Save, extensions),
                  "The name of the Python fit script which will be generated and saved in the selected location.");
}

std::map<std::string, std::string> GeneratePythonFitScript::validateInputs() {
  std::map<std::string, std::string> errors;
  auto const filename = getPropertyValue("ScriptName");
  if (!filename.empty())
    errors["ScriptName"] = "A name must be provided for the Python fit script to be generated.";

  return errors;
}

void GeneratePythonFitScript::exec() {
  auto const inputWorkspaces = getProperty("InputWorkspaces");
  auto const workspaceIndices = getProperty("WorkspaceIndices");
  auto const startXs = getProperty("StartXs");
  auto const endXs = getProperty("EndXs");

  auto const function = getProperty("Function");

  auto const maxIterations = getProperty("MaxIterations");
  auto const minimizer = getProperty("Minimizer");
  auto const costFunction = getProperty("CostFunction");
  auto const evaluationType = getProperty("EvaluationType");

  auto const filename = getPropertyValue("ScriptName");

  std::string generatedScript;
  generatedScript += "# Python script generated by Mantid\n";
  generatedScript += "from mantid.simpleapi import *\n\n";

  std::ofstream file(filename.c_str(), std::ofstream::trunc);
  file << generatedScript;
  file.flush();
  file.close();
}

} // namespace Algorithms
} // namespace Mantid
