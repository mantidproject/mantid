// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidQtWidgets/Common/MessageHandler.h"
#include "MantidQtWidgets/InstrumentView/IGLDisplay.h"
#include "MantidQtWidgets/InstrumentView/IQtDisplay.h"
#include "MantidQtWidgets/InstrumentView/InstrumentWidget.h"
#include "MantidQtWidgets/InstrumentView/InstrumentWidgetMaskTab.h"

#include "MockGLDisplay.h"
#include "MockInstrumentActor.h"
#include "MockInstrumentDisplay.h"
#include "MockInstrumentWidgetMaskTab.h"
#include "MockMessageHandler.h"
#include "MockProjectionSurface.h"
#include "MockQtConnect.h"
#include "MockQtDisplay.h"
#include "MockQtMetaObject.h"

#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/Workspace.h"
#include "MantidDataObjects/Peak.h"
#include "MantidFrameworkTestHelpers/WorkspaceCreationHelper.h"
#include "MantidKernel/V3D.h"

#include <QObject>

#include <cxxtest/TestSuite.h>
#include <gmock/gmock.h>
#include <memory>

using namespace MantidQt::MantidWidgets;
using namespace testing;
using Mantid::API::AnalysisDataService;
using Mantid::API::FrameworkManager;

class InstrumentWidgetTest : public CxxTest::TestSuite {
public:
  static InstrumentWidgetTest *createSuite() { return new InstrumentWidgetTest(); }
  static void destroySuite(InstrumentWidgetTest *suite) { delete suite; }

  using QtMock = StrictMock<MockQtDisplay>;
  using GLMock = StrictMock<MockGLDisplay>;
  using ConnectMock = StrictMock<MockQtConnect>;
  using MetaObjectMock = StrictMock<MockQtMetaObject>;
  using DisplayMock = NiceMock<MockInstrumentDisplay>;
  using MessageMock = NiceMock<MockMessageHandler>;

  void setUp() override {
    FrameworkManager::Instance();
    auto ws = WorkspaceCreationHelper::create2DWorkspaceWithFullInstrument(2, 2);
    AnalysisDataService::Instance().addOrReplace("test_ws", ws);

    m_glEnabledOriginal = Mantid::Kernel::ConfigService::Instance()
                              .getValue<bool>("MantidOptions.InstrumentView.UseOpenGL")
                              .get_value_or(true);
    setGl(true);
  }

  void tearDown() override {
    AnalysisDataService::Instance().clear();
    setGl(m_glEnabledOriginal);
  }

  void test_constructor() {
    for (const bool useLoadingThread : {true, false}) {
      auto qtMock = makeQtDisplay();
      auto glMock = makeGL();
      auto displayMock = makeDisplay();
      EXPECT_CALL(*displayMock, currentWidget()).Times(1).WillOnce(Return(glMock.get()));

      auto instance = constructWithProjectionSurface("test_ws", std::move(displayMock), qtMock.get(), glMock.get(), 22,
                                                     useLoadingThread);

      if (useLoadingThread) {
        InstrumentActor &actor = instance.getInstrumentActor();
        actor.initialize(true, true);
        instance.initWidget(true, true);
        instance.waitForThread();
      }
    }
  }

  void test_constructor_gl_disabled() {
    for (const bool useLoadingThread : {true, false}) {
      setGl(false);
      auto qtMock = makeQtDisplay();
      auto glMock = makeGL();
      auto displayMock = makeDisplay();
      EXPECT_CALL(*displayMock, currentWidget()).Times(1).WillOnce(Return(qtMock.get()));
      auto instance = constructWithProjectionSurface("test_ws", std::move(displayMock), qtMock.get(), glMock.get(), 24,
                                                     useLoadingThread);

      if (useLoadingThread) {
        InstrumentActor &actor = instance.getInstrumentActor();
        actor.initialize(true, true);
        instance.initWidget(true, true);
        instance.waitForThread();
      }
    }
  }

  void test_save_image_gl_enabled() {
    for (const bool useLoadingThread : {true, false}) {
      const auto inputName = QString::fromStdString("testFilename");
      const auto expectedName = inputName + ".png";

      auto qtMock = makeQtDisplay();
      auto glMock = makeGL();
      EXPECT_CALL(*glMock, saveToFile(expectedName)).Times(1);
      auto displayMock = makeDisplay();
      EXPECT_CALL(*displayMock, currentWidget()).Times(1).WillOnce(Return(glMock.get()));

      auto widget = constructWithProjectionSurface("test_ws", std::move(displayMock), qtMock.get(), glMock.get(), 22,
                                                   useLoadingThread);

      if (useLoadingThread) {
        InstrumentActor &actor = widget.getInstrumentActor();
        actor.initialize(true, true);
        widget.initWidget(true, true);
        widget.waitForThread();
      }
      widget.saveImage(inputName);
    }
  }

