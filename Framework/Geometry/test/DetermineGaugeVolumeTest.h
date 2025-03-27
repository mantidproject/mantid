// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidFrameworkTestHelpers/ComponentCreationHelper.h"
#include "MantidGeometry/DetermineGaugeVolume.h"
#include "MantidGeometry/Objects/CSGObject.h"
#include "MantidKernel/V3D.h"
#include <cxxtest/TestSuite.h>
#include <numeric>

using namespace Mantid::Geometry;
using Mantid::Kernel::V3D;

class DetermineGaugeVolumeTest : public CxxTest::TestSuite {
private:
  void checkGaugeVolume(const std::shared_ptr<IObject> &gaugeVolume, double expectedHeight, double expectedWidth,
                        double expectedDepth) {
    TS_ASSERT(gaugeVolume);
    double width, height, depth;
    height = gaugeVolume->getBoundingBox().xMax() - gaugeVolume->getBoundingBox().xMin();
    width = gaugeVolume->getBoundingBox().yMax() - gaugeVolume->getBoundingBox().yMin();
    depth = gaugeVolume->getBoundingBox().zMax() - gaugeVolume->getBoundingBox().zMin();

    TS_ASSERT_EQUALS(width, expectedWidth);
    TS_ASSERT_EQUALS(height, expectedHeight);
    TS_ASSERT_EQUALS(depth, expectedDepth);
  }

public:
  static DetermineGaugeVolumeTest *createSuite() { return new DetermineGaugeVolumeTest(); }
  static void destroySuite(DetermineGaugeVolumeTest *suite) { delete suite; }

  void test_beamMissesSample() {
    V3D sampleCenter(0., 0., 0.);
    const auto sample = ComponentCreationHelper::createCuboid(1., 1., 5., sampleCenter, "sample");

    BeamProfile beam;

    beam.shape = "Slit";
    beam.height = 10;
    beam.width = 10;
    beam.direction = V3D(0, 0, 1);
    beam.center = V3D(0, 10, -10);

    const auto gaugeVolume = GaugeVolume::determineGaugeVolume(*sample, beam);
    TS_ASSERT_EQUALS(gaugeVolume, nullptr);
  }

  void test_sampleEntirelyWithinBeam() {
    V3D sampleCenter(0., 0., 0.);
    const auto sample = ComponentCreationHelper::createCuboid(1., 1., 5., sampleCenter, "sample");

    BeamProfile beam;

    beam.shape = "Slit";
    beam.height = 10;
    beam.width = 10;
    beam.direction = V3D(0, 0, 1);
    beam.center = V3D(0, 0, -10);

    const auto gaugeVolume = GaugeVolume::determineGaugeVolume(*sample, beam);

    checkGaugeVolume(gaugeVolume, 2., 2., 10.);
  }

  void test_beamHalfHeightOfSample() {
    V3D sampleCenter(0., 0., 0.);
    const auto sample = ComponentCreationHelper::createCuboid(1., 5., 6., sampleCenter, "sample");

    BeamProfile beam;

    beam.shape = "Slit";
    beam.height = 5;
    beam.width = 10;
    beam.direction = V3D(0, 0, 1);
    beam.center = V3D(0, 0, -5);

    const auto gaugeVolume = GaugeVolume::determineGaugeVolume(*sample, beam);

    checkGaugeVolume(gaugeVolume, 2., 5., 12.);
  }

  void test_beamHalfWidthOfSample() {
    V3D sampleCenter(0., 0., 0.);
    const auto sample = ComponentCreationHelper::createCuboid(5., 1., 6., sampleCenter, "sample");

    BeamProfile beam;

    beam.shape = "Slit";
    beam.height = 10;
    beam.width = 5;
    beam.direction = V3D(0, 0, 1);
    beam.center = V3D(0, 0, -5);

    const auto gaugeVolume = GaugeVolume::determineGaugeVolume(*sample, beam);

    checkGaugeVolume(gaugeVolume, 5., 2., 12.);
  }

  void test_beamHalfWidthAndHeightOfSample() {
    V3D sampleCenter(0., 0., 0.);
    const auto sample = ComponentCreationHelper::createCuboid(5., 5., 6., sampleCenter, "sample");

    BeamProfile beam;

    beam.shape = "Slit";
    beam.height = 5;
    beam.width = 5;
    beam.direction = V3D(0, 0, 1);
    beam.center = V3D(0, 0, -5);

    const auto gaugeVolume = GaugeVolume::determineGaugeVolume(*sample, beam);

    checkGaugeVolume(gaugeVolume, 5., 5., 12.);
  }

