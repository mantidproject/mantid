// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2022 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <cxxtest/TestSuite.h>
#include <gmock/gmock.h>

#include "ALFInstrumentModel.h"
#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/AlgorithmProperties.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/MatrixWorkspace_fwd.h"
#include "MantidAPI/NumericAxis.h"
#include "MantidAPI/TextAxis.h"
#include "MantidFrameworkTestHelpers/WorkspaceCreationHelper.h"
#include "MantidGeometry/Instrument.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/Unit.h"
#include "MantidKernel/UnitFactory.h"
#include "MockInstrumentActor.h"

#include <memory>
#include <string>

using namespace Mantid::API;
using namespace Mantid::Geometry;
using namespace Mantid::Kernel;
using namespace MantidQt::CustomInterfaces;
using namespace testing;

namespace {
auto &ADS = AnalysisDataService::Instance();

std::string const NOT_IN_ADS = "not_stored_in_ads";

std::vector<DetectorTube> findWholeTubes(Mantid::Geometry::ComponentInfo const &componentInfo,
                                         std::vector<std::size_t> const &partTubeDetectorIndices) {
  std::vector<DetectorTube> tubes;
  std::vector<std::size_t> allocatedIndices;
  for (auto const &detectorIndex : partTubeDetectorIndices) {
    auto const iter = std::find(allocatedIndices.cbegin(), allocatedIndices.cend(), detectorIndex);
    // Check that the indices for this tube haven't already been added
    if (iter == allocatedIndices.cend() && componentInfo.isDetector(detectorIndex)) {
      // Find all of the detector indices for the whole tube
      auto tubeDetectorIndices = componentInfo.detectorsInSubtree(componentInfo.parent(detectorIndex));
      std::transform(tubeDetectorIndices.cbegin(), tubeDetectorIndices.cend(), std::back_inserter(allocatedIndices),
                     [](auto const &index) { return index; });
      tubes.emplace_back(tubeDetectorIndices);
    }
  }
  return tubes;
}

MatrixWorkspace_sptr loadFile(std::string const &filename) {
  auto alg = AlgorithmManager::Instance().create("Load");
  alg->initialize();
  alg->setAlwaysStoreInADS(false);
  alg->setProperty("Filename", filename);
  alg->setProperty("OutputWorkspace", NOT_IN_ADS);
  alg->execute();
  Workspace_sptr outputWorkspace = alg->getProperty("OutputWorkspace");
  return std::dynamic_pointer_cast<MatrixWorkspace>(outputWorkspace);
}

MatrixWorkspace_sptr normaliseByCurrent(MatrixWorkspace_sptr const &inputWorkspace) {
  auto alg = AlgorithmManager::Instance().create("NormaliseByCurrent");
  alg->initialize();
  alg->setAlwaysStoreInADS(false);
  alg->setProperty("InputWorkspace", inputWorkspace);
  alg->setProperty("OutputWorkspace", NOT_IN_ADS);
  alg->execute();
  MatrixWorkspace_sptr outputWorkspace = alg->getProperty("OutputWorkspace");
  return outputWorkspace;
}

MatrixWorkspace_sptr convertUnits(MatrixWorkspace_sptr const &inputWorkspace, std::string const &target) {
  auto alg = AlgorithmManager::Instance().create("ConvertUnits");
  alg->initialize();
  alg->setAlwaysStoreInADS(false);
  alg->setProperty("InputWorkspace", inputWorkspace);
  alg->setProperty("Target", target);
  alg->setProperty("OutputWorkspace", NOT_IN_ADS);
  alg->execute();
  MatrixWorkspace_sptr outputWorkspace = alg->getProperty("OutputWorkspace");
  return outputWorkspace;
}

} // namespace

class ALFInstrumentModelTest : public CxxTest::TestSuite {
public:
  ALFInstrumentModelTest() {
    FrameworkManager::Instance();

    m_ALFData = "ALF82301.raw";

    m_singleTubeDetectorIDs = std::vector<DetectorTube>{{2500u, 2501u, 2502u}};
    m_multiTubeDetectorIDs = std::vector<DetectorTube>{{2500u, 2501u, 2502u}, {3500u, 3501u, 3502u}};

    m_model = std::make_unique<ALFInstrumentModel>();

    m_nonALFLoadedWs = loadFile("IRIS00072464.raw");
    m_loadedWs = convertUnits(normaliseByCurrent(loadFile(m_ALFData)), "dSpacing");
  }

  static ALFInstrumentModelTest *createSuite() { return new ALFInstrumentModelTest(); }

