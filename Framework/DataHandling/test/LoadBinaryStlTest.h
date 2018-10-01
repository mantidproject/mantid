#ifndef LOAD_BinaryStl_TEST_H_
#define LOAD_BinaryStl_TEST_H_

#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/FileFinder.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/Sample.h"
#include "MantidDataHandling/LoadBinaryStl.h"
#include "MantidDataHandling/LoadInstrument.h"
#include "MantidGeometry/Objects/MeshObject.h"
#include "MantidKernel/OptionalBool.h"
#include "MantidTestHelpers/ComponentCreationHelper.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"
#include <cxxtest/TestSuite.h>

using namespace Mantid;
using namespace Mantid::API;
using namespace Mantid::DataHandling;
using namespace Mantid::DataObjects;
using namespace Mantid::Geometry;

class LoadBinaryStlTest : public CxxTest::TestSuite {
public:
  static LoadBinaryStlTest *createSuite() { return new LoadBinaryStlTest(); }
  static void destroySuite(LoadBinaryStlTest *suite) { delete suite; }

  void testInit() {}
  void test_cube() {
    std::string path = FileFinder::Instance().getFullPath("cubeBin.stl");
    std::unique_ptr<LoadBinaryStl> loader =
        std::make_unique<LoadBinaryStl>(path);
    auto cube = loader->readStl();
    TS_ASSERT(cube->hasValidShape());
    TS_ASSERT_EQUALS(cube->numberOfVertices(), 8);
    TS_ASSERT_EQUALS(cube->numberOfTriangles(), 12);
    TS_ASSERT_DELTA(cube->volume(), 3000, 0.001);
  }

  void test_cylinder() {
    std::string path = FileFinder::Instance().getFullPath("cylinderBin.stl");
    std::unique_ptr<LoadBinaryStl> loader =
        std::make_unique<LoadBinaryStl>(path);
    auto cylinder = loader->readStl();
    TS_ASSERT(cylinder->hasValidShape());
    TS_ASSERT_EQUALS(cylinder->numberOfVertices(), 722);
    TS_ASSERT_EQUALS(cylinder->numberOfTriangles(), 1440);
    TS_ASSERT_DELTA(cylinder->volume(), 589, 1);
  }

  void test_tube() {
    std::string path = FileFinder::Instance().getFullPath("tubeBin.stl");
    std::unique_ptr<LoadBinaryStl> loader =
        std::make_unique<LoadBinaryStl>(path);
    auto tube = loader->readStl();
    TS_ASSERT(tube->hasValidShape());
    TS_ASSERT_EQUALS(tube->numberOfVertices(), 1080);
    TS_ASSERT_EQUALS(tube->numberOfTriangles(), 2160);
    TS_ASSERT_DELTA(tube->volume(), 7068, 1);
  }
  // check that isBinaryStl returns false if the file contains an incomplete
  // vertex
  void test_fail_invalid_vertex() {
    std::string path =
        FileFinder::Instance().getFullPath("invalid_vertexBin.stl");
    std::unique_ptr<LoadBinaryStl> loader =
        std::make_unique<LoadBinaryStl>(path);
    TS_ASSERT(!(loader->isBinarySTL()));
  }
  // check that isBinaryStl returns false if the file contains an incomplete
  // triangle
  void test_fail_invalid_triangle() {
    std::string path =
        FileFinder::Instance().getFullPath("invalid_triangleBin.stl");
    std::unique_ptr<LoadBinaryStl> loader =
        std::make_unique<LoadBinaryStl>(path);
    TS_ASSERT(!(loader->isBinarySTL()));
  }

  void test_fail_ascii_stl() {
    std::string path = FileFinder::Instance().getFullPath("cube.stl");
    std::unique_ptr<LoadBinaryStl> loader =
        std::make_unique<LoadBinaryStl>(path);
    TS_ASSERT(!(loader->isBinarySTL()));
  }
};

#endif /* LOAD_BinaryStl_TEST_H_ */