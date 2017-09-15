#ifndef MANTID_MDALGORITHMS_DISPLAYNORMALIZATIONSETTERTEST_H_
#define MANTID_MDALGORITHMS_DISPLAYNORMALIZATIONSETTERTEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidTestHelpers/WorkspaceCreationHelper.h"
#include "MantidTestHelpers/MDEventsTestHelper.h"
#include "MantidKernel/DeltaEMode.h"
#include "MantidMDAlgorithms/DisplayNormalizationSetter.h"
#include "boost/pointer_cast.hpp"
using namespace Mantid::MDAlgorithms;

class DisplayNormalizationSetterTest : public CxxTest::TestSuite {
public:
  void test_that_MDHistoWorkspace_throws_exception() {
    // Arrange
    auto isQ = true;
    auto eventWorkspace =
        WorkspaceCreationHelper::createEventWorkspace2(10, 10);
    auto mdHistoWorkspace =
        Mantid::DataObjects::MDEventsTestHelper::makeFakeMDHistoWorkspace(
            1.0, 1, 10);
    auto emode = Mantid::Kernel::DeltaEMode::Direct;
    Mantid::MDAlgorithms::DisplayNormalizationSetter setter;
    // Act + Assert
    TSM_ASSERT_THROWS("Should throw for MDHistoWorkspace",
                      setter(mdHistoWorkspace, eventWorkspace, isQ, emode),
                      std::runtime_error &);
  }

  void test_that_direct_energy_mode_creates_a_volume_normalization() {
    // Arrange
    auto isQ = true;
    auto eventWorkspace =
        WorkspaceCreationHelper::createEventWorkspace2(10, 10);
    auto mdEventWorkspace =
        Mantid::DataObjects::MDEventsTestHelper::makeMDEW<3>(4, 0.0, 4.0, 1);
    auto emode = Mantid::Kernel::DeltaEMode::Elastic;
    Mantid::MDAlgorithms::DisplayNormalizationSetter setter;
    // Act
    setter(mdEventWorkspace, eventWorkspace, isQ, emode);
    // Assert
    TSM_ASSERT_EQUALS("Should be set to volume normalization",
                      mdEventWorkspace->displayNormalization(),
                      Mantid::API::VolumeNormalization);
    TSM_ASSERT_EQUALS("Should be set to volume normalization",
                      mdEventWorkspace->displayNormalizationHisto(),
                      Mantid::API::VolumeNormalization);
  }

  void
  test_that_indirect_energy_mode_with_an_input_event_workspace_creates_no_normalization() {
    // Arrange
    auto isQ = true;
    auto eventWorkspace =
        WorkspaceCreationHelper::createEventWorkspace2(10, 10);
    auto mdEventWorkspace =
        Mantid::DataObjects::MDEventsTestHelper::makeMDEW<3>(4, 0.0, 4.0, 1);
    auto emode = Mantid::Kernel::DeltaEMode::Direct;
    Mantid::MDAlgorithms::DisplayNormalizationSetter setter;
    // Act
    setter(mdEventWorkspace, eventWorkspace, isQ, emode);
    // Assert
    TSM_ASSERT_EQUALS("Should be set to no normalization",
                      mdEventWorkspace->displayNormalization(),
                      Mantid::API::VolumeNormalization);
    TSM_ASSERT_EQUALS("Should be set to no normalization",
                      mdEventWorkspace->displayNormalizationHisto(),
                      Mantid::API::NoNormalization);
  }

  void
  test_that_indirect_energy_mode_with_input_workspace2D_creates_num_event_normalization() {
    // Arrange
    auto isQ = true;
    auto histoWorkspace = WorkspaceCreationHelper::create2DWorkspace123(2, 2);
    auto mdEventWorkspace =
        Mantid::DataObjects::MDEventsTestHelper::makeMDEW<3>(4, 0.0, 4.0, 1);
    auto emode = Mantid::Kernel::DeltaEMode::Direct;
    Mantid::MDAlgorithms::DisplayNormalizationSetter setter;
    // Act
    setter(mdEventWorkspace, histoWorkspace, isQ, emode);
    // Assert
    TSM_ASSERT_EQUALS("Should be set to number events normalization",
                      mdEventWorkspace->displayNormalization(),
                      Mantid::API::VolumeNormalization);
    TSM_ASSERT_EQUALS("Should be set to number events normalization",
                      mdEventWorkspace->displayNormalizationHisto(),
                      Mantid::API::NumEventsNormalization);
  }

  void test_that_non_Q_creates_volume_normalization() {
    // Arrange
    auto isQ = false;
    auto histoWorkspace = WorkspaceCreationHelper::create2DWorkspace123(2, 2);
    auto mdEventWorkspace =
        Mantid::DataObjects::MDEventsTestHelper::makeMDEW<3>(4, 0.0, 4.0, 1);
    auto emode = Mantid::Kernel::DeltaEMode::Direct;
    Mantid::MDAlgorithms::DisplayNormalizationSetter setter;
    // Act
    setter(mdEventWorkspace, histoWorkspace, isQ, emode);
    // Assert
    TSM_ASSERT_EQUALS("Should be set to number volume normalization",
                      mdEventWorkspace->displayNormalization(),
                      Mantid::API::VolumeNormalization);
    TSM_ASSERT_EQUALS("Should be set to number volume normalization",
                      mdEventWorkspace->displayNormalizationHisto(),
                      Mantid::API::VolumeNormalization);
  }
};

#endif
