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
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/MatrixWorkspace_fwd.h"
#include "MantidAPI/NumericAxis.h"
#include "MantidAPI/TextAxis.h"
#include "MantidFrameworkTestHelpers/WorkspaceCreationHelper.h"
#include "MantidGeometry/Instrument.h"
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
  alg->setProperty("OutputWorkspace", "__not_in_ads");
  alg->execute();
  Workspace_sptr outputWorkspace = alg->getProperty("OutputWorkspace");
  return std::dynamic_pointer_cast<MatrixWorkspace>(outputWorkspace);
}

MatrixWorkspace_sptr changeBinOffset(MatrixWorkspace_sptr const &inputWorkspace) {
  auto alg = AlgorithmManager::Instance().create("ChangeBinOffset");
  alg->initialize();
  alg->setAlwaysStoreInADS(false);
  alg->setProperty("InputWorkspace", inputWorkspace);
  alg->setProperty("Offset", 0.1);
  alg->setProperty("OutputWorkspace", "__not_in_ads");
  alg->execute();
  MatrixWorkspace_sptr outputWorkspace = alg->getProperty("OutputWorkspace");
  return outputWorkspace;
}

} // namespace

class ALFInstrumentModelTest : public CxxTest::TestSuite {
public:
  ALFInstrumentModelTest() {
    FrameworkManager::Instance();

    m_nonALFData = "IRIS00072464.raw";
    m_ALFData = "ALF82301.raw";
    m_ALFEdgeCaseData = "ALF83743.raw";

    m_singleTubeDetectorIDs = std::vector<DetectorTube>{{2500u, 2501u, 2502u}};
    m_multiTubeDetectorIDs = std::vector<DetectorTube>{{2500u, 2501u, 2502u}, {3500u, 3501u, 3502u}};

    m_model = std::make_unique<ALFInstrumentModel>();

    m_loadedWs = loadFile(m_ALFData);
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
    TS_ASSERT_EQUALS(82301u, m_model->run(ALFData::SAMPLE));
  }

  void test_isALFData_returns_false_when_the_workspace_is_ALF_data() {
    TS_ASSERT(!m_model->isALFData(loadFile(m_nonALFData)));
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
    m_model->setData(ALFData::VANADIUM, loadFile(m_nonALFData));

    TS_ASSERT(!m_model->binningMismatch());
  }

  void test_axisIsDSpacing_returns_false_if_the_axis_is_not_dSpacing() {
    m_model->setData(ALFData::SAMPLE, loadFile(m_nonALFData));
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
    TS_ASSERT_EQUALS(m_ALFData, properties->getProperty("Filename"));
    TS_ASSERT_EQUALS(NOT_IN_ADS, properties->getProperty("OutputWorkspace"));
  }

  void test_normaliseByCurrentProperties_returns_the_expected_properties() {
    auto const properties = m_model->normaliseByCurrentProperties(m_loadedWs);
    TS_ASSERT_EQUALS(m_loadedWs, properties->getProperty("InputWorkspace"));
    TS_ASSERT_EQUALS(NOT_IN_ADS, properties->getProperty("OutputWorkspace"));
  }

  void test_rebinToWorkspaceProperties_returns_the_expected_properties() {
    m_model->setData(ALFData::SAMPLE, m_loadedWs);
    m_model->setData(ALFData::VANADIUM, m_loadedWs);

    auto const properties = m_model->rebinToWorkspaceProperties();

    TS_ASSERT_EQUALS(m_loadedWs, properties->getProperty("WorkspaceToRebin"));
    TS_ASSERT_EQUALS(m_loadedWs, properties->getProperty("WorkspaceToMatch"));
    TS_ASSERT_EQUALS(NOT_IN_ADS, properties->getProperty("OutputWorkspace"));
  }

  void test_divideProperties_returns_the_expected_properties() {
    m_model->setData(ALFData::SAMPLE, m_loadedWs);
    m_model->setData(ALFData::VANADIUM, m_loadedWs);

    auto const properties = m_model->divideProperties();

    TS_ASSERT_EQUALS(m_loadedWs, properties->getProperty("LHSWorkspace"));
    TS_ASSERT_EQUALS(m_loadedWs, properties->getProperty("RHSWorkspace"));
    TS_ASSERT(properties->getProperty("AllowDifferentNumberSpectra"));
    TS_ASSERT_EQUALS(NOT_IN_ADS, properties->getProperty("OutputWorkspace"));
  }

  void test_replaceSpecialValuesProperties_returns_the_expected_properties() {
    auto const properties = m_model->replaceSpecialValuesProperties(m_loadedWs);

    TS_ASSERT_EQUALS(m_loadedWs, properties->getProperty("InputWorkspace"));
    TS_ASSERT_EQUALS(0.0, properties->getProperty("InfinityValue"));
    TS_ASSERT_EQUALS(1.0, properties->getProperty("NaNValue"));
    TS_ASSERT(properties->getProperty("CheckErrorAxis"));
    TS_ASSERT_EQUALS(NOT_IN_ADS, properties->getProperty("OutputWorkspace"));
  }

