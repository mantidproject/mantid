#ifndef MANTID_DATAHANDLING_SAVESESANSTEST_H_
#define MANTID_DATAHANDLING_SAVESESANSTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/Sample.h"
#include "MantidDataHandling/LoadSESANS.h"
#include "MantidDataHandling/SaveSESANS.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"

#include <boost/algorithm/string/predicate.hpp>
#include <Poco/File.h>
#include <Poco/TemporaryFile.h>
#include <fstream>
#include <cmath>

using Mantid::DataHandling::SaveSESANS;
using namespace Mantid;

class SaveSESANSTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static SaveSESANSTest *createSuite() { return new SaveSESANSTest(); }
  static void destroySuite(SaveSESANSTest *suite) { delete suite; }
  
  void test_init() {
    TS_ASSERT_THROWS_NOTHING(testAlg.initialize());
    TS_ASSERT(testAlg.isInitialized());
    testAlg.setRethrows(true);
    TS_ASSERT_THROWS_NOTHING(testAlg.setProperty("Filename", "dummy.ses"));
    TS_ASSERT_THROWS_NOTHING(testAlg.setProperty("ThetaZMax", 0.09));
    TS_ASSERT_THROWS_NOTHING(testAlg.setProperty("ThetaYMax", 0.09));
    TS_ASSERT_THROWS_NOTHING(testAlg.setProperty("EchoConstant", "1"));
    TS_ASSERT_THROWS_NOTHING(testAlg.setProperty("Sample", "Sample set in algorithm"));
  }
  
  void test_rejectTooManySpectra() {
    auto ws = WorkspaceCreationHelper::create2DWorkspace(10, 10);
    TS_ASSERT_THROWS_NOTHING(testAlg.setProperty("InputWorkspace", ws));

    // Should throw, as we can't save more than one histogram
    TS_ASSERT_THROWS(testAlg.execute(), std::runtime_error);
  }
  
  void test_exec() {
    // Set up workspace
    // X = [1 to 11], Y = [2] * 10, E = [sqrt(2)] * 10
    auto ws = WorkspaceCreationHelper::create2DWorkspaceBinned(1, 10, 1, 1);

    // Set workspace attributes
    ws->setTitle("Sample workspace");

    testAlg.setProperty("InputWorkspace", ws);
	testAlg.setProperty("Sample", "Sample set in SaveSESANSTest");

    // Make a temporary file
    Poco::TemporaryFile tempFile;
    const std::string &tempFileName = tempFile.path();
    TS_ASSERT_THROWS_NOTHING(testAlg.setProperty("Filename", tempFileName));

    // Execute the algorithm
    TS_ASSERT_THROWS_NOTHING(testAlg.execute());

    // Get absolute path to the output file
    std::string outputPath = testAlg.getPropertyValue("Filename");

    // Make sure we can load the output file with no problems
    DataHandling::LoadSESANS loader;
    loader.initialize();
    std::string outWSName = "outWS";

    TS_ASSERT_THROWS_NOTHING(loader.setProperty("Filename", outputPath));
    TS_ASSERT_THROWS_NOTHING(loader.setProperty("OutputWorkspace", outWSName));
    TS_ASSERT_THROWS_NOTHING(loader.execute());
    TS_ASSERT(Poco::File(outputPath).exists());

    // Check the file against original data - load it into a workspace
    API::Workspace_sptr loadedWS;
    TS_ASSERT_THROWS_NOTHING(
        loadedWS = API::AnalysisDataService::Instance().retrieve(outWSName));
    API::MatrixWorkspace_sptr data =
        boost::dynamic_pointer_cast<API::MatrixWorkspace>(loadedWS);
    // Check titles were set
    TS_ASSERT_EQUALS(data->getTitle(), "Sample workspace");
    TS_ASSERT_EQUALS(data->sample().getName(), "Sample set in workspace");

    // Check (a small sample of) the values we wrote are correct
    TS_ASSERT_EQUALS(static_cast<int>(data->getNumberHistograms()), 1);
    auto xValues = data->x(0);
    auto yValues = data->y(0);
    auto eValues = data->e(0);

    TS_ASSERT_EQUALS(static_cast<int>(xValues.size()), 10);
    TS_ASSERT_EQUALS(static_cast<int>(yValues.size()), 10);
    TS_ASSERT_EQUALS(static_cast<int>(eValues.size()), 10);

    // Check the actual values match
    double tolerance(1e-05);
    for (size_t i = 0; i < xValues.size(); i++) {
      // X values are 0.5 higher than they were when we set them, as we set the
      // bin edges
      // but are now dealing with bin middles
      TS_ASSERT_DELTA(static_cast<double>(i) + 1.5, xValues[i], tolerance);
      TS_ASSERT_DELTA(yValues[i], 2.0, tolerance);
      TS_ASSERT_DELTA(eValues[i], SQRT_2, tolerance);
    }

    // Clean up the file
    TS_ASSERT_THROWS_NOTHING(Poco::File(outputPath).remove());
    TS_ASSERT(!Poco::File(outputPath).exists());
  }

private:
  SaveSESANS testAlg;
  const double SQRT_2 = std::sqrt(2.0);
};

#endif /* MANTID_DATAHANDLING_SAVESESANSTEST_H_ */
