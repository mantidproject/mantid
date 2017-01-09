#ifndef VATES_API_SAVE_MD_WORKSPACE_TO_VTK_IMPL_TEST_H_
#define VATES_API_SAVE_MD_WORKSPACE_TO_VTK_IMPL_TEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidVatesAPI/SaveMDWorkspaceToVTKImpl.h"
#include "MantidVatesAPI/Normalization.h"
#include "MantidTestHelpers/MDEventsTestHelper.h"
#include "MantidKernel/ConfigService.h"

#include <Poco/File.h>

using namespace Mantid::DataObjects;

class SaveMDWorkspaceToVTKImplTest : public CxxTest::TestSuite {
public:
  void test_that_vector_of_normalization_strings_has_all_values() {
    // Arrange
    Mantid::VATES::SaveMDWorkspaceToVTKImpl saveMDToVTK;

    // Act
    const auto normalizations =
        saveMDToVTK.getAllowedNormalizationsInStringRepresentation();

    // Assert
    TSM_ASSERT_EQUALS("There should be 4 normalization options.",
                      normalizations.size(), 4);
    TSM_ASSERT_EQUALS("First normalization should be AutoSelect.",
                      normalizations[0], "AutoSelect");
    TSM_ASSERT_EQUALS("First normalization should be NoNormalization.",
                      normalizations[1], "NoNormalization");
    TSM_ASSERT_EQUALS("First normalization should be NumEventsNormalization.",
                      normalizations[2], "NumEventsNormalization");
    TSM_ASSERT_EQUALS("First normalization should be VolumeNormalization.",
                      normalizations[3], "VolumeNormalization");
  }

  void test_string_representation_converts_to_visual_normalization() {
    // Arrange
    Mantid::VATES::SaveMDWorkspaceToVTKImpl saveMDToVTK;
    const auto normalizations =
        saveMDToVTK.getAllowedNormalizationsInStringRepresentation();

    // Act
    const auto autoSelect =
        saveMDToVTK.translateStringToVisualNormalization(normalizations[0]);
    const auto noNormalization =
        saveMDToVTK.translateStringToVisualNormalization(normalizations[1]);
    const auto numEventsNormalization =
        saveMDToVTK.translateStringToVisualNormalization(normalizations[2]);
    const auto volumeNormalization =
        saveMDToVTK.translateStringToVisualNormalization(normalizations[3]);

    // Assert
    TSM_ASSERT_EQUALS("The visual normalization should be AutoSelect.",
                      autoSelect, Mantid::VATES::AutoSelect);
    TSM_ASSERT_EQUALS("The visual normalization should be NoNormalization.",
                      noNormalization, Mantid::VATES::NoNormalization);
    TSM_ASSERT_EQUALS(
        "The visual normalization should be NumEventsNormalization.",
        numEventsNormalization, Mantid::VATES::NumEventsNormalization);
    TSM_ASSERT_EQUALS("The visual normalization should be VolumeNormalization.",
                      volumeNormalization, Mantid::VATES::VolumeNormalization);
  }

  void test_detects_when_not_3D_workspace() {
    // Arrange
    Mantid::VATES::SaveMDWorkspaceToVTKImpl saveMDToVTK;
    size_t numDims = 4;
    auto workspace = MDEventsTestHelper::makeFakeMDHistoWorkspace(1.0, numDims);

    // Act
    const auto is3D = saveMDToVTK.is3DWorkspace(*workspace);

    // Assert
    TSM_ASSERT("Detects a non-3D MD workspace", !is3D);
  }

  void test_detects_when_3D_workspace() {
    // Arrange
    Mantid::VATES::SaveMDWorkspaceToVTKImpl saveMDToVTK;
    const size_t numDims = 3;
    auto workspace = MDEventsTestHelper::makeFakeMDHistoWorkspace(1.0, numDims);

    // Act
    const auto is3D = saveMDToVTK.is3DWorkspace(*workspace);

    // Assert
    TSM_ASSERT("Detects that a 3D MD workspace", is3D);
  }

