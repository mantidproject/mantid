// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_MANTIDWIDGETS_OPTIONSDIALOGTEST_H
#define MANTID_MANTIDWIDGETS_OPTIONSDIALOGTEST_H

#include "MockOptionsDialogView.h"
#include "GUI/Options/OptionsDialogPresenter.h"
#include <cxxtest/TestSuite.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

using testing::_;
using testing::AtLeast;
using testing::Mock;
using testing::NiceMock;
using testing::Return;

using namespace MantidQt::CustomInterfaces::ISISReflectometry;

class OptionsDialogTest : public CxxTest::TestSuite {
public:
  static OptionsDialogTest *createSuite() { return new OptionsDialogTest(); }
  static void destroySuite(OptionsDialogTest *suite) { delete suite; }

  OptionsDialogTest() : m_view() {}

  void testPresenterSubscribesToView() {
    EXPECT_CALL(m_view, subscribe(_)).Times(1);
    auto presenter = makePresenter();
    verifyAndClear();
  }

  void testGetOptionsFromView() {
    auto presenter = makePresenter();
    std::map<std::string, bool> boolOptions;
    std::map<std::string, int> intOptions;
    EXPECT_CALL(m_view, getOptions(boolOptions, intOptions)).Times(1);
    verifyAndClear();
  }

  void testSetOptionsInView() {
    auto presenter = makePresenter();
    std::map<std::string, bool> boolOptions;
    std::map<std::string, int> intOptions;
    EXPECT_CALL(m_view, setOptions(boolOptions, intOptions)).Times(1);
    verifyAndClear();
  }

private:
  NiceMock<MockOptionsDialogView> m_view;

  OptionsDialogPresenter makePresenter() {
    auto presenter = OptionsDialogPresenter(&m_view);
    return presenter;
  }

  void verifyAndClear() {
    TS_ASSERT(Mock::VerifyAndClearExpectations(&m_view));
  }
};
#endif // MANTID_MANTIDWIDGETS_OPTIONSDIALOGTEST_H