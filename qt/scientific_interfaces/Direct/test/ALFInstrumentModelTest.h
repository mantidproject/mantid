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

#include <memory>
#include <string>

using namespace Mantid::API;
using namespace Mantid::Geometry;
using namespace Mantid::Kernel;
using namespace MantidQt::CustomInterfaces;
using namespace testing;

namespace {
auto &ADS = AnalysisDataService::Instance();

void addCurvesWorkspaceToADS(std::string const &unit, std::string const &label = "", double const yValue = 0.2) {
  auto workspace = WorkspaceCreationHelper::create2DWorkspaceWithValuesAndXerror(1, 10, false, 0.1, yValue, 0.01, 0.3);

  std::shared_ptr<Instrument> instrument = std::make_shared<Instrument>();
  instrument->setName("ALF");
  workspace->setInstrument(instrument);

  workspace->getAxis(0)->setUnit(unit);

  if (!label.empty()) {
    auto xUnit = std::dynamic_pointer_cast<Units::Label>(UnitFactory::Instance().create("Label"));
    xUnit->setLabel("Label", label);
    workspace->getAxis(0)->unit() = xUnit;
  }

  ADS.addOrReplace("Curves", workspace);
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

    m_model = std::make_unique<ALFInstrumentModel>();
  }

  void tearDown() override { ADS.clear(); }

  void test_instrumentName_returns_the_expected_instrument() { TS_ASSERT_EQUALS("ALF", m_model->instrumentName()); }

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

  void test_extractedWsName_returns_a_workspace_name_ending_in_zero_if_the_extracted_workspace_does_not_exist() {
    TS_ASSERT_EQUALS("extractedTubes_ALF0", m_model->extractedWsName());
  }

  void test_extractedWsName_returns_the_expected_extracted_workspace_name() {
    m_model->loadAndTransform(m_ALFData);

    addCurvesWorkspaceToADS("dSpacing");

    TS_ASSERT_EQUALS("extractedTubes_ALF82301", m_model->extractedWsName());
  }

  void test_extractSingleTube_will_not_change_the_number_of_tubes_if_a_single_tube_workspace_cannot_be_found() {
    m_model->extractSingleTube();

    TS_ASSERT(!ADS.doesExist(m_model->extractedWsName()));
    TS_ASSERT_EQUALS(0u, m_model->numberOfTubesInAverage());
  }

  void test_extractSingleTube_will_set_the_number_of_tubes_to_one_if_a_single_tube_workspace_exists() {
    m_model->loadAndTransform(m_ALFData);
    addCurvesWorkspaceToADS("dSpacing");

    m_model->extractSingleTube();

    TS_ASSERT(ADS.doesExist(m_model->extractedWsName()));
    TS_ASSERT_EQUALS(1u, m_model->numberOfTubesInAverage());
  }

  void test_extractSingleTube_will_create_a_workspace_with_the_expected_dSpacing_units_and_y_values() {
    m_model->loadAndTransform(m_ALFData);
    addCurvesWorkspaceToADS("dSpacing");

    m_model->extractSingleTube();

    auto workspace = ADS.retrieveWS<MatrixWorkspace>(m_model->extractedWsName());

    TS_ASSERT_EQUALS("dSpacing", workspace->getAxis(0)->unit()->unitID());
    TS_ASSERT_DELTA(workspace->readX(0)[1], 0.6, 0.000001);
    TS_ASSERT_DELTA(workspace->readX(0)[2], 1.6, 0.000001);
    TS_ASSERT_DELTA(workspace->readY(0)[1], 0.2, 0.000001);
    TS_ASSERT_DELTA(workspace->readY(0)[2], 0.2, 0.000001);
  }

  void test_extractSingleTube_will_create_a_workspace_with_the_expected_phi_units_and_y_values() {
    m_model->loadAndTransform(m_ALFData);
    addCurvesWorkspaceToADS("Phi");

    m_model->extractSingleTube();

    auto workspace = ADS.retrieveWS<MatrixWorkspace>(m_model->extractedWsName());

    TS_ASSERT_EQUALS("Phi", workspace->getAxis(0)->unit()->unitID());
    TS_ASSERT_DELTA(workspace->readX(0)[1], 0.6 * 180.0 / M_PI, 0.000001);
    TS_ASSERT_DELTA(workspace->readX(0)[2], 1.6 * 180.0 / M_PI, 0.000001);
    TS_ASSERT_DELTA(workspace->readY(0)[1], 0.2, 0.000001);
    TS_ASSERT_DELTA(workspace->readY(0)[2], 0.2, 0.000001);
  }

