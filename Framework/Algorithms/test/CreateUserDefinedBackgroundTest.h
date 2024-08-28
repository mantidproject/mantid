// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <cxxtest/TestSuite.h>

#include "MantidAPI/AlgorithmFactory.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/ITableWorkspace.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/TableRow.h"
#include "MantidAlgorithms/CreateUserDefinedBackground.h"
#include "MantidDataObjects/TableWorkspace.h"
#include "MantidFrameworkTestHelpers/WorkspaceCreationHelper.h"
#include <cmath>

using Mantid::Algorithms::CreateUserDefinedBackground;
using Mantid::API::ITableWorkspace_sptr;
using Mantid::API::MatrixWorkspace_sptr;

namespace {
// Gaussian
double gauss(double x, double height, double centre, double fwhm) {
  const double factor = 2.0 * sqrt(2.0 * log(2.0));
  const double sigma = fwhm / factor;
  return height * exp(-0.5 * (((x - centre) * (x - centre)) / (sigma * sigma)));
}

// Function to generate test background without peaks
double background(double xPoint, int iSpec = 0) {
  UNUSED_ARG(iSpec);
  return gauss(xPoint, 5.0, 0.0, 5.0) + gauss(xPoint, 2.0, 3.0, 2.0);
}

// Function to generate test peaks without background
double peaks(double xPoint) { return gauss(xPoint, 1.0, 2.0, 0.1) + gauss(xPoint, 1.0, 4.0, 0.1); }

// Function to generate test data: a background with some peaks
double dataFunction(double xPoint, int iSpec) { return background(xPoint, iSpec) + peaks(xPoint); }
} // namespace

class CreateUserDefinedBackgroundTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static CreateUserDefinedBackgroundTest *createSuite() { return new CreateUserDefinedBackgroundTest(); }
  static void destroySuite(CreateUserDefinedBackgroundTest *suite) { delete suite; }

  /// Constructor: cache the value of the setting
  CreateUserDefinedBackgroundTest() : m_key("graph1d.autodistribution") {
    m_option = Mantid::Kernel::ConfigService::Instance().getString(m_key);
  }

  /// Destructor: reset the setting to its stored value
  ~CreateUserDefinedBackgroundTest() { Mantid::Kernel::ConfigService::Instance().setString(m_key, m_option); }

  void test_Init() {
    CreateUserDefinedBackground alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
  }

  void test_Properties() {
    CreateUserDefinedBackground alg;
    TS_ASSERT_EQUALS(alg.version(), 1);
    TS_ASSERT_EQUALS(alg.name(), "CreateUserDefinedBackground");
    TS_ASSERT_EQUALS(alg.category(), "CorrectionFunctions\\BackgroundCorrections");
  }

  void test_exec_PointsWS_NormalisePlotsOff() {
    // Turn the "normalise plots" option off
    Mantid::Kernel::ConfigService::Instance().setString(m_key, "Off");

    doTest_pointsWS();
  }

  void test_exec_PointsWS_NormalisePlotsOn() {
    // Turn the "normalise plots" option on
    Mantid::Kernel::ConfigService::Instance().setString(m_key, "On");

    doTest_pointsWS();
  }

  void test_exec_HistoWS_NormalisePlotsOff() {
    // Turn the "normalise plots" option off
    Mantid::Kernel::ConfigService::Instance().setString(m_key, "Off");

    // Create test input
    const auto inputWS = createTestData(true);
    const auto bgPoints = createTable();

    CreateUserDefinedBackground alg;
    alg.setChild(true);
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("InputWorkspace", inputWS));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("OutputBackgroundWorkspace", "__NotUsed"));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("BackgroundPoints", bgPoints))
    TS_ASSERT_THROWS_NOTHING(alg.execute());
    TS_ASSERT(alg.isExecuted());

    MatrixWorkspace_sptr outputWS = alg.getProperty("OutputBackgroundWorkspace");
    TS_ASSERT(outputWS);

    // The expected result
    const auto expected = createExpectedResults(true);
    TS_ASSERT(workspacesEqual(expected, outputWS, 0.105, Comparison::RELATIVE));
  }

  void test_exec_HistoWS_NormalisePlotsOn() {
    // Turn the "normalise plots" option on
    Mantid::Kernel::ConfigService::Instance().setString(m_key, "On");

    // Create test input
    const auto inputWS = createTestData(true);
    const auto bgPoints = createTable();

    CreateUserDefinedBackground alg;
    alg.setChild(true);
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("InputWorkspace", inputWS));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("OutputBackgroundWorkspace", "__NotUsed"));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("BackgroundPoints", bgPoints))
    TS_ASSERT_THROWS_NOTHING(alg.execute());
    TS_ASSERT(alg.isExecuted());

    MatrixWorkspace_sptr outputWS = alg.getProperty("OutputBackgroundWorkspace");
    TS_ASSERT(outputWS);

    // The expected result
    const auto expected = createExpectedResults(true, true);
    TS_ASSERT(workspacesEqual(expected, outputWS, 5e-2, Comparison::ABSOLUTE));
  }

  void test_exec_PointsWS_extend() {
    // Create test input
    const auto inputWS = createTestData(false);
    const auto bgPoints = createTable();

    // Remove last row, to make it extend the background to the data
    bgPoints->removeRow(bgPoints->rowCount() - 1);

    CreateUserDefinedBackground alg;
    alg.setChild(true);
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("InputWorkspace", inputWS));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("OutputBackgroundWorkspace", "__NotUsed"));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("BackgroundPoints", bgPoints))
    TS_ASSERT_THROWS_NOTHING(alg.execute(););
    TS_ASSERT(alg.isExecuted());

    MatrixWorkspace_sptr outputWS = alg.getProperty("OutputBackgroundWorkspace");
    TS_ASSERT(outputWS);

    // The expected result
    const auto expected = createExpectedResults(false);
    TS_ASSERT_DELTA(expected->frequencies(0).back(), outputWS->frequencies(0).back(), 0.001);
  }

  void test_exec_HistoWS_extend_NormalisePlotsOff() {
    // Turn the "normalise plots" option off
    Mantid::Kernel::ConfigService::Instance().setString(m_key, "Off");

    // Create test input
    const auto inputWS = createTestData(true);
    const auto bgPoints = createTable();

    // Remove last row, to make it extend the background to the data
    bgPoints->removeRow(bgPoints->rowCount() - 1);

    CreateUserDefinedBackground alg;
    alg.setChild(true);
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("InputWorkspace", inputWS));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("OutputBackgroundWorkspace", "__NotUsed"));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("BackgroundPoints", bgPoints))
    TS_ASSERT_THROWS_NOTHING(alg.execute());
    TS_ASSERT(alg.isExecuted());

    MatrixWorkspace_sptr outputWS = alg.getProperty("OutputBackgroundWorkspace");
    TS_ASSERT(outputWS);

    // The expected result
    const auto expected = createExpectedResults(true);
    TS_ASSERT_DELTA(expected->counts(0).back(), outputWS->counts(0).back(), 0.001);
  }

  void test_exec_HistoWS_extend_NormalisePlotsOn() {
    // Turn the "normalise plots" option on
    Mantid::Kernel::ConfigService::Instance().setString(m_key, "On");

    // Create test input
    const auto inputWS = createTestData(true);
    const auto bgPoints = createTable();

    // Remove last row, to make it extend the background to the data
    bgPoints->removeRow(bgPoints->rowCount() - 1);

    CreateUserDefinedBackground alg;
    alg.setChild(true);
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("InputWorkspace", inputWS));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("OutputBackgroundWorkspace", "__NotUsed"));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("BackgroundPoints", bgPoints))
    TS_ASSERT_THROWS_NOTHING(alg.execute());
    TS_ASSERT(alg.isExecuted());

    MatrixWorkspace_sptr outputWS = alg.getProperty("OutputBackgroundWorkspace");
    TS_ASSERT(outputWS);

    // The expected result
    const auto expected = createExpectedResults(true, true);
    TS_ASSERT_DELTA(expected->counts(0).back(), outputWS->counts(0).back(), 0.001);
  }

  void test_exec_distribution() {
    // Create test input
    const auto inputWS = createTestData(true);
    const auto bgPoints = createTable(true);
    Mantid::API::WorkspaceHelpers::makeDistribution(inputWS);

    CreateUserDefinedBackground alg;
    alg.setChild(true);
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("InputWorkspace", inputWS));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("OutputBackgroundWorkspace", "__NotUsed"));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("BackgroundPoints", bgPoints))
    TS_ASSERT_THROWS_NOTHING(alg.execute());
    TS_ASSERT(alg.isExecuted());

    MatrixWorkspace_sptr outputWS = alg.getProperty("OutputBackgroundWorkspace");
    TS_ASSERT(outputWS);

    // The expected result
    const auto expected = createExpectedResults(true, false);
    Mantid::API::WorkspaceHelpers::makeDistribution(expected);

    TS_ASSERT(workspacesEqual(expected, outputWS, 0.105, Comparison::RELATIVE));
  }

