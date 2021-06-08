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
#include "MantidAPI/AnalysisDataService.h"
#include "MantidTestHelpers/IndirectFitDataCreationHelper.h"

using namespace MantidQt::CustomInterfaces::IDA;
using namespace MantidQt::MantidWidgets;

class IndirectFitDataModelTest : public CxxTest::TestSuite {
public:
  IndirectFitDataModelTest() = default;

  void setUp() override {
    m_fitData = std::make_unique<IndirectFitDataModel>();
    auto resolutionWorkspace = Mantid::IndirectFitDataCreationHelper::createWorkspace(4, 5);
    auto dataWorkspace = Mantid::IndirectFitDataCreationHelper::createWorkspace(4, 5);
    Mantid::API::AnalysisDataService::Instance().addOrReplace("resolution workspace", std::move(resolutionWorkspace));
    Mantid::API::AnalysisDataService::Instance().addOrReplace("data workspace", std::move(dataWorkspace));
    m_fitData->addWorkspace("data workspace");
    m_fitData->setResolution("resolution workspace", TableDatasetIndex{0});
  }

  void tearDown() override { AnalysisDataService::Instance().clear(); }

  void test_that_getResolutionsForFit_return_correctly() {
    auto resolutionVector = m_fitData->getResolutionsForFit();

    TS_ASSERT_EQUALS(resolutionVector[2].first, "resolution workspace");
    TS_ASSERT_EQUALS(resolutionVector[2].second, 2);
  }

  void test_that_getResolutionsForFit_return_correctly_if_resolution_workspace_removed() {
    Mantid::API::AnalysisDataService::Instance().clear();

    auto resolutionVector = m_fitData->getResolutionsForFit();

    TS_ASSERT_EQUALS(resolutionVector[2].first, "");
    TS_ASSERT_EQUALS(resolutionVector[2].second, 2);
  }

  void test_can_set_spectra_on_existing_workspace() {
    m_fitData->setSpectra("1", TableDatasetIndex{0});

    TS_ASSERT_EQUALS(m_fitData->getSpectra(TableDatasetIndex{0}), FunctionModelSpectra("1"));
  }

  void test_that_setting_spectra_on_non_existent_workspace_throws_exception() {
    TS_ASSERT_THROWS(m_fitData->setSpectra("1", TableDatasetIndex{1}), const std::out_of_range &)
    TS_ASSERT_THROWS(m_fitData->setSpectra(FunctionModelSpectra("1"), TableDatasetIndex{1}), const std::out_of_range &)
  }

  void test_that_setting_startX_on_non_existent_workspace_throws_exception() {
    TS_ASSERT_THROWS(m_fitData->setStartX(0, TableDatasetIndex{1}), const std::out_of_range &)
    TS_ASSERT_THROWS(m_fitData->setStartX(0, TableDatasetIndex{1}, MantidQt::MantidWidgets::WorkspaceIndex{10}),
                     const std::out_of_range &)
  }

private:
  std::unique_ptr<IIndirectFitDataModel> m_fitData;
};
