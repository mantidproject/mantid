#ifndef SAVEASCIITEST_H_
#define SAVEASCIITEST_H_

#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidDataHandling/SaveAscii.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidHistogramData/LinearGenerator.h"
#include <Poco/File.h>
#include <cxxtest/TestSuite.h>
#include <fstream>

using namespace Mantid::API;
using namespace Mantid::DataHandling;
using namespace Mantid::DataObjects;

// This test tests that SaveAscii produces a file of the expected form
// It does not test that the file can be loaded by loadAscii.
// The test LoadSaveAscii does that and should be run in addition to this test
// if you modify SaveAscii.

class SaveAsciiTest : public CxxTest::TestSuite {

public:
  static SaveAsciiTest *createSuite() { return new SaveAsciiTest(); }
  static void destroySuite(SaveAsciiTest *suite) { delete suite; }

  SaveAsciiTest() {}
  ~SaveAsciiTest() override {
    FrameworkManager::Instance().deleteWorkspace("SaveAsciiWS");
  }

  void testExec() {
    Mantid::DataObjects::Workspace2D_sptr wsToSave =
        boost::dynamic_pointer_cast<Mantid::DataObjects::Workspace2D>(
            WorkspaceFactory::Instance().create("Workspace2D", 2, 3, 3));
    for (int i = 0; i < 2; i++) {
      auto &X = wsToSave->mutableX(i);
      auto &Y = wsToSave->mutableY(i);
      auto &E = wsToSave->mutableE(i);
      for (int j = 0; j < 3; j++) {
        X[j] = 1.5 * j / 0.9;
        Y[j] = (i + 1) * (2. + 4. * X[j]);
        E[j] = 1.;
      }
    }
    const std::string name = "SaveAsciiWS";
    AnalysisDataService::Instance().add(name, wsToSave);

    std::string filename = "SaveAsciiTestFile.dat";
    std::string filename_nohead = "SaveAsciiTestFileWithoutHeader.dat";

    SaveAscii save;
    TS_ASSERT_THROWS_NOTHING(save.initialize());
    TS_ASSERT(save.isInitialized())
    TS_ASSERT_THROWS_NOTHING(save.setPropertyValue("Filename", filename));
    filename = save.getPropertyValue("Filename"); // Get absolute path
    TS_ASSERT_THROWS_NOTHING(save.setPropertyValue("InputWorkspace", name));
    TS_ASSERT_THROWS_NOTHING(save.execute());

    // has the algorithm written a file to disk?
    TS_ASSERT(Poco::File(filename).exists());

    // Now make some checks on the content of the file
    std::ifstream in(filename.c_str());
    std::string header1, header2, header3, header4;
    std::string separator;

    // Currently we just test that the first few column headers and a separator
    // are as expected
    in >> header1 >> separator >> header2 >> separator >> header3 >>
        separator >> header4;
    TS_ASSERT_EQUALS(separator, ",");
    TS_ASSERT_EQUALS(header1, "X");
    TS_ASSERT_EQUALS(header2, "Y0");
    TS_ASSERT_EQUALS(header3, "E0");
    TS_ASSERT_EQUALS(header4, "Y1");
    in.close();

    // Test ColumnHeader property - perhaps this should be a separate test
    save.setPropertyValue("Filename", filename_nohead);
    save.setPropertyValue("InputWorkspace", name);
    TS_ASSERT_THROWS_NOTHING(save.setProperty("ColumnHeader", false));
    filename_nohead = save.getPropertyValue("Filename"); // Get absolute path
    TS_ASSERT_THROWS_NOTHING(save.execute());

    // has the algorithm written a file to disk?
    TS_ASSERT(Poco::File(filename_nohead).exists());

    // Now we check that the first line of the file without header matches the
    // second line of the file with header
    std::ifstream in1(filename.c_str());
    std::string line2header;
    std::ifstream in2(filename_nohead.c_str());
    std::string line1noheader;
    getline(in1, line2header);
    getline(in1, line2header);   // 2nd line of file with header
    getline(in2, line1noheader); // 1st line of file without header
    TS_ASSERT_EQUALS(line1noheader, line2header);
    in1.close();
    in2.close();

    // Remove files
    Poco::File(filename).remove();
    Poco::File(filename_nohead).remove();
  }

  void testExec_DX() {
    Mantid::DataObjects::Workspace2D_sptr wsToSave =
        boost::dynamic_pointer_cast<Mantid::DataObjects::Workspace2D>(
            WorkspaceFactory::Instance().create("Workspace2D", 2, 3, 3));
    for (int i = 0; i < 2; i++) {
      auto &X = wsToSave->mutableX(i);
      auto &Y = wsToSave->mutableY(i);
      auto &E = wsToSave->mutableE(i);
      wsToSave->setPointStandardDeviations(i, 3);
      auto &DX = wsToSave->mutableDx(i);
      for (int j = 0; j < 3; j++) {
        X[j] = 1.5 * j / 0.9;
        Y[j] = (i + 1) * (2. + 4. * X[j]);
        E[j] = 1.;
        DX[j] = i + 1;
      }
    }

    const std::string WSname = "SaveAsciiDX_WS";
    AnalysisDataService::Instance().add(WSname, wsToSave);

    SaveAscii save;
    std::string filename = "SaveAsciiDXTestFile.dat";
    TS_ASSERT_THROWS_NOTHING(save.initialize());
    TS_ASSERT(save.isInitialized())
    TS_ASSERT_THROWS_NOTHING(save.setPropertyValue("Filename", filename));
    filename = save.getPropertyValue("Filename"); // Get absolute path
    TS_ASSERT_THROWS_NOTHING(save.setPropertyValue("InputWorkspace", WSname));
    TS_ASSERT_THROWS_NOTHING(save.setPropertyValue("WriteXError", "1"));
    TS_ASSERT_THROWS_NOTHING(save.execute());

    // has the algorithm written a file to disk?
    TS_ASSERT(Poco::File(filename).exists());

    // Now make some checks on the content of the file
    std::ifstream in(filename.c_str());
    std::string header1, header2, header3, header4, separator;

    // Test that the first few column headers, separator and first two bins are
    // as expected
    in >> header1 >> separator >> header2 >> separator >> header3 >>
        separator >> header4;
    TS_ASSERT_EQUALS(separator, ",");
    TS_ASSERT_EQUALS(header1, "X");
    TS_ASSERT_EQUALS(header2, "Y0");
    TS_ASSERT_EQUALS(header3, "E0");
    TS_ASSERT_EQUALS(header4, "DX0");
    in.close();

    Poco::File(filename).remove();
    AnalysisDataService::Instance().remove(WSname);
  }
};

#endif /*SAVEASCIITEST_H_*/
