// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_DATAHANDLING_APPENDGEOMETRYTOSNSNEXUSTEST_H_
#define MANTID_DATAHANDLING_APPENDGEOMETRYTOSNSNEXUSTEST_H_

#include "MantidKernel/System.h"
#include "MantidKernel/Timer.h"
#include <cxxtest/TestSuite.h>
#include <Poco/File.h>
#include <Poco/Path.h>

#include "MantidDataHandling/AppendGeometryToSNSNexus.h"

using namespace Mantid;
using namespace Mantid::DataHandling;
using namespace Mantid::API;

namespace {
constexpr auto NXS_FILENAME = "HYS_11092_event.nxs";
}

class AppendGeometryToSNSNexusTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static AppendGeometryToSNSNexusTest *createSuite() {
    return new AppendGeometryToSNSNexusTest();
  }
  static void destroySuite(AppendGeometryToSNSNexusTest *suite) {
    delete suite;
  }

  void test_Init() {
    AppendGeometryToSNSNexus alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
  }

  void test_exec() {
    //    // Name of the output workspace.
    //    std::string outWSName("AppendGeometryToSNSNexusTest_OutputWS");

    AppendGeometryToSNSNexus alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
    // TODO: Get a better test file.
    // Changed to use HYS_11088_event.nxs to test motors
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("Filename", NXS_FILENAME));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("MakeCopy", true));
    TS_ASSERT_THROWS_NOTHING(alg.execute(););
    TS_ASSERT(alg.isExecuted());

    std::string fullpath(Poco::Path::temp() + NXS_FILENAME);

    if (Poco::File(fullpath).exists()) {
      Poco::File(fullpath).remove();
    }

    // Retrieve the workspace from data service. TODO: Change to your desired
    // type
    //    Workspace_sptr ws;
    //    TS_ASSERT_THROWS_NOTHING( ws =
    //    AnalysisDataService::Instance().retrieveWS<Workspace>(outWSName) );
    //    TS_ASSERT(ws);
    //    if (!ws) return;

    // TODO: Check the results

    // Remove workspace from the data service.
    // AnalysisDataService::Instance().remove(outWSName);
  }
};

#endif /* MANTID_DATAHANDLING_APPENDGEOMETRYTOSNSNEXUSTEST_H_ */
