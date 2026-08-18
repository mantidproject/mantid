// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidFrameworkTestHelpers/ComponentCreationHelper.h"
#include "MantidGeometry/Objects/CSGObject.h"
#include "MantidGeometry/Rasterize.h"
#include "MantidKernel/V3D.h"
#include <cxxtest/TestSuite.h>
#include <numeric>

using namespace Mantid::Geometry;
using Mantid::Kernel::DblMatrix;
using Mantid::Kernel::V3D;

class RasterizeTest : public CxxTest::TestSuite {
private:
  static constexpr double CYLINDER_RADIUS = .1;
  static constexpr double CYLINDER_HEIGHT = 3.;
  static constexpr double CYLINDER_INNER_RADIUS = .09;
  static constexpr double CYLINDER_VOLUME = M_PI * CYLINDER_RADIUS * CYLINDER_RADIUS * CYLINDER_HEIGHT;
  static constexpr double HOLLOW_CYLINDER_VOLUME =
      M_PI * CYLINDER_HEIGHT * (CYLINDER_RADIUS * CYLINDER_RADIUS - CYLINDER_INNER_RADIUS * CYLINDER_INNER_RADIUS);

  static constexpr double SPHERE_RADIUS = 3.2;
  static constexpr double SPHERE_VOLUME = (4. / 3.) * M_PI * SPHERE_RADIUS * SPHERE_RADIUS * SPHERE_RADIUS;

  CSGObject createCylinder(bool centered) {
    V3D baseCenter;
    V3D symmetryAxis;
    if (centered) {
      baseCenter = V3D(0., -.5 * CYLINDER_HEIGHT, 0.);
      symmetryAxis = V3D(0., 1., 0.);
    } else {
      baseCenter = V3D(0.0, 3.0, 0.);
      symmetryAxis = V3D(0., 0., 1.);
    }

    std::shared_ptr<CSGObject> result = ComponentCreationHelper::createCappedCylinder(
        CYLINDER_RADIUS, CYLINDER_HEIGHT, baseCenter, symmetryAxis, "shape");
    TS_ASSERT(result);
    return *result;
  }

  CSGObject createHollowCylinder(bool centered) {
    V3D baseCenter;
    V3D symmetryAxis;
    if (centered) {
      baseCenter = V3D(0., -.5 * CYLINDER_HEIGHT, 0.);
      symmetryAxis = V3D(0., 1., 0.);
    } else {
      baseCenter = V3D(0.0, 3.0, 0.);
      symmetryAxis = V3D(0., 0., 1.);
    }

    std::shared_ptr<CSGObject> result = ComponentCreationHelper::createHollowCylinder(
        CYLINDER_INNER_RADIUS, CYLINDER_RADIUS, CYLINDER_HEIGHT, baseCenter, symmetryAxis, "shape");
    TS_ASSERT(result);
    return *result;
  }

  CSGObject createSphere(bool centered) {
    V3D center;
    if (centered)
      center = V3D(0., 0., 0.);
    else
      center = V3D(4., 4., 4.);

    std::shared_ptr<CSGObject> result = ComponentCreationHelper::createSphere(SPHERE_RADIUS, center, "shape");
    TS_ASSERT(result);
    return *result;
  }

  void simpleRasterChecks(const Raster &raster, const CSGObject &shape, const size_t numEle, const double volume,
                          const double relVolumeTol = 0.001) {
    TS_ASSERT_EQUALS(raster.l1.size(), numEle);
    TS_ASSERT_EQUALS(raster.position.size(), numEle);
    TS_ASSERT_EQUALS(raster.volume.size(), numEle);

    // all lengths into the shape should be positive
    size_t numPositiveL1 = 0;
    for (const auto &ele : raster.l1)
      if (ele > 0.)
        numPositiveL1 += 1;
    TS_ASSERT_EQUALS(numPositiveL1, numEle);

    // all element volumes should be positive
    size_t numPositiveVolume = 0;
    for (const auto &ele : raster.volume)
      if (ele > 0.)
        numPositiveVolume += 1;
    TS_ASSERT_EQUALS(numPositiveVolume, numEle);

    size_t numValidPos = 0;
    for (const auto &pos : raster.position)
      if (shape.isValid(pos))
        numValidPos++;
    TS_ASSERT_EQUALS(numValidPos, numEle);

    TS_ASSERT_DELTA(raster.totalvolume, volume,
                    volume * relVolumeTol); // no more than 1% different
    double sumOfVolumes = std::accumulate(raster.volume.begin(), raster.volume.end(), 0.);
    TS_ASSERT_DELTA(sumOfVolumes, volume,
                    volume * relVolumeTol); // no more than 1% different
  }

public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static RasterizeTest *createSuite() { return new RasterizeTest(); }
  static void destroySuite(RasterizeTest *suite) { delete suite; }

