// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidQtWidgets/Spectroscopy/DataModel.h"

#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/FunctionFactory.h"
#include "MantidAPI/SpectrumInfo.h"
#include "MantidFrameworkTestHelpers/IndirectFitDataCreationHelper.h"
#include "MantidGeometry/IDetector.h"
#include "MantidKernel/UnitConversion.h"
#include <cxxtest/TestSuite.h>
#include <gmock/gmock.h>
#include <math.h>

namespace DataModelTestingNamespace {
auto &ads_instance = Mantid::API::AnalysisDataService::Instance();
constexpr std::string DATA_WS1 = "DataWorkspace1";
constexpr std::string DATA_WS2 = "DataWorkspace2";
constexpr std::string RES_WS = "ResolutionWs";
constexpr std::string SPEC_RANGE = "0-3";

void createInvalidResolutionWorkspace(const std::string &name, bool storeADS = true) {
  const std::vector<double> x = {0., 1., 2., 0., 1., 2., 0., 1., 2., 0., 1., 2.};
  const std::vector<double> y = {sqrt(-1.), 1., 2., sqrt(-1.), 1., 2., sqrt(-1.), 1., 2., sqrt(-1.), 1., 2.};

  const auto alg = Mantid::API::AlgorithmManager::Instance().create("CreateWorkspace");
  alg->initialize();
  alg->setLogging(false);
  alg->setAlwaysStoreInADS(storeADS);
  alg->setProperty("OutputWorkspace", name);
  alg->setProperty("DataX", x);
  alg->setProperty("DataY", y);
  alg->setProperty("NSpec", 4);
  alg->execute();
}
} // namespace DataModelTestingNamespace

using namespace Mantid::API;
using namespace MantidQt::CustomInterfaces;
using namespace MantidQt::CustomInterfaces::Inelastic;
using namespace MantidQt::MantidWidgets;
using namespace DataModelTestingNamespace;

class DataModelTest : public CxxTest::TestSuite {
public:
  static DataModelTest *createSuite() { return new DataModelTest(); }

  static void destroySuite(DataModelTest *suite) { delete suite; }

  void setUp() override {
    m_fitData = std::make_unique<DataModel>();
    auto resolutionWorkspace = Mantid::IndirectFitDataCreationHelper::createWorkspace(4, 5);
    auto dataWorkspace1 = Mantid::IndirectFitDataCreationHelper::createWorkspace(4, 5);
    auto dataWorkspace2 = Mantid::IndirectFitDataCreationHelper::createWorkspace(4, 5);
    ads_instance.addOrReplace(RES_WS, std::move(resolutionWorkspace));
    ads_instance.addOrReplace(DATA_WS1, std::move(dataWorkspace1));
    ads_instance.addOrReplace(DATA_WS2, std::move(dataWorkspace2));
    m_fitData->addWorkspace(DATA_WS1, FunctionModelSpectra(SPEC_RANGE));
    m_fitData->addWorkspace(DATA_WS2, FunctionModelSpectra(SPEC_RANGE));
    m_fitData->setResolution(RES_WS, WorkspaceID{0}, FunctionModelSpectra(SPEC_RANGE));
    m_fitData->setResolution(RES_WS, WorkspaceID{1}, FunctionModelSpectra(SPEC_RANGE));
  }

  void tearDown() override { AnalysisDataService::Instance().clear(); }

  void test_get_workspace_by_id() {
    TS_ASSERT_EQUALS(m_fitData->getWorkspaceID(DATA_WS1).value, 0);
    TS_ASSERT_EQUALS(m_fitData->getWorkspaceID(DATA_WS2).value, 1);
    TS_ASSERT(m_fitData->getWorkspaceID("NotInModel").value >= m_fitData->getNumberOfWorkspaces().value);
  }

  void test_setResolution() {
    TS_ASSERT_EQUALS(m_fitData->setResolution(RES_WS, WorkspaceID{0}, FunctionModelSpectra(SPEC_RANGE)), true);
  }

