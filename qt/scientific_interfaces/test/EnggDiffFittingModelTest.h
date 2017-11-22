#ifndef MANTID_CUSTOMINTERFACES_ENGGDIFFFITTINGMODELTEST_H_
#define MANTID_CUSTOMINTERFACES_ENGGDIFFFITTINGMODELTEST_H_

#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/ITableWorkspace.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/Run.h"
#include "MantidAPI/TableRow.h"
#include "MantidAPI/WorkspaceFactory.h"

#include "../EnggDiffraction/EnggDiffFittingModel.h"

#include <cxxtest/TestSuite.h>
#include <vector>

// Lets us have pairs inside assertion macros
typedef std::pair<int, size_t> RunBankPair;

using namespace Mantid;
using namespace MantidQt::CustomInterfaces;

namespace {

// Helper class that exposes addFocusedWorkspace
// This means we can test the workspace maps without having to run Load
class EnggDiffFittingModelAddWSExposed : public EnggDiffFittingModel {
public:
  void addWorkspace(const int runNumber, const size_t bank,
                    Mantid::API::MatrixWorkspace_sptr ws);

  void addFitParams(const int runNumber, const size_t bank,
                    Mantid::API::ITableWorkspace_sptr ws);
};

inline void EnggDiffFittingModelAddWSExposed::addWorkspace(
    const int runNumber, const size_t bank, API::MatrixWorkspace_sptr ws) {
  addFocusedWorkspace(runNumber, bank, ws,
                      std::to_string(runNumber) + "_" + std::to_string(bank));
}

inline void EnggDiffFittingModelAddWSExposed::addFitParams(
    const int runNumber, const size_t bank,
    Mantid::API::ITableWorkspace_sptr ws) {
  addFitResults(runNumber, bank, ws);
}

void addSampleWorkspaceToModel(const int runNumber, const int bank,
                               EnggDiffFittingModelAddWSExposed &model) {
  API::MatrixWorkspace_sptr ws =
      API::WorkspaceFactory::Instance().create("Workspace2D", 1, 10, 10);
  model.addWorkspace(runNumber, bank, ws);
}

API::ITableWorkspace_sptr createFitParamsTable() {
  const size_t numColumns = 16;
  const size_t numRows = 4;
  auto table = API::WorkspaceFactory::Instance().createTable("TableWorkspace");
  const std::array<std::string, numColumns> headings({
      "dSpacing[Y]", "A0[Y]", "A0_Err[yEr]", "A1[Y]", "A1_Err[yEr]", "X0[Y]",
      "X0_Err[yEr]", "A[Y]", "A_Err[yEr]", "B[Y]", "B_Err[yEr]", "S[Y]",
      "S_Err[yEr]", "I[Y]", "I_Err[yEr]", "Chi[Y]",
  });

  for (const auto &columnHeading : headings) {
    table->addColumn("double", columnHeading);
  }

  const std::array<std::array<double, numColumns>, numRows> rows = {
      {{{1.4826999999999999, 0.093628531894011102, 0.66109193835092461,
         1.2564478992707699e-06, 2.4291293347225761e-05, 27140.960929827994,
         4.4430783321852303, 0.045621368052062856, 0.0092005773305902459,
         0.020298218347394655, 0.0025002243189996306, 11.741120992807753,
         5.3771683079349311, 34.202007864467461, 1.8695496489293224,
         1.4096728498206776}},
       {{1.7197, 1.0731062065126851, 0.72931461734063008,
         -2.9359794063082084e-05, 2.285663646689115e-05, 31770.101042814735,
         5.6899014393655358, 0.050855278541599255, 0.013915934527381201,
         0.029076388335360012, 0.002935493268317269, 27.132751332587915,
         4.5849081323418064, 89.646425792809978, 2.1570533782524279,
         0.79304374868658656}},
       {{2.2399, 1.3229681799066122, 0.45360789821414083,
         -3.0219780224537017e-05, 1.0941426250415265e-05, 41266.973604075109,
         4.0391546488412224, 0.043604800066098286, 0.0071406722143233931,
         0.021740542092941812, 0.001008755490980281, 36.523446658868707,
         3.2982922870662814, 205.36292151601506, 2.3728608996241367,
         0.90144473999482344}},
       {{2.552, 0.46162942972449567, 0.24323265893625406,
         -9.0850559562388256e-06, 5.1638893666718458e-06, 46982.314791027922,
         46.041577282817634, 0.14208244137460718, 0.61720906575104273,
         0.018444321135930489, 0.0078725143001187933, 45.171720946242374,
         18.656365897259217, 14.950355673087914, 1.02699955199189,
         0.68147322764610252}}}};

  for (const auto &row : rows) {
    API::TableRow tableRow = table->appendRow();
    for (const auto entry : row) {
      tableRow << entry;
    }
  }
  return table;
}

} // anonymous namespace

