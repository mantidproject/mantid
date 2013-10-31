#ifndef UPDATEINSTRUMENTTESTFROMFILE_H_
#define UPDATEINSTRUMENTTESTFROMFILE_H_

#include <cxxtest/TestSuite.h>

#include "MantidDataHandling/UpdateInstrumentFromFile.h"
#include "MantidDataHandling/LoadInstrumentFromNexus.h"
#include "MantidDataHandling/LoadInstrument.h"
#include "MantidAPI/Algorithm.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/InstrumentDataService.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidGeometry/Instrument.h"

#include "MantidTestHelpers/ComponentCreationHelper.h"
#include "MantidTestHelpers/ScopedFileHelper.h"

using Mantid::DataHandling::UpdateInstrumentFromFile;

class UpdateInstrumentFromFileTest : public CxxTest::TestSuite
{
public:
    static UpdateInstrumentFromFileTest *createSuite() { return new UpdateInstrumentFromFileTest(); }
  static void destroySuite(UpdateInstrumentFromFileTest *suite) { delete suite; }

  UpdateInstrumentFromFileTest()
    : wsName("UpdateInstrumentFromFileTestWS"), xmlFile("HRPD_for_UNIT_TESTING.xml")
  {
  }

  void test_Using_RAW_File()
  {
    using namespace Mantid::API;

    FrameworkManager::Instance().exec("LoadRaw", 4, "Filename", "IRS26173.raw",
                                      "OutputWorkspace", wsName.c_str());

    runUpdateInstrument("IRS26173.raw");

    MatrixWorkspace_sptr output;
    TS_ASSERT_THROWS_NOTHING(output = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(wsName));
    auto det10Pos = output->getDetector(10)->getPos();
    TS_ASSERT_DELTA(det10Pos.X(), 1.06477, 1e-4);
    TS_ASSERT_DELTA(det10Pos.Y(), 0.0, 1e-4);
    TS_ASSERT_DELTA(det10Pos.Z(), 0.984261, 1e-4);

    // Now try monitors
    runUpdateInstrument("IRS26173.raw","",false, true); // RAW,ascii header,ignorePhi,moveMonitors

    TS_ASSERT_THROWS_NOTHING(output = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(wsName));
    auto det0Pos = output->getDetector(0)->getPos();
    TS_ASSERT_DELTA(det0Pos.X(), 0.0, 1e-4);
    TS_ASSERT_DELTA(det0Pos.Y(), 0.0, 1e-4);
    TS_ASSERT_DELTA(det0Pos.Z(), 0.355, 1e-4);

    AnalysisDataService::Instance().remove(wsName);
    InstrumentDataService::Instance().remove(xmlFile);
  }


  void test_Using_Ascii_File_With_No_Header_In_File()
  {
    const std::string header = "spectrum,theta,t0,-,R";
    const std::string contents =
        "    3 130.4653  -0.4157  11.0050   0.6708\n"
        "    4 131.9319  -0.5338  11.0050   0.6545";

    const std::string filename = "__detpars.dat";
    ScopedFileHelper::ScopedFile datfile(contents, filename);

    doTestWithAsciiFile(datfile.getFileName(), header);
  }

  void test_Using_Ascii_File_With_Header_Lines_At_Top_Of_File_Skips_These_Lines()
  {
    const std::string colNames = "spectrum,theta,t0,-,R";
    const std::string contents =
        "plik det  t0 l0 l1\n"
        "    3 130.4653  -0.4157  11.0050   0.6708\n"
        "    4 131.9319  -0.5338  11.0050   0.6545";

    const std::string filename = "__detpars_with_header.dat";
    ScopedFileHelper::ScopedFile datfile(contents, filename);

    doTestWithAsciiFile(datfile.getFileName(), colNames);

  }

private:

  void doTestWithAsciiFile(const std::string & filename, const std::string & header)
  {
    using namespace Mantid::API;
    using namespace Mantid::Geometry;

    loadTestInstrument();
    // No header
    TS_ASSERT_THROWS(runUpdateInstrument(filename), std::invalid_argument);

    // Header claims fewer columns than there actually are
    std::string badHeader = "spectrum,theta,t0,-";
    checkErrorMessageFromBadHeader("UpdateInstrumentFromFile::updateFromAscii - File contains more than expected number of columns, check AsciiHeader property.",
        filename, badHeader);

    // Header claims more columns than there actually are
    badHeader = "spectrum,theta,t0,-,R,something";
    checkErrorMessageFromBadHeader("UpdateInstrumentFromFile::updateFromAscii - File contains fewer than expected number of columns, check AsciiHeader property.",
        filename, badHeader);

    // Just right...
    TS_ASSERT_THROWS_NOTHING(runUpdateInstrument(filename, header));
    // Instrument check
    MatrixWorkspace_sptr output;
    TS_ASSERT_THROWS_NOTHING(output = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(wsName));

    auto det4 = output->getDetector(3); // spectrum 4 = ws index 3
    TS_ASSERT(det4);
    double r(-1.0), theta(-1.0), phi(-1.0);
    det4->getPos().getSpherical(r, theta, phi);
    TS_ASSERT_DELTA(0.6545, r, 1e-4);
    TS_ASSERT_DELTA(131.9319, theta, 1e-4);
    TS_ASSERT_DELTA(180.0,phi,1e-4);

    std::vector<double> par = det4->getNumberParameter("t0");
    TS_ASSERT_EQUALS(1, par.size());
    if(par.size() == 1)
    {
      TS_ASSERT_DELTA(par[0], -0.5338, 1e-4)
    }
    // Check that the "-" column was skipped
    par = det4->getNumberParameter("-");
    TS_ASSERT_EQUALS(0, par.size());

    AnalysisDataService::Instance().remove(wsName);
  }

  void loadTestInstrument()
  {
    using namespace Mantid::API;
    using namespace Mantid::Geometry;

    const size_t nhist(9);
    MatrixWorkspace_sptr ws = WorkspaceFactory::Instance().create("Workspace2D",nhist,1,1);

    // Det IDs 1-9
    auto inst = ComponentCreationHelper::createTestInstrumentCylindrical(1);
    ws->setInstrument(inst);

    for(size_t i = 0; i < nhist; ++i)
    {
      ISpectrum * spec = ws->getSpectrum(0);
      spec->setSpectrumNo(static_cast<Mantid::specid_t>(i+1));
      spec->clearDetectorIDs();
      spec->addDetectorID(static_cast<Mantid::detid_t>(i+1));
    }

    TS_ASSERT_THROWS_NOTHING(AnalysisDataService::Instance().add(wsName, ws));

  }

  void runUpdateInstrument(const std::string & filename, const std::string & header = "",
                           const bool ignorePhi = false, const bool moveMonitors = false)
  {
    UpdateInstrumentFromFile updater;
    updater.initialize();
    updater.setPropertyValue("Workspace", wsName);
    updater.setPropertyValue("Filename", filename);
    updater.setProperty("IgnorePhi", ignorePhi);
    updater.setProperty("MoveMonitors", moveMonitors);
    if(!header.empty())
    {
      updater.setPropertyValue("AsciiHeader", header);
    }
    updater.setRethrows(true);
    updater.execute();
  }


  void checkErrorMessageFromBadHeader(const std::string & expectedMsg,
        const std::string & filename, const std::string & header)
  {
    try
    {
      runUpdateInstrument(filename, header);
      TS_FAIL("Expected runUpdateInstrument to throw but it didn't.");
    }
    catch(std::runtime_error & exc)
    {
      TSM_ASSERT_EQUALS("runUpdateInstrument threw a std::runtime_error but the message is not the expected message",
                        expectedMsg, std::string(exc.what()));
    }
    catch(...)
    {
      TS_FAIL("Expected runUpdateInstrument to throw a std::runtime_error but it threw something else.");
    }

  }

private:
  std::string wsName;
  std::string xmlFile;
};

#endif /*UPDATEINSTRUMENTTESTFROMFILE_H_*/
