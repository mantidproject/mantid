#ifndef MANTID_DATAHANDLING_LOADSWANSTEST_H_
#define MANTID_DATAHANDLING_LOADSWANSTEST_H_

#include "MantidDataObjects/EventWorkspace.h"
#include "MantidDataHandling/LoadSwans.h"
#include "MantidAPI/AnalysisDataService.h"

#include <cxxtest/TestSuite.h>
#include <Poco/TemporaryFile.h>

using Mantid::DataHandling::LoadSwans;

using namespace Mantid::API;
using namespace Mantid::DataObjects;
using namespace Mantid::Kernel;
using namespace Mantid::DataHandling;

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
    ofs << content << std::endl;
    ofs.close();
    TSM_ASSERT("File has not been created.", tmpFile.exists());
    TSM_ASSERT("File is not a file.", tmpFile.isFile());
    tmpFile.keepUntilExit();
    return tmpFile.path();
  }
  std::string getAutoRecordFile() {
    std::string content(
        "RUN	IPTS	Title	Notes	Sample	ITEM	StartTime	"
        "Duration	ProtonCharge	TotalCounts	Monitor1	"
        "Monitor2	X	Y	Z	O	HROT	VROT	"
        "BandCentre	BandWidth	Frequency	Guide	IX	"
        "IY	"
        "IZ	IHA	IVA	Collimator	MTSDisplacement	"
        "MTSForce	"
        "MTSStrain	MTSStress	MTSAngle	MTSTorque	"
        "MTSLaser	MTSlaserstrain	MTSDisplaceoffset	"
        "MTSAngleceoffset	MTST1	MTST2	MTST3	MTST4	"
        "MTSHighTempStrain	FurnaceT	FurnaceOT	"
        "FurnacePower	"
        "VacT	VacOT	EuroTherm1Powder	EuroTherm1SP	"
        "EuroTherm1Temp	"
        "EuroTherm2Powder	EuroTherm2SP	EuroTherm2Temp\n80814	"
        "IPTS-16013	AgBh	60Hz 3.5 0.7, 6.5 degee roughly 40mm off the "
        "center	No sample	-1.0	2016-02-14 "
        "07:19:57.394594666-EST	17832.98152	20949204394768.457031	"
        "409176.000000	464773.000000	2160057.000000	11.405	26.502	"
        "-97.007	"
        "0.002	0.0	0.0	3.5	0.7	60.0	181.99	100.003	"
        "-0.125201	-1.458688	4.999108	4.999849	"
        "0.0	"
        "2.14570234012	2159.78457157	0.009598	44.9782698796	"
        "-2.2986984	11.19247925	-0.010656	2.23210635811	"
        "1.0817855	1.470575	26.526489	28.344247	"
        "541.609974	541.609974	0.00455622943451	0.0	"
        "0.0	"
        "0.0	0.0	0.0	0.0	0.0	0.0	0.0	0.0	0.0\n");

    Poco::TemporaryFile tmpFile;
    std::ofstream ofs(tmpFile.path().c_str());
    ofs << content << std::endl;
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
    const std::string filename = "SWANS_80814.dat";
    LoadSwans alg;

    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())

    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("FilenameData", filename));

    std::string metadataFilename = getMetadataFile();
    TS_ASSERT_THROWS_NOTHING(
        alg.setPropertyValue("FilenameMetaData", metadataFilename));

    std::string autoRecordFilename = getAutoRecordFile();
    TS_ASSERT_THROWS_NOTHING(
        alg.setPropertyValue("FilenameAutoRecord", autoRecordFilename));

    TS_ASSERT_THROWS_NOTHING(
        alg.setPropertyValue("OutputWorkspace", "Output_ws_name"));

    TS_ASSERT_THROWS_NOTHING(alg.execute());
    TS_ASSERT(alg.isExecuted());

    EventWorkspace_sptr outputWS =
        AnalysisDataService::Instance().retrieveWS<EventWorkspace>(
            "Output_ws_name");

    TS_ASSERT_EQUALS(outputWS->getNumberHistograms(), 16384);
    TS_ASSERT_EQUALS(outputWS->getNumberEvents(), 2505292);
    TS_ASSERT_EQUALS(
        outputWS->run().getPropertyValueAsType<double>("wavelength"), 3.5);
  }
};

#endif /* MANTID_DATAHANDLING_LOADSWANSTEST_H_ */
