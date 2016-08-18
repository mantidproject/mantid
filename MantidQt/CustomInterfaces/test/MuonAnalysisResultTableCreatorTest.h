#ifndef MANTIDQT_CUSTOMINTERFACES_MUONANALYSISRESULTTABLECREATORTEST_H_
#define MANTIDQT_CUSTOMINTERFACES_MUONANALYSISRESULTTABLECREATORTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidQtCustomInterfaces/Muon/MuonAnalysisResultTableCreator.h"

using MantidQt::CustomInterfaces::MuonAnalysisResultTableCreator;

class MuonAnalysisResultTableCreatorTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static MuonAnalysisResultTableCreatorTest *createSuite() {
    return new MuonAnalysisResultTableCreatorTest();
  }
  static void destroySuite(MuonAnalysisResultTableCreatorTest *suite) {
    delete suite;
  }

  void test_Something() { TS_FAIL("You forgot to write a test!"); } 
};

#endif /* MANTIDQT_CUSTOMINTERFACES_MUONANALYSISRESULTTABLECREATORTEST_H_ */