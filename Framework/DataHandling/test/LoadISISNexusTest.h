// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <cxxtest/TestSuite.h>

#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/Sample.h"
#include "MantidAPI/SpectrumInfo.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidDataHandling/LoadISISNexus2.h"
#include "MantidGeometry/Instrument.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/FilteredTimeSeriesProperty.h"
#include "MantidKernel/TimeSeriesProperty.h"

#include <cmath>

using namespace Mantid::API;
using namespace Mantid::Kernel;
using namespace Mantid::DataHandling;

class LoadISISNexusTest : public CxxTest::TestSuite {
private:
  // Helper method to fetch the log property entry corresponding to period.
  Property *fetchPeriodLog(const MatrixWorkspace_sptr &workspace, int expectedPeriodNumber) {
    std::stringstream period_number_stream;
    period_number_stream << expectedPeriodNumber;
    std::string log_name = "period " + period_number_stream.str();
    Property *p = workspace->run().getLogData(log_name);
    return p;
  }

  // Helper method to fetch the log property entry corresponding to the current
  // period.
  Property *fetchCurrentPeriodLog(const MatrixWorkspace_sptr &workspace) {
    Property *p = workspace->run().getLogData("current_period");
    return p;
  }

  // Helper method to check that the log data contains a specific period number
  // entry.
  void checkPeriodLogData(const MatrixWorkspace_sptr &workspace, int expectedPeriodNumber) {
    Property *p = nullptr;
    TS_ASSERT_THROWS_NOTHING(p = fetchPeriodLog(workspace, expectedPeriodNumber));
    TS_ASSERT(p != nullptr)
    TSM_ASSERT_THROWS("Shouldn't have a period less than the expected entry",
                      fetchPeriodLog(workspace, expectedPeriodNumber - 1),
                      const Mantid::Kernel::Exception::NotFoundError &);
    TSM_ASSERT_THROWS("Shouldn't have a period greater than the expected entry",
                      fetchPeriodLog(workspace, expectedPeriodNumber + 1),
                      const Mantid::Kernel::Exception::NotFoundError &);
    Mantid::Kernel::TimeSeriesProperty<bool> *period_property =
        dynamic_cast<Mantid::Kernel::TimeSeriesProperty<bool> *>(p);
    TS_ASSERT(period_property);
    // Check that the logs also contain a current_period property.
    Property *current_period_log = fetchCurrentPeriodLog(workspace);
    TS_ASSERT_EQUALS(expectedPeriodNumber, std::stoi(current_period_log->value()));

    // Check time series properties have been filtered by period
    p = nullptr;
    TSM_ASSERT_THROWS_NOTHING("Cannot retrieve stheta log", p = workspace->run().getLogData("stheta"));
    auto stheta = dynamic_cast<FilteredTimeSeriesProperty<double> *>(p);
    TSM_ASSERT("stheta log has not been converted to a FilteredTimeSeries", stheta);
    TS_ASSERT(42 > stheta->size());
  }

public:
  void testExecMonSeparated() {
    LoadISISNexus2 ld;
    ld.initialize();
    ld.setPropertyValue("Filename", "LOQ49886.nxs");
    ld.setPropertyValue("OutputWorkspace", "outWS");
    ld.setPropertyValue("LoadMonitors", "1"); // should read "Separate"
    TS_ASSERT_THROWS_NOTHING(ld.execute());
    TS_ASSERT(ld.isExecuted());

    MatrixWorkspace_sptr ws = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>("outWS");
    MatrixWorkspace_sptr mon_ws = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>("outWS_monitors");

    TS_ASSERT_EQUALS(ws->blocksize(), 5);
    TS_ASSERT_EQUALS(ws->getNumberHistograms(), 17790);

    TS_ASSERT_EQUALS(mon_ws->blocksize(), 5);
    TS_ASSERT_EQUALS(mon_ws->getNumberHistograms(), 2);

    // Two monitors which form two first spectra are excluded by load separately

    // spectrum with ID 5 is now spectrum N 3 as 2 monitors
    TS_ASSERT_EQUALS(ws->y(5 - 2)[1], 1.);
    TS_ASSERT_EQUALS(ws->getSpectrum(5 - 2).getSpectrumNo(), 6);
    TS_ASSERT_EQUALS(*(ws->getSpectrum(5 - 2).getDetectorIDs().begin()), 6);
    // spectrum with ID 7 is now spectrum N 4
    TS_ASSERT_EQUALS(ws->y(6 - 2)[0], 1.);
    TS_ASSERT_EQUALS(ws->getSpectrum(6 - 2).getSpectrumNo(), 7);
    TS_ASSERT_EQUALS(*(ws->getSpectrum(6 - 2).getDetectorIDs().begin()), 7);
    //
    TS_ASSERT_EQUALS(ws->y(8 - 2)[3], 1.);

    TS_ASSERT_EQUALS(mon_ws->x(0)[0], 5.);
    TS_ASSERT_EQUALS(mon_ws->x(0)[1], 4005.);
    TS_ASSERT_EQUALS(mon_ws->x(0)[2], 8005.);

    // these spectra are not loaded as above so their values are different
    // (occasionally 0)
    TS_ASSERT_EQUALS(mon_ws->y(0)[1], 0);
    TS_ASSERT_EQUALS(mon_ws->y(1)[0], 0.);
    TS_ASSERT_EQUALS(mon_ws->y(0)[3], 0.);

    const std::vector<Property *> &logs = mon_ws->run().getLogData();
    TS_ASSERT_EQUALS(logs.size(), 50);

    TimeSeriesProperty<std::string> *slog =
        dynamic_cast<TimeSeriesProperty<std::string> *>(mon_ws->run().getLogData("icp_event"));
    TS_ASSERT(slog);
    std::string str = slog->value();
    TS_ASSERT_EQUALS(str.size(), 1023);
    TS_ASSERT_EQUALS(str.substr(0, 37), "2009-Apr-28 09:20:29  CHANGE_PERIOD 1");

    slog = dynamic_cast<TimeSeriesProperty<std::string> *>(mon_ws->run().getLogData("icp_debug"));
    TS_ASSERT(slog);
    TS_ASSERT_EQUALS(slog->size(), 50);

    AnalysisDataService::Instance().remove("outWS");
    AnalysisDataService::Instance().remove("outWS_monitors");
  }

  void testExec() {

    LoadISISNexus2 ld;
    ld.initialize();
    ld.setPropertyValue("Filename", "LOQ49886.nxs");
    ld.setPropertyValue("OutputWorkspace", "outWS");
    TS_ASSERT_THROWS_NOTHING(ld.execute());
    TS_ASSERT(ld.isExecuted());

    MatrixWorkspace_sptr ws = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>("outWS");
    TS_ASSERT_EQUALS(ws->blocksize(), 5);
    TS_ASSERT_EQUALS(ws->getNumberHistograms(), 17792);
    TS_ASSERT_EQUALS(ws->x(0)[0], 5.);
    TS_ASSERT_EQUALS(ws->x(0)[1], 4005.);
    TS_ASSERT_EQUALS(ws->x(0)[2], 8005.);
    TS_ASSERT_EQUALS(ws->getSpectrum(0).getSpectrumNo(), 1);
    TS_ASSERT_EQUALS(*(ws->getSpectrum(0).getDetectorIDs().begin()), 1);

    TS_ASSERT_EQUALS(ws->y(5)[1], 1.);
    TS_ASSERT_EQUALS(ws->getSpectrum(5).getSpectrumNo(), 6);
    TS_ASSERT_EQUALS(*(ws->getSpectrum(5).getDetectorIDs().begin()), 6);
    TS_ASSERT_EQUALS(ws->y(6)[0], 1.);
    TS_ASSERT_EQUALS(ws->getSpectrum(6).getSpectrumNo(), 7);
    TS_ASSERT_EQUALS(*(ws->getSpectrum(6).getDetectorIDs().begin()), 7);
    TS_ASSERT_EQUALS(ws->y(8)[3], 1.);
    TS_ASSERT_EQUALS(ws->getSpectrum(8).getSpectrumNo(), 9);
    TS_ASSERT_EQUALS(*(ws->getSpectrum(8).getDetectorIDs().begin()), 9);

    TS_ASSERT_EQUALS(ws->y(13)[1], 1.);
    TS_ASSERT_EQUALS(ws->getSpectrum(13).getSpectrumNo(), 14);
    TS_ASSERT_EQUALS(*(ws->getSpectrum(13).getDetectorIDs().begin()), 14);
    TS_ASSERT_EQUALS(ws->y(17)[1], 2.);
    TS_ASSERT_EQUALS(ws->getSpectrum(17).getSpectrumNo(), 18);
    TS_ASSERT_EQUALS(*(ws->getSpectrum(17).getDetectorIDs().begin()), 18);
    TS_ASSERT_EQUALS(ws->y(18)[1], 1.);
    TS_ASSERT_EQUALS(ws->getSpectrum(18).getSpectrumNo(), 19);
    TS_ASSERT_EQUALS(*(ws->getSpectrum(18).getDetectorIDs().begin()), 19);

    TS_ASSERT_EQUALS(ws->y(33)[2], 1.);
    TS_ASSERT_EQUALS(ws->getSpectrum(33).getSpectrumNo(), 34);
    TS_ASSERT_EQUALS(*(ws->getSpectrum(33).getDetectorIDs().begin()), 34);
    TS_ASSERT_EQUALS(ws->y(34)[1], 1.);
    TS_ASSERT_EQUALS(ws->getSpectrum(34).getSpectrumNo(), 35);
    TS_ASSERT_EQUALS(*(ws->getSpectrum(34).getDetectorIDs().begin()), 35);

    TS_ASSERT_EQUALS(ws->y(37)[3], 1.);
    TS_ASSERT_EQUALS(ws->y(37)[4], 1.);
    TS_ASSERT_EQUALS(ws->getSpectrum(37).getSpectrumNo(), 38);
    TS_ASSERT_EQUALS(*(ws->getSpectrum(37).getDetectorIDs().begin()), 38);

    TS_ASSERT_EQUALS(ws->getSpectrum(1234).getDetectorIDs().size(), 1);
    TS_ASSERT_EQUALS(*(ws->getSpectrum(1234).getDetectorIDs().begin()), 1235);

    TS_ASSERT_EQUALS(ws->getSpectrum(1234).getSpectrumNo(), 1235);
    TS_ASSERT(ws->getSpectrum(1234).hasDetectorID(1235));

    const std::vector<Property *> &logs = ws->run().getLogData();
    TS_ASSERT_EQUALS(logs.size(), 50);

    TimeSeriesProperty<std::string> *slog =
        dynamic_cast<TimeSeriesProperty<std::string> *>(ws->run().getLogData("icp_event"));
    TS_ASSERT(slog);
    std::string str = slog->value();
    TS_ASSERT_EQUALS(str.size(), 1023);
    TS_ASSERT_EQUALS(str.substr(0, 37), "2009-Apr-28 09:20:29  CHANGE_PERIOD 1");

    slog = dynamic_cast<TimeSeriesProperty<std::string> *>(ws->run().getLogData("icp_debug"));
    TS_ASSERT(slog);
    TS_ASSERT_EQUALS(slog->size(), 50);

    TimeSeriesProperty<int> *ilog = dynamic_cast<TimeSeriesProperty<int> *>(ws->run().getLogData("total_counts"));
    TS_ASSERT(ilog);
    TS_ASSERT_EQUALS(ilog->size(), 172);

    ilog = dynamic_cast<TimeSeriesProperty<int> *>(ws->run().getLogData("period"));
    TS_ASSERT(ilog);
    TS_ASSERT_EQUALS(ilog->size(), 172);

    // the filtering is automatically applied which cuts a notching in a log value
    // this, effectively, creates one extra value
    TimeSeriesProperty<double> *dlog =
        dynamic_cast<TimeSeriesProperty<double> *>(ws->run().getLogData("proton_charge"));
    TS_ASSERT(dlog);
    TS_ASSERT_EQUALS(dlog->size(), 173);

    TimeSeriesProperty<bool> *blog = dynamic_cast<TimeSeriesProperty<bool> *>(ws->run().getLogData("period 1"));
    TS_ASSERT(blog);
    TS_ASSERT_EQUALS(blog->size(), 1);

    blog = dynamic_cast<TimeSeriesProperty<bool> *>(ws->run().getLogData("running"));
    TS_ASSERT(blog);
    TS_ASSERT_EQUALS(blog->size(), 6);

    TS_ASSERT_EQUALS(ws->sample().getName(), "PMMA_SAN25_1.5%_TRANS_150");

    Property *l_property = ws->run().getLogData("run_number");
    TS_ASSERT_EQUALS(l_property->value(), "49886");
    AnalysisDataService::Instance().remove("outWS");
  }

