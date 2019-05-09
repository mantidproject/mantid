// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_DATAHANDLING_LOADPSIMUONBINTEST_H_
#define MANTID_DATAHANDLING_LOADPSIMUONBINTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/FileFinder.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/Sample.h"
#include "MantidDataHandling/LoadPSIMuonBin.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidKernel/FileDescriptor.h"

using namespace Mantid;
using namespace Mantid::DataHandling;
using namespace Mantid::API;
using namespace Mantid::Kernel;

class LoadPSIMuonBinTest : public CxxTest::TestSuite {

public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static LoadPSIMuonBinTest *createSuite() { return new LoadPSIMuonBinTest(); }
  static void destroySuite(LoadPSIMuonBinTest *suite) { delete suite; }

  void test_Init() {
    LoadPSIMuonBin alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())

    TS_ASSERT_THROWS_NOTHING(alg.setProperty(
        "Filename", getTestFilePath("deltat_tdc_dolly_1529.bin")));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("OutputWorkspace", "ws"));
  }

  void test_exec() {
    LoadPSIMuonBin alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    TS_ASSERT(alg.isInitialized());

    TS_ASSERT_THROWS_NOTHING(alg.setProperty(
        "Filename", getTestFilePath("deltat_tdc_dolly_1529.bin")));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("OutputWorkspace", "ws"));
    TS_ASSERT_THROWS_NOTHING(alg.execute());

    TS_ASSERT_THROWS_NOTHING(AnalysisDataService::Instance().remove("ws"));
  }

  void test_workspaceParticulars() {
    LoadPSIMuonBin alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    TS_ASSERT(alg.isInitialized());

    TS_ASSERT_THROWS_NOTHING(alg.setProperty(
        "Filename", getTestFilePath("deltat_tdc_dolly_1529.bin")));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("OutputWorkspace", "ws"));
    TS_ASSERT_THROWS_NOTHING(alg.execute());

    MatrixWorkspace_sptr ws;
    TS_ASSERT_THROWS_NOTHING(
        ws = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>("ws"));
    TS_ASSERT(ws);

    TS_ASSERT_EQUALS(ws->getTitle(), "BNFSO      - Run:1529");
    TS_ASSERT_EQUALS(ws->getLog("sample_magn_field")->value(), "0");
    TS_ASSERT_EQUALS(
        ws->getComment(),
        "Ba3NbFe3Si2O14, crystal                                       ");
    TS_ASSERT_DELTA(std::stod(ws->getLog("Spectra 1 Temperature")->value()),
                    4.99961, 0.00001)
    TS_ASSERT_DELTA(std::stod(ws->getLog("Spectra 2 Temperature")->value()),
                    5.19769, 0.00001)
    TS_ASSERT_EQUALS(ws->getLog("end_time")->value(), "2011-07-04T11:56:24");
    TS_ASSERT_EQUALS(ws->getLog("start_time")->value(), "2011-07-04T10:40:23");
    TS_ASSERT_EQUALS(ws->getLog("Label Spectra 0")->value(), "Forw");
    TS_ASSERT_EQUALS(ws->getLog("Scalar Spectra 0")->value(), "14493858");
    TS_ASSERT_EQUALS(ws->getLog("Label Spectra 3")->value(), "Rite");
    TS_ASSERT_EQUALS(ws->getLog("Scalar Spectra 3")->value(), "38247601");
    TS_ASSERT_EQUALS(ws->getLog("Length of Run")->value(), "10");
    TS_ASSERT_EQUALS(ws->getLog("sample_temp")->value(), "5");

    TS_ASSERT_EQUALS(ws->x(0).size(), 10241);
    TS_ASSERT_EQUALS(ws->y(0).size(), 10240);
    TS_ASSERT_EQUALS(ws->e(0).size(), 10240);

    TS_ASSERT_EQUALS(ws->x(0)[0], 0);
    TS_ASSERT_EQUALS(ws->x(0)[10240], 10);
    TS_ASSERT_EQUALS(ws->y(0)[0], 24);
    TS_ASSERT_EQUALS(ws->y(0)[10239], 44);
    TS_ASSERT_EQUALS(ws->e(0)[0], std::sqrt(ws->y(0)[0]));
    TS_ASSERT_EQUALS(ws->e(0)[10239], std::sqrt(ws->y(0)[10239]));

    TS_ASSERT_EQUALS(ws->getNumberHistograms(), 4);

    TS_ASSERT_THROWS_NOTHING(AnalysisDataService::Instance().remove("ws"));
  }

  void test_fileCheck() {
    LoadPSIMuonBin alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    TS_ASSERT(alg.isInitialized());

    TS_ASSERT_THROWS_NOTHING(alg.setProperty(
        "Filename", getTestFilePath("pid_offset_vulcan_new.dat.bin")));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("OutputWorkspace", "ws"));
    TS_ASSERT_THROWS_NOTHING(alg.execute());

    // If algorithm was successful there will be one, we are assuming it won't
    // have been
    TS_ASSERT_THROWS_ANYTHING(
        MatrixWorkspace_sptr ws =
            AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>("ws"));

    TS_ASSERT_THROWS_NOTHING(AnalysisDataService::Instance().remove("ws"));
  }

  void test_confidence() {
    LoadPSIMuonBin alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    TS_ASSERT(alg.isInitialized());

    FileDescriptor descriptor(getTestFilePath("deltat_tdc_dolly_1529.bin"));
    TS_ASSERT_EQUALS(alg.confidence(descriptor), 90);

    FileDescriptor descriptor1(
        getTestFilePath("pid_offset_vulcan_new.dat.bin"));
    TS_ASSERT_EQUALS(alg.confidence(descriptor1), 0);
  }

  void test_outputProperties() {
    LoadPSIMuonBin alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    TS_ASSERT(alg.isInitialized());

    TS_ASSERT_THROWS_NOTHING(alg.setProperty(
        "Filename", getTestFilePath("deltat_tdc_dolly_1529.bin")));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("OutputWorkspace", "ws"));
    TS_ASSERT_THROWS_NOTHING(alg.execute());

    double firstGoodData = std::stod(alg.getPropertyValue("FirstGoodData"));
    double lastGoodData = std::stod(alg.getPropertyValue("LastGoodData"));
    double timeZero = std::stod(alg.getPropertyValue("TimeZero"));

    TS_ASSERT_DELTA(firstGoodData, 0.167, 0.001);
    TS_ASSERT_DELTA(lastGoodData, 9.989, 0.001);
    TS_ASSERT_DELTA(timeZero, 0.158, 0.001);
  }

  void test_temperatureFileLoaded() {
    LoadPSIMuonBin alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    TS_ASSERT(alg.isInitialized());

    TS_ASSERT_THROWS_NOTHING(alg.setProperty(
        "Filename", getTestFilePath("deltat_tdc_dolly_1529.bin")));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("OutputWorkspace", "ws"));
    TS_ASSERT_THROWS_NOTHING(alg.execute());

    MatrixWorkspace_sptr ws;
    TS_ASSERT_THROWS_NOTHING(
        ws = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>("ws"));
    TS_ASSERT(ws);

    // These values rely on run_1529_templs0.mon being found and loaded by
    // LoadPSIMuonBin
    TS_ASSERT_EQUALS(ws->getLog("Temp_Heater")->value().substr(0, 28),
                     "2011-Jul-04 10:40:23  4.9906")
    TS_ASSERT_EQUALS(ws->getLog("Temp_Analog")->value().substr(0, 28),
                     "2011-Jul-04 10:40:23  5.1805")
    TS_ASSERT_EQUALS(ws->getLog("Temp_ChannelA")->value().substr(0, 28),
                     "2011-Jul-04 10:40:23  4.9921")
    TS_ASSERT_EQUALS(ws->getLog("Temp_ChannelB")->value().substr(0, 28),
                     "2011-Jul-04 10:40:23  5.1804")
    TS_ASSERT_EQUALS(ws->getLog("Temp_ChannelC")->value().substr(0, 28),
                     "2011-Jul-04 10:40:23  314.36")
    TS_ASSERT_EQUALS(ws->getLog("Temp_ChannelD")->value().substr(0, 28),
                     "2011-Jul-04 10:40:23  314.46")
  }

private:
  std::string getTestFilePath(const std::string &filename) {
    const std::string filepath =
        Mantid::API::FileFinder::Instance().getFullPath(filename);
    TS_ASSERT_DIFFERS(filepath, "");
    return filepath;
  }
};

#endif /* MANTID_DATAHANDLING_LOADPSIMUONBINTEST_H_ */