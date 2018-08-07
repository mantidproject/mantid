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
    TS_ASSERT_EQUALS(ws->getLog("Field")->value(), "0.000G");
    TS_ASSERT_EQUALS(
        ws->getComment(),
        "Ba3NbFe3Si2O14, crystal                                       ");
    TS_ASSERT_EQUALS(ws->getLog("Actual Temperature1")->value(), "4.999610");
    TS_ASSERT_EQUALS(ws->getLog("Actual Temperature2")->value(), "5.197690");
    TS_ASSERT_EQUALS(ws->getLog("end_time")->value(), "2011-07-04T11:56:24");
    TS_ASSERT_EQUALS(ws->getLog("start_time")->value(), "2011-07-04T10:40:23");
    TS_ASSERT_EQUALS(ws->getLog("Spectra0 label and scalar")->value(),
                     "Forw - 14493858");
    TS_ASSERT_EQUALS(ws->getLog("Spectra3 label and scalar")->value(),
                     "Rite - 38247601");
    TS_ASSERT_EQUALS(ws->getLog("Length of Run")->value(),
                     "10.000000MicroSeconds");
    TS_ASSERT_EQUALS(ws->getLog("Set Temperature")->value(), "5.000K");

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

private:
  std::string getTestFilePath(const std::string &filename) {
    const std::string filepath =
        Mantid::API::FileFinder::Instance().getFullPath(filename);
    TS_ASSERT_DIFFERS(filepath, "");
    return filepath;
  }
};

#endif /* MANTID_DATAHANDLING_LOADPSIMUONBINTEST_H_ */