  void test_beamMissesSampleCylinder() {
    V3D sampleCenter(0., 0., 0.);
    const auto sample = ComponentCreationHelper::createCuboid(1., 1., 5., sampleCenter, "sample");

    BeamProfile beam;

    beam.shape = "Circle";
    beam.radius = 1;
    beam.direction = V3D(0, 0, 1);
    beam.center = V3D(0, 10, -10);

    const auto gaugeVolume = GaugeVolume::determineGaugeVolume(*sample, beam);
    TS_ASSERT_EQUALS(gaugeVolume, nullptr);
  }

  void test_beamHitsSampleCylinder() {
    V3D sampleCenter(0., 0., 0.);
    const auto sample = ComponentCreationHelper::createCuboid(1., 1., 5., sampleCenter, "sample");

    BeamProfile beam;

    beam.shape = "Circle";
    beam.radius = 1;
    beam.direction = V3D(0, 0, 1);
    beam.center = V3D(0, 0, -10);

    const auto gaugeVolume = GaugeVolume::determineGaugeVolume(*sample, beam);

    checkGaugeVolume(gaugeVolume, 2., 2., 10.);
  }

  void test_beamNotOnZAxis() {
    V3D sampleCenter(0., 0., 0.);
    const auto sample = ComponentCreationHelper::createCuboid(2., 3., 5., sampleCenter, "sample");

    BeamProfile beam;

    beam.shape = "Slit";
    beam.height = 20;
    beam.width = 10;
    beam.center = V3D(0, -10, 0);
    beam.direction = normalize(sampleCenter - beam.center);
    printf("beam direction: %f %f %f\n", beam.direction[0], beam.direction[1], beam.direction[2]);

    const auto gaugeVolume = GaugeVolume::determineGaugeVolume(*sample, beam);
    checkGaugeVolume(gaugeVolume, 4., 5., 6.);
  }

  void test_bigSampleSmallBeamNotOnZAxis() {
    V3D sampleCenter(0., 0., 0.);
    const auto sample = ComponentCreationHelper::createCuboid(10., 10., 10., sampleCenter, "sample");

    BeamProfile beam;

    beam.shape = "Slit";
    beam.height = 5;
    beam.width = 5;
    beam.center = V3D(0, -10, 0);
    beam.direction = normalize(sampleCenter - beam.center);

    const auto gaugeVolume = GaugeVolume::determineGaugeVolume(*sample, beam);

    checkGaugeVolume(gaugeVolume, 5., 2.5, 20.);
  }

  void test_beamProfileSlit() {
    const auto inst = ComponentCreationHelper::createTestInstrumentRectangular(2, 10);
    ParameterMap_sptr pmap = std::make_shared<ParameterMap>();
    const auto newInstrument = Instrument(inst, pmap);
    const auto source = newInstrument.getSource()->getComponentID();

    pmap->addDouble(source, "beam-width", 10.);
    pmap->addDouble(source, "beam-height", 5.);
    pmap->addString(source, "beam-shape", "Slit");
    V3D direction(0., 0., 1.);

    auto beam = BeamProfile::create(newInstrument.getSource(), direction);

    TS_ASSERT_EQUALS(beam->shape, "Slit");
    TS_ASSERT_EQUALS(beam->height, 5.);
    TS_ASSERT_EQUALS(beam->width, 10.);
    TS_ASSERT_EQUALS(beam->direction, direction);
    TS_ASSERT_EQUALS(beam->center, V3D(0., 0., -10.));
  }

  void test_beamProfileCylinder() {
    const auto inst = ComponentCreationHelper::createTestInstrumentRectangular(2, 10);
    ParameterMap_sptr pmap = std::make_shared<ParameterMap>();
    const auto newInstrument = Instrument(inst, pmap);
    const auto source = newInstrument.getSource()->getComponentID();

    pmap->addDouble(source, "beam-radius", 10.);
    pmap->addString(source, "beam-shape", "Circle");

    V3D direction(0., 0., 1.);

    auto beam = BeamProfile::create(newInstrument.getSource(), direction);

    TS_ASSERT_EQUALS(beam->shape, "Circle");
    TS_ASSERT_EQUALS(beam->radius, 10.);
    TS_ASSERT_EQUALS(beam->direction, direction);
    TS_ASSERT_EQUALS(beam->center, V3D(0., 0., -10.));
  }

  void test_beamProfileNoBoundingBox() {
    const auto inst = ComponentCreationHelper::createTestInstrumentRectangular(2, 10);
    ParameterMap_sptr pmap = std::make_shared<ParameterMap>();
    const auto newInstrument = Instrument(inst, pmap);
    const auto source = newInstrument.getSource()->getComponentID();

    pmap->addDouble(source, "beam-radius", 10.);
    pmap->addString(source, "beam-shape", "VOID");

    auto beam = BeamProfile::create(newInstrument.getSource(), V3D(0., 0., 1.));

    TS_ASSERT_EQUALS(beam, std::nullopt);
  }
};
