#ifndef CUSTOM_INTERFACES_REFLMAINVIEWPRESENTER_TEST_H_
#define CUSTOM_INTERFACES_REFLMAINVIEWPRESENTER_TEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidQtCustomInterfaces/ReflMainViewPresenter.h"

using namespace MantidQt::CustomInterfaces;

//=====================================================================================
// Functional tests
//=====================================================================================
class ReflMainViewPresenterTest : public CxxTest::TestSuite
{

public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static ReflMainViewPresenterTest *createSuite() { return new ReflMainViewPresenterTest(); }
  static void destroySuite( ReflMainViewPresenterTest *suite ) { delete suite; }


  void test_Something()
  {
    TSM_ASSERT( "You forgot to write a test!", 0);
  }

};
#endif
