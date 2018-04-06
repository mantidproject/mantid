#ifndef LOAD_SHAPETEST_H_
#define LOAD_SHAPETEST_H_

#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/Sample.h"
#include "MantidDataHandling/LoadSampleShape.h"
#include "MantidDataHandling/LoadInstrument.h"
#include "MantidGeometry/Objects/MeshObject.h"
#include "MantidKernel/OptionalBool.h"
#include "MantidTestHelpers/ComponentCreationHelper.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"
#include <cxxtest/TestSuite.h>

using namespace Mantid;
using namespace Mantid::API;
using namespace Mantid::DataHandling;
using namespace Mantid::Geometry;

class LoadSampleShapeTest : public CxxTest::TestSuite {
public:
  static LoadSampleShapeTest *createSuite() {
    return new LoadSampleShapeTest();
  }
  static void destroySuite(LoadSampleShapeTest *suite) { delete suite; }

  void testName() { TS_ASSERT_EQUALS(loadShape.name(), "LoadSampleShape"); }

  void testVersion() { TS_ASSERT_EQUALS(loadShape.version(), 1); }

  void testInit() {

    TS_ASSERT_THROWS_NOTHING(loadShape.initialize());
    TS_ASSERT(loadShape.isInitialized());

    TSM_ASSERT_EQUALS("should be 3 properties here", 3,
                      (size_t)(loadShape.getProperties().size()));
  }

  void testConfidence() {
    LoadSampleShape testLoadSampleShape;
    testLoadSampleShape.initialize();
    testLoadSampleShape.setPropertyValue("Filename", "cube.stl");
    std::string path = testLoadSampleShape.getPropertyValue("Filename");
    auto *descriptor = new Kernel::FileDescriptor(path);
    TS_ASSERT_EQUALS(90, testLoadSampleShape.confidence(*descriptor));
    delete descriptor;
  }

  void testExec_2WS() {
    LoadSampleShape testLoadSampleShape;
    testLoadSampleShape.initialize();
    testLoadSampleShape.setPropertyValue("Filename", "cube.stl");
    prepareWorkspaces(testLoadSampleShape, "InputWS", "OutputWS");
    TS_ASSERT_THROWS_NOTHING(testLoadSampleShape.execute());
    TS_ASSERT(testLoadSampleShape.isExecuted());
    clearWorkspaces("InputWS", "OutputWS");
  }

  void test_output_workspace_has_MeshObject_2WS() {
    loadMeshObject("InputWS", "OutputWS", "cube.stl");
    clearWorkspaces("InputWS", "OutputWS");
  }

  void test_output_workspace_has_MeshObject_1WS() {
    loadMeshObject("InputWS", "InputWS", "cube.stl");
    clearWorkspaces("InputWS", "InputWS");
  }

  void test_cube() {
    auto cube = loadMeshObject("InputWS", "InputWS", "cube.stl");
    TS_ASSERT(cube->hasValidShape());
    TS_ASSERT_EQUALS(cube->numberOfVertices(), 8);
    TS_ASSERT_EQUALS(cube->numberOfTriangles(), 12);
    TS_ASSERT_DELTA(cube->volume(), 3000, 0.001);
    clearWorkspaces("InputWS", "InputWS");
  }

  void test_cylinder() {
    auto cylinder = loadMeshObject("InputWS", "InputWS", "cylinder.stl");
    TS_ASSERT(cylinder->hasValidShape());
    TS_ASSERT_EQUALS(cylinder->numberOfVertices(), 722);
    TS_ASSERT_EQUALS(cylinder->numberOfTriangles(), 1440);
    TS_ASSERT_DELTA(cylinder->volume(), 589, 1);
    clearWorkspaces("InputWS", "InputWS");
  }

  void test_tube() {
    auto tube = loadMeshObject("InputWS", "InputWS", "tube.stl");
    TS_ASSERT(tube->hasValidShape());
    TS_ASSERT_EQUALS(tube->numberOfVertices(), 1080);
    TS_ASSERT_EQUALS(tube->numberOfTriangles(), 2160);
    TS_ASSERT_DELTA(tube->volume(), 7068, 1);
    clearWorkspaces("InputWS", "InputWS");
  }

private:
  // Create workspaces and add them to algorithm properties
  void prepareWorkspaces(LoadSampleShape &alg, const std::string &inputWS,
                         const std::string &outputWS) {
    auto inWs =
        WorkspaceCreationHelper::create2DWorkspaceWithFullInstrument(10, 4);
    Mantid::API::AnalysisDataService::Instance().add(inputWS, inWs);
    alg.setPropertyValue("InputWorkspace", inputWS);
    alg.setPropertyValue("OutputWorkspace", outputWS);
  }

