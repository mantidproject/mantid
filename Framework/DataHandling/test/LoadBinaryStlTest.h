// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once
#include "MantidAPI/FileFinder.h"
#include "MantidDataHandling/LoadBinaryStl.h"
#include "MantidGeometry/Objects/MeshObject.h"
#include <cxxtest/TestSuite.h>
using namespace Mantid;
using namespace Mantid::API;
using namespace Mantid::DataHandling;
using namespace Mantid::Geometry;

class LoadBinaryStlTest : public CxxTest::TestSuite {
public:
  static LoadBinaryStlTest *createSuite() { return new LoadBinaryStlTest(); }
  static void destroySuite(LoadBinaryStlTest *suite) { delete suite; }

  void test_loading_cube_stl() {
    std::string path = FileFinder::Instance().getFullPath("cubeBin.stl");
    auto loader = LoadBinaryStl(path, units);
    auto cube = loader.readShape();
    assert_shape_matches(cube, 8, 12, 3000, 0.001);
  }

  void test_loading_cube_stl_cm() {
    std::string path = FileFinder::Instance().getFullPath("cubeBin.stl");
    const ScaleUnits cm = ScaleUnits::centimetres;
    auto loader = LoadBinaryStl(path, cm);
    auto cube = loader.readShape();
    assert_shape_matches(cube, 8, 12, 0.003, 0.00001);
  }

  void test_loading_cylinder_stl() {
    std::string path = FileFinder::Instance().getFullPath("cylinderBin.stl");
    auto loader = LoadBinaryStl(path, units);
    auto cylinder = loader.readShape();
    assert_shape_matches(cylinder, 722, 1440, 589, 1);
  }

  void test_loading_tube_stl() {
    std::string path = FileFinder::Instance().getFullPath("tubeBin.stl");
    auto loader = LoadBinaryStl(path, units);
    auto tube = loader.readShape();
    assert_shape_matches(tube, 1080, 2160, 7068, 1);
  }
  // check that isBinaryStl returns false if the file contains an incomplete
  // vertex
  void test_fail_invalid_vertex() {
    std::string path = FileFinder::Instance().getFullPath("invalid_vertexBin.stl");
    auto loader = LoadBinaryStl(path, units);
    TS_ASSERT(!(loader.isBinarySTL(path)));
  }
  // check that isBinaryStl returns false if the file contains an incomplete
  // triangle
  void test_fail_invalid_triangle() {
    std::string path = FileFinder::Instance().getFullPath("invalid_triangleBin.stl");
    auto loader = LoadBinaryStl(path, units);
    TS_ASSERT(!(loader.isBinarySTL(path)));
  }

  void test_fail_ascii_stl() {
    std::string path = FileFinder::Instance().getFullPath("cube.stl");
    auto loader = LoadBinaryStl(path, units);
    TS_ASSERT(!(loader.isBinarySTL(path)));
  }

  void test_loading_large_stl() {
    std::string path = FileFinder::Instance().getFullPath("SI-4200-610.stl");
    auto loader = LoadBinaryStl(path, units);
    auto LargeFile = loader.readShape();
    assert_shape_matches(LargeFile, 174388, 424694, 21218, 1);
  }

private:
  void assert_shape_matches(std::unique_ptr<Geometry::MeshObject> &shape, int vertices, int triangles, double volume,
                            double delta) {
    TS_ASSERT(shape->hasValidShape());
    TS_ASSERT_EQUALS(shape->numberOfVertices(), vertices);
    TS_ASSERT_EQUALS(shape->numberOfTriangles(), triangles);
    TS_ASSERT_DELTA(shape->volume(), volume, delta);
  }

  const ScaleUnits units = ScaleUnits::metres;
};
