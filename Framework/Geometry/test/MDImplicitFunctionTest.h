#ifndef MANTID_MDALGORITHMS_MDIMPLICITFUNCTIONTEST_H_
#define MANTID_MDALGORITHMS_MDIMPLICITFUNCTIONTEST_H_

#include "MantidGeometry/MDGeometry/MDImplicitFunction.h"
#include <boost/shared_array.hpp>
#include <cxxtest/TestSuite.h>

using namespace Mantid::Geometry;
using namespace Mantid;

class MDImplicitFunctionTest : public CxxTest::TestSuite {
public:
  void test_addPlane() {
    MDImplicitFunction f;

    coord_t normal[3] = {1234, 456, 678};
    coord_t point[3] = {1, 2, 3};
    MDPlane p1(3, normal, point);
    MDPlane p2(2, normal, point);
    MDPlane p3(3, normal, point);

    TS_ASSERT_EQUALS(f.getNumDims(), 0);
    TS_ASSERT_THROWS_NOTHING(f.addPlane(p1));
    TS_ASSERT_EQUALS(f.getNumDims(), 3);
    TS_ASSERT_THROWS_ANYTHING(f.addPlane(p2));
    TS_ASSERT_THROWS_NOTHING(f.addPlane(p3));
    TS_ASSERT_EQUALS(f.getNumDims(), 3);
  }

  /// Helper function for the 2D case
  bool try2Dpoint(MDImplicitFunction &f, double x, double y) {
    coord_t centers[2] = {static_cast<coord_t>(x), static_cast<coord_t>(y)};
    return f.isPointContained(centers);
  }

  void test_isPointContained() {
    MDImplicitFunction f;
    coord_t origin[2] = {0, 0};

    // Everything below a 45 degree line
    coord_t normal1[2] = {1, -1};
    f.addPlane(MDPlane(2, normal1, origin));

    // These points will be blocked by adding the second plane
    TS_ASSERT(try2Dpoint(f, -1, -2));
    TS_ASSERT(try2Dpoint(f, 0.2, -0.1));

    // Everything above y=0
    coord_t normal2[2] = {0, 1};
    f.addPlane(MDPlane(2, normal2, origin));

    // Are both planes doing the checking?
    TS_ASSERT(try2Dpoint(f, 0.2, 0.1));
    TS_ASSERT(!try2Dpoint(f, 0.2, -0.1));
    TS_ASSERT(!try2Dpoint(f, 0.2, 0.3));
    TS_ASSERT(try2Dpoint(f, 2000, 1999));
    TS_ASSERT(!try2Dpoint(f, -1, -2));
  }

  void test_everythingIsContained_ifNoPlanes() {
    MDImplicitFunction f;
    TS_ASSERT(try2Dpoint(f, -1, -2));
    TS_ASSERT(try2Dpoint(f, 0.2, -0.1));
    TS_ASSERT(try2Dpoint(f, 12, 33));
  }

  void test_isPointContained_vectorVersion() {
    MDImplicitFunction f;
    coord_t origin[2] = {0, 0};

    // Everything below a 45 degree line
    coord_t normal1[2] = {1, -1};
    f.addPlane(MDPlane(2, normal1, origin));

    // These points will be blocked by adding the second plane
    std::vector<coord_t> point;
    point.clear();
    point.push_back(-1);
    point.push_back(-2);
    TS_ASSERT(f.isPointContained(point));

    point.clear();
    point.push_back(2.5);
    point.push_back(3.5);
    TS_ASSERT(!f.isPointContained(point));
  }

  void add2DVertex(std::vector<std::vector<coord_t>> &vertexes, double x,
                   double y) {
    std::vector<coord_t> vertex;
    vertex.push_back(static_cast<coord_t>(x));
    vertex.push_back(static_cast<coord_t>(y));
    vertexes.push_back(vertex);
  }

  /** Make the 4 points that define a square/rectangle
   *
   * @param vertexes :: returns the vertex array
   * @return also a bare-array version of the same thing
   */
  boost::shared_array<coord_t>
  make2DVertexSquare(std::vector<std::vector<coord_t>> &vertexes, double x1,
                     double y1, double x2, double y2) {
    auto out = boost::shared_array<coord_t>(new coord_t[8]);
    vertexes.clear();
    add2DVertex(vertexes, x1, y1);
    out[0] = static_cast<coord_t>(x1);
    out[1] = static_cast<coord_t>(y1);
    add2DVertex(vertexes, x2, y1);
    out[2] = static_cast<coord_t>(x2);
    out[3] = static_cast<coord_t>(y1);
    add2DVertex(vertexes, x2, y2);
    out[4] = static_cast<coord_t>(x2);
    out[5] = static_cast<coord_t>(y2);
    add2DVertex(vertexes, x1, y2);
    out[6] = static_cast<coord_t>(x1);
    out[7] = static_cast<coord_t>(y2);

    return out;
  }

