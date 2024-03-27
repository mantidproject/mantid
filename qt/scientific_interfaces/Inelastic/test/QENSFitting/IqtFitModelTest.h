// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <cxxtest/TestSuite.h>

#include "QENSFitting/IqtFitModel.h"

#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/FunctionFactory.h"
#include "MantidAPI/IFunction.h"
#include "MantidAPI/MatrixWorkspace_fwd.h"
#include "MantidAPI/MultiDomainFunction.h"
#include "MantidFrameworkTestHelpers/IndirectFitDataCreationHelper.h"

using namespace Mantid::API;
using namespace Mantid::IndirectFitDataCreationHelper;
using namespace MantidQt::CustomInterfaces::Inelastic;

class IqtFitModelTest : public CxxTest::TestSuite {
public:
  static IqtFitModelTest *createSuite() { return new IqtFitModelTest(); }

  static void destroySuite(IqtFitModelTest *suite) { delete suite; }

  void setUp() override {
    // The only difference in IqtFitModel is the default parameters and a currently unused process for constraining
    // parameters. these tests will be included in the PR for fit functions and function browser formatting.
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
    FunctionModelSpectra const spectra = FunctionModelSpectra("0-1");

    m_model->getFitDataModel()->addWorkspace(m_workspace->getName(), spectra);

    TS_ASSERT_EQUALS(m_model->getNumberOfWorkspaces(), WorkspaceID{1});
  }

private:
  MatrixWorkspace_sptr m_workspace;
  std::unique_ptr<SetUpADSWithWorkspace> m_ads;
  std::unique_ptr<IqtFitModel> m_model;
};
