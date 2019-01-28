// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_GEOMETRY_RASTERIZETEST_H_
#define MANTID_GEOMETRY_RASTERIZETEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidGeometry/Rasterize.h"
#include "MantidGeometry/Objects/IObject.h"
#include "MantidGeometry/Objects/CSGObject.h"
#include "MantidGeometry/Objects/ShapeFactory.h"
#include "MantidKernel/V3D.h"

using namespace Mantid::Geometry;
using Mantid::Kernel::V3D;

class RasterizeTest : public CxxTest::TestSuite {
private:
  // all examples are taken from ShapeFactoryTest
  std::string cylinderXml =
      "<cylinder id=\"shape\"> "
      "(<centre-of-bottom-base x=\"0.0\" y=\"0.0\" z=\"0.0\"/>)"
      "(<axis x=\"0.0\" y=\"0.0\" z=\"1\" /> )"
      "<radius val=\"0.1\" /> "
      "<height val=\"3\" /> "
      "</cylinder>";

  std::string sphereXml = "<sphere id=\"shape\"> "
                          "(<centre x=\"4.1\"  y=\"2.1\" z=\"8.1\" /> )"
                          "<radius val=\"3.2\" /> "
                          "</sphere>"
                          "<algebra val=\"shape\" /> ";

  boost::shared_ptr<IObject> createCylinder() {
    return ShapeFactory().createShape(cylinderXml);
  }

  boost::shared_ptr<IObject> createSphere() {
    return ShapeFactory().createShape(sphereXml);
  }

  void simpleRasterChecks(const Raster &raster, const size_t numEle) {
    TS_ASSERT_EQUALS(raster.l1.size(), numEle);
    TS_ASSERT_EQUALS(raster.position.size(), numEle);
    TS_ASSERT_EQUALS(raster.volume.size(), numEle);

    // all lengths into the shape should be positive
    for (const auto &ele : raster.l1)
      TS_ASSERT(ele > 0.);

    // all element volumes should be positive
    for (const auto &ele : raster.volume)
      TS_ASSERT(ele > 0.);
  }

public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static RasterizeTest *createSuite() { return new RasterizeTest(); }
  static void destroySuite(RasterizeTest *suite) { delete suite; }

  void test_calculateCylinder() {
    const size_t NUM_SLICE{ 3 };
    const size_t NUM_ANNULLI{ 3 };

    boost::shared_ptr<IObject> cylinder = createCylinder();
    TS_ASSERT(cylinder);
    const auto raster = Rasterize::calculateCylinder(V3D(0., 0., 1.), cylinder,
                                                     NUM_SLICE, NUM_ANNULLI);

    // all the vector lengths should match
    constexpr size_t NUM_ELEMENTS =
        NUM_SLICE * NUM_ANNULLI * (NUM_ANNULLI + 1) * 3;
    simpleRasterChecks(raster, NUM_ELEMENTS);
  }

  void test_calculateCylinderFromSphere() {
    boost::shared_ptr<IObject> sphere = createSphere();
    TS_ASSERT(sphere);
    TS_ASSERT_THROWS(
        Rasterize::calculateCylinder(V3D(0., 0., 1.), sphere, 3, 3),
        std::logic_error);
  }
};

#endif /* MANTID_GEOMETRY_RASTERIZETEST_H_ */