  MDImplicitFunction makeA2Dfunction() {
    MDImplicitFunction f;
    coord_t normal1[2] = {1, 0};
    coord_t origin1[2] = {0, 0};
    f.addPlane(MDPlane(2, normal1, origin1));
    coord_t normal2[2] = {-1, 0};
    coord_t origin2[2] = {1, 0};
    f.addPlane(MDPlane(2, normal2, origin2));
    coord_t normal3[2] = {0, 1};
    coord_t origin3[2] = {0, 0};
    f.addPlane(MDPlane(2, normal3, origin3));
    coord_t normal4[2] = {0, -1};
    coord_t origin4[2] = {0, 1};
    f.addPlane(MDPlane(2, normal4, origin4));
    return f;
  }

  /** Test both the vector and bare array version of isBoxTouching() */
  void test_isBoxTouching() {
    // Make an implicit function for a square from 0,0 to 1,1
    MDImplicitFunction f = makeA2Dfunction();

    // Couple of checks that it is indeed what we said
    TS_ASSERT(try2Dpoint(f, 0.5, 0.5));
    TS_ASSERT(!try2Dpoint(f, 1.5, 0.5));
    TS_ASSERT(!try2Dpoint(f, 0.5, 1.5));
    TS_ASSERT(!try2Dpoint(f, -0.5, 0.5));
    TS_ASSERT(!try2Dpoint(f, 0.5, -0.5));

    // 3 ways to do the same thing
    std::vector<std::vector<coord_t>> vertexes;
    boost::shared_array<coord_t> bareVertexes;

    bareVertexes = make2DVertexSquare(vertexes, 1.2, 0.2, 1.8, 0.8);
    TSM_ASSERT("Box that is to the right; not touching",
               !f.isBoxTouching(vertexes));
    TSM_ASSERT("Box that is to the right; not touching",
               !f.isBoxTouching(bareVertexes.get(), 4));
    TSM_ASSERT("Box that is to the right; not touching",
               f.boxContact(bareVertexes.get(), 4) ==
                   MDImplicitFunction::NOT_TOUCHING);

    bareVertexes = make2DVertexSquare(vertexes, 0.2, 1.2, 0.8, 1.8);
    TSM_ASSERT("Box that is above; not touching", !f.isBoxTouching(vertexes));
    TSM_ASSERT("Box that is above; not touching",
               !f.isBoxTouching(bareVertexes.get(), 4));
    TSM_ASSERT("Box that is above; not touching",
               f.boxContact(bareVertexes.get(), 4) ==
                   MDImplicitFunction::NOT_TOUCHING);

    bareVertexes = make2DVertexSquare(vertexes, 0.8, 0.8, 1.8, 1.8);
    TSM_ASSERT("Box with one corner touching in the upper right; touches",
               f.isBoxTouching(vertexes));
    TSM_ASSERT("Box with one corner touching in the upper right; touches",
               f.isBoxTouching(bareVertexes.get(), 4));
    TSM_ASSERT("Box with one corner touching in the upper right; touches",
               f.boxContact(bareVertexes.get(), 4) ==
                   MDImplicitFunction::TOUCHING);

    bareVertexes = make2DVertexSquare(vertexes, 0.8, 0.2, 1.8, 0.8);
    TSM_ASSERT("Box with both right-hand vertexes inside; touches",
               f.isBoxTouching(vertexes));
    TSM_ASSERT("Box with both right-hand vertexes inside; touches",
               f.isBoxTouching(bareVertexes.get(), 4));
    TSM_ASSERT("Box with both right-hand vertexes inside; touches",
               f.boxContact(bareVertexes.get(), 4) ==
                   MDImplicitFunction::TOUCHING);

    bareVertexes = make2DVertexSquare(vertexes, 0.8, -1.0, 1.8, +3.0);
    TSM_ASSERT("Box overlapping on the right side, no vertexes inside; touches",
               f.isBoxTouching(vertexes));
    TSM_ASSERT("Box overlapping on the right side, no vertexes inside; touches",
               f.isBoxTouching(bareVertexes.get(), 4));
    TSM_ASSERT("Box overlapping on the right side, no vertexes inside; touches",
               f.boxContact(bareVertexes.get(), 4) ==
                   MDImplicitFunction::TOUCHING);

    bareVertexes = make2DVertexSquare(vertexes, -2.0, -1.0, 0.2, +3.0);
    TSM_ASSERT("Box overlapping on the left side, no vertexes inside; touches",
               f.isBoxTouching(vertexes));
    TSM_ASSERT("Box overlapping on the left side, no vertexes inside; touches",
               f.isBoxTouching(bareVertexes.get(), 4));
    TSM_ASSERT("Box overlapping on the left side, no vertexes inside; touches",
               f.boxContact(bareVertexes.get(), 4) ==
                   MDImplicitFunction::TOUCHING);

    bareVertexes = make2DVertexSquare(vertexes, -2.0, 0.9, +3.0, +3.0);
    TSM_ASSERT("Box overlapping on the top side, no vertexes inside; touches",
               f.isBoxTouching(vertexes));
    TSM_ASSERT("Box overlapping on the top side, no vertexes inside; touches",
               f.isBoxTouching(bareVertexes.get(), 4));
    TSM_ASSERT("Box overlapping on the top side, no vertexes inside; touches",
               f.boxContact(bareVertexes.get(), 4) ==
                   MDImplicitFunction::TOUCHING);

    bareVertexes = make2DVertexSquare(vertexes, -2.0, -3.0, +3.0, +0.1);
    TSM_ASSERT(
        "Box overlapping on the bottom side, no vertexes inside; touches",
        f.isBoxTouching(vertexes));
    TSM_ASSERT(
        "Box overlapping on the bottom side, no vertexes inside; touches",
        f.isBoxTouching(bareVertexes.get(), 4));
    TSM_ASSERT(
        "Box overlapping on the bottom side, no vertexes inside; touches",
        f.boxContact(bareVertexes.get(), 4) == MDImplicitFunction::TOUCHING);

    bareVertexes = make2DVertexSquare(vertexes, -2.0, -2.0, 3.0, 3.0);
    TSM_ASSERT(
        "Box bigger than region on all directions, no vertexes inside; touches",
        f.isBoxTouching(vertexes));
    TSM_ASSERT(
        "Box bigger than region on all directions, no vertexes inside; touches",
        f.isBoxTouching(bareVertexes.get(), 4));
    TSM_ASSERT(
        "Box bigger than region on all directions, no vertexes inside; touches",
        f.boxContact(bareVertexes.get(), 4) == MDImplicitFunction::TOUCHING);

    bareVertexes = make2DVertexSquare(vertexes, 0.5, -10.0, 0.55, +10.0);
    TSM_ASSERT(
        "Narrow box passing through the middle, no vertexes inside; touches",
        f.isBoxTouching(vertexes));
    TSM_ASSERT(
        "Narrow box passing through the middle, no vertexes inside; touches",
        f.isBoxTouching(bareVertexes.get(), 4));
    TSM_ASSERT(
        "Narrow box passing through the middle, no vertexes inside; touches",
        f.boxContact(bareVertexes.get(), 4) == MDImplicitFunction::TOUCHING);

    bareVertexes = make2DVertexSquare(vertexes, 0.5, 1.1, 0.55, +10.0);
    TSM_ASSERT("Narrow box but above; not touching",
               !f.isBoxTouching(vertexes));
    TSM_ASSERT("Narrow box but above; not touching",
               !f.isBoxTouching(bareVertexes.get(), 4));
    TSM_ASSERT("Narrow box but above; not touching",
               f.boxContact(bareVertexes.get(), 4) ==
                   MDImplicitFunction::NOT_TOUCHING);

    bareVertexes = make2DVertexSquare(vertexes, 0.1, 0.1, 0.9, 0.9);
    TSM_ASSERT("Box that is completely within region; touches ",
               f.isBoxTouching(vertexes));
    TSM_ASSERT("Box that is completely within region; touches ",
               f.isBoxTouching(bareVertexes.get(), 4));
    TSM_ASSERT("Box that is completely within region; touches ",
               f.boxContact(bareVertexes.get(), 4) ==
                   MDImplicitFunction::CONTAINED);

    vertexes.clear();
    add2DVertex(vertexes, 3.0, -0.1);
    add2DVertex(vertexes, 4.0, -0.1);
    add2DVertex(vertexes, -0.1, 3.0);
    add2DVertex(vertexes, -0.1, 4.0);
    TSM_ASSERT("Weird trapezoid that intersects both the X and Y axes but does "
               "not actually overlap; reports a false positive.",
               f.isBoxTouching(vertexes));
  }
};

#endif /* MANTID_MDALGORITHMS_MDIMPLICITFUNCTIONTEST_H_ */
