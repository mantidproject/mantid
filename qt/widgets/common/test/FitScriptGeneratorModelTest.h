// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/CompositeFunction.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/FunctionFactory.h"
#include "MantidAPI/IFunction.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidQtWidgets/Common/FitDomain.h"
#include "MantidQtWidgets/Common/FitScriptGeneratorMockObjects.h"
#include "MantidQtWidgets/Common/FitScriptGeneratorModel.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"

#include <cxxtest/TestSuite.h>

#include <memory>

using namespace MantidQt::MantidWidgets;
using namespace WorkspaceCreationHelper;

namespace {

Mantid::API::IFunction_sptr createIFunction(std::string const &functionString) {
  return Mantid::API::FunctionFactory::Instance().createInitialized(
      functionString);
}

Mantid::API::CompositeFunction_sptr
toComposite(Mantid::API::IFunction_sptr function) {
  return std::dynamic_pointer_cast<Mantid::API::CompositeFunction>(function);
}

Mantid::API::CompositeFunction_sptr createEmptyComposite() {
  return toComposite(createIFunction("name=CompositeFunction"));
}

} // namespace

class FitScriptGeneratorModelTest : public CxxTest::TestSuite {

public:
  FitScriptGeneratorModelTest()
      : m_wsName("Name"), m_wsIndex(MantidQt::MantidWidgets::WorkspaceIndex(0)),
        m_workspace(create2DWorkspace(3, 3)),
        m_startX(m_workspace->x(m_wsIndex.value).front()),
        m_endX(m_workspace->x(m_wsIndex.value).back()) {
    Mantid::API::FrameworkManager::Instance();
  }

  static FitScriptGeneratorModelTest *createSuite() {
    return new FitScriptGeneratorModelTest;
  }

  static void destroySuite(FitScriptGeneratorModelTest *suite) { delete suite; }

  void setUp() override {
    m_flatBackground = createIFunction("name=FlatBackground");
    m_expDecay = createIFunction("name=ExpDecay");

    auto composite = createEmptyComposite();
    composite->addFunction(createIFunction("name=FlatBackground"));
    composite->addFunction(createIFunction("name=ExpDecay"));
    m_composite = composite;

    Mantid::API::AnalysisDataService::Instance().addOrReplace(m_wsName,
                                                              m_workspace);

    m_model = std::make_unique<FitScriptGeneratorModel>();
    m_presenter =
        std::make_unique<MockFitScriptGeneratorPresenter>(m_model.get());
  }

  void tearDown() override {
    Mantid::API::AnalysisDataService::Instance().clear();
  }

  void test_end() {}

private:
  std::string m_wsName;
  MantidQt::MantidWidgets::WorkspaceIndex m_wsIndex;
  Mantid::API::MatrixWorkspace_sptr m_workspace;
  double m_startX;
  double m_endX;
  Mantid::API::IFunction_sptr m_flatBackground;
  Mantid::API::IFunction_sptr m_expDecay;
  Mantid::API::IFunction_sptr m_composite;

  std::unique_ptr<FitScriptGeneratorModel> m_model;
  std::unique_ptr<MockFitScriptGeneratorPresenter> m_presenter;
};
