// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_MD_CONVERT2_QxyzDE_TEST_H_
#define MANTID_MD_CONVERT2_QxyzDE_TEST_H_

#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/Sample.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidGeometry/Crystal/OrientedLattice.h"
#include "MantidGeometry/Instrument/Goniometer.h"

#include "MantidMDAlgorithms/ConvertToMD.h"
#include "MantidMDAlgorithms/MDWSDescription.h"
#include "MantidTestHelpers/ComponentCreationHelper.h"
#include "MantidTestHelpers/MDEventsTestHelper.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"

#include <cxxtest/TestSuite.h>

using namespace Mantid;
using namespace Mantid::Kernel;
using Mantid::Geometry::OrientedLattice;
using namespace Mantid::API;
using namespace Mantid::DataObjects;
using namespace Mantid::MDAlgorithms;

class ConvertTo3DdETestHelper : public ConvertToMD {
public:
  ConvertTo3DdETestHelper() {}
};

// Test is transformed from ConvetToQ3DdE but actually tests some aspects of
// ConvertToMD algorithm.
class ConvertToQ3DdETest : public CxxTest::TestSuite {
  std::unique_ptr<ConvertTo3DdETestHelper> pAlg;

public:
  static ConvertToQ3DdETest *createSuite() { return new ConvertToQ3DdETest(); }
  static void destroySuite(ConvertToQ3DdETest *suite) { delete suite; }

  void testInit() { TS_ASSERT(pAlg->isInitialized()) }

  void testExecThrow() {
    Mantid::API::MatrixWorkspace_sptr ws2D =
        WorkspaceCreationHelper::createGroupedWorkspace2DWithRingsAndBoxes();

    AnalysisDataService::Instance().addOrReplace("testWSProcessed", ws2D);

    TSM_ASSERT_THROWS(" the workspace X axis does not have units ",
                      pAlg->setPropertyValue("InputWorkspace", ws2D->getName()),
                      const std::invalid_argument &);
    TS_ASSERT_THROWS_NOTHING(
        pAlg->setPropertyValue("OutputWorkspace", "EnergyTransfer4DWS"));
  }

  /** Calculate min-max value defaults*/
  Mantid::API::IAlgorithm *
  calcMinMaxValDefaults(const std::string &QMode, const std::string &QFrame,
                        std::string OtherProperties = std::string("")) {

    Mantid::API::IAlgorithm *childAlg =
        Mantid::API::FrameworkManager::Instance().createAlgorithm(
            "ConvertToMDMinMaxLocal");
    if (!childAlg) {
      TSM_ASSERT("Can not create child ChildAlgorithm to found min/max values",
                 false);
      return nullptr;
    }
    childAlg->initialize();
    if (!childAlg->isInitialized()) {
      TSM_ASSERT(
          "Can not initialize child ChildAlgorithm to found min/max values",
          false);
      return nullptr;
    }
    childAlg->setPropertyValue("InputWorkspace", "testWSProcessed");
    childAlg->setPropertyValue("QDimensions", QMode);
    childAlg->setPropertyValue("dEAnalysisMode", "Direct");
    childAlg->setPropertyValue("Q3DFrames", QFrame);
    childAlg->setPropertyValue("OtherDimensions", OtherProperties);

    childAlg->execute();
    if (!childAlg->isExecuted()) {
      TSM_ASSERT("Can not execute child ChildAlgorithm to found min/max values",
                 false);
      return nullptr;
    }
    return childAlg;
  }

  void testExecRunsOnNewWorkspaceNoLimits() {
    Mantid::API::MatrixWorkspace_sptr ws2D = WorkspaceCreationHelper::
        createProcessedWorkspaceWithCylComplexInstrument(100, 10, true);
    // add workspace energy
    ws2D->mutableRun().addProperty("Ei", 12., "meV", true);

    AnalysisDataService::Instance().addOrReplace("testWSProcessed", ws2D);
    // clear stuff from analysis data service for test to work in specified way
    AnalysisDataService::Instance().remove("EnergyTransfer4DWS");

    TSM_ASSERT_THROWS_NOTHING(
        "the inital is not in the units of energy transfer",
        pAlg->setPropertyValue("InputWorkspace", ws2D->getName()));
    TS_ASSERT_THROWS_NOTHING(
        pAlg->setPropertyValue("OutputWorkspace", "EnergyTransfer4DWS"));
    TS_ASSERT_THROWS_NOTHING(pAlg->setPropertyValue("QDimensions", "Q3D"));
    TS_ASSERT_THROWS_NOTHING(
        pAlg->setPropertyValue("dEAnalysisMode", "Direct"));

    pAlg->execute();
    if (!pAlg->isExecuted()) {
      TSM_ASSERT(
          "have not executed convertToMD without min-max limits specied ",
          false);
      return;
    }

    auto childAlg = calcMinMaxValDefaults("Q3D", "HKL");
    if (!childAlg)
      return;
    // get the results
    std::vector<double> minVal = childAlg->getProperty("MinValues");
    std::vector<double> maxVal = childAlg->getProperty("MaxValues");
    auto outWS = AnalysisDataService::Instance().retrieveWS<IMDWorkspace>(
        "EnergyTransfer4DWS");

    size_t NDims = outWS->getNumDims();
    for (size_t i = 0; i < NDims; i++) {
      const Geometry::IMDDimension *pDim = outWS->getDimension(i).get();
      TS_ASSERT_DELTA(minVal[i], pDim->getMinimum(), 1.e-4);
      TS_ASSERT_DELTA(maxVal[i], pDim->getMaximum(), 1.e-4);
    }
  }

