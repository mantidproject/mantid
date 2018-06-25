#ifndef CALCULATEIQTTEST_H_
#define CALCULATEIQTTEST_H_

#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidAlgorithms/CalculateIqt.h"

#include <cxxtest/TestSuite.h>

#include "MantidTestHelpers/WorkspaceCreationHelper.h"

using namespace Mantid::API;
using namespace Mantid::Kernel;

class CalculateIqtTest : public CxxTest::TestSuite {
public:
  static CalculateIqtTest *createSuite() { return new CalculateIqtTest(); }
  static void destroySuite(CalculateIqtTest *suite) { delete suite; }
  MatrixWorkspace_sptr m_sampleWorkspace;
  MatrixWorkspace_sptr m_resolutionWorkspace;

  CalculateIqtTest() {
    m_sampleWorkspace = setUpWorkspace(true);
    m_resolutionWorkspace = setUpWorkspace(false);
  }

  void test_execSuccess() {
    auto algorithm =
        calculateIqtAlgorithm(m_sampleWorkspace, m_resolutionWorkspace);
    TS_ASSERT_THROWS_NOTHING(algorithm->execute());
    TS_ASSERT(algorithm->isExecuted());
    MatrixWorkspace_sptr outWorkspace =
        algorithm->getProperty("OutputWorkspace");
    TS_ASSERT_EQUALS(outWorkspace->getNumberHistograms(), 1);
    TS_ASSERT_EQUALS(outWorkspace->blocksize(), 6);
  }

  void test_invalidEnergyBounds() {
    auto algorithm = calculateIqtAlgorithm(m_sampleWorkspace,
                                           m_resolutionWorkspace, 0.5, -1);
    TS_ASSERT_THROWS(algorithm->execute(), std::runtime_error);
    TS_ASSERT(!algorithm->isExecuted());
  }

  void test_invalidIterations() {
    TS_ASSERT_THROWS(calculateIqtAlgorithm(m_sampleWorkspace,
                                           m_resolutionWorkspace, -0.5, 0.5,
                                           0.1, -1),
                     std::invalid_argument);
  }

  Mantid::API::MatrixWorkspace_sptr setUpWorkspace(const bool numericAxis) {
    std::vector<double> xData{1, 2, 3, 4, 5};
    std::vector<double> yData{1, 2, 4, 2, 1};

    auto createWorkspace =
        AlgorithmManager::Instance().create("CreateWorkspace");
    createWorkspace->setChild(true);
    createWorkspace->initialize();
    if (numericAxis) {
      createWorkspace->setProperty("UnitX", "DeltaE");
      createWorkspace->setProperty("VerticalAxisUnit", "MomentumTransfer");
      createWorkspace->setProperty("VerticalAxisValues", "1");
    } else {
      createWorkspace->setProperty("UnitX", "DeltaE");
      createWorkspace->setProperty("VerticalAxisUnit", "SpectraNumber");
    }
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
};

#endif
