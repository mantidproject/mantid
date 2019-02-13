// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTIDQT_IQTFITMODELTEST_H_
#define MANTIDQT_IQTFITMODELTEST_H_

#include <cxxtest/TestSuite.h>

#include "IqtFitModel.h"

#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/FunctionFactory.h"
#include "MantidAPI/IFunction.h"
#include "MantidAPI/MatrixWorkspace_fwd.h"
#include "MantidTestHelpers/IndirectFitDataCreationHelper.h"

using namespace Mantid::API;
using namespace Mantid::IndirectFitDataCreationHelper;
using namespace MantidQt::CustomInterfaces::IDA;

namespace {

std::string getFunctionString(bool multipleIntensities) {
  return multipleIntensities
             ? "name=ExpDecay,Height=1,Lifetime=1;name=ExpDecay,Height=1,"
               "Lifetime=0.0247558;name=FlatBackground,A0=0"
             : "name=LinearBackground,A0=0,A1=0,ties=(A0=0.000000,A1=0.0);"
               "(composite=Convolution,FixResolution=true,NumDeriv=true;"
               "name=Resolution,Workspace=Name,WorkspaceIndex=0;((composite="
               "ProductFunction,NumDeriv=false;name=Lorentzian,Amplitude=1,"
               "PeakCentre=0,FWHM=0.0175)))";
}

IFunction_sptr getFunction(std::string const &functionString) {
  return FunctionFactory::Instance().createInitialized(functionString);
}

} // namespace

class IqtFitModelTest : public CxxTest::TestSuite {
public:
  /// WorkflowAlgorithms do not appear in the FrameworkManager without this line
  IqtFitModelTest() { FrameworkManager::Instance(); }

  static IqtFitModelTest *createSuite() { return new IqtFitModelTest(); }

  static void destroySuite(IqtFitModelTest *suite) { delete suite; }

  void setUp() override {
    m_workspace = createWorkspace(4, 5);
    m_ads = std::make_unique<SetUpADSWithWorkspace>("Name", m_workspace);
    m_model = std::make_unique<IqtFitModel>();
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

  void test_that_getSpectrumDependentAttributes_will_return_an_empty_vector() {
    TS_ASSERT(m_model->getSpectrumDependentAttributes().empty());
  }

  void
  test_that_canConstrainIntensities_returns_false_if_it_contains_less_than_2_intensity_parameters() {
    /// Intensity can either be represented by A0 or Height IqtFit
    Spectra const spectra = DiscontinuousSpectra<std::size_t>("0-1");

    m_model->addWorkspace(m_workspace, spectra);
    m_model->setFitFunction(getFunction(getFunctionString(false)));

    TS_ASSERT(!m_model->canConstrainIntensities());
  }

  void
  test_that_canConstrainIntensities_returns_true_if_it_contains_2_or_more_intensity_parameters() {
    /// Intensity can either be represented by A0 or Height in IqtFit
    Spectra const spectra = DiscontinuousSpectra<std::size_t>("0-1");

    m_model->addWorkspace(m_workspace, spectra);
    m_model->setFitFunction(getFunction(getFunctionString(true)));

    TS_ASSERT(m_model->canConstrainIntensities());
  }

  void
  test_that_setConstrainIntensities_returns_false_if_there_is_not_multiple_intensities_to_be_constrained() {
    Spectra const spectra = DiscontinuousSpectra<std::size_t>("0-1");

    m_model->addWorkspace(m_workspace, spectra);
    m_model->setFitFunction(getFunction(getFunctionString(false)));

    TS_ASSERT(!m_model->setConstrainIntensities(true));
  }

  void
  test_that_setConstrainIntensities_returns_true_if_there_are_multiple_intensities_to_be_constrained() {
    Spectra const spectra = DiscontinuousSpectra<std::size_t>("0-1");

    m_model->addWorkspace(m_workspace, spectra);
    m_model->setFitFunction(getFunction(getFunctionString(true)));

    TS_ASSERT(m_model->setConstrainIntensities(true));
  }

private:
  MatrixWorkspace_sptr m_workspace;
  std::unique_ptr<SetUpADSWithWorkspace> m_ads;
  std::unique_ptr<IqtFitModel> m_model;
};

#endif /* MANTIDQT_IQTFITMODELTEST_H_ */
