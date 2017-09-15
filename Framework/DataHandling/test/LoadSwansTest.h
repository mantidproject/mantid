#ifndef MANTID_DATAHANDLING_LOADSWANSTEST_H_
#define MANTID_DATAHANDLING_LOADSWANSTEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidDataHandling/LoadSwans.h"
#include "MantidAPI/Run.h"
#include <Poco/TemporaryFile.h>

using Mantid::DataHandling::LoadSwans;

class LoadSwansTest : public CxxTest::TestSuite {
private:
  /**
   * Creates the metadatafile
   * @return its path
   */
  std::string getMetadataFile() {
    //                  runnumber,   wavelength, chopper frequency, time offset,
    //                  ?, angle
    std::string content("80814.000000	3.500000	60.000000	"
                        "11200.715115	0.000000	6.500000");
    Poco::TemporaryFile tmpFile;
    std::ofstream ofs(tmpFile.path().c_str());
    ofs << content << '\n';
    ofs.close();
    TSM_ASSERT("File has not been created.", tmpFile.exists());
    TSM_ASSERT("File is not a file.", tmpFile.isFile());
    tmpFile.keepUntilExit();
    return tmpFile.path();
  }

public:
  // This pair of boilerplate methods prevent the suite being created
  // statically
  // This means the constructor isn't called when running other tests
  static LoadSwansTest *createSuite() { return new LoadSwansTest(); }
  static void destroySuite(LoadSwansTest *suite) { delete suite; }

  void test_Init() {
    LoadSwans alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
  }

  void test_exec() {
    // Data file
    const std::string filename = "SWANS_RUN80814.dat";
    LoadSwans alg;

    alg.setChild(true);
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())

    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("FilenameData", filename));
    std::string metadataFilename = getMetadataFile();
    TS_ASSERT_THROWS_NOTHING(
        alg.setPropertyValue("FilenameMetaData", metadataFilename));
    TS_ASSERT_THROWS_NOTHING(
        alg.setPropertyValue("OutputWorkspace", "Output_ws_name"));

    TS_ASSERT_THROWS_NOTHING(alg.execute());
    TS_ASSERT(alg.isExecuted());
    Mantid::DataObjects::EventWorkspace_sptr outputWS =
        alg.getProperty("OutputWorkspace");

    TS_ASSERT_EQUALS(outputWS->getNumberHistograms(), 16384);
    TS_ASSERT_EQUALS(outputWS->getNumberEvents(), 2505292);
    TS_ASSERT_EQUALS(
        outputWS->run().getPropertyValueAsType<double>("wavelength"), 3.5);
  }
};

#endif /* MANTID_DATAHANDLING_LOADSWANSTEST_H_ */
