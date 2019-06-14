// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTIDQT_MSDFITMODELTEST_H_
#define MANTIDQT_MSDFITMODELTEST_H_

#include <cxxtest/TestSuite.h>

#include "MSDFitModel.h"

#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/MatrixWorkspace_fwd.h"
#include "MantidTestHelpers/IndirectFitDataCreationHelper.h"

using namespace Mantid::API;
using namespace Mantid::IndirectFitDataCreationHelper;
using namespace MantidQt::CustomInterfaces::IDA;

class MSDFitModelTest : public CxxTest::TestSuite {
public:
  /// WorkflowAlgorithms do not appear in the FrameworkManager without this line
  MSDFitModelTest() { FrameworkManager::Instance(); }

  static MSDFitModelTest *createSuite() { return new MSDFitModelTest(); }

  static void destroySuite(MSDFitModelTest *suite) { delete suite; }

  void setUp() override {
    m_workspace = createWorkspace(4, 3);
    m_ads = std::make_unique<SetUpADSWithWorkspace>("Name", m_workspace);
    m_model = std::make_unique<MSDFitModel>();
  }

  void tearDown() override {
    AnalysisDataService::Instance().clear();

    m_ads.reset();
    m_workspace.reset();
    m_model.reset();
  }

  void test_that_the_model_is_instantiated_and_can_hold_a_workspace() {
    Spectra const spectra = DiscontinuousSpectra<std::size_t>("0-1");

    m_model->addWorkspace(m_workspace, spectra);

    TS_ASSERT_EQUALS(m_model->numberOfWorkspaces(), 1);
  }

  void
  test_that_sequentialFitOutputName_returns_the_correct_name_which_uses_the_fit_string_set() {
    Spectra const spectra = DiscontinuousSpectra<std::size_t>("0-1");

    m_model->addWorkspace(m_workspace, spectra);
    m_model->setFitType("Gaussian");
    TS_ASSERT_EQUALS(m_model->sequentialFitOutputName(),
                     "Name_MSDFit_Gaussian_s0-1_Results");
  }

  void
  test_that_simultaneousFitOutputName_returns_the_correct_name_which_uses_the_fit_string_set() {
    Spectra const spectra = DiscontinuousSpectra<std::size_t>("0-1");

    m_model->addWorkspace(m_workspace, spectra);
    m_model->setFitType("Gaussian");
    TS_ASSERT_EQUALS(m_model->simultaneousFitOutputName(),
                     "Name_MSDFit_Gaussian_s0-1_Results");
  }

  void
  test_that_singleFitOutputName_returns_the_correct_name_which_uses_the_fit_string_set() {
    Spectra const spectra = DiscontinuousSpectra<std::size_t>("0-1");

    m_model->addWorkspace(m_workspace, spectra);
    m_model->setFitType("Gaussian");
    TS_ASSERT_EQUALS(m_model->singleFitOutputName(0, 0),
                     "Name_MSDFit_Gaussian_s0_Results");
  }

  void test_that_getSpectrumDependentAttributes_returns_an_empty_vector() {
    TS_ASSERT(m_model->getSpectrumDependentAttributes().empty());
  }

private:
  MatrixWorkspace_sptr m_workspace;
  std::unique_ptr<SetUpADSWithWorkspace> m_ads;
  std::unique_ptr<MSDFitModel> m_model;
};

#endif /* MANTIDQT_MSDFITMODELTEST_H_ */