class EnggDiffFittingModelTest : public CxxTest::TestSuite {

public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static EnggDiffFittingModelTest *createSuite() {
    return new EnggDiffFittingModelTest();
  }
  static void destroySuite(EnggDiffFittingModelTest *suite) { delete suite; }

  EnggDiffFittingModelTest() { API::FrameworkManager::Instance(); }

  void test_addAndGetWorkspace() {
    auto model = EnggDiffFittingModelAddWSExposed();
    API::MatrixWorkspace_sptr ws =
        API::WorkspaceFactory::Instance().create("Workspace2D", 1, 10, 10);
    const int runNumber = 100;
    const int bank = 1;
    TS_ASSERT_THROWS_NOTHING(model.addWorkspace(runNumber, bank, ws));
    const auto retrievedWS = model.getFocusedWorkspace(runNumber, bank);

    TS_ASSERT(retrievedWS != nullptr);
    TS_ASSERT_EQUALS(ws, retrievedWS);
  }

  void test_getRunNumbersAndBankIDs() {
    auto model = EnggDiffFittingModelAddWSExposed();

    addSampleWorkspaceToModel(123, 1, model);
    addSampleWorkspaceToModel(456, 2, model);
    addSampleWorkspaceToModel(789, 1, model);
    addSampleWorkspaceToModel(123, 2, model);

    const auto runNoBankPairs = model.getRunNumbersAndBankIDs();

    TS_ASSERT_EQUALS(runNoBankPairs.size(), 4);
    TS_ASSERT_EQUALS(runNoBankPairs[0], RunBankPair(123, 1));
    TS_ASSERT_EQUALS(runNoBankPairs[1], RunBankPair(123, 2));
    TS_ASSERT_EQUALS(runNoBankPairs[2], RunBankPair(456, 2));
    TS_ASSERT_EQUALS(runNoBankPairs[3], RunBankPair(789, 1));
  }

  void test_loadWorkspaces() {
    auto model = EnggDiffFittingModel();
    TS_ASSERT_THROWS_NOTHING(model.loadWorkspaces(FOCUSED_WS_FILENAME));
    API::MatrixWorkspace_sptr ws;

    TS_ASSERT_THROWS_NOTHING(
        ws = model.getFocusedWorkspace(FOCUSED_WS_RUN_NUMBER, FOCUSED_WS_BANK));
    TS_ASSERT_EQUALS(ws->getNumberHistograms(), 1);
    TS_ASSERT_EQUALS(ws->getRunNumber(), FOCUSED_WS_RUN_NUMBER);
  }

  void test_setDifcTzero() {
    auto model = EnggDiffFittingModel();
    TS_ASSERT_THROWS_NOTHING(model.loadWorkspaces(FOCUSED_WS_FILENAME));

    TS_ASSERT_THROWS_NOTHING(
        model.setDifcTzero(FOCUSED_WS_RUN_NUMBER, FOCUSED_WS_BANK,
                           std::vector<GSASCalibrationParms>()));
    auto ws = model.getFocusedWorkspace(FOCUSED_WS_RUN_NUMBER, FOCUSED_WS_BANK);
    auto run = ws->run();
    TS_ASSERT(run.hasProperty("difa"));
    TS_ASSERT(run.hasProperty("difc"));
    TS_ASSERT(run.hasProperty("tzero"));
  }

