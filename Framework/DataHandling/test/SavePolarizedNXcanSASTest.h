// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2025 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <cxxtest/TestSuite.h>

#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidDataHandling/SaveNXcanSASHelper.h"
#include "MantidDataHandling/SavePolarizedNXcanSAS.h"
#include "MantidFrameworkTestHelpers/WorkspaceCreationHelper.h"
#include "NXcanSASFileTest.h"
#include "NXcanSASTestHelper.h"

#include <H5Cpp.h>
#include <filesystem>

using Mantid::DataHandling::SavePolarizedNXcanSAS;
using namespace Mantid::DataHandling::NXcanSAS;
using namespace NXcanSASTestHelper;

class SavePolarizedNXcanSASTest : public CxxTest::TestSuite, public NXcanSASFileTest {
public:
  SavePolarizedNXcanSASTest() : m_ads(Mantid::API::AnalysisDataService::Instance()), m_parameters() {}

  // This pair of boilerplate methods prevent the suite being created
  // statically
  // This means the constructor isn't called when running other tests
  static SavePolarizedNXcanSASTest *createSuite() { return new SavePolarizedNXcanSASTest(); }
  static void destroySuite(SavePolarizedNXcanSASTest *suite) { delete suite; }
  void setUp() override {
    m_parameters = NXcanSASTestParameters();
    setPolarizedParameters(m_parameters);
  }
  void tearDown() override {
    m_ads.clear();
    removeFile(m_parameters.filename);
  }

  void test_algorithm_saves_with_no_issue_for_1D_test_data_full_polarization() {
    const auto groupWS = providePolarizedGroup(m_ads, m_parameters);
    const auto savePolAlg = prepareSaveAlg(groupWS);
    assertSavedFileFormat(savePolAlg);
  }

  void test_save_algorithm_throws_for_matrix_workspaces() {
    const auto ws = provide1DWorkspace(m_parameters);

    const auto saveAlg = Mantid::API::AlgorithmManager::Instance().createUnmanaged("SavePolarizedNXcanSAS");
    saveAlg->initialize();
    saveAlg->setProperty("Filename", m_parameters.filename);
    saveAlg->setProperty("InputWorkspace", ws);

    TSM_ASSERT_THROWS("Incompatible workspace", saveAlg->execute(), const std::runtime_error &);
  }

  void test_group_ws_throws_with_groups_different_than_4_or_2_members() {
    const auto groupWS = providePolarizedGroup(m_ads, m_parameters);

    m_ads.removeFromGroup("GroupPol", "group_0");
    const auto savePolAlg = prepareSaveAlg(groupWS);

    TSM_ASSERT_THROWS("Incompatible group workspace", savePolAlg->execute(), const std::runtime_error &);
  }

  void test_full_spin_polarized_data_cant_contain_zero_spin() {
    const auto groupWS = providePolarizedGroup(m_ads, m_parameters);

    m_parameters.inputSpinStates = "0+1,+1+1,-1+1,-1-1";
    const auto savePolAlg = prepareSaveAlg(groupWS);

    TSM_ASSERT_THROWS("Incompatible input spin string", savePolAlg->execute(), const std::runtime_error &);
  }

  void test_save_algorithm_does_not_throw_for_wrong_component_name() {
    const auto groupWS = providePolarizedGroup(m_ads, m_parameters);

    m_parameters.polarizerComponent.compName = m_parameters.wrongComponentName;
    const auto savePolAlg = prepareSaveAlg(groupWS);

    assertSavedFileFormat(savePolAlg);
  }

  void test_save_algorithm_is_saved_correctly_for_multiple_components() {
    const auto groupWS = providePolarizedGroup(m_ads, m_parameters);

    // There can be multiple flippers
    m_parameters.flipperComponent.compName = "test-flipper1, test-flipper2";
    const auto savePolAlg = prepareSaveAlg(groupWS);

    assertSavedFileFormat(savePolAlg);
  }

  void test_magnetic_field_strength_is_saved_correctly_from_logs() {
    const auto groupWS = providePolarizedGroup(m_ads, m_parameters);
    const auto ws0 = std::dynamic_pointer_cast<Mantid::API::MatrixWorkspace>(groupWS->getItem(0));

    m_parameters.magneticFieldStrengthLogName = "MagField";
    add_sample_log(ws0, m_parameters.magneticFieldStrengthLogName, std::to_string(m_parameters.magneticFieldStrength),
                   m_parameters.magneticFieldUnit);
    const auto savePolAlg = prepareSaveAlg(groupWS);

    assertSavedFileFormat(savePolAlg);
  }

  void test_wrong_magnetic_field_directions_throw() {
    const auto groupWS = providePolarizedGroup(m_ads, m_parameters);

    m_parameters.magneticFieldDirection = "1,2,a";
    const auto savePolAlg = prepareSaveAlg(groupWS);

    TSM_ASSERT_THROWS("Incompatible magnetic field direction magnitude", savePolAlg->execute(),
                      const std::runtime_error &);

    // not 3D vector
    m_parameters.magneticFieldDirection = "1,2,3,4";
    savePolAlg->setProperty("MagneticFieldDirection", m_parameters.magneticFieldDirection);
    TSM_ASSERT_THROWS("Incompatible magnetic field direction vector", savePolAlg->execute(),
                      const std::runtime_error &);
  }