  void testExec2() {

    LoadISISNexus2 ld;
    ld.initialize();
    ld.setPropertyValue("Filename", "LOQ49886.nxs");
    ld.setPropertyValue("OutputWorkspace", "outWS");
    ld.setPropertyValue("SpectrumMin", "10");
    ld.setPropertyValue("SpectrumMax", "20");
    ld.setPropertyValue("SpectrumList", "5,34,35,38");
    TS_ASSERT_THROWS_NOTHING(ld.execute());
    TS_ASSERT(ld.isExecuted());

    MatrixWorkspace_sptr ws = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>("outWS");
    TS_ASSERT_EQUALS(ws->blocksize(), 5);
    TS_ASSERT_EQUALS(ws->getNumberHistograms(), 15);

    TS_ASSERT_EQUALS(ws->x(0)[0], 5.);
    TS_ASSERT_EQUALS(ws->x(0)[1], 4005.);
    TS_ASSERT_EQUALS(ws->x(0)[2], 8005.);
    TS_ASSERT_EQUALS(ws->getSpectrum(0).getSpectrumNo(), 5);
    TS_ASSERT_EQUALS(*(ws->getSpectrum(0).getDetectorIDs().begin()), 5);

    // these spectra are not loaded as above so their values are different
    // (occasionally 0)
    TSM_ASSERT_EQUALS("Total workspace spectra N13, index 1 is occasionally 1 ", ws->y(5)[1], 1.);
    TS_ASSERT_EQUALS(ws->getSpectrum(5).getSpectrumNo(), 14);
    TS_ASSERT_EQUALS(*(ws->getSpectrum(5).getDetectorIDs().begin()), 14);
    TS_ASSERT_EQUALS(ws->y(6)[0], 0.);
    TS_ASSERT_EQUALS(ws->getSpectrum(6).getSpectrumNo(), 15);
    TS_ASSERT_EQUALS(*(ws->getSpectrum(6).getDetectorIDs().begin()), 15);
    TS_ASSERT_EQUALS(ws->y(8)[3], 0.);
    TS_ASSERT_EQUALS(ws->getSpectrum(8).getSpectrumNo(), 17);
    TS_ASSERT_EQUALS(*(ws->getSpectrum(8).getDetectorIDs().begin()), 17);

    // look at the same values as the full loader above
    TS_ASSERT_EQUALS(ws->y(13 - 8)[1], 1.);
    TS_ASSERT_EQUALS(ws->getSpectrum(13 - 8).getSpectrumNo(), 14);
    TS_ASSERT_EQUALS(*(ws->getSpectrum(13 - 8).getDetectorIDs().begin()), 14);

    TS_ASSERT_EQUALS(ws->y(17 - 8)[1], 2.);
    TS_ASSERT_EQUALS(ws->getSpectrum(17 - 8).getSpectrumNo(), 18);
    TS_ASSERT_EQUALS(*(ws->getSpectrum(17 - 8).getDetectorIDs().begin()), 18);
    TS_ASSERT_EQUALS(ws->y(18 - 8)[1], 1.);
    TS_ASSERT_EQUALS(ws->getSpectrum(18 - 8).getSpectrumNo(), 19);
    TS_ASSERT_EQUALS(*(ws->getSpectrum(18 - 8).getDetectorIDs().begin()), 19);

    // look at the same values as the full loader above
    TS_ASSERT_EQUALS(ws->y(33 - 21)[2], 1.);
    TS_ASSERT_EQUALS(ws->getSpectrum(33 - 21).getSpectrumNo(), 34);
    TS_ASSERT_EQUALS(*(ws->getSpectrum(33 - 21).getDetectorIDs().begin()), 34);
    TS_ASSERT_EQUALS(ws->y(34 - 21)[1], 1.);
    TS_ASSERT_EQUALS(ws->getSpectrum(34 - 21).getSpectrumNo(), 35);
    TS_ASSERT_EQUALS(*(ws->getSpectrum(34 - 21).getDetectorIDs().begin()), 35);
    TS_ASSERT_EQUALS(ws->y(37 - 23)[3], 1.);
    TS_ASSERT_EQUALS(ws->y(37 - 23)[4], 1.);
    TS_ASSERT_EQUALS(ws->getSpectrum(37 - 23).getSpectrumNo(), 38);
    TS_ASSERT_EQUALS(*(ws->getSpectrum(37 - 23).getDetectorIDs().begin()), 38);

    TS_ASSERT_EQUALS(ws->getSpectrum(0).getSpectrumNo(), 5);
    TS_ASSERT_EQUALS(*(ws->getSpectrum(0).getDetectorIDs().begin()), 5);
    TS_ASSERT(ws->getSpectrum(0).hasDetectorID(5));

    TS_ASSERT_EQUALS(ws->getSpectrum(1).getSpectrumNo(), 10);
    TS_ASSERT_EQUALS(*(ws->getSpectrum(1).getDetectorIDs().begin()), 10);
    TS_ASSERT(ws->getSpectrum(1).hasDetectorID(10));

    Mantid::specnum_t offset;
    auto spectNum2WSInd = ws->getSpectrumToWorkspaceIndexVector(offset);

    TS_ASSERT_EQUALS(38 + offset + 1, spectNum2WSInd.size());
    size_t sample[] = {5, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 34, 35, 38};
    std::vector<size_t> Sample(sample, sample + 15);
    for (size_t i = 0; i < Sample.size(); i++) {
      TS_ASSERT_EQUALS(i, spectNum2WSInd[Sample[i] + offset]);
    }

    TS_ASSERT_EQUALS(ws->getSpectrum(14).getSpectrumNo(), 38);
    TS_ASSERT_EQUALS(*(ws->getSpectrum(14).getDetectorIDs().begin()), 38);
    TS_ASSERT(ws->getSpectrum(14).hasDetectorID(38));

    AnalysisDataService::Instance().remove("outWS");
  }

  void testExec3() {

    LoadISISNexus2 ld;
    ld.initialize();
    ld.setPropertyValue("Filename", "LOQ49886.nxs");
    ld.setPropertyValue("OutputWorkspace", "outWS");
    ld.setPropertyValue("SpectrumMin", "10");
    ld.setPropertyValue("SpectrumMax", "20");
    TS_ASSERT_THROWS_NOTHING(ld.execute());
    TS_ASSERT(ld.isExecuted());

    MatrixWorkspace_sptr ws = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>("outWS");
    TS_ASSERT_EQUALS(ws->blocksize(), 5);
    TS_ASSERT_EQUALS(ws->getNumberHistograms(), 11);

    TS_ASSERT_EQUALS(ws->x(0)[0], 5.);
    TS_ASSERT_EQUALS(ws->x(0)[1], 4005.);
    TS_ASSERT_EQUALS(ws->x(0)[2], 8005.);

    // these spectra are not loaded as above so their values are different
    // (occasionally 0)
    TS_ASSERT_EQUALS(ws->y(5)[1], 0);
    TS_ASSERT_EQUALS(ws->y(6)[0], 0.);
    TS_ASSERT_EQUALS(ws->y(8)[3], 0.);

    // look at the same values as the full/partial loader above
    TS_ASSERT_EQUALS(ws->y(13 - 9)[1], 1.);
    TS_ASSERT_EQUALS(ws->getSpectrum(13 - 9).getSpectrumNo(), 14);
    TS_ASSERT_EQUALS(ws->y(17 - 9)[1], 2.);
    TS_ASSERT_EQUALS(ws->getSpectrum(17 - 9).getSpectrumNo(), 18);
    TS_ASSERT_EQUALS(ws->y(18 - 9)[1], 1.);
    TS_ASSERT_EQUALS(ws->getSpectrum(18 - 9).getSpectrumNo(), 19);

    Mantid::specnum_t offset;
    auto spectNum2WSInd = ws->getSpectrumToWorkspaceIndexVector(offset);
    TS_ASSERT_EQUALS(20 + offset + 1, spectNum2WSInd.size());
    size_t sample[] = {10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20};
    std::vector<size_t> Sample(sample, sample + 10);
    for (size_t i = 0; i < Sample.size(); i++) {
      TS_ASSERT_EQUALS(i, spectNum2WSInd[Sample[i] + offset]);
    }

    AnalysisDataService::Instance().remove("outWS");
  }

