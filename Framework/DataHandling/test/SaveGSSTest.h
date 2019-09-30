// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef SAVEGSSTEST_H_
#define SAVEGSSTEST_H_

#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/Axis.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidDataHandling/SaveGSS.h"
#include "MantidGeometry/Instrument.h"

#include "MantidTestHelpers/FileComparisonHelper.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"

#include "cxxtest/TestSuite.h"

#include <Poco/File.h>
#include <Poco/Glob.h>
#include <Poco/TemporaryFile.h>
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

// Generates a test matrix workspace populated with data and registers it into
// the ADS
API::MatrixWorkspace_sptr generateTestMatrixWorkspace(const std::string &wsName,
                                                      int numHistograms,
                                                      int numBins) {
  // Create workspace
  MatrixWorkspace_sptr dataws = boost::dynamic_pointer_cast<MatrixWorkspace>(
      WorkspaceCreationHelper::create2DWorkspaceWithFullInstrument(
          numHistograms, numBins, false, false, true, "TestFake"));
  populateWorkspaceWithLogData(dataws.get());
  AnalysisDataService::Instance().addOrReplace(wsName, dataws);
  return dataws;
}

// Generates a matrix WS with no instrument but with data and registers it into
// ADS
API::MatrixWorkspace_sptr generateNoInstWorkspace(const std::string &wsName,
                                                  int numHistograms,
                                                  int numBins) {
  MatrixWorkspace_sptr dataws =
      WorkspaceCreationHelper::create2DWorkspace(numHistograms, numBins);
  populateWorkspaceWithLogData(dataws.get());
  AnalysisDataService::Instance().addOrReplace(wsName, dataws);
  return dataws;
}

} // End of anonymous namespace

class SaveGSSTest : public CxxTest::TestSuite {
public:
  void test_TheBasics() {
    Mantid::DataHandling::SaveGSS saver;
    TS_ASSERT_THROWS_NOTHING(saver.initialize())
    TS_ASSERT_EQUALS(saver.name(), "SaveGSS")
    TS_ASSERT_EQUALS(saver.version(), 1)
  }

  void test_2BankInstrumentSLOG() {
    // Save a 2 banks diffraction data with instrument using SLOG format
    const std::string wsName = "SaveGSS_2BankSLOG";
    auto dataws = generateTestMatrixWorkspace(wsName, m_defaultNumHistograms,
                                              m_defaultNumBins);
    // Get the output file handle
    auto outputFileHandle = Poco::TemporaryFile();
    const std::string outPath = outputFileHandle.path();

    // Execute
    auto alg = setupSaveGSSAlg(outPath, wsName, "SLOG");
    alg->execute();
    TS_ASSERT(alg->isExecuted());

    // Check file is identical
    TS_ASSERT(FileComparisonHelper::isEqualToReferenceFile(
        "SaveGSS_test2BankInstSLOG_Ref.gsa", outPath));

    // Clean
    AnalysisDataService::Instance().remove(wsName);
  }

