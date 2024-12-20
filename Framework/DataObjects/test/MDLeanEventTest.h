// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidDataObjects/MDLeanEvent.h"
#include "MantidKernel/Timer.h"
#include <cxxtest/TestSuite.h>

#include <boost/scoped_array.hpp>

using namespace Mantid;
using namespace Mantid::DataObjects;

class MDLeanEventTest : public CxxTest::TestSuite {
public:
  using EventAccessType = DataObjects::EventAccessor;
  void test_Constructors() {
    MDLeanEvent<3> a;
    TS_ASSERT_EQUALS(a.getNumDims(), 3);
    TS_ASSERT_EQUALS(a.getSignal(), 1.0);
    TS_ASSERT_EQUALS(a.getErrorSquared(), 1.0);

    MDLeanEvent<4> b(2.5, 1.5);
    TS_ASSERT_EQUALS(b.getNumDims(), 4);
    TS_ASSERT_EQUALS(b.getSignal(), 2.5);
    TS_ASSERT_EQUALS(b.getErrorSquared(), 1.5);

    TS_ASSERT_EQUALS(sizeof(a), sizeof(coord_t) * 3 + 8);
    TS_ASSERT_EQUALS(sizeof(b), sizeof(coord_t) * 4 + 8);
  }

  void test_ConstructorsWithCoords() {
    // Fixed-size array
    coord_t coords[3] = {0.125, 1.25, 2.5};
    MDLeanEvent<3> a(2.5, 1.5, coords);
    TS_ASSERT_EQUALS(a.getSignal(), 2.5);
    TS_ASSERT_EQUALS(a.getErrorSquared(), 1.5);
    TS_ASSERT_EQUALS(a.getCenter(0), 0.125);
    TS_ASSERT_EQUALS(a.getCenter(1), 1.25);
    TS_ASSERT_EQUALS(a.getCenter(2), 2.5);

    // Dynamic array
    boost::scoped_array<coord_t> coords2(new coord_t[5]);
    coords2[0] = 1.0;
    coords2[1] = 2.0;
    coords2[2] = 3.0;

    MDLeanEvent<3> b(2.5, 1.5, coords2.get());
    TS_ASSERT_EQUALS(b.getSignal(), 2.5);
    TS_ASSERT_EQUALS(b.getErrorSquared(), 1.5);
    TS_ASSERT_EQUALS(b.getCenter(0), 1);
    TS_ASSERT_EQUALS(b.getCenter(1), 2);
    TS_ASSERT_EQUALS(b.getCenter(2), 3);
  }

  void test_Coord() {
    MDLeanEvent<3> a;
    TS_ASSERT_EQUALS(a.getNumDims(), 3);

    a.setCenter(0, 0.125);
    TS_ASSERT_EQUALS(a.getCenter(0), 0.125);

    a.setCenter(1, 1.25);
    TS_ASSERT_EQUALS(a.getCenter(0), 0.125);
    TS_ASSERT_EQUALS(a.getCenter(1), 1.25);

    a.setCenter(2, 2.5);
    TS_ASSERT_EQUALS(a.getCenter(0), 0.125);
    TS_ASSERT_EQUALS(a.getCenter(1), 1.25);
    TS_ASSERT_EQUALS(a.getCenter(2), 2.5);
    TS_ASSERT_EQUALS(a.getCenter()[0], 0.125);
    TS_ASSERT_EQUALS(a.getCenter()[1], 1.25);
    TS_ASSERT_EQUALS(a.getCenter()[2], 2.5);
  }

  void test_setCenter_array() {
    MDLeanEvent<3> a;
    coord_t coords[3] = {0.125, 1.25, 2.5};
    a.setCoords(coords);
    TS_ASSERT_EQUALS(a.getCenter(0), 0.125);
    TS_ASSERT_EQUALS(a.getCenter(1), 1.25);
    TS_ASSERT_EQUALS(a.getCenter(2), 2.5);
  }

  /** Note: the copy constructor is not explicitely written but rather is filled
   * in by the compiler */
  void test_CopyConstructor() {
    coord_t coords[3] = {0.125, 1.25, 2.5};
    MDLeanEvent<3> b(2.5, 1.5, coords);
    MDLeanEvent<3> a(b);
    TS_ASSERT_EQUALS(a.getNumDims(), 3);
    TS_ASSERT_EQUALS(a.getSignal(), 2.5);
    TS_ASSERT_EQUALS(a.getErrorSquared(), 1.5);
    TS_ASSERT_EQUALS(a.getCenter(0), 0.125);
    TS_ASSERT_EQUALS(a.getCenter(1), 1.25);
    TS_ASSERT_EQUALS(a.getCenter(2), 2.5);
  }

  void test_getError() {
    MDLeanEvent<3> a(2.0, 4.0);
    TS_ASSERT_EQUALS(a.getSignal(), 2.0);
    TS_ASSERT_EQUALS(a.getError(), 2.0);
  }

  void test_retrieve_coordinates() {
    constexpr size_t ND(4);

    morton_index::MDSpaceBounds<ND> bounds;
    // clang-format off
    bounds <<
           0.0f, 2.0f,
        0.0f, 10.0f,
        3.0f, 7.0f,
        5.0f, 15.0f;
    // clang-format on

    morton_index::MDCoordinate<ND> floatCoord;
    floatCoord << 1.5f, 10.0f, 5.0f, 5.0f;

    MDLeanEvent<ND> event(0.0f, 0.0f, &floatCoord[0]);
    using EventAccess = MDLeanEvent<ND>::template AccessFor<MDLeanEventTest>;

    EventAccess::convertToIndex(event, bounds);
    EventAccess::convertToCoordinates(event, bounds);

    TS_ASSERT_EQUALS(event.getCenter(0), 1.5);
    TS_ASSERT_EQUALS(event.getCenter(1), 10);
    TS_ASSERT_EQUALS(event.getCenter(2), 5);
    TS_ASSERT_EQUALS(event.getCenter(3), 5);
  }

  void test_retrieve_coordinates_3d() {
    constexpr size_t ND(3);

    morton_index::MDSpaceBounds<ND> bounds;
    // clang-format off
    bounds <<
           0.0f, 2.0f,
        0.0f, 10.0f,
        3.0f, 7.0f;
    // clang-format on

    morton_index::MDCoordinate<ND> floatCoord;
    floatCoord << 1.5f, 10.0f, 5.0f;

    MDLeanEvent<ND> event(0.0f, 0.0f, &floatCoord[0]);
    using EventAccess = MDLeanEvent<ND>::template AccessFor<MDLeanEventTest>;

    EventAccess::convertToIndex(event, bounds);
    EventAccess::convertToCoordinates(event, bounds);

    TS_ASSERT_EQUALS(event.getCenter(0), 1.5);
    TS_ASSERT_EQUALS(event.getCenter(1), 10);
    TS_ASSERT_EQUALS(event.getCenter(2), 5);
  }
};
