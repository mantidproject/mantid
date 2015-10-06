#ifndef MANTID_CUSTOMINTERFACES_IMAGECORPRESENTERTEST_H
#define MANTID_CUSTOMINTERFACES_IMAGECORPRESENTERTEST_H

#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidQtCustomInterfaces/Tomography/ImageCoRPresenter.h"

#include <cxxtest/TestSuite.h>

#include "ImageCoRViewMock.h"

using namespace MantidQt::CustomInterfaces;
using testing::TypedEq;
using testing::Return;

class ImageCoRPresenterTest : public CxxTest::TestSuite {

public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static ImageCoRPresenterTest *createSuite() {
    return new ImageCoRPresenterTest();
  }

  static void destroySuite(ImageCoRPresenterTest *suite) { delete suite; }

  ImageCoRPresenterTest() {
    Mantid::API::FrameworkManager::Instance(); // make sure the framework is
                                               // initialized
  }

  void setUp() {
    m_view.reset(new testing::NiceMock<MockImageCoRView>());
    m_presenter.reset(
        new MantidQt::CustomInterfaces::ImageCoRPresenter(m_view.get()));
  }

  void tearDown() {
    TS_ASSERT(testing::Mock::VerifyAndClearExpectations(m_view.get()));
  }

  void test_initOK() {
    testing::NiceMock<MockImageCoRView> mockView;
    MantidQt::CustomInterfaces::ImageCoRPresenter pres(&mockView);

    EXPECT_CALL(mockView, setParams(testing::_)).Times(1);

    // No errors/warnings
    EXPECT_CALL(mockView, userError(testing::_, testing::_)).Times(0);
    EXPECT_CALL(mockView, userWarning(testing::_, testing::_)).Times(0);

    pres.notify(ImageCoRPresenter::Init);
  }

  void test_initWithWrongParams() {
    testing::NiceMock<MockImageCoRView> mockView;
    MantidQt::CustomInterfaces::ImageCoRPresenter pres(&mockView);

    EXPECT_CALL(mockView, setParams(testing::_)).Times(1);

    // One error, no warnings
    EXPECT_CALL(mockView, userError(testing::_, testing::_)).Times(0);
    EXPECT_CALL(mockView, userWarning(testing::_, testing::_)).Times(0);

    pres.notify(ImageCoRPresenter::Init);
  }

  void test_browseImgEmptyPath() {
    testing::NiceMock<MockImageCoRView> mockView;
    MantidQt::CustomInterfaces::ImageCoRPresenter pres(&mockView);

    EXPECT_CALL(mockView, askImgOrStackPath()).Times(1).WillOnce(Return(""));

    // No error, no warnings, just ignored
    EXPECT_CALL(mockView, userError(testing::_, testing::_)).Times(0);
    EXPECT_CALL(mockView, userWarning(testing::_, testing::_)).Times(0);

    // should not get there:
    EXPECT_CALL(mockView, showStack(testing::An<const std::string &>()))
        .Times(0);
    EXPECT_CALL(mockView,
                showStack(testing::An<Mantid::API::WorkspaceGroup_sptr &>(),
                          testing::An<const std::string &>())).Times(0);
    EXPECT_CALL(mockView, updateImgWithIndex(testing::_)).Times(0);

    pres.notify(IImageCoRPresenter::BrowseImgOrStack);
  }

  void test_shutDown() {
    testing::NiceMock<MockImageCoRView> mockView;
    MantidQt::CustomInterfaces::ImageCoRPresenter pres(&mockView);

    EXPECT_CALL(mockView, saveSettings()).Times(1);
    // No errors, no warnings
    EXPECT_CALL(mockView, userError(testing::_, testing::_)).Times(0);
    EXPECT_CALL(mockView, userWarning(testing::_, testing::_)).Times(0);

    pres.notify(ImageCoRPresenter::ShutDown);
  }

private:
  // boost::shared_ptr
  boost::scoped_ptr<testing::NiceMock<MockImageCoRView>> m_view;
  boost::scoped_ptr<MantidQt::CustomInterfaces::ImageCoRPresenter> m_presenter;

  // To have one FITS, etc.
  Mantid::API::MatrixWorkspace_sptr m_ws;
};

#endif // MANTID_CUSTOMINTERFACES_IMAGECORPRESENTERTEST_H