  void test_convertUnitsProperties_returns_the_expected_properties() {
    auto const properties = m_model->convertUnitsProperties(m_loadedWs);

    TS_ASSERT_EQUALS(m_loadedWs, properties->getProperty("InputWorkspace"));
    TS_ASSERT_EQUALS("dSpacing", properties->getProperty("Target"));
    TS_ASSERT_EQUALS(NOT_IN_ADS, properties->getProperty("OutputWorkspace"));
  }

  void test_scaleXProperties_returns_the_expected_properties() {
    auto const properties = m_model->scaleXProperties(m_loadedWs);

    TS_ASSERT_EQUALS(m_loadedWs, properties->getProperty("InputWorkspace"));
    TS_ASSERT_EQUALS(180.0 / M_PI, properties->getProperty("Factor"));
    TS_ASSERT_EQUALS(NOT_IN_ADS, properties->getProperty("OutputWorkspace"));
  }

  void test_rebunchProperties_returns_the_expected_properties() {
    auto const properties = m_model->rebunchProperties(m_loadedWs);

    TS_ASSERT_EQUALS(m_loadedWs, properties->getProperty("InputWorkspace"));
    TS_ASSERT_EQUALS(2, properties->getProperty("NBunch"));
    TS_ASSERT_EQUALS(NOT_IN_ADS, properties->getProperty("OutputWorkspace"));
  }

  void test_generateOutOfPlaneAngleWorkspace_returns_nullptr_when_no_selected_detectors() {
    auto [workspace, twoThetas] = m_model->generateOutOfPlaneAngleWorkspace(*m_instrumentActor);

    TS_ASSERT_EQUALS(nullptr, workspace);
    TS_ASSERT(twoThetas.empty());
  }

  void test_generateOutOfPlaneAngleWorkspace_does_not_return_a_nullptr_when_single_tube_selected() {
    setSingleTubeSelected();
    expectInstrumentActorCalls();

    auto [workspace, twoThetas] = m_model->generateOutOfPlaneAngleWorkspace(*m_instrumentActor);

    TS_ASSERT(workspace);
    TS_ASSERT_EQUALS(1u, twoThetas.size());
    TS_ASSERT_DELTA(39.879471, twoThetas[0], 0.00001);
  }

  void test_generateOutOfPlaneAngleWorkspace_does_not_return_a_nullptr_when_multiple_tubes_selected() {
    setMultipleTubesSelected();
    expectInstrumentActorCalls();

    auto [workspace, twoThetas] = m_model->generateOutOfPlaneAngleWorkspace(*m_instrumentActor);

    TS_ASSERT(workspace);
    TS_ASSERT_EQUALS(2u, twoThetas.size());
    // The two thetas are the same because we use the same workspace index in the expectations
    TS_ASSERT_DELTA(39.879471, twoThetas[0], 0.00001);
    TS_ASSERT_DELTA(39.879471, twoThetas[1], 0.00001);
  }

  void test_generateLoadedWorkspace_does_not_throw_when_no_sample_is_set() {
    TS_ASSERT_THROWS_NOTHING(m_model->generateLoadedWorkspace());
  }

  void test_generateLoadedWorkspace_outputs_the_sample_workspace_when_no_vanadium_is_set() {
    m_model->setData(ALFData::SAMPLE, m_loadedWs);

    m_model->generateLoadedWorkspace();

    TS_ASSERT(ADS.doesExist("ALFData"));

    auto const workspace = ADS.retrieveWS<MatrixWorkspace>("ALFData");

    TS_ASSERT_EQUALS("dSpacing", workspace->getAxis(0)->unit()->unitID());
    TS_ASSERT_EQUALS(m_loadedWs->y(0), workspace->y(0));
  }

  void test_generateLoadedWorkspace_outputs_a_normalised_workspace_when_the_vanadium_is_set() {
    m_model->setData(ALFData::SAMPLE, m_loadedWs);
    m_model->setData(ALFData::VANADIUM, m_loadedWs);

    m_model->generateLoadedWorkspace();

    TS_ASSERT(ADS.doesExist("ALFData"));

    auto const workspace = ADS.retrieveWS<MatrixWorkspace>("ALFData");
    TS_ASSERT_EQUALS("dSpacing", workspace->getAxis(0)->unit()->unitID());
    TS_ASSERT_EQUALS(1.0, workspace->y(0)[0]);
    TS_ASSERT_EQUALS(1.0, workspace->y(0)[1]);
  }

  void test_generateLoadedWorkspace_handles_vanadium_with_different_binning() {
    auto const dataWs = m_model->loadAndNormalise(m_ALFData);
    m_model->setData(ALFData::SAMPLE, dataWs);
    m_model->setData(ALFData::VANADIUM, changeBinOffset(dataWs));

    m_model->generateLoadedWorkspace();

    TS_ASSERT(ADS.doesExist("ALFData"));

    auto const workspace = ADS.retrieveWS<MatrixWorkspace>("ALFData");
    TS_ASSERT_EQUALS("dSpacing", workspace->getAxis(0)->unit()->unitID());
    TS_ASSERT_EQUALS(1.0, workspace->y(0)[0]);
    TS_ASSERT_EQUALS(1.0, workspace->y(0)[1]);
  }

