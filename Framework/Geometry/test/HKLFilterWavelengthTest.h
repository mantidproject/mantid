// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_GEOMETRY_HKLFILTERWAVELENGTHTEST_H_
#define MANTID_GEOMETRY_HKLFILTERWAVELENGTHTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidGeometry/Crystal/HKLFilterWavelength.h"
#include "MantidGeometry/Crystal/OrientedLattice.h"

using namespace Mantid::Geometry;
using namespace Mantid::Kernel;

class HKLFilterWavelengthTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static HKLFilterWavelengthTest *createSuite() {
    return new HKLFilterWavelengthTest();
  }
  static void destroySuite(HKLFilterWavelengthTest *suite) { delete suite; }

  void testConstructor() {
    OrientedLattice ol(5.5, 6.1, 8.2);
    DblMatrix ub = ol.getUB();

    TS_ASSERT_THROWS_NOTHING(
        HKLFilterWavelength wavelengthFilter(ub, 0.9, 6.0));

    TS_ASSERT_THROWS(HKLFilterWavelength wavelengthFilter(ub, -0.9, 6.0),
                     const std::range_error &);
    TS_ASSERT_THROWS(HKLFilterWavelength wavelengthFilter(ub, 0.9, -6.0),
                     const std::range_error &);
    TS_ASSERT_THROWS(HKLFilterWavelength wavelengthFilter(ub, -0.9, -6.0),
                     const std::range_error &);
    TS_ASSERT_THROWS(HKLFilterWavelength wavelengthFilter(ub, 0.0, 6.0),
                     const std::range_error &);
    TS_ASSERT_THROWS(HKLFilterWavelength wavelengthFilter(ub, 0.9, 0.0),
                     const std::range_error &);
    TS_ASSERT_THROWS(HKLFilterWavelength wavelengthFilter(ub, 0.0, 0.0),
                     const std::range_error &);
  }

  void testDescription() {
    OrientedLattice ol(5.5, 6.1, 8.2);
    DblMatrix ub = ol.getUB();

    std::ostringstream reference;
    reference << "(" << 0.9 << " <= lambda <= " << 6.0 << ")";

    HKLFilterWavelength wlFilter(ub, 0.9, 6.0);
    TS_ASSERT_EQUALS(wlFilter.getDescription(), reference.str());
  }

  void testIsAllowed() {
    OrientedLattice ol(5.0, 6.0, 7.0);
    DblMatrix ub = ol.getUB();

    HKLFilterWavelength wlFilter(ub, 0.6, 2.0);

    TS_ASSERT_EQUALS(wlFilter.isAllowed(V3D(1, 3, 5)), true);
    TS_ASSERT_EQUALS(wlFilter.isAllowed(V3D(2, 4, 7)), true);

    // smaller than lower limit
    TS_ASSERT_EQUALS(wlFilter.isAllowed(V3D(6, 7, 9)), false);

    // larger than upper limit
    TS_ASSERT_EQUALS(wlFilter.isAllowed(V3D(1, 0, 2)), false);
  }
};

#endif /* MANTID_GEOMETRY_HKLFILTERWAVELENGTHTEST_H_ */