  void testMultiPeriodEntryNumberZero() {

    LoadISISNexus2 ld;
    ld.initialize();
    ld.setPropertyValue("Filename", "TEST00000008.nxs");
    ld.setPropertyValue("OutputWorkspace", "outWS");
    ld.setPropertyValue("SpectrumMin", "10");
    ld.setPropertyValue("SpectrumMax", "19");
    ld.setPropertyValue("EntryNumber", "0");
    // ld.setPropertyValue("SpectrumList","30,31");
    TS_ASSERT_THROWS_NOTHING(ld.execute());
    TS_ASSERT(ld.isExecuted());

    WorkspaceGroup_sptr grpout; //=WorkspaceGroup_sptr(new WorkspaceGroup);
    TS_ASSERT_THROWS_NOTHING(grpout = AnalysisDataService::Instance().retrieveWS<WorkspaceGroup>("outWS"));

    MatrixWorkspace_sptr ws = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>("outWS_1");
    TS_ASSERT_EQUALS(ws->blocksize(), 995);
    TS_ASSERT_EQUALS(ws->getNumberHistograms(), 10);
    TS_ASSERT_DELTA(ws->run().getProtonCharge(), 0.069991, 1e-6);

    TS_ASSERT_EQUALS(ws->x(0)[0], 5.);
    TS_ASSERT_EQUALS(ws->x(0)[1], 6.);
    TS_ASSERT_EQUALS(ws->x(0)[2], 7.);

    TS_ASSERT_EQUALS(ws->y(5)[1], 0);
    TS_ASSERT_EQUALS(ws->y(6)[0], 0.);
    TS_ASSERT_EQUALS(ws->y(8)[3], 0.);

    TS_ASSERT_EQUALS(ws->y(7)[1], 0.);
    TS_ASSERT_EQUALS(ws->y(9)[3], 0.);
    TS_ASSERT_EQUALS(ws->y(9)[1], 0.);
    AnalysisDataService::Instance().remove("outWS");
  }

  void testMultiPeriodEntryNumberNonZero() {

    LoadISISNexus2 ld;
    ld.initialize();
    ld.setPropertyValue("Filename", "TEST00000008.nxs");
    ld.setPropertyValue("OutputWorkspace", "outWS");
    ld.setPropertyValue("SpectrumMin", "10");
    ld.setPropertyValue("SpectrumMax", "20");
    //      ld.setPropertyValue("SpectrumList","29,30,31");
    ld.setPropertyValue("EntryNumber", "5");
    TS_ASSERT_THROWS_NOTHING(ld.execute());
    TS_ASSERT(ld.isExecuted());

    MatrixWorkspace_sptr ws = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>("outWS");
    TS_ASSERT_EQUALS(ws->blocksize(), 995);
    //   TS_ASSERT_EQUALS(ws->getNumberHistograms(),14);
    TS_ASSERT_EQUALS(ws->getTitle(), "hello\\0");
    TS_ASSERT_DELTA(ws->run().getProtonCharge(), 0.069991, 1e-6);
    TS_ASSERT_EQUALS(ws->x(0)[0], 5.);
    TS_ASSERT_EQUALS(ws->x(0)[1], 6.);
    TS_ASSERT_EQUALS(ws->x(0)[2], 7.);

    TS_ASSERT_EQUALS(ws->y(5)[1], 0.);
    TS_ASSERT_EQUALS(ws->y(6)[0], 0.);
    TS_ASSERT_EQUALS(ws->y(8)[3], 0.);

    TS_ASSERT_EQUALS(ws->y(7)[1], 0.);
    TS_ASSERT_EQUALS(ws->y(9)[3], 0.);
    TS_ASSERT_EQUALS(ws->y(9)[1], 0.);
    AnalysisDataService::Instance().remove("outWS");
  }

  void testLoadMultiPeriodData() {

    const std::string wsName = "outWS";
    LoadISISNexus2 loadingAlg;
    loadingAlg.initialize();
    loadingAlg.setRethrows(true);
    loadingAlg.setPropertyValue("Filename", "POLREF00004699.nxs");
    loadingAlg.setPropertyValue("OutputWorkspace", wsName);
    loadingAlg.execute();
    TS_ASSERT(loadingAlg.isExecuted());

    AnalysisDataServiceImpl &ADS = AnalysisDataService::Instance();

    WorkspaceGroup_sptr grpWs;
    TS_ASSERT_THROWS_NOTHING(grpWs = ADS.retrieveWS<WorkspaceGroup>(wsName));
    TSM_ASSERT_EQUALS("Should be two workspaces in the group", 2, grpWs->size());

    // Check the individual workspace group members.
    MatrixWorkspace_sptr ws1 = std::dynamic_pointer_cast<MatrixWorkspace>(grpWs->getItem(0));
    MatrixWorkspace_sptr ws2 = std::dynamic_pointer_cast<MatrixWorkspace>(grpWs->getItem(1));
    TS_ASSERT(ws1 != nullptr);
    TS_ASSERT(ws2 != nullptr);
    // Check that workspace 1 has the correct period data, and no other period
    // log data
    checkPeriodLogData(ws1, 1);
    // Check that workspace 2 has the correct period data, and no other period
    // log data
    checkPeriodLogData(ws2, 2);
    // Check the multi-period proton charge extraction
    const Run &run = ws1->run();
    ArrayProperty<double> *protonChargeProperty =
        dynamic_cast<ArrayProperty<double> *>(run.getLogData("proton_charge_by_period"));
    double chargeSum = 0;
    for (size_t i = 0; i < grpWs->size(); ++i) {
      chargeSum += protonChargeProperty->operator()()[i];
    }
    PropertyWithValue<double> *totalChargeProperty =
        dynamic_cast<PropertyWithValue<double> *>(run.getLogData("gd_prtn_chrg"));
    double totalCharge = std::stod(totalChargeProperty->value());
    TSM_ASSERT_DELTA("Something is badly wrong if the sum across the periods "
                     "does not correspond to the total charge.",
                     totalCharge, chargeSum, 0.000001);
    AnalysisDataService::Instance().remove(wsName);
  }

  void test_instrument_and_default_param_loaded_when_inst_not_in_nexus_file() {

    const std::string wsName = "InstNotInNexus";
    LoadISISNexus2 loadingAlg;
    loadingAlg.initialize();
    loadingAlg.setRethrows(true);
    loadingAlg.setPropertyValue("Filename", "POLREF00004699.nxs");
    loadingAlg.setPropertyValue("OutputWorkspace", wsName);
    loadingAlg.execute();
    TS_ASSERT(loadingAlg.isExecuted());

    AnalysisDataServiceImpl &ADS = AnalysisDataService::Instance();
    WorkspaceGroup_sptr grpWs;
    TS_ASSERT_THROWS_NOTHING(grpWs = ADS.retrieveWS<WorkspaceGroup>(wsName));
    MatrixWorkspace_sptr ws1 = std::dynamic_pointer_cast<MatrixWorkspace>(grpWs->getItem(0));

    auto inst = ws1->getInstrument();
    TS_ASSERT(!inst->getFilename().empty()); // This is how we know we didn't
                                             // get it from inside the nexus
                                             // file
    TS_ASSERT_EQUALS(inst->getName(), "POLREF");
    TS_ASSERT_EQUALS(inst->getNumberDetectors(), 885);

    // check that POLREF_Parameters.xml has been loaded
    auto params = inst->getParameterMap();
    TS_ASSERT_EQUALS(params->getString(inst.get(), "show-signed-theta"), "Always");
  }

