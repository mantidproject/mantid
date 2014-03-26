#ifndef MANTID_CUSTOMINTERFACES_ALCDATALOADINGTEST_H_
#define MANTID_CUSTOMINTERFACES_ALCDATALOADINGTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidQtCustomInterfaces/Muon/ALCDataLoading.h"

using namespace Mantid::CustomInterfaces;

class MockALCDataLoadingView : public IALCDataLoadingView
{
public:
  std::string firstRun() { return ""; }
  void setData() {}
};

class ALCDataLoadingTest : public CxxTest::TestSuite
{
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static ALCDataLoadingTest *createSuite() { return new ALCDataLoadingTest(); }
  static void destroySuite( ALCDataLoadingTest *suite ) { delete suite; }

  void test_initialize()
  {
    MockALCDataLoadingView view;
    ALCDataLoading step(&view);
    TS_ASSERT_THROWS_NOTHING(step.initialize());
  }
};


#endif /* MANTID_CUSTOMINTERFACES_ALCDATALOADINGTEST_H_ */
