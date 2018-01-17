#ifndef MANTIDQT_CUSTOMINTERFACES_ENGGDIFFGSASFITTINGMODELTEST_H_
#define MANTIDQT_CUSTOMINTERFACES_ENGGDIFFGSASFITTINGMODELTEST_H_

#include "../EnggDiffraction/EnggDiffGSASFittingModel.h"

#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/WorkspaceFactory.h"

#include <cxxtest/TestSuite.h>

using namespace Mantid;
using namespace MantidQt::CustomInterfaces;

namespace { // Helpers

// Helper class with some protected methods exposed
class TestEnggDiffGSASFittingModel : public EnggDiffGSASFittingModel {
public:
  bool containsFocusedRun(const int runNumber, const size_t bank) const;

  void addFittedPeaksWS(const int runNumber, const size_t bank,
                        API::MatrixWorkspace_sptr ws);

  void addFocusedWorkspace(const int runNumber, const size_t bank,
                           API::MatrixWorkspace_sptr ws);
};

inline void TestEnggDiffGSASFittingModel::addFittedPeaksWS(
    const int runNumber, const size_t bank, API::MatrixWorkspace_sptr ws) {
  addFittedPeaks(runNumber, bank, ws);
}

inline void TestEnggDiffGSASFittingModel::addFocusedWorkspace(
    const int runNumber, const size_t bank, API::MatrixWorkspace_sptr ws) {
  addFocusedRun(runNumber, bank, ws);
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
};

#endif // MANTIDQT_CUSTOMINTERFACES_ENGGDIFFGSASFITTINGMODELTEST_H_