  static void destroySuite(ALFInstrumentModelTest *suite) { delete suite; }

  void setUp() override {
    m_instrumentActor = std::make_unique<NiceMock<MockInstrumentActor>>();

    // Clear the model
    m_model->setData(ALFData::SAMPLE, nullptr);
    m_model->setData(ALFData::VANADIUM, nullptr);
    m_model->setSelectedTubes({});
  }

  void tearDown() override { ADS.clear(); }

  void test_loadedWsName_returns_the_expected_name() { TS_ASSERT_EQUALS("ALFData", m_model->loadedWsName()); }

  void test_setData_will_not_load_an_empty_instrument_workspace_if_the_sample_was_previously_null() {
    ADS.clear();

    m_model->setData(ALFData::SAMPLE, m_loadedWs);

    TS_ASSERT(!ADS.doesExist("ALFData"));
  }

  void test_setData_does_not_load_an_instrument_workspace_if_the_sample_provided_is_not_null() {
    m_model->setData(ALFData::SAMPLE, m_loadedWs);
    ADS.clear();

    m_model->setData(ALFData::SAMPLE, m_loadedWs);

    TS_ASSERT(!ADS.doesExist("ALFData"));
  }

  void test_setData_loads_an_instrument_workspace_if_previous_sample_is_not_null_and_new_sample_is_null() {
    m_model->setData(ALFData::SAMPLE, m_loadedWs);
    ADS.clear();

    m_model->setData(ALFData::SAMPLE, nullptr);

    TS_ASSERT(ADS.doesExist("ALFData"));
  }

  void test_hasData_returns_false_when_a_sample_or_vanadium_is_not_loaded() {
    TS_ASSERT(!m_model->hasData(ALFData::SAMPLE));
    TS_ASSERT(!m_model->hasData(ALFData::VANADIUM));
  }

  void test_hasData_returns_true_when_a_sample_or_vanadium_is_loaded() {
    m_model->setData(ALFData::SAMPLE, m_loadedWs);
    m_model->setData(ALFData::VANADIUM, m_loadedWs);

    TS_ASSERT(m_model->hasData(ALFData::SAMPLE));
    TS_ASSERT(m_model->hasData(ALFData::VANADIUM));
  }

  void test_replaceSampleWorkspaceInADS_will_add_a_workspace_to_the_ADS() {
    ADS.clear();

    m_model->replaceSampleWorkspaceInADS(m_loadedWs);

    TS_ASSERT(ADS.doesExist("ALFData"));
  }

  void test_sampleRun_and_vanadiumRun_returns_zero_when_no_data_is_loaded() {
    TS_ASSERT_EQUALS(0u, m_model->run(ALFData::SAMPLE));
    TS_ASSERT_EQUALS(0u, m_model->run(ALFData::VANADIUM));
  }

  void test_sampleRun_returns_the_run_number_of_the_loaded_data() {
    m_model->setData(ALFData::SAMPLE, m_loadedWs);
    TS_ASSERT_EQUALS(82301u, m_model->run(ALFData::SAMPLE));
  }

  void test_vanadiumRun_returns_the_run_number_of_the_loaded_data() {
    m_model->setData(ALFData::VANADIUM, m_loadedWs);
    TS_ASSERT_EQUALS(82301u, m_model->run(ALFData::VANADIUM));
  }

  void test_isALFData_returns_false_when_the_workspace_is_ALF_data() {
    TS_ASSERT(!m_model->isALFData(m_nonALFLoadedWs));
  }

  void test_isALFData_returns_true_when_the_workspace_is_ALF_data() { TS_ASSERT(m_model->isALFData(m_loadedWs)); }

  void test_binningMismatch_returns_false_if_there_is_no_vanadium() {
    m_model->setData(ALFData::SAMPLE, m_loadedWs);
    TS_ASSERT(!m_model->binningMismatch());
  }

  void test_binningMismatch_returns_false_if_the_sample_and_vanadium_have_the_same_binning() {
    m_model->setData(ALFData::SAMPLE, m_loadedWs);
    m_model->setData(ALFData::VANADIUM, m_loadedWs);

    TS_ASSERT(!m_model->binningMismatch());
  }

  void test_binningMismatch_returns_true_if_the_sample_and_vanadium_have_different_binning() {
    m_model->setData(ALFData::SAMPLE, m_loadedWs);
    m_model->setData(ALFData::VANADIUM, m_nonALFLoadedWs);

    TS_ASSERT(m_model->binningMismatch());
  }

