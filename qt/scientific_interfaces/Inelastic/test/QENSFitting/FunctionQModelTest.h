// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <cxxtest/TestSuite.h>

#include "QENSFitting/FunctionQModel.h"

#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/MatrixWorkspace_fwd.h"
#include "MantidFrameworkTestHelpers/IndirectFitDataCreationHelper.h"

using namespace Mantid::API;
using namespace Mantid::IndirectFitDataCreationHelper;
using namespace MantidQt::CustomInterfaces::Inelastic;

namespace {

std::vector<std::string> getParameterLabels() { return {"f0.EISF", "f1.Width", "f1.FWHM", "f1.EISF"}; }

} // namespace

class FunctionQModelTest : public CxxTest::TestSuite {
public:
  static FunctionQModelTest *createSuite() { return new FunctionQModelTest(); }

  static void destroySuite(FunctionQModelTest *suite) { delete suite; }

  void setUp() override {
    m_workspace = createWorkspaceWithTextAxis(4, getParameterLabels());
    m_ads = std::make_unique<SetUpADSWithWorkspace>("Name", m_workspace);
    m_model = std::make_unique<FunctionQModel>();
  }

  void tearDown() override {
    AnalysisDataService::Instance().clear();

    m_ads.reset();
    m_workspace.reset();
    m_model.reset();
  }

  void test_that_the_model_is_instantiated_and_can_hold_a_workspace() {

    m_model->getFitDataModel()->addWorkspace(m_workspace, FunctionModelSpectra("0-3"));
    TS_ASSERT_EQUALS(m_model->getNumberOfWorkspaces(), WorkspaceID{1});
  }

private:
  MatrixWorkspace_sptr m_workspace;
  std::unique_ptr<SetUpADSWithWorkspace> m_ads;
  std::unique_ptr<FunctionQModel> m_model;
};