  void clearWorkspaces(const std::string &inputWS,
                       const std::string &outputWS) {
    TS_ASSERT_THROWS_NOTHING(
        Mantid::API::AnalysisDataService::Instance().remove(inputWS));
    if (outputWS != inputWS) {
      TS_ASSERT_THROWS_NOTHING(
          Mantid::API::AnalysisDataService::Instance().remove(outputWS));
    }
  }

  const MeshObject *loadMeshObject(const std::string &inputWS,
                                   const std::string &outputWS,
                                   const std::string filename) {
    LoadSampleShape testLoadSampleShape;
    testLoadSampleShape.initialize();
    testLoadSampleShape.setPropertyValue("Filename", filename);
    prepareWorkspaces(testLoadSampleShape, inputWS, outputWS);
    TS_ASSERT_THROWS_NOTHING(testLoadSampleShape.execute());
    TS_ASSERT(testLoadSampleShape.isExecuted());
    return getMeshObject(outputWS);
  }

  const MeshObject *getMeshObject(const std::string &outputWS) {
    MatrixWorkspace_sptr ws = getWorkspace(outputWS);
    Sample s;
    TS_ASSERT_THROWS_NOTHING(s = ws->sample());
    auto &obj = s.getShape();
    auto mObj = dynamic_cast<const MeshObject *>(&obj);
    TSM_ASSERT_DIFFERS("Shape is not a mesh object", mObj, nullptr);
    return mObj;
  }

  MatrixWorkspace_sptr getWorkspace(const std::string &outputWS) {
    AnalysisDataServiceImpl &dataStore = AnalysisDataService::Instance();
    if (dataStore.doesExist(outputWS)) {
      TS_ASSERT_EQUALS(dataStore.doesExist(outputWS), true);
      Workspace_sptr output;
      TS_ASSERT_THROWS_NOTHING(output = dataStore.retrieve(outputWS));
      MatrixWorkspace_sptr outputWorkspace =
          boost::dynamic_pointer_cast<MatrixWorkspace>(output);
      return outputWorkspace;
    } else {
      return nullptr;
    }
  }

  LoadSampleShape loadShape;
};

class LoadShapeTestPerformance : public CxxTest::TestSuite {
public:
  void setUp() override {
    auto inWs =
        WorkspaceCreationHelper::create2DWorkspaceWithFullInstrument(10, 4);
    Mantid::API::AnalysisDataService::Instance().add(inWsName, inWs);
    for (int i = 0; i < numberOfIterations; ++i) {
      loadAlgPtrs.emplace_back(setupAlg());
    }
  }

  void testLoadSampleShapePerformance() {
    for (auto alg : loadAlgPtrs) {
      TS_ASSERT_THROWS_NOTHING(alg->execute());
    }
  }

  void tearDown() override {
    for (int i = 0; i < numberOfIterations; i++) {
      delete loadAlgPtrs[i];
      loadAlgPtrs[i] = nullptr;
    }
    API::AnalysisDataService::Instance().remove(outWsName);
    API::AnalysisDataService::Instance().remove(inWsName);
  }

private:
  std::vector<LoadSampleShape *> loadAlgPtrs;
  const int numberOfIterations = 5;
  const std::string inWsName = "InWS";
  const std::string outWsName = "OutWS";

  LoadSampleShape *setupAlg() {
    LoadSampleShape *loadAlg = new LoadSampleShape();
    loadAlg->initialize();

    loadAlg->setPropertyValue("Filename", "tube.stl");
    loadAlg->setProperty("InputWorkspace", inWsName);
    loadAlg->setProperty("OutputWorkspace", outWsName);

    loadAlg->setRethrows(true);
    return loadAlg;
  }
};
#endif /* LOAD_SHAPETEST_H_ */
