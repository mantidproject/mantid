// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/ExperimentInfo.h"
#include "MantidAPI/Sample.h"
#include "MantidAlgorithms/BeamProfileFactory.h"
#include "MantidFrameworkTestHelpers/ComponentCreationHelper.h"
#include "MantidGeometry/Instrument.h"
#include "MantidGeometry/Objects/CSGObject.h"
#include "MantidKernel/V3D.h"
#include <cxxtest/TestSuite.h>
#include <numeric>

using namespace Mantid::Geometry;
using ComponentCreationHelper::createCuboid;
using Mantid::Algorithms::BeamProfileFactory;
using Mantid::API::ExperimentInfo;
using Mantid::API::ExperimentInfo_sptr;
using Mantid::Geometry::Component;
using Mantid::Geometry::Instrument;
using Mantid::Geometry::Instrument_sptr;
using Mantid::Kernel::V3D;

class IBeamProfileTest : public CxxTest::TestSuite {
private:
  void checkIntersectionVolume(const std::shared_ptr<IObject> &intersectionVolume, double expectedHeight,
                               double expectedWidth, double expectedDepth) {
    TS_ASSERT(intersectionVolume);
    double width, height, depth;
    height = intersectionVolume->getBoundingBox().xMax() - intersectionVolume->getBoundingBox().xMin();
    width = intersectionVolume->getBoundingBox().yMax() - intersectionVolume->getBoundingBox().yMin();
    depth = intersectionVolume->getBoundingBox().zMax() - intersectionVolume->getBoundingBox().zMin();

    TS_ASSERT_EQUALS(width, expectedWidth);
    TS_ASSERT_EQUALS(height, expectedHeight);
    TS_ASSERT_EQUALS(depth, expectedDepth);
  }

  ExperimentInfo_sptr createInstrument(const V3D &sourcePos) {
    ExperimentInfo_sptr experiment(new ExperimentInfo);
    Instrument_sptr instrument = std::make_shared<Instrument>();
    ObjComponent *source = new ObjComponent("source");
    Component *sample = new Component("sample");
    source->setPos(sourcePos);
    sample->setPos(V3D(0., 0., 0.));

    instrument->add(source);
    instrument->add(sample);
    instrument->markAsSamplePos(sample);
    instrument->markAsSource(source);
    experiment->setInstrument(instrument);
    return experiment;
  }

  ExperimentInfo_sptr createExperimentWithSlitBeam(const V3D &sourcePos, const double beamWidth,
                                                   const double beamHeight) {
    auto experiment = createInstrument(sourcePos);
    auto sourceID = experiment->getInstrument()->getSource()->getComponentID();
    auto &pmap = experiment->instrumentParameters();
    pmap.addDouble(sourceID, "beam-width", beamWidth);
    pmap.addDouble(sourceID, "beam-height", beamHeight);
    pmap.addString(sourceID, "beam-shape", "Slit");
    return experiment;
  }

  ExperimentInfo_sptr createExperimentWithCircleBeam(const V3D &sourcePos, const double radius) {
    auto experiment = createInstrument(sourcePos);
    auto sourceID = experiment->getInstrument()->getSource()->getComponentID();
    auto &pmap = experiment->instrumentParameters();
    pmap.addDouble(sourceID, "beam-radius", radius);
    pmap.addString(sourceID, "beam-shape", "Circle");
    return experiment;
  }

public:
  static IBeamProfileTest *createSuite() { return new IBeamProfileTest(); }
  static void destroySuite(IBeamProfileTest *suite) { delete suite; }

  void test_beamMissesSample() {
    const auto sample = createCuboid(1., 1., 5., V3D(0., 0., 0.), "sample");
    auto experiment = createExperimentWithSlitBeam(V3D(0., 10., -10.), 10., 10.);

    const auto beamProfile = BeamProfileFactory::createBeamProfile(*experiment->getInstrument(), experiment->sample());
    const auto intersectionVolume = beamProfile->getIntersectionWithSample(*sample);

    TS_ASSERT_EQUALS(intersectionVolume, nullptr);
  }

