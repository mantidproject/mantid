// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <cxxtest/TestSuite.h>

#include "MantidAPI/Algorithm.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/InstrumentDataService.h"
#include "MantidAPI/SpectrumInfo.h"
#include "MantidDataHandling/GroupDetectors2.h"
#include "MantidDataHandling/LoadInstrument.h"
#include "MantidDataHandling/LoadInstrumentFromNexus.h"
#include "MantidDataHandling/UpdateInstrumentFromFile.h"
#include "MantidDataObjects/WorkspaceCreation.h"
#include "MantidGeometry/Instrument.h"
#include "MantidGeometry/Instrument/DetectorGroup.h"
#include "MantidIndexing/IndexInfo.h"

#include "MantidTestHelpers/ComponentCreationHelper.h"
#include "MantidTestHelpers/ScopedFileHelper.h"

using Mantid::DataHandling::UpdateInstrumentFromFile;

class UpdateInstrumentFromFileTest : public CxxTest::TestSuite {
public:
  static UpdateInstrumentFromFileTest *createSuite() { return new UpdateInstrumentFromFileTest(); }
  static void destroySuite(UpdateInstrumentFromFileTest *suite) { delete suite; }

  UpdateInstrumentFromFileTest() : wsName("UpdateInstrumentFromFileTestWS"), xmlFile("HRPD_for_UNIT_TESTING.xml") {}

  void test_Using_RAW_File() {
    using namespace Mantid::API;

    FrameworkManager::Instance().exec("LoadRaw", 4, "Filename", "IRS26173.raw", "OutputWorkspace", wsName.c_str());

    runUpdateInstrument("IRS26173.raw");

    MatrixWorkspace_sptr output;
    TS_ASSERT_THROWS_NOTHING(output = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(wsName));
    const auto &spectrumInfo = output->spectrumInfo();
    auto det10Pos = spectrumInfo.position(10);
    TS_ASSERT_DELTA(det10Pos.X(), 1.06477, 1e-4);
    TS_ASSERT_DELTA(det10Pos.Y(), 0.0, 1e-4);
    TS_ASSERT_DELTA(det10Pos.Z(), 0.984261, 1e-4);

    // Now try monitors
    runUpdateInstrument("IRS26173.raw", "", false,
                        true); // RAW,ascii header,ignorePhi,moveMonitors

    TS_ASSERT_THROWS_NOTHING(output = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(wsName));
    const auto &spectrumInfo2 = output->spectrumInfo();
    auto det0Pos = spectrumInfo2.position(0);
    TS_ASSERT_DELTA(det0Pos.X(), 0.0, 1e-4);
    TS_ASSERT_DELTA(det0Pos.Y(), 0.0, 1e-4);
    TS_ASSERT_DELTA(det0Pos.Z(), 0.355, 1e-4);

    AnalysisDataService::Instance().remove(wsName);
    InstrumentDataService::Instance().remove(xmlFile);
  }

  void test_Using_Ascii_File_With_No_Header_In_File() {
    const std::string header = "spectrum,theta,t0,-,R";
    const std::string contents = "    3 130.4653  -0.4157  11.0050   0.6708\n"
                                 "    4 131.9319  -0.5338  11.0050   0.6545";

    const std::string filename = "__detpars.par";
    ScopedFileHelper::ScopedFile datfile(contents, filename);

    doTestWithAsciiFile(datfile.getFileName(), header);
  }

  void test_Using_Ascii_File_With_Header_Lines_At_Top_Of_File_Skips_These_Lines() {
    const std::string colNames = "spectrum,theta,t0,-,R";
    const std::string contents = "plik det  t0 l0 l1\n"
                                 "    3 130.4653  -0.4157  11.0050   0.6708\n"
                                 "    4 131.9319  -0.5338  11.0050   0.6545";

    const std::string filename = "__detpars_with_header.par";
    ScopedFileHelper::ScopedFile parfile(contents, filename);

    doTestWithAsciiFile(parfile.getFileName(), colNames);
  }

