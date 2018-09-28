#ifndef MANTID_CUSTOMINTERFACES_INSTRUMENTPRESENTERTEST_H_
#define MANTID_CUSTOMINTERFACES_INSTRUMENTPRESENTERTEST_H_

#include "../../../ISISReflectometry/GUI/Instrument/InstrumentPresenter.h"
#include "MockInstrumentView.h"

#include <cxxtest/TestSuite.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

using namespace MantidQt::CustomInterfaces;
using testing::Mock;
using testing::NiceMock;
using testing::Return;
using testing::_;

class InstrumentPresenterTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static InstrumentPresenterTest *createSuite() {
    return new InstrumentPresenterTest();
  }
  static void destroySuite(InstrumentPresenterTest *suite) { delete suite; }

  InstrumentPresenterTest() : m_view() {}

  bool verifyAndClearExpectations() {
    TS_ASSERT(Mock::VerifyAndClearExpectations(&m_view));
    return true;
  }

  InstrumentPresenter makePresenter(IInstrumentView &view) {
    return InstrumentPresenter(&view);
  }

  void test_this_is_a_test() {
    //...
  }

protected:
  MockInstrumentView m_view;
};

#endif // MANTID_CUSTOMINTERFACES_INSTRUMENTPRESENTERTEST_H_
