#ifndef MANTIDQT_CUSTOMINTERFACES_MUONANALYSISDATALOADERTEST_H_
#define MANTIDQT_CUSTOMINTERFACES_MUONANALYSISDATALOADERTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidQtCustomInterfaces/Muon/MuonAnalysisDataLoader.h"

using MantidQt::CustomInterfaces::MuonAnalysisDataLoader;

class MuonAnalysisDataLoaderTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static MuonAnalysisDataLoaderTest *createSuite() { return new MuonAnalysisDataLoaderTest(); }
  static void destroySuite( MuonAnalysisDataLoaderTest *suite ) { delete suite; }


  void test_Something()
  {
    TS_FAIL( "You forgot to write a test!");
  }


};


#endif /* MANTIDQT_CUSTOMINTERFACES_MUONANALYSISDATALOADERTEST_H_ */