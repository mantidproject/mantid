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

} // namespace

class ALFInstrumentModelTest : public CxxTest::TestSuite {
public:
  ALFInstrumentModelTest() { FrameworkManager::Instance(); }

  static ALFInstrumentModelTest *createSuite() { return new ALFInstrumentModelTest(); }

  static void destroySuite(ALFInstrumentModelTest *suite) { delete suite; }

  void setUp() override {
    m_nonALFData = "IRIS00072464.raw";
    m_ALFData = "ALF82301.raw";
    m_ALFEdgeCaseData = "ALF83743.raw";

    m_singleTubeDetectorIDs = std::vector<DetectorTube>{{2500u, 2501u, 2502u}};
    m_multiTubeDetectorIDs = std::vector<DetectorTube>{{2500u, 2501u, 2502u}, {3500u, 3501u, 3502u}};

    m_instrumentActor = std::make_unique<NiceMock<MockInstrumentActor>>();
    m_model = std::make_unique<ALFInstrumentModel>();
  }

  void tearDown() override { ADS.clear(); }

  void test_loadedWsName_returns_the_expected_instrument() { TS_ASSERT_EQUALS("ALFData", m_model->loadedWsName()); }

  void test_loadAndTransform_returns_an_error_if_the_data_is_not_ALF_data() {
    auto const message = m_model->loadAndTransform(m_nonALFData);

    TS_ASSERT(message);
    TS_ASSERT_EQUALS("The loaded data is not from the ALF instrument", *message);
  }

  void test_loadAndTransform_returns_a_nullopt_if_the_data_is_valid() {
    auto const message = m_model->loadAndTransform(m_ALFData);
    TS_ASSERT(!message);
  }

  void test_loadAndTransform_transforms_the_data_to_dSpacing_if_not_already() {
    auto const message = m_model->loadAndTransform(m_ALFData);

    TS_ASSERT(ADS.doesExist(m_model->loadedWsName()));

    auto workspace = ADS.retrieveWS<MatrixWorkspace>(m_model->loadedWsName());
    TS_ASSERT_EQUALS("dSpacing", workspace->getAxis(0)->unit()->unitID());
  }

  void test_runNumber_returns_zero_when_no_data_is_loaded() { TS_ASSERT_EQUALS(0u, m_model->runNumber()); }

  void test_runNumber_returns_the_run_number_of_the_loaded_data() {
    m_model->loadAndTransform(m_ALFData);
    TS_ASSERT_EQUALS(82301u, m_model->runNumber());
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
    m_model->loadAndTransform(m_ALFEdgeCaseData);

    auto const ws = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(m_model->loadedWsName());
    m_singleTubeDetectorIDs = findWholeTubes(ws->componentInfo(), {2500u, 2501u, 2502u});
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
    m_model->loadAndTransform(m_ALFData);

    auto const workspace = ADS.retrieveWS<MatrixWorkspace>(m_model->loadedWsName());
    m_singleTubeDetectorIDs = findWholeTubes(workspace->componentInfo(), {2500u, 2501u, 2502u});

    TS_ASSERT(m_model->setSelectedTubes(m_singleTubeDetectorIDs));
  }

  void setMultipleTubesSelected() {
    m_model->loadAndTransform(m_ALFData);

    auto const workspace = ADS.retrieveWS<MatrixWorkspace>(m_model->loadedWsName());
    m_multiTubeDetectorIDs = findWholeTubes(workspace->componentInfo(), {2500u, 2501u, 2502u, 3500u, 3501u, 3502u});

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

  std::vector<DetectorTube> m_singleTubeDetectorIDs;
  std::vector<DetectorTube> m_multiTubeDetectorIDs;

  std::unique_ptr<NiceMock<MockInstrumentActor>> m_instrumentActor;
  std::unique_ptr<ALFInstrumentModel> m_model;
};
