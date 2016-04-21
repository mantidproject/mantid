#ifndef MANTID_CUSTOMINTERFACES_IMAGINGFORMATSCONVERTPRESENTERTEST_H
#define MANTID_CUSTOMINTERFACES_IMAGINGFORMATSCONVERTPRESENTERTEST_H

#include "MantidAPI/FrameworkManager.h"
#include "MantidQtCustomInterfaces/Tomography/ImagingFormatsConvertPresenter.h"

#include <cxxtest/TestSuite.h>
#include "ImagingFormatsConvertViewMock.h"

using namespace MantidQt::CustomInterfaces;
using testing::TypedEq;
using testing::Return;

class ImagingFormatsConvertPresenterTest : public CxxTest::TestSuite {

public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static ImagingFormatsConvertPresenterTest *createSuite() {
    return new ImagingFormatsConvertPresenterTest();
  }

  static void destroySuite(ImagingFormatsConvertPresenterTest *suite) {
    delete suite;
  }

  ImagingFormatsConvertPresenterTest() {
    Mantid::API::FrameworkManager::Instance(); // make sure framework is
                                               // initialized
  }

  void setUp() override {
    m_view.reset(new testing::NiceMock<ImagingFormatsConvertViewMock>());
    m_presenter.reset(
        new MantidQt::CustomInterfaces::ImagingFormatsConvertPresenter(
            m_view.get()));
  }

  void tearDown() override {
    TS_ASSERT(testing::Mock::VerifyAndClearExpectations(m_view.get()));
  }

  void test_init() {
    testing::NiceMock<ImagingFormatsConvertViewMock> mockView;
    MantidQt::CustomInterfaces::ImagingFormatsConvertPresenter pres(&mockView);

    EXPECT_CALL(mockView, setFormats(testing::_, testing::_)).Times(1);

    // No errors, no warnings
    EXPECT_CALL(mockView, userError(testing::_, testing::_)).Times(0);
    EXPECT_CALL(mockView, userWarning(testing::_, testing::_)).Times(0);

    pres.notify(IImagingFormatsConvertPresenter::Init);
    TSM_ASSERT("Expected use of Mock view not satisfied.",
               testing::Mock::VerifyAndClearExpectations(&mockView));
  }

  void test_convertFails() {
    testing::NiceMock<ImagingFormatsConvertViewMock> mockView;
    MantidQt::CustomInterfaces::ImagingFormatsConvertPresenter pres(&mockView);

    EXPECT_CALL(mockView, inputPath()).Times(1).WillRepeatedly(Return(""));
    EXPECT_CALL(mockView, outputPath()).Times(1).WillRepeatedly(Return(""));

    pres.notify(IImagingFormatsConvertPresenter::Convert);
    TSM_ASSERT("Expected use of Mock view not satisfied.",
               testing::Mock::VerifyAndClearExpectations(&mockView));
  }

  void test_shutDown() {
    testing::NiceMock<ImagingFormatsConvertViewMock> mockView;
    MantidQt::CustomInterfaces::ImagingFormatsConvertPresenter pres(&mockView);

    EXPECT_CALL(mockView, saveSettings()).Times(1);

    // No errors, no warnings
    EXPECT_CALL(mockView, userError(testing::_, testing::_)).Times(0);
    EXPECT_CALL(mockView, userWarning(testing::_, testing::_)).Times(0);

    pres.notify(IImagingFormatsConvertPresenter::ShutDown);
    TSM_ASSERT("Expected use of Mock view not satisfied.",
               testing::Mock::VerifyAndClearExpectations(&mockView));
  }

private:
  std::unique_ptr<testing::NiceMock<ImagingFormatsConvertViewMock>> m_view;

  std::unique_ptr<MantidQt::CustomInterfaces::ImagingFormatsConvertPresenter>
      m_presenter;
  // To have one FITS, etc.
  Mantid::API::MatrixWorkspace_sptr m_ws;
  static std::string g_scarfName;
  static std::string g_ccpi;
};

#endif // MANTID_CUSTOMINTERFACES_IMAGINGFORMATSCONVERTPRESENTERTEST_H
