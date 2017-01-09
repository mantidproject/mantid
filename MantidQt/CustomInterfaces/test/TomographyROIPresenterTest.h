#ifndef MANTID_CUSTOMINTERFACES_TOMOGRAPHYROIPRESENTERTEST_H
#define MANTID_CUSTOMINTERFACES_TOMOGRAPHYROIPRESENTERTEST_H

#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidQtCustomInterfaces/Tomography/TomographyROIPresenter.h"
#include "MantidTestHelpers/FakeObjects.h"

#include <cxxtest/TestSuite.h>

#include <Poco/File.h>

#include "TomographyROIViewMock.h"

using namespace MantidQt::CustomInterfaces;
using testing::TypedEq;
using testing::Return;

class TomographyROIPresenterTest : public CxxTest::TestSuite {

public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static TomographyROIPresenterTest *createSuite() {
    return new TomographyROIPresenterTest();
  }

  static void destroySuite(TomographyROIPresenterTest *suite) { delete suite; }

  TomographyROIPresenterTest() {
    Mantid::API::FrameworkManager::Instance(); // make sure the framework is
                                               // initialized
  }

  void setUp() override {
    m_view.reset(new testing::NiceMock<MockTomographyROIView>());
    m_presenter.reset(
        new MantidQt::CustomInterfaces::TomographyROIPresenter(m_view.get()));
  }

  void tearDown() override {
    TS_ASSERT(testing::Mock::VerifyAndClearExpectations(m_view.get()));
  }

  void test_initOK() {
    testing::NiceMock<MockTomographyROIView> mockView;
    MantidQt::CustomInterfaces::TomographyROIPresenter pres(&mockView);

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

    pres.notify(TomographyROIPresenter::Init);

    TSM_ASSERT(
        "Mock not used as expected. Some EXPECT_CALL conditions were not "
        "satisfied.",
        testing::Mock::VerifyAndClearExpectations(&mockView));
  }

  void test_initWithWrongParams() {
    testing::NiceMock<MockTomographyROIView> mockView;
    MantidQt::CustomInterfaces::TomographyROIPresenter pres(&mockView);

    EXPECT_CALL(mockView, setParams(testing::_)).Times(1);

    EXPECT_CALL(mockView, resetWidgetsOnNewStack()).Times(0);

    // One error, no warnings
    EXPECT_CALL(mockView, userError(testing::_, testing::_)).Times(0);
    EXPECT_CALL(mockView, userWarning(testing::_, testing::_)).Times(0);

    pres.notify(TomographyROIPresenter::Init);

    TSM_ASSERT(
        "Mock not used as expected. Some EXPECT_CALL conditions were not "
        "satisfied.",
        testing::Mock::VerifyAndClearExpectations(&mockView));
  }

  void test_browseSingleImg_EmptyPath() {
    testing::NiceMock<MockTomographyROIView> mockView;
    MantidQt::CustomInterfaces::TomographyROIPresenter pres(&mockView);

    EXPECT_CALL(mockView, askImagePath(testing::_)).Times(1);

    // No error, no warning, just ignore
    EXPECT_CALL(mockView, userError(testing::_, testing::_)).Times(0);
    EXPECT_CALL(mockView, userWarning(testing::_, testing::_)).Times(0);

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

    pres.notify(ITomographyROIPresenter::BrowseImage);

    TSM_ASSERT(
        "Mock not used as expected. Some EXPECT_CALL conditions were not "
        "satisfied.",
        testing::Mock::VerifyAndClearExpectations(&mockView));
  }

  void test_browseStack_EmptyPath() {
    testing::NiceMock<MockTomographyROIView> mockView;
    MantidQt::CustomInterfaces::TomographyROIPresenter pres(&mockView);

    EXPECT_CALL(mockView, askImagePath(testing::_)).Times(1).WillOnce(Return(""));

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

    pres.notify(ITomographyROIPresenter::BrowseStack);

    TSM_ASSERT(
        "Mock not used as expected. Some EXPECT_CALL conditions were not "
        "satisfied.",
        testing::Mock::VerifyAndClearExpectations(&mockView));
  }

