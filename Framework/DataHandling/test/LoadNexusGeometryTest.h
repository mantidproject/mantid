#ifndef LOAD_NEXUS_GEOMETRY_TEST_H_
#define LOAD_NEXUS_GEOMETRY_TEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidAPI/InstrumentDataService.h"
#include "MantidBeamline/ComponentInfo.h"
#include "MantidBeamline/DetectorInfo.h"
#include "MantidDataHandling/LoadNexusGeometry.h"
#include "MantidGeometry/Instrument.h"
#include "MantidGeometry/Instrument/InstrumentVisitor.h"

#include "Eigen/Core"

using namespace Mantid;
using namespace DataHandling;

typedef boost::shared_ptr<Beamline::ComponentInfo> ComponentInfo_sptr;

class LoadNexusGeometryTest : public CxxTest::TestSuite {
public:
  void testInit() {
    LoadNexusGeometry loader;
    loader.setChild(true);
    TS_ASSERT(!loader.isInitialized());
    loader.initialize();
    TS_ASSERT(loader.isInitialized());

    TS_ASSERT_THROWS_NOTHING(
        loader.setProperty("InstrumentName", "testInstName"));
    TS_ASSERT(loader.getPropertyValue("InstrumentName") == "testInstName");
  }

  void testExec() {
    LoadNexusGeometry loader;
    loader.initialize();
    loader.setProperty("InstrumentName", "testInstrument");

    TS_ASSERT(!loader.isExecuted());
    TS_ASSERT(loader.execute());
    TS_ASSERT(loader.isExecuted());

    // Get instrument from data service
    Geometry::Instrument_sptr testInst1;
    TS_ASSERT_THROWS_NOTHING(
        testInst1 =
            API::InstrumentDataService::Instance().retrieve("testInstrument"));
  }

  void testMemberFunctions() {
    LoadNexusGeometry loader;
    std::string instName = "testInst1";
    Geometry::Instrument_sptr instrument(new Geometry::Instrument(instName));

    setUpAddDetectorWithParent(loader, instrument);
    setUpAddSource(loader, instrument);
    setUpAddSample(loader, instrument);

    // Convert to instrument_2 interface for testing
    Geometry::InstrumentVisitor inst2 = Geometry::InstrumentVisitor(instrument);
    TS_ASSERT(!inst2.isEmpty());
    inst2.walkInstrument();

    ComponentInfo_sptr compInfo = inst2.componentInfo();

    checkSource(compInfo);
    checkSample(compInfo);

    checkDetectorWithParent(inst2, compInfo);
  }

private:
  void setUpAddDetectorWithParent(LoadNexusGeometry &loader,
                                  Geometry::Instrument_sptr instrument) {
    std::string detName("testDetector");
    Eigen::Vector3d detPos(1.0, 1.0, 0.5);
    int id(5);
    TS_ASSERT_THROWS_NOTHING(
        loader.addDetector(detName, detPos, id, instrument));
  }
  void setUpAddSource(LoadNexusGeometry &loader,
                      Geometry::Instrument_sptr instrument) {
    std::string name = "testSource";
    Eigen::Vector3d pos(0.0, 0.0, -1.0);
    TS_ASSERT_THROWS_NOTHING(loader.addSource(name, pos, instrument));
  }
  void setUpAddSample(LoadNexusGeometry &loader,
                      Geometry::Instrument_sptr instrument) {
    std::string name = "testSample";
    Eigen::Vector3d pos(1.0, 0.0, 2.0);
    TS_ASSERT_THROWS_NOTHING(loader.addSample(name, pos, instrument));
  }

  void checkDetectorWithParent(Geometry::InstrumentVisitor &inst,
                               ComponentInfo_sptr compInfo) {
    // Get index of sole detector
    auto detIndexMap = inst.detectorIdToIndexMap();
    TS_ASSERT(!detIndexMap->empty());
    TS_ASSERT(detIndexMap->begin()->first == 5);
    auto detIndex(detIndexMap->begin()->second);

    TS_ASSERT(compInfo->isDetector(detIndex));
    TS_ASSERT(inst.detectorInfo()->position(detIndex) ==
              Eigen::Vector3d(1.0, 1.0, 0.5));
  }
  void checkSource(ComponentInfo_sptr compInfo) {
    TS_ASSERT(compInfo->hasSource());
    TS_ASSERT(compInfo->sourcePosition() == Eigen::Vector3d(0.0, 0.0, -1.0));
  }
  void checkSample(ComponentInfo_sptr compInfo) {
    TS_ASSERT(compInfo->hasSample());
    TS_ASSERT(compInfo->samplePosition() == Eigen::Vector3d(1.0, 0.0, 2.0));
  }
};

#endif // LOAD_NEXUS_GEOMETRY_TEST_H_
