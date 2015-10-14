#ifndef LOADQKKTEST_H_
#define LOADQKKTEST_H_

//-----------------
// Includes
//-----------------
#include "MantidDataHandling/LoadQKK.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidKernel/ConfigService.h"
#include <cxxtest/TestSuite.h>
#include <Poco/Path.h>

using Mantid::Kernel::ConfigServiceImpl;
using namespace Mantid::API;
using namespace Mantid::DataObjects;

class LoadQKKTest : public CxxTest::TestSuite {
public:
  static LoadQKKTest *createSuite() { return new LoadQKKTest(); }
  static void destroySuite(LoadQKKTest *suite) { delete suite; }

  // A sample file is in the repository
  LoadQKKTest() {}

  void test_File_Check_Confidence() {
    Mantid::DataHandling::LoadQKK loader;
    loader.initialize();
    loader.setPropertyValue("Filename",
                            "QKK0029775.nx.hdf"); // find the full path
    Mantid::Kernel::NexusDescriptor descr(loader.getPropertyValue("Filename"));
    TS_ASSERT_EQUALS(80, loader.confidence(descr));
  }

  void testInit() {
    Mantid::DataHandling::LoadQKK load;
    TS_ASSERT_THROWS_NOTHING(load.initialize());
    TS_ASSERT(load.isInitialized());
  }

  void testLoad() {
    std::string wsName = "QKK0029775";
    Mantid::DataHandling::LoadQKK load;
    TS_ASSERT_THROWS_NOTHING(load.initialize());
    load.setPropertyValue("Filename", "QKK0029775.nx.hdf");
    load.setPropertyValue("OutputWorkspace", wsName);
    load.execute();
    TS_ASSERT(load.isExecuted());

    Workspace_sptr ws;
    TS_ASSERT_THROWS_NOTHING(
        ws = AnalysisDataService::Instance().retrieve(wsName));
    Workspace2D_sptr data = boost::dynamic_pointer_cast<Workspace2D>(ws);
    TS_ASSERT(data);
    TS_ASSERT_EQUALS(data->getNumberHistograms(), 192 * 192);
    Mantid::Geometry::IDetector_const_sptr det;
    for (size_t i = 0; i < data->getNumberHistograms(); ++i) {
      TS_ASSERT_THROWS_NOTHING(det = data->getDetector(i));
    }
  }
};

#endif // LOADQKKTEST_H_
