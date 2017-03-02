#ifndef MANTID_DATAHANDLING_LOADILLREFLECTOMETRYTEST_H_
#define MANTID_DATAHANDLING_LOADILLREFLECTOMETRYTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidDataHandling/LoadILLReflectometry.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/MatrixWorkspace.h"

using namespace Mantid::API;
using Mantid::DataHandling::LoadILLReflectometry;

class LoadILLReflectometryTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static LoadILLReflectometryTest *createSuite() {
    return new LoadILLReflectometryTest();
  }
  static void destroySuite(LoadILLReflectometryTest *suite) { delete suite; }

  LoadILLReflectometryTest() : m_dataFile("ILLD17-161876-Ni.nxs") {}

  void test_Init() {
    LoadILLReflectometry loader;
    TS_ASSERT_THROWS_NOTHING(loader.initialize())
    TS_ASSERT(loader.isInitialized())
  }

  void testName() {
    LoadILLReflectometry loader;
    TS_ASSERT_EQUALS(loader.name(), "LoadILLReflectometry");
  }

  void test_exec() {
    // Name of the output workspace.
    std::string outWSName("LoadILLReflectometryTest_OutputWS");

    LoadILLReflectometry loader;
    TS_ASSERT_THROWS_NOTHING(loader.initialize())
    TS_ASSERT(loader.isInitialized())
    TS_ASSERT_THROWS_NOTHING(loader.setPropertyValue("Filename", m_dataFile));
    TS_ASSERT_THROWS_NOTHING(
        loader.setPropertyValue("OutputWorkspace", outWSName));
    TS_ASSERT_THROWS_NOTHING(loader.execute(););
    TS_ASSERT(loader.isExecuted());

    MatrixWorkspace_sptr output =
        AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(outWSName);
    TS_ASSERT(output);

    TS_ASSERT_EQUALS(output->getNumberHistograms(), 256 + 2);

    double channelWidth =
        output->run().getPropertyValueAsType<double>("channel_width");
    TS_ASSERT_EQUALS(channelWidth, 57.0);

    double analyserAngle =
        output->run().getPropertyValueAsType<double>("dan.value");
    TS_ASSERT_EQUALS(analyserAngle, 3.1909999847412109);

    if (!output)
      return;

    // Remove workspace from the data service.
    AnalysisDataService::Instance().clear();
  }

private:
  std::string m_dataFile;
};

#endif /* MANTID_DATAHANDLING_LOADILLREFLECTOMETRYTEST_H_ */
