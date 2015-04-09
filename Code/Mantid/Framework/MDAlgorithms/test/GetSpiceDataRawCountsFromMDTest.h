#ifndef MANTID_MDALGORITHMS_GETSPICEDATARAWCOUNTSFROMMDTEST_H_
#define MANTID_MDALGORITHMS_GETSPICEDATARAWCOUNTSFROMMDTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidMDAlgorithms/GetSpiceDataRawCountsFromMD.h"
#include "MantidDataHandling/LoadSpiceAscii.h"
#include "MantidMDAlgorithms/ConvertSpiceDataToRealSpace.h"
#include "MantidAPI/MatrixWorkspace.h"

using Mantid::MDAlgorithms::GetSpiceDataRawCountsFromMD;
using Mantid::DataHandling::LoadSpiceAscii;
using Mantid::MDAlgorithms::ConvertSpiceDataToRealSpace;

using namespace Mantid::API;

class GetSpiceDataRawCountsFromMDTest : public CxxTest::TestSuite
{
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static GetSpiceDataRawCountsFromMDTest *createSuite() { return new GetSpiceDataRawCountsFromMDTest(); }
  static void destroySuite( GetSpiceDataRawCountsFromMDTest *suite ) { delete suite; }

  //----------------------------------------------------------------------------------------------
  /** Test for initialization
   */
  void test_Init() {
    GetSpiceDataRawCountsFromMD tetalg;
    tetalg.initialize();
    TS_ASSERT(tetalg.isInitialized());

    // Create test workspaces
    createTestWorkspaces();
  }

  //----------------------------------------------------------------------------------------------
  void test_PtMode() {
    GetSpiceDataRawCountsFromMD testalg;
    testalg.initialize();

    TS_ASSERT_THROWS_NOTHING(testalg.setProperty("InputWorkspace", m_dataMD));
    TS_ASSERT_THROWS_NOTHING(
        testalg.setProperty("MonitorWorkspace", m_monitorMD));
    TS_ASSERT_THROWS_NOTHING(testalg.setProperty("Mode", "Pt."));
    TS_ASSERT_THROWS_NOTHING(testalg.setProperty("Pt", 30));
    TS_ASSERT_THROWS_NOTHING(
        testalg.setPropertyValue("OutputWorkspace", "Run1CountsMatrixWS"));
    TS_ASSERT_THROWS_NOTHING(
        testalg.setProperty("NormalizeByMonitorCounts", true));

    testalg.execute();
    TS_ASSERT(testalg.isExecuted());

    MatrixWorkspace_sptr outws = boost::dynamic_pointer_cast<MatrixWorkspace>(
        AnalysisDataService::Instance().retrieve("Run1CountsMatrixWS"));
    TS_ASSERT(outws);

    const Mantid::MantidVec &vecX = outws->readX(0);
    const Mantid::MantidVec &vecY = outws->readY(0);
    TS_ASSERT_EQUALS(vecX.size(), 44);
    TS_ASSERT_EQUALS(vecY.size(), 44);
    double twotheta1 = 8.9;
    // double twotheta44 = 10;
    TS_ASSERT_DELTA(vecX.front(), twotheta1, 0.0001);
    // TS_ASSERT_DELTA(vecX.back(), twotheta44, 0.0001);

    double y1 = 135.;
    double y35 = 82.;
    double monitor = 31906.000;
    TS_ASSERT_DELTA(vecY[1], y1 / monitor, 0.0001);
    TS_ASSERT_DELTA(vecY[35], y35 / monitor, 0.0001);
  }

  //----------------------------------------------------------------------------------------------
  void test_DetMode2Theta() {
    GetSpiceDataRawCountsFromMD testalg;
    testalg.initialize();

    TS_ASSERT_THROWS_NOTHING(testalg.setProperty("InputWorkspace", m_dataMD));
    TS_ASSERT_THROWS_NOTHING(
        testalg.setProperty("MonitorWorkspace", m_monitorMD));
    TS_ASSERT_THROWS_NOTHING(testalg.setProperty("Mode", "Detector"));
    TS_ASSERT_THROWS_NOTHING(testalg.setProperty("DetectorID", 1));
    TS_ASSERT_THROWS_NOTHING(
        testalg.setPropertyValue("OutputWorkspace", "Run1CountsMatrixWS"));
    TS_ASSERT_THROWS_NOTHING(
        testalg.setProperty("NormalizeByMonitorCounts", false));

    testalg.execute();
    TS_ASSERT(testalg.isExecuted());

    MatrixWorkspace_sptr outws = boost::dynamic_pointer_cast<MatrixWorkspace>(
        AnalysisDataService::Instance().retrieve("Run1CountsMatrixWS"));
    TS_ASSERT(outws);

    const Mantid::MantidVec &vecX = outws->readX(0);
    const Mantid::MantidVec &vecY = outws->readY(0);
    TS_ASSERT_EQUALS(vecX.size(), 61);
    TS_ASSERT_EQUALS(vecY.size(), 61);
    double twotheta1 = 6.000; // integer as Pt. number
    double twotheta61 = 12.000;
    TS_ASSERT_DELTA(vecX.front(), twotheta1, 0.0001);
    TS_ASSERT_DELTA(vecX.back(), twotheta61, 0.0001);

    double y1 = 124.;
    double y35 = 107.;
    double monitor = 1.0;
    TS_ASSERT_DELTA(vecY[1], y1 / monitor, 0.0001);
    TS_ASSERT_DELTA(vecY[35], y35 / monitor, 0.0001);
  }