  void test_setResolution_bad_data() {
    createInvalidResolutionWorkspace("NAN");
    TS_ASSERT_EQUALS(m_fitData->setResolution("NAN", WorkspaceID{0}, FunctionModelSpectra(SPEC_RANGE)), false);
  }

  void test_setResolution_ws_not_in_ads() {
    createInvalidResolutionWorkspace("NotADS_WS", false);
    TS_ASSERT_THROWS(m_fitData->setResolution("NotADS_WS", WorkspaceID{0}, FunctionModelSpectra(SPEC_RANGE)),
                     std::runtime_error const &);
  }

  void test_setResolution_wrong_name() {
    createInvalidResolutionWorkspace("res");
    TS_ASSERT_THROWS(m_fitData->setResolution("", WorkspaceID{0}, FunctionModelSpectra(SPEC_RANGE)),
                     std::runtime_error const &);
  }

  void test_hasWorkspace_returns_true_for_ws_in_model() { TS_ASSERT(m_fitData->hasWorkspace(DATA_WS1)); }

  void test_hasWorkspace_returns_false_for_ws_in_model() { TS_ASSERT(!m_fitData->hasWorkspace("FakeWS")); }

  void test_getWorkspace_returns_nullptr_is_outside_of_range() {
    TS_ASSERT_EQUALS(m_fitData->getWorkspace(WorkspaceID{2}), nullptr);
  }

  void test_getWorkspace_returns_ws_in_range() {
    TS_ASSERT_EQUALS(m_fitData->getWorkspace(WorkspaceID{0})->getName(), DATA_WS1);
  }

  void test_getSpectra_returns_empty_spectra_is_outside_of_range() {
    TS_ASSERT_EQUALS(m_fitData->getSpectra(WorkspaceID{2}).getString(), "");
  }

  void test_getSpectra_returns_spectra_in_range() {
    TS_ASSERT_EQUALS(m_fitData->getSpectra(WorkspaceID{0}).getString(), SPEC_RANGE);
  }

  void test_getNumberOfWorkspaces_returns_correct_number_of_workspaces() {
    TS_ASSERT_EQUALS(m_fitData->getNumberOfWorkspaces(), 2);
    auto dataWorkspace = Mantid::IndirectFitDataCreationHelper::createWorkspace(4, 5);
    ads_instance.addOrReplace("DataWorkspace3", std::move(dataWorkspace));
    m_fitData->addWorkspace("DataWorkspace3", FunctionModelSpectra("0"));
    TS_ASSERT_EQUALS(m_fitData->getNumberOfWorkspaces(), 3);
  }

  void test_getNumberOfSpectra_returns_correct_number_of_spectra() {
    TS_ASSERT_EQUALS(m_fitData->getNumberOfSpectra(WorkspaceID{0}), 4);
    auto dataWorkspace = Mantid::IndirectFitDataCreationHelper::createWorkspace(5, 5);
    ads_instance.addOrReplace("DataWorkspace3", std::move(dataWorkspace));
    m_fitData->addWorkspace("DataWorkspace3", FunctionModelSpectra("0-4"));
    TS_ASSERT_EQUALS(m_fitData->getNumberOfSpectra(WorkspaceID{2}), 5);
  }

  void test_getNumberOfSpectra_raise_error_when_out_of_ws_range() {
    TS_ASSERT_EQUALS(m_fitData->getNumberOfSpectra(WorkspaceID{0}), 4);
    TS_ASSERT_THROWS(m_fitData->getNumberOfSpectra(WorkspaceID{2}), const std::runtime_error &);
  }

  void test_getNumberOfDomains_returns_total_spectra_of_all_data() {
    TS_ASSERT_EQUALS(m_fitData->getNumberOfDomains(), 8);
  }