  void test_sampleEntirelyWithinBeam() {
    const auto sample = createCuboid(1., 1., 5., V3D(0., 0., 0.), "sample");
    auto experiment = createExperimentWithSlitBeam(V3D(0., 0., -10.), 10., 10.);

    const auto beamProfile = BeamProfileFactory::createBeamProfile(*experiment->getInstrument(), experiment->sample());
    const auto intersectionVolume = beamProfile->getIntersectionWithSample(*sample);

    checkIntersectionVolume(intersectionVolume, 2., 2., 10.);
  }

  void test_beamHalfHeightOfSample() {
    const auto sample = createCuboid(1., 5., 6., V3D(0., 0., 0.), "sample");
    auto experiment = createExperimentWithSlitBeam(V3D(0., 0., -5.), 10., 5.);

    const auto beamProfile = BeamProfileFactory::createBeamProfile(*experiment->getInstrument(), experiment->sample());
    const auto intersectionVolume = beamProfile->getIntersectionWithSample(*sample);

    checkIntersectionVolume(intersectionVolume, 2., 5., 12.);
  }

  void test_beamHalfWidthOfSample() {
    const auto sample = createCuboid(5., 1., 6., V3D(0., 0., 0.), "sample");
    auto experiment = createExperimentWithSlitBeam(V3D(0., 0., -5.), 5., 10.);

    const auto beamProfile = BeamProfileFactory::createBeamProfile(*experiment->getInstrument(), experiment->sample());
    const auto intersectionVolume = beamProfile->getIntersectionWithSample(*sample);

    checkIntersectionVolume(intersectionVolume, 5., 2., 12.);
  }

  void test_beamHalfWidthAndHeightOfSample() {
    const auto sample = createCuboid(5., 5., 6., V3D(0., 0., 0.), "sample");
    auto experiment = createExperimentWithSlitBeam(V3D(0., 0., -5.), 5., 5.);

    const auto beamProfile = BeamProfileFactory::createBeamProfile(*experiment->getInstrument(), experiment->sample());
    const auto intersectionVolume = beamProfile->getIntersectionWithSample(*sample);

    checkIntersectionVolume(intersectionVolume, 5., 5., 12.);
  }

  void test_beamMissesSampleCylinder() {
    const auto sample = createCuboid(1., 1., 5., V3D(0., 0., 0.), "sample");
    auto experiment = createExperimentWithCircleBeam(V3D(0., 10., -10.), 1.);

    const auto beamProfile = BeamProfileFactory::createBeamProfile(*experiment->getInstrument(), experiment->sample());
    const auto intersectionVolume = beamProfile->getIntersectionWithSample(*sample);

    TS_ASSERT_EQUALS(intersectionVolume, nullptr);
  }

  void test_beamHitsSampleCylinder() {
    const auto sample = createCuboid(1., 1., 6., V3D(0., 0., 0.), "sample");
    auto experiment = createExperimentWithCircleBeam(V3D(0., 0., -10.), 1.);

    const auto beamProfile = BeamProfileFactory::createBeamProfile(*experiment->getInstrument(), experiment->sample());
    const auto intersectionVolume = beamProfile->getIntersectionWithSample(*sample);

    checkIntersectionVolume(intersectionVolume, 2., 2., 12.);
  }

  void test_beamNotOnZAxis() {
    const auto sample = createCuboid(2., 3., 5., V3D(0., 0., 0.), "sample");
    auto experiment = createExperimentWithSlitBeam(V3D(0., -10., 0.), 10., 20.);

    const auto beamProfile = BeamProfileFactory::createBeamProfile(*experiment->getInstrument(), experiment->sample());
    const auto intersectionVolume = beamProfile->getIntersectionWithSample(*sample, V3D(0., 1., 0.));

    checkIntersectionVolume(intersectionVolume, 4., 5., 6.);
  }

  void test_bigSampleSmallBeamNotOnZAxis() {
    const auto sample = createCuboid(10., 10., 10., V3D(0., 0., 0.), "sample");
    auto experiment = createExperimentWithSlitBeam(V3D(0., -10., 0.), 5., 5.);

    const auto beamProfile = BeamProfileFactory::createBeamProfile(*experiment->getInstrument(), experiment->sample());
    const auto intersectionVolume = beamProfile->getIntersectionWithSample(*sample, V3D(0., 1., 0.));

    checkIntersectionVolume(intersectionVolume, 5., 2.5, 20.);
  }
};