  void test_save_image_gl_disabled() {
    for (const bool useLoadingThread : {true, false}) {
      setGl(false);
      const auto inputName = QString::fromStdString("testFilename");
      const auto expectedName = inputName + ".png";

      auto qtMock = makeQtDisplay();
      auto glMock = makeGL();
      EXPECT_CALL(*qtMock, saveToFile(expectedName)).Times(1);
      auto displayMock = makeDisplay();
      EXPECT_CALL(*displayMock, currentWidget()).Times(1).WillOnce(Return(qtMock.get()));

      auto widget = constructWithProjectionSurface("test_ws", std::move(displayMock), qtMock.get(), glMock.get(), 24,
                                                   useLoadingThread);

      if (useLoadingThread) {
        InstrumentActor &actor = widget.getInstrumentActor();
        actor.initialize(true, true);
        widget.initWidget(true, true);
        widget.waitForThread();
      }
      widget.saveImage(inputName);
    }
  }

  void test_update_instrument_detectors_gl_display_selected() {
    for (const bool useLoadingThread : {true, false}) {
      auto qtMock = makeQtDisplay();
      auto glMock = makeGL();
      auto displayMock = makeDisplay();
      EXPECT_CALL(*glMock, updateDetectors()).Times(1);
      EXPECT_CALL(*displayMock, currentWidget()).Times(2).WillRepeatedly(Return(glMock.get()));

      auto widget = constructWithProjectionSurface("test_ws", std::move(displayMock), qtMock.get(), glMock.get(), 22,
                                                   useLoadingThread);

      if (useLoadingThread) {
        InstrumentActor &actor = widget.getInstrumentActor();
        actor.initialize(true, true);
        widget.initWidget(true, true);
        widget.waitForThread();
      }

      widget.updateInstrumentDetectors();
    }
  }

  void test_update_instrument_detectors_qt_display_selected() {
    for (const bool useLoadingThread : {true, false}) {
      auto qtMock = makeQtDisplay();
      auto glMock = makeGL();
      auto displayMock = makeDisplay();
      EXPECT_CALL(*qtMock, updateDetectors()).Times(1);
      EXPECT_CALL(*displayMock, currentWidget()).Times(2).WillRepeatedly(Return(qtMock.get()));
      auto widget = constructWithProjectionSurface("test_ws", std::move(displayMock), qtMock.get(), glMock.get(), 22,
                                                   useLoadingThread);

      if (useLoadingThread) {
        InstrumentActor &actor = widget.getInstrumentActor();
        actor.initialize(true, true);
        widget.initWidget(true, true);
        widget.waitForThread();
      }

      widget.updateInstrumentDetectors();
    }
  }

  void test_update_instrument_detectors_gl_disabled() {
    for (const bool useLoadingThread : {true, false}) {
      // When GL is disabled, but somehow we still have a GL Display, we expect an update on that
      // This is likely a bug, but we are ensuring bug-compatibility
      setGl(false);
      auto qtMock = makeQtDisplay();
      auto glMock = makeGL();
      auto displayMock = makeDisplay();
      EXPECT_CALL(*glMock, updateDetectors()).Times(1);
      EXPECT_CALL(*displayMock, currentWidget()).Times(2).WillRepeatedly(Return(glMock.get()));

      auto widget = constructWithProjectionSurface("test_ws", std::move(displayMock), qtMock.get(), glMock.get(), 24,
                                                   useLoadingThread);

      if (useLoadingThread) {
        InstrumentActor &actor = widget.getInstrumentActor();
        actor.initialize(true, true);
        widget.initWidget(true, true);
        widget.waitForThread();
      }

      widget.updateInstrumentDetectors();
    }
  }

  void test_update_instrument_detectors_gl_disabled_qt_display_selected() {
    for (const bool useLoadingThread : {true, false}) {
      setGl(false);
      auto qtMock = makeQtDisplay();
      auto glMock = makeGL();
      auto displayMock = makeDisplay();
      EXPECT_CALL(*qtMock, updateDetectors()).Times(1);
      EXPECT_CALL(*displayMock, currentWidget()).Times(2).WillRepeatedly(Return(qtMock.get()));

      auto widget = constructWithProjectionSurface("test_ws", std::move(displayMock), qtMock.get(), glMock.get(), 24,
                                                   useLoadingThread);

      if (useLoadingThread) {
        InstrumentActor &actor = widget.getInstrumentActor();
        actor.initialize(true, true);
        widget.initWidget(true, true);
        widget.waitForThread();
      }

      widget.updateInstrumentDetectors();
    }
  }