  void test_getDomainIndex_calculates_correct_value() {
    TS_ASSERT_EQUALS(m_fitData->getDomainIndex(WorkspaceID(0), WorkspaceIndex(0)), FitDomainIndex{0});
    TS_ASSERT_EQUALS(m_fitData->getDomainIndex(WorkspaceID(0), WorkspaceIndex(1)), FitDomainIndex{1});
    TS_ASSERT_EQUALS(m_fitData->getDomainIndex(WorkspaceID(0), WorkspaceIndex(2)), FitDomainIndex{2});
    TS_ASSERT_EQUALS(m_fitData->getDomainIndex(WorkspaceID(0), WorkspaceIndex(3)), FitDomainIndex{3});
    TS_ASSERT_EQUALS(m_fitData->getDomainIndex(WorkspaceID(1), WorkspaceIndex(0)), FitDomainIndex{4});
  }

  void test_getQValuesForData_returns_correct_value() {
    auto dataWorkspace = Mantid::IndirectFitDataCreationHelper::createWorkspaceWithInelasticInstrument(4);
    ads_instance.addOrReplace("DataWorkspaceInelastic", dataWorkspace);
    m_fitData->addWorkspace("DataWorkspaceInelastic", FunctionModelSpectra("0"));
    auto spectrumInfo = dataWorkspace->spectrumInfo();
    auto detID = spectrumInfo.detector(0).getID();
    double efixed = dataWorkspace->getEFixed(detID);
    double usignTheta = 0.5 * spectrumInfo.twoTheta(0);
    double q = Mantid::Kernel::UnitConversion::convertToElasticQ(usignTheta, efixed);

    TS_ASSERT_EQUALS(m_fitData->getQValuesForData()[0], q)
  }

  void test_that_getResolutionsForFit_return_correctly() {
    auto resolutionVector = m_fitData->getResolutionsForFit();

    TS_ASSERT_EQUALS(resolutionVector[2].first, RES_WS);
    TS_ASSERT_EQUALS(resolutionVector[2].second, 2);
  }

  void test_that_getResolutionsForFit_return_correctly_if_resolution_workspace_removed() {
    ads_instance.clear();

    auto resolutionVector = m_fitData->getResolutionsForFit();

    TS_ASSERT_EQUALS(resolutionVector[2].first, "");
    TS_ASSERT_EQUALS(resolutionVector[2].second, 2);
  }

  void test_getWorkspaceNames_returns_all_names() {
    std::vector<std::string> wsNames = {DATA_WS1, DATA_WS2};
    TS_ASSERT_EQUALS(m_fitData->getWorkspaceNames(), wsNames);
  }

  void test_removeWorkspace_functions_as_required() {
    std::vector<std::string> wsNames = {DATA_WS1};
    m_fitData->removeWorkspace(WorkspaceID{1});
    TS_ASSERT_EQUALS(m_fitData->getWorkspaceNames(), wsNames);
  }

  void test_removeDataByIndex_removes_only_single_spectra() {
    m_fitData->removeDataByIndex(FitDomainIndex{2});
    TS_ASSERT(m_fitData->hasWorkspace(DATA_WS1));
    TS_ASSERT_EQUALS(m_fitData->getSpectra(WorkspaceID{0}).getString(), "0-1,3");
  }

  void test_getExcludeRegion_returns_range_for_spectra() {
    std::vector<double> exclusionVector = {0.1, 0.3};
    auto excludeString = "0.100,0.300";
    m_fitData->setExcludeRegion(excludeString, FitDomainIndex{0});
    TS_ASSERT_EQUALS(m_fitData->getExcludeRegion(WorkspaceID{0}, WorkspaceIndex{0}), excludeString);
    TS_ASSERT_EQUALS(m_fitData->getExcludeRegionVector(WorkspaceID{0}, WorkspaceIndex{0}), exclusionVector);
    TS_ASSERT_EQUALS(m_fitData->getExcludeRegion(FitDomainIndex{0}), excludeString);
    TS_ASSERT_EQUALS(m_fitData->getExcludeRegionVector(FitDomainIndex{0}), exclusionVector);
  }