  void test_browseStack_WrongPath() {
    testing::NiceMock<MockTomographyROIView> mockView;
    MantidQt::CustomInterfaces::TomographyROIPresenter pres(&mockView);

    EXPECT_CALL(mockView, askImagePath(testing::_))
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
    //                  pres.notify(ITomographyROIPresenter::BrowseImgOrStack),
    //                  Poco::FileNotFoundException);
    pres.notify(ITomographyROIPresenter::BrowseStack);

    TSM_ASSERT(
        "Mock not used as expected. Some EXPECT_CALL conditions were not "
        "satisfied.",
        testing::Mock::VerifyAndClearExpectations(&mockView));
  }

  void test_changeImageType() {
    testing::NiceMock<MockTomographyROIView> mockView;
    MantidQt::CustomInterfaces::TomographyROIPresenter pres(&mockView);

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

    pres.notify(ITomographyROIPresenter::ChangeImageType);

    TSM_ASSERT(
        "Mock not used as expected. Some EXPECT_CALL conditions were not "
        "satisfied.",
        testing::Mock::VerifyAndClearExpectations(&mockView));
  }

  void test_changeRotation() {
    testing::NiceMock<MockTomographyROIView> mockView;
    MantidQt::CustomInterfaces::TomographyROIPresenter pres(&mockView);

    EXPECT_CALL(mockView, currentRotationAngle()).Times(1).WillOnce(Return(0));
    EXPECT_CALL(mockView, updateRotationAngle(0.0f)).Times(1);

    // No errors, no warnings
    EXPECT_CALL(mockView, userError(testing::_, testing::_)).Times(0);
    EXPECT_CALL(mockView, userWarning(testing::_, testing::_)).Times(0);

    pres.notify(ITomographyROIPresenter::ChangeRotation);

    TSM_ASSERT(
        "Mock not used as expected. Some EXPECT_CALL conditions were not "
        "satisfied.",
        testing::Mock::VerifyAndClearExpectations(&mockView));
  }

  void test_updateImgIndex() {
    testing::NiceMock<MockTomographyROIView> mockView;
    MantidQt::CustomInterfaces::TomographyROIPresenter pres(&mockView);

    int idx = 0;
    EXPECT_CALL(mockView, currentImgIndex()).Times(1).WillOnce(Return(idx));

    EXPECT_CALL(mockView, updateImgWithIndex(idx)).Times(1);

    // No errors, no warnings
    EXPECT_CALL(mockView, userError(testing::_, testing::_)).Times(0);
    EXPECT_CALL(mockView, userWarning(testing::_, testing::_)).Times(0);

    pres.notify(ITomographyROIPresenter::UpdateImgIndex);

    TSM_ASSERT(
        "Mock not used as expected. Some EXPECT_CALL conditions were not "
        "satisfied.",
        testing::Mock::VerifyAndClearExpectations(&mockView));
  }

  // when the user clicks on 'play', with no images
  void test_playStartEmpty() {
    testing::NiceMock<MockTomographyROIView> mockView;
    MantidQt::CustomInterfaces::TomographyROIPresenter pres(&mockView);

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

    pres.notify(ITomographyROIPresenter::PlayStartStop);

    TSM_ASSERT(
        "Mock not used as expected. Some EXPECT_CALL conditions were not "
        "satisfied.",
        testing::Mock::VerifyAndClearExpectations(&mockView));
  }

