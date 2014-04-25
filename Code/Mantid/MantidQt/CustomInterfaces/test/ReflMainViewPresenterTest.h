#ifndef CUSTOM_INTERFACES_REFLMAINVIEWPRESENTER_TEST_H_
#define CUSTOM_INTERFACES_REFLMAINVIEWPRESENTER_TEST_H_

#include <cxxtest/TestSuite.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <boost/make_shared.hpp>
#include "MantidDataObjects/TableWorkspace.h"
#include "MantidAPI/ITableWorkspace.h"
#include "MantidQtCustomInterfaces/ReflMainView.h"
#include "MantidQtCustomInterfaces/ReflMainViewPresenter.h"

using namespace MantidQt::CustomInterfaces;
using namespace testing;

//=====================================================================================
// Functional tests
//=====================================================================================
class ReflMainViewPresenterTest : public CxxTest::TestSuite
{

private:

  class MockView : public ReflMainView
  {
    public:
      MockView(){};
      MOCK_METHOD1(showTable, void(Mantid::API::ITableWorkspace_sptr));
      virtual ~MockView(){}
  };

public:

  void testShowModel()
  {
    MockView mockView;
    EXPECT_CALL(mockView, showTable(_)).Times(1);
    ReflMainViewPresenter presenter(boost::make_shared<Mantid::DataObjects::TableWorkspace>(),&mockView);
    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockView));
  }
};
#endif
