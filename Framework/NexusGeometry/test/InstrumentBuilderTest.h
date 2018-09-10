#ifndef INSTRUMENT_GEOMETRY_ABSTRACTION_TEST_H_
#define INSTRUMENT_GEOMETRY_ABSTRACTION_TEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidBeamline/ComponentInfo.h"
#include "MantidBeamline/DetectorInfo.h"
#include "MantidGeometry/Instrument.h"
#include "MantidGeometry/Instrument/InstrumentVisitor.h"
#include "MantidGeometry/Objects/CSGObject.h"
#include "MantidNexusGeometry/InstrumentBuilder.h"

#include "Eigen/Core"

#include <string>

using namespace Mantid;
using namespace NexusGeometry;

class InstrumentBuilderTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static InstrumentBuilderTest *createSuite() {
    return new InstrumentBuilderTest();
  }
  static void destroySuite(InstrumentBuilderTest *suite) { delete suite; }

  void testAddComponent() {
    InstrumentBuilder builder(this->iTestName);

    TS_ASSERT_THROWS_NOTHING(
        builder.addComponent(this->cTestName, this->testPos1));
    builder.addSample("sample", {0, 0, 0});
    builder.addSource("source", {-10, 0, 0});
    auto iVisitor = Geometry::InstrumentVisitor(builder.createInstrument());
    iVisitor.walkInstrument();
    auto iCompInfo = iVisitor.componentInfo();
    TS_ASSERT_EQUALS(iCompInfo->position(0), this->testPos1);
  }

  void testAddDetector_and_testSortDetectors() {
    InstrumentBuilder builder(this->iTestName);
    builder.addSample("sample", {0, 0, 0});
    builder.addSource("source", {-10, 0, 0});
    builder.addDetectorToInstrument(this->dTestName, 1, this->testPos2,
                                    this->shape);
    builder.addDetectorToInstrument(this->dTestName, 2, this->testPos1,
                                    this->shape);
    auto iVisitor = Geometry::InstrumentVisitor(builder.createInstrument());
    iVisitor.walkInstrument();
    auto iDetInfo = iVisitor.detectorInfo();
    TS_ASSERT_EQUALS(iDetInfo->position(0), this->testPos2);
    TS_ASSERT_EQUALS(iDetInfo->position(1), this->testPos1);
  }

  void testAddSample_and_testAddSource() {
    InstrumentBuilder builder(this->iTestName);
    TS_ASSERT_THROWS_NOTHING(
        builder.addSample(this->sampleName, this->testPos1));
    TS_ASSERT_THROWS_NOTHING(
        builder.addSource(this->sourceName, this->testPos2));
    auto iVisitor = Geometry::InstrumentVisitor(builder.createInstrument());
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
  boost::shared_ptr<const Geometry::IObject> shape =
      boost::make_shared<const Geometry::CSGObject>();
};

#endif // INSTRUMENT_GEOMETRY_ABSTRACTION_TEST_H_