private:
  /// Create workspace containing test data
  MatrixWorkspace_sptr createTestData(bool isHisto) {
    return WorkspaceCreationHelper::create2DWorkspaceFromFunction(dataFunction, 1, 0.0, 10.0, 0.1, isHisto);
  }

  /// Create table containing user-selected background points
  ITableWorkspace_sptr createTable(bool isDistribution = false) {
    auto table = std::make_shared<Mantid::DataObjects::TableWorkspace>();
    table->addColumn("double", "X");
    table->addColumn("double", "Y");
    double width = 0.1;
    for (int i = 0; i < 100; i++) {
      const auto x = static_cast<double>(i) * width;
      Mantid::API::TableRow row = table->appendRow();
      row << x << (isDistribution ? background(x) / width : background(x));
    }
    return table;
  }

  /// Create expected results
  MatrixWorkspace_sptr createExpectedResults(bool isHisto, bool plotsNormalised = false) {
    std::vector<double> xData, yData, eData;
    constexpr double binWidth = 0.1;
    for (size_t i = 0; i < 100; ++i) {
      const double x = binWidth * static_cast<double>(i);
      xData.emplace_back(x);
      const double y = background(x);
      yData.emplace_back(isHisto && plotsNormalised ? y * binWidth : y);
      eData.emplace_back(0.0);
    }
    if (isHisto) {
      // add last bin edge
      xData.emplace_back(10.0);
    } else {
      // add extra point
      xData.emplace_back(10.0);
      yData.emplace_back(background(10.0));
      eData.emplace_back(0.0);
    }
    auto alg = Mantid::API::AlgorithmFactory::Instance().create("CreateWorkspace", 1);
    alg->initialize();
    alg->setChild(true);
    alg->setProperty("OutputWorkspace", "__NotUsed");
    alg->setProperty("DataX", xData);
    alg->setProperty("DataY", yData);
    alg->setProperty("DataE", eData);
    alg->execute();
    MatrixWorkspace_sptr output = alg->getProperty("OutputWorkspace");

    return output;
  }

  /// Compare workspaces
  enum Comparison : bool { RELATIVE = true, ABSOLUTE = false };
  bool workspacesEqual(const MatrixWorkspace_sptr &lhs, const MatrixWorkspace_sptr &rhs, double tolerance,
                       bool relativeError = Comparison::ABSOLUTE) {
    auto alg = Mantid::API::AlgorithmFactory::Instance().create("CompareWorkspaces", 1);
    alg->setChild(true);
    alg->initialize();
    alg->setProperty<MatrixWorkspace_sptr>("Workspace1", lhs);
    alg->setProperty<MatrixWorkspace_sptr>("Workspace2", rhs);
    alg->setProperty<double>("Tolerance", tolerance);
    alg->setProperty<bool>("ToleranceRelErr", relativeError);
    alg->setProperty<bool>("CheckAxes", false);
    alg->execute();
    return alg->getProperty("Result");
  }

  /// Run test for a point data workspace
  void doTest_pointsWS() {
    // Create test input
    const auto inputWS = createTestData(false);
    const auto bgPoints = createTable();

    CreateUserDefinedBackground alg;
    alg.setChild(true);
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("InputWorkspace", inputWS));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("OutputBackgroundWorkspace", "__NotUsed"));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("BackgroundPoints", bgPoints))
    TS_ASSERT_THROWS_NOTHING(alg.execute(););
    TS_ASSERT(alg.isExecuted());

    MatrixWorkspace_sptr outputWS = alg.getProperty("OutputBackgroundWorkspace");
    TS_ASSERT(outputWS);

    // The expected result
    const auto expected = createExpectedResults(false);
    TS_ASSERT(workspacesEqual(expected, outputWS, 1e-4, Comparison::ABSOLUTE));
  }

  /// Cached string for option
  std::string m_option;

  /// Key for option
  const std::string m_key;
};