  void test_axisIsDSpacing_returns_false_if_the_axis_is_not_dSpacing() {
    m_model->setData(ALFData::SAMPLE, m_nonALFLoadedWs);
    TS_ASSERT(!m_model->axisIsDSpacing());
  }

  void test_axisIsDSpacing_returns_true_if_the_axis_is_dSpacing() {
    m_model->setData(ALFData::SAMPLE, m_loadedWs);
    TS_ASSERT(m_model->axisIsDSpacing());
  }

  void test_setSelectedTubes_will_set_an_empty_vector_of_tubes_when_provided_an_empty_vector() {
    TS_ASSERT(m_model->setSelectedTubes({{0u, 1u}}));

    TS_ASSERT(m_model->setSelectedTubes({}));

    TS_ASSERT(m_model->selectedTubes().empty());
  }

  void test_setSelectedTubes_returns_false_when_trying_to_set_the_tubes_to_a_list_which_is_the_same_as_previous() {
    TS_ASSERT(m_model->setSelectedTubes({{0u, 1u}}));
    TS_ASSERT(!m_model->setSelectedTubes({{0u, 1u}}));
  }

  void test_setSelectedTubes_will_select_the_detectors_in_an_entire_tube() {
    setSingleTubeSelected();

    auto const tubes = m_model->selectedTubes();
    TS_ASSERT_EQUALS(1u, tubes.size());
    TS_ASSERT_EQUALS(512u, tubes[0].size());
  }

  void test_setSelectedTubes_will_select_the_detectors_in_two_entire_tubes() {
    setMultipleTubesSelected();

    auto const tubes = m_model->selectedTubes();
    TS_ASSERT_EQUALS(2u, tubes.size());
    TS_ASSERT_EQUALS(512u, tubes[0].size());
    TS_ASSERT_EQUALS(512u, tubes[1].size());
  }

  void test_addSelectedTube_will_add_a_tube_to_the_model_when_it_is_new() {
    TS_ASSERT(m_model->addSelectedTube({{0u, 1u}}));
    TS_ASSERT(m_model->addSelectedTube({{1u, 2u}}));

    TS_ASSERT_EQUALS(2u, m_model->selectedTubes().size());
  }

  void test_addSelectedTube_will_not_add_a_tube_to_the_model_if_it_already_exists() {
    TS_ASSERT(m_model->addSelectedTube({{0u, 1u}}));
    TS_ASSERT(!m_model->addSelectedTube({{0u, 1u}}));

    TS_ASSERT_EQUALS(1u, m_model->selectedTubes().size());
  }

  void test_loadProperties_returns_the_expected_properties() {
    auto const properties = m_model->loadProperties(m_ALFData);

    std::string filename = properties->getProperty("Filename");
    std::string output = properties->getProperty("OutputWorkspace");

    TS_ASSERT_EQUALS(m_ALFData, filename);
    TS_ASSERT_EQUALS(NOT_IN_ADS, output);
  }

  void test_normaliseByCurrentProperties_returns_the_expected_properties() {
    auto const properties = m_model->normaliseByCurrentProperties(m_loadedWs);

    MatrixWorkspace_sptr input = properties->getProperty("InputWorkspace");
    std::string output = properties->getProperty("OutputWorkspace");

    TS_ASSERT_EQUALS(m_loadedWs, input);
    TS_ASSERT_EQUALS(NOT_IN_ADS, output);
  }

  void test_rebinToWorkspaceProperties_returns_the_expected_properties() {
    m_model->setData(ALFData::SAMPLE, m_loadedWs);
    m_model->setData(ALFData::VANADIUM, m_loadedWs);

    auto const properties = m_model->rebinToWorkspaceProperties();

    MatrixWorkspace_sptr toRebin = properties->getProperty("WorkspaceToRebin");
    MatrixWorkspace_sptr toMatch = properties->getProperty("WorkspaceToMatch");
    std::string output = properties->getProperty("OutputWorkspace");

    TS_ASSERT_EQUALS(m_loadedWs, toRebin);
    TS_ASSERT_EQUALS(m_loadedWs, toMatch);
    TS_ASSERT_EQUALS(NOT_IN_ADS, output);
  }

  void test_divideProperties_returns_the_expected_properties() {
    m_model->setData(ALFData::SAMPLE, m_loadedWs);
    m_model->setData(ALFData::VANADIUM, m_loadedWs);

    auto const properties = m_model->divideProperties();

    MatrixWorkspace_sptr lhs = properties->getProperty("LHSWorkspace");
    MatrixWorkspace_sptr rhs = properties->getProperty("RHSWorkspace");
    bool allowDiffNumSpectra = properties->getProperty("AllowDifferentNumberSpectra");

    std::string output = properties->getProperty("OutputWorkspace");

    TS_ASSERT_EQUALS(m_loadedWs, lhs);
    TS_ASSERT_EQUALS(m_loadedWs, rhs);
    TS_ASSERT(allowDiffNumSpectra);
    TS_ASSERT_EQUALS(NOT_IN_ADS, output);
  }

