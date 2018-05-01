#ifndef MANTIDQT_CUSTOMINTERFACES_ENGGVANADIUMCORRECTIONSMODELTEST_H_
#define MANTIDQT_CUSTOMINTERFACES_ENGGVANADIUMCORRECTIONSMODELTEST_H_

#include "../EnggDiffraction/EnggVanadiumCorrectionsModel.h"

#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/TableRow.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidTestHelpers/HistogramDataTestHelper.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"

#include <boost/filesystem.hpp>

#include <cxxtest/TestSuite.h>

using namespace MantidQt::CustomInterfaces;

namespace {

Mantid::API::MatrixWorkspace_sptr createSampleMatrixWorkspace() {
  return WorkspaceCreationHelper::create2DWorkspaceBinned(1, 1, 2);
}

Mantid::API::ITableWorkspace_sptr createSampleTableWorkspace() {
  auto table = Mantid::API::WorkspaceFactory::Instance().createTable();
  table->addColumn("double", "x");
  Mantid::API::TableRow newRow = table->appendRow();
  newRow << 1.0;
  return table;
}

/// Helper class to allow us to fake EnggVanadiumCorrections
class TestEnggVanadiumCorrectionsModel : public EnggVanadiumCorrectionsModel {
public:
  TestEnggVanadiumCorrectionsModel(const EnggDiffCalibSettings &calibSettings,
                                   const std::string &currentInstrument);

  mutable bool m_calculateCorrectionsCalled;

private:
  std::pair<Mantid::API::ITableWorkspace_sptr,
            Mantid::API::MatrixWorkspace_sptr>
  calculateCorrectionWorkspaces(const std::string &runNumber) const override;
};

inline TestEnggVanadiumCorrectionsModel::TestEnggVanadiumCorrectionsModel(
    const EnggDiffCalibSettings &calibSettings,
    const std::string &currentInstrument)
    : EnggVanadiumCorrectionsModel(calibSettings, currentInstrument),
      m_calculateCorrectionsCalled(false) {}

inline std::pair<Mantid::API::ITableWorkspace_sptr,
                 Mantid::API::MatrixWorkspace_sptr>
TestEnggVanadiumCorrectionsModel::calculateCorrectionWorkspaces(
    const std::string &runNumber) const {
  UNUSED_ARG(runNumber);
  m_calculateCorrectionsCalled = true;

  auto &ADS = Mantid::API::AnalysisDataService::Instance();

  Mantid::API::MatrixWorkspace_sptr curvesWS = createSampleMatrixWorkspace();
  ADS.addOrReplace(CURVES_WORKSPACE_NAME, curvesWS);

  auto integratedWS = createSampleTableWorkspace();
  ADS.addOrReplace(INTEGRATED_WORKSPACE_NAME, integratedWS);

  return std::make_pair(integratedWS, curvesWS);
}

} // anonymous namespace

class EnggVanadiumCorrectionsModelTest : public CxxTest::TestSuite {

public:
  static EnggVanadiumCorrectionsModelTest *createSuite() {
    return new EnggVanadiumCorrectionsModelTest();
  }

  static void destroySuite(EnggVanadiumCorrectionsModelTest *suite) {
    delete suite;
  }

  EnggVanadiumCorrectionsModelTest() {
    Mantid::API::FrameworkManager::Instance();
  }

  void setUp() override {
    const auto tempDir = boost::filesystem::temp_directory_path();
    m_inputDir = tempDir / INPUT_DIR_NAME;
    boost::filesystem::create_directory(m_inputDir);
  }

  void tearDown() override { boost::filesystem::remove_all(m_inputDir); }

  void test_generateNewWorkspacesWhenNoCache() {
    // We've created the calib directory but not populated it with any
    // workspaces, so we should get our fake ones
    EnggDiffCalibSettings calibSettings;
    calibSettings.m_inputDirCalib = m_inputDir.string();
    calibSettings.m_forceRecalcOverwrite = false;

    TestEnggVanadiumCorrectionsModel model(calibSettings, CURRENT_INSTRUMENT);
    std::pair<Mantid::API::ITableWorkspace_sptr,
              Mantid::API::MatrixWorkspace_sptr>
        correctionWorkspaces;
    TS_ASSERT_THROWS_NOTHING(correctionWorkspaces =
                                 model.fetchCorrectionWorkspaces("123"));
    TS_ASSERT(model.m_calculateCorrectionsCalled);
    TS_ASSERT(correctionWorkspaces.first);
    TS_ASSERT(correctionWorkspaces.second);
    TS_ASSERT(boost::filesystem::exists(
        m_inputDir / (CURRENT_INSTRUMENT +
                      "00000123_precalculated_vanadium_run_bank_curves.nxs")));
    TS_ASSERT(boost::filesystem::exists(
        m_inputDir / (CURRENT_INSTRUMENT +
                      "00000123_precalculated_vanadium_run_integration.nxs")));
  }