  void testExecMonExcluded() {

    LoadISISNexus2 ld;
    ld.initialize();
    ld.setPropertyValue("Filename", "LOQ49886.nxs");
    ld.setPropertyValue("OutputWorkspace", "outWS");
    ld.setPropertyValue("LoadMonitors", "0"); // should read "exclude"
    TS_ASSERT_THROWS_NOTHING(ld.execute());
    TS_ASSERT(ld.isExecuted());

    MatrixWorkspace_sptr ws = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>("outWS");
    TS_ASSERT_EQUALS(ws->blocksize(), 5);
    TS_ASSERT_EQUALS(ws->getNumberHistograms(), 17790);

    // Two monitors which form two first spectra are excluded by load separately

    // spectrum with ID 5 is now spectrum N 3 as 2 monitors
    TS_ASSERT_EQUALS(ws->y(5 - 2)[1], 1.);
    TS_ASSERT_EQUALS(ws->getSpectrum(5 - 2).getSpectrumNo(), 6);
    TS_ASSERT_EQUALS(*(ws->getSpectrum(5 - 2).getDetectorIDs().begin()), 6);
    // spectrum with ID 7 is now spectrum N 4
    TS_ASSERT_EQUALS(ws->y(6 - 2)[0], 1.);
    TS_ASSERT_EQUALS(ws->getSpectrum(6 - 2).getSpectrumNo(), 7);
    TS_ASSERT_EQUALS(*(ws->getSpectrum(6 - 2).getDetectorIDs().begin()), 7);
    //
    TS_ASSERT_EQUALS(ws->y(8 - 2)[3], 1.);

    // spectrum with ID 9 is now spectrum N 6
    TS_ASSERT_EQUALS(ws->getSpectrum(8 - 2).getSpectrumNo(), 9);
    TS_ASSERT_EQUALS(*(ws->getSpectrum(8 - 2).getDetectorIDs().begin()), 9);
    // spectrum with ID 14 is now spectrum N 11
    TS_ASSERT_EQUALS(ws->y(13 - 2)[1], 1.);
    TS_ASSERT_EQUALS(ws->getSpectrum(13 - 2).getSpectrumNo(), 14);
    TS_ASSERT_EQUALS(*(ws->getSpectrum(13 - 2).getDetectorIDs().begin()), 14);
    // spectrum with ID 18 is now spectrum N 15
    TS_ASSERT_EQUALS(ws->y(17 - 2)[1], 2.);
    TS_ASSERT_EQUALS(ws->getSpectrum(17 - 2).getSpectrumNo(), 18);
    TS_ASSERT_EQUALS(*(ws->getSpectrum(17 - 2).getDetectorIDs().begin()), 18);
    // spectrum with ID 19 is now spectrum N 16
    TS_ASSERT_EQUALS(ws->y(18 - 2)[1], 1.);
    TS_ASSERT_EQUALS(ws->getSpectrum(18 - 2).getSpectrumNo(), 19);
    TS_ASSERT_EQUALS(*(ws->getSpectrum(18 - 2).getDetectorIDs().begin()), 19);

    TS_ASSERT_EQUALS(ws->y(33 - 2)[2], 1.);
    TS_ASSERT_EQUALS(ws->getSpectrum(33 - 2).getSpectrumNo(), 34);
    TS_ASSERT_EQUALS(*(ws->getSpectrum(33 - 2).getDetectorIDs().begin()), 34);
    //
    TS_ASSERT_EQUALS(ws->y(34 - 2)[1], 1.);
    TS_ASSERT_EQUALS(ws->getSpectrum(34 - 2).getSpectrumNo(), 35);
    TS_ASSERT_EQUALS(*(ws->getSpectrum(34 - 2).getDetectorIDs().begin()), 35);

    TS_ASSERT_EQUALS(ws->y(37 - 2)[3], 1.);
    TS_ASSERT_EQUALS(ws->y(37 - 2)[4], 1.);
    TS_ASSERT_EQUALS(ws->getSpectrum(37 - 2).getSpectrumNo(), 38);
    TS_ASSERT_EQUALS(*(ws->getSpectrum(37 - 2).getDetectorIDs().begin()), 38);

    TS_ASSERT_EQUALS(ws->getSpectrum(1234 - 2).getDetectorIDs().size(), 1);
    TS_ASSERT_EQUALS(*(ws->getSpectrum(1234 - 2).getDetectorIDs().begin()), 1235);

    TS_ASSERT_EQUALS(ws->getSpectrum(1234 - 2).getSpectrumNo(), 1235);
    TS_ASSERT(ws->getSpectrum(1234 - 2).hasDetectorID(1235));

    const std::vector<Property *> &logs = ws->run().getLogData();
    TS_ASSERT_EQUALS(logs.size(), 50);

    TimeSeriesProperty<std::string> *slog =
        dynamic_cast<TimeSeriesProperty<std::string> *>(ws->run().getLogData("icp_event"));
    TS_ASSERT(slog);
    std::string str = slog->value();
    TS_ASSERT_EQUALS(str.size(), 1023);
    TS_ASSERT_EQUALS(str.substr(0, 37), "2009-Apr-28 09:20:29  CHANGE_PERIOD 1");

    slog = dynamic_cast<TimeSeriesProperty<std::string> *>(ws->run().getLogData("icp_debug"));
    TS_ASSERT(slog);
    TS_ASSERT_EQUALS(slog->size(), 50);

    TimeSeriesProperty<int> *ilog = dynamic_cast<TimeSeriesProperty<int> *>(ws->run().getLogData("total_counts"));
    TS_ASSERT(ilog);
    TS_ASSERT_EQUALS(ilog->size(), 172);

    ilog = dynamic_cast<TimeSeriesProperty<int> *>(ws->run().getLogData("period"));
    TS_ASSERT(ilog);
    TS_ASSERT_EQUALS(ilog->size(), 172);

    // the filtering is automatically applied which cuts a notching in a log value
    // this, effectively, creates one extra value
    TimeSeriesProperty<double> *dlog =
        dynamic_cast<TimeSeriesProperty<double> *>(ws->run().getLogData("proton_charge"));
    TS_ASSERT(dlog);
    TS_ASSERT_EQUALS(dlog->size(), 173);

    TimeSeriesProperty<bool> *blog = dynamic_cast<TimeSeriesProperty<bool> *>(ws->run().getLogData("period 1"));
    TS_ASSERT(blog);
    TS_ASSERT_EQUALS(blog->size(), 1);

    blog = dynamic_cast<TimeSeriesProperty<bool> *>(ws->run().getLogData("running"));
    TS_ASSERT(blog);
    TS_ASSERT_EQUALS(blog->size(), 6);

    TS_ASSERT_EQUALS(ws->sample().getName(), "PMMA_SAN25_1.5%_TRANS_150");

    Property *l_property = ws->run().getLogData("run_number");
    TS_ASSERT_EQUALS(l_property->value(), "49886");
    AnalysisDataService::Instance().remove("outWS");
  }

  void testExecMultiPeriodMonitorSeparate() {
    LoadISISNexus2 ld;
    ld.setChild(true);
    ld.initialize();
    ld.setPropertyValue("Filename", "POLREF00004699.nxs");
    ld.setPropertyValue("OutputWorkspace", "__unused_for_child");
    ld.setPropertyValue("LoadMonitors", "Separate");
    TS_ASSERT_THROWS_NOTHING(ld.execute());
    TS_ASSERT(ld.isExecuted());

    Workspace_sptr detWS = ld.getProperty("OutputWorkspace");
    auto detGroup = std::dynamic_pointer_cast<WorkspaceGroup>(detWS);
    TS_ASSERT(detGroup);
    Workspace_sptr monWS = ld.getProperty("MonitorWorkspace");
    auto monGroup = std::dynamic_pointer_cast<WorkspaceGroup>(monWS);
    TS_ASSERT(monGroup);

    if (!(detGroup && monGroup))
      return;

    TS_ASSERT_EQUALS(2, detGroup->size());
    TS_ASSERT_EQUALS(2, monGroup->size());

    auto detWS0 = std::dynamic_pointer_cast<MatrixWorkspace>(detGroup->getItem(0));
    TS_ASSERT_EQUALS(1000, detWS0->blocksize());
    TS_ASSERT_EQUALS(243, detWS0->getNumberHistograms());
    TS_ASSERT_DELTA(105, detWS0->x(1)[1], 1e-08);
    TS_ASSERT_DELTA(2, detWS0->y(1)[1], 1e-08);
    TS_ASSERT_DELTA(M_SQRT2, detWS0->e(1)[1], 1e-08);
    TS_ASSERT_EQUALS(detWS0->getSpectrum(0).getSpectrumNo(), 4);

    auto monWS0 = std::dynamic_pointer_cast<MatrixWorkspace>(monGroup->getItem(0));
    TS_ASSERT_EQUALS(1000, monWS0->blocksize());
    TS_ASSERT_EQUALS(3, monWS0->getNumberHistograms());
    TS_ASSERT_DELTA(105, monWS0->x(1)[1], 1e-08);
    TS_ASSERT_DELTA(12563.0, monWS0->y(0)[1], 1e-08);
    TS_ASSERT_DELTA(std::sqrt(12563.0), monWS0->e(0)[1], 1e-08);
    TS_ASSERT_EQUALS(monWS0->getSpectrum(0).getSpectrumNo(), 1);
    TS_ASSERT_EQUALS(monWS0->getSpectrum(2).getSpectrumNo(), 3);

    auto monWS1 = std::dynamic_pointer_cast<MatrixWorkspace>(monGroup->getItem(1));
    TS_ASSERT_EQUALS(1000, monWS1->blocksize());
    TS_ASSERT_EQUALS(3, monWS1->getNumberHistograms());
    TS_ASSERT_DELTA(105, monWS1->x(1)[1], 1e-08);
    TS_ASSERT_DELTA(12595.0, monWS1->y(0)[1], 1e-08);
    TS_ASSERT_DELTA(std::sqrt(12595.0), monWS1->e(0)[1], 1e-08);
    TS_ASSERT_EQUALS(monWS1->getSpectrum(0).getSpectrumNo(), 1);
    TS_ASSERT_EQUALS(monWS1->getSpectrum(2).getSpectrumNo(), 3);

    // Same number of logs
    const auto &monPeriod1Run = monWS0->run();
    const auto &monPeriod2Run = monWS1->run();
    TS_ASSERT_EQUALS(monPeriod1Run.getLogData().size(), monPeriod2Run.getLogData().size());
    TS_ASSERT(monPeriod1Run.hasProperty("period 1"))
    TS_ASSERT(monPeriod2Run.hasProperty("period 2"))
  }

  std::string extractStringLog(const MatrixWorkspace &matrixWS, const std::string &logName) {
    auto run = matrixWS.run();
    PropertyWithValue<std::string> *log = dynamic_cast<PropertyWithValue<std::string> *>(run.getLogData(logName));
    return log->value();
  }

