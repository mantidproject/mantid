#ifndef MANTID_CUSTOMINTERFACES_ALCLATESTFILEFINDERTEST_H_
#define MANTID_CUSTOMINTERFACES_ALCLATESTFILEFINDERTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidQtCustomInterfaces/Muon/ALCLatestFileFinder.h"

using MantidQt::CustomInterfaces::ALCLatestFileFinder;

class ALCLatestFileFinderTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static ALCLatestFileFinderTest *createSuite() { return new ALCLatestFileFinderTest(); }
  static void destroySuite( ALCLatestFileFinderTest *suite ) { delete suite; }


  void test_Something()
  {
    TS_FAIL( "You forgot to write a test!");
  }


};


#endif /* MANTID_CUSTOMINTERFACES_ALCLATESTFILEFINDERTEST_H_ */