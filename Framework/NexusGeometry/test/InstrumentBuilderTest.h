// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <cxxtest/TestSuite.h>

#include "MantidBeamline/ComponentInfo.h"
#include "MantidBeamline/DetectorInfo.h"
#include "MantidFrameworkTestHelpers/NexusGeometryTestHelpers.h"
#include "MantidGeometry/Instrument.h"
#include "MantidGeometry/Instrument/ComponentInfo.h"
#include "MantidGeometry/Instrument/DetectorInfo.h"
#include "MantidGeometry/Instrument/InstrumentVisitor.h"
#include "MantidGeometry/Instrument/ReferenceFrame.h"
#include "MantidGeometry/Objects/CSGObject.h"
#include "MantidNexusGeometry/InstrumentBuilder.h"
#include "MantidNexusGeometry/TubeBuilder.h"

#include "Eigen/Core"
#include "Eigen/Geometry"

#include <string>

using namespace Mantid;
using namespace NexusGeometry;

class InstrumentBuilderTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static InstrumentBuilderTest *createSuite() { return new InstrumentBuilderTest(); }
  static void destroySuite(InstrumentBuilderTest *suite) { delete suite; }

  void testAddComponent() {
    InstrumentBuilder builder(this->iTestName);

    TS_ASSERT_THROWS_NOTHING(builder.addComponent(this->cTestName, this->testPos1));
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
    builder.addDetectorToInstrument(this->dTestName, 1, this->testPos2, this->shape);
    builder.addDetectorToInstrument(this->dTestName, 2, this->testPos1, this->shape);
    auto iVisitor = Geometry::InstrumentVisitor(builder.createInstrument());
    iVisitor.walkInstrument();
    auto iDetInfo = iVisitor.detectorInfo();
    TS_ASSERT_EQUALS(iDetInfo->position(0), this->testPos2);
    TS_ASSERT_EQUALS(iDetInfo->position(1), this->testPos1);
  }

  void testAddSample_and_testAddSource() {
    InstrumentBuilder builder(this->iTestName);
    TS_ASSERT_THROWS_NOTHING(builder.addSample(this->sampleName, this->testPos1));
    TS_ASSERT_THROWS_NOTHING(builder.addSource(this->sourceName, this->testPos2));
    auto iVisitor = Geometry::InstrumentVisitor(builder.createInstrument());
    iVisitor.walkInstrument();
    auto iCompInfo = iVisitor.componentInfo();
    TS_ASSERT(iCompInfo->hasSample());
    TS_ASSERT(iCompInfo->samplePosition() == this->testPos1);
    TS_ASSERT(iCompInfo->hasSource());
    TS_ASSERT(iCompInfo->sourcePosition() == this->testPos2);
  }

  void test_banks_tubes_and_monitors() {
    auto const instrument = makeInstrument();
    auto const [componentInfo, detectorInfo] = instrument->makeBaseBeamline();
    TS_ASSERT_EQUALS(detectorInfo->detectorIDs(), (std::vector<detid_t>{1, 3, 10, 11}));
    TS_ASSERT(detectorInfo->isMonitor(0));
    TS_ASSERT(!detectorInfo->isMonitor(1));
    TS_ASSERT_DELTA(detectorInfo->position(0).distance(Kernel::V3D(0, 0, -1)), 0, 1e-12);
    // The bank turns a pixel's offset along x to -z
    TS_ASSERT_DELTA(detectorInfo->position(1).distance(Kernel::V3D(0, 0, 0.9)), 0, 1e-12);
    TS_ASSERT_DELTA(detectorInfo->position(2).distance(Kernel::V3D(0, 1, 1 - 0.00202)), 0, 1e-12);
    TS_ASSERT_DELTA(detectorInfo->position(3).distance(Kernel::V3D(0, 1, 1 - 0.00404)), 0, 1e-12);

    auto const tube = componentInfo->indexOfAny("bank_tube_0");
    TS_ASSERT_EQUALS(componentInfo->componentType(tube), Beamline::ComponentType::OutlineComposite);
    TS_ASSERT_EQUALS(componentInfo->detectorsInSubtree(tube), (std::vector<size_t>{2, 3}));
    TS_ASSERT_EQUALS(componentInfo->name(2), "bank_tube_0_0");
    auto const bank = componentInfo->indexOfAny("bank");
    TS_ASSERT_EQUALS(componentInfo->parent(tube), bank);
    TS_ASSERT_EQUALS(componentInfo->detectorsInSubtree(bank), (std::vector<size_t>{1, 2, 3}));
    TS_ASSERT_EQUALS(componentInfo->name(componentInfo->source()), "source");
    TS_ASSERT_EQUALS(componentInfo->name(componentInfo->sample()), "sample");
  }

  void test_the_tree_matches_its_base_beamline() {
    auto const instrument = makeInstrument();
    auto const [componentInfo, detectorInfo] = instrument->makeBaseBeamline();
    for (auto const &name : {"testInstrument", "bank", "bank_tube_0", "bank_tube_0_1", "monitor", "source"}) {
      TS_ASSERT_EQUALS(componentInfo->name(instrument->getComponentByName(name)->index()), name);
    }
    Geometry::InstrumentVisitor visitor(instrument);
    visitor.walkInstrument();
    auto const [walked, walkedDetectors] = visitor.makeWrappers();
    TS_ASSERT(!walked->hasSameBase(*componentInfo));
    TS_ASSERT_EQUALS(walked->size(), componentInfo->size());
    for (size_t i = 0; i < walked->size(); ++i) {
      TS_ASSERT_EQUALS(walked->name(i), componentInfo->name(i));
      TS_ASSERT_EQUALS(walked->position(i), componentInfo->position(i));
      TS_ASSERT_EQUALS(walked->rotation(i), componentInfo->rotation(i));
    }
  }

  void test_the_reference_frame_and_default_view() {
    InstrumentBuilder builder(this->iTestName);
    auto const instrument = builder.createInstrument();
    auto const [componentInfo, detectorInfo] = instrument->makeBaseBeamline();
    auto const &frame = componentInfo->referenceFrame();
    TS_ASSERT_EQUALS(frame->pointingUp(), Geometry::Y);
    TS_ASSERT_EQUALS(frame->pointingAlongBeam(), Geometry::Z);
    TS_ASSERT_EQUALS(frame->pointingHorizontal(), Geometry::X);
    TS_ASSERT_EQUALS(frame->getHandedness(), Geometry::Right);
    TS_ASSERT_EQUALS(frame->origin(), "");
    TS_ASSERT_EQUALS(instrument->getDefaultAxis(), "z");
    TS_ASSERT_EQUALS(instrument->getDefaultView(), "3D");
  }

  void test_the_first_source_and_sample_are_kept() {
    InstrumentBuilder builder(this->iTestName);
    builder.addSource("source", {0, 0, -10});
    builder.addSource("second source", {0, 0, -20});
    builder.addSample("sample", {0, 0, 0});
    builder.addSample("second sample", {0, 0, 1});
    auto const instrument = builder.createInstrument();
    auto const [componentInfo, detectorInfo] = instrument->makeBaseBeamline();
    TS_ASSERT_EQUALS(componentInfo->name(componentInfo->source()), "source");
    TS_ASSERT_EQUALS(componentInfo->name(componentInfo->sample()), "sample");
    TS_ASSERT_EQUALS(instrument->getSource()->getName(), "source");
    TS_ASSERT_EQUALS(instrument->getSample()->getName(), "sample");
    TS_ASSERT_THROWS_NOTHING(componentInfo->indexOfAny("second source"));
    TS_ASSERT_THROWS_NOTHING(componentInfo->indexOfAny("second sample"));
  }

  void test_bank_contents_need_a_bank() {
    InstrumentBuilder builder(this->iTestName);
    TS_ASSERT_THROWS(builder.addDetectorToLastBank(this->dTestName, 1, this->testPos1, this->shape),
                     std::runtime_error const &);
    auto const pixelShape = NexusGeometryTestHelpers::createShape();
    std::vector<detail::TubeBuilder> const tubes{detail::TubeBuilder(*pixelShape, Eigen::Vector3d(0, 1, 0), 1)};
    TS_ASSERT_THROWS(builder.addTubes("bank", tubes, pixelShape), std::runtime_error const &);
  }

