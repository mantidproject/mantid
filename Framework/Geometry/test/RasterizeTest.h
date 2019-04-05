// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_GEOMETRY_RASTERIZETEST_H_
#define MANTID_GEOMETRY_RASTERIZETEST_H_

#include "MantidGeometry/Objects/CSGObject.h"
#include "MantidGeometry/Rasterize.h"
#include "MantidKernel/V3D.h"
#include "MantidTestHelpers/ComponentCreationHelper.h"
#include <cxxtest/TestSuite.h>
#include <numeric>

using namespace Mantid::Geometry;
using Mantid::Kernel::V3D;

class RasterizeTest : public CxxTest::TestSuite {
private:
  static constexpr double CYLINDER_RADIUS = .1;
  static constexpr double CYLINDER_HEIGHT = 3.;
  static constexpr double CYLINDER_INNER_RADIUS = .09;
  static constexpr double CYLINDER_VOLUME =
      M_PI * CYLINDER_RADIUS * CYLINDER_RADIUS * CYLINDER_HEIGHT;
  static constexpr double HOLLOW_CYLINDER_VOLUME =
      M_PI * CYLINDER_HEIGHT *
      (CYLINDER_RADIUS * CYLINDER_RADIUS -
       CYLINDER_INNER_RADIUS * CYLINDER_INNER_RADIUS);

  static constexpr double SPHERE_RADIUS = 3.2;
  static constexpr double SPHERE_VOLUME =
      (4. / 3.) * M_PI * SPHERE_RADIUS * SPHERE_RADIUS * SPHERE_RADIUS;

  boost::shared_ptr<CSGObject> createCylinder(bool centered) {
    V3D baseCenter;
    V3D symmetryAxis;
    if (centered) {
      baseCenter = V3D(0., -.5 * CYLINDER_HEIGHT, 0.);
      symmetryAxis = V3D(0., 1., 0.);
    } else {
      baseCenter = V3D(0.0, 3.0, 0.);
      symmetryAxis = V3D(0., 0., 1.);
    }

    return ComponentCreationHelper::createCappedCylinder(
        CYLINDER_RADIUS, CYLINDER_HEIGHT, baseCenter, symmetryAxis, "shape");
  }

  boost::shared_ptr<CSGObject> createHollowCylinder(bool centered) {
    V3D baseCenter;
    V3D symmetryAxis;
    if (centered) {
      baseCenter = V3D(0., -.5 * CYLINDER_HEIGHT, 0.);
      symmetryAxis = V3D(0., 1., 0.);
    } else {
      baseCenter = V3D(0.0, 3.0, 0.);
      symmetryAxis = V3D(0., 0., 1.);
    }

    return ComponentCreationHelper::createHollowCylinder(
        CYLINDER_INNER_RADIUS, CYLINDER_RADIUS, CYLINDER_HEIGHT, baseCenter,
        symmetryAxis, "shape");
  }

  boost::shared_ptr<CSGObject> createSphere(bool centered) {
    V3D center;
    if (centered)
      center = V3D(0., 0., 0.);
    else
      center = V3D(4., 4., 4.);

    return ComponentCreationHelper::createSphere(SPHERE_RADIUS, center,
                                                 "shape");
  }

  void simpleRasterChecks(const Raster &raster,
                          const boost::shared_ptr<CSGObject> &shape,
                          const size_t numEle, const double volume,
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
      if (shape->isValid(pos))
        numValidPos++;
    TS_ASSERT_EQUALS(numValidPos, numEle);

    TS_ASSERT_DELTA(raster.totalvolume, volume,
                    volume * relVolumeTol); // no more than 1% different
    double sumOfVolumes =
        std::accumulate(raster.volume.begin(), raster.volume.end(), 0.);
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

    boost::shared_ptr<CSGObject> cylinder = createCylinder(true);
    TS_ASSERT(cylinder);
    const auto raster = Rasterize::calculateCylinder(V3D(0., 0., 1.), cylinder,
                                                     NUM_SLICE, NUM_ANNULLI);

    // all the vector lengths should match
    constexpr size_t NUM_ELEMENTS =
        NUM_SLICE * NUM_ANNULLI * (NUM_ANNULLI + 1) * 3;
    simpleRasterChecks(raster, cylinder, NUM_ELEMENTS, CYLINDER_VOLUME);
  }

