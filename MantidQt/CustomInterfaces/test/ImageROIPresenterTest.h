#ifndef MANTID_CUSTOMINTERFACES_IMAGEROIPRESENTERTEST_H
#define MANTID_CUSTOMINTERFACES_IMAGEROIPRESENTERTEST_H

#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidQtCustomInterfaces/Tomography/ImageROIPresenter.h"

#include <cxxtest/TestSuite.h>

#include <Poco/File.h>

#include "ImageROIViewMock.h"

using namespace MantidQt::CustomInterfaces;
using testing::TypedEq;
using testing::Return;

class ImageROIPresenterTest : public CxxTest::TestSuite {

public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static ImageROIPresenterTest *createSuite() {
    return new ImageROIPresenterTest();
  }

  static void destroySuite(ImageROIPresenterTest *suite) { delete suite; }

  ImageROIPresenterTest() {
    Mantid::API::FrameworkManager::Instance(); // make sure the framework is
                                               // initialized
  }

  void setUp() {
    m_view.reset(new testing::NiceMock<MockImageROIView>());
    m_presenter.reset(
        new MantidQt::CustomInterfaces::ImageROIPresenter(m_view.get()));
  }

  void tearDown() {
    TS_ASSERT(testing::Mock::VerifyAndClearExpectations(m_view.get()));
  }

  void test_initOK() {
    testing::NiceMock<MockImageROIView> mockView;
    MantidQt::CustomInterfaces::ImageROIPresenter pres(&mockView);

    EXPECT_CALL(mockView, setParams(testing::_)).Times(1);

    // No errors/warnings
    EXPECT_CALL(mockView, userError(testing::_, testing::_)).Times(0);
    EXPECT_CALL(mockView, userWarning(testing::_, testing::_)).Times(0);

    pres.notify(ImageROIPresenter::Init);
  }

  void test_initWithWrongParams() {
    testing::NiceMock<MockImageROIView> mockView;
    MantidQt::CustomInterfaces::ImageROIPresenter pres(&mockView);

    EXPECT_CALL(mockView, setParams(testing::_)).Times(1);

    // One error, no warnings
    EXPECT_CALL(mockView, userError(testing::_, testing::_)).Times(0);
    EXPECT_CALL(mockView, userWarning(testing::_, testing::_)).Times(0);

    pres.notify(ImageROIPresenter::Init);
  }

  void xxtest_browseImg_EmptyPath() {
    testing::NiceMock<MockImageROIView> mockView;
    MantidQt::CustomInterfaces::ImageROIPresenter pres(&mockView);

    EXPECT_CALL(mockView, askImgOrStackPath()).Times(1).WillOnce(Return(""));

    // No error, no warnings, just ignored
    EXPECT_CALL(mockView, userError(testing::_, testing::_)).Times(0);
    EXPECT_CALL(mockView, userWarning(testing::_, testing::_)).Times(0);

    // should not get there:
    EXPECT_CALL(mockView, showStack(testing::An<const std::string &>()))
        .Times(0);
    EXPECT_CALL(mockView,
                showStack(testing::An<Mantid::API::WorkspaceGroup_sptr &>()))
        .Times(0);
    EXPECT_CALL(mockView, updateImgWithIndex(testing::_)).Times(0);

    pres.notify(IImageROIPresenter::BrowseImgOrStack);
  }

  void xxtest_newImg_EmptyPath() {
    testing::NiceMock<MockImageROIView> mockView;
    MantidQt::CustomInterfaces::ImageROIPresenter pres(&mockView);

    EXPECT_CALL(mockView, askImgOrStackPath()).Times(0);

    // No error, one warning pop-up because a stack is not found
    EXPECT_CALL(mockView, userError(testing::_, testing::_)).Times(0);
    EXPECT_CALL(mockView, userWarning(testing::_, testing::_)).Times(1);

    // should not get there because there's no stack/img - it's just ignored:
    EXPECT_CALL(mockView, showStack(testing::An<const std::string &>()))
        .Times(0);
    EXPECT_CALL(mockView,
                showStack(testing::An<Mantid::API::WorkspaceGroup_sptr &>()))
        .Times(0);
    EXPECT_CALL(mockView, updateImgWithIndex(testing::_)).Times(0);

    pres.notify(IImageROIPresenter::NewImgOrStack);
  }

