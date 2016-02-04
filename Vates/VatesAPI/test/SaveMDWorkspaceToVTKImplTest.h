#ifndef VATES_API_SAVE_MD_WORKSPACE_TO_VTK_IMPL_TEST_H_
#define VATES_API_SAVE_MD_WORKSPACE_TO_VTK_IMPL_TEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidVatesAPI/SaveMDWorkspaceToVTKImpl.h"
#include "MantidVatesAPI/Normalization.h"
#include "MantidVatesAPI/IgnoreZerosThresholdRange.h"
#include "MantidTestHelpers/MDEventsTestHelper.h"
#include "MantidKernel/ConfigService.h"

#include <Poco/File.h>

using namespace Mantid::DataObjects;

class SaveMDWorkspaceToVTKImplTest : public CxxTest::TestSuite
{
public:
    void test_that_vector_of_normalization_strings_has_all_values()
    {
        // Arrange
        Mantid::VATES::SaveMDWorkspaceToVTKImpl saveMDToVTK;

        // Act
        const auto normalizations
            = saveMDToVTK.getAllowedNormalizationsInStringRepresentation();

        // Assert
        TSM_ASSERT_EQUALS("There should be 4 normalization options.",
                          normalizations.size(), 4);
        TSM_ASSERT_EQUALS("First normalization should be AutoSelect.",
                          normalizations[0], "AutoSelect");
        TSM_ASSERT_EQUALS("First normalization should be NoNormalization.",
                          normalizations[1], "NoNormalization");
        TSM_ASSERT_EQUALS(
            "First normalization should be NumEventsNormalization.",
            normalizations[2], "NumEventsNormalization");
        TSM_ASSERT_EQUALS("First normalization should be VolumeNormalization.",
                          normalizations[3], "VolumeNormalization");
    }

    void test_string_representation_converts_to_visual_normalization()
    {
        // Arrange
        Mantid::VATES::SaveMDWorkspaceToVTKImpl saveMDToVTK;
        const auto normalizations
            = saveMDToVTK.getAllowedNormalizationsInStringRepresentation();

        // Act
        const auto autoSelect
            = saveMDToVTK.translateStringToVisualNormalization(
                normalizations[0]);
        const auto noNormalization
            = saveMDToVTK.translateStringToVisualNormalization(
                normalizations[1]);
        const auto numEventsNormalization
            = saveMDToVTK.translateStringToVisualNormalization(
                normalizations[2]);
        const auto volumeNormalization
            = saveMDToVTK.translateStringToVisualNormalization(
                normalizations[3]);

        // Assert
        TSM_ASSERT_EQUALS("The visual normalization should be AutoSelect.",
                          autoSelect, Mantid::VATES::AutoSelect);
        TSM_ASSERT_EQUALS("The visual normalization should be NoNormalization.",
                          noNormalization, Mantid::VATES::NoNormalization);
        TSM_ASSERT_EQUALS(
            "The visual normalization should be NumEventsNormalization.",
            numEventsNormalization, Mantid::VATES::NumEventsNormalization);
        TSM_ASSERT_EQUALS(
            "The visual normalization should be VolumeNormalization.",
            volumeNormalization, Mantid::VATES::VolumeNormalization);
    }

    void test_that_vector_of_threshold_strings_has_all_values()
    {
        // Arrange
        Mantid::VATES::SaveMDWorkspaceToVTKImpl saveMDToVTK;

        // Act
        const auto thresholds
            = saveMDToVTK.getAllowedThresholdsInStringRepresentation();

        // Assert
        TSM_ASSERT_EQUALS("There should be 2 normalization options",
                          thresholds.size(), 2);
        TSM_ASSERT_EQUALS(
            "First normalization should be IgnoreZerosThresholdRange.",
            thresholds[0], "IgnoreZerosThresholdRange");
        TSM_ASSERT_EQUALS("Second normalization should be NoThresholdRange.",
                          thresholds[1], "NoThresholdRange");
    }

