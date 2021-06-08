// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <cxxtest/TestSuite.h>

#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"
#include <Poco/File.h>

#include <fstream>

using namespace Mantid::API;
using namespace Mantid::Kernel;
using namespace WorkspaceCreationHelper;

namespace {

std::vector<std::string> splitStringBy(std::string const &str, std::string const &delimiter) {
  std::vector<std::string> subStrings;
  boost::split(subStrings, str, boost::is_any_of(delimiter));
  return subStrings;
}

} // namespace

class GeneratePythonFitScriptTest : public CxxTest::TestSuite {
public:
  GeneratePythonFitScriptTest() { FrameworkManager::Instance(); }

  static GeneratePythonFitScriptTest *createSuite() { return new GeneratePythonFitScriptTest(); }

  static void destroySuite(GeneratePythonFitScriptTest *suite) { delete suite; }

  void setUp() override {
    m_inputWorkspaces = std::vector<std::string>{"Name1", "Name2"};
    m_workspaceIndices = std::vector<std::size_t>{0u, 1u};
    m_startXs = std::vector<double>{0.5, 0.6};
    m_endXs = std::vector<double>{1.5, 1.6};
    m_function = "name=GausOsc,A=0.2,Sigma=0.2,Frequency=0.1,Phi=0";

    m_maxIterations = 500;
    m_minimizer = "Levenberg-Marquardt";
    m_costFunction = "Least squares";
    m_evaluationType = "CentrePoint";

    m_filepath = ConfigService::Instance().getString("defaultsave.directory") + "TestPythonScript.py";

    AnalysisDataService::Instance().addOrReplace(m_inputWorkspaces[0], create2DWorkspace(5, 5));
    AnalysisDataService::Instance().addOrReplace(m_inputWorkspaces[1], create2DWorkspace(5, 5));

    m_algorithm = AlgorithmManager::Instance().create("GeneratePythonFitScript");
    m_algorithm->initialize();
    m_algorithm->setProperty("InputWorkspaces", m_inputWorkspaces);
    m_algorithm->setProperty("WorkspaceIndices", m_workspaceIndices);
    m_algorithm->setProperty("StartXs", m_startXs);
    m_algorithm->setProperty("EndXs", m_endXs);

    m_algorithm->setProperty("Function", m_function);

    m_algorithm->setProperty("MaxIterations", std::to_string(m_maxIterations));
    m_algorithm->setProperty("Minimizer", m_minimizer);
    m_algorithm->setProperty("CostFunction", m_costFunction);
    m_algorithm->setProperty("EvaluationType", m_evaluationType);
  }

  void tearDown() override { AnalysisDataService::Instance().clear(); }

  void test_that_the_expected_python_script_is_generated_when_a_filepath_is_set() {
    m_algorithm->setProperty("Filepath", m_filepath);
    m_algorithm->execute();

    auto const expectedText = createExpectedSequentialScriptText();
    auto const expectedLines = splitStringBy(expectedText, "\n");

    std::string const text = m_algorithm->getPropertyValue("ScriptText");
    TS_ASSERT_EQUALS(text, expectedText);

    assertExpectedScriptExists(expectedLines);
  }

  void test_that_the_expected_script_text_is_returned_using_an_output_property_when_a_filepath_is_not_set() {
    m_algorithm->execute();

    auto const expectedText = createExpectedSequentialScriptText();
    auto const expectedLines = splitStringBy(expectedText, "\n");

    std::string const text = m_algorithm->getPropertyValue("ScriptText");
    TS_ASSERT_EQUALS(text, expectedText);
  }

private:
  void assertExpectedScriptExists(std::vector<std::string> const &expectedLines) {
    TS_ASSERT(Poco::File(m_filepath).exists());

    std::ifstream file(m_filepath.c_str(), std::ifstream::in);

    std::string scriptLine;
    int lineCount = 0;
    while (std::getline(file, scriptLine)) {
      TS_ASSERT_EQUALS(scriptLine, expectedLines[lineCount])
      ++lineCount;
    }

    file.close();
    Poco::File(m_filepath).remove();
  }

  std::string createExpectedSequentialScriptText() const {
    std::string expectedScript;

    expectedScript += "# A python script generated to perform a sequential fit\n";
    expectedScript += "from mantid.simpleapi import *\n";
    expectedScript += "import matplotlib.pyplot as plt\n";
    expectedScript += "\n";
    expectedScript += "# Dictionary { workspace_name: (workspace_index, start_x, end_x) }\n";
    expectedScript += "input_data = {\n";
    expectedScript += "    \"" + m_inputWorkspaces[0] + "\": (" + std::to_string(m_workspaceIndices[0]) + ", " +
                      std::to_string(m_startXs[0]) + ", " + std::to_string(m_endXs[0]) + "),\n";
    expectedScript += "    \"" + m_inputWorkspaces[1] + "\": (" + std::to_string(m_workspaceIndices[1]) + ", " +
                      std::to_string(m_startXs[1]) + ", " + std::to_string(m_endXs[1]) + ")\n";
    expectedScript += "}\n";
    expectedScript += "\n";
    expectedScript += "# Fit function as a string\n";
    expectedScript += "function = \"" + m_function + "\"\n";
    expectedScript += "\n";
    expectedScript += "# Fitting options\n";
    expectedScript += "max_iterations = " + std::to_string(m_maxIterations) + "\n";
    expectedScript += "minimizer = \"" + m_minimizer + "\"\n";
    expectedScript += "cost_function = \"" + m_costFunction + "\"\n";
    expectedScript += "evaluation_type = \"" + m_evaluationType + "\"\n";
    expectedScript += "\n";

    expectedScript +=
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
        "fig.show()";
    return expectedScript;
  }

  Mantid::API::IAlgorithm_sptr m_algorithm;

  std::vector<std::string> m_inputWorkspaces;
  std::vector<std::size_t> m_workspaceIndices;
  std::vector<double> m_startXs;
  std::vector<double> m_endXs;
  std::string m_function;

  std::size_t m_maxIterations;
  std::string m_minimizer;
  std::string m_costFunction;
  std::string m_evaluationType;

  std::string m_filepath;
};
