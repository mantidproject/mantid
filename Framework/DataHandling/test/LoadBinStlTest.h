#ifndef LOAD_BINSTL_TEST_H_
#define LOAD_BINSTL_TEST_H_

#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/Sample.h"
#include "MantidDataHandling/LoadInstrument.h"
#include "MantidDataHandling/LoadBinStl.h"
#include "MantidGeometry/Objects/MeshObject.h"
#include "MantidKernel/OptionalBool.h"
#include "MantidTestHelpers/ComponentCreationHelper.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"
#include "MantidAPI/FileFinder.h"
#include <cxxtest/TestSuite.h>


using namespace Mantid;
using namespace Mantid::API;
using namespace Mantid::DataHandling;
using namespace Mantid::DataObjects;
using namespace Mantid::Geometry;


class LoadBinStlTest : public CxxTest::TestSuite {
public:

  static LoadBinStlTest *createSuite() {
    return new LoadBinStlTest();
  }
  static void destroySuite(LoadBinStlTest *suite) { delete suite; }

  void testInit() {


  }
void test_cube() {
    std::string path = FileFinder::Instance().getFullPath("cubeBin.stl");
    std::unique_ptr<LoadBinStl> Loader = std::make_unique<LoadBinStl>(path);
    auto cube = Loader->readStl();
    TS_ASSERT(cube->hasValidShape());
    TS_ASSERT_EQUALS(cube->numberOfVertices(), 8);
    TS_ASSERT_EQUALS(cube->numberOfTriangles(), 12);
    TS_ASSERT_DELTA(cube->volume(), 3000, 0.001);
  }

  void test_cylinder() {
    std::string path = FileFinder::Instance().getFullPath("cylinderBin.stl");
    std::unique_ptr<LoadBinStl> Loader = std::make_unique<LoadBinStl>(path);
    auto cylinder = Loader->readStl();
    TS_ASSERT(cylinder->hasValidShape());
    TS_ASSERT_EQUALS(cylinder->numberOfVertices(), 722);
    TS_ASSERT_EQUALS(cylinder->numberOfTriangles(), 1440);
    TS_ASSERT_DELTA(cylinder->volume(), 589, 1);
  }

  void test_tube() {
    std::string path = FileFinder::Instance().getFullPath("tubeBin.stl");
    std::unique_ptr<LoadBinStl> Loader = std::make_unique<LoadBinStl>(path);
    auto tube = Loader->readStl();
    TS_ASSERT(tube->hasValidShape());
    TS_ASSERT_EQUALS(tube->numberOfVertices(), 1080);
    TS_ASSERT_EQUALS(tube->numberOfTriangles(), 2160);
    TS_ASSERT_DELTA(tube->volume(), 7068, 1);
  }  
  //check that isBinaryStl returns false if the file contains an incomplete vertex
  void test_fail_invalid_vertex() {
    std::string path = FileFinder::Instance().getFullPath("invalid_vertexBin.stl");
    std::unique_ptr<LoadBinStl> Loader = std::make_unique<LoadBinStl>(path);
    TS_ASSERT(!(Loader->isBinarySTL()));
  }
  //check that isBinaryStl returns false if the file contains an incomplete triangle
  void test_fail_invalid_triangle() {
    std::string path = FileFinder::Instance().getFullPath("invalid_triangleBin.stl");
    std::unique_ptr<LoadBinStl> Loader = std::make_unique<LoadBinStl>(path);
    TS_ASSERT(!(Loader->isBinarySTL()));
  }
};
//add tests for isBinaryStl e.g. attempting to load an ascii .stl

#endif /* LOAD_BINSTL_TEST_H_ */