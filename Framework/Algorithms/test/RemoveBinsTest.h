// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <cxxtest/TestSuite.h>

#include <sstream>
#include <stdexcept>
#include <string>

#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/Axis.h"
#include "MantidAlgorithms/RemoveBins.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidFrameworkTestHelpers/WorkspaceCreationHelper.h"
#include "MantidHistogramData/LinearGenerator.h"
#include "MantidKernel/UnitFactory.h"

using namespace Mantid::Algorithms;
using namespace Mantid::API;
using namespace Mantid::DataObjects;
using Mantid::HistogramData::BinEdges;
using Mantid::HistogramData::LinearGenerator;

class RemoveBinsTest : public CxxTest::TestSuite {
public:
  void testName() { TS_ASSERT_EQUALS(alg.name(), "RemoveBins") }

  void testInit() {
    alg.initialize();
    TS_ASSERT(alg.isInitialized())
  }

  void testSetProperties() {
    makeDummyWorkspace2D();

    alg.setPropertyValue("InputWorkspace", "input2D");
    alg.setPropertyValue("OutputWorkspace", "output");
    alg.setPropertyValue("XMin", "0");
    alg.setPropertyValue("XMax", "5");

    TS_ASSERT_EQUALS(alg.getPropertyValue("XMin"), "0");
    TS_ASSERT_EQUALS(alg.getPropertyValue("XMax"), "5");
  }

  void testExec() {

    try {
      TS_ASSERT_EQUALS(alg.execute(), true);
    } catch (std::runtime_error &e) {
      TS_FAIL(e.what());
    }

    MatrixWorkspace_const_sptr outputWS = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>("output");

    // Should give:
    // 10   20   30   40   X
    //     2     5     6       Y

    TS_ASSERT_EQUALS(outputWS->x(0).size(), 4);
    TS_ASSERT_EQUALS(outputWS->y(0).size(), 3);
    TS_ASSERT_EQUALS(outputWS->x(0)[0], 10);
    TS_ASSERT_EQUALS(outputWS->y(0)[0], 2);
  }

  void testRemoveFromBack() {
    alg3.initialize();
    TS_ASSERT(alg3.isInitialized())

    alg3.setPropertyValue("InputWorkspace", "input2D");
    alg3.setPropertyValue("OutputWorkspace", "output2");
    alg3.setPropertyValue("XMin", "35");
    alg3.setPropertyValue("XMax", "40");

    TS_ASSERT_EQUALS(alg3.getPropertyValue("XMin"), "35");
    TS_ASSERT_EQUALS(alg3.getPropertyValue("XMax"), "40");

    try {
      TS_ASSERT_EQUALS(alg3.execute(), true);
    } catch (std::runtime_error &e) {
      TS_FAIL(e.what());
    }

    MatrixWorkspace_const_sptr outputWS = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>("output2");

    // 0   10   20   30    X
    //   0     2     5        Y

    TS_ASSERT_EQUALS(outputWS->x(0).size(), 4);
    TS_ASSERT_EQUALS(outputWS->y(0).size(), 3);
    TS_ASSERT_EQUALS(outputWS->x(0)[0], 0);
    TS_ASSERT_EQUALS(outputWS->y(0)[0], 0);
    TS_ASSERT_EQUALS(outputWS->x(0)[3], 30);
    TS_ASSERT_EQUALS(outputWS->y(0)[2], 5);
  }

