#ifndef VATES_API_SAVE_MD_WORKSPACE_TO_VTK_TEST_H_
#define VATES_API_SAVE_MD_WORKSPACE_TO_VTK_TEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidVatesAPI/SaveMDWorkspaceToVTK.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"
#include "MantidTestHelpers/MDEventsTestHelper.h"
#include <Poco/File.h>

class SaveMDWorkspaceToVTKTest : public CxxTest::TestSuite {
public:
  void test_that_wrong_workspace_type_throws() {
    // Arrange
    auto workspace = WorkspaceCreationHelper::create2DWorkspace(1, 10);

    Mantid::VATES::SaveMDWorkspaceToVTK alg;
    alg.setChild(true);
    alg.setRethrows(true);
    alg.initialize();
    alg.setProperty("InputWorkspace", workspace);
    alg.setProperty("Filename", "test_file_name");
    alg.setProperty("Normalization", "AutoSelect");
    alg.setProperty("RecursionDepth", 5);
    alg.setProperty("CompressorType", "NONE");

    // Act + Assert
    TSM_ASSERT_THROWS_ANYTHING(
        "Wrong workspace type should cause the algorithm to throw",
        alg.execute());
  }

  void test_that_non_3D_workspace_throws() {
    // Arrange
    const size_t numDims = 4;
    const size_t numBins = 5;
    auto workspace =
        Mantid::DataObjects::MDEventsTestHelper::makeFakeMDHistoWorkspace(
            1.0, numDims, numBins);

    Mantid::VATES::SaveMDWorkspaceToVTK alg;
    alg.setChild(true);
    alg.setRethrows(true);
    alg.initialize();
    alg.setProperty("InputWorkspace", workspace);
    alg.setProperty("Filename", "test_file_name");
    alg.setProperty("Normalization", "AutoSelect");
    alg.setProperty("RecursionDepth", 5);
    alg.setProperty("CompressorType", "NONE");

    // Act + Assert
    TSM_ASSERT_THROWS_ANYTHING(
        "Four dimensional workspace should not be accepted", alg.execute());
  }

  void test_that_saves_MDHisto_without_issues_under_normal_conditions() {
    // Algthough the actual saving should be tested in the implementation
    // file, we should test that the algorithm can run without issues
    // Arrange
    const size_t numDims = 3;
    const size_t numBins = 5;
    auto workspace =
        Mantid::DataObjects::MDEventsTestHelper::makeFakeMDHistoWorkspace(
            1.0, numDims, numBins);

    std::string filename = "SaveMDWorkspaceToVTK_test_file.vts";
    auto fullFilename = getTemporaryFilename(filename);
    removeTemporaryFile(fullFilename);

    Mantid::VATES::SaveMDWorkspaceToVTK alg;
    alg.setChild(true);
    alg.setRethrows(true);
    alg.initialize();
    alg.setProperty("InputWorkspace", workspace);
    alg.setProperty("Filename", fullFilename);
    alg.setProperty("Normalization", "AutoSelect");
    alg.setProperty("RecursionDepth", 5);
    alg.setProperty("CompressorType", "NONE");

    // Act and Assert
    TSM_ASSERT_THROWS_NOTHING("Should save without any issues.", alg.execute());

    auto fileExists = doesFileExist(fullFilename);
    TSM_ASSERT("The file should have been saved out", fileExists);

    // Clean up
    removeTemporaryFile(fullFilename);
  }

private:
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
