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
    m_presenter->notify(IImagingFormatsConvertPresenter::Init);
  }

  void test_convertFails() {
    m_presenter->notify(IImagingFormatsConvertPresenter::Convert);
  }

  void test_shutDown() {

    m_presenter->notify(IImagingFormatsConvertPresenter::ShutDown);
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