  void test_update_instrument_view() {
    for (const bool useLoadingThread : {true, false}) {
      for (bool expected : {true, false}) {
        auto qtMock = makeQtDisplay();
        auto glMock = makeGL();
        auto displayMock = makeDisplay();
        EXPECT_CALL(*displayMock, currentWidget()).Times(1).WillOnce(Return(glMock.get()));
        EXPECT_CALL(*displayMock, updateView(expected)).Times(1);
        auto widget = constructWithProjectionSurface("test_ws", std::move(displayMock), qtMock.get(), glMock.get(), 22,
                                                     useLoadingThread);

        if (useLoadingThread) {
          InstrumentActor &actor = widget.getInstrumentActor();
          actor.initialize(true, true);
          widget.initWidget(true, true);
          widget.waitForThread();
        }

        widget.updateInstrumentView(expected);
      }
    }
  }

  void draw_tab_save_actions(const QString &wsname, const int nCallsShowMessageBox) {
    for (const bool useLoadingThread : {true, false}) {
      auto qtMock = makeQtDisplay();
      auto glMock = makeGL();
      auto displayMock = makeDisplay();
      EXPECT_CALL(*displayMock, currentWidget()).Times(3).WillRepeatedly(Return(qtMock.get()));

      auto widget = constructWithProjectionSurface(wsname, std::move(displayMock), qtMock.get(), glMock.get(), 46,
                                                   useLoadingThread);

      if (useLoadingThread) {
        InstrumentActor &actor = widget.getInstrumentActor();
        actor.initialize(true, true);
        widget.initWidget(true, true);
        widget.waitForThread();
      }

      MockInstrumentWidgetMaskTab drawTab(&widget);
      EXPECT_CALL(drawTab, showMessageBox(_)).Times(nCallsShowMessageBox);
      // these actions may show a message box on error
      QMetaObject::invokeMethod(&drawTab, "sumDetsToWorkspace", Qt::DirectConnection);
      QMetaObject::invokeMethod(&drawTab, "extractDetsToWorkspace", Qt::DirectConnection);
      // these actions should always work
      EXPECT_CALL(*qtMock, updateDetectors()).Times(2);
      QMetaObject::invokeMethod(&drawTab, "saveMaskToWorkspace", Qt::DirectConnection);
      QMetaObject::invokeMethod(&drawTab, "saveInvertedMaskToWorkspace", Qt::DirectConnection);
    }
  }

  void test_draw_tab_save_actions() {
    // sum\extract detectors only work if workspace spectra have common bins
    draw_tab_save_actions("test_ws", 0);
    // create a second workspace that doesn't have common bins
    auto ws_d = WorkspaceCreationHelper::create2DWorkspaceWithFullInstrument(2, 2);
    AnalysisDataService::Instance().addOrReplace("test_ws_d", ws_d);
    auto alg = Mantid::API::AlgorithmManager::Instance().create("ConvertUnits");
    alg->setRethrows(true);
    alg->setProperty("InputWorkspace", "test_ws_d");
    alg->setProperty("OutputWorkspace", "test_ws_d");
    alg->setProperty("Target", "dSpacing");
    alg->execute();
    draw_tab_save_actions("test_ws_d", 2);
  }

  void test_peak_with_no_detector() {
    for (const bool useLoadingThread : {true, false}) {
      auto qtMock = makeQtDisplay();
      auto glMock = makeGL();
      auto displayMock = makeDisplay();
      EXPECT_CALL(*displayMock, currentWidget()).WillRepeatedly(Return(qtMock.get()));

      auto widget = constructWithUnwrappedSurface("test_ws", std::move(displayMock), qtMock.get(), glMock.get(),
                                                  useLoadingThread);
      widget.setViewType("CYLINDRICAL_X");

      auto createPeaksWs = Mantid::API::AlgorithmManager::Instance().create("CreatePeaksWorkspace");
      createPeaksWs->setRethrows(true);
      createPeaksWs->setProperty("InstrumentWorkspace", "test_ws");
      createPeaksWs->setProperty("OutputWorkspace", "peaks");
      createPeaksWs->execute();
      auto ws = AnalysisDataService::Instance().retrieve("peaks");
      auto peaksWs = std::dynamic_pointer_cast<IPeaksWorkspace>(ws);
      auto peak = Mantid::DataObjects::Peak(peaksWs->getInstrument(), Mantid::Kernel::V3D(1, 1, 1));
      peaksWs->addPeak(peak);
      TS_ASSERT_THROWS_NOTHING(widget.overlay("peaks"));
    }
  }

private:
  bool m_glEnabledOriginal = true;
  bool m_glEnabled = true;