  void testExecExtractMeasurmentData() {
    LoadISISNexus2 ld;
    ld.setChild(true);
    ld.initialize();
    ld.setPropertyValue("Filename", "POLREF00014966.nxs");
    ld.setPropertyValue("OutputWorkspace", "__unused_for_child");
    ld.setPropertyValue("LoadMonitors", "Separate");
    ld.execute();

    Workspace_sptr detWS = ld.getProperty("OutputWorkspace");

    auto groupWS = std::dynamic_pointer_cast<WorkspaceGroup>(detWS);
    TSM_ASSERT("Should have got back a workspace group", groupWS);

    auto firstMatrixWS = std::dynamic_pointer_cast<MatrixWorkspace>(groupWS->getItem(0));

    TS_ASSERT_EQUALS("34", extractStringLog(*firstMatrixWS, "measurement_id"));
    TS_ASSERT_EQUALS("0", extractStringLog(*firstMatrixWS, "measurement_subid"));
    TS_ASSERT_EQUALS("", extractStringLog(*firstMatrixWS, "measurement_label"));
    TS_ASSERT_EQUALS("PNR", extractStringLog(*firstMatrixWS, "measurement_type"));

    auto secondMatrixWS = std::dynamic_pointer_cast<MatrixWorkspace>(groupWS->getItem(1));

    TS_ASSERT_EQUALS("34", extractStringLog(*secondMatrixWS, "measurement_id"));
    TS_ASSERT_EQUALS("0", extractStringLog(*secondMatrixWS, "measurement_subid"));
    TS_ASSERT_EQUALS("", extractStringLog(*secondMatrixWS, "measurement_label"));
    TS_ASSERT_EQUALS("PNR", extractStringLog(*secondMatrixWS, "measurement_type"));
  }

  //------------------------------------------------------------------
  // Non-contiguous and excluded monitors
  //------------------------------------------------------------------
  void test_that_non_contiguous_data_loads_for_excluded_monitors() {
    /*
    Monitors can be found at detID: 145, 146, 147, 148
    */
    // Arrange

    LoadISISNexus2 ld;
    ld.initialize();
    ld.setPropertyValue("Filename", "INS09161.nxs");
    ld.setPropertyValue("OutputWorkspace", "outWS");
    ld.setPropertyValue("LoadMonitors", "0" /* exclude monitors*/);
    // Act
    TS_ASSERT_THROWS_NOTHING(ld.execute());
    TS_ASSERT(ld.isExecuted());

    // Assert
    MatrixWorkspace_sptr ws = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>("outWS");
    TSM_ASSERT_EQUALS("Should have 17036 bins", ws->blocksize(), 17036);
    TSM_ASSERT_EQUALS("Should have 168 detectors (no monitors)", ws->getNumberHistograms(), 168);

    std::vector<Mantid::detid_t> monitorDetIDs{145, 146, 147, 148};
    std::vector<Mantid::detid_t> neighborsToCheck{140, 141, 142, 143, 144, 149, 150, 151};
    auto detIDtoWSIndexMap = ws->getDetectorIDToWorkspaceIndexMap();

    // Check monitors are not in workspace
    for (const auto &monitorDetID : monitorDetIDs) {
      TSM_ASSERT("Should not be in the detID2WSIndexMap.", detIDtoWSIndexMap.count(monitorDetID) == 0);
    }
    for (const auto &neighborToCheck : neighborsToCheck) {
      TSM_ASSERT("Should be in the detID2WSIndexMap.", detIDtoWSIndexMap.count(neighborToCheck) == 1);
    }

    // Check some of the data
    double delta = 1e-6;
    TS_ASSERT_DELTA(ws->y(142)[0], 0.0, delta);
    TS_ASSERT_DELTA(ws->y(142)[1], 82.0, delta);
    TS_ASSERT_DELTA(ws->y(142)[2], 57.0, delta);
    TS_ASSERT_DELTA(ws->y(142)[17034], 5.0, delta);
    TS_ASSERT_DELTA(ws->y(142)[17035], 8.0, delta);

    TS_ASSERT_DELTA(ws->y(143)[0], 0.0, delta);
    TS_ASSERT_DELTA(ws->y(143)[1], 0.0, delta);
    TS_ASSERT_DELTA(ws->y(143)[2], 0.0, delta);

    // Check that the data has the expected spectrum number and the expected
    // detecor ID
    TSM_ASSERT_EQUALS("Detector at WS index 142 should have a spectrum number of 143", 143,
                      ws->getSpectrum(142).getSpectrumNo());
    TSM_ASSERT_EQUALS("Detector at WS index 143 should have a spectrum number of 144", 144,
                      ws->getSpectrum(143).getSpectrumNo());

    const auto &spectrumInfo = ws->spectrumInfo();

    TSM_ASSERT_EQUALS("Detector at WS index 142 should have a detector ID of 143", 143,
                      spectrumInfo.detector(142).getID());
    TSM_ASSERT_EQUALS("Detector at WS index 143 should have a detector ID of 144", 144,
                      spectrumInfo.detector(143).getID());

    // Clean up
    AnalysisDataService::Instance().remove("outWS");
  }

  void test_that_non_contiguous_data_loads_for_excluded_monitors_and_spetra_list_which_contains_monitors() {
    /*
    Monitors can be found at detID: 145, 146, 147, 148
    */
    // Arrange

    LoadISISNexus2 ld;
    ld.initialize();
    ld.setPropertyValue("Filename", "INS09161.nxs");
    ld.setPropertyValue("OutputWorkspace", "outWS");
    ld.setPropertyValue("LoadMonitors", "0" /* exclude monitors*/);

    ld.setPropertyValue("SpectrumMin", "50");
    ld.setPropertyValue("SpectrumMax", "73");
    ld.setPropertyValue("SpectrumList", "12, 145");

    // Act
    TS_ASSERT_THROWS_NOTHING(ld.execute());
    TS_ASSERT(ld.isExecuted());

    // Assert
    MatrixWorkspace_sptr ws = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>("outWS");

    TSM_ASSERT_EQUALS("Should have 17036 bins", ws->blocksize(), 17036);
    TSM_ASSERT_EQUALS("Should have 25 detectors", ws->getNumberHistograms(), 25);

    // Check elements in workspace
    auto detIDtoWSIndexMap = ws->getDetectorIDToWorkspaceIndexMap();

    // Range from 1 to 11
    for (Mantid::detid_t detID = 1; detID < 12; ++detID) {
      TSM_ASSERT("Should not be in workspace", detIDtoWSIndexMap.count(detID) == 0);
    }

    // 12
    TSM_ASSERT("Should be in workspace", detIDtoWSIndexMap.count(12) == 1);

    // Range from 13 t0 49
    for (Mantid::detid_t detID = 13; detID < 50; ++detID) {
      TSM_ASSERT("Should not be in workspace", detIDtoWSIndexMap.count(detID) == 0);
    }

    // Range from 50 to 73
    for (Mantid::detid_t detID = 50; detID < 74; ++detID) {
      TSM_ASSERT("Should be in workspace", detIDtoWSIndexMap.count(detID) == 1);
    }

    // Range from 74 to 144
    for (Mantid::detid_t detID = 74; detID < 145; ++detID) {
      TSM_ASSERT("Should not be in workspace", detIDtoWSIndexMap.count(detID) == 0);
    }

    // 145 --> not in workspace !!!
    TSM_ASSERT("Should NOT be in workspace", detIDtoWSIndexMap.count(145) == 0);

    // Range from 146 to 172 (which is the number of detectors + monitors)
    for (Mantid::detid_t detID = 146; detID <= 172; ++detID) {
      TSM_ASSERT("Should not be in workspace", detIDtoWSIndexMap.count(detID) == 0);
    }
    // Check that the data has the expected spectrum number and the expected
    // detecor ID (for some sample spectra)
    TSM_ASSERT_EQUALS("Detector at WS index 24 should have a spectrum number of 73", 73,
                      ws->getSpectrum(24).getSpectrumNo());

    const auto &spectrumInfo = ws->spectrumInfo();
    TSM_ASSERT_EQUALS("Detector at WS index 24 should have a detector ID of 73", 73, spectrumInfo.detector(24).getID());

    // Clean up
    AnalysisDataService::Instance().remove("outWS");
  }

  //-----------------------------------------------------------------
  // Non-contiguous and included monitors
  //------------------------------------------------------------------
  void test_that_non_contiguous_data_loads_for_included_monitors() {
    /*
    Monitors can be found at detID: 145, 146, 147, 148
    */
    // Arrange

    LoadISISNexus2 ld;
    ld.initialize();
    ld.setPropertyValue("Filename", "INS09161.nxs");
    ld.setPropertyValue("OutputWorkspace", "outWS");
    // Act
    TS_ASSERT_THROWS_NOTHING(ld.execute());
    TS_ASSERT(ld.isExecuted());

    // Assert
    MatrixWorkspace_sptr ws = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>("outWS");
    TSM_ASSERT_EQUALS("Should have 17036 bins", ws->blocksize(), 17036);
    TSM_ASSERT_EQUALS("Should have 172 detectors (including 4 monitors)", ws->getNumberHistograms(), 172);

    std::vector<Mantid::detid_t> monitorDetIDs{145, 146, 147, 148};
    std::vector<Mantid::detid_t> neighborsToCheck{140, 141, 142, 143, 144, 149, 150, 151};
    auto detIDtoWSIndexMap = ws->getDetectorIDToWorkspaceIndexMap();

    // Check monitors are not in workspace
    for (const auto &monitorDetID : monitorDetIDs) {
      TSM_ASSERT("Should be in the detID2WSIndexMap.", detIDtoWSIndexMap.count(monitorDetID) == 1);
    }
    for (const auto &neighborToCheck : neighborsToCheck) {
      TSM_ASSERT("Should be in the detID2WSIndexMap.", detIDtoWSIndexMap.count(neighborToCheck) == 1);
    }

    // Check some of the data
    double delta = 1e-6;
    TS_ASSERT_DELTA(ws->y(142)[0], 0.0, delta);
    TS_ASSERT_DELTA(ws->y(142)[1], 82.0, delta);
    TS_ASSERT_DELTA(ws->y(142)[2], 57.0, delta);
    TS_ASSERT_DELTA(ws->y(142)[17034], 5.0, delta);
    TS_ASSERT_DELTA(ws->y(142)[17035], 8.0, delta);

    TS_ASSERT_DELTA(ws->y(144)[0], 0.0, delta);
    TS_ASSERT_DELTA(ws->y(144)[1], 176660.0, delta);
    TS_ASSERT_DELTA(ws->y(144)[2], 57659.0, delta);
    TS_ASSERT_DELTA(ws->y(144)[17034], 4851.0, delta);
    TS_ASSERT_DELTA(ws->y(144)[17035], 4513.0, delta);

    // Check that the data has the expected spectrum number and the expected
    // detecor ID
    TSM_ASSERT_EQUALS("Detector at WS index 142 should have a spectrum number of 143", 143,
                      ws->getSpectrum(142).getSpectrumNo());
    TSM_ASSERT_EQUALS("Monitor at WS index 144 should have a spectrum number of 145", 145,
                      ws->getSpectrum(144).getSpectrumNo());

    const auto &spectrumInfo = ws->spectrumInfo();
    TSM_ASSERT_EQUALS("Detector at WS index 142 should have a detector ID of 143", 143,
                      spectrumInfo.detector(142).getID());
    TSM_ASSERT_EQUALS("Detector at WS index 144 should have a detector ID of 145", 145,
                      spectrumInfo.detector(144).getID());

    // Clean up
    AnalysisDataService::Instance().remove("outWS");
  }