  void testRemoveFromMiddle() {
    alg4.initialize();
    TS_ASSERT(alg4.isInitialized())
    alg4.setPropertyValue("InputWorkspace", "input2D");
    alg4.setPropertyValue("OutputWorkspace", "output3");
    alg4.setPropertyValue("XMin", "11");
    alg4.setPropertyValue("XMax", "21");
    alg4.setPropertyValue("Interpolation", "Linear");

    TS_ASSERT_EQUALS(alg4.getPropertyValue("XMin"), "11");
    TS_ASSERT_EQUALS(alg4.getPropertyValue("XMax"), "21");
    TS_ASSERT_EQUALS(alg4.getPropertyValue("Interpolation"), "Linear");

    try {
      TS_ASSERT_EQUALS(alg4.execute(), true);
    } catch (std::runtime_error &e) {
      TS_FAIL(e.what());
    }

    MatrixWorkspace_const_sptr outputWS = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>("output3");

    // 0   10   20   30   40   X
    //   0     2     4     6       Y

    TS_ASSERT_EQUALS(outputWS->x(0).size(), 5);
    TS_ASSERT_EQUALS(outputWS->y(0).size(), 4);
    TS_ASSERT_EQUALS(outputWS->x(0)[0], 0);
    TS_ASSERT_EQUALS(outputWS->x(0)[3], 30);
    TS_ASSERT_EQUALS(outputWS->y(0)[0], 0);
    TS_ASSERT_EQUALS(outputWS->y(0)[1], 1.5);
    TS_ASSERT_EQUALS(outputWS->y(0)[2], 3);
    TS_ASSERT_EQUALS(outputWS->y(0)[3], 6);
  }

  void testSingleSpectrum() {
    RemoveBins rb;
    TS_ASSERT_THROWS_NOTHING(rb.initialize())
    TS_ASSERT(rb.isInitialized())
    rb.setPropertyValue("InputWorkspace", "input2D");
    rb.setPropertyValue("OutputWorkspace", "output4");
    rb.setPropertyValue("XMin", "0");
    rb.setPropertyValue("XMax", "40");
    rb.setPropertyValue("WorkspaceIndex", "0");

    TS_ASSERT(rb.execute())

    MatrixWorkspace_const_sptr inputWS = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>("input2D");
    MatrixWorkspace_const_sptr outputWS = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>("output4");
    TS_ASSERT_EQUALS(inputWS->x(0).rawData(), outputWS->x(0).rawData())
    TS_ASSERT_EQUALS(inputWS->x(1).rawData(), outputWS->x(1).rawData())
    TS_ASSERT_EQUALS(inputWS->y(1).rawData(), outputWS->y(1).rawData())
    TS_ASSERT_EQUALS(inputWS->e(1).rawData(), outputWS->e(1).rawData())
    for (int i = 0; i < 4; ++i) {
      TS_ASSERT_EQUALS(outputWS->y(0)[i], 0.0)
      TS_ASSERT_EQUALS(outputWS->e(0)[i], 0.0)
    }

    AnalysisDataService::Instance().remove("output4");
  }

  void testSingleSpectrumNotWS0() {
    RemoveBins rb;
    Workspace2D_sptr inputWS = makeDummyWorkspace2D();
    std::string outputWSName = "output44";
    TS_ASSERT_THROWS_NOTHING(rb.initialize())
    TS_ASSERT(rb.isInitialized())
    rb.setPropertyValue("InputWorkspace", inputWS->getName());
    rb.setPropertyValue("OutputWorkspace", outputWSName);
    rb.setPropertyValue("XMin", "0");
    rb.setPropertyValue("XMax", "40");
    rb.setPropertyValue("WorkspaceIndex", "1");

    TS_ASSERT(rb.execute())

    MatrixWorkspace_const_sptr outputWS = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(outputWSName);
    TS_ASSERT_EQUALS(inputWS->x(1).rawData(), outputWS->x(1).rawData())
    TS_ASSERT_EQUALS(inputWS->x(0).rawData(), outputWS->x(0).rawData())
    TS_ASSERT_EQUALS(inputWS->y(0).rawData(), outputWS->y(0).rawData())
    TS_ASSERT_EQUALS(inputWS->e(0).rawData(), outputWS->e(0).rawData())
    for (int i = 0; i < 4; ++i) {
      TS_ASSERT_EQUALS(outputWS->y(1)[i], 0.0)
      TS_ASSERT_EQUALS(outputWS->e(1)[i], 0.0)
    }

    AnalysisDataService::Instance().remove(outputWSName);
  }

