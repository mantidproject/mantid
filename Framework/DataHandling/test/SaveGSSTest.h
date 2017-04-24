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
    // Create a workspace for writing out
    const std::string wsName = "SaveGSS_2BankSLOG";
    MatrixWorkspace_sptr dataws =
        generateTestMatrixWorkspace(m_defaultNumHistograms, m_defaultNumBins);
    AnalysisDataService::Instance().addOrReplace(wsName, dataws);

    Mantid::DataHandling::SaveGSS saver;
    saver.initialize();

    // Get the output file handle
    auto outputFileHandle = Poco::TemporaryFile();
    const std::string outPath = outputFileHandle.path();

    // Set properties
    TS_ASSERT_THROWS_NOTHING(saver.setPropertyValue("InputWorkspace", wsName));
    TS_ASSERT_THROWS_NOTHING(saver.setProperty("Filename", outPath));
    TS_ASSERT_THROWS_NOTHING(saver.setProperty("Format", "SLOG"));
    TS_ASSERT_THROWS_NOTHING(saver.setProperty("SplitFiles", false));
    TS_ASSERT_THROWS_NOTHING(saver.setProperty("MultiplyByBinWidth", false));
    TS_ASSERT_THROWS_NOTHING(saver.setProperty("Append", false));
    saver.setRethrows(true);

    // Execute
    saver.execute();
    TS_ASSERT(saver.isExecuted());

    // Check file is identical
    TS_ASSERT(FileComparisonHelper::isEqualToReferenceFile(
        "SaveGSS_test2BankInstSLOG_Ref.gsa", outPath));

    // Clean
    AnalysisDataService::Instance().remove(wsName);
  }

  void test_2BankInstrumentRALF() {
    // Save a 2 banks diffraction data with RALF format

    // Create a workspace for writing out
    const std::string wsName = "SaveGSS_2BankRALF";
    MatrixWorkspace_sptr dataws =
        generateTestMatrixWorkspace(m_defaultNumHistograms, m_defaultNumBins);
    AnalysisDataService::Instance().addOrReplace(wsName, dataws);

    // Get file handle
    auto outFileHandle = Poco::TemporaryFile();
    const std::string outFilePath = outFileHandle.path();

    Mantid::DataHandling::SaveGSS saver;
    saver.initialize();

    // Set properties
    TS_ASSERT_THROWS_NOTHING(saver.setPropertyValue("InputWorkspace", wsName));
    TS_ASSERT_THROWS_NOTHING(saver.setProperty("Filename", outFilePath));
    TS_ASSERT_THROWS_NOTHING(saver.setProperty("Format", "RALF"));
    TS_ASSERT_THROWS_NOTHING(saver.setProperty("SplitFiles", false));
    TS_ASSERT_THROWS_NOTHING(saver.setProperty("MultiplyByBinWidth", false));
    TS_ASSERT_THROWS_NOTHING(saver.setProperty("Append", false));

    // Execute
    saver.execute();
    TS_ASSERT(saver.isExecuted());

    // Check result
    TS_ASSERT(FileComparisonHelper::isEqualToReferenceFile(
        "SaveGSS_test2BankInstRALF_ref.gsa", outFilePath));

    // Clean
    AnalysisDataService::Instance().remove(wsName);
  }

  void test_2BankNoInstrumentData() {
  // Save a 2 bank workspace in point data format and without instrument
	  const std::string wsName = "SaveGSS_NoInstWs";
    MatrixWorkspace_sptr dataws =
        generateNoInstrumentWorkspace(m_defaultNumHistograms, m_defaultNumBins);

    AnalysisDataService::Instance().addOrReplace(wsName, dataws);

    Mantid::DataHandling::SaveGSS saver;
    saver.initialize();

	// Get file handle
	auto outFileHandle = Poco::TemporaryFile();
	const std::string outFilePath = outFileHandle.path();

    // Set properties
    TS_ASSERT_THROWS_NOTHING(
        saver.setPropertyValue("InputWorkspace", wsName));
    TS_ASSERT_THROWS_NOTHING(saver.setProperty("Filename", outFilePath));
    TS_ASSERT_THROWS_NOTHING(saver.setProperty("Format", "SLOG"));
    TS_ASSERT_THROWS_NOTHING(saver.setProperty("SplitFiles", false));
    TS_ASSERT_THROWS_NOTHING(saver.setProperty("MultiplyByBinWidth", false));
    TS_ASSERT_THROWS_NOTHING(saver.setProperty("Append", false));

    // Execute
    saver.execute();
    TS_ASSERT(saver.isExecuted());

	TS_ASSERT(FileComparisonHelper::isEqualToReferenceFile("SaveGSS_test2BankNoInst_Ref.gsa", outFilePath));

    // Clean
    AnalysisDataService::Instance().remove(wsName);
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
    m_alg->setPropertyValue("InputWorkspace", wsName);
    m_alg->setProperty("Filename", filename);
    m_alg->setRethrows(true);
  }

  void testSaveGSSPerformance() { TS_ASSERT_THROWS_NOTHING(m_alg->execute()); }

  void tearDown() override {
    delete m_alg;
    m_alg = nullptr;
    Mantid::API::AnalysisDataService::Instance().remove(wsName);
    Poco::File gsasfile(filename);
    if (gsasfile.exists())
      gsasfile.remove();
  }

private:
  // Controls the speed of the test
  const int m_numberOfBinsToSave = 100000;

  const std::string wsName = "Test2BankWS";
  const std::string filename = "test_performance.gsa";

  SaveGSS *m_alg = nullptr;
};

#endif // SAVEGSSTEST_H_