  void test_DAT_Extension_Without_AsciiHeader_Assumes_Detector_Calibration_File() {
    const std::string contents = "DETECTOR.DAT generated by CREATE_DETECTOR_FILE\n"
                                 "163848      14\n"
                                 "det no.  offset    l2     code     theta        phi         w_x       "
                                 "  w_y         w_z         f_x         f_y         f_z         a_x     "
                                 "    a_y         a_z        det_1       det_2       det_3       det4\n"
                                 "3 0.0 0.6708 5 130.4653 179.0 0 0 0 0 0 0 0 0 0 0 0 0 0\n"
                                 "4 5.3 0.6545 5 131.9319 179.0 0 0 0 0 0 0 0 0 0 0 0 0 0";

    const std::string filename = "__detpars_with_header.dat";
    ScopedFileHelper::ScopedFile datfile(contents, filename);

    loadTestInstrument();
    // No header
    TS_ASSERT_THROWS_NOTHING(runUpdateInstrument(datfile.getFileName()));

    using namespace Mantid::API;
    // Instrument check
    MatrixWorkspace_sptr output;
    TS_ASSERT_THROWS_NOTHING(output = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(wsName));

    const auto &spectrumInfo = output->spectrumInfo();
    auto det4Position = spectrumInfo.position(3); // spectrum 4 = ws index 3
    double r(-1.0), theta(-1.0), phi(-1.0);
    det4Position.getSpherical(r, theta, phi);
    TS_ASSERT_DELTA(0.6545, r, 1e-4);
    TS_ASSERT_DELTA(131.9319, theta, 1e-4);
    TS_ASSERT_DELTA(179.0, phi, 1e-4);

    AnalysisDataService::Instance().remove(wsName);
  }

  void test_DAT_Extension_Without_Header_Assumes_Detector_Calibration_File_And_Fails_If_Number_Of_Columns_Incorrect() {
    const std::string contents = "plik det  t0 l0 l1\n"
                                 "    3 130.4653  -0.4157  11.0050   0.6708\n"
                                 "    4 131.9319  -0.5338  11.0050   0.6545";

    const std::string filename = "__detpars_with_header.dat";
    ScopedFileHelper::ScopedFile datfile(contents, filename);

    loadTestInstrument();
    // No header
    TS_ASSERT_THROWS(runUpdateInstrument(datfile.getFileName()), const std::runtime_error &);
  }

  void test_GroupedDetector_Has_All_Components_Moved_To_Same_Coordinates() {
    using namespace Mantid::API;
    using namespace Mantid::Geometry;
    const std::string colNames = "spectrum,theta,t0,-,R";
    const std::string contents = "plik det  t0 l0 l1\n"
                                 "    0 130.4653  -0.4157  11.0050   0.6708\n";

    const std::string filename = "__detpars_with_header.dat";
    ScopedFileHelper::ScopedFile datfile(contents, filename);

    const bool groupDetectors(true);
    const bool ignorePhi(true);
    loadTestInstrument(groupDetectors);
    runUpdateInstrument(datfile.getFileName(), colNames, ignorePhi);

    auto output = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(wsName);
    const auto &spectrumInfo = output->spectrumInfo();
    const auto &detector = spectrumInfo.detector(0);

    auto canCastToGroup(true);
    try {
      const auto &group = dynamic_cast<const DetectorGroup &>(detector);
      double expectedR(0.6708), expectedTheta(130.4653), expectedPhi(-90.0);
      const auto dets = group.getDetectors();
      for (const auto &comp : dets) {
        double r(-1.0), theta(-1.0), phi(-1.0);
        comp->getPos().getSpherical(r, theta, phi);
        TS_ASSERT_DELTA(expectedR, r, 1e-4);
        TS_ASSERT_DELTA(expectedTheta, theta, 1e-4);
        TS_ASSERT_DELTA(expectedPhi, phi, 1e-4);

        std::vector<double> par = comp->getNumberParameter("t0");
        TSM_ASSERT_EQUALS("Expected a single t0 parameter", 1, par.size());
        TS_ASSERT_DELTA(par[0], -0.4157, 1e-4)
      }
    } catch (const std::bad_cast &) {
      canCastToGroup = false;
    }
    TS_ASSERT(canCastToGroup);

    AnalysisDataService::Instance().remove(wsName);
  }

private:
  void doTestWithAsciiFile(const std::string &filename, const std::string &header) {
    using namespace Mantid::API;
    using namespace Mantid::Geometry;

    loadTestInstrument();
    // No header
    TS_ASSERT_THROWS(runUpdateInstrument(filename), const std::invalid_argument &);

    // Header claims fewer columns than there actually are
    std::string badHeader = "spectrum,theta,t0,-";
    checkErrorMessageFromBadHeader("UpdateInstrumentFromFile::updateFromAscii "
                                   "- File contains more than expected number "
                                   "of columns, check AsciiHeader property.",
                                   filename, badHeader);

    // Header claims more columns than there actually are
    badHeader = "spectrum,theta,t0,-,R,something";
    checkErrorMessageFromBadHeader("UpdateInstrumentFromFile::updateFromAscii "
                                   "- File contains fewer than expected number "
                                   "of columns, check AsciiHeader property.",
                                   filename, badHeader);

    // Just right...
    TS_ASSERT_THROWS_NOTHING(runUpdateInstrument(filename, header));
    // Instrument check
    MatrixWorkspace_sptr output;
    TS_ASSERT_THROWS_NOTHING(output = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(wsName));
    auto spectrumInfo = output->spectrumInfo();
    auto det4Position = spectrumInfo.position(3); // spectrum 4 = ws index 3

    double r(-1.0), theta(-1.0), phi(-1.0);
    det4Position.getSpherical(r, theta, phi);
    TS_ASSERT_DELTA(0.6545, r, 1e-4);
    TS_ASSERT_DELTA(131.9319, theta, 1e-4);
    TS_ASSERT_DELTA(180.0, phi, 1e-4);

    const auto &det4 = spectrumInfo.detector(3);
    std::vector<double> par = det4.getNumberParameter("t0");
    TS_ASSERT_EQUALS(1, par.size());
    if (par.size() == 1) {
      TS_ASSERT_DELTA(par[0], -0.5338, 1e-4)
    }
    // Check that the "-" column was skipped
    par = det4.getNumberParameter("-");
    TS_ASSERT_EQUALS(0, par.size());

    AnalysisDataService::Instance().remove(wsName);
  }