  void
  test_generateOutOfPlaneAngleWorkspace_will_create_a_workspace_with_the_expected_out_of_plane_angle_label_and_y_values() {
    setSingleTubeSelected();
    expectInstrumentActorCalls();

    auto [workspace, _] = m_model->generateOutOfPlaneAngleWorkspace(*m_instrumentActor);

    TS_ASSERT_EQUALS("Label", workspace->getAxis(0)->unit()->unitID());
    TS_ASSERT_EQUALS("Out of plane angle", std::string(workspace->getAxis(0)->unit()->label()));

    TS_ASSERT_DELTA(workspace->readX(0)[1], -20.544269, 0.000001);
    TS_ASSERT_DELTA(workspace->readX(0)[2], -20.472433, 0.000001);
    TS_ASSERT_DELTA(workspace->readY(0)[1], 0.0, 0.000001);
    TS_ASSERT_DELTA(workspace->readY(0)[2], 0.0, 0.000001);
  }

  void test_generateOutOfPlaneAngleWorkspace_will_create_a_workspace_with_the_expected_data_for_an_edge_case_dataset() {
    auto const sample = m_model->loadAndNormalise(m_ALFEdgeCaseData);
    m_model->setData(ALFData::SAMPLE, sample);
    m_model->generateLoadedWorkspace();

    m_singleTubeDetectorIDs = findWholeTubes(sample->componentInfo(), {2500u, 2501u, 2502u});
    TS_ASSERT(m_model->setSelectedTubes(m_singleTubeDetectorIDs));

    expectInstrumentActorCalls(12288u);

    auto [workspace, _] = m_model->generateOutOfPlaneAngleWorkspace(*m_instrumentActor);

    TS_ASSERT_EQUALS("Label", workspace->getAxis(0)->unit()->unitID());
    TS_ASSERT_EQUALS("Out of plane angle", std::string(workspace->getAxis(0)->unit()->label()));

    TS_ASSERT_DELTA(workspace->readX(0)[1], -20.318228, 0.000001);
    TS_ASSERT_DELTA(workspace->readX(0)[2], -20.242194, 0.000001);
    TS_ASSERT_DELTA(workspace->readY(0)[1], 0.001780, 0.000001);
    TS_ASSERT_DELTA(workspace->readY(0)[2], 0.001780, 0.000001);
  }

private:
  void setSingleTubeSelected() {
    m_model->setData(ALFData::SAMPLE, m_loadedWs);
    m_model->generateLoadedWorkspace();

    m_singleTubeDetectorIDs = findWholeTubes(m_loadedWs->componentInfo(), {2500u, 2501u, 2502u});

    TS_ASSERT(m_model->setSelectedTubes(m_singleTubeDetectorIDs));
  }

  void setMultipleTubesSelected() {
    m_model->setData(ALFData::SAMPLE, m_loadedWs);
    m_model->generateLoadedWorkspace();

    m_multiTubeDetectorIDs = findWholeTubes(m_loadedWs->componentInfo(), {2500u, 2501u, 2502u, 3500u, 3501u, 3502u});

    TS_ASSERT(m_model->setSelectedTubes(m_multiTubeDetectorIDs));
  }

  void expectInstrumentActorCalls(std::size_t const workspaceIndex = 0u) {
    auto const loadedWorkspace = ADS.retrieveWS<MatrixWorkspace>(m_model->loadedWsName());

    EXPECT_CALL(*m_instrumentActor, getWorkspace()).WillRepeatedly(Return(loadedWorkspace));
    EXPECT_CALL(*m_instrumentActor, componentInfo()).WillRepeatedly(ReturnRef(loadedWorkspace->componentInfo()));
    EXPECT_CALL(*m_instrumentActor, detectorInfo()).WillOnce(ReturnRef(loadedWorkspace->detectorInfo()));
    EXPECT_CALL(*m_instrumentActor, getInstrument()).WillOnce(Return(loadedWorkspace->getInstrument()));
    EXPECT_CALL(*m_instrumentActor, getWorkspaceIndex(_)).WillRepeatedly(Return(workspaceIndex));
    EXPECT_CALL(*m_instrumentActor, getBinMinMaxIndex(workspaceIndex, _, _))
        .WillRepeatedly(DoAll(SetArgReferee<1>(0u), SetArgReferee<2>(50u)));
  }

  std::string m_nonALFData;
  std::string m_ALFData;
  std::string m_ALFEdgeCaseData;

  Mantid::API::MatrixWorkspace_sptr m_loadedWs;

  std::vector<DetectorTube> m_singleTubeDetectorIDs;
  std::vector<DetectorTube> m_multiTubeDetectorIDs;

  std::unique_ptr<NiceMock<MockInstrumentActor>> m_instrumentActor;
  std::unique_ptr<ALFInstrumentModel> m_model;
};
