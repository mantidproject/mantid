#ifndef SAVEGSSTEST_H_
#define SAVEGSSTEST_H_

#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/Axis.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidDataHandling/SaveGSS.h"
#include "MantidGeometry/Instrument.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"
#include "cxxtest/TestSuite.h"

#include <Poco/File.h>
#include <Poco/Glob.h>
#include <fstream>

using namespace Mantid;
using namespace Mantid::API;
using namespace Mantid::DataObjects;
using namespace Mantid::Geometry;
using namespace Mantid::Kernel;
using Mantid::DataHandling::SaveGSS;

namespace {
void populateWorkspaceWithLogData(MatrixWorkspace *wsPointer) {
  wsPointer->getAxis(0)->setUnit("TOF");
  // Set data with logarithm bin
  double t0 = 5000.;
  double dt = 0.01;
  const size_t numhist = wsPointer->getNumberHistograms();
  for (size_t iws = 0; iws < numhist; ++iws) {
    auto &mutableXVals = wsPointer->mutableX(iws);
    mutableXVals[0] = t0;
    for (size_t i = 1; i < mutableXVals.size(); ++i)
      mutableXVals[i] = (1 + dt) * mutableXVals[i - 1];
  }

  // Set y and e
  for (size_t iws = 0; iws < numhist; ++iws) {
    const auto &xVals = wsPointer->x(iws);
    auto &mutableYVals = wsPointer->mutableY(iws);
    auto &mutableEVals = wsPointer->mutableE(iws);
    const double factor = (static_cast<double>(iws) + 1) * 1000.;
    for (size_t i = 0; i < mutableYVals.size(); ++i) {
      mutableYVals[i] = factor * std::exp(-(xVals[i] - 7000. - factor) *
                                          (xVals[i] - 7000. - factor) /
                                          (0.01 * factor * factor));
      if (mutableYVals[i] < 0.01)
        mutableEVals[i] = 0.1;
      else
        mutableEVals[i] = std::sqrt(mutableYVals[i]);
    }
  }
}

/**
  * Generates a fully defined instrument with specified number
  * of histograms and bins for the test
  *
  * @param numHistograms :: The number of histograms the output workspace should
  *have
  * @param numBins :: The number of bins the workspace should have
  * @return :: A workspace with logarithmic binning and test data
  */
API::MatrixWorkspace_sptr generateTestMatrixWorkspace(int numHistograms,
                                                      int numBins) {
  // Create workspace
  MatrixWorkspace_sptr dataws = boost::dynamic_pointer_cast<MatrixWorkspace>(
      WorkspaceCreationHelper::create2DWorkspaceWithFullInstrument(
          numHistograms, numBins, false, false, true, "TestFake"));
  populateWorkspaceWithLogData(dataws.get());

  return dataws;
}
}

class SaveGSSTest : public CxxTest::TestSuite {
public:
  void test_TheBasics() {
    Mantid::DataHandling::SaveGSS saver;
    TS_ASSERT_THROWS_NOTHING(saver.initialize())
    TS_ASSERT_EQUALS(saver.name(), "SaveGSS")
    TS_ASSERT_EQUALS(saver.version(), 1)
  }

  //----------------------------------------------------------------------------------------------
  /** Save a 2 banks diffraction data with instrument
    */
  void test_2BankInstrument() {
    // Create a workspace for writing out
    MatrixWorkspace_sptr dataws =
        generateTestMatrixWorkspace(m_defaultNumHistograms, m_defaultNumBins);
    AnalysisDataService::Instance().addOrReplace("Test2BankWS", dataws);

    Mantid::DataHandling::SaveGSS saver;
    saver.initialize();

    // Set properties
    TS_ASSERT_THROWS_NOTHING(
        saver.setPropertyValue("InputWorkspace", "Test2BankWS"));
    TS_ASSERT_THROWS_NOTHING(saver.setProperty("Filename", "test1.gsa"));
    TS_ASSERT_THROWS_NOTHING(saver.setProperty("Format", "SLOG"));
    TS_ASSERT_THROWS_NOTHING(saver.setProperty("SplitFiles", false));
    TS_ASSERT_THROWS_NOTHING(saver.setProperty("MultiplyByBinWidth", false));
    TS_ASSERT_THROWS_NOTHING(saver.setProperty("Append", false));

    // Execute
    saver.execute();
    TS_ASSERT(saver.isExecuted());

    // Check result
    // locate output file
    std::string outfilepath = saver.getPropertyValue("Filename");
    std::cout << "Output file is located at " << outfilepath << "\n";

    Poco::File gsasfile(outfilepath);
    TS_ASSERT(gsasfile.exists());

    // check file
    if (gsasfile.exists()) {
      size_t numlines = 0;
      std::ifstream fs_gsas(outfilepath.c_str());
      std::string line;
      while (std::getline(fs_gsas, line)) {
        size_t linenumber = numlines;
        std::stringstream liness(line);
        if (linenumber == 11) {
          std::string bank;
          int banknumber;
          liness >> bank >> banknumber;
          TS_ASSERT_EQUALS(bank, "BANK");
          TS_ASSERT_EQUALS(banknumber, 1);
        } else if (linenumber == 60) {
          double x, y, e;
          liness >> x >> y >> e;
          TS_ASSERT_DELTA(x, 8101.43, 0.01);
          TS_ASSERT_DELTA(y, 688.18, 0.01);
          TS_ASSERT_DELTA(e, 26.23, 0.01);
        } else if (linenumber == 114) {
          std::string bank;
          int banknumber;
          liness >> bank >> banknumber;
          TS_ASSERT_EQUALS(bank, "BANK");
          TS_ASSERT_EQUALS(banknumber, 2);
        } else if (linenumber == 173) {
          double x, y, e;
          liness >> x >> y >> e;
          TS_ASSERT_DELTA(x, 8949.02, 0.01);
          TS_ASSERT_DELTA(y, 1592.26, 0.01);
          TS_ASSERT_DELTA(e, 39.90, 0.01);
        }

        ++numlines;
      }

      TS_ASSERT_EQUALS(numlines, 215);
    }

    // Clean
    AnalysisDataService::Instance().remove("Test2BankWS");
    if (gsasfile.exists())
      gsasfile.remove();

    return;
  }