  void loadTestInstrument(bool grouped = false) {
    using namespace Mantid::API;
    using namespace Mantid::Geometry;
    using namespace Mantid::DataObjects;
    using namespace Mantid::Indexing;
    using namespace Mantid::HistogramData;

    const size_t nhist(9);
    auto ws =
        create<Workspace2D>(ComponentCreationHelper::createTestInstrumentCylindrical(1), IndexInfo(nhist), Points(1));

    AnalysisDataService::Instance().addOrReplace(wsName, std::move(ws));
    if (grouped) {
      using Mantid::DataHandling::GroupDetectors2;
      GroupDetectors2 alg;
      alg.initialize();
      alg.setProperty("InputWorkspace", wsName);
      alg.setProperty("OutputWorkspace", wsName);
      alg.setProperty("SpectraList", "1-9");
      alg.execute();
    }
  }

  void runUpdateInstrument(const std::string &filename, const std::string &header = "", const bool ignorePhi = false,
                           const bool moveMonitors = false) {
    UpdateInstrumentFromFile updater;
    updater.initialize();
    updater.setPropertyValue("Workspace", wsName);
    updater.setPropertyValue("Filename", filename);
    updater.setProperty("IgnorePhi", ignorePhi);
    updater.setProperty("MoveMonitors", moveMonitors);
    if (!header.empty()) {
      updater.setPropertyValue("AsciiHeader", header);
    }
    updater.setRethrows(true);
    updater.execute();
  }

  void checkErrorMessageFromBadHeader(const std::string &expectedMsg, const std::string &filename,
                                      const std::string &header) {
    try {
      runUpdateInstrument(filename, header);
      TS_FAIL("Expected runUpdateInstrument to throw but it didn't.");
    } catch (std::runtime_error &exc) {
      TSM_ASSERT_EQUALS("runUpdateInstrument threw a std::runtime_error but "
                        "the message is not the expected message",
                        expectedMsg, std::string(exc.what()));
    } catch (...) {
      TS_FAIL("Expected runUpdateInstrument to throw a std::runtime_error but "
              "it threw something else.");
    }
  }

private:
  std::string wsName;
  std::string xmlFile;
};
