// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTIDQT_ALFCUSTOMINSTRUMENTPRESENTERTEST_H_
#define MANTIDQT_ALFCUSTOMINSTRUMENTPRESENTERTEST_H_

#include <cxxtest/TestSuite.h>
#include <gmock/gmock.h>

#include "ALFCustomInstrumentModel.h"
#include "ALFCustomInstrumentView.h"
#include "ALFCustomInstrumentPresenter.h"
#include "MantidQtWidgets/InstrumentView/PlotFitAnalysisPanePresenter.h"
#include "MantidQtWidgets/InstrumentView/PlotFitAnalysisPaneView.h"
#include "MantidQtWidgets/InstrumentView/PlotFitAnalysisPaneModel.h"
#include "MantidQtWidgets/Common/ObserverPattern.h"

#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/MatrixWorkspace_fwd.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"
#include "MantidGeometry/Instrument.h"


#include <string>
#include <utility>
#include<iostream>
using namespace Mantid::API;
using namespace MantidQt::CustomInterfaces;
using namespace testing;
using Mantid::Geometry::Instrument;
using namespace MantidQt::MantidWidgets;


//// need to add mock objects..
class FullALFModelTest : public IALFCustomInstrumentModel{
public:
    FullALFModelTest(){};
    ~FullALFModelTest(){};
    MOCK_METHOD1(loadAlg, void(const std::string &name));
    MOCK_METHOD0(transformData, void());
    MOCK_METHOD0(isDataValid, std::map<std::string, bool>());
    MOCK_METHOD1(storeSingleTube, void(const std::string &name));
    MOCK_METHOD0(averageTube,void());
    MOCK_METHOD1(hasTubeBeenExtracted, bool(const std::string &name));
    MOCK_METHOD1(extractTubeCondition, bool(std::map<std::string, bool> tabBools));
    MOCK_METHOD1(averageTubeCondition, bool(std::map<std::string, bool> tabBools));
    MOCK_METHOD0(extractSingleTube, void());
    MOCK_METHOD0(WSName, std::string());
    MOCK_METHOD0(getDefaultFunction, Mantid::API::CompositeFunction_sptr());

    MOCK_METHOD0(loadEmptyInstrument, void());
    MOCK_METHOD1(loadData, std::pair<int, std::string>(const std::string &name));
    MOCK_METHOD1(setCurrentRun, void(int &run));
    MOCK_METHOD0(getCurrentRun, int());
    MOCK_METHOD0(rename, void());
    MOCK_METHOD0(remove, void());
    MOCK_METHOD0(dataFileName, std::string());
    MOCK_METHOD0(currentRun, int());
    MOCK_METHOD1(isErrorCode, bool(const int run));
    MOCK_METHOD0(getInstrument, const std::string());
    MOCK_METHOD0(getTmpName, const std::string());
    MOCK_METHOD0(getWSName, const std::string());
};
//
//class ALFViewTest : public IALFCustomInstrumentView, public IBaseCustomInstrumentView{
class ALFViewTest : public IALFCustomInstrumentView{//, public IBaseCustomInstrumentView{
public:
    explicit ALFViewTest(const std::string &instrument, QWidget *parent = nullptr) {};
    ~ALFViewTest(){};

    MOCK_METHOD1(observeExtractSingleTube, void(Observer *listner));
    MOCK_METHOD1(observeAverageTube, void(Observer *listner));
    MOCK_METHOD1(addSpectrum, void(std::string name));
    MOCK_METHOD1(setupAnalysisPane, void(IPlotFitAnalysisPaneView *analysis));

    MOCK_METHOD0(getFile, std::string());
    MOCK_METHOD1(setRunQuietly, void(const std::string &runNumber));
    MOCK_METHOD1(observeLoadRun, void( Observer *listener));
    MOCK_METHOD1(warningBox, void(const std::string &error));
    MOCK_METHOD1(setInstrumentWidget, void(InstrumentWidget *instrument));
    MOCK_METHOD0(getInstrumentView, InstrumentWidget*());
    MOCK_METHOD2(setUpInstrument,void(const std::string &fileName, std::vector<std::function<bool(std::map<std::string,bool>)>> &binders));
    MOCK_METHOD1(addObserver, void(std::tuple<std::string, Observer *> &listener));
    MOCK_METHOD1(setupInstrumentAnalysisSplitters, void(QWidget *analysis));
    MOCK_METHOD0(setupHelp, void());
   
};

