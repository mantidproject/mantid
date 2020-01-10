// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTIDQT_ALFCUSTOMINSTRUMENTPRESENTERTEST_H_
#define MANTIDQT_ALFCUSTOMINSTRUMENTPRESENTERTEST_H_

#include <cxxtest/TestSuite.h>

#include "ALFCustomInstrumentModel.h"

#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/MatrixWorkspace_fwd.h"

using namespace Mantid::API;
using namespace MantidQt::CustomInterfaces;

// need to add mock objects..

class ALFCustomInstrumentPresenterTest : public CxxTest::TestSuite {
public:
  /// WorkflowAlgorithms do not appear in the FrameworkManager without this line
  ALFCustomInstrumentPresenterTest() { FrameworkManager::Instance(); }

  static ALFCustomInstrumentPresenterTest *createSuite() { return new ALFCustomInstrumentPresenterTest(); }

  static void destroySuite(ALFCustomInstrumentPresenterTest *suite) { delete suite; }

  void setUp() override {
    //m_workspace = createWorkspace(4, 3);
    //m_ads = std::make_unique<SetUpADSWithWorkspace>("Name", m_workspace);
  m_model = new ALFCustomInstrumentModel();
  //m_view = new ALFCustomInstrumentView("ALF", this);
  //m_presenter = new ALFCustomInstrumentPResenter(m_view, m_model);
  }

  void tearDown() override {
    AnalysisDataService::Instance().clear();
    delete m_model;
    //m_ads.reset();
    //m_workspace.reset();
    //m_model.reset();
  }

  void test_that_the_model_is_instantiated_and_can_hold_a_workspace() {
    return; //SpectraLegacy const spectra = DiscontinuousSpectra<std::size_t>("0-1");

    //m_model->addWorkspace(m_workspace, spectra);

    //TS_ASSERT_EQUALS(m_model->numberOfWorkspaces(), 1);
  }
private:
  //MatrixWorkspace_sptr m_workspace;
  //std::unique_ptr<SetUpADSWithWorkspace> m_ads;
  ALFCustomInstrumentModel *m_model;
  //ALFCustomInstrumentView *m_view;
  //ALFCustomInstrumentPresenter *m_presenter;
};

#endif /* MANTIDQT_ALFCUSTOMINSTRUMENTPRESENTERTEST_H_ */