  void test_calculateCylinder() {
    constexpr size_t NUM_SLICE{3};
    constexpr size_t NUM_ANNULLI{3};

    const auto cylinder = createCylinder(true);
    const auto raster = Rasterize::calculateCylinder(V3D(0., 0., 1.), cylinder, cylinder, NUM_SLICE, NUM_ANNULLI);

    // all the vector lengths should match
    constexpr size_t NUM_ELEMENTS = NUM_SLICE * NUM_ANNULLI * (NUM_ANNULLI + 1) * 3;
    simpleRasterChecks(raster, cylinder, NUM_ELEMENTS, CYLINDER_VOLUME);
  }

  void test_calculateHollowCylinder() {
    constexpr size_t NUM_SLICE{3};
    constexpr size_t NUM_ANNULLI{3};

    const auto hollowCylinder = createHollowCylinder(true);
    const auto raster =
        Rasterize::calculateHollowCylinder(V3D(0., 0., 1.), hollowCylinder, hollowCylinder, NUM_SLICE, NUM_ANNULLI);

    // all the vector lengths should match
    size_t NUM_ELEMENTS = 0;
    const double dR = (CYLINDER_RADIUS - CYLINDER_INNER_RADIUS) / static_cast<double>(NUM_ANNULLI);
    size_t Ni = static_cast<size_t>(CYLINDER_INNER_RADIUS / dR) * 6;
    for (size_t i = 0; i < NUM_ANNULLI; i++) {
      Ni += 6;
      NUM_ELEMENTS += Ni;
    }
    NUM_ELEMENTS *= NUM_SLICE;
    simpleRasterChecks(raster, hollowCylinder, NUM_ELEMENTS, HOLLOW_CYLINDER_VOLUME);

    // check to ensure that all points are within the shell of the hollow cylinder
    // NOTE:
    // For a centered hollow cylinder, the element_i should have
    // * x^2 + z^2 < CYLINDER_RADIUS^2
    // * x^2 + z^2 > CYLINDER_INNER_RADIUS^2
    // * |y| < 0.5 * CYLINDER_HEIGHT
    // where (x, y, z) is the element position vector
    const double Rsquared = CYLINDER_RADIUS * CYLINDER_RADIUS;
    const double Risquared = CYLINDER_INNER_RADIUS * CYLINDER_INNER_RADIUS;
    double r, h;
    for (const auto &pos : raster.position) {
      r = pos[0] * pos[0] + pos[2] * pos[2];
      h = std::abs(pos[1]);
      TS_ASSERT_LESS_THAN_EQUALS(r, Rsquared);
      TS_ASSERT_LESS_THAN_EQUALS(Risquared, r);
      TS_ASSERT_LESS_THAN_EQUALS(h, 0.5 * CYLINDER_HEIGHT);
    }
  }

  void test_calculateOffsetCylinder() {
    constexpr size_t NUM_SLICE{3};
    constexpr size_t NUM_ANNULLI{3};

    const auto cylinder = createCylinder(false);
    const auto raster = Rasterize::calculateCylinder(V3D(0., 0., 1.), cylinder, cylinder, NUM_SLICE, NUM_ANNULLI);

    // all the vector lengths should match
    constexpr size_t NUM_ELEMENTS = NUM_SLICE * NUM_ANNULLI * (NUM_ANNULLI + 1) * 3;
    simpleRasterChecks(raster, cylinder, NUM_ELEMENTS, CYLINDER_VOLUME);
  }

