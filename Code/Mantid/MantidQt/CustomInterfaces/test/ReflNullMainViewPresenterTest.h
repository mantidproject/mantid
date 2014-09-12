#ifndef CUSTOM_INTERFACES_REFLNULLMAINVIEWPRESENTER_TEST_H_
#define CUSTOM_INTERFACES_REFLNULLMAINVIEWPRESENTER_TEST_H_

#include <cxxtest/TestSuite.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <boost/make_shared.hpp>
#include "MantidQtCustomInterfaces/ReflNullMainViewPresenter.h"

using namespace MantidQt::CustomInterfaces;
using namespace testing;

//=====================================================================================
// Functional tests
//=====================================================================================
class ReflNullMainViewPresenterTest : public CxxTest::TestSuite
{

public:

  void testNullThrow()
  {
    ReflNullMainViewPresenter presenter;
    TS_ASSERT_THROWS(presenter.notify(),std::runtime_error&);
  }
};
#endif
