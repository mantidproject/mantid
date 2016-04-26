#ifndef MANTID_CUSTOMINTERFACES_IMAGEROIPRESENTERTEST_H
#define MANTID_CUSTOMINTERFACES_IMAGEROIPRESENTERTEST_H

#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidQtCustomInterfaces/Tomography/ImageROIPresenter.h"
#include "MantidTestHelpers/FakeObjects.h"

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

  void setUp() override {
    m_view.reset(new testing::NiceMock<MockImageROIView>());
    m_presenter.reset(
        new MantidQt::CustomInterfaces::ImageROIPresenter(m_view.get()));
  }

  void tearDown() override {
    TS_ASSERT(testing::Mock::VerifyAndClearExpectations(m_view.get()));
  }

  void test_initOK() {
    testing::NiceMock<MockImageROIView> mockView;
    MantidQt::CustomInterfaces::ImageROIPresenter pres(&mockView);

    EXPECT_CALL(mockView, setParams(testing::_)).Times(1);

    EXPECT_CALL(mockView, resetCoR()).Times(0);
    EXPECT_CALL(mockView, resetROI()).Times(0);
    EXPECT_CALL(mockView, resetNormArea()).Times(0);
    EXPECT_CALL(mockView, resetWidgetsOnNewStack()).Times(0);
    EXPECT_CALL(mockView, currentRotationAngle()).Times(0);
    EXPECT_CALL(mockView, updateRotationAngle(testing::_)).Times(0);

    // No errors/warnings
    EXPECT_CALL(mockView, userError(testing::_, testing::_)).Times(0);
    EXPECT_CALL(mockView, userWarning(testing::_, testing::_)).Times(0);

    pres.notify(ImageROIPresenter::Init);
  }

  void test_initWithWrongParams() {
    testing::NiceMock<MockImageROIView> mockView;
    MantidQt::CustomInterfaces::ImageROIPresenter pres(&mockView);

    EXPECT_CALL(mockView, setParams(testing::_)).Times(1);

    EXPECT_CALL(mockView, resetWidgetsOnNewStack()).Times(0);

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

    // because the path is wrong this should not happen
    EXPECT_CALL(mockView, resetWidgetsOnNewStack()).Times(0);

    // should not get there:
    EXPECT_CALL(mockView, showStack(testing::An<const std::string &>()))
        .Times(0);
    EXPECT_CALL(
        mockView,
        showStack(testing::An<const Mantid::API::WorkspaceGroup_sptr &>(),
                  testing::An<const Mantid::API::WorkspaceGroup_sptr &>(),
                  testing::An<const Mantid::API::WorkspaceGroup_sptr &>()))
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

    // because the path is wrong this should not happen
    EXPECT_CALL(mockView, resetWidgetsOnNewStack()).Times(0);

    // should not get there because there's no stack/img - it's just ignored:
    EXPECT_CALL(mockView, showStack(testing::An<const std::string &>()))
        .Times(0);
    EXPECT_CALL(
        mockView,
        showStack(testing::An<const Mantid::API::WorkspaceGroup_sptr &>(),
                  testing::An<const Mantid::API::WorkspaceGroup_sptr &>(),
                  testing::An<const Mantid::API::WorkspaceGroup_sptr &>()))
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

    // because the path is wrong this should not happen
    EXPECT_CALL(mockView, resetWidgetsOnNewStack()).Times(0);

    // should not get there because there's no stack/img
    EXPECT_CALL(mockView, showStack(testing::An<const std::string &>()))
        .Times(0);
    EXPECT_CALL(
        mockView,
        showStack(testing::An<const Mantid::API::WorkspaceGroup_sptr &>(),
                  testing::An<const Mantid::API::WorkspaceGroup_sptr &>(),
                  testing::An<const Mantid::API::WorkspaceGroup_sptr &>()))
        .Times(0);
    EXPECT_CALL(mockView, updateImgWithIndex(testing::_)).Times(0);

    // this exception is currently handled, and a warning given
    // TSM_ASSERT_THROWS("There should be an exception if there is an unexpected
    // "
    //                  "error with the images path",
    //                  pres.notify(IImageROIPresenter::BrowseImgOrStack),
    //                  Poco::FileNotFoundException);
    pres.notify(IImageROIPresenter::BrowseImgOrStack);
  }

  void test_changeImageType() {
    testing::NiceMock<MockImageROIView> mockView;
    MantidQt::CustomInterfaces::ImageROIPresenter pres(&mockView);

    Mantid::API::WorkspaceGroup_sptr stack;
    EXPECT_CALL(mockView, currentImageTypeStack())
        .Times(1)
        .WillOnce(Return(stack));
    EXPECT_CALL(mockView, updateImageType(stack)).Times(1);

    // should not mix up with the img index
    EXPECT_CALL(mockView, currentImgIndex()).Times(0);
    EXPECT_CALL(mockView, updateImgWithIndex(testing::_)).Times(0);

    // Change without issues
    EXPECT_CALL(mockView, userError(testing::_, testing::_)).Times(0);
    EXPECT_CALL(mockView, userWarning(testing::_, testing::_)).Times(0);

    pres.notify(IImageROIPresenter::ChangeImageType);
  }

  void test_changeRotation() {
    testing::NiceMock<MockImageROIView> mockView;
    MantidQt::CustomInterfaces::ImageROIPresenter pres(&mockView);

    EXPECT_CALL(mockView, currentRotationAngle()).Times(1).WillOnce(Return(0));
    EXPECT_CALL(mockView, updateRotationAngle(0.0f)).Times(1);

    // No errors, no warnings
    EXPECT_CALL(mockView, userError(testing::_, testing::_)).Times(0);
    EXPECT_CALL(mockView, userWarning(testing::_, testing::_)).Times(0);

    pres.notify(IImageROIPresenter::ChangeRotation);
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

  // when the user clicks on 'play', with no images
  void test_playStartEmpty() {
    testing::NiceMock<MockImageROIView> mockView;
    MantidQt::CustomInterfaces::ImageROIPresenter pres(&mockView);

    EXPECT_CALL(mockView, currentImgIndex()).Times(0);

    Mantid::API::WorkspaceGroup_sptr emptyStack;
    EXPECT_CALL(mockView, currentImageTypeStack())
        .Times(1)
        .WillOnce(Return(emptyStack));

    EXPECT_CALL(mockView, enableActions(false)).Times(0);
    EXPECT_CALL(mockView, playStart()).Times(0);

    // No errors, no warnings
    EXPECT_CALL(mockView, userError(testing::_, testing::_)).Times(0);
    EXPECT_CALL(mockView, userWarning(testing::_, testing::_)).Times(0);

    pres.notify(IImageROIPresenter::PlayStartStop);
  }

  // try to play a single image => a warning will pop up
  void test_playStartSingleImage() {
    testing::NiceMock<MockImageROIView> mockView;
    MantidQt::CustomInterfaces::ImageROIPresenter pres(&mockView);

    auto stack = boost::make_shared<Mantid::API::WorkspaceGroup>();
    auto img = boost::make_shared<WorkspaceTester>();
    stack->addWorkspace(img);

    EXPECT_CALL(mockView, currentImgIndex()).Times(0);

    EXPECT_CALL(mockView, currentImageTypeStack())
        .Times(1)
        .WillOnce(Return(stack));

    EXPECT_CALL(mockView, enableActions(false)).Times(1);
    EXPECT_CALL(mockView, playStart()).Times(1);

    // No errors, no warnings
    EXPECT_CALL(mockView, userError(testing::_, testing::_)).Times(0);
    EXPECT_CALL(mockView, userWarning(testing::_, testing::_)).Times(1);

    pres.notify(IImageROIPresenter::PlayStartStop);
  }

  // when the user clicks on 'play' with a reasonable stack of images
  void test_playOK() {
    testing::NiceMock<MockImageROIView> mockView;
    MantidQt::CustomInterfaces::ImageROIPresenter pres(&mockView);

    auto stack = boost::make_shared<Mantid::API::WorkspaceGroup>();
    auto img = boost::make_shared<WorkspaceTester>();
    auto img2 = boost::make_shared<WorkspaceTester>();
    stack->addWorkspace(img);
    stack->addWorkspace(img2);

    EXPECT_CALL(mockView, currentImageTypeStack())
        .Times(1)
        .WillOnce(Return(stack));

    EXPECT_CALL(mockView, currentImgIndex()).Times(0);

    EXPECT_CALL(mockView, enableActions(testing::_)).Times(1);
    EXPECT_CALL(mockView, playStart()).Times(1);
    EXPECT_CALL(mockView, playStop()).Times(0);

    // No errors, no warnings
    EXPECT_CALL(mockView, userError(testing::_, testing::_)).Times(0);
    EXPECT_CALL(mockView, userWarning(testing::_, testing::_)).Times(0);

    // start to play
    pres.notify(IImageROIPresenter::PlayStartStop);
  }

  // when the user clicks on 'play', then 'stop', with a reasonable stack of
  // images
  void test_playStartStop() {
    testing::NiceMock<MockImageROIView> mockView;
    MantidQt::CustomInterfaces::ImageROIPresenter pres(&mockView);

    auto stack = boost::make_shared<Mantid::API::WorkspaceGroup>();
    auto img = boost::make_shared<WorkspaceTester>();
    auto img2 = boost::make_shared<WorkspaceTester>();
    stack->addWorkspace(img);
    stack->addWorkspace(img2);

    EXPECT_CALL(mockView, currentImageTypeStack())
        .Times(2)
        .WillRepeatedly(Return(stack));

    EXPECT_CALL(mockView, currentImgIndex()).Times(0);

    EXPECT_CALL(mockView, enableActions(testing::_)).Times(2);
    EXPECT_CALL(mockView, playStart()).Times(1);
    EXPECT_CALL(mockView, playStop()).Times(1);

    // No errors, no warnings
    EXPECT_CALL(mockView, userError(testing::_, testing::_)).Times(0);
    EXPECT_CALL(mockView, userWarning(testing::_, testing::_)).Times(0);

    // start first
    pres.notify(IImageROIPresenter::PlayStartStop);
    // then stop
    pres.notify(IImageROIPresenter::PlayStartStop);
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
    EXPECT_CALL(
        mockView,
        showStack(testing::An<const Mantid::API::WorkspaceGroup_sptr &>(),
                  testing::An<const Mantid::API::WorkspaceGroup_sptr &>(),
                  testing::An<const Mantid::API::WorkspaceGroup_sptr &>()))
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
    EXPECT_CALL(
        mockView,
        showStack(testing::An<const Mantid::API::WorkspaceGroup_sptr &>(),
                  testing::An<const Mantid::API::WorkspaceGroup_sptr &>(),
                  testing::An<const Mantid::API::WorkspaceGroup_sptr &>()))
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
    EXPECT_CALL(
        mockView,
        showStack(testing::An<const Mantid::API::WorkspaceGroup_sptr &>(),
                  testing::An<const Mantid::API::WorkspaceGroup_sptr &>(),
                  testing::An<const Mantid::API::WorkspaceGroup_sptr &>()))
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