  void test_calculateOffsetHollowCylinder() {
    constexpr size_t NUM_SLICE{3};
    constexpr size_t NUM_ANNULLI{3};

    const auto hollowCylinder = createHollowCylinder(false);
    const auto raster =
        Rasterize::calculateHollowCylinder(V3D(0., 0., 1.), hollowCylinder, hollowCylinder, NUM_SLICE, NUM_ANNULLI);

    // all the vector lengths should match
    size_t NUM_ELEMENTS = 0;
    const double dR = (CYLINDER_RADIUS - CYLINDER_INNER_RADIUS) / static_cast<double>(NUM_ANNULLI);
    size_t Ni = static_cast<size_t>(CYLINDER_INNER_RADIUS / dR) * 6;
    for (size_t i = 0; i < NUM_ANNULLI; i++) {
      Ni += 6;
      NUM_ELEMENTS += Ni;
    }
    NUM_ELEMENTS *= NUM_SLICE;
    simpleRasterChecks(raster, hollowCylinder, NUM_ELEMENTS, HOLLOW_CYLINDER_VOLUME);
  }

  void test_calculateHollowCylinderShell() {
    // tests a hollow cylinder with a shell of one element size
    constexpr double ELEMENT_SIZE{5.0e-4}; // 0.5 mm element size
    constexpr double HEIGHT{1.0};
    constexpr V3D BASE{0.0, -0.5 * HEIGHT, 0.0};
    constexpr V3D AXIS{0.0, 1.0, 0.0}; // sym along +Y
    constexpr double RADIUS{0.3};

    std::shared_ptr<CSGObject> hollowCylinder =
        ComponentCreationHelper::createHollowCylinder(RADIUS - ELEMENT_SIZE, RADIUS, HEIGHT, BASE, AXIS, "shape");
    const auto raster = Rasterize::calculate(V3D(0., 0., 1.), *hollowCylinder, *hollowCylinder, ELEMENT_SIZE);

    const double vol = M_PI * HEIGHT * (RADIUS * RADIUS - (RADIUS - ELEMENT_SIZE) * (RADIUS - ELEMENT_SIZE));
    simpleRasterChecks(raster, *hollowCylinder, raster.l1.size(), vol);
  }

  void test_calculateHollowCylinderSingleElement() {
    // tests a hollow cylinder with one slice and annuli
    constexpr double ELEMENT_SIZE{5.0e-4}; // 0.5 mm element size
    constexpr double HEIGHT{1.0};
    constexpr V3D BASE{0.0, -0.5 * ELEMENT_SIZE, 0.0};
    constexpr V3D AXIS{0.0, 1.0, 0.0}; // sym along +Y
    constexpr double RADIUS{0.3};

    std::shared_ptr<CSGObject> hollowCylinder =
        ComponentCreationHelper::createHollowCylinder(RADIUS - ELEMENT_SIZE, RADIUS, HEIGHT, BASE, AXIS, "shape");
    // test using the generic calculate function
    const auto raster = Rasterize::calculate(V3D(0., 0., 1.), *hollowCylinder, *hollowCylinder, ELEMENT_SIZE);

    const double vol = M_PI * HEIGHT * (RADIUS * RADIUS - (RADIUS - ELEMENT_SIZE) * (RADIUS - ELEMENT_SIZE));
    simpleRasterChecks(raster, *hollowCylinder, raster.l1.size(), vol);
  }

  void test_calculateHollowCylinderManyElements() {
    // tests a hollow cylinder with more slices and annuli
    constexpr double ELEMENT_SIZE{0.005};
    constexpr double HEIGHT{0.1};
    constexpr V3D BASE{0.0, -0.5 * HEIGHT, 0.0};
    constexpr V3D AXIS{0.0, 1.0, 0.0}; // sym along +Y
    constexpr double INNER_RADIUS{0.2};
    constexpr double OUTER_RADIUS{0.3};

    std::shared_ptr<CSGObject> hollowCylinder =
        ComponentCreationHelper::createHollowCylinder(INNER_RADIUS, OUTER_RADIUS, HEIGHT, BASE, AXIS, "shape");
    const auto raster = Rasterize::calculate(V3D(0., 0., 1.), *hollowCylinder, *hollowCylinder, ELEMENT_SIZE);

    const double vol = M_PI * HEIGHT * (OUTER_RADIUS * OUTER_RADIUS - INNER_RADIUS * INNER_RADIUS);
    simpleRasterChecks(raster, *hollowCylinder, raster.l1.size(), vol);
  }