private:
  /// An instrument with a monitor, and a bank rotated 90 degrees about y holding a pixel and a tube of two pixels
  std::shared_ptr<const Geometry::Instrument> makeInstrument() const {
    InstrumentBuilder builder(this->iTestName);
    builder.addBank("bank", Eigen::Vector3d(0, 0, 1),
                    Eigen::Quaterniond(Eigen::AngleAxisd(M_PI / 2, Eigen::Vector3d::UnitY())));
    builder.addDetectorToLastBank("pixel", 3, Eigen::Vector3d(0.1, 0, 0), this->shape);
    auto const pixelShape = NexusGeometryTestHelpers::createShape();
    detail::TubeBuilder tube(*pixelShape, Eigen::Vector3d(0.00202, 1, 0), 10);
    tube.addDetectorIfCoLinear(Eigen::Vector3d(0.00404, 1, 0), 11);
    builder.addTubes("bank", {tube}, pixelShape);
    // Added last, but first by detector ID
    builder.addMonitor("monitor", 1, Eigen::Vector3d(0, 0, -1), this->shape);
    builder.addSample("sample", {0, 0, 0});
    builder.addSource("source", {0, 0, -10});
    return builder.createInstrument();
  }

  std::string iTestName = "testInstrument";
  std::string cTestName = "testComponent";
  std::string dTestName = "testDetector";
  std::string sourceName = "testSource";
  std::string sampleName = "testSample";
  Eigen::Vector3d testPos1 = Eigen::Vector3d(1.0, -0.5, 2.9);
  Eigen::Vector3d testPos2 = Eigen::Vector3d(-1.2, 0.5, 1.9);
  // Placeholder empty shape
  std::shared_ptr<const Geometry::IObject> shape = std::make_shared<const Geometry::CSGObject>();
};
