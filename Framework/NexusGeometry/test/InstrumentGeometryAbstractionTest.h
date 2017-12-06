#ifndef INSTRUMENT_GEOMETRY_ABSTRACTION_TEST_H_
#define INSTRUMENT_GEOMETRY_ABSTRACTION_TEST_H_

//---------------------------
// Includes
//---------------------------

#include <cxxtest/TestSuite.h>

#include "MantidBeamline/ComponentInfo.h"
#include "MantidBeamline/DetectorInfo.h"
#include "MantidGeometry/Instrument.h"
#include "MantidGeometry/Instrument/InstrumentVisitor.h"
#include "MantidNexusGeometry/InstrumentGeometryAbstraction.h"

#include "Eigen/Core"

#include <string>

using namespace Mantid;
using namespace NexusGeometry;

class InstrumentGeometryAbstractionTest : public CxxTest::TestSuite {
public:
  void testConstructor_and_test_unAbstractInstrument() {
    InstrumentGeometryAbstraction iGeoAbstract(this->iTestName);
    TS_ASSERT(iGeoAbstract._unAbstractInstrument()->getName() ==
              this->iTestName);
  }
  void testAddComponent() {
    InstrumentGeometryAbstraction iGeoAbstract(this->iTestName);
    TS_ASSERT_THROWS_NOTHING(
        auto comp = iGeoAbstract.addComponent(this->cTestName, this->testPos1));
    auto iVisitor =
        Geometry::InstrumentVisitor(iGeoAbstract._unAbstractInstrument());
    iVisitor.walkInstrument();
    auto iCompInfo = iVisitor.componentInfo();
    TS_ASSERT(iCompInfo->position(0) == this->testPos1);
  }

  void testAddDetector_and_testSortDetectors() {
    InstrumentGeometryAbstraction iGeoAbstract(this->iTestName);
    TS_ASSERT_THROWS_NOTHING(iGeoAbstract.addDetector(
        this->dTestName, 2, this->testPos2, this->shape));
    iGeoAbstract.addDetector(this->dTestName, 1, this->testPos1, this->shape);
    for (int i = 0; i < 2; ++i) {
      auto iVisitor =
          Geometry::InstrumentVisitor(iGeoAbstract._unAbstractInstrument());
      iVisitor.walkInstrument();
      auto iDetInfo = iVisitor.detectorInfo();
      if (i == 0) {
        TS_ASSERT(iDetInfo->position(0) == this->testPos2);
      } else {
        TS_ASSERT(iDetInfo->position(0) == this->testPos1);
      }
      TS_ASSERT_THROWS_NOTHING(iGeoAbstract.sortDetectors());
    }
  }

  void testAddSample_and_testAddSource() {
    InstrumentGeometryAbstraction iGeoAbstract(this->iTestName);
    TS_ASSERT_THROWS_NOTHING(
        iGeoAbstract.addSample(this->sampleName, this->testPos1));
    TS_ASSERT_THROWS_NOTHING(
        iGeoAbstract.addSource(this->sourceName, this->testPos2));
    auto iVisitor =
        Geometry::InstrumentVisitor(iGeoAbstract._unAbstractInstrument());
    iVisitor.walkInstrument();
    auto iCompInfo = iVisitor.componentInfo();
    TS_ASSERT(iCompInfo->hasSample());
    TS_ASSERT(iCompInfo->samplePosition() == this->testPos1);
    TS_ASSERT(iCompInfo->hasSource());
    TS_ASSERT(iCompInfo->sourcePosition() == this->testPos2);
  }

private:
  std::string iTestName = "testInstrument";
  std::string cTestName = "testComponent";
  std::string dTestName = "testDetector";
  std::string sourceName = "testSource";
  std::string sampleName = "testSample";
  Eigen::Vector3d testPos1 = Eigen::Vector3d(1.0, -0.5, 2.9);
  Eigen::Vector3d testPos2 = Eigen::Vector3d(-1.2, 0.5, 1.9);
  // Placeholder empty shape
  objectHolder shape = objectHolder();
};

#endif // INSTRUMENT_GEOMETRY_ABSTRACTION_TEST_H_
