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
#include "MantidAPI/MultiDomainFunction.h"
#include "MantidTestHelpers/IndirectFitDataCreationHelper.h"

using namespace Mantid::API;
using namespace Mantid::IndirectFitDataCreationHelper;
using namespace MantidQt::CustomInterfaces::IDA;

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
    Spectra const spectra = Spectra("0-1");

    m_model->addWorkspace(m_workspace, spectra);

    TS_ASSERT_EQUALS(m_model->numberOfWorkspaces(), TableDatasetIndex{1});
  }

  void test_that_getSpectrumDependentAttributes_will_return_an_empty_vector() {
    TS_ASSERT(m_model->getSpectrumDependentAttributes().empty());
  }

private:
  MatrixWorkspace_sptr m_workspace;
  std::unique_ptr<SetUpADSWithWorkspace> m_ads;
  std::unique_ptr<IqtFitModel> m_model;
};

#endif /* MANTIDQT_IQTFITMODELTEST_H_ */