  //----------------------------------------------------------------------------------------------
  /** test writing a 2-bank GSAS file with user input header and user specified
   * precision on XYE
   * @brief test_2BankInstrumentSLOGUserHeader
   */
  void test_2BankInstrumentSLOGUserHeader() {
    // Save a 2 banks diffraction data with instrument using SLOG format
    const std::string wsName = "SaveGSS_2BankSLOG";
    generateTestMatrixWorkspace(wsName, m_defaultNumHistograms,
                                m_defaultNumBins);
    // Get the output file handle
    auto outputFileHandle = Poco::TemporaryFile();
    const std::string outPath = outputFileHandle.path();

    // define user specified arbitrary header
    std::vector<std::string> user_header;
    user_header.push_back("user line 1");
    user_header.push_back("user line 2");
    user_header.push_back("user line 3");

    // define user specified arbitrary bank header
    std::vector<std::string> user_bank_headers;
    user_bank_headers.push_back("Bank 1 some information");
    user_bank_headers.push_back("Bank 2 some information different");

    // SLOG XYE precision
    std::vector<int> slog_xye_precision(3, 1);

    // Execute
    auto alg = setupSaveGSSAlg(outPath, wsName, "SLOG");
    TS_ASSERT_THROWS_NOTHING(
        alg->setProperty("UserSpecifiedGSASHeader", user_header));
    TS_ASSERT_THROWS_NOTHING(alg->setProperty("OverwriteStandardHeader", true));
    TS_ASSERT_THROWS_NOTHING(
        alg->setProperty("UserSpecifiedBankHeader", user_bank_headers));
    TS_ASSERT_THROWS_NOTHING(
        alg->setProperty("SLOGXYEPrecision", slog_xye_precision))
    alg->execute();
    TS_ASSERT(alg->isExecuted());

    // Check file is identical
    TS_ASSERT(!FileComparisonHelper::isEqualToReferenceFile(
        "SaveGSS_test2BankInstSLOG_Ref.gsa", outPath));

    // Clean
    AnalysisDataService::Instance().remove(wsName);
  }

  /**
   * @brief test_2BankInstrumentRALF
   */
  void test_unwritablePath() {
    if (Poco::File("/usr").exists()) {
      // Save a 2 banks diffraction data with instrument using SLOG format
      const std::string wsName = "unwritablePath";
      auto dataws = generateTestMatrixWorkspace(wsName, m_defaultNumHistograms,
                                                m_defaultNumBins);
      // Get the output file handle
      const std::string outPath =
          "/usr/unwritablePath.gss"; // outputFileHandle.path();

      // Execute
      auto alg = setupSaveGSSAlg(outPath, wsName, "SLOG");
      TS_ASSERT_THROWS_ANYTHING(alg->execute());

      // Clean
      AnalysisDataService::Instance().remove(wsName);
    } else {
      std::cout << "skipping test_unwritablePath because /usr does not exist"
                << std::endl;
    }
  }

  void test_2BankInstrumentRALF() {
    // Save a 2 banks diffraction data with RALF format
    const std::string wsName = "SaveGSS_2BankRALF";
    auto dataws = generateTestMatrixWorkspace(wsName, m_defaultNumHistograms,
                                              m_defaultNumBins);
    // Get file handle
    auto outFileHandle = Poco::TemporaryFile();
    const std::string outFilePath = outFileHandle.path();

    // Execute
    auto alg = setupSaveGSSAlg(outFilePath, wsName, "RALF");
    alg->execute();
    TS_ASSERT(alg->isExecuted());

    // Check result
    TS_ASSERT(FileComparisonHelper::isEqualToReferenceFile(
        "SaveGSS_test2BankInstRALF_Ref.gsa", outFilePath));

    // Clean
    AnalysisDataService::Instance().remove(wsName);
  }

  void test_2BankNoInstrumentData() {
    // Test saving a 2 bank workspace in point data format and without
    // instrument
    const std::string wsName = "SaveGSS_NoInstWs";
    auto dataws = generateNoInstWorkspace(wsName, m_defaultNumHistograms,
                                          m_defaultNumBins);
    // Get file handle
    auto outFileHandle = Poco::TemporaryFile();
    const std::string outFilePath = outFileHandle.path();

    // Execute
    auto alg = setupSaveGSSAlg(outFilePath, wsName, "SLOG");
    alg->execute();
    TS_ASSERT(alg->isExecuted());

    TS_ASSERT(FileComparisonHelper::isEqualToReferenceFile(
        "SaveGSS_test2BankNoInst_Ref.gsa", outFilePath));

    // Clean
    AnalysisDataService::Instance().remove(wsName);
  }