  void test_calculateHollowCylinder() {
    constexpr size_t NUM_SLICE{3};
    constexpr size_t NUM_ANNULLI{3};

    boost::shared_ptr<CSGObject> hollowCylinder = createHollowCylinder(true);
    TS_ASSERT(hollowCylinder);
    const auto raster = Rasterize::calculateHollowCylinder(
        V3D(0., 0., 1.), hollowCylinder, NUM_SLICE, NUM_ANNULLI);

    // all the vector lengths should match
    constexpr size_t NUM_ELEMENTS =
        NUM_SLICE * NUM_ANNULLI * (NUM_ANNULLI + 1) * 3;
    simpleRasterChecks(raster, hollowCylinder, NUM_ELEMENTS,
                       HOLLOW_CYLINDER_VOLUME);
  }

  void test_calculateOffsetCylinder() {
    constexpr size_t NUM_SLICE{3};
    constexpr size_t NUM_ANNULLI{3};

    boost::shared_ptr<CSGObject> cylinder = createCylinder(false);
    TS_ASSERT(cylinder);
    const auto raster = Rasterize::calculateCylinder(V3D(0., 0., 1.), cylinder,
                                                     NUM_SLICE, NUM_ANNULLI);

    // all the vector lengths should match
    constexpr size_t NUM_ELEMENTS =
        NUM_SLICE * NUM_ANNULLI * (NUM_ANNULLI + 1) * 3;
    simpleRasterChecks(raster, cylinder, NUM_ELEMENTS, CYLINDER_VOLUME);
  }

  void test_calculateOffsetHollowCylinder() {
    constexpr size_t NUM_SLICE{3};
    constexpr size_t NUM_ANNULLI{3};

    boost::shared_ptr<CSGObject> hollowCylinder = createHollowCylinder(false);
    TS_ASSERT(hollowCylinder);
    const auto raster = Rasterize::calculateHollowCylinder(
        V3D(0., 0., 1.), hollowCylinder, NUM_SLICE, NUM_ANNULLI);

    // all the vector lengths should match
    constexpr size_t NUM_ELEMENTS =
        NUM_SLICE * NUM_ANNULLI * (NUM_ANNULLI + 1) * 3;
    simpleRasterChecks(raster, hollowCylinder, NUM_ELEMENTS,
                       HOLLOW_CYLINDER_VOLUME);
  }

  void test_calculateCylinderOnSphere() {
    boost::shared_ptr<CSGObject> sphere = createSphere(true);
    TS_ASSERT(sphere);
    TS_ASSERT_THROWS(
        Rasterize::calculateCylinder(V3D(0., 0., 1.), sphere, 3, 3),
        std::invalid_argument);
  }

  void test_calculateArbitraryOnCylinder() {
    boost::shared_ptr<CSGObject> cylinder = createCylinder(true);
    TS_ASSERT(cylinder);
    const auto raster = Rasterize::calculate(V3D(0., 0., 1.), *cylinder, .1);

    // all the vector lengths should match
    simpleRasterChecks(raster, cylinder, 180, CYLINDER_VOLUME);
  }

  void test_calculateArbitraryOnSphere() {
    boost::shared_ptr<CSGObject> sphere = createSphere(true);
    TS_ASSERT(sphere);
    const auto raster = Rasterize::calculate(V3D(0., 0., 1.), sphere, .5);

    // volume is calculated poorly due to approximating all volume elements as
    // boxes
    simpleRasterChecks(raster, sphere, 912, SPHERE_VOLUME, .01);
  }

  void test_calculateArbitraryOnOffsetSphere() {
    boost::shared_ptr<CSGObject> sphere = createSphere(false);
    TS_ASSERT(sphere);
    const auto raster = Rasterize::calculate(V3D(0., 0., 1.), sphere, .5);

    // volume is calculated poorly due to approximating all volume elements as
    // boxes
    simpleRasterChecks(raster, sphere, 912, SPHERE_VOLUME, .01);
  }
};

#endif /* MANTID_GEOMETRY_RASTERIZETEST_H_ */
