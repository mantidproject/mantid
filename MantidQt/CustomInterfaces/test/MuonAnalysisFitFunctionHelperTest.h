#ifndef MANTID_CUSTOMINTERFACES_MUONANALYSISFITFUNCTIONHELPERTEST_H_
#define MANTID_CUSTOMINTERFACES_MUONANALYSISFITFUNCTIONHELPERTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidQtCustomInterfaces/Muon/MuonAnalysisFitFunctionHelper.h"

using MantidQt::CustomInterfaces::MuonAnalysisFitFunctionHelper;

class MuonAnalysisFitFunctionHelperTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static MuonAnalysisFitFunctionHelperTest *createSuite() {
    return new MuonAnalysisFitFunctionHelperTest();
  }
  static void destroySuite(MuonAnalysisFitFunctionHelperTest *suite) {
    delete suite;
  }

  void test_Something() { TS_FAIL("You forgot to write a test!"); }
};

#endif /* MANTID_CUSTOMINTERFACES_MUONANALYSISFITFUNCTIONHELPERTEST_H_ */