  void test_replaceSpecialValuesProperties_returns_the_expected_properties() {
    auto const properties = m_model->replaceSpecialValuesProperties(m_loadedWs);

    MatrixWorkspace_sptr input = properties->getProperty("InputWorkspace");
    double infinityValue = properties->getProperty("InfinityValue");
    double nanValue = properties->getProperty("NaNValue");
    bool checkErrorAxis = properties->getProperty("CheckErrorAxis");
    std::string output = properties->getProperty("OutputWorkspace");

    TS_ASSERT_EQUALS(m_loadedWs, input);
    TS_ASSERT_EQUALS(0.0, infinityValue);
    TS_ASSERT_EQUALS(1.0, nanValue);
    TS_ASSERT(checkErrorAxis);
    TS_ASSERT_EQUALS(NOT_IN_ADS, output);
  }

  void test_convertUnitsProperties_returns_the_expected_properties() {
    auto const properties = m_model->convertUnitsProperties(m_loadedWs);

    MatrixWorkspace_sptr input = properties->getProperty("InputWorkspace");
    std::string target = properties->getProperty("Target");
    std::string output = properties->getProperty("OutputWorkspace");

    TS_ASSERT_EQUALS(m_loadedWs, input);
    TS_ASSERT_EQUALS("dSpacing", target);
    TS_ASSERT_EQUALS(NOT_IN_ADS, output);
  }

  void test_scaleXProperties_returns_the_expected_properties() {
    auto const properties = m_model->scaleXProperties(m_loadedWs);

    MatrixWorkspace_sptr input = properties->getProperty("InputWorkspace");
    double factor = properties->getProperty("Factor");
    std::string output = properties->getProperty("OutputWorkspace");

    TS_ASSERT_EQUALS(m_loadedWs, input);
    TS_ASSERT_EQUALS(180.0 / M_PI, factor);
    TS_ASSERT_EQUALS(NOT_IN_ADS, output);
  }

  void test_rebunchProperties_returns_the_expected_properties() {
    setSingleTubeSelected();

    auto const properties = m_model->rebunchProperties(m_loadedWs);

    MatrixWorkspace_sptr input = properties->getProperty("InputWorkspace");
    int nBunch = properties->getProperty("NBunch");
    std::string output = properties->getProperty("OutputWorkspace");

    TS_ASSERT_EQUALS(m_loadedWs, input);
    TS_ASSERT_EQUALS(1, nBunch);
    TS_ASSERT_EQUALS(NOT_IN_ADS, output);
  }

  void test_createWorkspaceAlgorithmProperties_returns_the_expected_properties() {
    setSingleTubeSelected();
    expectInstrumentActorCalls();

    auto const properties = m_model->createWorkspaceAlgorithmProperties(*m_instrumentActor);

    std::string parentName = properties->getProperty("ParentWorkspace");
    int nSpec = properties->getProperty("NSpec");
    std::string unitX = properties->getProperty("UnitX");
    std::string output = properties->getProperty("OutputWorkspace");

    TS_ASSERT_EQUALS("ALFData", parentName);
    TS_ASSERT_EQUALS(1, nSpec);
    TS_ASSERT_EQUALS("Out of plane angle", unitX);
    TS_ASSERT_EQUALS(NOT_IN_ADS, output);

    std::vector<double> dataX = properties->getProperty("DataX");
    std::vector<double> dataY = properties->getProperty("DataY");

    TS_ASSERT_DELTA(dataX[1], -0.358565, 0.000001);
    TS_ASSERT_DELTA(dataX[2], -0.357311, 0.000001);
    TS_ASSERT_DELTA(dataY[1], 0.0, 0.000001);
    TS_ASSERT_DELTA(dataY[2], 0.0, 0.000001);
  }

  void test_twoThetasClosestToZero_will_initially_return_an_empty_vector() {
    TS_ASSERT(m_model->twoThetasClosestToZero().empty());
  }