  std::unique_ptr<QtMock> makeQtDisplay() const { return std::make_unique<QtMock>(); }
  std::unique_ptr<GLMock> makeGL() const { return std::make_unique<GLMock>(); }
  std::unique_ptr<DisplayMock> makeDisplay() const { return std::make_unique<DisplayMock>(); }
  std::unique_ptr<MessageMock> makeMessage() const { return std::make_unique<MessageMock>(); }

  void setGl(bool state) {
    m_glEnabled = state;
    auto const stateStr = state ? "On" : "Off";
    Mantid::Kernel::ConfigService::Instance().setString("MantidOptions.InstrumentView.UseOpenGL", stateStr);
  }

  void mockConnect(MockQtConnect &mock, const char *signal, const char *slot, const bool checkNumberOfCalls) const {
    if (checkNumberOfCalls) {
      EXPECT_CALL(mock, connect(_, StrEq(signal), _, StrEq(slot))).Times(1);
    } else {
      EXPECT_CALL(mock, connect(_, StrEq(signal), _, StrEq(slot))).Times(testing::AtLeast(1));
    }
  }

  std::unique_ptr<ConnectMock> makeConnect(const bool useLoadingThread, const bool checkNumberOfCalls = true) const {
    auto mock = std::make_unique<ConnectMock>();
    mockConnect(*mock, SIGNAL(enableLighting(bool)), SLOT(enableLighting(bool)), checkNumberOfCalls);

    mockConnect(*mock, SIGNAL(changed(double, double)), SLOT(setIntegrationRange(double, double)), checkNumberOfCalls);
    mockConnect(*mock, SIGNAL(clicked()), SLOT(helpClicked()), checkNumberOfCalls);
    mockConnect(*mock, SIGNAL(setAutoscaling(bool)), SLOT(setColorMapAutoscaling(bool)), checkNumberOfCalls);
    mockConnect(*mock, SIGNAL(rescaleColorMap()), SLOT(setupColorMap()), checkNumberOfCalls);
    mockConnect(*mock, SIGNAL(executeAlgorithm(const QString &, const QString &)),
                SLOT(executeAlgorithm(const QString &, const QString &)), checkNumberOfCalls);
    mockConnect(*mock, SIGNAL(changed(double, double)), SLOT(changedIntegrationRange(double, double)),
                checkNumberOfCalls);
    mockConnect(*mock, SIGNAL(currentChanged(int)), SLOT(tabChanged(int)), checkNumberOfCalls);
    mockConnect(*mock, SIGNAL(triggered()), SLOT(clearPeakOverlays()), checkNumberOfCalls);
    mockConnect(*mock, SIGNAL(triggered()), SLOT(clearAlignmentPlane()), checkNumberOfCalls);

    EXPECT_CALL(*mock, connect(_, StrEq(SIGNAL(needSetIntegrationRange(double, double))), _,
                               StrEq(SLOT(setIntegrationRange(double, double))), Qt::QueuedConnection))
        .Times(1);
    mockConnect(*mock, SIGNAL(executeAlgorithm(Mantid::API::IAlgorithm_sptr)),
                SLOT(executeAlgorithm(Mantid::API::IAlgorithm_sptr)), checkNumberOfCalls);

    if (useLoadingThread) {
      mockConnect(*mock, SIGNAL(initWidget(bool, bool)), SLOT(initWidget(bool, bool)), checkNumberOfCalls);
      if (checkNumberOfCalls) {
        EXPECT_CALL(*mock, connect(_, StrEq(SIGNAL(destroyed())), _, StrEq(SLOT(threadFinished())))).Times(2);
      } else {
        EXPECT_CALL(*mock, connect(_, StrEq(SIGNAL(destroyed())), _, StrEq(SLOT(threadFinished()))))
            .Times(testing::AtLeast(1));
      }
    }

    if (checkNumberOfCalls) {

      EXPECT_CALL(*mock,
                  connect(_, StrEq(SIGNAL(updateInfoText())), _, StrEq(SLOT(updateInfoText())), Qt::QueuedConnection))
          .Times(1);
    } else {
      EXPECT_CALL(*mock,
                  connect(_, StrEq(SIGNAL(updateInfoText())), _, StrEq(SLOT(updateInfoText())), Qt::QueuedConnection))
          .Times(testing::AtLeast(1));
    }
    return mock;
  }