  void test_that_non_contiguous_data_loads_for_included_monitors_and_spectra_range_and_spetra_list() {
    /*
    Monitors can be found at detID: 145, 146, 147, 148
    */
    // Arrange

    LoadISISNexus2 ld;
    ld.initialize();
    ld.setPropertyValue("Filename", "INS09161.nxs");
    ld.setPropertyValue("OutputWorkspace", "outWS");

    ld.setPropertyValue("SpectrumMin", "50");
    ld.setPropertyValue("SpectrumMax", "73");
    ld.setPropertyValue("SpectrumList", "12, 145");

    // Act
    TS_ASSERT_THROWS_NOTHING(ld.execute());
    TS_ASSERT(ld.isExecuted());

    // Assert
    MatrixWorkspace_sptr ws = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>("outWS");

    TSM_ASSERT_EQUALS("Should have 17036 bins", ws->blocksize(), 17036);
    TSM_ASSERT_EQUALS("Should have 26 detectors", ws->getNumberHistograms(), 26);

    // Check elements in workspace
    auto detIDtoWSIndexMap = ws->getDetectorIDToWorkspaceIndexMap();

    // Range from 1 to 11
    for (Mantid::detid_t detID = 1; detID < 12; ++detID) {
      TSM_ASSERT("Should not be in workspace", detIDtoWSIndexMap.count(detID) == 0);
    }

    // 12
    TSM_ASSERT("Should be in workspace", detIDtoWSIndexMap.count(12) == 1);

    // Range from 13 t0 49
    for (Mantid::detid_t detID = 13; detID < 50; ++detID) {
      TSM_ASSERT("Should not be in workspace", detIDtoWSIndexMap.count(detID) == 0);
    }

    // Range from 50 to 73
    for (Mantid::detid_t detID = 50; detID < 74; ++detID) {
      TSM_ASSERT("Should be in workspace", detIDtoWSIndexMap.count(detID) == 1);
    }

    // Range from 74 to 144
    for (Mantid::detid_t detID = 74; detID < 145; ++detID) {
      TSM_ASSERT("Should not be in workspace", detIDtoWSIndexMap.count(detID) == 0);
    }

    // 145
    TSM_ASSERT("Should be in workspace", detIDtoWSIndexMap.count(145) == 1);

    // Range from 146 to 172 (which is the number of detectors + monitors)
    for (Mantid::detid_t detID = 146; detID <= 172; ++detID) {
      TSM_ASSERT("Should not be in workspace", detIDtoWSIndexMap.count(detID) == 0);
    }

    double delta = 1e-6;
    // Make sure that the monitor data is correct (should be workspace index 26)
    TS_ASSERT_DELTA(ws->y(25)[0], 0.0, delta);
    TS_ASSERT_DELTA(ws->y(25)[1], 176660.0, delta);
    TS_ASSERT_DELTA(ws->y(25)[2], 57659.0, delta);
    TS_ASSERT_DELTA(ws->y(25)[17034], 4851.0, delta);
    TS_ASSERT_DELTA(ws->y(25)[17035], 4513.0, delta);

    // Check that the data has the expected spectrum number and the expected
    // detecor ID (for some sample spectra)
    const auto &spectrumInfo = ws->spectrumInfo();

    TSM_ASSERT_EQUALS("Detector at WS index 24 should have a spectrum number of 73", 73,
                      ws->getSpectrum(24).getSpectrumNo());
    TSM_ASSERT_EQUALS("Detector at WS index 24 should have a detector ID of 73", 73, spectrumInfo.detector(24).getID());

    TSM_ASSERT_EQUALS("Monitor at WS index 25 should have a spectrum number of 145", 145,
                      ws->getSpectrum(25).getSpectrumNo());
    TSM_ASSERT_EQUALS("Detector at WS index 25 should have a detector ID of 145", 145,
                      spectrumInfo.detector(25).getID());

    // Clean up
    AnalysisDataService::Instance().remove("outWS");
  }

  //------------------------------------------------------------------
  // Non-contiguous and separate monitors
  //------------------------------------------------------------------
  void test_that_non_contiguous_data_loads_for_separate_monitors() {
    /*
    Monitors can be found at detID: 145, 146, 147, 148
    */
    // Arrange

    LoadISISNexus2 ld;
    ld.initialize();
    ld.setPropertyValue("Filename", "INS09161.nxs");
    ld.setPropertyValue("OutputWorkspace", "outWS");
    ld.setPropertyValue("LoadMonitors", "1" /* separate monitors*/);

    // Act
    TS_ASSERT_THROWS_NOTHING(ld.execute());
    TS_ASSERT(ld.isExecuted());

    // Assert
    MatrixWorkspace_sptr ws = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>("outWS");
    MatrixWorkspace_sptr mon_ws = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>("outWS_monitors");

    TSM_ASSERT_EQUALS("Should have 17036 bins", ws->blocksize(), 17036);
    TSM_ASSERT_EQUALS("Should have 168 detectors", ws->getNumberHistograms(), 168);

    TSM_ASSERT_EQUALS("Should have 17036 bins", mon_ws->blocksize(), 17036);
    TSM_ASSERT_EQUALS("Should have 4 monitors", mon_ws->getNumberHistograms(), 4);

    std::vector<Mantid::detid_t> monitorDetIDs{145, 146, 147, 148};
    std::vector<Mantid::detid_t> neighborsToCheck{140, 141, 142, 143, 144, 149, 150, 151};
    auto detIDtoWSIndexMap = ws->getDetectorIDToWorkspaceIndexMap();
    auto detIDtoWSIndexMapMon = mon_ws->getDetectorIDToWorkspaceIndexMap();

    // Check monitors are not in workspace
    for (const auto &monitorDetID : monitorDetIDs) {
      TSM_ASSERT("Should not be in the detID2WSIndexMap.", detIDtoWSIndexMap.count(monitorDetID) == 0);
    }

    for (const auto &monitorDetID : monitorDetIDs) {
      TSM_ASSERT("Should be in the detID2WSIndexMapMon.", detIDtoWSIndexMapMon.count(monitorDetID) == 1);
    }

    for (const auto &neighborToCheck : neighborsToCheck) {
      TSM_ASSERT("Should be in the detID2WSIndexMap.", detIDtoWSIndexMap.count(neighborToCheck) == 1);
    }

    // Check some of the data
    double delta = 1e-6;
    TS_ASSERT_DELTA(ws->y(142)[0], 0.0, delta);
    TS_ASSERT_DELTA(ws->y(142)[1], 82.0, delta);
    TS_ASSERT_DELTA(ws->y(142)[2], 57.0, delta);
    TS_ASSERT_DELTA(ws->y(142)[17034], 5.0, delta);
    TS_ASSERT_DELTA(ws->y(142)[17035], 8.0, delta);

    TS_ASSERT_DELTA(ws->y(143)[0], 0.0, delta);
    TS_ASSERT_DELTA(ws->y(143)[1], 0.0, delta);
    TS_ASSERT_DELTA(ws->y(143)[2], 0.0, delta);

    TS_ASSERT_DELTA(mon_ws->y(0)[0], 0.0, delta);
    TS_ASSERT_DELTA(mon_ws->y(0)[1], 176660.0, delta);
    TS_ASSERT_DELTA(mon_ws->y(0)[2], 57659.0, delta);
    TS_ASSERT_DELTA(mon_ws->y(0)[17034], 4851.0, delta);
    TS_ASSERT_DELTA(mon_ws->y(0)[17035], 4513.0, delta);

    // Check that the data has the expected spectrum number and the expected
    // detecor ID
    TSM_ASSERT_EQUALS("Detector at WS index 142 should have a spectrum number of 143", 143,
                      ws->getSpectrum(142).getSpectrumNo());
    TSM_ASSERT_EQUALS("Detector at WS index 143 should have a spectrum number of 144", 144,
                      ws->getSpectrum(143).getSpectrumNo());

    const auto &spectrumInfo = ws->spectrumInfo();
    TSM_ASSERT_EQUALS("Detector at WS index 142 should have a detector ID of 143", 143,
                      spectrumInfo.detector(142).getID());
    TSM_ASSERT_EQUALS("Detector at WS index 143 should have a detector ID of 144", 144,
                      spectrumInfo.detector(143).getID());

    TSM_ASSERT_EQUALS("Monitor at WS index 0 should have a spectrum number of 145", 145,
                      mon_ws->getSpectrum(0).getSpectrumNo());

    const auto &monSpectrumInfo = mon_ws->spectrumInfo();
    TSM_ASSERT_EQUALS("Monitor at WS index 0 should have a detector ID of 145", 145,
                      monSpectrumInfo.detector(0).getID());

    // Clean up
    AnalysisDataService::Instance().remove("outWS");
    AnalysisDataService::Instance().remove("outWS_monitors");
  }