  void test_twoThetasClosestToZero_returns_the_expected_two_theta_values() {
    setSingleTubeSelected();
    expectInstrumentActorCalls();

    (void)m_model->createWorkspaceAlgorithmProperties(*m_instrumentActor);

    auto const twoThetas = m_model->twoThetasClosestToZero();

    TS_ASSERT_EQUALS(1, twoThetas.size());
    TS_ASSERT_DELTA(twoThetas[0], 39.879471, 0.000001);
  }

  void test_createWorkspaceAlgorithmProperties_returns_the_expected_properties_for_multiple_tubes() {
    setMultipleTubesSelected();
    expectInstrumentActorCalls();

    auto const properties = m_model->createWorkspaceAlgorithmProperties(*m_instrumentActor);

    std::string parentName = properties->getProperty("ParentWorkspace");
    int nSpec = properties->getProperty("NSpec");
    std::string unitX = properties->getProperty("UnitX");
    std::string output = properties->getProperty("OutputWorkspace");

    TS_ASSERT_EQUALS("ALFData", parentName);
    TS_ASSERT_EQUALS(1, nSpec);
    TS_ASSERT_EQUALS("Out of plane angle", unitX);
    TS_ASSERT_EQUALS(NOT_IN_ADS, output);

    std::vector<double> dataX = properties->getProperty("DataX");
    std::vector<double> dataY = properties->getProperty("DataY");

    TS_ASSERT_DELTA(dataX[1], -0.358650, 0.000001);
    TS_ASSERT_DELTA(dataX[2], -0.358565, 0.000001);
    TS_ASSERT_DELTA(dataY[1], 0.0, 0.000001);
    TS_ASSERT_DELTA(dataY[2], 0.0, 0.000001);
  }

  void test_twoThetasClosestToZero_returns_the_expected_two_theta_values_for_multiple_tubes() {
    setMultipleTubesSelected();
    expectInstrumentActorCalls();

    (void)m_model->createWorkspaceAlgorithmProperties(*m_instrumentActor);

    auto const twoThetas = m_model->twoThetasClosestToZero();

    TS_ASSERT_EQUALS(2, twoThetas.size());
    TS_ASSERT_DELTA(twoThetas[0], 39.879471, 0.000001);
    TS_ASSERT_DELTA(twoThetas[1], 39.879471, 0.000001);
  }

private:
  void setSingleTubeSelected() {
    m_model->setData(ALFData::SAMPLE, m_loadedWs);
    m_model->replaceSampleWorkspaceInADS(m_loadedWs);

    m_singleTubeDetectorIDs = findWholeTubes(m_loadedWs->componentInfo(), {2500u, 2501u, 2502u});

    TS_ASSERT(m_model->setSelectedTubes(m_singleTubeDetectorIDs));
  }

  void setMultipleTubesSelected() {
    m_model->setData(ALFData::SAMPLE, m_loadedWs);
    m_model->replaceSampleWorkspaceInADS(m_loadedWs);

    m_multiTubeDetectorIDs = findWholeTubes(m_loadedWs->componentInfo(), {2500u, 2501u, 2502u, 3500u, 3501u, 3502u});

    TS_ASSERT(m_model->setSelectedTubes(m_multiTubeDetectorIDs));
  }

  void expectInstrumentActorCalls(std::size_t const workspaceIndex = 0u) {
    EXPECT_CALL(*m_instrumentActor, getWorkspace()).WillRepeatedly(Return(m_loadedWs));
    EXPECT_CALL(*m_instrumentActor, componentInfo()).WillRepeatedly(ReturnRef(m_loadedWs->componentInfo()));
    EXPECT_CALL(*m_instrumentActor, detectorInfo()).WillOnce(ReturnRef(m_loadedWs->detectorInfo()));
    EXPECT_CALL(*m_instrumentActor, getInstrument()).WillOnce(Return(m_loadedWs->getInstrument()));
    EXPECT_CALL(*m_instrumentActor, getWorkspaceIndex(_)).WillRepeatedly(Return(workspaceIndex));
    EXPECT_CALL(*m_instrumentActor, getBinMinMaxIndex(workspaceIndex, _, _))
        .WillRepeatedly(DoAll(SetArgReferee<1>(0u), SetArgReferee<2>(50u)));
  }

  std::string m_ALFData;

  Mantid::API::MatrixWorkspace_sptr m_nonALFLoadedWs;
  Mantid::API::MatrixWorkspace_sptr m_loadedWs;

  std::vector<DetectorTube> m_singleTubeDetectorIDs;
  std::vector<DetectorTube> m_multiTubeDetectorIDs;

  std::unique_ptr<NiceMock<MockInstrumentActor>> m_instrumentActor;
  std::unique_ptr<ALFInstrumentModel> m_model;
};
