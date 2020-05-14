// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "IndirectFitAnalysisTab.h"
#include "MantidAPI/FunctionFactory.h"
#include <cxxtest/TestSuite.h>
#include <gmock/gmock.h>

#include "IndirectFitDataModel.h"
#include "MantidTestHelpers/IndirectFitDataCreationHelper.h"
#include "MantidAPI/AnalysisDataService.h"

using namespace MantidQt::CustomInterfaces::IDA;

class IndirectFitDataModelTest : public CxxTest::TestSuite {
public:
  IndirectFitDataModelTest() = default;

  void setUp() override {
    m_model.clear();
    auto resolutionWorkspace =
        Mantid::IndirectFitDataCreationHelper::createWorkspace(4, 5);
    auto dataWorkspace =
        Mantid::IndirectFitDataCreationHelper::createWorkspace(4, 5);
    Mantid::API::AnalysisDataService::Instance().addOrReplace(
        "resolution workspace", std::move(resolutionWorkspace));
    Mantid::API::AnalysisDataService::Instance().addOrReplace(
        "data workspace", std::move(dataWorkspace));
    m_model.addWorkspace("data workspace");
    m_model.setResolution("resolution workspace", TableDatasetIndex{0});
  }

  void tearDown() override { AnalysisDataService::Instance().clear(); }

  void test_that_getResolutionsForFit_return_correctly() {
    auto resolutionVector = m_model.getResolutionsForFit();

    TS_ASSERT_EQUALS(resolutionVector[2].first, "resolution workspace");
    TS_ASSERT_EQUALS(resolutionVector[2].second, 2);
  }

  void
  test_that_getResolutionsForFit_return_correctly_if_resolution_workspace_removed() {
    Mantid::API::AnalysisDataService::Instance().clear();

    auto resolutionVector = m_model.getResolutionsForFit();

    TS_ASSERT_EQUALS(resolutionVector[2].first, "");
    TS_ASSERT_EQUALS(resolutionVector[2].second, 0);
  }

private:
  IndirectFitDataModel m_model;
};