  void
  test_that_saves_MD_Event_workspace_to_vts_file_without_extension_in_path_name() {
    // Arrange
    auto workspace = getTestWorkspace("MDEvent");

    const std::string filenameBare = "SaveMDWorkspaceToVTKImplTestFile";
    const std::string filenameWithExtension = filenameBare + ".vtu";

    auto filenameExpected = getTemporaryFilename(filenameWithExtension);
    removeTemporaryFile(filenameExpected);

    auto filename = getTemporaryFilename(filenameBare);

    // Act
    do_test_saving_to_vtk_file(workspace, filename);

    // Assert
    verify_file_creation(filenameExpected);
  }

  void
  test_that_saves_MD_Event_workspace_to_vts_file_with_extension_in_path_name() {
    // Arrange
    auto workspace = getTestWorkspace("MDEvent");

    const std::string filename = "SaveMDWorkspaceToVTKImplTestFile.vtu";
    removeTemporaryFile(filename);

    // Act
    do_test_saving_to_vtk_file(workspace, filename);

    // Assert
    verify_file_creation(filename);
  }

  void
  test_that_saves_MD_Histo_workspace_to_vts_file_without_extension_in_path_name() {
    // Arrange
    auto workspace = getTestWorkspace("MDHisto");

    const std::string filenameBare = "SaveMDWorkspaceToVTKImplTestFile";
    const std::string filenameWithExtension = filenameBare + ".vts";

    auto filenameExpected = getTemporaryFilename(filenameWithExtension);
    removeTemporaryFile(filenameExpected);

    auto filename = getTemporaryFilename(filenameBare);

    // Act
    do_test_saving_to_vtk_file(workspace, filename);

    // Assert
    verify_file_creation(filenameExpected);
  }

  void
  test_that_saves_MD_Histo_workspace_to_vts_file_with_extension_in_path_name() {
    // Arrange
    auto workspace = getTestWorkspace("MDHisto");

    std::string filename = "SaveMDWorkspaceToVTKImplTestFile.vts";
    filename = getTemporaryFilename(filename);
    removeTemporaryFile(filename);

    // Act
    do_test_saving_to_vtk_file(workspace, filename);

    // Assert
    verify_file_creation(filename);
  }

private:
  void do_test_saving_to_vtk_file(Mantid::API::IMDWorkspace_sptr workspace,
                                  std::string filename) {
    const int recursionDepth = 5;

    Mantid::VATES::SaveMDWorkspaceToVTKImpl saveMDToVTK;

    const auto normalizations =
        saveMDToVTK.getAllowedNormalizationsInStringRepresentation();
    const auto normalization =
        saveMDToVTK.translateStringToVisualNormalization(normalizations[0]);
    saveMDToVTK.saveMDWorkspace(workspace, filename, normalization,
                                recursionDepth, "NONE");
  }

  Mantid::API::IMDWorkspace_sptr getTestWorkspace(std::string workspaceType) {
    Mantid::API::IMDWorkspace_sptr workspace;
    if (workspaceType == "MDEvent") {
      const std::string name = "SaveMDEventToVTKTestWorkspace";
      workspace = MDEventsTestHelper::makeFakeMDEventWorkspace(name);
    } else {
      const size_t numDims = 3;
      const size_t numBins = 5;
      workspace =
          MDEventsTestHelper::makeFakeMDHistoWorkspace(1.0, numDims, numBins);
    }
    return workspace;
  }

  void verify_file_creation(std::string filename) {
    // Assert
    auto fileExists = doesFileExist(filename);
    TSM_ASSERT("The according file should have been saved out", fileExists);
    // Cleanup
    removeTemporaryFile(filename);
  }

  std::string getTemporaryFilename(std::string filenameWithoutPath) const {
    auto default_save_directory =
        Mantid::Kernel::ConfigService::Instance().getString(
            "defaultsave.directory");
    std::string filenameWithPath(default_save_directory + filenameWithoutPath);
    return filenameWithPath;
  }

  void removeTemporaryFile(std::string fileNameWithPath) const {
    if (Poco::File(fileNameWithPath).exists()) {
      Poco::File(fileNameWithPath).remove();
    }
  }

  bool doesFileExist(std::string filename) {
    return Poco::File(filename).exists();
  }
};
#endif
