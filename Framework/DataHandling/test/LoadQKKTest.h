#ifndef LOADQKKTEST_H_
#define LOADQKKTEST_H_

#include "MantidDataHandling/LoadQKK.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/SpectrumInfo.h"
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
    const auto &spectrumInfo = data->spectrumInfo();
    for (size_t i = 0; i < data->getNumberHistograms(); ++i) {
      TS_ASSERT_THROWS_NOTHING(spectrumInfo.detector(i));

      auto x = data->x(i);
      TS_ASSERT_EQUALS(x.size(), 2);
      TS_ASSERT_EQUALS(x[0], 1);
      TS_ASSERT_EQUALS(x[1], 2);

      auto y = data->y(i);
      TS_ASSERT_EQUALS(y[0], 0.0);

      auto e = data->e(i);
      TS_ASSERT_EQUALS(e[0], 0.0);
    }
  }
};

#endif // LOADQKKTEST_H_
