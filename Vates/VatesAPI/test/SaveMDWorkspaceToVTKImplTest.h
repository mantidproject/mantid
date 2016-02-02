#ifndef VATES_API_SAVE_MD_WORKSPACE_TO_VTK_IMPL_TEST_H_
#define VATES_API_SAVE_MD_WORKSPACE_TO_VTK_IMPL_TEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidVatesAPI/SaveMDWorkspaceToVTKImpl.h"
#include "MantidVatesAPI/Normalization.h"
#include "MantidVatesAPI/IgnoreZerosThresholdRange.h"
#include "MantidTestHelpers/MDEventsTestHelper.h"

using namespace Mantid::DataObjects;

class SaveMDWorkspaceToVTKImplTest : public CxxTest::TestSuite
{
public:
  void test_that_vector_of_normalization_strings_has_all_values() {
    // Arrange
    Mantid::VATES::SaveMDWorkspaceToVTKImpl saveMDToVTK;

    // Act
    auto normalizations = saveMDToVTK.getAllowedNormalizationsInStringRepresentation();

    // Assert
    TSM_ASSERT_EQUALS("There should be 4 normalization options.", normalizations.size(), 4);
    TSM_ASSERT_EQUALS("First normalization should be AutoSelect.", normalizations[0], "AutoSelect");
    TSM_ASSERT_EQUALS("First normalization should be NoNormalization.", normalizations[1], "NoNormalization");
    TSM_ASSERT_EQUALS("First normalization should be NumEventsNormalization.", normalizations[2], "NumEventsNormalization");
    TSM_ASSERT_EQUALS("First normalization should be VolumeNormalization.", normalizations[3], "VolumeNormalization");
  }

  void test_string_representation_converts_to_visual_normalization() {
    // Arrange
    Mantid::VATES::SaveMDWorkspaceToVTKImpl saveMDToVTK;
    auto normalizations = saveMDToVTK.getAllowedNormalizationsInStringRepresentation();

    // Act
    auto autoSelect = saveMDToVTK.translateStringToVisualNormalization(normalizations[0]);
    auto noNormalization = saveMDToVTK.translateStringToVisualNormalization(normalizations[1]);
    auto numEventsNormalization = saveMDToVTK.translateStringToVisualNormalization(normalizations[2]);
    auto volumeNormalization = saveMDToVTK.translateStringToVisualNormalization(normalizations[3]);

    // Assert
    TSM_ASSERT_EQUALS("The visual normalization should be AutoSelect.", autoSelect, Mantid::VATES::AutoSelect);
    TSM_ASSERT_EQUALS("The visual normalization should be NoNormalization.", noNormalization, Mantid::VATES::NoNormalization);
    TSM_ASSERT_EQUALS("The visual normalization should be NumEventsNormalization.", numEventsNormalization, Mantid::VATES::NumEventsNormalization);
    TSM_ASSERT_EQUALS("The visual normalization should be VolumeNormalization.", volumeNormalization, Mantid::VATES::VolumeNormalization);
  }

  void test_that_vector_of_threshold_strings_has_all_values() {
    // Arrange
    Mantid::VATES::SaveMDWorkspaceToVTKImpl saveMDToVTK;

    // Act
    auto thresholds = saveMDToVTK.getAllowedThresholdsInStringRepresentation();

    // Assert
    TSM_ASSERT_EQUALS("There should be 2 normalization options", thresholds.size(), 2);
    TSM_ASSERT_EQUALS("First normalization should be IgnoreZerosThresholdRange.", thresholds[0], "IgnoreZerosThresholdRange");
    TSM_ASSERT_EQUALS("Second normalization should be NoThresholdRange.", thresholds[1], "NoThresholdRange");
  }

  void test_string_representation_converts_to_TresholdRange() {
    // Arrange
    Mantid::VATES::SaveMDWorkspaceToVTKImpl saveMDToVTK;
    auto thresholds = saveMDToVTK.getAllowedThresholdsInStringRepresentation();
    // Act
    auto ignoreZerosThresholdRange = saveMDToVTK.translateStringToThresholdRange(thresholds[0]);
    auto noThresholdRange = saveMDToVTK.translateStringToThresholdRange(thresholds[1]);
    // Assert
    TSM_ASSERT("Should be a IgnoreZerosTresholdRange", boost::dynamic_pointer_cast<Mantid::VATES::IgnoreZerosThresholdRange>(ignoreZerosThresholdRange));
    TSM_ASSERT("Should be a NoTresholdRange", boost::dynamic_pointer_cast<Mantid::VATES::ThresholdRange>(noThresholdRange));
  }

  void test_detects_when_4D_workspace() {
    // Arrange
    Mantid::VATES::SaveMDWorkspaceToVTKImpl saveMDToVTK;
    size_t numDims = 4;
    auto workspace = MDEventsTestHelper::makeFakeMDHistoWorkspace(1.0, numDims);

    // Act
    auto is4D = saveMDToVTK.is4DWorkspace(workspace);

    // Assert
    TSM_ASSERT("Detects a 4D MD workspace", is4D);
  }

  void test_detects_when_not_4D_workspace() {
    // Arrange
    Mantid::VATES::SaveMDWorkspaceToVTKImpl saveMDToVTK;
    size_t numDims = 2;
    auto workspace = MDEventsTestHelper::makeFakeMDHistoWorkspace(1.0, numDims);

    // Act
    auto is4D = saveMDToVTK.is4DWorkspace(workspace);

    // Assert
    TSM_ASSERT("Detects that not a 4D MD workspace", !is4D);
  }

  void test_that_saves_MD_Histo_workspace_to_vts_file() {
    // Arrange
    //Mantid::VATES::SaveMDWorkspaceToVTKImpl saveMDToVTK;

    // TODO: FINISH with fake data here, also write a system test with real data

    // Act

    // Assert

  }
};
#endif
