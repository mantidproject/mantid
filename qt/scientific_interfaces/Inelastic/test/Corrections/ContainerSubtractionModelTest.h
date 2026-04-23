// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "Corrections/ContainerSubtractionModel.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidFrameworkTestHelpers/WorkspaceCreationHelper.h"
#include <cxxtest/TestSuite.h>
#include <gmock/gmock.h>

using namespace testing;
using namespace MantidQt::CustomInterfaces;

class ContainerSubtractionModelTest : public CxxTest::TestSuite {
public:
  ContainerSubtractionModelTest() : m_ads(Mantid::API::AnalysisDataService::Instance()) {}
  static ContainerSubtractionModelTest *createSuite() { return new ContainerSubtractionModelTest(); }

  static void destroySuite(ContainerSubtractionModelTest *suite) { delete suite; }

  void setUp() override {
    m_model = std::make_unique<ContainerSubtractionModel>();
    m_workspace = WorkspaceCreationHelper::create2DWorkspace(5, 4);
  }

  void tearDown() override {
    m_model.reset();
    m_ads.clear();
  }

  void test_getters_and_setters() {
    const auto wsName = "test";
    m_ads.add(wsName, m_workspace);

    m_model->setSampleWS(wsName);
    const auto sampleWsName = m_model->sampleWS()->getName();
    TS_ASSERT_EQUALS(wsName, sampleWsName);

    m_model->setCanWS(wsName);
    const auto canWsName = m_model->canWS()->getName();
    TS_ASSERT_EQUALS(wsName, canWsName);

    m_model->setSubtractedWS(wsName);
    const auto subWsName = m_model->subtractedWS()->getName();
    TS_ASSERT_EQUALS(wsName, subWsName);
  }

  void test_reset_subtract_ws() {
    const auto wsName = "test";
    m_ads.add(wsName, m_workspace);

    m_model->setSubtractedWS(wsName);
    TS_ASSERT(m_model->subtractedWS());
    m_model->removeSubtractedWS();
    TS_ASSERT(!m_model->subtractedWS());
  }

  void test_valid_workspace_return() {
    auto outNames = std::vector<std::string>({"test"});
    const auto wsName = "test";
    m_ads.add(wsName, m_workspace);

    m_model->setSampleWS(wsName);
    TS_ASSERT_EQUALS(m_model->getAllValidWorkspaceNames().size(), outNames.size());
    TS_ASSERT_EQUALS(m_model->getAllValidWorkspaceNames(), outNames);
    m_model->setCanWS(wsName);
    outNames.emplace_back("test");
    TS_ASSERT_EQUALS(m_model->getAllValidWorkspaceNames().size(), outNames.size());
    TS_ASSERT_EQUALS(m_model->getAllValidWorkspaceNames(), outNames);
    m_model->setSubtractedWS(wsName);
    outNames.emplace_back("test");
    TS_ASSERT_EQUALS(m_model->getAllValidWorkspaceNames().size(), outNames.size());
    TS_ASSERT_EQUALS(m_model->getAllValidWorkspaceNames(), outNames);
  }

  void test_update_container_does_rebin_to_sample_for_shift_different_than_zero() {
    std::string wsName = "test";
    auto ws2 = WorkspaceCreationHelper::create2DWorkspace(5, 8);
    m_ads.add(wsName, m_workspace);
    m_ads.add(wsName + "can", ws2);
    m_model->setSampleWS(wsName);
    m_model->setCanWS(wsName + "can");

    m_model->updateContainer(0.0, 1.0);
    TS_ASSERT_DIFFERS(m_model->sampleWS()->getNumberBins(0), m_model->modCanWS()->getNumberBins(0));
    m_model->updateContainer(2.0, 1.0);
    TS_ASSERT_EQUALS(m_model->sampleWS()->getNumberBins(0), m_model->modCanWS()->getNumberBins(0));
  }

private:
  Mantid::API::AnalysisDataServiceImpl &m_ads;
  std::unique_ptr<IContainerSubtractionModel> m_model;
  Mantid::API::MatrixWorkspace_sptr m_workspace;
};
