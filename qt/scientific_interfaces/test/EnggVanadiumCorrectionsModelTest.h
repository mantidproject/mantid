// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTIDQT_CUSTOMINTERFACES_ENGGVANADIUMCORRECTIONSMODELTEST_H_
#define MANTIDQT_CUSTOMINTERFACES_ENGGVANADIUMCORRECTIONSMODELTEST_H_

#include "../EnggDiffraction/EnggVanadiumCorrectionsModel.h"

#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/TableRow.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"

#include <Poco/File.h>
#include <Poco/Path.h>

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
  calculateCorrectionWorkspaces(
      const std::string &vanadiumRunNumber) const override;
};

inline TestEnggVanadiumCorrectionsModel::TestEnggVanadiumCorrectionsModel(
    const EnggDiffCalibSettings &calibSettings,
    const std::string &currentInstrument)
    : EnggVanadiumCorrectionsModel(calibSettings, currentInstrument),
      m_calculateCorrectionsCalled(false) {}

inline std::pair<Mantid::API::ITableWorkspace_sptr,
                 Mantid::API::MatrixWorkspace_sptr>
TestEnggVanadiumCorrectionsModel::calculateCorrectionWorkspaces(
    const std::string &) const {
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
    Poco::Path tempDir(Poco::Path::temp());
    tempDir.append(INPUT_DIR_NAME);
    m_inputDir = tempDir;
    Mantid::API::FrameworkManager::Instance();
  }

  void setUp() override { m_inputDir.createDirectory(); }

  void tearDown() override { m_inputDir.remove(true); }

  void test_generateNewWorkspacesWhenNoCache() {
    // We've created the calib directory but not populated it with any
    // workspaces, so we should get our fake ones
    EnggDiffCalibSettings calibSettings;
    calibSettings.m_inputDirCalib = m_inputDir.path();
    calibSettings.m_forceRecalcOverwrite = false;

    if (m_inputDir.exists()) {
      // Make sure that m_inputDir doesn't exist, as if a previous test exited
      // abnormally tearDown() may not have been called
      m_inputDir.remove(true);
    }

    TestEnggVanadiumCorrectionsModel model(calibSettings, CURRENT_INSTRUMENT);
    std::pair<Mantid::API::ITableWorkspace_sptr,
              Mantid::API::MatrixWorkspace_sptr>
        correctionWorkspaces;
    TS_ASSERT_THROWS_NOTHING(correctionWorkspaces =
                                 model.fetchCorrectionWorkspaces("123"));
    TS_ASSERT(model.m_calculateCorrectionsCalled);
    TS_ASSERT(correctionWorkspaces.first);
    TS_ASSERT(correctionWorkspaces.second);

    Poco::Path curvesWSPath(m_inputDir.path());
    curvesWSPath.append(CURRENT_INSTRUMENT +
                        "00000123_precalculated_vanadium_run_bank_curves.nxs");
    TS_ASSERT(Poco::File(curvesWSPath).exists());

    Poco::Path integWSPath(m_inputDir.path());
    integWSPath.append(CURRENT_INSTRUMENT +
                       "00000123_precalculated_vanadium_run_integration.nxs");
    TS_ASSERT(Poco::File(integWSPath).exists());
  }

  void test_cacheUsedWhenAvailable() {
    const auto curvesWS = createSampleMatrixWorkspace();
    const auto integratedWS = createSampleTableWorkspace();
    writeOutSampleCorrectionWorkspaces(integratedWS, curvesWS);

    EnggDiffCalibSettings calibSettings;
    calibSettings.m_inputDirCalib = m_inputDir.path();
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
    calibSettings.m_inputDirCalib = m_inputDir.path();
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
  const static std::string INPUT_DIR_NAME;

  Poco::File m_inputDir;

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
    Poco::Path curvesWSPath(m_inputDir.path());
    curvesWSPath.append(CURRENT_INSTRUMENT +
                        "00000123_precalculated_vanadium_run_bank_curves.nxs");
    saveNexus(curvesWSPath.toString(), curvesWS);

    Poco::Path integWSPath(m_inputDir.path());
    integWSPath.append(CURRENT_INSTRUMENT +
                       "00000123_precalculated_vanadium_run_integration.nxs");
    saveNexus(integWSPath.toString(), integratedWS);
  }
};

const std::string EnggVanadiumCorrectionsModelTest::CURRENT_INSTRUMENT =
    "TESTINST";

const std::string EnggVanadiumCorrectionsModelTest::INPUT_DIR_NAME(
    "EnggVanadiumCorrectionsModelTestData");

#endif // MANTIDQT_CUSTOMINTERFACES_ENGGVANADIUMCORRECTIONSMODELTEST_H_
