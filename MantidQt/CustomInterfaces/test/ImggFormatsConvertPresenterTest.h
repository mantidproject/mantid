#ifndef MANTID_CUSTOMINTERFACES_IMGGFORMATSCONVERTPRESENTERTEST_H
#define MANTID_CUSTOMINTERFACES_IMGGFORMATSCONVERTPRESENTERTEST_H

#include "MantidAPI/FrameworkManager.h"
#include "MantidQtCustomInterfaces/Tomography/ImggFormatsConvertPresenter.h"

#include <cxxtest/TestSuite.h>
#include "ImggFormatsConvertViewMock.h"

using namespace MantidQt::CustomInterfaces;
using testing::TypedEq;
using testing::Return;

class ImggFormatsConvertPresenterTest : public CxxTest::TestSuite {

public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static ImggFormatsConvertPresenterTest *createSuite() {
    return new ImggFormatsConvertPresenterTest();
  }

  static void destroySuite(ImggFormatsConvertPresenterTest *suite) {
    delete suite;
  }

  ImggFormatsConvertPresenterTest() {
    Mantid::API::FrameworkManager::Instance(); // make sure framework is
                                               // initialized
  }

  void setUp() override {
    m_view.reset(new testing::NiceMock<ImggFormatsConvertViewMock>());
    m_presenter.reset(
        new MantidQt::CustomInterfaces::ImggFormatsConvertPresenter(
            m_view.get()));
  }

  void tearDown() override {
    TS_ASSERT(testing::Mock::VerifyAndClearExpectations(m_view.get()));
  }

  void test_init() {
    testing::NiceMock<ImggFormatsConvertViewMock> mockView;
    MantidQt::CustomInterfaces::ImggFormatsConvertPresenter pres(&mockView);

    EXPECT_CALL(mockView, setFormats(testing::_, testing::_, testing::_))
        .Times(1);

    // No errors, no warnings
    EXPECT_CALL(mockView, userError(testing::_, testing::_)).Times(0);
    EXPECT_CALL(mockView, userWarning(testing::_, testing::_)).Times(0);

    pres.notify(IImggFormatsConvertPresenter::Init);
    TSM_ASSERT("Mock view not used as expected. Some EXPECT_CALL conditions "
               "were not satisfied",
               testing::Mock::VerifyAndClearExpectations(&mockView));
  }

  void test_convertFails() {
    testing::NiceMock<ImggFormatsConvertViewMock> mockView;
    MantidQt::CustomInterfaces::ImggFormatsConvertPresenter pres(&mockView);

    EXPECT_CALL(mockView, inputPath()).Times(1).WillRepeatedly(Return(""));
    EXPECT_CALL(mockView, outputPath()).Times(1).WillRepeatedly(Return(""));
    EXPECT_CALL(mockView, maxSearchDepth()).Times(1).WillRepeatedly(Return(3));
    // should not get there
    EXPECT_CALL(mockView, inputFormatName()).Times(0);
    EXPECT_CALL(mockView, outputFormatName()).Times(0);

    // empty paths should produce an error
    EXPECT_CALL(mockView, userError(testing::_, testing::_)).Times(1);
    EXPECT_CALL(mockView, userWarning(testing::_, testing::_)).Times(0);

    pres.notify(IImggFormatsConvertPresenter::Convert);
    TSM_ASSERT("Mock view not used as expected. Some EXPECT_CALL conditions "
               "were not satisfied",
               testing::Mock::VerifyAndClearExpectations(&mockView));
  }

  void test_shutDown() {
    testing::NiceMock<ImggFormatsConvertViewMock> mockView;
    MantidQt::CustomInterfaces::ImggFormatsConvertPresenter pres(&mockView);

    EXPECT_CALL(mockView, saveSettings()).Times(1);

    // No errors, no warnings
    EXPECT_CALL(mockView, userError(testing::_, testing::_)).Times(0);
    EXPECT_CALL(mockView, userWarning(testing::_, testing::_)).Times(0);

    pres.notify(IImggFormatsConvertPresenter::ShutDown);
    TSM_ASSERT("Mock view not used as expected. Some EXPECT_CALL conditions "
               "were not satisfied",
               testing::Mock::VerifyAndClearExpectations(&mockView));
  }

private:
  std::unique_ptr<testing::NiceMock<ImggFormatsConvertViewMock>> m_view;

  std::unique_ptr<MantidQt::CustomInterfaces::ImggFormatsConvertPresenter>
      m_presenter;
  // To have one FITS, etc.
  Mantid::API::MatrixWorkspace_sptr m_ws;
  static std::string g_scarfName;
  static std::string g_ccpi;
};

#endif // MANTID_CUSTOMINTERFACES_IMGGFORMATSCONVERTPRESENTERTEST_H
