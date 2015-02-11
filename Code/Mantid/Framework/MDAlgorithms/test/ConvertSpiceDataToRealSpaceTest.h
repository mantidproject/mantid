#ifndef MANTID_MDALGORITHMS_CONVERTSPICEDATATOREALSPACETEST_H_
#define MANTID_MDALGORITHMS_CONVERTSPICEDATATOREALSPACETEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidMDAlgorithms/ConvertSpiceDataToRealSpace.h"
#include "MantidDataHandling/LoadSpiceAscii.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/ITableWorkspace.h"
#include "MantidDataHandling/LoadInstrument.h"
#include "MantidAPI/IMDIterator.h"
#include "MantidGeometry/IComponent.h"
#include "MantidKernel/Property.h"
#include "MantidKernel/TimeSeriesProperty.h"

using Mantid::MDAlgorithms::ConvertSpiceDataToRealSpace;
using Mantid::DataHandling::LoadInstrument;
using Mantid::DataHandling::LoadSpiceAscii;

using namespace Mantid::API;
using namespace Mantid::DataObjects;
using namespace Mantid::Kernel;

class ConvertSpiceDataToRealSpaceTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static ConvertSpiceDataToRealSpaceTest *createSuite() {
    return new ConvertSpiceDataToRealSpaceTest();
  }
  static void destroySuite(ConvertSpiceDataToRealSpaceTest *suite) {
    delete suite;
  }

  //-----------------------------------------------------------------------------------------------------
  void test_Init() {
    ConvertSpiceDataToRealSpace loader;
    loader.initialize();
    TS_ASSERT(loader.isInitialized());
  }

  //-----------------------------------------------------------------------------------------------------
  /** Test loading HB2A's IDF file
   * @brief test_HB2BIDF
   */
  void test_HB2AIDF() {
    MatrixWorkspace_sptr dataws =
        WorkspaceFactory::Instance().create("Workspace2D", 44, 2, 1);
    AnalysisDataService::Instance().addOrReplace("EmptyWS", dataws);

    LoadInstrument loader;
    loader.initialize();

    loader.setProperty("InstrumentName", "HB2A");
    loader.setProperty("Workspace", dataws);

    loader.execute();
    TS_ASSERT(loader.isExecuted());

    MatrixWorkspace_sptr outws = boost::dynamic_pointer_cast<MatrixWorkspace>(
        AnalysisDataService::Instance().retrieve("EmptyWS"));
    TS_ASSERT(outws);

    Mantid::Geometry::Instrument_const_sptr hb2a = outws->getInstrument();
    TS_ASSERT(hb2a);
    if (!hb2a)
      return;

    TS_ASSERT_EQUALS(hb2a->getName(), "HB2A");

    Mantid::Geometry::IComponent_const_sptr source = hb2a->getSource();
    TS_ASSERT(source);
    Mantid::Geometry::IComponent_const_sptr sample = hb2a->getSample();
    TS_ASSERT(sample);
    const Mantid::Kernel::V3D &samplepos = sample->getPos();
    TS_ASSERT_DELTA(samplepos.X(), 0.0, 0.00001);
    std::vector<Mantid::detid_t> detids = hb2a->getDetectorIDs();
    TS_ASSERT_EQUALS(detids.size(), 44);
  }

  //-----------------------------------------------------------------------------------------------------
  /** Test to load HB2A's SPICE data to MDWorkspaces
   * @brief test_LoadHB2AData
   */
  void test_LoadHB2AData() {
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

    // Get IMDEventWorkspace
    IMDEventWorkspace_sptr mdws =
        boost::dynamic_pointer_cast<IMDEventWorkspace>(
            AnalysisDataService::Instance().retrieve("HB2A_MD"));
    TS_ASSERT(mdws);

    // Check the IMDEvent workspace generated
    size_t numevents = mdws->getNEvents();
    TS_ASSERT_EQUALS(numevents, 44 * 61);

    IMDIterator *mditer = mdws->createIterator();
    TS_ASSERT_EQUALS(mditer->getNumEvents(), 44 * 61);

    double y0 = mditer->getInnerSignal(0);
    TS_ASSERT_DELTA(y0, 125.0, 0.1);
    double yl = mditer->getInnerSignal(44 * 61 - 1);
    TS_ASSERT_DELTA(yl, 76.0, 0.1);

    // Detector ID
    Mantid::detid_t detid0 = mditer->getInnerDetectorID(0);
    TS_ASSERT_EQUALS(detid0, 1);
    Mantid::detid_t detid1 = mditer->getInnerDetectorID(1);
    TS_ASSERT_EQUALS(detid1, 2);
    Mantid::detid_t detid43 = mditer->getInnerDetectorID(43);
    TS_ASSERT_EQUALS(detid43, 44);
    Mantid::detid_t detid44 = mditer->getInnerDetectorID(44);
    TS_ASSERT_EQUALS(detid44, 45);
    Mantid::detid_t detid61 = mditer->getInnerDetectorID(61);
    TS_ASSERT_EQUALS(detid61, 62);

    // Run index
    uint16_t run0 = mditer->getInnerRunIndex(0);
    TS_ASSERT_EQUALS(run0, 1);
    uint16_t run1 = mditer->getInnerRunIndex(44);
    TS_ASSERT_EQUALS(run1, 2);
    uint16_t runLast = mditer->getInnerRunIndex(44 * 61 - 1);
    TS_ASSERT_EQUALS(runLast, 61);

    // Verify the ldetector's position as 2theta angle
    Mantid::coord_t x0 = mditer->getInnerPosition(0, 0);
    Mantid::coord_t z0 = mditer->getInnerPosition(0, 2);
    double twotheta0 = atan(x0 / z0) * 180. / M_PI;
    TS_ASSERT_DELTA(twotheta0, 6.0, 0.0001);

    // Pt.=2
    Mantid::coord_t x1_0 = mditer->getInnerPosition(44, 0);
    Mantid::coord_t z1_0 = mditer->getInnerPosition(44, 2);
    double twotheta1_0 = atan(x1_0 / z1_0) * 180. / M_PI;
    TS_ASSERT_DELTA(twotheta1_0, 6.1, 0.0001);
    Mantid::coord_t x1_1 = mditer->getInnerPosition(45, 0);
    Mantid::coord_t z1_1 = mditer->getInnerPosition(45, 2);
    double twotheta1_1 = atan(x1_1 / z1_1) * 180. / M_PI;
    TS_ASSERT_DELTA(twotheta1_1, 6.1 + 2.642, 0.0001);

    // Pt.=61
    Mantid::coord_t x60_0 = mditer->getInnerPosition(44 * 60, 0);
    Mantid::coord_t z60_0 = mditer->getInnerPosition(44 * 60, 2);
    double twotheta60_0 = atan(x60_0 / z60_0) * 180. / M_PI;
    TS_ASSERT_DELTA(twotheta60_0, 12.0, 0.0001);

    Mantid::coord_t lastx = mditer->getInnerPosition(44 * 61 - 1, 0);
    Mantid::coord_t lasty = mditer->getInnerPosition(44 * 61 - 1, 1);
    Mantid::coord_t lastz = mditer->getInnerPosition(44 * 61 - 1, 2);
    TS_ASSERT_DELTA(lastx, 1.57956, 0.0001);
    TS_ASSERT_DELTA(lasty, 0.00, 0.0001);
    double last2theta = atan(lastx / lastz) * 180 / M_PI;
    TS_ASSERT_DELTA(last2theta + 180.0, 12.0 + 115.835, 0.001);

    // Experiment information
    uint16_t numexpinfo = mdws->getNumExperimentInfo();
    TS_ASSERT_EQUALS(numexpinfo, 61 + 1);

    // Check run number
    ExperimentInfo_const_sptr expinfo0 = mdws->getExperimentInfo(0);
    TS_ASSERT(expinfo0);
    TS_ASSERT_EQUALS(expinfo0->getRunNumber(), 1);

    ExperimentInfo_const_sptr expinfo61 = mdws->getExperimentInfo(61);
    TS_ASSERT(expinfo61);
    TS_ASSERT_EQUALS(expinfo61->getRunNumber(), -1);

    // Check log and comparing with run_start
    Mantid::Kernel::Property *tempa = expinfo61->run().getProperty("temp_a");
    TS_ASSERT(tempa);
    Mantid::Kernel::TimeSeriesProperty<double> *timeseriestempa =
        dynamic_cast<Mantid::Kernel::TimeSeriesProperty<double> *>(tempa);
    TS_ASSERT(timeseriestempa);

    std::vector<DateAndTime> times = timeseriestempa->timesAsVector();
    TS_ASSERT_EQUALS(times.size(), 61);
    DateAndTime time0 = times[0];
    TS_ASSERT_EQUALS(time0.toFormattedString(), "2012-Aug-13 11:57:33");
    DateAndTime time1 = times[1];
    TS_ASSERT_EQUALS(time1.toFormattedString(), "2012-Aug-13 11:58:03");

    // Examine Monitor MDWorkspace
    IMDWorkspace_const_sptr monmdws = boost::dynamic_pointer_cast<IMDWorkspace>(
        AnalysisDataService::Instance().retrieve("MonitorMDW"));

    // Check the IMDEvent workspace generated
    numevents = monmdws->getNEvents();
    TS_ASSERT_EQUALS(numevents, 44 * 61);

    mditer = monmdws->createIterator();
    TS_ASSERT_EQUALS(mditer->getNumEvents(), 44 * 61);

    y0 = mditer->getInnerSignal(0);
    TS_ASSERT_DELTA(y0, 31964.000, 0.1);
    yl = mditer->getInnerSignal(44 * 61 - 1);
    TS_ASSERT_DELTA(yl, 31968.0, 0.1);

    // Remove workspaces
    AnalysisDataService::Instance().remove("DataTable");
    AnalysisDataService::Instance().remove("LogParentWS");
    AnalysisDataService::Instance().remove("HB2A_MD");
    AnalysisDataService::Instance().remove("MonitorMDW");
  }
};

#endif /* MANTID_MDALGORITHMS_CONVERTSPICEDATATOREALSPACETEST_H_ */
