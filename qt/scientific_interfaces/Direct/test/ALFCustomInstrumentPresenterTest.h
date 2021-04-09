// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <cxxtest/TestSuite.h>
#include <gmock/gmock.h>

#include "ALFCustomInstrumentMocks.h"
#include "ALFCustomInstrumentModel.h"
#include "ALFCustomInstrumentPresenter.h"
#include "ALFCustomInstrumentView.h"
#include "MantidQtWidgets/Common/ObserverPattern.h"
#include "MantidQtWidgets/InstrumentView/PlotFitAnalysisPaneMocks.h"
#include "MantidQtWidgets/InstrumentView/PlotFitAnalysisPaneModel.h"
#include "MantidQtWidgets/InstrumentView/PlotFitAnalysisPanePresenter.h"
#include "MantidQtWidgets/InstrumentView/PlotFitAnalysisPaneView.h"

#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/CompositeFunction.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/FunctionFactory.h"
#include "MantidAPI/MatrixWorkspace_fwd.h"
#include "MantidGeometry/Instrument.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"

#include <iostream>
#include <string>
#include <utility>
using namespace Mantid::API;
using namespace MantidQt::CustomInterfaces;
using namespace testing;
using Mantid::Geometry::Instrument;
using namespace MantidQt::MantidWidgets;

class ALFCustomInstrumentPresenterTest : public CxxTest::TestSuite {
public:
  ALFCustomInstrumentPresenterTest() { FrameworkManager::Instance(); }

  static ALFCustomInstrumentPresenterTest *createSuite() { return new ALFCustomInstrumentPresenterTest(); }

  static void destroySuite(ALFCustomInstrumentPresenterTest *suite) { delete suite; }

  void setUp() override {
    m_model = new NiceMock<MockALFCustomInstrumentModel>();
    m_view = new NiceMock<MockALFCustomInstrumentView>("ALF");
    m_paneView = new NiceMock<MockPlotFitAnalysisPaneView>();
    m_paneModel = new NiceMock<MockPlotFitAnalysisPaneModel>();
    m_pane = new NiceMock<MockPlotFitAnalysisPanePresenter>(m_paneView, m_paneModel);
    m_presenter = new ALFCustomInstrumentPresenter(m_view, m_model, m_pane);
  }

  void tearDown() override {
    AnalysisDataService::Instance().clear();
    delete m_model;
    delete m_view;
    delete m_paneView;
    delete m_paneModel;
    delete m_presenter;
    delete m_pane;
  }

  void test_setUpInstrumentAnalysisSplitter() {
    CompositeFunction_sptr composite = std::dynamic_pointer_cast<Mantid::API::CompositeFunction>(
        Mantid::API::FunctionFactory::Instance().createFunction("CompositeFunction"));

    auto func = Mantid::API::FunctionFactory::Instance().createInitialized("name = FlatBackground");
    composite->addFunction(func);

    EXPECT_CALL(*m_model, getDefaultFunction()).Times(1).WillOnce(Return(composite));
    EXPECT_CALL(*m_view, setupAnalysisPane(m_pane->getView())).Times(1);
    // this function is called at start up -> count is 1
    TS_ASSERT_EQUALS(m_pane->getAddCount(), 1);
    m_presenter->setUpInstrumentAnalysisSplitter();
    TS_ASSERT_EQUALS(m_pane->getAddCount(), 2);
  }

  void test_loadSideEffects() {
    EXPECT_CALL(*m_pane, clearCurrentWS()).Times(1);
    // need to write a wrapper of protected members to get this to work
    m_presenter->loadSideEffects();
  }

  void test_addInstrument() {
    // this is only called as part of initLayout
    EXPECT_CALL(*m_pane, getView()).Times(1);
    m_presenter->addInstrument();
    // want to check initLayout is called from base class or mock base class
    // funcs?
  }

  void test_setupALFInstrument() {

    EXPECT_CALL(*m_model, dataFileName()).Times(1).WillOnce(Return("ALF"));
    auto setup = m_presenter->setupALFInstrument();
    auto tmp = setup.first;
    TS_ASSERT_EQUALS(tmp.first, "ALF");
    std::function<bool(std::map<std::string, bool>)> func1 =
        std::bind(&IALFCustomInstrumentModel::extractTubeCondition, m_model, std::placeholders::_1);
    std::function<bool(std::map<std::string, bool>)> func2 =
        std::bind(&IALFCustomInstrumentModel::averageTubeCondition, m_model, std::placeholders::_1);
    // cannot compare std::function directly
    // check behaviour instead
    int run = 6113;
    auto data = mockALFData("CURVES", "ALF", run, false);
    m_model->setCurrentRun(run);

    std::map<std::string, bool> conditions = {{"plotStored", true}, {"hasCurve", true}, {"isTube", true}};
    TS_ASSERT_EQUALS(tmp.second[0](conditions), func1(conditions));
    TS_ASSERT_EQUALS(tmp.second[1](conditions), func2(conditions));

    conditions = {{"plotStored", true}, {"hasCurve", true}, {"isTube", false}};
    TS_ASSERT_EQUALS(tmp.second[0](conditions), func1(conditions));
    TS_ASSERT_EQUALS(tmp.second[1](conditions), func2(conditions));

    conditions = {{"plotStored", false}, {"hasCurve", true}, {"isTube", true}};
    TS_ASSERT_EQUALS(tmp.second[0](conditions), func1(conditions));
    TS_ASSERT_EQUALS(tmp.second[1](conditions), func2(conditions));

    conditions = {{"plotStored", true}, {"hasCurve", false}, {"isTube", true}};
    TS_ASSERT_EQUALS(tmp.second[0](conditions), func1(conditions));
    TS_ASSERT_EQUALS(tmp.second[1](conditions), func2(conditions));

    conditions = {{"plotStored", false}, {"hasCurve", false}, {"isTube", true}};
    TS_ASSERT_EQUALS(tmp.second[0](conditions), func1(conditions));
    TS_ASSERT_EQUALS(tmp.second[1](conditions), func2(conditions));

    AnalysisDataService::Instance().remove("extractedTubes_ALF6113");
  }

  void test_extractSingleTube() {
    EXPECT_CALL(*m_model, extractSingleTube()).Times(1);
    EXPECT_CALL(*m_model, WSName()).Times(1).WillOnce(Return("test"));
    EXPECT_CALL(*m_pane, addSpectrum("test")).Times(1);
    EXPECT_CALL(*m_pane, updateEstimateAfterExtraction()).Times(1);
    m_presenter->extractSingleTube();
  }

  void test_averageTube() {
    EXPECT_CALL(*m_model, averageTube()).Times(1);
    EXPECT_CALL(*m_model, WSName()).Times(1).WillOnce(Return("test"));
    EXPECT_CALL(*m_pane, addSpectrum("test")).Times(1);

    m_presenter->averageTube();
  }

private:
  NiceMock<MockALFCustomInstrumentModel> *m_model;
  NiceMock<MockALFCustomInstrumentView> *m_view;
  NiceMock<MockPlotFitAnalysisPaneView> *m_paneView;
  NiceMock<MockPlotFitAnalysisPaneModel> *m_paneModel;
  NiceMock<MockPlotFitAnalysisPanePresenter> *m_pane;
  ALFCustomInstrumentPresenter *m_presenter;
};