  void testExecRunsOnNewWorkspaceNoLimits5D() {
    Mantid::API::MatrixWorkspace_sptr ws2D = WorkspaceCreationHelper::
        createProcessedWorkspaceWithCylComplexInstrument(100, 10, true);
    // add workspace energy
    ws2D->mutableRun().addProperty("Ei", 12., "meV", true);

    AnalysisDataService::Instance().addOrReplace("testWSProcessed", ws2D);
    // clear stuff from analysis data service for test to work in specified way
    AnalysisDataService::Instance().remove("EnergyTransfer4DWS");

    TSM_ASSERT_THROWS_NOTHING(
        "the inital is not in the units of energy transfer",
        pAlg->setPropertyValue("InputWorkspace", ws2D->getName()));
    TS_ASSERT_THROWS_NOTHING(
        pAlg->setPropertyValue("OutputWorkspace", "EnergyTransfer5DWS"));
    TS_ASSERT_THROWS_NOTHING(pAlg->setPropertyValue("QDimensions", "Q3D"));
    TS_ASSERT_THROWS_NOTHING(pAlg->setPropertyValue("OtherDimensions", "Ei"));
    TS_ASSERT_THROWS_NOTHING(
        pAlg->setPropertyValue("dEAnalysisMode", "Direct"));

    pAlg->execute();
    if (!pAlg->isExecuted()) {
      TSM_ASSERT(
          "have not executed convertToMD without min-max limits specied ",
          false);
      return;
    }

    auto childAlg = calcMinMaxValDefaults("Q3D", "HKL", std::string("Ei"));
    if (!childAlg)
      return;
    // get the results
    std::vector<double> minVal = childAlg->getProperty("MinValues");
    std::vector<double> maxVal = childAlg->getProperty("MaxValues");
    auto outWS = AnalysisDataService::Instance().retrieveWS<IMDWorkspace>(
        "EnergyTransfer5DWS");

    size_t NDims = outWS->getNumDims();
    for (size_t i = 0; i < NDims - 1; i++) {
      const Geometry::IMDDimension *pDim = outWS->getDimension(i).get();
      TS_ASSERT_DELTA(minVal[i], pDim->getMinimum(), 1.e-4);
      TS_ASSERT_DELTA(maxVal[i], pDim->getMaximum(), 1.e-4);
    }
    size_t nun5D = 4;
    const Geometry::IMDDimension *pDim = outWS->getDimension(nun5D).get();
    TS_ASSERT_DELTA(minVal[nun5D] * 0.9, pDim->getMinimum(), 1.e-4);
    TS_ASSERT_DELTA(maxVal[nun5D] * 1.1, pDim->getMaximum(), 1.e-4);
  }