  void test_extractSingleTube_will_create_a_workspace_with_the_expected_out_of_plane_angle_label_and_y_values() {
    m_model->loadAndTransform(m_ALFData);
    addCurvesWorkspaceToADS("Degrees", "Out of plane angle");

    m_model->extractSingleTube();

    auto workspace = ADS.retrieveWS<MatrixWorkspace>(m_model->extractedWsName());

    TS_ASSERT_EQUALS("Label", workspace->getAxis(0)->unit()->unitID());
    TS_ASSERT_DELTA(workspace->readX(0)[1], 0.6 * 180.0 / M_PI, 0.000001);
    TS_ASSERT_DELTA(workspace->readX(0)[2], 1.6 * 180.0 / M_PI, 0.000001);
    TS_ASSERT_DELTA(workspace->readY(0)[1], 0.2, 0.000001);
    TS_ASSERT_DELTA(workspace->readY(0)[2], 0.2, 0.000001);
  }

  void test_averageTube_increments_the_number_of_averaged_tubes() {
    m_model->loadAndTransform(m_ALFData);
    addCurvesWorkspaceToADS("Degrees", "Out of plane angle");

    m_model->extractSingleTube();
    addCurvesWorkspaceToADS("Degrees", "Out of plane angle");

    m_model->averageTube();

    TS_ASSERT(!ADS.doesExist("Curves"));
    TS_ASSERT(ADS.doesExist(m_model->extractedWsName()));
    TS_ASSERT_EQUALS(2u, m_model->numberOfTubesInAverage());
  }

  void test_averageTube_creates_an_average_with_the_existing_extracted_workspace() {
    m_model->loadAndTransform(m_ALFData);
    addCurvesWorkspaceToADS("Degrees", "Out of plane angle", 0.2);

    m_model->extractSingleTube();
    addCurvesWorkspaceToADS("Degrees", "Out of plane angle", 0.4);

    m_model->averageTube();

    auto workspace = ADS.retrieveWS<MatrixWorkspace>(m_model->extractedWsName());
    TS_ASSERT_EQUALS("Label", workspace->getAxis(0)->unit()->unitID());
    TS_ASSERT_DELTA(workspace->readX(0)[1], 0.6 * 180.0 / M_PI, 0.000001);
    TS_ASSERT_DELTA(workspace->readX(0)[2], 1.6 * 180.0 / M_PI, 0.000001);
    TS_ASSERT_DELTA(workspace->readY(0)[1], 0.3, 0.000001);
    TS_ASSERT_DELTA(workspace->readY(0)[2], 0.3, 0.000001);
  }

  void test_checkDataIsExtracted_returns_false_if_number_of_tubes_is_zero() {
    ADS.addOrReplace(m_model->extractedWsName(), WorkspaceCreationHelper::create2DWorkspace(1, 2));
    TS_ASSERT(!m_model->checkDataIsExtracted());
  }

  void test_checkDataIsExtracted_returns_false_if_extracted_workspace_does_not_exist() {
    m_model->loadAndTransform(m_ALFData);
    addCurvesWorkspaceToADS("Degrees", "Out of plane angle", 0.2);

    m_model->extractSingleTube();
    ADS.remove(m_model->extractedWsName());

    TS_ASSERT(!m_model->checkDataIsExtracted());
  }

  void test_checkDataIsExtracted_returns_true_if_a_tube_has_already_been_extracted() {
    m_model->loadAndTransform(m_ALFData);
    addCurvesWorkspaceToADS("Degrees", "Out of plane angle", 0.2);

    m_model->extractSingleTube();

    TS_ASSERT(m_model->checkDataIsExtracted());
  }

private:
  std::string m_nonALFData;
  std::string m_ALFData;

  std::unique_ptr<ALFInstrumentModel> m_model;
};