class paneTest : public MantidQt::MantidWidgets::PlotFitAnalysisPanePresenter{
public:
    // define init
    paneTest(IPlotFitAnalysisPaneView *view, PlotFitAnalysisPaneModel *model):PlotFitAnalysisPanePresenter(view, model) {}; 
    ~paneTest(){};
    MOCK_METHOD1(addSpectrum, void (const std::string &name));
};

class paneViewTest : public MantidQt::MantidWidgets::IPlotFitAnalysisPaneView{
public:
   explicit paneViewTest(const double &start=1., const double &end=5., QWidget *parent = nullptr){};
   ~paneViewTest(){};
   MOCK_METHOD1(observeFitButton, void(Observer *listener));
   MOCK_METHOD0(getRange, std::pair<double, double>());
   MOCK_METHOD0(getFunction, Mantid::API::IFunction_sptr());
   MOCK_METHOD1(addSpectrum, void(std::string name));
   MOCK_METHOD1(addFitSpectrum, void(std::string name));
   MOCK_METHOD1(addFunction, void(Mantid::API::IFunction_sptr));
   MOCK_METHOD1(updateFunction, void(Mantid::API::IFunction_sptr));
   MOCK_METHOD1(fitWarning, void(const std::string &message));
   
   MOCK_METHOD0(getQWidget, QWidget*());
   MOCK_METHOD2(setupPlotFitSplitter, void(const double &start, const double &end)); 
   MOCK_METHOD2(createFitPane, QWidget*(const double &start, const double &end)); 
};
class paneModelTest : public MantidQt::MantidWidgets::PlotFitAnalysisPaneModel{
    void empty(){};
};

class ALFCustomInstrumentPresenterTest : public CxxTest::TestSuite {
public:
  /// WorkflowAlgorithms do not appear in the FrameworkManager without this line
  ALFCustomInstrumentPresenterTest() { FrameworkManager::Instance(); }

  static ALFCustomInstrumentPresenterTest *createSuite() { return new ALFCustomInstrumentPresenterTest(); }

  static void destroySuite(ALFCustomInstrumentPresenterTest *suite) { delete suite; }

  void setUp() override {
    //m_workspace = createWorkspace(4, 3);
    //m_ads = std::make_unique<SetUpADSWithWorkspace>("Name", m_workspace);
  m_model = new NiceMock<FullALFModelTest>();
  m_view = new NiceMock<ALFViewTest>("ALF");
  m_paneView = new NiceMock<paneViewTest>();
  m_paneModel = new NiceMock<paneModelTest>();
  m_pane = new NiceMock<paneTest>(m_paneView, m_paneModel);
  m_presenter = new ALFCustomInstrumentPresenter(m_view, m_model,m_pane);
  }

  void tearDown() override {
  std::cout<<"hi"<<std::endl;
    AnalysisDataService::Instance().clear();
    delete m_presenter;
    m_model = NULL;
    delete m_view;
    m_pane = NULL;
    m_paneModel = NULL;
    delete m_paneView;
  std::cout<<"bye"<<std::endl;
    //m_ads.reset();
    //m_workspace.reset();
    //m_model.reset();
  }

  void test_that_the_model_is_instantiated_and_can_hold_a_workspace() {
    return; //SpectraLegacy const spectra = DiscontinuousSpectra<std::size_t>("0-1");

    //m_model->addWorkspace(m_workspace, spectra);

    //TS_ASSERT_EQUALS(m_model->numberOfWorkspaces(), 1);
  }
  void test_extractSingleTube(){
  m_presenter->extractSingleTube();
  EXPECT_CALL(*m_model, averageTube()).Times(1);
}

private:
  //MatrixWorkspace_sptr m_workspace;
  //std::unique_ptr<SetUpADSWithWorkspace> m_ads;
  NiceMock<FullALFModelTest> *m_model;
  NiceMock<ALFViewTest> *m_view;
  NiceMock<paneViewTest> *m_paneView;
  NiceMock<paneModelTest> *m_paneModel;
  NiceMock<paneTest> *m_pane;
  ALFCustomInstrumentPresenter *m_presenter;
};

#endif /* MANTIDQT_ALFCUSTOMINSTRUMENTPRESENTERTEST_H_ */