  void test_getFittingRange_returns_range_for_spectra() {
    std::pair<double, double> fittingPair = {0.0, 5.0};
    TS_ASSERT_EQUALS(m_fitData->getFittingRange(WorkspaceID{0}, WorkspaceIndex{0}), fittingPair);
    TS_ASSERT_EQUALS(m_fitData->getFittingRange(FitDomainIndex{0}), fittingPair);
  }

  void test_getSubIndices_returns_correct_value() {
    std::pair<WorkspaceID, WorkspaceIndex> subindices = {0, 3};
    TS_ASSERT_EQUALS(m_fitData->getSubIndices(FitDomainIndex{3}), subindices)
    subindices = {1, 0};
    TS_ASSERT_EQUALS(m_fitData->getSubIndices(FitDomainIndex{4}), subindices)
  }

  void test_can_set_spectra_on_existing_workspace() {
    m_fitData->setSpectra("1", WorkspaceID{0});

    TS_ASSERT_EQUALS(m_fitData->getSpectra(WorkspaceID{0}), FunctionModelSpectra("1"));
  }

  void test_that_setting_spectra_on_non_existent_workspace_throws_exception() {
    TS_ASSERT_THROWS(m_fitData->setSpectra("1", WorkspaceID{2}), const std::out_of_range &)
    TS_ASSERT_THROWS(m_fitData->setSpectra(FunctionModelSpectra("1"), WorkspaceID{2}), const std::out_of_range &)
  }

  void test_that_setting_startX_on_non_existent_workspace_throws_exception() {
    TS_ASSERT_THROWS(m_fitData->setStartX(0, WorkspaceID{2}), const std::out_of_range &)
    TS_ASSERT_THROWS(m_fitData->setStartX(0, WorkspaceID{2}, WorkspaceIndex{10}), const std::out_of_range &)
    TS_ASSERT_THROWS(m_fitData->setStartX(0, FitDomainIndex{20}), const std::runtime_error &)
  }

  void test_clear_model() {
    m_fitData->clearModel();
    TS_ASSERT(m_fitData->getFittingData()->size() == 0);
  }

  void test_names_reference() {
    m_fitData->updateWorkspaceNames();
    TS_ASSERT_EQUALS(m_fitData->workspaceNames(), std::set<std::string>({DATA_WS1, DATA_WS2}));

    m_fitData->removeWorkspaceByName(DATA_WS1);
    m_fitData->updateWorkspaceNames();
    TS_ASSERT_EQUALS(m_fitData->workspaceNames(), std::set<std::string>{DATA_WS2});

    m_fitData->removeWorkspaceByName(DATA_WS2);
    m_fitData->updateWorkspaceNames();
    TS_ASSERT(m_fitData->workspaceNames().empty());
  }

  void test_resolutions_reference() {
    m_fitData->updateWorkspaceNames();
    TS_ASSERT_EQUALS(m_fitData->resolutionNames(), std::set<std::string>({RES_WS}));

    m_fitData->removeResolution(RES_WS);
    m_fitData->updateWorkspaceNames();
    TS_ASSERT(m_fitData->resolutionNames().empty());
    // There can't be workspaces without resolution
    TS_ASSERT(m_fitData->workspaceNames().empty());
  }

  void test_get_resolution_name_by_id_and_index() {
    TS_ASSERT_EQUALS(m_fitData->getResolutionName(0, 0), RES_WS);
    TS_ASSERT_EQUALS(m_fitData->getResolutionName(1, 2), RES_WS);
    TS_ASSERT_EQUALS(m_fitData->getResolutionName(1, 5), "");
    TS_ASSERT_EQUALS(m_fitData->getResolutionName(5, 1), "");
  }

private:
  std::unique_ptr<IDataModel> m_fitData;
};