  void
  test_that_non_contiguous_data_loads_selected_monitors_for_separate_monitors_and_spectra_range_and_spectra_list() {
    /*
    Monitors can be found at detID: 145, 146, 147, 148
    */
    // Arrange

    LoadISISNexus2 ld;
    ld.initialize();
    ld.setPropertyValue("Filename", "INS09161.nxs");
    ld.setPropertyValue("OutputWorkspace", "outWS");
    ld.setPropertyValue("LoadMonitors", "1"); /*separate monitors*/
    ld.setPropertyValue("SpectrumMin", "50");
    ld.setPropertyValue("SpectrumMax", "73");
    ld.setPropertyValue("SpectrumList", "12, 145");

    // Act
    TS_ASSERT_THROWS_NOTHING(ld.execute());
    TS_ASSERT(ld.isExecuted());

    // Assert
    MatrixWorkspace_sptr ws = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>("outWS");
    MatrixWorkspace_sptr mon_ws = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>("outWS_monitors");

    TSM_ASSERT_EQUALS("Should have 17036 bins", ws->blocksize(), 17036);
    TSM_ASSERT_EQUALS("Should have 25 detectors", ws->getNumberHistograms(), 25);

    TSM_ASSERT_EQUALS("Monitor workspace should have 17036 bins", mon_ws->blocksize(), 17036);
    TSM_ASSERT_EQUALS("Monitor workspace should have 1 detector, hence "
                      "respecting the selection",
                      mon_ws->getNumberHistograms(), 1);

    // Check elements in workspace
    auto detIDtoWSIndexMap = ws->getDetectorIDToWorkspaceIndexMap();

    // Range from 1 to 11
    for (Mantid::detid_t detID = 1; detID < 12; ++detID) {
      TSM_ASSERT("Should not be in workspace", detIDtoWSIndexMap.count(detID) == 0);
    }

    // 12
    TSM_ASSERT("Should be in workspace", detIDtoWSIndexMap.count(12) == 1);

    // Range from 13 t0 49
    for (Mantid::detid_t detID = 13; detID < 50; ++detID) {
      TSM_ASSERT("Should not be in workspace", detIDtoWSIndexMap.count(detID) == 0);
    }

    // Range from 50 to 73
    for (Mantid::detid_t detID = 50; detID < 74; ++detID) {
      TSM_ASSERT("Should be in workspace", detIDtoWSIndexMap.count(detID) == 1);
    }

    // Range from 74 to 144
    for (Mantid::detid_t detID = 74; detID < 145; ++detID) {
      TSM_ASSERT("Should not be in workspace", detIDtoWSIndexMap.count(detID) == 0);
    }

    // 145
    TSM_ASSERT("Should not be in workspace", detIDtoWSIndexMap.count(145) == 0);

    // Range from 146 to 172 (which is the number of detectors + monitors)
    for (Mantid::detid_t detID = 146; detID <= 172; ++detID) {
      TSM_ASSERT("Should not be in workspace", detIDtoWSIndexMap.count(detID) == 0);
    }

    TSM_ASSERT_EQUALS("Detector at WS index 2 should have a spectrum number of 51", 51,
                      ws->getSpectrum(2).getSpectrumNo());
    const auto &spectrumInfo = ws->spectrumInfo();
    TSM_ASSERT_EQUALS("Detector at WS index 2 should have a detector ID of 51", 51, spectrumInfo.detector(2).getID());

    // Test the monitor workspace
    TSM_ASSERT_EQUALS("Detector at WS index 0 should have a spectrum number of 145", 145,
                      mon_ws->getSpectrum(0).getSpectrumNo());
    const auto &monSpectrumInfo = mon_ws->spectrumInfo();
    TSM_ASSERT_EQUALS("Detector at WS index 0 should have a detector ID of 145", 145,
                      monSpectrumInfo.detector(0).getID());

    // Clean up
    AnalysisDataService::Instance().remove("outWS");
    AnalysisDataService::Instance().remove("outWS_monitors");
  }

  void test_that_non_contiguous_data_loads_all_monitors_for_separate_monitors_and_spectra_range_and_spectra_list() {
    /*
    Monitors can be found at detID: 145, 146, 147, 148
    */
    // Arrange

    LoadISISNexus2 ld;
    ld.initialize();
    ld.setPropertyValue("Filename", "INS09161.nxs");
    ld.setPropertyValue("OutputWorkspace", "outWS");
    ld.setPropertyValue("LoadMonitors", "1"); /*separate monitors*/
    ld.setPropertyValue("SpectrumMin", "50");
    ld.setPropertyValue("SpectrumMax", "73");
    ld.setPropertyValue("SpectrumList", "12");

    // Act
    TS_ASSERT_THROWS_NOTHING(ld.execute());
    TS_ASSERT(ld.isExecuted());

    // Assert
    MatrixWorkspace_sptr ws = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>("outWS");
    MatrixWorkspace_sptr mon_ws = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>("outWS_monitors");

    TSM_ASSERT_EQUALS("Should have 17036 bins", ws->blocksize(), 17036);
    TSM_ASSERT_EQUALS("Should have 25 detectors", ws->getNumberHistograms(), 25);

    TSM_ASSERT_EQUALS("Monitor workspace should have 17036 bins", mon_ws->blocksize(), 17036);
    TSM_ASSERT_EQUALS("Monitor workspace should have 4 detector, since none "
                      "was specifically selected",
                      mon_ws->getNumberHistograms(), 4);

    // Check elements in workspace
    auto detIDtoWSIndexMap = ws->getDetectorIDToWorkspaceIndexMap();

    // Range from 1 to 11
    for (Mantid::detid_t detID = 1; detID < 12; ++detID) {
      TSM_ASSERT("Should not be in workspace", detIDtoWSIndexMap.count(detID) == 0);
    }

    // 12
    TSM_ASSERT("Should be in workspace", detIDtoWSIndexMap.count(12) == 1);

    // Range from 13 t0 49
    for (Mantid::detid_t detID = 13; detID < 50; ++detID) {
      TSM_ASSERT("Should not be in workspace", detIDtoWSIndexMap.count(detID) == 0);
    }

    // Range from 50 to 73
    for (Mantid::detid_t detID = 50; detID < 74; ++detID) {
      TSM_ASSERT("Should be in workspace", detIDtoWSIndexMap.count(detID) == 1);
    }

    // Range from 74 to 144
    for (Mantid::detid_t detID = 74; detID < 145; ++detID) {
      TSM_ASSERT("Should not be in workspace", detIDtoWSIndexMap.count(detID) == 0);
    }

    // 145
    TSM_ASSERT("Should not be in workspace", detIDtoWSIndexMap.count(145) == 0);

    // Range from 146 to 172 (which is the number of detectors + monitors)
    for (Mantid::detid_t detID = 146; detID <= 172; ++detID) {
      TSM_ASSERT("Should not be in workspace", detIDtoWSIndexMap.count(detID) == 0);
    }

    TSM_ASSERT_EQUALS("Detector at WS index 2 should have a spectrum number of 51", 51,
                      ws->getSpectrum(2).getSpectrumNo());
    const auto &spectrumInfo = ws->spectrumInfo();
    TSM_ASSERT_EQUALS("Detector at WS index 2 should have a detector ID of 51", 51, spectrumInfo.detector(2).getID());

    // Test the monitor workspace
    TSM_ASSERT_EQUALS("Detector at WS index 2 should have a spectrum number of 147", 147,
                      mon_ws->getSpectrum(2).getSpectrumNo());
    const auto &monSpectrumInfo = mon_ws->spectrumInfo();
    TSM_ASSERT_EQUALS("Detector at WS index 2 should have a detector ID of 147", 147,
                      monSpectrumInfo.detector(2).getID());

    // Clean up
    AnalysisDataService::Instance().remove("outWS");
    AnalysisDataService::Instance().remove("outWS_monitors");
  }

  void
  test_that_only_monitors_load_in_original_workspace_for_separate_monitors_when_spectra_list_only_contains_monitors() {
    /*
    Monitors can be found at detID: 145, 146, 147, 148
    */
    // Arrange

    LoadISISNexus2 ld;
    ld.initialize();
    ld.setRethrows(true);
    ld.setPropertyValue("Filename", "INS09161.nxs");
    ld.setPropertyValue("OutputWorkspace", "outWS");
    ld.setPropertyValue("LoadMonitors", "1"); /*separate monitors*/
    ld.setPropertyValue("SpectrumList", "145, 147");

    // Act
    TS_ASSERT_THROWS_NOTHING(ld.execute());
    TS_ASSERT(ld.isExecuted());

    // Assert
    MatrixWorkspace_sptr ws = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>("outWS");

    TSM_ASSERT_THROWS_ANYTHING("We should not see the creation of a separate monitor workspace",
                               AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>("outWS_monitors"));

    TSM_ASSERT_EQUALS("Should have 17036 bins", ws->blocksize(), 17036);
    TSM_ASSERT_EQUALS("Should have 2 detectors", ws->getNumberHistograms(), 2);

    const auto &spectrumInfo = ws->spectrumInfo();

    TSM_ASSERT_EQUALS("Monitor at WS index 0 should have a spectrum number of 145", 145,
                      ws->getSpectrum(0).getSpectrumNo());
    TSM_ASSERT_EQUALS("Monitor at WS index 0 should have a detector ID of 145", 145, spectrumInfo.detector(0).getID());
    TSM_ASSERT_EQUALS("Monitor at WS index 1 should have a spectrum number of 147", 147,
                      ws->getSpectrum(1).getSpectrumNo());
    TSM_ASSERT_EQUALS("Monitor at WS index 1 should have a detector ID of 147", 147, spectrumInfo.detector(1).getID());

    // Clean up
    AnalysisDataService::Instance().remove("outWS");
  }