  void test_browseImg_WrongPath() {
    testing::NiceMock<MockImageROIView> mockView;
    MantidQt::CustomInterfaces::ImageROIPresenter pres(&mockView);

    EXPECT_CALL(mockView, askImgOrStackPath())
        .Times(1)
        .WillOnce(Return("dont_look_for_me_i_dont_exist"));

    // A warning
    EXPECT_CALL(mockView, userError(testing::_, testing::_)).Times(0);
    EXPECT_CALL(mockView, userWarning(testing::_, testing::_)).Times(1);

    // should not get there because there's no stack/img
    EXPECT_CALL(mockView, showStack(testing::An<const std::string &>()))
        .Times(0);
    EXPECT_CALL(mockView,
                showStack(testing::An<Mantid::API::WorkspaceGroup_sptr &>()))
        .Times(0);
    EXPECT_CALL(mockView, updateImgWithIndex(testing::_)).Times(0);

    // this exception is currently handled, and a warning given
    //TSM_ASSERT_THROWS("There should be an exception if there is an unexpected "
    //                  "error with the images path",
    //                  pres.notify(IImageROIPresenter::BrowseImgOrStack),
    //                  Poco::FileNotFoundException);
    pres.notify(IImageROIPresenter::BrowseImgOrStack);
  }

  void test_updateImgIndex() {
    testing::NiceMock<MockImageROIView> mockView;
    MantidQt::CustomInterfaces::ImageROIPresenter pres(&mockView);

    int idx = 0;
    EXPECT_CALL(mockView, currentImgIndex()).Times(1).WillOnce(Return(idx));

    EXPECT_CALL(mockView, updateImgWithIndex(idx)).Times(1);

    // No errors, no warnings
    EXPECT_CALL(mockView, userError(testing::_, testing::_)).Times(0);
    EXPECT_CALL(mockView, userWarning(testing::_, testing::_)).Times(0);

    pres.notify(IImageROIPresenter::UpdateImgIndex);
  }

  void test_selectCoR() {
    testing::NiceMock<MockImageROIView> mockView;
    MantidQt::CustomInterfaces::ImageROIPresenter pres(&mockView);

    EXPECT_CALL(mockView, changeSelectionState(IImageROIView::SelectCoR))
        .Times(1);

    // No errors, no warnings
    EXPECT_CALL(mockView, userError(testing::_, testing::_)).Times(0);
    EXPECT_CALL(mockView, userWarning(testing::_, testing::_)).Times(0);

    pres.notify(IImageROIPresenter::SelectCoR);
  }

  void test_resetCoR() {
    testing::NiceMock<MockImageROIView> mockView;
    MantidQt::CustomInterfaces::ImageROIPresenter pres(&mockView);

    EXPECT_CALL(mockView, resetCoR()).Times(1);
    EXPECT_CALL(mockView, changeSelectionState(IImageROIView::SelectNone))
        .Times(1);

    // just a few calls that should not happen
    EXPECT_CALL(mockView, resetROI()).Times(0);
    EXPECT_CALL(mockView, showStack(testing::An<const std::string &>()))
        .Times(0);
    EXPECT_CALL(mockView,
                showStack(testing::An<Mantid::API::WorkspaceGroup_sptr &>()))
        .Times(0);
    EXPECT_CALL(mockView, updateImgWithIndex(testing::_)).Times(0);

    // No errors, no warnings
    EXPECT_CALL(mockView, userError(testing::_, testing::_)).Times(0);
    EXPECT_CALL(mockView, userWarning(testing::_, testing::_)).Times(0);

    pres.notify(IImageROIPresenter::ResetCoR);
  }

  void test_selectROI() {
    testing::NiceMock<MockImageROIView> mockView;
    MantidQt::CustomInterfaces::ImageROIPresenter pres(&mockView);

    EXPECT_CALL(mockView, changeSelectionState(IImageROIView::SelectROIFirst))
        .Times(1);

    // No errors, no warnings
    EXPECT_CALL(mockView, userError(testing::_, testing::_)).Times(0);
    EXPECT_CALL(mockView, userWarning(testing::_, testing::_)).Times(0);

    pres.notify(IImageROIPresenter::SelectROI);
  }

  void test_finishROI() {
    testing::NiceMock<MockImageROIView> mockView;
    MantidQt::CustomInterfaces::ImageROIPresenter pres(&mockView);

    EXPECT_CALL(mockView, changeSelectionState(IImageROIView::SelectNone))
        .Times(1);

    // No errors, no warnings
    EXPECT_CALL(mockView, userError(testing::_, testing::_)).Times(0);
    EXPECT_CALL(mockView, userWarning(testing::_, testing::_)).Times(0);

    pres.notify(IImageROIPresenter::FinishedROI);
  }