  //----------------------------------------------------------------------------------------------
  /** Save a 2 banks diffraction data with instrument
    */
  void test_2BankInstrumentRALF() {
    // Create a workspace for writing out
    MatrixWorkspace_sptr dataws =
        generateTestMatrixWorkspace(m_defaultNumHistograms, m_defaultNumBins);
    AnalysisDataService::Instance().addOrReplace("Test2BankWS", dataws);

    Mantid::DataHandling::SaveGSS saver;
    saver.initialize();

    // Set properties
    TS_ASSERT_THROWS_NOTHING(
        saver.setPropertyValue("InputWorkspace", "Test2BankWS"));
    TS_ASSERT_THROWS_NOTHING(saver.setProperty("Filename", "test1r.gsa"));
    TS_ASSERT_THROWS_NOTHING(saver.setProperty("Format", "RALF"));
    TS_ASSERT_THROWS_NOTHING(saver.setProperty("SplitFiles", false));
    TS_ASSERT_THROWS_NOTHING(saver.setProperty("MultiplyByBinWidth", false));
    TS_ASSERT_THROWS_NOTHING(saver.setProperty("Append", false));

    // Execute
    saver.execute();
    TS_ASSERT(saver.isExecuted());

    // Check result
    // locate output file
    std::string outfilepath = saver.getPropertyValue("Filename");
    std::cout << "Output file is located at " << outfilepath << "\n";

    Poco::File gsasfile(outfilepath);
    TS_ASSERT(gsasfile.exists());

    // check file
    if (gsasfile.exists()) {
      size_t numlines = 0;
      std::ifstream fs_gsas(outfilepath.c_str());
      std::string line;
      while (std::getline(fs_gsas, line)) {
        size_t linenumber = numlines;
        std::stringstream liness(line);
        if (linenumber == 8) {
          std::string bank;
          int banknumber;
          liness >> bank >> banknumber;
          TS_ASSERT_EQUALS(bank, "BANK");
          TS_ASSERT_EQUALS(banknumber, 1);
        } else if (linenumber == 57) {
          double x, y, e;
          liness >> x >> y >> e;
          TS_ASSERT_DELTA(x, 8101.43, 0.01);
          TS_ASSERT_DELTA(y, 688.18, 0.01);
          TS_ASSERT_DELTA(e, 26.23, 0.01);
        } else if (linenumber == 111) {
          std::string bank;
          int banknumber;
          liness >> bank >> banknumber;
          TS_ASSERT_EQUALS(bank, "BANK");
          TS_ASSERT_EQUALS(banknumber, 2);
        } else if (linenumber == 170) {
          double x, y, e;
          liness >> x >> y >> e;
          TS_ASSERT_DELTA(x, 8949.02, 0.01);
          TS_ASSERT_DELTA(y, 1592.26, 0.01);
          TS_ASSERT_DELTA(e, 39.90, 0.01);
        }

        ++numlines;
      }

      TS_ASSERT_EQUALS(numlines, 212);
    }

    // Clean
    AnalysisDataService::Instance().remove("Test2BankWS");
    if (gsasfile.exists())
      gsasfile.remove();

    return;
  }