  void testExecWorksAutoLimitsOnNewWorkspaceNoMinMaxLimits() {
    Mantid::API::MatrixWorkspace_sptr ws2D = WorkspaceCreationHelper::
        createProcessedWorkspaceWithCylComplexInstrument(100, 10, true);
    // add workspace energy
    ws2D->mutableRun().addProperty("Ei", 12., "meV", true);

    AnalysisDataService::Instance().addOrReplace("testWSProcessed", ws2D);
    // clear stuff from analysis data service for test to work in specified way
    AnalysisDataService::Instance().remove("EnergyTransfer4DWS");

    TS_ASSERT_THROWS_NOTHING(pAlg->setPropertyValue("QDimensions", "Q3D"));
    TS_ASSERT_THROWS_NOTHING(
        pAlg->setPropertyValue("dEAnalysisMode", "Direct"));
    TS_ASSERT_THROWS_NOTHING(pAlg->setPropertyValue("OtherDimensions", ""));
    TS_ASSERT_THROWS_NOTHING(
        pAlg->setPropertyValue("InputWorkspace", ws2D->getName()));

    TS_ASSERT_THROWS_NOTHING(
        pAlg->setPropertyValue("OutputWorkspace", "EnergyTransfer4DWS"));
    TS_ASSERT_THROWS_NOTHING(pAlg->setPropertyValue("MaxValues", ""));
    TS_ASSERT_THROWS_NOTHING(pAlg->setPropertyValue("MinValues", ""));

    //  pAlg->setRethrows(true);
    pAlg->execute();
    if (!pAlg->isExecuted()) {
      TSM_ASSERT("have not executed convertToMD with only min limits specied ",
                 false);
      return;
    }

    auto childAlg = calcMinMaxValDefaults("Q3D", "HKL");
    if (!childAlg)
      return;

    // get the results
    std::vector<double> minVal = childAlg->getProperty("MinValues");
    std::vector<double> maxVal = childAlg->getProperty("MaxValues");

    auto outWS = AnalysisDataService::Instance().retrieveWS<IMDWorkspace>(
        "EnergyTransfer4DWS");

    size_t NDims = outWS->getNumDims();
    for (size_t i = 0; i < NDims; i++) {
      const Geometry::IMDDimension *pDim = outWS->getDimension(i).get();
      TS_ASSERT_DELTA(minVal[i], pDim->getMinimum(), 1.e-4);
      TS_ASSERT_DELTA(maxVal[i], pDim->getMaximum(), 1.e-4);
    }
  }
  void testExecFine() {
    // create model processed workpsace with 10x10 cylindrical detectors, 10
    // energy levels and oriented lattice
    Mantid::API::MatrixWorkspace_sptr ws2D = WorkspaceCreationHelper::
        createProcessedWorkspaceWithCylComplexInstrument(100, 10, true);
    // add workspace energy
    ws2D->mutableRun().addProperty("Ei", 12., "meV", true);

    AnalysisDataService::Instance().addOrReplace("testWSProcessed", ws2D);

    TS_ASSERT_THROWS_NOTHING(pAlg->setPropertyValue("QDimensions", "Q3D"));
    TS_ASSERT_THROWS_NOTHING(
        pAlg->setPropertyValue("dEAnalysisMode", "Direct"));
    TS_ASSERT_THROWS_NOTHING(
        pAlg->setPropertyValue("InputWorkspace", ws2D->getName()));

    TS_ASSERT_THROWS_NOTHING(
        pAlg->setPropertyValue("OutputWorkspace", "EnergyTransfer4DWS"));
    TS_ASSERT_THROWS_NOTHING(
        pAlg->setPropertyValue("MinValues", "-50.,-50.,-50,-2"));
    TS_ASSERT_THROWS_NOTHING(
        pAlg->setPropertyValue("MaxValues", " 50., 50., 50, 20"));

    pAlg->execute();
    TSM_ASSERT("Should be successful ", pAlg->isExecuted());
  }
  void testExecAndAdd() {
    // create model processed workpsace with 10x10 cylindrical detectors, 10
    // energy levels and oriented lattice
    Mantid::API::MatrixWorkspace_sptr ws2D = WorkspaceCreationHelper::
        createProcessedWorkspaceWithCylComplexInstrument(100, 10, true);

    // rotate the crystal by twenty degrees back;
    ws2D->mutableRun().mutableGoniometer().setRotationAngle(0, 20);
    // add workspace energy
    ws2D->mutableRun().addProperty("Ei", 13., "meV", true);
    //

    AnalysisDataService::Instance().addOrReplace("testWSProcessed", ws2D);

    TS_ASSERT_THROWS_NOTHING(pAlg->setPropertyValue("QDimensions", "Q3D"));
    TS_ASSERT_THROWS_NOTHING(
        pAlg->setPropertyValue("dEAnalysisMode", "Indirect"));
    TS_ASSERT_THROWS_NOTHING(
        pAlg->setPropertyValue("InputWorkspace", ws2D->getName()));

    TS_ASSERT_THROWS_NOTHING(
        pAlg->setPropertyValue("OutputWorkspace", "EnergyTransfer4DWS"));
    TS_ASSERT_THROWS_NOTHING(
        pAlg->setPropertyValue("MinValues", "-50.,-50.,-50,-2"));
    TS_ASSERT_THROWS_NOTHING(
        pAlg->setPropertyValue("MaxValues", " 50., 50., 50, 20"));

    pAlg->execute();
    TSM_ASSERT("Should succseed as adding should work fine ",
               pAlg->isExecuted());
  }