  void test_calculateCylinderOnSphere() {
    const auto sphere = createSphere(true);
    TS_ASSERT_THROWS(Rasterize::calculateCylinder(V3D(0., 0., 1.), sphere, sphere, 3, 3),
                     const std::invalid_argument &);
  }

  void test_calculateArbitraryOnCylinder() {
    const auto cylinder = createCylinder(true);
    const auto raster = Rasterize::calculate(V3D(0., 0., 1.), cylinder, cylinder, .1);

    // all the vector lengths should match
    simpleRasterChecks(raster, cylinder, 180, CYLINDER_VOLUME);
  }

  void test_calculateArbitraryOnSphere() {
    const auto sphere = createSphere(true);
    const auto raster = Rasterize::calculate(V3D(0., 0., 1.), sphere, sphere, .5);

    // volume is calculated poorly due to approximating all volume elements as
    // boxes
    simpleRasterChecks(raster, sphere, 912, SPHERE_VOLUME, .01);
  }

  void test_calculateArbitraryOnOffsetSphere() {
    const auto sphere = createSphere(false);
    const auto raster = Rasterize::calculate(V3D(0., 0., 1.), sphere, sphere, .5);

    // volume is calculated poorly due to approximating all volume elements as
    // boxes
    simpleRasterChecks(raster, sphere, 912, SPHERE_VOLUME, .01);
  }

  void test_SmallerIntegrationVolumeWithinBiggerSample() {
    V3D sampleCenter(0., 0., 0.);
    const std::shared_ptr<CSGObject> sample = ComponentCreationHelper::createCuboid(2, 2, 2, sampleCenter, "sample");
    const std::shared_ptr<CSGObject> integVolume =
        ComponentCreationHelper::createCuboid(1, 1, 1, sampleCenter, "sample");
    const auto raster_result = Rasterize::calculate(V3D(0., 0., 1.), *integVolume, *sample, 1.0);

    const int size = static_cast<int>(raster_result.l1.size());

    TS_ASSERT_EQUALS(size, 8);
    TS_ASSERT_DELTA(raster_result.totalvolume, 8.0, 0.001);
    for (int i = 0; i < size; i++) {
      // all l1s should be greater or equal to 1
      TS_ASSERT_LESS_THAN_EQUALS(1.0, raster_result.l1[i])
    };
  }

  void test_calculateInLabFrame_WithIdentityRotationMatchesCalculate() {
    // With no rotation the two frames coincide, so the lab-frame overload must reproduce the
    // existing generic raster exactly - this is what makes it safe to route callers through it.
    const V3D center(0., 0., 0.);
    const std::shared_ptr<CSGObject> sample = ComponentCreationHelper::createCuboid(2, 2, 2, center, "sample");
    const std::shared_ptr<CSGObject> integVolume = ComponentCreationHelper::createCuboid(1, 1, 1, center, "gauge");
    const V3D beamDirection(0., 0., 1.);

    const auto expected = Rasterize::calculate(beamDirection, *integVolume, *sample, 1.0);
    const auto actual =
        Rasterize::calculateInLabFrame(beamDirection, *integVolume, *sample, 1.0, DblMatrix(3, 3, true));

    TS_ASSERT_EQUALS(actual.position.size(), expected.position.size());
    TS_ASSERT_DELTA(actual.totalvolume, expected.totalvolume, 1e-12);
    for (size_t i = 0; i < expected.position.size(); ++i) {
      TS_ASSERT_EQUALS(actual.position[i], expected.position[i]);
      TS_ASSERT_DELTA(actual.l1[i], expected.l1[i], 1e-12);
      TS_ASSERT_DELTA(actual.volume[i], expected.volume[i], 1e-12);
    }
  }

