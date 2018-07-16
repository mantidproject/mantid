#ifndef CALCULATEIQTTEST_H_
#define CALCULATEIQTTEST_H_

#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidAlgorithms/CalculateIqt.h"

#include <cxxtest/TestSuite.h>

#include "MantidTestHelpers/WorkspaceCreationHelper.h"

using namespace Mantid::API;
using namespace Mantid::Kernel;

namespace {

Mantid::API::MatrixWorkspace_sptr setUpSampleWorkspace() {
  std::vector<double> xData{0.1, 0.2, 0.3, 0.4, 0.5};
  std::vector<double> yData{0.001, 0.02, 0.4, 0.02, 0.1};

  auto createWorkspace = AlgorithmManager::Instance().create("CreateWorkspace");
  createWorkspace->setChild(true);
  createWorkspace->initialize();
  createWorkspace->setProperty("UnitX", "DeltaE");
  createWorkspace->setProperty("VerticalAxisUnit", "MomentumTransfer");
  createWorkspace->setProperty("VerticalAxisValues", "1");
  createWorkspace->setProperty("DataX", xData);
  createWorkspace->setProperty("DataY", yData);
  createWorkspace->setProperty("NSpec", 1);
  createWorkspace->setPropertyValue("OutputWorkspace", "__calcIqtTest");
  createWorkspace->execute();
  return createWorkspace->getProperty("OutputWorkspace");
}

Mantid::API::MatrixWorkspace_sptr setUpResolutionWorkspace() {
  std::vector<double> xData{0.1, 0.2, 0.3, 0.4, 0.5};
  std::vector<double> yData{0.03, 0.22, 0.05, 0.25, 0.3};

  auto createWorkspace = AlgorithmManager::Instance().create("CreateWorkspace");
  createWorkspace->setChild(true);
  createWorkspace->initialize();
  createWorkspace->setProperty("UnitX", "DeltaE");
  createWorkspace->setProperty("VerticalAxisUnit", "SpectraNumber");
  createWorkspace->setProperty("DataX", xData);
  createWorkspace->setProperty("DataY", yData);
  createWorkspace->setProperty("NSpec", 1);
  createWorkspace->setPropertyValue("OutputWorkspace", "__calcIqtTest");
  createWorkspace->execute();
  return createWorkspace->getProperty("OutputWorkspace");
}

IAlgorithm_sptr calculateIqtAlgorithm(MatrixWorkspace_sptr sample,
                                      MatrixWorkspace_sptr resolution,
                                      const double EnergyMin = -0.5,
                                      const double EnergyMax = 0.5,
                                      const double EnergyWidth = 0.1,
                                      const int NumberOfIterations = 10) {
  auto calculateIqt = AlgorithmManager::Instance().create("CalculateIqt");
  calculateIqt->setChild(true);
  calculateIqt->initialize();
  calculateIqt->setProperty("InputWorkspace", sample);
  calculateIqt->setProperty("ResolutionWorkspace", resolution);
  calculateIqt->setProperty("OutputWorkspace", "_");
  calculateIqt->setProperty("EnergyMin", EnergyMin);
  calculateIqt->setProperty("EnergyMax", EnergyMax);
  calculateIqt->setProperty("EnergyWidth", EnergyWidth);
  calculateIqt->setProperty("NumberOfIterations", NumberOfIterations);
  return calculateIqt;
}
} // namespace

class CalculateIqtTest : public CxxTest::TestSuite {
public:
  static CalculateIqtTest *createSuite() { return new CalculateIqtTest(); }
  static void destroySuite(CalculateIqtTest *suite) { delete suite; }
  MatrixWorkspace_sptr m_sampleWorkspace;
  MatrixWorkspace_sptr m_resolutionWorkspace;

  CalculateIqtTest() {
    m_sampleWorkspace = setUpSampleWorkspace();
    m_resolutionWorkspace = setUpResolutionWorkspace();
  }

  void test_algorithm_executes() {
    auto algorithm =
        calculateIqtAlgorithm(m_sampleWorkspace, m_resolutionWorkspace);
    TS_ASSERT_THROWS_NOTHING(algorithm->execute());
    TS_ASSERT(algorithm->isExecuted());
  }

  void test_output_dimensions_are_correct() {
    auto algorithm =
        calculateIqtAlgorithm(m_sampleWorkspace, m_resolutionWorkspace);
    algorithm->execute();
    MatrixWorkspace_sptr outWorkspace =
        algorithm->getProperty("OutputWorkspace");
    TS_ASSERT_EQUALS(outWorkspace->getNumberHistograms(), 1);
    TS_ASSERT_EQUALS(outWorkspace->blocksize(), 6);
  }

  void test_sample_output_values_are_correct() {
    auto algorithm =
        calculateIqtAlgorithm(m_sampleWorkspace, m_resolutionWorkspace);
    algorithm->execute();
    MatrixWorkspace_sptr outWorkspace =
        algorithm->getProperty("OutputWorkspace");
    auto yValues = outWorkspace->y(0);
    auto eValues = outWorkspace->e(0);
    TS_ASSERT_DELTA(yValues[0], 1, 0.0001);
    TS_ASSERT_DELTA(yValues[1], 0, 0.0001);
    TS_ASSERT_DELTA(yValues[4], 0.4831171, 0.0001);
    TS_ASSERT_DELTA(eValues[0], 0, 0.0001);
  }

  void test_throws_if_energy_bounds_invalid() {
    auto energyMin = 0.5;
    auto energyMax = -1; // invalid - less than energyMin
    auto algorithm = calculateIqtAlgorithm(
        m_sampleWorkspace, m_resolutionWorkspace, energyMin, energyMax);
    TS_ASSERT_THROWS(algorithm->execute(), std::runtime_error);
    TS_ASSERT(!algorithm->isExecuted());
  }

  void test_throws_if_number_of_iterations_is_negative() {
    auto nIterations = -1;
    TS_ASSERT_THROWS(calculateIqtAlgorithm(m_sampleWorkspace,
                                           m_resolutionWorkspace, -0.5, 0.5,
                                           0.1, nIterations),
                     std::invalid_argument);
  }

  void test_throws_if_number_of_iterations_is_zero() {
    auto nIterations = 0;
    TS_ASSERT_THROWS(calculateIqtAlgorithm(m_sampleWorkspace,
                                           m_resolutionWorkspace, -0.5, 0.5,
                                           0.1, nIterations),
                     std::invalid_argument);
  }

  // void test_throws_if_number_of_iterations_is_not_an_integer() {
  //  auto nIterations = 0.2;
  //  TS_ASSERT_THROWS(calculateIqtAlgorithm(m_sampleWorkspace,
  //                                         m_resolutionWorkspace, -0.5, 0.5,
  //                                         0.1, nIterations),
  //                   std::invalid_argument);
  //}
};
#endif