    void test_string_representation_converts_to_TresholdRange()
    {
        // Arrange
        Mantid::VATES::SaveMDWorkspaceToVTKImpl saveMDToVTK;
        auto thresholds
            = saveMDToVTK.getAllowedThresholdsInStringRepresentation();
        // Act
        auto ignoreZerosThresholdRange
            = saveMDToVTK.translateStringToThresholdRange(thresholds[0]);
        auto noThresholdRange
            = saveMDToVTK.translateStringToThresholdRange(thresholds[1]);
        // Assert
        TSM_ASSERT(
            "Should be a IgnoreZerosTresholdRange",
            boost::
                dynamic_pointer_cast<Mantid::VATES::IgnoreZerosThresholdRange>(
                    ignoreZerosThresholdRange));
        TSM_ASSERT("Should be a NoTresholdRange",
                   boost::dynamic_pointer_cast<Mantid::VATES::ThresholdRange>(
                       noThresholdRange));
    }

    void test_detects_when_4D_workspace()
    {
        // Arrange
        Mantid::VATES::SaveMDWorkspaceToVTKImpl saveMDToVTK;
        size_t numDims = 4;
        auto workspace
            = MDEventsTestHelper::makeFakeMDHistoWorkspace(1.0, numDims);

        // Act
        const auto is4D = saveMDToVTK.is4DWorkspace(workspace);

        // Assert
        TSM_ASSERT("Detects a 4D MD workspace", is4D);
    }

    void test_detects_when_not_4D_workspace()
    {
        // Arrange
        Mantid::VATES::SaveMDWorkspaceToVTKImpl saveMDToVTK;
        const size_t numDims = 2;
        auto workspace
            = MDEventsTestHelper::makeFakeMDHistoWorkspace(1.0, numDims);

        // Act
        const auto is4D = saveMDToVTK.is4DWorkspace(workspace);

        // Assert
        TSM_ASSERT("Detects that not a 4D MD workspace", !is4D);
    }

    void test_that_saves_MD_Histo_workspace_to_vts_file()
    {
        // Arrange
        Mantid::VATES::SaveMDWorkspaceToVTKImpl saveMDToVTK;
        const size_t numDims = 3;
        const size_t numBins = 5;
        auto workspace = MDEventsTestHelper::makeFakeMDHistoWorkspace(
            1.0, numDims, numBins);

        const int recursionDepth
            = 5; // Note that this does not matter since we are testing MDHisto

        const auto normalizations
            = saveMDToVTK.getAllowedNormalizationsInStringRepresentation();
        const auto normalization
            = saveMDToVTK.translateStringToVisualNormalization(
                normalizations[0]);

        const auto thresholds
            = saveMDToVTK.getAllowedThresholdsInStringRepresentation();
        const auto threshold
            = saveMDToVTK.translateStringToThresholdRange(thresholds[0]);

        const std::string filenameBare = "SaveMDWorkspaceToVTKImplTestFile";
        const std::string filenameWithExtension = filenameBare + ".vts";

        auto filenameExpected = getTemporaryFilename(filenameWithExtension);
        removeTemporaryFile(filenameExpected);

        auto filename = getTemporaryFilename(filenameBare);

        // Act
        saveMDToVTK.saveMDWorkspace(workspace, filename, normalization,
                                    threshold, recursionDepth);

        // Assert -- at this point we can only check that a vtk (xml) structured
        // grid file (vts) has been created
        auto fileExists = doesFileExist(filenameExpected);
        TSM_ASSERT("The .vts file should have been saved to the default save "
                   "directory",
                   fileExists);
        // Cleanup
        removeTemporaryFile(filenameExpected);
    }

private:
    std::string getTemporaryFilename(std::string filenameWithoutPath) const
    {
        auto default_save_directory
            = Mantid::Kernel::ConfigService::Instance().getString(
                "defaultsave.directory");
        std::string filenameWithPath(default_save_directory
                                     + filenameWithoutPath);
        return filenameWithPath;
    }

    void removeTemporaryFile(std::string fileNameWithPath) const
    {
        if (Poco::File(fileNameWithPath).exists()) {
            Poco::File(fileNameWithPath).remove();
        }
    }

    bool doesFileExist(std::string filename)
    {
        return Poco::File(filename).exists();
    }
};
#endif
