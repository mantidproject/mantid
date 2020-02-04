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

#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/MatrixWorkspace_fwd.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"
#include "MantidGeometry/Instrument.h"


#include <string>
#include <utility>

using namespace Mantid::API;
using namespace MantidQt::CustomInterfaces;
using namespace testing;
using Mantid::Geometry::Instrument;


// need to add mock objects..
class ALFModelTest : public ALFCustomInstrumentModel{
public:
    MOCK_METHOD0(extractSingleTube, void());
    MOCK_METHOD0(averageTube, void());
};

class ALFViewTest : public ALFCustomInstrumentView{
public:
    MOCK_METHOD1(addSpectrum, void(std::string name));
};

class paneTest : public MantidQt::MantidWidgets::PlotFitAnalysisPanePresenter{
public:
    // define init
    //using #mantidQt....;
    //paneTest(view, model){};
    MOCK_METHOD1(addSpectrum, void (const std::string &name));
};

class paneViewTest: public MantidQt::MantidWidgets::PlotFitAnalysisPaneView{
    void test(){};
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
 // m_model = new NiceMock<ALFModelTest>();
 // m_view = new NiceMock<ALFViewTest>();
 // auto paneView = new NiceMock<paneViewTest>();
 // auto paneModel = new NiceMock<paneModelTest>();
 // m_pane = new NiceMock<paneTest>(paneView, paneModel);
 // m_presenter = new ALFCustomInstrumentPresenter(m_view, m_model,m_pane);
  }

  void tearDown() override {
    AnalysisDataService::Instance().clear();
    //delete m_model;
    //delete m_view;
    //delete m_presenter;
    //delete m_pane;
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
 // m_presenter->extractSingleTube();
  //EXPECT_CALL(*m_model, averageTube()).Times(1);
}

private:
  //MatrixWorkspace_sptr m_workspace;
  //std::unique_ptr<SetUpADSWithWorkspace> m_ads;
  ALFModelTest *m_model;
  ALFViewTest *m_view;
  paneTest *m_pane;
  //ALFCustomInstrumentView *m_view;
  ALFCustomInstrumentPresenter *m_presenter;
};

#endif /* MANTIDQT_ALFCUSTOMINSTRUMENTPRESENTERTEST_H_ */