  void test_that_non_contiguous_data_loads_only_monitors_in_parts_when_only_lower_bound_is_specfied() {
    /*
    Monitors can be found at detID: 145, 146, 147, 148
    */
    // Arrange

    LoadISISNexus2 ld;
    ld.initialize();
    ld.setPropertyValue("Filename", "INS09161.nxs");
    ld.setPropertyValue("OutputWorkspace", "outWS");
    ld.setPropertyValue("LoadMonitors", "1"); /*separate monitors*/
    ld.setPropertyValue("SpectrumMin", "50"); /* Note that we don't specify a max*/
    ld.setPropertyValue("SpectrumList", "145");

    // Act
    TS_ASSERT_THROWS_NOTHING(ld.execute());
    TS_ASSERT(ld.isExecuted());

    // Assert
    MatrixWorkspace_sptr ws = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>("outWS");
    MatrixWorkspace_sptr mon_ws = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>("outWS_monitors");

    TSM_ASSERT_EQUALS("Should have 17036 bins", ws->blocksize(), 17036);
    TSM_ASSERT_EQUALS("Should have 110(172-50 +1 -4) detectors", ws->getNumberHistograms(), 119);

    TSM_ASSERT_EQUALS("Monitor workspace should have 17036 bins", mon_ws->blocksize(), 17036);
    TSM_ASSERT_EQUALS("Monitor workspace should have 4 detector, since none "
                      "was specifically selected",
                      mon_ws->getNumberHistograms(), 4);

    // Check elements in workspace
    auto detIDtoWSIndexMap = ws->getDetectorIDToWorkspaceIndexMap();

    // Range from 1 to 49
    for (Mantid::detid_t detID = 1; detID < 50; ++detID) {
      TSM_ASSERT("Should not be in workspace", detIDtoWSIndexMap.count(detID) == 0);
    }

    // Range from 50 to 144
    for (Mantid::detid_t detID = 50; detID < 74; ++detID) {
      TSM_ASSERT("Should be in workspace", detIDtoWSIndexMap.count(detID) == 1);
    }

    // 145, 146, 147, 148
    TSM_ASSERT("Should not be in workspace", detIDtoWSIndexMap.count(145) == 0);
    TSM_ASSERT("Should not be in workspace", detIDtoWSIndexMap.count(146) == 0);
    TSM_ASSERT("Should not be in workspace", detIDtoWSIndexMap.count(147) == 0);
    TSM_ASSERT("Should not be in workspace", detIDtoWSIndexMap.count(148) == 0);

    // Range from 146 to 172 (which is the number of detectors + monitors)
    for (Mantid::detid_t detID = 149; detID <= 172; ++detID) {
      TSM_ASSERT("Should not be in workspace", detIDtoWSIndexMap.count(detID) == 1);
    }

    TSM_ASSERT_EQUALS("Detector at WS index 2 should have a spectrum number of 52", 52,
                      ws->getSpectrum(2).getSpectrumNo());

    const auto &spectrumInfo = ws->spectrumInfo();
    TSM_ASSERT_EQUALS("Detector at WS index 2 should have a detector ID of 52", 52, spectrumInfo.detector(2).getID());

    // Clean up
    AnalysisDataService::Instance().remove("outWS");
    AnalysisDataService::Instance().remove("outWS_monitors");
  }

  void
  test_that_non_contiguous_data_loads_only_monitors_in_parts_when_only_lower_bound_is_specified_without_spectra_list() {
    /*
    Monitors can be found at detID: 145, 146, 147, 148
    */
    // Arrange

    LoadISISNexus2 ld;
    ld.initialize();
    ld.setPropertyValue("Filename", "LOQ49886.nxs");
    ld.setPropertyValue("OutputWorkspace", "outWS");
    ld.setPropertyValue("LoadMonitors", "1"); /*separate monitors*/
    ld.setPropertyValue("SpectrumMin", "2");  /* Note that we don't specify a max*/

    // Act
    TS_ASSERT_THROWS_NOTHING(ld.execute());
    TS_ASSERT(ld.isExecuted());

    // Assert
    MatrixWorkspace_sptr ws = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>("outWS");
    MatrixWorkspace_sptr mon_ws = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>("outWS_monitors");

    TSM_ASSERT_EQUALS("Should have 5 bins", ws->blocksize(), 5);
    TSM_ASSERT_EQUALS("Should have 17790 detectors", ws->getNumberHistograms(), 17790);

    TSM_ASSERT_EQUALS("Monitor workspace should have 5 bins", mon_ws->blocksize(), 5);
    TSM_ASSERT_EQUALS("Monitor workspace should have 1 detector, since none "
                      "was specifically selected",
                      mon_ws->getNumberHistograms(), 1);

    // Check some samples
    const auto &spectrumInfo = ws->spectrumInfo();
    TSM_ASSERT_EQUALS("Detector at WS index 2 should have a spectrum number of 5", 5,
                      ws->getSpectrum(2).getSpectrumNo());
    TSM_ASSERT_EQUALS("Detector at WS index 2 should have a detector ID of 5", 5, spectrumInfo.detector(2).getID());
    TSM_ASSERT_EQUALS("Detector at WS index 3 should have a spectrum number of 6", 6,
                      ws->getSpectrum(3).getSpectrumNo());
    TSM_ASSERT_EQUALS("Detector at WS index 3 should have a detector ID of 6", 6, spectrumInfo.detector(3).getID());

    TSM_ASSERT_EQUALS("Detector at WS index 0 should have a spectrum number of 2", 2,
                      mon_ws->getSpectrum(0).getSpectrumNo());
    const auto &monSpectrumInfo = mon_ws->spectrumInfo();
    TSM_ASSERT_EQUALS("Detector at WS index 0 should have a detector ID of 2", 2, monSpectrumInfo.detector(0).getID());

    // Clean up
    AnalysisDataService::Instance().remove("outWS");
    AnalysisDataService::Instance().remove("outWS_monitors");
  }

  void test_that_multiple_time_regime_file_is_detected_and_loads() {
    // Arrange
    LoadISISNexus2 ld;
    ld.setRethrows(true);
    ld.initialize();
    ld.setPropertyValue("Filename", "LARMOR00009848.nxs");
    ld.setPropertyValue("OutputWorkspace", "outWS");
    ld.setPropertyValue("LoadMonitors", "1"); /*separate monitors*/
    TS_ASSERT_THROWS_NOTHING(ld.execute());
    TS_ASSERT(ld.isExecuted());
  }

  //------------------------------------------------------------------
  // Exceptions
  //------------------------------------------------------------------
  void test_that_when_selecting_range_with_only_monitors_and_exclude_monitors_exception_is_thrown() {
    // Scenario:
    // Data:    |--Mon--||--Det--||--Mon--||--Det--|
    // Select:   |  |
    // Arrange

    LoadISISNexus2 ld;
    ld.setRethrows(true);
    ld.initialize();
    ld.setPropertyValue("Filename", "LOQ49886.nxs");
    ld.setPropertyValue("OutputWorkspace", "outWS");
    ld.setPropertyValue("SpectrumMin", "1");
    ld.setPropertyValue("SpectrumMax", "1");
    ld.setPropertyValue("LoadMonitors", "0"); /*excluded monitors*/

    // Act + Assert
    TSM_ASSERT_THROWS("Should throw, since it does not makes sense to only "
                      "select monitors, but to also exclude them",
                      ld.execute(), const std::invalid_argument &);
  }

  void test_that_when_selecting_range_with_only_monitors_in_the_middle_and_exclude_monitors_exception_is_thrown() {
    // Scenario:
    // Data:    |--Mon--||--Det--||--Mon--||--Det--|
    // Select:                      |  |
    // Arrange

    LoadISISNexus2 ld;
    ld.setRethrows(true);
    ld.initialize();
    ld.setPropertyValue("Filename", "INS09161.nxs");
    ld.setPropertyValue("OutputWorkspace", "outWS");
    ld.setPropertyValue("SpectrumList", "145");
    ld.setPropertyValue("LoadMonitors", "0"); /*excluded monitors*/

    // Act + Assert
    TSM_ASSERT_THROWS("Should throw, since it does not makes sense to only "
                      "select monitors, but to also exclude them",
                      ld.execute(), const std::invalid_argument &);
  }

  void test_that_when_selecting_list_with_only_monitors_and_exclude_monitors_exception_is_thrown() {
    // Arrange

    LoadISISNexus2 ld;
    ld.setRethrows(true);
    ld.initialize();
    ld.setPropertyValue("Filename", "LOQ49886.nxs");
    ld.setPropertyValue("OutputWorkspace", "outWS");
    ld.setPropertyValue("SpectrumList", "1");
    ld.setPropertyValue("LoadMonitors", "0"); /*excluded monitors*/

    // Act + Assert
    TSM_ASSERT_THROWS("Should throw, since it does not makes sense to only "
                      "select monitors, but to also exclude them",
                      ld.execute(), const std::invalid_argument &);
  }

  void testReadNotes() {

    LoadISISNexus2 load;
    load.initialize();
    load.setPropertyValue("Filename", "EMU00102347.nxs_v2");
    load.setPropertyValue("OutputWorkspace", "outWS");
    load.setPropertyValue("SpectrumMin", "10");
    load.setPropertyValue("SpectrumMax", "20");
    TS_ASSERT_THROWS_NOTHING(load.execute());
    TS_ASSERT(load.isExecuted());

    MatrixWorkspace_sptr ws = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>("outWS");
    TS_ASSERT_EQUALS(ws->getComment(), "TF20 as insert cools");
  }

  void testDoesNotFailWhenNoNotes() {

    LoadISISNexus2 load;
    load.initialize();
    load.setPropertyValue("Filename", "LOQ49886.nxs");
    load.setPropertyValue("OutputWorkspace", "outWS");
    load.setPropertyValue("SpectrumMin", "10");
    load.setPropertyValue("SpectrumMax", "20");
    TS_ASSERT_THROWS_NOTHING(load.execute());
    TS_ASSERT(load.isExecuted());

    MatrixWorkspace_sptr ws = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>("outWS");
    TS_ASSERT_EQUALS(ws->getComment(), "");
  }
};

//------------------------------------------------------------------------------
// Performance test
//------------------------------------------------------------------------------

class LoadISISNexusTestPerformance : public CxxTest::TestSuite {
public:
  void testDefaultLoad() {
    LoadISISNexus2 loader;
    loader.initialize();
    loader.setPropertyValue("Filename", "LOQ49886.nxs");
    loader.setPropertyValue("OutputWorkspace", "ws");
    TS_ASSERT(loader.execute());
  }
};
