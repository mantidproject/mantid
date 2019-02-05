// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_GEOMETRY_RASTERIZETEST_H_
#define MANTID_GEOMETRY_RASTERIZETEST_H_

#include "MantidGeometry/Objects/CSGObject.h"
#include "MantidGeometry/Objects/IObject.h"
#include "MantidGeometry/Objects/ShapeFactory.h"
#include "MantidGeometry/Rasterize.h"
#include "MantidKernel/V3D.h"
#include <cxxtest/TestSuite.h>
#include <numeric>

using namespace Mantid::Geometry;
using Mantid::Kernel::V3D;

class RasterizeTest : public CxxTest::TestSuite {
private:
  // all examples are taken from ShapeFactoryTest with some small modifications
  const std::string cylinderXml =
      "<cylinder id=\"shape\"> "
      "(<centre-of-bottom-base x=\"0.0\" y=\"-1.5\" z=\"0\"/>)"
      "(<axis x=\"0.0\" y=\"1\" z=\"0\" /> )"
      "<radius val=\"0.1\" /> "
      "<height val=\"3\" /> "
      "</cylinder>";
  const std::string cylinderOffsetXml =
      "<cylinder id=\"shape\"> "
      "(<centre-of-bottom-base x=\"0.0\" y=\"3.0\" z=\"0.0\"/>)"
      "(<axis x=\"0.0\" y=\"0.0\" z=\"1\" /> )"
      "<radius val=\"0.1\" /> "
      "<height val=\"3\" /> "
      "</cylinder>";
  static constexpr double CYLINDER_VOLUME = M_PI * .1 * .1 * 3.;

  const std::string sphereXml = "<sphere id=\"shape\"> "
                                "(<centre x=\"0.0\"  y=\"0.0\" z=\"0.0\" /> )"
                                "<radius val=\"3.2\" /> "
                                "</sphere>"
                                "<algebra val=\"shape\" /> ";
  const std::string sphereOffsetXml = "<sphere id=\"shape\"> "
                                      "(<centre x=\"4\"  y=\"4\" z=\"4\" /> )"
                                      "<radius val=\"3.2\" /> "
                                      "</sphere>"
                                      "<algebra val=\"shape\" /> ";
  static constexpr double SPHERE_VOLUME = (4. / 3.) * M_PI * 3.2 * 3.2 * 3.2;

  boost::shared_ptr<IObject> createCylinder(bool centered) {
    if (centered)
      return ShapeFactory().createShape(cylinderXml);
    else
      return ShapeFactory().createShape(cylinderOffsetXml);
  }

  boost::shared_ptr<IObject> createSphere(bool centered) {
    if (centered)
      return ShapeFactory().createShape(sphereXml);
    else
      return ShapeFactory().createShape(sphereOffsetXml);
  }

  void simpleRasterChecks(const Raster &raster,
                          const boost::shared_ptr<IObject> &shape,
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
    const size_t NUM_SLICE{3};
    const size_t NUM_ANNULLI{3};

    boost::shared_ptr<IObject> cylinder = createCylinder(true);
    TS_ASSERT(cylinder);
    const auto raster = Rasterize::calculateCylinder(V3D(0., 0., 1.), cylinder,
                                                     NUM_SLICE, NUM_ANNULLI);

    // all the vector lengths should match
    constexpr size_t NUM_ELEMENTS =
        NUM_SLICE * NUM_ANNULLI * (NUM_ANNULLI + 1) * 3;
    simpleRasterChecks(raster, cylinder, NUM_ELEMENTS, CYLINDER_VOLUME);
  }

  void test_calculateOffsetCylinder() {
    const size_t NUM_SLICE{3};
    const size_t NUM_ANNULLI{3};

    boost::shared_ptr<IObject> cylinder = createCylinder(false);
    TS_ASSERT(cylinder);
    const auto raster = Rasterize::calculateCylinder(V3D(0., 0., 1.), cylinder,
                                                     NUM_SLICE, NUM_ANNULLI);

    // all the vector lengths should match
    constexpr size_t NUM_ELEMENTS =
        NUM_SLICE * NUM_ANNULLI * (NUM_ANNULLI + 1) * 3;
    simpleRasterChecks(raster, cylinder, NUM_ELEMENTS, CYLINDER_VOLUME);
  }

  void test_calculateCylinderOnSphere() {
    boost::shared_ptr<IObject> sphere = createSphere(true);
    TS_ASSERT(sphere);
    TS_ASSERT_THROWS(
        Rasterize::calculateCylinder(V3D(0., 0., 1.), sphere, 3, 3),
        std::logic_error);
  }

  void test_calculateArbitraryOnCylinder() {
    boost::shared_ptr<IObject> cylinder = createCylinder(true);
    TS_ASSERT(cylinder);
    const auto raster = Rasterize::calculate(V3D(0., 0., 1.), cylinder, .1);

    // all the vector lengths should match
    simpleRasterChecks(raster, cylinder, 180, CYLINDER_VOLUME);
  }

  void test_calculateArbitraryOnSphere() {
    boost::shared_ptr<IObject> sphere = createSphere(true);
    TS_ASSERT(sphere);
    const auto raster = Rasterize::calculate(V3D(0., 0., 1.), sphere, .5);

    // volume is calculated poorly due to approximating all volume elements as
    // boxes
    simpleRasterChecks(raster, sphere, 912, SPHERE_VOLUME, .01);
  }

  void test_calculateArbitraryOnOffsetSphere() {
    boost::shared_ptr<IObject> sphere = createSphere(false);
    TS_ASSERT(sphere);
    const auto raster = Rasterize::calculate(V3D(0., 0., 1.), sphere, .5);

    // volume is calculated poorly due to approximating all volume elements as
    // boxes
    simpleRasterChecks(raster, sphere, 912, SPHERE_VOLUME, .01);
  }
};

#endif /* MANTID_GEOMETRY_RASTERIZETEST_H_ */