  void test_createFittedPeaksWS() {
    auto model = EnggDiffFittingModelAddWSExposed();

    const auto fitParams = createFitParamsTable();
    TS_ASSERT_THROWS_NOTHING(
        model.addFitParams(FOCUSED_WS_RUN_NUMBER, FOCUSED_WS_BANK, fitParams));
    TS_ASSERT_THROWS_NOTHING(model.loadWorkspaces(FOCUSED_WS_FILENAME));

    TS_ASSERT_THROWS_NOTHING(
        model.setDifcTzero(FOCUSED_WS_RUN_NUMBER, FOCUSED_WS_BANK,
                           std::vector<GSASCalibrationParms>()));
    TS_ASSERT_THROWS_NOTHING(
        model.createFittedPeaksWS(FOCUSED_WS_RUN_NUMBER, FOCUSED_WS_BANK));

    API::MatrixWorkspace_sptr fittedPeaksWS;
    TS_ASSERT_THROWS_NOTHING(fittedPeaksWS = model.getFittedPeaksWS(
                                 FOCUSED_WS_RUN_NUMBER, FOCUSED_WS_BANK));
    TS_ASSERT_EQUALS(fittedPeaksWS->getNumberHistograms(), 4);
  }

void test_setDifcTzero() {
    auto model = EnggDiffFittingModel();
    TS_ASSERT_THROWS_NOTHING(model.loadWorkspaces(FOCUSED_WS_FILENAME));

    TS_ASSERT_THROWS_NOTHING(
        model.setDifcTzero(FOCUSED_WS_RUN_NUMBER, FOCUSED_WS_BANK,
                           std::vector<GSASCalibrationParms>()));
    auto ws = model.getFocusedWorkspace(FOCUSED_WS_RUN_NUMBER, FOCUSED_WS_BANK);
    auto run = ws->run();
    TS_ASSERT(run.hasProperty("difa"));
    TS_ASSERT(run.hasProperty("difc"));
    TS_ASSERT(run.hasProperty("tzero"));
  }

  void test_createFittedPeaksWS() {
    auto model = EnggDiffFittingModelAddWSExposed();

    const auto fitParams = createFitParamsTable();
    TS_ASSERT_THROWS_NOTHING(
        model.addFitParams(FOCUSED_WS_RUN_NUMBER, FOCUSED_WS_BANK, fitParams));
    TS_ASSERT_THROWS_NOTHING(model.loadWorkspaces(FOCUSED_WS_FILENAME));

    TS_ASSERT_THROWS_NOTHING(
        model.setDifcTzero(FOCUSED_WS_RUN_NUMBER, FOCUSED_WS_BANK,
                           std::vector<GSASCalibrationParms>()));
    TS_ASSERT_THROWS_NOTHING(
        model.createFittedPeaksWS(FOCUSED_WS_RUN_NUMBER, FOCUSED_WS_BANK));

    API::MatrixWorkspace_sptr fittedPeaksWS;
    TS_ASSERT_THROWS_NOTHING(fittedPeaksWS = model.getFittedPeaksWS(
                                 FOCUSED_WS_RUN_NUMBER, FOCUSED_WS_BANK));
    TS_ASSERT_EQUALS(fittedPeaksWS->getNumberHistograms(), 4);
  }

private:
  const static std::string FOCUSED_WS_FILENAME;
  const static int FOCUSED_WS_RUN_NUMBER;
  const static size_t FOCUSED_WS_BANK;
};

const std::string EnggDiffFittingModelTest::FOCUSED_WS_FILENAME =
    "ENGINX_277208_focused_bank_2.nxs";

const int EnggDiffFittingModelTest::FOCUSED_WS_RUN_NUMBER = 277208;

const size_t EnggDiffFittingModelTest::FOCUSED_WS_BANK = 2;

#endif
