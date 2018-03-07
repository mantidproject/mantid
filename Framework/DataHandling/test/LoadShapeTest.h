#ifndef LOAD_SHAPETEST_H_
#define LOAD_SHAPETEST_H_

#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/Sample.h"
#include "MantidDataHandling/LoadShape.h"
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


class LoadShapeTest : public CxxTest::TestSuite {
public:
  static LoadShapeTest *createSuite() {
    return new LoadShapeTest();
  }
  static void destroySuite(LoadShapeTest *suite) { delete suite; }


  void testName() { TS_ASSERT_EQUALS(loadShape.name(), "LoadShape"); }

  void testVersion() { TS_ASSERT_EQUALS(loadShape.version(), 1); }

  void testInit() {

    TS_ASSERT_THROWS_NOTHING(loadShape.initialize());
    TS_ASSERT(loadShape.isInitialized());

    TSM_ASSERT_EQUALS("should be 3 properties here", 3,
                      (size_t)(loadShape.getProperties().size()));
  }

  void testConfidence() {
    LoadShape testLoadShape;
    testLoadShape.initialize();
    testLoadShape.setPropertyValue("Filename", "cube.stl");
    std::string path = testLoadShape.getPropertyValue("Filename");
    auto *descriptor = new Kernel::FileDescriptor(path);
    TS_ASSERT_EQUALS(90, testLoadShape.confidence(*descriptor));
    delete descriptor;
  }

  void testExec_2WS() {
    LoadShape testLoadShape;
    testLoadShape.initialize();
    testLoadShape.setPropertyValue("Filename", "cube.stl");
    prepareWorkspaces(testLoadShape, "InputWS", "OutputWS");
    TS_ASSERT_THROWS_NOTHING(testLoadShape.execute());
    TS_ASSERT(testLoadShape.isExecuted());
    clearWorkspaces("InputWS", "OutputWS");
  }

  void testExec_1WS() {
    LoadShape testLoadShape;
    testLoadShape.initialize();
    testLoadShape.setPropertyValue("Filename", "cube.stl");
    prepareWorkspaces(testLoadShape, "InputWS", "InputWS");
    TS_ASSERT_THROWS_NOTHING(testLoadShape.execute());
    TS_ASSERT(testLoadShape.isExecuted());
    clearWorkspaces("InputWS", "InputWS");
  }

  void test_output_workspace_has_MeshObject_2WS() {
    LoadShape testLoadShape;
    testLoadShape.initialize();
    testLoadShape.setPropertyValue("Filename", "cube.stl");
    prepareWorkspaces(testLoadShape, "InputWS", "OutputWS");
    TS_ASSERT_THROWS_NOTHING(testLoadShape.execute());
    TS_ASSERT(testLoadShape.isExecuted());
    getMeshObject("OutputWS");
    clearWorkspaces("InputWS", "OutputWS");
  }

  void test_output_workspace_has_MeshObject_1WS() {
    LoadShape testLoadShape;
    testLoadShape.initialize();
    testLoadShape.setPropertyValue("Filename", "cube.stl");
    prepareWorkspaces(testLoadShape, "InputWS", "InputWS");
    TS_ASSERT_THROWS_NOTHING(testLoadShape.execute());
    TS_ASSERT(testLoadShape.isExecuted());
    getMeshObject("InputWS");
    clearWorkspaces("InputWS", "InputWS");
  }

private:

  // Create workspaces and add them to algorithm properties
  void prepareWorkspaces(LoadShape &alg, const std::string &inputWS, const std::string &outputWS) {
    auto inWs = WorkspaceCreationHelper::create2DWorkspaceWithFullInstrument(10, 4);
    Mantid::API::AnalysisDataService::Instance().add(
      inputWS, inWs);
    alg.setPropertyValue("InputWorkspace", inputWS);
    if (outputWS != inputWS) {
      auto outWs = WorkspaceCreationHelper::create2DWorkspaceWithFullInstrument(10, 4);
      Mantid::API::AnalysisDataService::Instance().add(
        outputWS, outWs);
    }
    alg.setPropertyValue("OutputWorkspace", outputWS);
  }

  void clearWorkspaces(const std::string &inputWS, const std::string &outputWS) {
    TS_ASSERT_THROWS_NOTHING(
      Mantid::API::AnalysisDataService::Instance().remove(inputWS));
    if (outputWS != inputWS) {
      TS_ASSERT_THROWS_NOTHING(
        Mantid::API::AnalysisDataService::Instance().remove(outputWS));
    }
  }

  const MeshObject* getMeshObject(const std::string &outputWS) {
    MatrixWorkspace_sptr ws = getWorkspace("InputWS");
    Sample s;
    TS_ASSERT_THROWS_NOTHING(s = ws->sample());
    auto &obj = s.getShape();
    auto mObj = dynamic_cast<const MeshObject*>(&obj);
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
    }
    else {
      return nullptr;
    }
  }

  LoadShape loadShape; 
};
#endif /* LOAD_SHAPETEST_H_ */
