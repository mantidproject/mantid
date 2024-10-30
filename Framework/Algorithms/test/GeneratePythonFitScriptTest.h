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
#include "MantidFrameworkTestHelpers/WorkspaceCreationHelper.h"

#include <fstream>

#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/split.hpp>

#include <Poco/File.h>

using namespace Mantid::API;
using namespace Mantid::Kernel;
using namespace WorkspaceCreationHelper;

namespace {

std::vector<std::string> splitStringBy(std::string const &str, std::string const &delimiter) {
  std::vector<std::string> subStrings;
  boost::split(subStrings, str, boost::is_any_of(delimiter));
  return subStrings;
}

std::string getFileContents(std::string const &filename) {
  auto const directory = ConfigService::Instance().getString("python.templates.directory") + "/reference";

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
    m_sequentialFunction = "name=GausOsc,A=0.2,Sigma=0.2,Frequency=0.1,Phi=0";
    m_simultaneousFunction = "composite=MultiDomainFunction,NumDeriv=true;name=GausOsc,A=0.2,Sigma=0.2,Frequency=1,Phi="
                             "0,$domains=i;name=GausOsc,A=0.2,Sigma=0.2,Frequency=1,Phi=0,$domains=i";

    m_maxIterations = 500;
    m_minimizer = "Levenberg-Marquardt";
    m_costFunction = "Least squares";
    m_evaluationType = "CentrePoint";
    m_outputBaseName = "Output_Fit";
    m_plotOutput = true;

    m_filepath = ConfigService::Instance().getString("defaultsave.directory") + "TestPythonScript.py";

    AnalysisDataService::Instance().addOrReplace(m_inputWorkspaces[0], create2DWorkspace(5, 5));
    AnalysisDataService::Instance().addOrReplace(m_inputWorkspaces[1], create2DWorkspace(5, 5));

    m_algorithm = AlgorithmManager::Instance().create("GeneratePythonFitScript");
    m_algorithm->initialize();
    m_algorithm->setProperty("InputWorkspaces", m_inputWorkspaces);
    m_algorithm->setProperty("WorkspaceIndices", m_workspaceIndices);
    m_algorithm->setProperty("StartXs", m_startXs);
    m_algorithm->setProperty("EndXs", m_endXs);

    m_algorithm->setProperty("MaxIterations", std::to_string(m_maxIterations));
    m_algorithm->setProperty("Minimizer", m_minimizer);
    m_algorithm->setProperty("CostFunction", m_costFunction);
    m_algorithm->setProperty("EvaluationType", m_evaluationType);
    m_algorithm->setProperty("OutputBaseName", m_outputBaseName);
    m_algorithm->setProperty("PlotOutput", m_plotOutput);
  }

  void tearDown() override { AnalysisDataService::Instance().clear(); }

  void test_that_the_expected_python_script_is_generated_when_a_filepath_is_set_in_sequential_mode() {
    m_algorithm->setProperty("FittingType", "Sequential");
    m_algorithm->setProperty("Function", m_sequentialFunction);
    m_algorithm->setProperty("Filepath", m_filepath);
    m_algorithm->execute();

    auto const expectedText = getFileContents("generate_sequential_fit_script_reference_file.py");
    auto const expectedLines = splitStringBy(expectedText, "\n");

    std::string const text = m_algorithm->getPropertyValue("ScriptText") + "\n";
    TS_ASSERT_EQUALS(text, expectedText);

    assertExpectedScriptExists(expectedLines);
  }

  void
  test_that_the_expected_script_text_is_returned_using_an_output_property_when_a_filepath_is_not_set_in_sequential_mode() {
    m_algorithm->setProperty("FittingType", "Sequential");
    m_algorithm->setProperty("Function", m_sequentialFunction);
    m_algorithm->execute();

    auto const expectedText = getFileContents("generate_sequential_fit_script_reference_file.py");

    std::string const text = m_algorithm->getPropertyValue("ScriptText") + "\n";
    TS_ASSERT_EQUALS(text, expectedText);
  }

  void test_that_the_expected_python_script_is_generated_when_a_filepath_is_set_in_simultaneous_mode() {
    m_algorithm->setProperty("FittingType", "Simultaneous");
    m_algorithm->setProperty("Function", m_simultaneousFunction);
    m_algorithm->setProperty("Filepath", m_filepath);
    m_algorithm->execute();

    auto const expectedText = getFileContents("generate_simultaneous_fit_script_reference_file.py");
    auto const expectedLines = splitStringBy(expectedText, "\n");

    std::string const text = m_algorithm->getPropertyValue("ScriptText") + "\n";
    TS_ASSERT_EQUALS(text, expectedText);

    assertExpectedScriptExists(expectedLines);
  }

  void
  test_that_the_expected_script_text_is_returned_using_an_output_property_when_a_filepath_is_not_set_in_simultaneous_mode() {
    m_algorithm->setProperty("FittingType", "Simultaneous");
    m_algorithm->setProperty("Function", m_simultaneousFunction);
    m_algorithm->execute();

    auto const expectedText = getFileContents("generate_simultaneous_fit_script_reference_file.py");

    std::string const text = m_algorithm->getPropertyValue("ScriptText") + "\n";
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

  Mantid::API::IAlgorithm_sptr m_algorithm;

  std::vector<std::string> m_inputWorkspaces;
  std::vector<std::size_t> m_workspaceIndices;
  std::vector<double> m_startXs;
  std::vector<double> m_endXs;
  std::string m_sequentialFunction;
  std::string m_simultaneousFunction;

  std::size_t m_maxIterations;
  std::string m_minimizer;
  std::string m_costFunction;
  std::string m_evaluationType;
  std::string m_outputBaseName;
  bool m_plotOutput;

  std::string m_filepath;
};