  void testRangeUnit() {
    MatrixWorkspace_sptr inputWS = WorkspaceCreationHelper::create2DWorkspaceWithFullInstrument(1, 10);
    // scale x values up to 0-1000 range
    inputWS->getSpectrum(0).mutableX() *= 100;
    std::string wsName = "RemoveBins_RangeUnit";
    std::string wsNameOutput = wsName + "_Output";
    AnalysisDataService::Instance().addOrReplace(wsName, inputWS);

    RemoveBins algRU;

    algRU.initialize();
    TS_ASSERT_THROWS_NOTHING(algRU.setPropertyValue("InputWorkspace", wsName);)
    algRU.setPropertyValue("OutputWorkspace", wsNameOutput);
    algRU.setPropertyValue("XMin", "0.05");
    algRU.setPropertyValue("XMax", "0.1");
    algRU.setPropertyValue("RangeUnit", "Wavelength");
    try {
      TS_ASSERT_EQUALS(algRU.execute(), true);
    } catch (std::runtime_error &e) {
      TS_FAIL(e.what());
    }
    if (algRU.isExecuted() == false)
      return;

    MatrixWorkspace_const_sptr outputWS = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(wsNameOutput);

    std::vector<double> expected = {2, 2, 2, 1.68054, 0, 0, 0.638921, 2, 2, 2};
    for (size_t i = 0; i < outputWS->y(0).size(); i++) {
      TS_ASSERT_DELTA(outputWS->y(0)[i], expected[i], 0.0001);
    }
  }

  Workspace2D_sptr makeDummyWorkspace2D() {
    Workspace2D_sptr testWorkspace(new Workspace2D);

    testWorkspace->setTitle("input2D");
    testWorkspace->initialize(2, 5, 4);

    BinEdges X(5, LinearGenerator(0, 10));
    std::vector<double> Y{0, 2, 5, 6};
    std::vector<double> E{0, 2, 5, 6};

    testWorkspace->setBinEdges(0, X);
    testWorkspace->setBinEdges(1, X);
    testWorkspace->mutableY(0) = Y;
    testWorkspace->mutableE(0) = E;
    testWorkspace->mutableY(1) = std::move(Y);
    testWorkspace->mutableE(1) = std::move(E);

    AnalysisDataService::Instance().addOrReplace("input2D", testWorkspace);

    return testWorkspace;
  }

private:
  RemoveBins alg;
  RemoveBins alg2;
  RemoveBins alg3;
  RemoveBins alg4;
};

class RemoveBinsTestPerformance : public CxxTest::TestSuite {
public:
  static RemoveBinsTestPerformance *createSuite() { return new RemoveBinsTestPerformance(); }

  static void destroySuite(RemoveBinsTestPerformance *suite) { delete suite; }

  RemoveBinsTestPerformance() {
    auto wksp = std::make_shared<Workspace2D>();
    wksp->setTitle("input");
    wksp->initialize(numHists, 10000, 9999);
    BinEdges edges(10000, LinearGenerator(0, 10));

    for (size_t i = 0; i < numHists; i++)
      wksp->setBinEdges(i, edges);

    wksp->getAxis(0)->unit() = Mantid::Kernel::UnitFactory::Instance().create("TOF");

    AnalysisDataService::Instance().addOrReplace("input", wksp);
  }

  ~RemoveBinsTestPerformance() {
    AnalysisDataService::Instance().remove("input");
    AnalysisDataService::Instance().remove("outputBack");
    AnalysisDataService::Instance().remove("outputMiddle");
  }

  void testRemoveFromBack() {
    alg.initialize();
    alg.setPropertyValue("InputWorkspace", "input");
    alg.setPropertyValue("OutputWorkspace", "outputBack");
    alg.setPropertyValue("XMin", "80000");
    alg.setPropertyValue("XMax", "100000");
    alg.setPropertyValue("Interpolation", "Linear");
    alg.execute();
  }

  void testRemoveFromMiddle() {
    alg.initialize();
    alg.setPropertyValue("InputWorkspace", "input");
    alg.setPropertyValue("OutputWorkspace", "outputMiddle");
    alg.setPropertyValue("XMin", "32000");
    alg.setPropertyValue("XMax", "53000");
    alg.setPropertyValue("Interpolation", "Linear");
    alg.execute();
  }

private:
  const size_t numHists = 10000;
  RemoveBins alg;
};