  // check the results;
  void testAlgorithmExecution() {
    std::vector<double> L2(3, 10), polar(3, 0), azim(3, 0);
    polar[1] = 1;
    polar[2] = 2;
    azim[0] = -1;
    azim[2] = 1;

    Mantid::API::MatrixWorkspace_sptr ws2D =
        WorkspaceCreationHelper::createProcessedInelasticWS(L2, polar, azim, 3,
                                                            -1, 2, 10);

    ws2D->mutableRun().mutableGoniometer().setRotationAngle(0, 0); // gl
    ws2D->mutableRun().mutableGoniometer().setRotationAngle(1, 0);
    ws2D->mutableRun().mutableGoniometer().setRotationAngle(2, 0);

    AnalysisDataService::Instance().addOrReplace("testWSProcessed", ws2D);

    TSM_ASSERT_THROWS_NOTHING(
        "the inital is not in the units of energy transfer",
        pAlg->setPropertyValue("InputWorkspace", ws2D->getName()));
    TS_ASSERT_THROWS_NOTHING(
        pAlg->setPropertyValue("OutputWorkspace", "EnergyTransfer4DWS"));
    TS_ASSERT_THROWS_NOTHING(
        pAlg->setPropertyValue("dEAnalysisMode", "Direct"));
    TS_ASSERT_THROWS_NOTHING(
        pAlg->setPropertyValue("MinValues", "-10.,-10.,-10,-2"));
    TS_ASSERT_THROWS_NOTHING(
        pAlg->setPropertyValue("MaxValues", " 10., 10., 10, 8"))
    TS_ASSERT_THROWS_NOTHING(pAlg->setPropertyValue("PreprocDetectorsWS", ""));

    pAlg->execute();
    TSM_ASSERT("Should be successful ", pAlg->isExecuted());

    Mantid::API::Workspace_sptr wsOut =
        AnalysisDataService::Instance().retrieve("EnergyTransfer4DWS");
    TSM_ASSERT(
        "Can not retrieve resulting workspace from the analysis data service ",
        wsOut);
  }

  void testWithExistingLatticeTrowsLowEnergy() {
    // create model processed workpsace with 10x10 cylindrical detectors, 10
    // energy levels and oriented lattice
    Mantid::API::MatrixWorkspace_sptr ws2D = WorkspaceCreationHelper::
        createProcessedWorkspaceWithCylComplexInstrument(100, 10, true);
    // add workspace energy
    ws2D->mutableRun().addProperty("Ei", 2., "meV", true);

    AnalysisDataService::Instance().addOrReplace("testWSProcessed", ws2D);

    auto alg = std::make_unique<ConvertTo3DdETestHelper>();
    alg->initialize();
    TS_ASSERT_THROWS_NOTHING(alg->setPropertyValue("QDimensions", "Q3D"));
    TS_ASSERT_THROWS_NOTHING(alg->setPropertyValue("dEAnalysisMode", "Direct"));
    TS_ASSERT_THROWS_NOTHING(
        alg->setPropertyValue("InputWorkspace", ws2D->getName()));
    TS_ASSERT_THROWS_NOTHING(
        alg->setPropertyValue("OutputWorkspace", "EnergyTransfer4DWS"));
    TS_ASSERT_THROWS_NOTHING(
        alg->setPropertyValue("MinValues", "-50.,-50.,-50,-2"));
    TS_ASSERT_THROWS_NOTHING(
        alg->setPropertyValue("MaxValues", " 50., 50.,-50,10"));

    TS_ASSERT_THROWS(alg->execute(), const std::runtime_error &);
    TSM_ASSERT(
        "Should be not-successful as input energy was lower then obtained",
        !alg->isExecuted());
  }

  ConvertToQ3DdETest() {
    pAlg = std::make_unique<ConvertTo3DdETestHelper>();
    pAlg->initialize();
    // initialize (load)Matid algorithm framework -- needed to run this test
    // separately
    Mantid::API::IAlgorithm *childAlg =
        Mantid::API::FrameworkManager::Instance().createAlgorithm(
            "ConvertUnits");
    TSM_ASSERT("Can not initialize Mantid algorithm framework", childAlg);
    if (!childAlg) {
      throw(std::runtime_error("Can not initalize/Load MantidAlgorithm dll"));
    }
  }
};

#endif /* MANTID_MDEVENTS_MAKEDIFFRACTIONMDEVENTWORKSPACETEST_H_ */