  void test_resetROI() {
    testing::NiceMock<MockImageROIView> mockView;
    MantidQt::CustomInterfaces::ImageROIPresenter pres(&mockView);

    EXPECT_CALL(mockView, resetROI()).Times(1);
    EXPECT_CALL(mockView, changeSelectionState(IImageROIView::SelectNone))
        .Times(1);

    // just a few calls that should not happen
    EXPECT_CALL(mockView, resetCoR()).Times(0);
    EXPECT_CALL(mockView, showStack(testing::An<const std::string &>()))
        .Times(0);
    EXPECT_CALL(mockView,
                showStack(testing::An<Mantid::API::WorkspaceGroup_sptr &>()))
        .Times(0);
    EXPECT_CALL(mockView, updateImgWithIndex(testing::_)).Times(0);

    // No errors, no warnings
    EXPECT_CALL(mockView, userError(testing::_, testing::_)).Times(0);
    EXPECT_CALL(mockView, userWarning(testing::_, testing::_)).Times(0);

    pres.notify(IImageROIPresenter::ResetROI);
  }

  void test_selectNormalization() {
    testing::NiceMock<MockImageROIView> mockView;
    MantidQt::CustomInterfaces::ImageROIPresenter pres(&mockView);

    EXPECT_CALL(mockView, changeSelectionState(
                              IImageROIView::SelectNormAreaFirst)).Times(1);

    // No errors, no warnings
    EXPECT_CALL(mockView, userError(testing::_, testing::_)).Times(0);
    EXPECT_CALL(mockView, userWarning(testing::_, testing::_)).Times(0);

    pres.notify(IImageROIPresenter::SelectNormalization);
  }

  void test_finishNormalization() {
    testing::NiceMock<MockImageROIView> mockView;
    MantidQt::CustomInterfaces::ImageROIPresenter pres(&mockView);

    EXPECT_CALL(mockView, changeSelectionState(IImageROIView::SelectNone))
        .Times(1);

    // No errors, no warnings
    EXPECT_CALL(mockView, userError(testing::_, testing::_)).Times(0);
    EXPECT_CALL(mockView, userWarning(testing::_, testing::_)).Times(0);

    pres.notify(IImageROIPresenter::FinishedNormalization);
  }

  void test_resetNormalization() {
    testing::NiceMock<MockImageROIView> mockView;
    MantidQt::CustomInterfaces::ImageROIPresenter pres(&mockView);

    EXPECT_CALL(mockView, resetNormArea()).Times(1);
    EXPECT_CALL(mockView, changeSelectionState(IImageROIView::SelectNone))
        .Times(1);

    // just a few calls that should not happen
    EXPECT_CALL(mockView, resetCoR()).Times(0);
    EXPECT_CALL(mockView, resetROI()).Times(0);
    EXPECT_CALL(mockView, showStack(testing::An<const std::string &>()))
        .Times(0);
    EXPECT_CALL(mockView,
                showStack(testing::An<Mantid::API::WorkspaceGroup_sptr &>()))
        .Times(0);
    EXPECT_CALL(mockView, updateImgWithIndex(testing::_)).Times(0);

    // No errors, no warnings
    EXPECT_CALL(mockView, userError(testing::_, testing::_)).Times(0);
    EXPECT_CALL(mockView, userWarning(testing::_, testing::_)).Times(0);

    pres.notify(IImageROIPresenter::ResetNormalization);
  }

  void test_shutDown() {
    testing::NiceMock<MockImageROIView> mockView;
    MantidQt::CustomInterfaces::ImageROIPresenter pres(&mockView);

    EXPECT_CALL(mockView, saveSettings()).Times(1);
    // No errors, no warnings
    EXPECT_CALL(mockView, userError(testing::_, testing::_)).Times(0);
    EXPECT_CALL(mockView, userWarning(testing::_, testing::_)).Times(0);

    pres.notify(ImageROIPresenter::ShutDown);
  }

private:
  // boost::shared_ptr
  boost::scoped_ptr<testing::NiceMock<MockImageROIView>> m_view;
  boost::scoped_ptr<MantidQt::CustomInterfaces::ImageROIPresenter> m_presenter;

  // To have one FITS, etc.
  Mantid::API::MatrixWorkspace_sptr m_ws;
};

#endif // MANTID_CUSTOMINTERFACES_IMAGEROIPRESENTERTEST_H
