#ifndef RemoveBinsTest_H_
#define RemoveBinsTest_H_

#include <cxxtest/TestSuite.h>

#include <sstream>
#include <string>
#include <stdexcept>

#include "MantidAlgorithms/RemoveBins.h"
#include "MantidAPI/Axis.h"
#include "MantidDataHandling/LoadMuonNexus2.h"
#include "MantidDataHandling/LoadInstrument.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidKernel/UnitFactory.h"

using namespace Mantid::Algorithms;
using namespace Mantid::API;
using namespace Mantid::DataObjects;
using Mantid::HistogramData::BinEdges;

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

    MatrixWorkspace_const_sptr outputWS =
        AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>("output");

    // Should give:
    // 10   20   30   40   X
    //     2     5     6       Y

    TS_ASSERT_EQUALS(outputWS->dataX(0).size(), 4);
    TS_ASSERT_EQUALS(outputWS->dataY(0).size(), 3);
    TS_ASSERT_EQUALS(outputWS->dataX(0)[0], 10);
    TS_ASSERT_EQUALS(outputWS->dataY(0)[0], 2);
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

    MatrixWorkspace_const_sptr outputWS =
        AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>("output2");

    // 0   10   20   30    X
    //   0     2     5        Y

    TS_ASSERT_EQUALS(outputWS->dataX(0).size(), 4);
    TS_ASSERT_EQUALS(outputWS->dataY(0).size(), 3);
    TS_ASSERT_EQUALS(outputWS->dataX(0)[0], 0);
    TS_ASSERT_EQUALS(outputWS->dataY(0)[0], 0);
    TS_ASSERT_EQUALS(outputWS->dataX(0)[3], 30);
    TS_ASSERT_EQUALS(outputWS->dataY(0)[2], 5);
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

    MatrixWorkspace_const_sptr outputWS =
        AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>("output3");

    // 0   10   20   30   40   X
    //   0     2     4     6       Y

    TS_ASSERT_EQUALS(outputWS->dataX(0).size(), 5);
    TS_ASSERT_EQUALS(outputWS->dataY(0).size(), 4);
    TS_ASSERT_EQUALS(outputWS->dataX(0)[0], 0);
    TS_ASSERT_EQUALS(outputWS->dataX(0)[3], 30);
    TS_ASSERT_EQUALS(outputWS->dataY(0)[0], 0);
    TS_ASSERT_EQUALS(outputWS->dataY(0)[1], 1.5);
    TS_ASSERT_EQUALS(outputWS->dataY(0)[2], 3);
    TS_ASSERT_EQUALS(outputWS->dataY(0)[3], 6);
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

    MatrixWorkspace_const_sptr inputWS =
        AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>("input2D");
    MatrixWorkspace_const_sptr outputWS =
        AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>("output4");
    TS_ASSERT_EQUALS(inputWS->readX(0), outputWS->readX(0))
    TS_ASSERT_EQUALS(inputWS->readX(1), outputWS->readX(1))
    TS_ASSERT_EQUALS(inputWS->readY(1), outputWS->readY(1))
    TS_ASSERT_EQUALS(inputWS->readE(1), outputWS->readE(1))
    for (int i = 0; i < 4; ++i) {
      TS_ASSERT_EQUALS(outputWS->readY(0)[i], 0.0)
      TS_ASSERT_EQUALS(outputWS->readE(0)[i], 0.0)
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

    MatrixWorkspace_const_sptr outputWS =
        AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(
            outputWSName);
    TS_ASSERT_EQUALS(inputWS->readX(1), outputWS->readX(1))
    TS_ASSERT_EQUALS(inputWS->readX(0), outputWS->readX(0))
    TS_ASSERT_EQUALS(inputWS->readY(0), outputWS->readY(0))
    TS_ASSERT_EQUALS(inputWS->readE(0), outputWS->readE(0))
    for (int i = 0; i < 4; ++i) {
      TS_ASSERT_EQUALS(outputWS->readY(1)[i], 0.0)
      TS_ASSERT_EQUALS(outputWS->readE(1)[i], 0.0)
    }

    AnalysisDataService::Instance().remove(outputWSName);
  }

  void xtestRealData() {
    Mantid::DataHandling::LoadMuonNexus2 loader;
    loader.initialize();
    loader.setPropertyValue("Filename", "emu00006473.nxs");
    loader.setPropertyValue("OutputWorkspace", "EMU6473");
    loader.execute();
    TS_ASSERT(loader.isExecuted())

    // Test removing time bins from the front
    alg2.initialize();
    TS_ASSERT(alg2.isInitialized())

    alg2.setPropertyValue("InputWorkspace", "EMU6473");
    alg2.setPropertyValue("OutputWorkspace", "result1");
    alg2.setPropertyValue("XMin", "-0.255");
    alg2.setPropertyValue("XMax", "-0.158");

    try {
      TS_ASSERT_EQUALS(alg2.execute(), true);
    } catch (std::runtime_error &e) {
      TS_FAIL(e.what());
    }

    MatrixWorkspace_const_sptr outputWS =
        AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>("result1");

    TS_ASSERT_EQUALS(outputWS->dataX(0).size(), 1994);
  }

  Workspace2D_sptr makeDummyWorkspace2D() {
    Workspace2D_sptr testWorkspace(new Workspace2D);

    testWorkspace->setTitle("input2D");
    testWorkspace->initialize(2, 5, 4);

    BinEdges X{0, 10, 20, 30, 40};
    auto Y = boost::make_shared<Mantid::MantidVec>();

    for (int i = 0; i < 4; ++i) {
      if (i == 2) {
        Y->push_back(2.0 * i + 1);
      } else {
        Y->push_back(2.0 * i);
      }
    }

    //   0     2     5     6       Y

    testWorkspace->setBinEdges(0, X);
    testWorkspace->setBinEdges(1, X);
    testWorkspace->setData(0, Y, Y);
    testWorkspace->setData(1, Y, Y);

    testWorkspace->getAxis(0)->unit() =
        Mantid::Kernel::UnitFactory::Instance().create("TOF");

    AnalysisDataService::Instance().addOrReplace("input2D", testWorkspace);

    return testWorkspace;
  }

private:
  RemoveBins alg;
  RemoveBins alg2;
  RemoveBins alg3;
  RemoveBins alg4;
};

#endif /*RemoveBinsTest_H_*/