  //----------------------------------------------------------------------------------------------
  /** Save a 2 bank workspace in point data format and without instrument
    */
  void test_2BankNoInstrumentData() {
    MatrixWorkspace_sptr dataws =
        generateNoInstrumentWorkspace(m_defaultNumHistograms, m_defaultNumBins);

    AnalysisDataService::Instance().addOrReplace("TestNoInstWS", dataws);

    Mantid::DataHandling::SaveGSS saver;
    saver.initialize();

    // Set properties
    TS_ASSERT_THROWS_NOTHING(
        saver.setPropertyValue("InputWorkspace", "TestNoInstWS"));
    TS_ASSERT_THROWS_NOTHING(saver.setProperty("Filename", "test2.gsa"));
    TS_ASSERT_THROWS_NOTHING(saver.setProperty("Format", "SLOG"));
    TS_ASSERT_THROWS_NOTHING(saver.setProperty("SplitFiles", false));
    TS_ASSERT_THROWS_NOTHING(saver.setProperty("MultiplyByBinWidth", false));
    TS_ASSERT_THROWS_NOTHING(saver.setProperty("Append", false));

    // Execute
    saver.execute();
    TS_ASSERT(saver.isExecuted());

    // Check result
    // locate output file
    std::string outfilepath = saver.getPropertyValue("Filename");
    std::cout << "Output file is located at " << outfilepath << "\n";

    Poco::File gsasfile(outfilepath);
    TS_ASSERT(gsasfile.exists());

    // check file
    if (gsasfile.exists()) {
      size_t numlines = 0;
      std::ifstream fs_gsas(outfilepath.c_str());
      std::string line;
      while (std::getline(fs_gsas, line)) {
        size_t linenumber = numlines;
        std::stringstream liness(line);
        if (linenumber == 10) {
          std::string bank;
          int banknumber;
          liness >> bank >> banknumber;
          TS_ASSERT_EQUALS(bank, "BANK");
          TS_ASSERT_EQUALS(banknumber, 1);
        } else if (linenumber == 59) {
          double x, y, e;
          liness >> x >> y >> e;
          TS_ASSERT_DELTA(x, 8101.43, 0.01);
          TS_ASSERT_DELTA(y, 688.18, 0.01);
          TS_ASSERT_DELTA(e, 26.23, 0.01);
        } else if (linenumber == 112) {
          std::string bank;
          int banknumber;
          liness >> bank >> banknumber;
          TS_ASSERT_EQUALS(bank, "BANK");
          TS_ASSERT_EQUALS(banknumber, 2);
        } else if (linenumber == 171) {
          double x, y, e;
          liness >> x >> y >> e;
          TS_ASSERT_DELTA(x, 8949.02, 0.01);
          TS_ASSERT_DELTA(y, 1592.26, 0.01);
          TS_ASSERT_DELTA(e, 39.90, 0.01);
        }

        ++numlines;
      }

      TS_ASSERT_EQUALS(numlines, 213);
    }

    // Clean
    AnalysisDataService::Instance().remove("TestNoInstWS");
    if (gsasfile.exists())
      gsasfile.remove();

    return;
  }

private:
  API::MatrixWorkspace_sptr generateNoInstrumentWorkspace(int numHistograms,
                                                          int numBins) {
    MatrixWorkspace_sptr dataws =
        WorkspaceCreationHelper::create2DWorkspace(numHistograms, numBins);
    populateWorkspaceWithLogData(dataws.get());
    return dataws;
  }

  const int m_defaultNumHistograms = 2;
  const int m_defaultNumBins = 100;
};

class SaveGSSTestPerformance : public CxxTest::TestSuite {
public:
  void setUp() override {
    // Create a workspace for writing out
    MatrixWorkspace_sptr dataws =
        generateTestMatrixWorkspace(2, m_numberOfBinsToSave);
    AnalysisDataService::Instance().addOrReplace(wsName, dataws);

    m_alg = new SaveGSS();
    m_alg->initialize();
    m_alg->setProperty("Append", false);
    m_alg->setPropertyValue("InputWorkspace", wsName);
    m_alg->setProperty("Filename", filename);
    m_alg->setRethrows(true);
  }

  void testSaveGSSPerformance() { TS_ASSERT_THROWS_NOTHING(m_alg->execute()); }

  void tearDown() override {
    delete m_alg;
    m_alg = nullptr;
    Mantid::API::AnalysisDataService::Instance().remove(wsName);

    // Use glob to find any files that match the output pattern
    std::set<std::string> returnedFiles;
    Poco::Glob::glob(globPattern, returnedFiles);
    for (const auto &filename : returnedFiles) {
      Poco::File pocoFile{filename};
      pocoFile.remove();
    }
  }

private:
  // Controls the speed of the test
  const int m_numberOfBinsToSave = 100000;

  const std::string wsName = "Test2BankWS";
  const std::string filename = "test_performance.gsa";
  const std::string globPattern = "test_performance*.gsa";

  SaveGSS *m_alg = nullptr;
};

#endif // SAVEGSSTEST_H_