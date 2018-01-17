#ifndef MANTIDQT_CUSTOMINTERFACES_ENGGDIFFGSASFITTINGMODELTEST_H_
#define MANTIDQT_CUSTOMINTERFACES_ENGGDIFFGSASFITTINGMODELTEST_H_

#include "../EnggDiffraction/EnggDiffGSASFittingModel.h"

#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/TableRow.h"
#include "MantidAPI/WorkspaceFactory.h"

#include <cxxtest/TestSuite.h>

using namespace Mantid;
using namespace MantidQt::CustomInterfaces;

namespace { // Helpers

template <size_t numColumns, size_t numRows>
API::ITableWorkspace_sptr createDummyTable(
    const std::array<std::string, numColumns> &columnHeadings,
    const std::array<std::array<double, numColumns>, numRows> tableContents) {
  auto table = API::WorkspaceFactory::Instance().createTable();
  for (const auto &header : columnHeadings) {
    table->addColumn("double", header);
  }
  for (const auto &row : tableContents) {
    API::TableRow newRow = table->appendRow();
    for (const auto value : row) {
      newRow << value;
    }
  }
  return table;
}

// Helper class with some protected methods exposed
class TestEnggDiffGSASFittingModel : public EnggDiffGSASFittingModel {
public:
  bool containsFocusedRun(const int runNumber, const size_t bank) const;

  void addFittedPeaksWS(const int runNumber, const size_t bank,
                        API::MatrixWorkspace_sptr ws);

  void addFocusedWorkspace(const int runNumber, const size_t bank,
                           API::MatrixWorkspace_sptr ws);

  void addLatticeParamTable(const int runNumber, const size_t bank,
                            API::ITableWorkspace_sptr table);

  void addRwpValue(const int runNumber, const size_t bank, const double rwp);
};

inline void TestEnggDiffGSASFittingModel::addFittedPeaksWS(
    const int runNumber, const size_t bank, API::MatrixWorkspace_sptr ws) {
  addFittedPeaks(runNumber, bank, ws);
}

inline void TestEnggDiffGSASFittingModel::addFocusedWorkspace(
    const int runNumber, const size_t bank, API::MatrixWorkspace_sptr ws) {
  addFocusedRun(runNumber, bank, ws);
}

inline void TestEnggDiffGSASFittingModel::addLatticeParamTable(
    const int runNumber, const size_t bank, API::ITableWorkspace_sptr table) {
  addLatticeParams(runNumber, bank, table);
}

inline void TestEnggDiffGSASFittingModel::addRwpValue(const int runNumber,
                                                      const size_t bank,
                                                      const double rwp) {
  addRwp(runNumber, bank, rwp);
}

inline bool
TestEnggDiffGSASFittingModel::containsFocusedRun(const int runNumber,
                                                 const size_t bank) const {
  return hasFocusedRun(runNumber, bank);
}

} // Anonymous namespace

class EnggDiffGSASFittingModelTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static EnggDiffGSASFittingModelTest *createSuite() {
    return new EnggDiffGSASFittingModelTest();
  }
  static void destroySuite(EnggDiffGSASFittingModelTest *suite) {
    delete suite;
  }

  EnggDiffGSASFittingModelTest() { API::FrameworkManager::Instance(); }

  void test_validLoadRun() {
    const static std::string inputFilename = "ENGINX_277208_focused_bank_2.nxs";
    TestEnggDiffGSASFittingModel model;

    bool success = false;
    TS_ASSERT_THROWS_NOTHING(success = model.loadFocusedRun(inputFilename));
    TS_ASSERT(success);

    TS_ASSERT(model.containsFocusedRun(277208, 2));
  }

  void test_invalidLoadRun() {
    const static std::string inputFilename = "ENGINX_277209_focused_bank_2.nxs";
    TestEnggDiffGSASFittingModel model;

    bool success = false;
    TS_ASSERT_THROWS_NOTHING(success = model.loadFocusedRun(inputFilename));
    TS_ASSERT(!success);
  }

  void test_getFocusedRun() {
    TestEnggDiffGSASFittingModel model;

    API::MatrixWorkspace_sptr ws =
        API::WorkspaceFactory::Instance().create("Workspace2D", 1, 10, 10);

    model.addFocusedWorkspace(123, 1, ws);

    boost::optional<API::MatrixWorkspace_sptr> retrievedWS;
    TS_ASSERT_THROWS_NOTHING(retrievedWS = model.getFocusedWorkspace(123, 1));
    TS_ASSERT(retrievedWS);
    TS_ASSERT_EQUALS(ws, *retrievedWS);

    TS_ASSERT_THROWS_NOTHING(retrievedWS = model.getFocusedWorkspace(456, 2));
    TS_ASSERT_EQUALS(retrievedWS, boost::none);
  }

  void test_getRunLabels() {
    TestEnggDiffGSASFittingModel model;

    for (int i = 1; i < 5; i++) {
      API::MatrixWorkspace_sptr ws =
          API::WorkspaceFactory::Instance().create("Workspace2D", 1, 10, 10);
      model.addFocusedWorkspace(i * 111, i % 2 + 1, ws);
    }

    std::vector<std::pair<int, size_t>> runLabels;
    TS_ASSERT_THROWS_NOTHING(runLabels = model.getRunLabels());

    TS_ASSERT_EQUALS(runLabels.size(), 4);
    for (int i = 1; i < 5; i++) {
      TS_ASSERT_EQUALS(runLabels[i - 1],
                       std::make_pair(i * 111, size_t(i % 2 + 1)));
    }
  }

  void test_getFittedPeaks() {
    TestEnggDiffGSASFittingModel model;

    API::MatrixWorkspace_sptr ws =
        API::WorkspaceFactory::Instance().create("Workspace2D", 1, 10, 10);

    model.addFittedPeaksWS(123, 1, ws);

    boost::optional<API::MatrixWorkspace_sptr> retrievedWS;
    TS_ASSERT_THROWS_NOTHING(retrievedWS = model.getFittedPeaks(123, 1));
    TS_ASSERT(retrievedWS);
    TS_ASSERT_EQUALS(ws, *retrievedWS);

    TS_ASSERT_THROWS_NOTHING(retrievedWS = model.getFittedPeaks(456, 2));
    TS_ASSERT_EQUALS(retrievedWS, boost::none);
  }

  void test_hasFittedPeaksForRun() {
    TestEnggDiffGSASFittingModel model;

    API::MatrixWorkspace_sptr ws =
        API::WorkspaceFactory::Instance().create("Workspace2D", 1, 10, 10);

    model.addFittedPeaksWS(123, 1, ws);
    TS_ASSERT(model.hasFittedPeaksForRun(123, 1));
    TS_ASSERT(!model.hasFittedPeaksForRun(456, 1));
  }

  void test_getRwp() {
    TestEnggDiffGSASFittingModel model;

    const double rwp = 75.5;
    model.addRwpValue(123, 1, rwp);

    boost::optional<double> retrievedRwp;
    TS_ASSERT_THROWS_NOTHING(retrievedRwp = model.getRwp(123, 1));
    TS_ASSERT(retrievedRwp);
    TS_ASSERT_EQUALS(rwp, *retrievedRwp);

    TS_ASSERT_THROWS_NOTHING(retrievedRwp = model.getRwp(456, 2));
    TS_ASSERT_EQUALS(retrievedRwp, boost::none);
  }

  void test_getLatticeParams() {
    const std::array<std::string, 3> columnHeadings = {{"a", "b", "c"}};
    const std::array<std::array<double, 3>, 1> targetTableValues = {
        {{1, 2, 3}}};
    const auto table = createDummyTable(columnHeadings, targetTableValues);

    TestEnggDiffGSASFittingModel model;

    TS_ASSERT_THROWS_NOTHING(model.addLatticeParamTable(123, 1, table));

    // auto retrievedTable = model.getLatticeParams(123, 1);
    boost::optional<API::ITableWorkspace_sptr> retrievedTable;
    TS_ASSERT_THROWS_NOTHING(retrievedTable = model.getLatticeParams(123, 1));
    TS_ASSERT(retrievedTable);

    API::TableRow row = (*retrievedTable)->getRow(0);
    const double expectedA = 1;
    const double expectedB = 2;
    const double expectedC = 3;
    auto a = expectedA + 1;
    auto b = expectedB + 1;
    auto c = expectedC + 1;

    TS_ASSERT_THROWS_NOTHING(row >> a >> b >> c);
    TS_ASSERT_EQUALS(a, expectedA);
    TS_ASSERT_EQUALS(b, expectedB);
    TS_ASSERT_EQUALS(c, expectedC);

    TS_ASSERT_THROWS_NOTHING(retrievedTable = model.getLatticeParams(456, 2));
    TS_ASSERT_EQUALS(retrievedTable, boost::none);
  }
};

#endif // MANTIDQT_CUSTOMINTERFACES_ENGGDIFFGSASFITTINGMODELTEST_H_
