// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef REMOVE_BACKGROUD_TEST_H_
#define REMOVE_BACKGROUD_TEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/Axis.h"

#include "MantidAlgorithms/CalculateFlatBackground.h"
#include "MantidAlgorithms/ConvertUnits.h"
#include "MantidAlgorithms/Rebin.h"
#include "MantidAlgorithms/RemoveBackground.h"

#include "MantidTestHelpers/WorkspaceCreationHelper.h"
using namespace Mantid;

void init_workspaces(int nHist, int nBins, API::MatrixWorkspace_sptr &BgWS,
                     API::MatrixWorkspace_sptr &SourceWS) {
  DataObjects::Workspace2D_sptr theWS =
      WorkspaceCreationHelper::create2DWorkspaceWithFullInstrument(nHist,
                                                                   nBins);
  // Add incident energy necessary for unit conversion
  theWS->mutableRun().addProperty("Ei", 13., "meV", true);

  API::AnalysisDataService::Instance().addOrReplace("sourceWS", theWS);

  Algorithms::Rebin rebinner;
  rebinner.initialize();
  rebinner.setPropertyValue("InputWorkspace", theWS->getName());
  rebinner.setPropertyValue("OutputWorkspace", "Background");
  rebinner.setPropertyValue("Params", "10000,5000,15000");

  rebinner.execute();

  BgWS = API::AnalysisDataService::Instance().retrieveWS<API::MatrixWorkspace>(
      "Background");

  Algorithms::ConvertUnits unitsConv;
  unitsConv.initialize();
  unitsConv.setPropertyValue("InputWorkspace", theWS->getName());
  unitsConv.setPropertyValue("OutputWorkspace", "sourceWSdE");
  unitsConv.setPropertyValue("Target", "DeltaE");
  unitsConv.setPropertyValue("EMode", "Direct");

  unitsConv.execute();

  Algorithms::CalculateFlatBackground bgRemoval;

  bgRemoval.initialize();
  bgRemoval.setPropertyValue("InputWorkspace", theWS->getName());
  bgRemoval.setPropertyValue("OutputWorkspace", theWS->getName());
  bgRemoval.setPropertyValue("StartX", "10000");
  bgRemoval.setPropertyValue("EndX", "15000");
  bgRemoval.setPropertyValue("Mode", "Mean");

  bgRemoval.execute();

  unitsConv.setPropertyValue("InputWorkspace", theWS->getName());
  unitsConv.setPropertyValue("OutputWorkspace", "sampleWSdE");
  unitsConv.setPropertyValue("Target", "DeltaE");
  unitsConv.setPropertyValue("EMode", "Direct");

  unitsConv.execute();

  SourceWS =
      API::AnalysisDataService::Instance().retrieveWS<API::MatrixWorkspace>(
          "sourceWSdE");
}

class RemoveBackgroundTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static RemoveBackgroundTest *createSuite() {
    return new RemoveBackgroundTest();
  }
  static void destroySuite(RemoveBackgroundTest *suite) { delete suite; }

  RemoveBackgroundTest() { init_workspaces(1, 15000, BgWS, SourceWS); }

  ~RemoveBackgroundTest() override {
    BgWS.reset();
    SourceWS.reset();
  }
  void testWrongInit() {
    Algorithms::BackgroundHelper bgRemoval;
    // create workspace with units of energy transfer
    auto bkgWS = WorkspaceCreationHelper::createProcessedInelasticWS(
        std::vector<double>(1, 1.), std::vector<double>(1, 20.),
        std::vector<double>(1, 10.));
    TSM_ASSERT_THROWS(
        "Should throw if background workspace is not in TOF units",
        bgRemoval.initialize(bkgWS, SourceWS, 0), const std::invalid_argument &);

    bkgWS = WorkspaceCreationHelper::create2DWorkspaceWithFullInstrument(2, 15);
    TSM_ASSERT_THROWS("Should throw if background is not 1 or equal to source",
                      bgRemoval.initialize(bkgWS, SourceWS, 0),
                      const std::invalid_argument &);
  }

  void testBackgroundHelper() {
    Algorithms::BackgroundHelper bgRemoval;
    auto clone = cloneSourceWS();

    API::AnalysisDataService::Instance().addOrReplace("TestWS", clone);

    int emode = static_cast<int>(Kernel::DeltaEMode().fromString("Direct"));
    bgRemoval.initialize(BgWS, SourceWS, emode);

    auto &dataX = clone->mutableX(0);
    auto &dataY = clone->mutableY(0);
    auto &dataE = clone->mutableE(0);

    bgRemoval.removeBackground(0, dataX, dataY, dataE);

    auto SampleWS =
        API::AnalysisDataService::Instance().retrieveWS<API::MatrixWorkspace>(
            "sampleWSdE");

    auto &sampleX = SampleWS->x(0);
    auto &sampleY = SampleWS->y(0);
    // const MantidVec & sampleE = SampleWS->readE(0);
    for (size_t i = 0; i < sampleY.size(); i++) {
      TS_ASSERT_DELTA(dataX[i], sampleX[i], 1.e-7);
      TS_ASSERT_DELTA(dataY[i], sampleY[i], 1.e-7);
    }
  }

  void testRemoveBkgInPlace() {
    auto clone = cloneSourceWS();
    API::AnalysisDataService::Instance().addOrReplace("TestWS", clone);

    Algorithms::RemoveBackground bkgRem;
    bkgRem.initialize();
    bkgRem.setPropertyValue("InputWorkspace", "TestWS");
    bkgRem.setPropertyValue("OutputWorkspace", "TestWS");
    bkgRem.setPropertyValue("BkgWorkspace", BgWS->getName());
    bkgRem.setPropertyValue("EMode", "Direct");

    TS_ASSERT_THROWS_NOTHING(bkgRem.execute());

    auto SampleWS =
        API::AnalysisDataService::Instance().retrieveWS<API::MatrixWorkspace>(
            "sampleWSdE");
    auto result = clone;

    auto &sampleX = SampleWS->x(0);
    auto &sampleY = SampleWS->y(0);

    auto &resultX = result->x(0);
    auto &resultY = result->y(0);

    // const MantidVec & sampleE = SampleWS->readE(0);
    for (size_t i = 0; i < sampleY.size(); i++) {
      TS_ASSERT_DELTA(resultX[i], sampleX[i], 1.e-7);
      TS_ASSERT_DELTA(resultY[i], sampleY[i], 1.e-7);
    }
  }

  void testRemoveBkgNewRez() {
    auto clone = cloneSourceWS();
    API::AnalysisDataService::Instance().addOrReplace("TestWS", clone);

    Algorithms::RemoveBackground bkgRem;
    bkgRem.initialize();
    bkgRem.setPropertyValue("InputWorkspace", "TestWS");
    bkgRem.setPropertyValue("OutputWorkspace", "TestWS2");
    bkgRem.setPropertyValue("BkgWorkspace", BgWS->getName());
    bkgRem.setPropertyValue("EMode", "Direct");

    TS_ASSERT_THROWS_NOTHING(bkgRem.execute());

    auto SampleWS =
        API::AnalysisDataService::Instance().retrieveWS<API::MatrixWorkspace>(
            "sampleWSdE");
    auto result =
        API::AnalysisDataService::Instance().retrieveWS<API::MatrixWorkspace>(
            "TestWS2");

    auto &sampleX = SampleWS->x(0);
    auto &sampleY = SampleWS->y(0);

    auto &resultX = result->x(0);
    auto &resultY = result->y(0);

    // const MantidVec & sampleE = SampleWS->readE(0);
    for (size_t i = 0; i < sampleY.size(); i++) {
      TS_ASSERT_DELTA(resultX[i], sampleX[i], 1.e-7);
      TS_ASSERT_DELTA(resultY[i], sampleY[i], 1.e-7);
    }
  }

  void testNullifyNegatives() {

    auto clone = cloneSourceWS();
    // set negative values to signal
    auto &Y = clone->dataY(0);
    for (double &i : Y) {
      i = -1000;
    }
    // Create zero background workspace
    // Create the workspace
    auto bgWS = boost::dynamic_pointer_cast<DataObjects::Workspace2D>(
        API::WorkspaceFactory::Instance().create("Workspace2D", 1, 2, 1));
    auto binEdges = {0.0, 1.0};
    bgWS->setBinEdges(0, binEdges);
    bgWS->getAxis(0)->setUnit("TOF");
    auto &Ybg = bgWS->mutableY(0);
    Ybg[0] = 0;
    auto &Ebg = bgWS->mutableE(0);
    Ebg[0] = 0;

    // remove background. If bacground is fully 0, algorithm just removes
    // negative values
    Algorithms::RemoveBackground bkgRem;
    bkgRem.initialize();
    bkgRem.setProperty("InputWorkspace", clone);
    bkgRem.setPropertyValue("OutputWorkspace", "RemovedBgWS");
    bkgRem.setProperty("BkgWorkspace", bgWS);
    bkgRem.setPropertyValue("EMode", "Direct");
    bkgRem.setProperty("NullifyNegativeValues", true);

    TS_ASSERT_THROWS_NOTHING(bkgRem.execute());

    auto result =
        API::AnalysisDataService::Instance().retrieveWS<API::MatrixWorkspace>(
            "RemovedBgWS");

    auto &resY = result->y(0);
    auto &resE = result->e(0);

    // const MantidVec & sampleE = SampleWS->readE(0);
    for (size_t i = 0; i < resY.size(); i++) {
      TS_ASSERT_DELTA(resY[i], 0, 1.e-7);
      TS_ASSERT_DELTA(resE[i], 1000., 1.e-3);
    }
  }

private:
  API::MatrixWorkspace_sptr cloneSourceWS() {
    auto cloneWS = API::WorkspaceFactory::Instance().create(SourceWS);

    cloneWS->setHistogram(0, SourceWS->histogram(0));

    return cloneWS;
  }

  API::MatrixWorkspace_sptr BgWS;
  API::MatrixWorkspace_sptr SourceWS;
};

class RemoveBackgroundTestPerformance : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other xests
  static RemoveBackgroundTestPerformance *createSuite() {
    return new RemoveBackgroundTestPerformance();
  }
  static void destroySuite(RemoveBackgroundTestPerformance *suite) {
    delete suite;
  }

  RemoveBackgroundTestPerformance() {
    init_workspaces(1000, 15000, BgWS, SourceWS);
  }

  void testRemoveBkgInPlace() {

    Algorithms::RemoveBackground bkgRem;
    bkgRem.initialize();
    bkgRem.setPropertyValue("InputWorkspace", "sourceWSdE");
    bkgRem.setPropertyValue("OutputWorkspace", "sourceWSdE");
    bkgRem.setPropertyValue("BkgWorkspace", BgWS->getName());
    bkgRem.setPropertyValue("EMode", "Direct");

    bkgRem.execute();
  }

private:
  API::MatrixWorkspace_sptr BgWS;
  API::MatrixWorkspace_sptr SourceWS;
};

#endif /*REMOVE_BACKGROUD_TEST_H_*/