  void test_2BankRalfAltFormat() {
    // Tests saving a 2 bank instrument in RALF - Alternative format
    const std::string wsName = "SaveGSS_RALF_ALT_ws";

    // Choose a number where mod 4 != 0 to check it handles having
    // not enough data for one line correctly
    const int numBins = 10;
    auto dataWs =
        generateTestMatrixWorkspace(wsName, m_defaultNumHistograms, numBins);

    auto outFileHandle = Poco::TemporaryFile();
    const std::string outFilePath = outFileHandle.path();

    auto alg = setupSaveGSSAlg(outFilePath, wsName, "RALF");
    alg->setProperty("DataFormat", "ALT");
    alg->setProperty("SplitFiles", false);
    alg->execute();
    TS_ASSERT(alg->isExecuted());
    TS_ASSERT(FileComparisonHelper::isEqualToReferenceFile(
        "SaveGSS_test2BankRalfAltFormat_ref.gsa", outFilePath));

    AnalysisDataService::Instance().remove(wsName);
  }

  void test_splitFiles() {
    // Tests saving a 2 bank instrument with split files selected
    const std::string wsName = "SaveGSS_2BankSplit_ws";

    auto dataWs = generateTestMatrixWorkspace(wsName, 2, m_defaultNumBins);

    // Have to do some custom handling of temp files to use extensions
    auto outFileHandle = Poco::TemporaryFile();
    const std::string outFilePath = outFileHandle.path();
    const std::string outFilePathWithExt = outFilePath + ".gsas";

    outFileHandle.registerForDeletion(outFilePathWithExt);

    auto alg = setupSaveGSSAlg(outFilePathWithExt, wsName, "RALF");
    alg->setProperty("DataFormat", "FXYE");
    alg->setProperty("SplitFiles", true);
    alg->execute();
    TS_ASSERT(alg->isExecuted());

    // The alg will automatically append 0 and 1 when we split the files
    const std::string fileOnePath = outFilePath + "-0.gsas";
    const std::string fileTwoPath = outFilePath + "-1.gsas";

    TS_ASSERT(FileComparisonHelper::isEqualToReferenceFile(
        "SaveGSS-SplitRef-0.gsas", fileOnePath));
    TS_ASSERT(FileComparisonHelper::isEqualToReferenceFile(
        "SaveGSS-SplitRef-1.gsas", fileTwoPath));

    // Use glob to find any files that match the output pattern
    std::set<std::string> returnedFiles;
    Poco::Glob::glob(outFilePath, returnedFiles);
    for (const auto &filename : returnedFiles) {
      Poco::File pocoFile{filename};
      pocoFile.remove();
    }

    AnalysisDataService::Instance().remove(wsName);
  }

private:
  const int m_defaultNumHistograms = 2;
  const int m_defaultNumBins = 100;

  std::unique_ptr<SaveGSS> setupSaveGSSAlg(const std::string &filePath,
                                           const std::string &wsName,
                                           const std::string &formatMode) {
    auto saver = std::make_unique<SaveGSS>();
    saver->initialize();
    saver->setRethrows(true);
    // Set properties
    TS_ASSERT_THROWS_NOTHING(saver->setPropertyValue("InputWorkspace", wsName));
    TS_ASSERT_THROWS_NOTHING(saver->setProperty("Filename", filePath));
    TS_ASSERT_THROWS_NOTHING(saver->setProperty("Format", formatMode));
    TS_ASSERT_THROWS_NOTHING(saver->setProperty("SplitFiles", false));
    TS_ASSERT_THROWS_NOTHING(saver->setProperty("MultiplyByBinWidth", false));
    TS_ASSERT_THROWS_NOTHING(saver->setProperty("Append", false));
    return saver;
  }
};

class SaveGSSTestPerformance : public CxxTest::TestSuite {
public:
  void setUp() override {
    // Create a workspace for writing out
    MatrixWorkspace_sptr dataws =
        generateTestMatrixWorkspace(wsName, 2, m_numberOfBinsToSave);
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