  void test_cacheUsedWhenAvailable() {
    const auto curvesWS = createSampleMatrixWorkspace();
    const auto integratedWS = createSampleTableWorkspace();
    writeOutSampleCorrectionWorkspaces(integratedWS, curvesWS);

    EnggDiffCalibSettings calibSettings;
    calibSettings.m_inputDirCalib = m_inputDir.string();
    calibSettings.m_forceRecalcOverwrite = false;
    TestEnggVanadiumCorrectionsModel model(calibSettings, CURRENT_INSTRUMENT);

    std::pair<Mantid::API::ITableWorkspace_sptr,
              Mantid::API::MatrixWorkspace_sptr>
        correctionWorkspaces;
    TS_ASSERT_THROWS_NOTHING(correctionWorkspaces =
                                 model.fetchCorrectionWorkspaces("123"));
    TS_ASSERT(!model.m_calculateCorrectionsCalled);

    TS_ASSERT_EQUALS(curvesWS->y(0), correctionWorkspaces.second->y(0));

    Mantid::API::TableRow sampleDataRow = integratedWS->getRow(0);
    Mantid::API::TableRow readDataRow = correctionWorkspaces.first->getRow(0);
    TS_ASSERT_EQUALS(sampleDataRow.Double(0), readDataRow.Double(0));
  }

  void test_recalculateIfRequired() {
    const auto curvesWS = createSampleMatrixWorkspace();
    const auto integratedWS = createSampleTableWorkspace();
    writeOutSampleCorrectionWorkspaces(integratedWS, curvesWS);

    EnggDiffCalibSettings calibSettings;
    calibSettings.m_inputDirCalib = m_inputDir.string();
    calibSettings.m_forceRecalcOverwrite = true;
    TestEnggVanadiumCorrectionsModel model(calibSettings, CURRENT_INSTRUMENT);

    std::pair<Mantid::API::ITableWorkspace_sptr,
              Mantid::API::MatrixWorkspace_sptr>
        correctionWorkspaces;
    TS_ASSERT_THROWS_NOTHING(correctionWorkspaces =
                                 model.fetchCorrectionWorkspaces("123"));
    TS_ASSERT(model.m_calculateCorrectionsCalled);
  }

private:
  const static std::string CURRENT_INSTRUMENT;
  const static boost::filesystem::path INPUT_DIR_NAME;

  boost::filesystem::path m_inputDir;

  void saveNexus(const std::string &filename,
                 const Mantid::API::Workspace_sptr workspace) const {
    auto save = Mantid::API::AlgorithmManager::Instance().create("SaveNexus");
    save->initialize();
    save->setProperty("InputWorkspace", workspace);
    save->setProperty("Filename", filename);
    save->execute();
  }

  void writeOutSampleCorrectionWorkspaces(
      Mantid::API::ITableWorkspace_sptr integratedWS,
      Mantid::API::MatrixWorkspace_sptr curvesWS) {
    saveNexus(
        (m_inputDir / (CURRENT_INSTRUMENT +
                       "00000123_precalculated_vanadium_run_bank_curves.nxs"))
            .string(),
        curvesWS);
    saveNexus(
        (m_inputDir / (CURRENT_INSTRUMENT +
                       "00000123_precalculated_vanadium_run_integration.nxs"))
            .string(),
        integratedWS);
  }
};

const std::string EnggVanadiumCorrectionsModelTest::CURRENT_INSTRUMENT =
    "TESTINST";

const boost::filesystem::path EnggVanadiumCorrectionsModelTest::INPUT_DIR_NAME(
    "EnggVanadiumCorrectionsModelTestData");

#endif // MANTIDQT_CUSTOMINTERFACES_ENGGVANADIUMCORRECTIONSMODELTEST_H_