  std::unique_ptr<MetaObjectMock> makeMetaObject(const bool useLoadingThread) const {
    auto mock = std::make_unique<MetaObjectMock>();
    if (useLoadingThread) {
      EXPECT_CALL(*mock, invokeMethod(_, StrEq("initialize"), Qt::QueuedConnection, _, _, _, _, _, _, _, _, _, _))
          .Times(1);
      EXPECT_CALL(*mock, invokeMethod(_, StrEq("cancel"), Qt::DirectConnection, _, _, _, _, _, _, _, _, _, _)).Times(1);
    } else {
      EXPECT_CALL(*mock, invokeMethod(_, StrEq("initialize"), Qt::DirectConnection, _, _, _, _, _, _, _, _, _, _))
          .Times(1);
    }
    return mock;
  }

  InstrumentWidget constructWithProjectionSurface(QString wsname, std::unique_ptr<DisplayMock> displayMock,
                                                  QtMock *qtMock, GLMock *glMock, const int getSurfaceCalls,
                                                  const bool useLoadingThread) const {

    auto metaObjectMock = makeMetaObject(useLoadingThread);
    auto connectMock = makeConnect(useLoadingThread);

    ON_CALL(*displayMock, getGLDisplay()).WillByDefault(Return(glMock));
    ON_CALL(*displayMock, getQtDisplay()).WillByDefault(Return(qtMock));

    auto surfaceMock = std::make_shared<MockProjectionSurface>();
    EXPECT_CALL(*glMock, currentBackgroundColor()).Times(1);

    EXPECT_CALL(*displayMock, getSurface()).Times(getSurfaceCalls).WillRepeatedly(Return(surfaceMock));
    EXPECT_CALL(*displayMock, setSurfaceProxy(_)).Times(1);
    EXPECT_CALL(*displayMock, installEventFilter(NotNull())).Times(1);

    auto detIDs = std::vector<size_t>{0, 1};
    EXPECT_CALL(*surfaceMock, getMaskedDetectors(_)).WillRepeatedly(SetArgReferee<0>(detIDs));
    EXPECT_CALL(*surfaceMock, setInteractionMode(_)).Times(testing::AtLeast(1));

    InstrumentWidget::Dependencies deps{std::move(displayMock),    nullptr,      nullptr, std::move(connectMock),
                                        std::move(metaObjectMock), makeMessage()};

    return InstrumentWidget(wsname, nullptr, true, true, 0.0, 0.0, true, std::move(deps), useLoadingThread);
  }

  InstrumentWidget constructWithUnwrappedSurface(QString wsname, std::unique_ptr<DisplayMock> displayMock,
                                                 QtMock *qtMock, GLMock *glMock, const bool useLoadingThread) const {

    auto metaObjectMock = makeMetaObject(useLoadingThread);
    auto connectMock = makeConnect(useLoadingThread, false);

    ON_CALL(*displayMock, getGLDisplay()).WillByDefault(Return(glMock));
    ON_CALL(*displayMock, getQtDisplay()).WillByDefault(Return(qtMock));

    auto messageHandler = MantidQt::MantidWidgets::MessageHandler();
    auto instrumentActor = new InstrumentActor(wsname.toStdString(), messageHandler);
    auto surfaceMock = std::make_shared<MockUnwrappedSphere>(instrumentActor);
    EXPECT_CALL(*glMock, currentBackgroundColor()).Times(1);

    EXPECT_CALL(*displayMock, getSurface()).Times(testing::AtLeast(1)).WillRepeatedly(Return(surfaceMock));
    EXPECT_CALL(*displayMock, setSurfaceProxy(_)).Times(testing::AtLeast(1));
    EXPECT_CALL(*displayMock, installEventFilter(NotNull())).Times(1);

    auto detIDs = std::vector<size_t>{0, 1};
    EXPECT_CALL(*surfaceMock, getMaskedDetectors(_)).WillRepeatedly(SetArgReferee<0>(detIDs));
    EXPECT_CALL(*surfaceMock, setInteractionMode(_)).Times(testing::AtLeast(1));

    InstrumentWidget::Dependencies deps{std::move(displayMock),    nullptr,      nullptr, std::move(connectMock),
                                        std::move(metaObjectMock), makeMessage()};

    return InstrumentWidget(wsname, nullptr, true, true, 0.0, 0.0, true, std::move(deps), useLoadingThread);
  }
};
