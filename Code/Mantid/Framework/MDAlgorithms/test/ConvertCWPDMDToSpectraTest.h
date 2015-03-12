#ifndef MANTID_MDALGORITHMS_CONVERTCWPDMDTOSPECTRATEST_H_
#define MANTID_MDALGORITHMS_CONVERTCWPDMDTOSPECTRATEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidMDAlgorithms/ConvertCWPDMDToSpectra.h"
#include "MantidDataHandling/LoadSpiceAscii.h"
#include "MantidMDAlgorithms/ConvertSpiceDataToRealSpace.h"
#include "MantidKernel/TimeSeriesProperty.h"
#include "MantidAPI/IMDEventWorkspace.h"

using Mantid::MDAlgorithms::ConvertCWPDMDToSpectra;
using Mantid::DataHandling::LoadSpiceAscii;
using Mantid::MDAlgorithms::ConvertSpiceDataToRealSpace;
using namespace Mantid::API;
using namespace Mantid::Kernel;

class ConvertCWPDMDToSpectraTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static ConvertCWPDMDToSpectraTest *createSuite() {
    return new ConvertCWPDMDToSpectraTest();
  }
  static void destroySuite(ConvertCWPDMDToSpectraTest *suite) { delete suite; }

  //----------------------------------------------------------------------------------------------
  void test_Init() {
    ConvertCWPDMDToSpectra tetalg;
    tetalg.initialize();
    TS_ASSERT(tetalg.isInitialized());

    // Create test workspaces
    createTestWorkspaces();
  }

  //----------------------------------------------------------------------------------------------
  /** Unit test to reduce/bin the HB2A data
   * @brief test_ReduceHB2AData
   */
  void test_ReduceHB2AData() {
    // Init
    ConvertCWPDMDToSpectra alg;
    alg.initialize();

    // Set properties
    TS_ASSERT_THROWS_NOTHING(
        alg.setPropertyValue("InputWorkspace", m_dataMD->name()));
    TS_ASSERT_THROWS_NOTHING(
        alg.setPropertyValue("InputMonitorWorkspace", m_monitorMD->name()));
    TS_ASSERT_THROWS_NOTHING(
        alg.setPropertyValue("BinningParams", "0, 0.1, 120."));
    TS_ASSERT_THROWS_NOTHING(
        alg.setProperty("LinearInterpolateZeroCounts", false));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("ScaleFactor", 65000.0));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("OutputWorkspace", "ReducedData"));

    // Execute
    TS_ASSERT_THROWS_NOTHING(alg.execute());
    TS_ASSERT(alg.isExecuted());

    // Get ouput
    MatrixWorkspace_sptr outws = boost::dynamic_pointer_cast<MatrixWorkspace>(
        AnalysisDataService::Instance().retrieve("ReducedData"));
    TS_ASSERT(outws);

    // Check output
    TS_ASSERT_EQUALS(outws->getNumberHistograms(), 1);

    // X, Y and E values
    const Mantid::MantidVec &vecX = outws->readX(0);
    const Mantid::MantidVec &vecY = outws->readY(0);
    const Mantid::MantidVec &vecE = outws->readE(0);

    TS_ASSERT_DELTA(vecX.front(), 0.0, 0.0001);
    TS_ASSERT_DELTA(vecX.back(), 120.0, 0.0001);

    double y1101 = vecY[1101];
    double e1101 = vecE[1101];
    TS_ASSERT_DELTA(y1101, 186.0716, 0.0001);
    TS_ASSERT(e1101 > sqrt(y1101));
    TS_ASSERT(e1101 < sqrt(y1101 * 1.05));

    // Sample logs: temperature
    TimeSeriesProperty<double> *tempbseries =
        dynamic_cast<TimeSeriesProperty<double> *>(
            outws->run().getProperty("temp_b"));
    TS_ASSERT(tempbseries);
    TS_ASSERT_EQUALS(tempbseries->size(), 61);
    DateAndTime t0 = tempbseries->nthTime(0);
    DateAndTime t3 = tempbseries->nthTime(3);
    TS_ASSERT_EQUALS(
        (t3.totalNanoseconds() - t0.totalNanoseconds()) / 1000000000, 90);

    // Clean
    AnalysisDataService::Instance().remove("ReducedData");
  }

  //----------------------------------------------------------------------------------------------
  /** Unit test to reduce/bin the HB2A data with more options
   * @brief test_ReduceHB2AData
   */
  void test_ReduceHB2ADataMoreOptions() {
    // Init
    ConvertCWPDMDToSpectra alg;
    alg.initialize();

    // Set properties
    TS_ASSERT_THROWS_NOTHING(
        alg.setPropertyValue("InputWorkspace", m_dataMD->name()));
    TS_ASSERT_THROWS_NOTHING(
        alg.setPropertyValue("InputMonitorWorkspace", m_monitorMD->name()));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("UnitOutput", "dSpacing"));
    TS_ASSERT_THROWS_NOTHING(
        alg.setPropertyValue("BinningParams", "0.5, 0.01, 5.0"));
    TS_ASSERT_THROWS_NOTHING(
        alg.setProperty("LinearInterpolateZeroCounts", true));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("ScaleFactor", 10.0));
    TS_ASSERT_THROWS_NOTHING(
          alg.setProperty("NeutronWaveLength", 2.41));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("OutputWorkspace", "ReducedData"));

    // Execute
    TS_ASSERT_THROWS_NOTHING(alg.execute());
    TS_ASSERT(alg.isExecuted());

    // Get ouput
    MatrixWorkspace_sptr outws = boost::dynamic_pointer_cast<MatrixWorkspace>(
        AnalysisDataService::Instance().retrieve("ReducedData"));
    TS_ASSERT(outws);

    // Check unit and range of X
    std::string unit = outws->getAxis(0)->unit()->unitID();
    TS_ASSERT_EQUALS(unit, "dSpacing");

    const Mantid::MantidVec &vecX = outws->readX(0);
    TS_ASSERT_DELTA(vecX.front(), 0.5, 0.0001);
    TS_ASSERT_DELTA(vecX.back(), 5.00, 0.0001);

    // Check statistics

    // Clean
    AnalysisDataService::Instance().remove("ReducedData");
  }

  //----------------------------------------------------------------------------------------------
  /** Unit test to reduce/bin the HB2A data with more options
   * @brief test_ReduceHB2AData
   */
  void test_ReduceHB2ADataAutoBinBoundary() {
    // Init
    ConvertCWPDMDToSpectra alg;
    alg.initialize();

    // Set properties
    TS_ASSERT_THROWS_NOTHING(
        alg.setPropertyValue("InputWorkspace", m_dataMD->name()));
    TS_ASSERT_THROWS_NOTHING(
        alg.setPropertyValue("InputMonitorWorkspace", m_monitorMD->name()));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("UnitOutput", "dSpacing"));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("BinningParams", "0.01"));
    TS_ASSERT_THROWS_NOTHING(
        alg.setProperty("LinearInterpolateZeroCounts", true));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("ScaleFactor", 10.0));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("NeutronWaveLength", 2.41));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("OutputWorkspace", "ReducedData"));

    // Execute
    TS_ASSERT_THROWS_NOTHING(alg.execute());
    TS_ASSERT(alg.isExecuted());

    // Get ouput
    MatrixWorkspace_sptr outws = boost::dynamic_pointer_cast<MatrixWorkspace>(
        AnalysisDataService::Instance().retrieve("ReducedData"));
    TS_ASSERT(outws);

    // Check unit and range of X
    std::string unit = outws->getAxis(0)->unit()->unitID();
    TS_ASSERT_EQUALS(unit, "dSpacing");

    const Mantid::MantidVec &vecX = outws->readX(0);
    TS_ASSERT_DELTA(vecX.front(), 1.3416, 0.0001);
    TS_ASSERT_DELTA(vecX.back(), 23.0216, 0.001);

    // Check statistics

    // Clean
    AnalysisDataService::Instance().remove("ReducedData");

  }

  //----------------------------------------------------------------------------------------------
  /** Create workspaces for testing
   * @brief createTestWorkspaces
   */
  void createTestWorkspaces() {
    LoadSpiceAscii spcloader;
    spcloader.initialize();

    // Load HB2A spice file
    TS_ASSERT_THROWS_NOTHING(
        spcloader.setProperty("Filename", "HB2A_exp0231_scan0001.dat"));
    TS_ASSERT_THROWS_NOTHING(
        spcloader.setProperty("OutputWorkspace", "DataTable"));
    TS_ASSERT_THROWS_NOTHING(
        spcloader.setProperty("RunInfoWorkspace", "LogParentWS"));
    TS_ASSERT_THROWS_NOTHING(spcloader.setPropertyValue(
        "DateAndTimeLog", "date,MM/DD/YYYY,time,HH:MM:SS AM"));
    TS_ASSERT_THROWS_NOTHING(
        spcloader.setProperty("IgnoreUnlistedLogs", false));
    spcloader.execute();

    // Retrieve the workspaces as the inputs of ConvertSpiceDataToRealSpace
    ITableWorkspace_sptr datatablews =
        boost::dynamic_pointer_cast<ITableWorkspace>(
            AnalysisDataService::Instance().retrieve("DataTable"));
    TS_ASSERT(datatablews);

    MatrixWorkspace_sptr parentlogws =
        boost::dynamic_pointer_cast<MatrixWorkspace>(
            AnalysisDataService::Instance().retrieve("LogParentWS"));
    TS_ASSERT(parentlogws);

    // Set up ConvertSpiceDataToRealSpace
    ConvertSpiceDataToRealSpace loader;
    loader.initialize();

    loader.setProperty("InputWorkspace", datatablews);
    loader.setProperty("RunInfoWorkspace", parentlogws);
    loader.setProperty("Instrument", "HB2A");
    loader.setPropertyValue("OutputWorkspace", "HB2A_MD");
    loader.setPropertyValue("OutputMonitorWorkspace", "MonitorMDW");

    loader.execute();
    TS_ASSERT(loader.isExecuted());

    // Get on hold of MDWorkspaces for test
    m_dataMD = boost::dynamic_pointer_cast<IMDEventWorkspace>(
        AnalysisDataService::Instance().retrieve("HB2A_MD"));
    m_monitorMD = boost::dynamic_pointer_cast<IMDEventWorkspace>(
        AnalysisDataService::Instance().retrieve("MonitorMDW"));
    TS_ASSERT(m_dataMD);
    TS_ASSERT(m_monitorMD);

    // Clean
    AnalysisDataService::Instance().remove(datatablews->name());
    AnalysisDataService::Instance().remove(parentlogws->name());
  }

  //----------------------------------------------------------------------------------------------
  /** Clean the testing workspaces
   */
  void test_Clean() {
    AnalysisDataService::Instance().remove(m_dataMD->name());
    AnalysisDataService::Instance().remove(m_monitorMD->name());
  }

private:
  IMDEventWorkspace_sptr m_dataMD;
  IMDEventWorkspace_sptr m_monitorMD;
};

#endif /* MANTID_MDALGORITHMS_CONVERTCWPDMDTOSPECTRATEST_H_ */
