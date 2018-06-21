#ifndef CALCULATEIQTTEST_H_
#define CALCULATEIQTTEST_H_

#include "MantidAPI/AlgorithmManager.h"
#include "MantidAlgorithms/CalculateIqt.h"
#include "MantidAPI/FrameworkManager.h"

#include <cxxtest/TestSuite.h>

#include "MantidTestHelpers/WorkspaceCreationHelper.h"

using namespace Mantid::API;
using namespace Mantid::Kernel;

class CalculateIqtTest : public CxxTest::TestSuite {
public:
 
  static CalculateIqtTest *createSuite() { return new CalculateIqtTest(); }
  static void destroySuite(CalculateIqtTest *suite) { delete suite; }

  CalculateIqtTest() { FrameworkManager::Instance(); }


  void testExec_success() {
    auto sampleWorkspace = setUpWorkspace(true);
    auto resolutionWorkspace = setUpWorkspace(false);
    auto ws = calculateIqt(sampleWorkspace, resolutionWorkspace);
    TS_ASSERT(ws);
  }

  Mantid::API::MatrixWorkspace_sptr setUpWorkspace(const bool numericAxis) {
    const std::vector<double> xData{1, 2, 3, 4, 5};
    const std::vector<double> yData{0, 1, 3, 1, 0};

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

  MatrixWorkspace_sptr calculateIqt(MatrixWorkspace_sptr sample,
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
    calculateIqt->setProperty("EnergyMin", -0.5);
    calculateIqt->setProperty("EnergyMax", 0.5);
    calculateIqt->setProperty("EnergyWidth", 0.1);
    calculateIqt->setProperty("NumberOfIterations", NumberOfIterations);
    return calculateIqt->getProperty("OutputWorkspace");
  }
  
};

#endif
