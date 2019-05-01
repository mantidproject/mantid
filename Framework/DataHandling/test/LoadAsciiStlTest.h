// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef LOAD_ASCIISTL_TEST_H_
#define LOAD_ASCIISTL_TEST_H_

#include "MantidAPI/FileFinder.h"
#include "MantidDataHandling/LoadAsciiStl.h"
#include "MantidGeometry/Objects/MeshObject.h"
#include <cxxtest/TestSuite.h>

using namespace Mantid;
using namespace Mantid::API;
using namespace Mantid::DataHandling;

using namespace Mantid::Geometry;

class LoadAsciiStlTest : public CxxTest::TestSuite {
public:
  static LoadAsciiStlTest *createSuite() { return new LoadAsciiStlTest(); }
  static void destroySuite(LoadAsciiStlTest *suite) { delete suite; }

  void test_cube() {
    std::string path = FileFinder::Instance().getFullPath("cube.stl");
    auto Loader = LoadAsciiStl(path, units);
    auto cube = Loader.readStl();
    assert_shape_matches(cube, 8, 12, 3000, 0.001);
  }

  void test_cylinder() {
    std::string path = FileFinder::Instance().getFullPath("cylinder.stl");
    auto Loader = LoadAsciiStl(path, units);
    auto cylinder = Loader.readStl();
    assert_shape_matches(cylinder, 722, 1440, 589, 1);
  }

  void test_tube() {
    std::string path = FileFinder::Instance().getFullPath("tube.stl");
    auto Loader = LoadAsciiStl(path, units);
    auto tube = Loader.readStl();
    assert_shape_matches(tube, 1080, 2160, 7068, 1);
  }

  void test_fail_invalid_stl_keyword() {
    loadFailureTest("invalid_keyword.stl");
  }

  void test_fail_invalid_stl_vertex() { loadFailureTest("invalid_vertex.stl"); }

  void test_fail_invalid_stl_triangle() {
    loadFailureTest("invalid_triangle.stl");
  }

  void loadFailureTest(const std::string filename) {
    std::string path = FileFinder::Instance().getFullPath(filename);
    auto Loader = LoadAsciiStl(path, units);
    TS_ASSERT_THROWS_ANYTHING(Loader.readStl());
  }

  void test_return_false_on_binary_stl() {
    std::string path = FileFinder::Instance().getFullPath("cubeBin.stl");
    auto Loader = LoadAsciiStl(path, units);
    TS_ASSERT(!(Loader.isAsciiSTL(path)));
  }

  void test_return_false_on_invalid_solid() {
    std::string path = FileFinder::Instance().getFullPath("invalid_solid.stl");
    auto Loader = LoadAsciiStl(path, units);
    TS_ASSERT(!(Loader.isAsciiSTL(path)));
  }

private:
  void assert_shape_matches(std::unique_ptr<Geometry::MeshObject> &shape,
                            int vertices, int triangles, double volume,
                            double delta) {
    TS_ASSERT(shape->hasValidShape());
    TS_ASSERT_EQUALS(shape->numberOfVertices(), vertices);
    TS_ASSERT_EQUALS(shape->numberOfTriangles(), triangles);
    TS_ASSERT_DELTA(shape->volume(), volume, delta);
  }
  const ScaleUnits units = ScaleUnits::metres;
};

#endif /* LOAD_ASCIISTL_TEST_H_ */