#ifndef MANTID_CUSTOMINTERFACES_MUONANALYSISFITDATAHELPERTEST_H_
#define MANTID_CUSTOMINTERFACES_MUONANALYSISFITDATAHELPERTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidQtCustomInterfaces/Muon/MuonAnalysisFitDataHelper.h"

using MantidQt::CustomInterfaces::MuonAnalysisFitDataHelper;

class MuonAnalysisFitDataHelperTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static MuonAnalysisFitDataHelperTest *createSuite() {
    return new MuonAnalysisFitDataHelperTest();
  }
  static void destroySuite(MuonAnalysisFitDataHelperTest *suite) {
    delete suite;
  }

  void test_Something() { TS_FAIL("You forgot to write a test!"); }
};

#endif /* MANTID_CUSTOMINTERFACES_MUONANALYSISFITDATAHELPERTEST_H_ */