  // try to play a single image => a warning will pop up
  void test_playStartSingleImage() {
    testing::NiceMock<MockTomographyROIView> mockView;
    MantidQt::CustomInterfaces::TomographyROIPresenter pres(&mockView);

    auto stack = boost::make_shared<Mantid::API::WorkspaceGroup>();
    auto img = boost::make_shared<WorkspaceTester>();
    stack->addWorkspace(img);

    EXPECT_CALL(mockView, currentImgIndex()).Times(0);

    EXPECT_CALL(mockView, currentImageTypeStack())
        .Times(1)
        .WillOnce(Return(stack));

    // for a single image, there should be a warning message, and we
    // should not even try to play
    EXPECT_CALL(mockView, enableActions(false)).Times(0);
    EXPECT_CALL(mockView, playStart()).Times(0);

    // No errors, no warnings
    EXPECT_CALL(mockView, userError(testing::_, testing::_)).Times(0);
    EXPECT_CALL(mockView, userWarning(testing::_, testing::_)).Times(1);

    pres.notify(ITomographyROIPresenter::PlayStartStop);

    TSM_ASSERT(
        "Mock not used as expected. Some EXPECT_CALL conditions were not "
        "satisfied.",
        testing::Mock::VerifyAndClearExpectations(&mockView));
  }

  // when the user clicks on 'play' with a reasonable stack of images
  void test_playOK() {
    testing::NiceMock<MockTomographyROIView> mockView;
    MantidQt::CustomInterfaces::TomographyROIPresenter pres(&mockView);

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
    pres.notify(ITomographyROIPresenter::PlayStartStop);

    TSM_ASSERT(
        "Mock not used as expected. Some EXPECT_CALL conditions were not "
        "satisfied.",
        testing::Mock::VerifyAndClearExpectations(&mockView));
  }

  // when the user clicks on 'play', then 'stop', with a reasonable stack of
  // images
  void test_playStartStop() {
    testing::NiceMock<MockTomographyROIView> mockView;
    MantidQt::CustomInterfaces::TomographyROIPresenter pres(&mockView);

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
    pres.notify(ITomographyROIPresenter::PlayStartStop);
    // then stop
    pres.notify(ITomographyROIPresenter::PlayStartStop);

    TSM_ASSERT(
        "Mock not used as expected. Some EXPECT_CALL conditions were not "
        "satisfied.",
        testing::Mock::VerifyAndClearExpectations(&mockView));
  }

  void test_updateColorMapEmpty() {
    testing::NiceMock<MockTomographyROIView> mockView;
    MantidQt::CustomInterfaces::TomographyROIPresenter pres(&mockView);

    EXPECT_CALL(mockView, askColorMapFile()).Times(1).WillOnce(Return(""));

    // Should not get there
    EXPECT_CALL(mockView, updateColorMap(testing::_)).Times(0);

    // No errors, no warnings
    EXPECT_CALL(mockView, userError(testing::_, testing::_)).Times(0);
    EXPECT_CALL(mockView, userWarning(testing::_, testing::_)).Times(0);

    pres.notify(ITomographyROIPresenter::UpdateColorMap);

    TSM_ASSERT(
        "Mock not used as expected. Some EXPECT_CALL conditions were not "
        "satisfied.",
        testing::Mock::VerifyAndClearExpectations(&mockView));
  }

  void test_updateColorMapOK() {
    testing::NiceMock<MockTomographyROIView> mockView;
    MantidQt::CustomInterfaces::TomographyROIPresenter pres(&mockView);

    const std::string filename = "test_inexistent_colormap.map";
    EXPECT_CALL(mockView, askColorMapFile())
        .Times(1)
        .WillOnce(Return(filename));

    EXPECT_CALL(mockView, updateColorMap(filename)).Times(1);

    // No errors, no warnings
    EXPECT_CALL(mockView, userError(testing::_, testing::_)).Times(0);
    EXPECT_CALL(mockView, userWarning(testing::_, testing::_)).Times(0);

    pres.notify(ITomographyROIPresenter::UpdateColorMap);

    TSM_ASSERT(
        "Mock not used as expected. Some EXPECT_CALL conditions were not "
        "satisfied.",
        testing::Mock::VerifyAndClearExpectations(&mockView));
  }

