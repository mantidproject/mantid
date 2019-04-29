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
    auto Loader = LoadAsciiStl(path);
    auto cube = Loader.readStl();
    TS_ASSERT(cube->hasValidShape());
    TS_ASSERT_EQUALS(cube->numberOfVertices(), 8);
    TS_ASSERT_EQUALS(cube->numberOfTriangles(), 12);
    TS_ASSERT_DELTA(cube->volume(), 3000, 0.001);
  }

  void test_cylinder() {
    std::string path = FileFinder::Instance().getFullPath("cylinder.stl");
    auto Loader = LoadAsciiStl(path);
    auto cylinder = Loader.readStl();
    TS_ASSERT(cylinder->hasValidShape());
    TS_ASSERT_EQUALS(cylinder->numberOfVertices(), 722);
    TS_ASSERT_EQUALS(cylinder->numberOfTriangles(), 1440);
    TS_ASSERT_DELTA(cylinder->volume(), 589, 1);
  }

  void test_tube() {
    std::string path = FileFinder::Instance().getFullPath("tube.stl");
    auto Loader = LoadAsciiStl(path);
    auto tube = Loader.readStl();
    TS_ASSERT(tube->hasValidShape());
    TS_ASSERT_EQUALS(tube->numberOfVertices(), 1080);
    TS_ASSERT_EQUALS(tube->numberOfTriangles(), 2160);
    TS_ASSERT_DELTA(tube->volume(), 7068, 1);
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
    auto Loader = LoadAsciiStl(path);
    TS_ASSERT_THROWS_ANYTHING(Loader.readStl());
  }

  void test_return_false_on_binary_stl() {
    std::string path = FileFinder::Instance().getFullPath("cubeBin.stl");
    auto Loader = LoadAsciiStl(path);
    TS_ASSERT(!(Loader.isAsciiSTL(path)));
  }

  void test_return_false_on_invalid_solid() {
    std::string path = FileFinder::Instance().getFullPath("invalid_solid.stl");
    auto Loader = LoadAsciiStl(path);
    TS_ASSERT(!(Loader.isAsciiSTL(path)));
  }
};

#endif /* LOAD_ASCIISTL_TEST_H_ */