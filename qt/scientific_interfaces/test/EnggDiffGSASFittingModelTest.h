#ifndef MANTIDQT_CUSTOMINTERFACES_ENGGDIFFGSASFITTINGMODELTEST_H_
#define MANTIDQT_CUSTOMINTERFACES_ENGGDIFFGSASFITTINGMODELTEST_H_

#include "../EnggDiffraction/EnggDiffGSASFittingModel.h"

#include "MantidAPI/FrameworkManager.h"

#include <cxxtest/TestSuite.h>

using namespace MantidQt::CustomInterfaces;

namespace { // Helpers

// Helper class with some protected methods exposed
class TestEnggDiffGSASFittingModel : public EnggDiffGSASFittingModel {
public:
  bool containsFocusedRun(const int runNumber, const size_t bank) const;
};

bool TestEnggDiffGSASFittingModel::containsFocusedRun(const int runNumber,
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

  EnggDiffGSASFittingModelTest() { Mantid::API::FrameworkManager::Instance(); }

  void test_validLoadRun() {
    const static std::string inputFilename = "ENGINX_277208_focused_bank_2.nxs";
    TestEnggDiffGSASFittingModel model;

    bool success = false;
    TS_ASSERT_THROWS_NOTHING(success = model.loadFocusedRun(inputFilename));
    TS_ASSERT(success);

    TS_ASSERT(model.containsFocusedRun(277208, 2));
  }
};

#endif // MANTIDQT_CUSTOMINTERFACES_ENGGDIFFGSASFITTINGMODELTEST_H_
