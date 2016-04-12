#ifndef MANTID_MANTIDWIDGETS_MUONFITDATAPRESENTERTEST_H_
#define MANTID_MANTIDWIDGETS_MUONFITDATAPRESENTERTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidQtMantidWidgets/MuonFitDataPresenter.h"

using MantidQt::MantidWidgets::MuonFitDataPresenter;

class MuonFitDataPresenterTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static MuonFitDataPresenterTest *createSuite() { return new MuonFitDataPresenterTest(); }
  static void destroySuite( MuonFitDataPresenterTest *suite ) { delete suite; }


  void test_Something()
  {
    TS_FAIL( "You forgot to write a test!");
  }


};


#endif /* MANTID_MANTIDWIDGETS_MUONFITDATAPRESENTERTEST_H_ */