  //----------------------------------------------------------------------------------------------
  void test_DetModePt() {
    GetSpiceDataRawCountsFromMD testalg;
    testalg.initialize();

    TS_ASSERT_THROWS_NOTHING(testalg.setProperty("InputWorkspace", m_dataMD));
    TS_ASSERT_THROWS_NOTHING(
        testalg.setProperty("MonitorWorkspace", m_monitorMD));
    TS_ASSERT_THROWS_NOTHING(testalg.setProperty("Mode", "Detector"));
    TS_ASSERT_THROWS_NOTHING(testalg.setProperty("DetectorID", 1));
    TS_ASSERT_THROWS_NOTHING(testalg.setProperty("XLabel", "Pt."))
    TS_ASSERT_THROWS_NOTHING(
        testalg.setPropertyValue("OutputWorkspace", "Run1CountsMatrixWS"));
    TS_ASSERT_THROWS_NOTHING(
        testalg.setProperty("NormalizeByMonitorCounts", true));

    testalg.execute();
    TS_ASSERT(testalg.isExecuted());

    MatrixWorkspace_sptr outws = boost::dynamic_pointer_cast<MatrixWorkspace>(
        AnalysisDataService::Instance().retrieve("Run1CountsMatrixWS"));
    TS_ASSERT(outws);

    const Mantid::MantidVec &vecX = outws->readX(0);
    const Mantid::MantidVec &vecY = outws->readY(0);
    TS_ASSERT_EQUALS(vecX.size(), 61);
    TS_ASSERT_EQUALS(vecY.size(), 61);
    double pt1 = 1.000; // integer as Pt. number
    double pt61 = 61.000;
    TS_ASSERT_DELTA(vecX.front(), pt1, 0.0001);
    TS_ASSERT_DELTA(vecX.back(), pt61, 0.0001);

    double y1 = 124.;
    double y35 = 107.;
    double monitor1 = 31937.000;
    double monitor35 = 32024.000;
    TS_ASSERT_DELTA(vecY[1], y1 / monitor1, 0.0001);
    TS_ASSERT_DELTA(vecY[35], y35 / monitor35, 0.0001);
  }

  //----------------------------------------------------------------------------------------------
  void test_SampleLogMode() {
    GetSpiceDataRawCountsFromMD testalg;
    testalg.initialize();

    TS_ASSERT_THROWS_NOTHING(testalg.setProperty("InputWorkspace", m_dataMD));
    TS_ASSERT_THROWS_NOTHING(
        testalg.setProperty("MonitorWorkspace", m_monitorMD));
    TS_ASSERT_THROWS_NOTHING(testalg.setProperty("Mode", "Sample Log"));
    TS_ASSERT_THROWS_NOTHING(testalg.setProperty("SampleLogName", "2theta"));
    TS_ASSERT_THROWS_NOTHING(
        testalg.setPropertyValue("OutputWorkspace", "Run1CountsMatrixWS"));
    TS_ASSERT_THROWS_NOTHING(
        testalg.setProperty("NormalizeByMonitorCounts", false));

    testalg.execute();
    TS_ASSERT(testalg.isExecuted());

    MatrixWorkspace_sptr outws = boost::dynamic_pointer_cast<MatrixWorkspace>(
        AnalysisDataService::Instance().retrieve("Run1CountsMatrixWS"));
    TS_ASSERT(outws);

    const Mantid::MantidVec &vecX = outws->readX(0);
    const Mantid::MantidVec &vecY = outws->readY(0);
    TS_ASSERT_EQUALS(vecX.size(), 61);
    TS_ASSERT_EQUALS(vecY.size(), 61);
    double pt1 = 1.000; // integer as Pt. number
    double pt61 = 61.000;
    TS_ASSERT_DELTA(vecX.front(), pt1, 0.0001);
    TS_ASSERT_DELTA(vecX.back(), pt61, 0.0001);

    double y1 = 6.1;
    double y35 = 9.5;
    TS_ASSERT_DELTA(vecY[1], y1, 0.0001);
    TS_ASSERT_DELTA(vecY[35], y35, 0.0001);
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

    return;
  }
};

#endif /* MANTID_MDALGORITHMS_GETSPICEDATARAWCOUNTSFROMMDTEST_H_ */