  void test_magnetic_field_direction_is_saved_correctly() {
    const auto groupWS = providePolarizedGroup(m_ads, m_parameters);

    m_parameters.magneticFieldDirection = "1,2,3";
    const auto savePolAlg = prepareSaveAlg(groupWS);

    assertSavedFileFormat(savePolAlg);
  }

  void test_algorithm_saves_with_no_issue_for_test_data_half_polarized_1D_and_2D() {
    // only analyzer and  only polarizer data
    const std::vector<std::string> states = {"0+1,0-1", "+10,-10"};
    const std::vector<bool> is2D = {true, false};
    m_parameters.hasDx = false;
    m_parameters.polWorkspaceNumber = 2;

    for (auto const &dim : is2D) {
      m_parameters.is2dData = dim;
      auto groupWS = providePolarizedGroup(m_ads, m_parameters);

      for (auto const &spinState : states) {
        m_parameters.inputSpinStates = spinState;
        auto savePolAlg = prepareSaveAlg(groupWS);

        assertSavedFileFormat(savePolAlg);

        // clean
        removeFile(m_parameters.filename);
        m_ads.clear();
      }
    }
  }

  void test_algorithm_saves_with_no_issue_for_2D_test_data_full_polarized() {
    m_parameters.is2dData = true;
    m_parameters.hasDx = false;
    const auto groupWS = providePolarizedGroup(m_ads, m_parameters);

    const auto savePolAlg = prepareSaveAlg(groupWS);

    assertSavedFileFormat(savePolAlg);
  }

  void test_algorithm_saves_with_same_spin_order_for_different_input_order() {
    m_parameters.is2dData = true;
    m_parameters.hasDx = false;
    const std::vector<double> defaultReference = {1, 2, 3, 4};
    const std::map<std::string, std::vector<double>> states = {{"+1+1, -1+1, +1-1, -1-1", {4, 2, 3, 1}},
                                                               {"-1+1, -1-1, +1+1, +1-1", {2, 1, 4, 3}}};

    for (auto const &[state, reference] : states) {
      m_parameters.inputSpinStates = state;
      m_parameters.referenceValues = reference;
      auto groupWS = providePolarizedGroup(m_ads, m_parameters);

      auto savePolAlg = prepareSaveAlg(groupWS);
      m_parameters.referenceValues = defaultReference;
      assertSavedFileFormat(savePolAlg);

      // clean
      m_ads.clear();
      removeFile(m_parameters.filename);
    }
  }

private:
  Mantid::API::AnalysisDataServiceImpl &m_ads;
  NXcanSASTestParameters m_parameters;
  Mantid::API::Algorithm_sptr prepareSaveAlg(const Mantid::API::WorkspaceGroup_sptr &workspace) {
    auto saveAlg = Mantid::API::AlgorithmManager::Instance().createUnmanaged("SavePolarizedNXcanSAS");
    saveAlg->initialize();

    saveAlg->setProperty("Filename", m_parameters.filename);
    saveAlg->setProperty("InputWorkspace", workspace);

    // Standard Metadata
    saveAlg->setProperty("RadiationSource", m_parameters.radiationSource);
    saveAlg->setProperty("Geometry", m_parameters.geometry);
    saveAlg->setProperty("SampleHeight", m_parameters.beamHeight);
    saveAlg->setProperty("SampleWidth", m_parameters.beamWidth);
    if (!m_parameters.detectors.empty()) {
      std::string detectorsAsString = concatenateStringVector(m_parameters.detectors);
      saveAlg->setProperty("DetectorNames", detectorsAsString);
    }
    saveAlg->setProperty("SampleThickness", m_parameters.sampleThickness);

    // Polarized Metadata
    saveAlg->setProperty("InputSpinStates", m_parameters.inputSpinStates);
    saveAlg->setProperty("PolarizerComponentName", m_parameters.polarizerComponent.compName);
    saveAlg->setProperty("AnalyzerComponentName", m_parameters.analyzerComponent.compName);
    saveAlg->setProperty("FlipperComponentNames", m_parameters.flipperComponent.compName);
    saveAlg->setProperty("MagneticFieldStrengthLogName", m_parameters.magneticFieldStrengthLogName);
    saveAlg->setProperty("MagneticFieldDirection", m_parameters.magneticFieldDirection);

    return saveAlg;
  }

  void assertSavedFileFormat(const Mantid::API::Algorithm_sptr &algorithm) {
    TSM_ASSERT_THROWS_NOTHING("Should not throw anything", algorithm->execute());
    TSM_ASSERT("Should have executed", algorithm->isExecuted());
    do_assert(m_parameters);
  }
};