  void test_calculateInLabFrame_AppliesTheRotationToTheSample() {
    // The sample is thin along x in its own frame; the integration volume is long along x in the
    // lab frame. Rotating the sample by 90 degrees about y turns its thin direction into z, so the
    // whole length of the integration volume then lies inside it. If the rotation were ignored the
    // sample would still be thin along x and most of the volume would be rejected, so the rotated
    // raster having strictly more elements is what proves the transform is applied.
    const V3D center(0., 0., 0.);
    const std::shared_ptr<CSGObject> sample = ComponentCreationHelper::createCuboid(0.5, 3.0, 3.0, center, "sample");
    const std::shared_ptr<CSGObject> integVolume =
        ComponentCreationHelper::createCuboid(2.0, 0.5, 0.5, center, "gauge");
    const V3D beamDirection(0., 0., 1.);
    constexpr double ELEMENT_SIZE{0.5};

    // maps shape (1,0,0) to lab (0,0,-1), so lab x corresponds to the sample's z
    DblMatrix rotationY90(3, 3, false);
    rotationY90[0][2] = 1.0;
    rotationY90[1][1] = 1.0;
    rotationY90[2][0] = -1.0;

    const auto unrotated =
        Rasterize::calculateInLabFrame(beamDirection, *integVolume, *sample, ELEMENT_SIZE, DblMatrix(3, 3, true));
    const auto rotated =
        Rasterize::calculateInLabFrame(beamDirection, *integVolume, *sample, ELEMENT_SIZE, rotationY90);

    TS_ASSERT_LESS_THAN(unrotated.position.size(), rotated.position.size());

    // Every returned position is reported in the lab frame, so it must lie inside the integration
    // volume as authored, and must lie inside the sample once mapped back into the sample's frame.
    DblMatrix rotationInv(rotationY90);
    rotationInv.Invert();
    for (const auto &positionInLabFrame : rotated.position) {
      TS_ASSERT(integVolume->isValid(positionInLabFrame));
      TS_ASSERT(sample->isValid(rotationInv * positionInLabFrame));
    }

    // l1 is traced through the sample, so it cannot exceed the sample's longest dimension
    for (const auto &l1 : rotated.l1) {
      TS_ASSERT_LESS_THAN(0.0, l1);
      TS_ASSERT_LESS_THAN_EQUALS(l1, 6.0);
    }
    TS_ASSERT_DELTA(rotated.totalvolume,
                    static_cast<double>(rotated.volume.size()) * ELEMENT_SIZE * ELEMENT_SIZE * ELEMENT_SIZE, 1e-12);
  }

  void test_calculateInLabFrame_ThrowsWhenVolumeIsThinnerThanOneElement() {
    const V3D center(0., 0., 0.);
    const std::shared_ptr<CSGObject> sample = ComponentCreationHelper::createCuboid(2, 2, 2, center, "sample");
    const std::shared_ptr<CSGObject> integVolume =
        ComponentCreationHelper::createCuboid(0.1, 0.1, 0.1, center, "gauge");

    TS_ASSERT_THROWS(Rasterize::calculateInLabFrame(V3D(0., 0., 1.), *integVolume, *sample, 1.0, DblMatrix(3, 3, true)),
                     const std::runtime_error &);
  }

  void test_LargerIntegrationVolumeThanSample() {
    V3D sampleCenter(0., 0., 0.);
    const std::shared_ptr<CSGObject> integVolume =
        ComponentCreationHelper::createCuboid(2, 2, 2, sampleCenter, "sample");
    const std::shared_ptr<CSGObject> sample = ComponentCreationHelper::createCuboid(1, 1, 1, sampleCenter, "sample");
    const auto raster_result = Rasterize::calculate(V3D(0., 0., 1.), *integVolume, *sample, 1.0);

    const int size = static_cast<int>(raster_result.l1.size());

    TS_ASSERT_EQUALS(size, 8);
    TS_ASSERT_DELTA(raster_result.totalvolume, 8.0, 0.001);
    for (int i = 0; i < size; i++) {
      // all l1s should be less than or equal to 2
      TS_ASSERT_LESS_THAN_EQUALS(raster_result.l1[i], 2.0)
    };
  }
};
