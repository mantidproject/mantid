#ifndef MANTID_DATAHANDLING_LOADILLSANSTEST_H_
#define MANTID_DATAHANDLING_LOADILLSANSTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidDataHandling/LoadILLSANS.h"
#include "MantidGeometry/IComponent.h"
#include "MantidGeometry/Instrument.h"
#include "MantidGeometry/Instrument/DetectorInfo.h"
#include "MantidKernel/ConfigService.h"

using Mantid::DataHandling::LoadILLSANS;
using Mantid::API::MatrixWorkspace_const_sptr;
using Mantid::API::AnalysisDataService;
using Mantid::Kernel::ConfigService;
using Mantid::Kernel::V3D;
using Mantid::Geometry::IComponent_const_sptr;
using Mantid::Geometry::Instrument;

class LoadILLSANSTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static LoadILLSANSTest *createSuite() { return new LoadILLSANSTest(); }
  static void destroySuite(LoadILLSANSTest *suite) { delete suite; }

  void setUp() override {
    ConfigService::Instance().appendDataSearchSubDir("ILL/D11/");
    ConfigService::Instance().appendDataSearchSubDir("ILL/D22/");
    ConfigService::Instance().appendDataSearchSubDir("ILL/D33/");
    ConfigService::Instance().setFacility("ILL");
  }

  void tearDown() override {
      AnalysisDataService::Instance().clear();
  }

  void testName() {
    LoadILLSANS alg;
    TS_ASSERT_EQUALS(alg.name(), "LoadILLSANS");
  }

  void testVersion() {
    LoadILLSANS alg;
    TS_ASSERT_EQUALS(alg.version(), 1);
  }

  void test_init() {
    LoadILLSANS alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
  }

void test_D11() {
    LoadILLSANS alg;
    alg.setChild(true);
    alg.initialize();
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("Filename", "010560.nxs"))
    TS_ASSERT_THROWS_NOTHING(
        alg.setPropertyValue("OutputWorkspace", "__unused_for_child"))
    TS_ASSERT_THROWS_NOTHING(alg.execute())
    TS_ASSERT(alg.isExecuted())
    MatrixWorkspace_const_sptr outputWS = alg.getProperty("OutputWorkspace");
    TS_ASSERT(outputWS)
    TS_ASSERT_EQUALS(outputWS->getNumberHistograms(),128*128+2)
    TS_ASSERT_EQUALS(outputWS->blocksize(), 1)
    TS_ASSERT(outputWS->detectorInfo().isMonitor(128*128))
    TS_ASSERT(outputWS->detectorInfo().isMonitor(128*128+1))
    TS_ASSERT(outputWS->isHistogramData())
    TS_ASSERT(!outputWS->isDistribution())
    const auto &instrument = outputWS->getInstrument();
    IComponent_const_sptr component = instrument->getComponentByName("detector");
    V3D pos = component->getPos();
    TS_ASSERT_DELTA(pos.Z(), 20.007, 1E-3)
    const auto& xAxis = outputWS->x(0).rawData();
    const auto& spec6 = outputWS->y(6).rawData();
    TS_ASSERT_EQUALS(xAxis.size(), 2)
    TS_ASSERT_DELTA(xAxis[0], 5.72778, 1E-5)
    TS_ASSERT_DELTA(xAxis[1], 6.26757, 1E-5)
    TS_ASSERT_EQUALS(spec6[0], 20)
}

void test_D22() {
    LoadILLSANS alg;
    alg.setChild(true);
    alg.initialize();
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("Filename", "192068.nxs"))
    TS_ASSERT_THROWS_NOTHING(
        alg.setPropertyValue("OutputWorkspace", "__unused_for_child"))
    TS_ASSERT_THROWS_NOTHING(alg.execute())
    TS_ASSERT(alg.isExecuted())
    MatrixWorkspace_const_sptr outputWS = alg.getProperty("OutputWorkspace");
    TS_ASSERT(outputWS)
    TS_ASSERT_EQUALS(outputWS->getNumberHistograms(),128*256+2)
    TS_ASSERT_EQUALS(outputWS->blocksize(), 1)
    TS_ASSERT(outputWS->detectorInfo().isMonitor(128*256))
    TS_ASSERT(outputWS->detectorInfo().isMonitor(128*256+1))
    TS_ASSERT(outputWS->isHistogramData())
    TS_ASSERT(!outputWS->isDistribution())
    const auto &instrument = outputWS->getInstrument();
    IComponent_const_sptr component = instrument->getComponentByName("detector");
    V3D pos = component->getPos();
    TS_ASSERT_DELTA(pos.Z(), 8, 0.01)
    TS_ASSERT_DELTA(pos.X(), 0.35, 0.01)
    const auto& xAxis = outputWS->x(0).rawData();
    const auto& spec6 = outputWS->y(6).rawData();
    TS_ASSERT_EQUALS(xAxis.size(), 2)
    TS_ASSERT_DELTA(xAxis[0], 4.75015, 1E-5)
    TS_ASSERT_DELTA(xAxis[1], 5.25016, 1E-5)
    TS_ASSERT_EQUALS(spec6[0], 45)
}

};

class LoadILLSANSTestPerformance : public CxxTest::TestSuite {
public:
  void setUp() override {
    for (int i = 0; i < numberOfIterations; ++i) {
      loadAlgPtrs.emplace_back(setupAlg());
    }
  }

  void testLoadILLSANSPerformance() {
    for (auto alg : loadAlgPtrs) {
      TS_ASSERT_THROWS_NOTHING(alg->execute());
    }
  }

  void tearDown() override {
    for (int i = 0; i < numberOfIterations; i++) {
      delete loadAlgPtrs[i];
      loadAlgPtrs[i] = nullptr;
    }
    Mantid::API::AnalysisDataService::Instance().remove(outWSName);
  }

private:
  std::vector<LoadILLSANS *> loadAlgPtrs;

  const int numberOfIterations = 2;

  const std::string inFileName = "ILLD33_041714_NonTof.nxs";
  const std::string outWSName = "LoadILLSANSWsOut";

  LoadILLSANS *setupAlg() {
    LoadILLSANS *loader = new LoadILLSANS;
    loader->initialize();
    loader->isInitialized();
    loader->setPropertyValue("Filename", inFileName);
    loader->setPropertyValue("OutputWorkspace", outWSName);

    loader->setRethrows(true);
    return loader;
  }
};

#endif /* MANTID_DATAHANDLING_LOADILLSANSTEST_H_ */
