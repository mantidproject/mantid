#ifndef MANTID_CUSTOMINTERFACES_REFLMAINVIEWPRESENTERTEST_H
#define MANTID_CUSTOMINTERFACES_REFLMAINVIEWPRESENTERTEST_H

#include <cxxtest/TestSuite.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "MantidAPI/FrameworkManager.h"
#include "MantidQtCustomInterfaces/Reflectometry/ReflMainViewPresenter.h"

#include "ProgressableViewMockObject.h"
#include "ReflMainViewMockObjects.h"

using namespace MantidQt::CustomInterfaces;
using namespace Mantid::API;
using namespace Mantid::Kernel;
using namespace testing;

//=====================================================================================
// Functional tests
//=====================================================================================
class ReflMainViewPresenterTest : public CxxTest::TestSuite {

private:
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static ReflMainViewPresenterTest *createSuite() {
    return new ReflMainViewPresenterTest();
  }
  static void destroySuite(ReflMainViewPresenterTest *suite) { delete suite; }

  ReflMainViewPresenterTest() { FrameworkManager::Instance(); }

  void test_constructor_sets_possible_transfer_methods() {
    NiceMock<MockView> mockView;
    MockProgressableView mockProgress;

    // Expect that the transfer methods get initialized on the view
    EXPECT_CALL(mockView, setTransferMethods(_)).Times(Exactly(1));

    // Constructor
    ReflMainViewPresenter presenter(&mockView, &mockProgress);

    // Verify expectations
    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockView));
  }
};
#endif /* MANTID_CUSTOMINTERFACES_REFLMAINVIEWPRESENTERTEST_H */