  void test_changeColorRange() {
    testing::NiceMock<MockTomographyROIView> mockView;
    MantidQt::CustomInterfaces::TomographyROIPresenter pres(&mockView);

    EXPECT_CALL(mockView, askColorMapFile()).Times(0);

    size_t img_idx = 0;
    EXPECT_CALL(mockView, currentImgIndex()).Times(1).WillOnce(Return(img_idx));
    EXPECT_CALL(mockView, updateImgWithIndex(img_idx)).Times(1);

    // No errors, no warnings
    EXPECT_CALL(mockView, userError(testing::_, testing::_)).Times(0);
    EXPECT_CALL(mockView, userWarning(testing::_, testing::_)).Times(0);

    pres.notify(ITomographyROIPresenter::ColorRangeUpdated);

    TSM_ASSERT(
        "Mock not used as expected. Some EXPECT_CALL conditions were not "
        "satisfied.",
        testing::Mock::VerifyAndClearExpectations(&mockView));
  }

  void test_selectCoR() {
    testing::NiceMock<MockTomographyROIView> mockView;
    MantidQt::CustomInterfaces::TomographyROIPresenter pres(&mockView);

    EXPECT_CALL(mockView, changeSelectionState(ITomographyROIView::SelectCoR))
        .Times(1);

    // No errors, no warnings
    EXPECT_CALL(mockView, userError(testing::_, testing::_)).Times(0);
    EXPECT_CALL(mockView, userWarning(testing::_, testing::_)).Times(0);

    pres.notify(ITomographyROIPresenter::SelectCoR);

    TSM_ASSERT(
        "Mock not used as expected. Some EXPECT_CALL conditions were not "
        "satisfied.",
        testing::Mock::VerifyAndClearExpectations(&mockView));
  }

  void test_resetCoR() {
    testing::NiceMock<MockTomographyROIView> mockView;
    MantidQt::CustomInterfaces::TomographyROIPresenter pres(&mockView);

    EXPECT_CALL(mockView, resetCoR()).Times(1);
    EXPECT_CALL(mockView, changeSelectionState(ITomographyROIView::SelectNone))
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

    pres.notify(ITomographyROIPresenter::ResetCoR);

    TSM_ASSERT(
        "Mock not used as expected. Some EXPECT_CALL conditions were not "
        "satisfied.",
        testing::Mock::VerifyAndClearExpectations(&mockView));
  }

  void test_selectROI() {
    testing::NiceMock<MockTomographyROIView> mockView;
    MantidQt::CustomInterfaces::TomographyROIPresenter pres(&mockView);

    EXPECT_CALL(mockView, changeSelectionState(
                              ITomographyROIView::SelectROIFirst)).Times(1);

    // No errors, no warnings
    EXPECT_CALL(mockView, userError(testing::_, testing::_)).Times(0);
    EXPECT_CALL(mockView, userWarning(testing::_, testing::_)).Times(0);

    pres.notify(ITomographyROIPresenter::SelectROI);

    TSM_ASSERT(
        "Mock not used as expected. Some EXPECT_CALL conditions were not "
        "satisfied.",
        testing::Mock::VerifyAndClearExpectations(&mockView));
  }

  void test_finishROI() {
    testing::NiceMock<MockTomographyROIView> mockView;
    MantidQt::CustomInterfaces::TomographyROIPresenter pres(&mockView);

    EXPECT_CALL(mockView, changeSelectionState(ITomographyROIView::SelectNone))
        .Times(1);

    // No errors, no warnings
    EXPECT_CALL(mockView, userError(testing::_, testing::_)).Times(0);
    EXPECT_CALL(mockView, userWarning(testing::_, testing::_)).Times(0);

    pres.notify(ITomographyROIPresenter::FinishedROI);

    TSM_ASSERT(
        "Mock not used as expected. Some EXPECT_CALL conditions were not "
        "satisfied.",
        testing::Mock::VerifyAndClearExpectations(&mockView));
  }

