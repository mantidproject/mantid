#ifndef MANTID_MDALGORITHMS_CONVERTCWPDMDTOSPECTRATEST_H_
#define MANTID_MDALGORITHMS_CONVERTCWPDMDTOSPECTRATEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidMDAlgorithms/ConvertCWPDMDToSpectra.h"
#include "MantidMDAlgorithms/LoadMD.h"

#include "MantidAPI/IMDEventWorkspace.h"

using Mantid::MDAlgorithms::ConvertCWPDMDToSpectra;
using Mantid::MDAlgorithms::LoadMD;
using namespace Mantid::API;

class ConvertCWPDMDToSpectraTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static ConvertCWPDMDToSpectraTest *createSuite() {
    return new ConvertCWPDMDToSpectraTest();
  }
  static void destroySuite(ConvertCWPDMDToSpectraTest *suite) { delete suite; }

  void test_Init() {
    ConvertCWPDMDToSpectra alg;
    alg.initialize();
    TS_ASSERT(alg.isInitialized());
  }

  /** Unit test to reduce/bin the HB2A data
   * @brief test_ReduceHB2AData
   */
  void test_ReduceHB2AData() {
    // Load data
    LoadMD loader1;
    loader1.initialize();
    loader1.setProperty("Filename", "data_md.nxs");
    loader1.setProperty("OutputWorkspace", "DataMDWS");
    loader1.execute();
    IMDEventWorkspace_const_sptr datamdws =
        boost::dynamic_pointer_cast<const IMDEventWorkspace>(
            AnalysisDataService::Instance().retrieve("DataMDWS"));
    TS_ASSERT(datamdws);

    LoadMD loader2;
    loader2.initialize();
    loader2.setProperty("Filename", "monitor_md.nxs");
    loader2.setProperty("OutputWorkspace", "MonitorMDWS");
    loader2.execute();
    IMDEventWorkspace_sptr monitormdws =
        boost::dynamic_pointer_cast<IMDEventWorkspace>(
            AnalysisDataService::Instance().retrieve("MonitorMDWS"));
    TS_ASSERT(monitormdws);

    // Init
    ConvertCWPDMDToSpectra alg;
    alg.initialize();

    // Set properties
    TS_ASSERT_THROWS_NOTHING(
        alg.setPropertyValue("InputWorkspace", datamdws->name()));
    TS_ASSERT_THROWS_NOTHING(
        alg.setPropertyValue("InputMonitorWorkspace", monitormdws->name()));
    TS_ASSERT_THROWS_NOTHING(
        alg.setPropertyValue("BinningParams", "0, 0.05, 120."));
    TS_ASSERT_THROWS_NOTHING(
        alg.setProperty("LinearInterpolateZeroCounts", true));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("OutputWorkspace", "ReducedData"));

    // Execute
    TS_ASSERT_THROWS_NOTHING(alg.execute());
    TS_ASSERT(alg.isExecuted());

    // Get ouput
    MatrixWorkspace_sptr outws = boost::dynamic_pointer_cast<MatrixWorkspace>(
        AnalysisDataService::Instance().retrieve("ReducedData"));
  }
};

#endif /* MANTID_MDALGORITHMS_CONVERTCWPDMDTOSPECTRATEST_H_ */
