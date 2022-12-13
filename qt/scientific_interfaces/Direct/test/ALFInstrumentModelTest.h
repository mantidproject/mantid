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

V3D const SAMPLE_POSITION = V3D(0.0, 0.0, 0.0);
V3D const SOURCE_POSITION = V3D(0.0, 0.0, -25.0);

MatrixWorkspace_sptr generateExtractedWorkspace(std::string const &unit, std::string const &label = "",
                                                double const yValue = 0.2) {
  auto workspace = WorkspaceCreationHelper::create2DWorkspaceWithValuesAndXerror(1, 10, false, 0.1, yValue, 0.01, 0.3);

  std::shared_ptr<Instrument> instrument = std::make_shared<Instrument>();
  instrument->setName("ALF");

  // Set source position
  auto *source = new ObjComponent("source");
  source->setPos(SOURCE_POSITION);
  instrument->add(source);
  instrument->markAsSource(source);

  // Set sample position
  auto *sampleHolder = new Component("samplePos");
  sampleHolder->setPos(SAMPLE_POSITION);
  instrument->add(sampleHolder);
  instrument->markAsSamplePos(sampleHolder);

  workspace->setInstrument(instrument);

  workspace->getAxis(0)->setUnit(unit);

  if (!label.empty()) {
    auto xUnit = std::dynamic_pointer_cast<Units::Label>(UnitFactory::Instance().create("Label"));
    xUnit->setLabel("Label", label);
    workspace->getAxis(0)->unit() = xUnit;
  }

  return workspace;
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

    m_singleTubeDetectorIDs = std::vector<std::size_t>{2500u, 2501u, 2502u};
    m_multiTubeDetectorIDs = std::vector<std::size_t>{2500u, 2501u, 2502u, 3500u, 3501u, 3502u};

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

  void test_setSelectedDetectors_will_set_an_empty_vector_of_detector_ids_when_provided_an_empty_vector() {
    auto workspace = generateExtractedWorkspace("dSpacing");
    auto &compInfo = workspace->componentInfo();
    m_model->setSelectedDetectors(compInfo, {});

    TS_ASSERT(m_model->selectedDetectors().empty());
  }

  void test_setSelectedDetectors_will_select_the_detectors_in_an_entire_tube() {
    setSingleTubeSelected();
    TS_ASSERT_EQUALS(512u, m_model->selectedDetectors().size());
  }

  void test_setSelectedDetectors_will_select_the_detectors_in_two_entire_tubes() {
    setMultipleTubesSelected();
    TS_ASSERT_EQUALS(1024u, m_model->selectedDetectors().size());
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
    m_model->setSelectedDetectors(ws->componentInfo(), m_singleTubeDetectorIDs);

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

    auto const workspace = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(m_model->loadedWsName());
    m_model->setSelectedDetectors(workspace->componentInfo(), m_singleTubeDetectorIDs);
  }

  void setMultipleTubesSelected() {
    m_model->loadAndTransform(m_ALFData);

    auto const workspace = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(m_model->loadedWsName());
    m_model->setSelectedDetectors(workspace->componentInfo(), m_multiTubeDetectorIDs);
  }

  void expectInstrumentActorCalls(std::size_t const workspaceIndex = 0u) {
    auto const loadedWorkspace = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(m_model->loadedWsName());

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

  std::vector<std::size_t> m_singleTubeDetectorIDs;
  std::vector<std::size_t> m_multiTubeDetectorIDs;

  std::unique_ptr<NiceMock<MockInstrumentActor>> m_instrumentActor;
  std::unique_ptr<ALFInstrumentModel> m_model;
};
