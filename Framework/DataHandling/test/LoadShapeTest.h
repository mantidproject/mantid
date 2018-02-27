#ifndef LOAD_SHAPETEST_H_
#define LOAD_SHAPETEST_H_

#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidDataHandling/LoadShape.h"
#include "MantidDataHandling/LoadInstrument.h"
#include "MantidKernel/OptionalBool.h"
#include "MantidTestHelpers/ComponentCreationHelper.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"
#include <cxxtest/TestSuite.h>

using namespace Mantid;
using namespace Mantid::API;
using namespace Mantid::DataHandling;


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

    TSM_ASSERT_EQUALS("should be 5 properties here", 5,
                      (size_t)(loadShape.getProperties().size()));
  }

  void testConfidence() {
    LoadShape testLoad;
    testLoad.initialize();
    testLoad.setPropertyValue("Filename", "cube.stl");
    std::string path = testLoad.getPropertyValue("Filename");
    auto *descriptor = new Kernel::FileDescriptor(path);
    TS_ASSERT_EQUALS(90, testLoad.confidence(*descriptor));
    delete descriptor;
  }


private:
  LoadShape loadShape; 
};
#endif /* LOAD_SHAPETEST_H_ */