  void test_resetROI() {
    testing::NiceMock<MockTomographyROIView> mockView;
    MantidQt::CustomInterfaces::TomographyROIPresenter pres(&mockView);

    EXPECT_CALL(mockView, resetROI()).Times(1);
    EXPECT_CALL(mockView, changeSelectionState(ITomographyROIView::SelectNone))
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

    pres.notify(ITomographyROIPresenter::ResetROI);

    TSM_ASSERT(
        "Mock not used as expected. Some EXPECT_CALL conditions were not "
        "satisfied.",
        testing::Mock::VerifyAndClearExpectations(&mockView));
  }

  void test_selectNormalization() {
    testing::NiceMock<MockTomographyROIView> mockView;
    MantidQt::CustomInterfaces::TomographyROIPresenter pres(&mockView);

    EXPECT_CALL(mockView,
                changeSelectionState(ITomographyROIView::SelectNormAreaFirst))
        .Times(1);

    // No errors, no warnings
    EXPECT_CALL(mockView, userError(testing::_, testing::_)).Times(0);
    EXPECT_CALL(mockView, userWarning(testing::_, testing::_)).Times(0);

    pres.notify(ITomographyROIPresenter::SelectNormalization);

    TSM_ASSERT(
        "Mock not used as expected. Some EXPECT_CALL conditions were not "
        "satisfied.",
        testing::Mock::VerifyAndClearExpectations(&mockView));
  }

  void test_finishNormalization() {
    testing::NiceMock<MockTomographyROIView> mockView;
    MantidQt::CustomInterfaces::TomographyROIPresenter pres(&mockView);

    EXPECT_CALL(mockView, changeSelectionState(ITomographyROIView::SelectNone))
        .Times(1);

    // No errors, no warnings
    EXPECT_CALL(mockView, userError(testing::_, testing::_)).Times(0);
    EXPECT_CALL(mockView, userWarning(testing::_, testing::_)).Times(0);

    pres.notify(ITomographyROIPresenter::FinishedNormalization);

    TSM_ASSERT(
        "Mock not used as expected. Some EXPECT_CALL conditions were not "
        "satisfied.",
        testing::Mock::VerifyAndClearExpectations(&mockView));
  }

  void test_resetNormalization() {
    testing::NiceMock<MockTomographyROIView> mockView;
    MantidQt::CustomInterfaces::TomographyROIPresenter pres(&mockView);

    EXPECT_CALL(mockView, resetNormArea()).Times(1);
    EXPECT_CALL(mockView, changeSelectionState(ITomographyROIView::SelectNone))
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

    pres.notify(ITomographyROIPresenter::ResetNormalization);

    TSM_ASSERT(
        "Mock not used as expected. Some EXPECT_CALL conditions were not "
        "satisfied.",
        testing::Mock::VerifyAndClearExpectations(&mockView));
  }

  void test_shutDown() {
    testing::NiceMock<MockTomographyROIView> mockView;
    MantidQt::CustomInterfaces::TomographyROIPresenter pres(&mockView);

    EXPECT_CALL(mockView, saveSettings()).Times(1);
    // No errors, no warnings
    EXPECT_CALL(mockView, userError(testing::_, testing::_)).Times(0);
    EXPECT_CALL(mockView, userWarning(testing::_, testing::_)).Times(0);

    pres.notify(TomographyROIPresenter::ShutDown);

    TSM_ASSERT(
        "Mock not used as expected. Some EXPECT_CALL conditions were not "
        "satisfied.",
        testing::Mock::VerifyAndClearExpectations(&mockView));
  }

private:
  // boost::shared_ptr
  boost::scoped_ptr<testing::NiceMock<MockTomographyROIView>> m_view;
  boost::scoped_ptr<MantidQt::CustomInterfaces::TomographyROIPresenter>
      m_presenter;

  // To have one FITS, etc.
  Mantid::API::MatrixWorkspace_sptr m_ws;
};

#endif // MANTID_CUSTOMINTERFACES_TOMOGRAPHYROIPRESENTERTEST_H
