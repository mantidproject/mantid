// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_ALGORITHMS_CROPTOCOMPONENTTEST_H_
#define MANTID_ALGORITHMS_CROPTOCOMPONENTTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/SpectrumInfo.h"
#include "MantidAlgorithms/CreateSampleWorkspace.h"
#include "MantidAlgorithms/CropToComponent.h"
#include "MantidDataHandling/LoadRaw3.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"

#include <array>
#include <numeric>

class CropToComponentTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static CropToComponentTest *createSuite() {
    return new CropToComponentTest();
  }
  static void destroySuite(CropToComponentTest *suite) { delete suite; }

  void test_Init() {
    Mantid::Algorithms::CropToComponent alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
  }

  void test_exec() {
    // Arrange
    int numberOfBanks = 4;
    int numberOfPixelsPerBank = 3;

    auto inputWorkspace =
        getSampleWorkspace(numberOfBanks, numberOfPixelsPerBank);
    std::vector<std::string> componentNames = {"bank2", "bank3"};

    // Act
    Mantid::Algorithms::CropToComponent crop;
    crop.setChild(true);
    crop.initialize();
    crop.setProperty("InputWorkspace", inputWorkspace);
    crop.setProperty("OutputWorkspace", "dummy");
    crop.setProperty("ComponentNames", componentNames);
    crop.execute();
    TS_ASSERT(crop.isExecuted())
    Mantid::API::MatrixWorkspace_sptr outputWorkspace =
        crop.getProperty("OutputWorkspace");

    // Assert
    size_t expectedNumberOfHistograms = 18;
    std::vector<Mantid::detid_t> expectedIDs(expectedNumberOfHistograms);
    std::iota(expectedIDs.begin(), expectedIDs.end(), 18);
    doAsssert(outputWorkspace, expectedIDs, expectedNumberOfHistograms);
  }

  void test_that_no_specified_bank_returns_everything() {
    // Arrange
    int numberOfBanks = 4;
    int numberOfPixelsPerBank = 3;

    auto inputWorkspace =
        getSampleWorkspace(numberOfBanks, numberOfPixelsPerBank);
    std::vector<std::string> componentNames = {};

    // Act
    Mantid::Algorithms::CropToComponent crop;
    crop.setChild(true);
    crop.initialize();
    crop.setProperty("InputWorkspace", inputWorkspace);
    crop.setProperty("OutputWorkspace", "dummy");
    crop.setProperty("ComponentNames", componentNames);
    crop.execute();
    TS_ASSERT(crop.isExecuted())
    Mantid::API::MatrixWorkspace_sptr outputWorkspace =
        crop.getProperty("OutputWorkspace");

    // Assert
    size_t expectedNumberOfHistograms = 36;
    std::vector<Mantid::detid_t> expectedIDs(expectedNumberOfHistograms);
    std::iota(expectedIDs.begin(), expectedIDs.end(), 9);
    doAsssert(outputWorkspace, expectedIDs, expectedNumberOfHistograms);
  }

  void test_that_single_bank_can_be_extracted() {
    // Arrange
    int numberOfBanks = 4;
    int numberOfPixelsPerBank = 3;

    auto inputWorkspace =
        getSampleWorkspace(numberOfBanks, numberOfPixelsPerBank);
    std::vector<std::string> componentNames = {"bank3"};
    // Clearing some IDs in bank2 should not cause issues, compare
    // test_throws_if_no_spectrum_for_detector.
    inputWorkspace->getSpectrum(9).clearDetectorIDs();

    // Act
    Mantid::Algorithms::CropToComponent crop;
    crop.setChild(true);
    crop.initialize();
    crop.setProperty("InputWorkspace", inputWorkspace);
    crop.setProperty("OutputWorkspace", "dummy");
    crop.setProperty("ComponentNames", componentNames);
    crop.execute();
    TS_ASSERT(crop.isExecuted())
    Mantid::API::MatrixWorkspace_sptr outputWorkspace =
        crop.getProperty("OutputWorkspace");

    // Assert
    size_t expectedNumberOfHistograms = 9;
    std::vector<Mantid::detid_t> expectedIDs(expectedNumberOfHistograms);
    std::iota(expectedIDs.begin(), expectedIDs.end(), 27);
  }

  void test_throws_if_no_spectrum_for_detector() {
    // Arrange
    int numberOfBanks = 4;
    int numberOfPixelsPerBank = 3;

    auto inputWorkspace =
        getSampleWorkspace(numberOfBanks, numberOfPixelsPerBank);
    std::vector<std::string> componentNames = {"bank3"};
    // Clear some IDs in bank3.
    inputWorkspace->getSpectrum(18).clearDetectorIDs();

    // Act
    Mantid::Algorithms::CropToComponent crop;
    crop.setChild(true);
    crop.initialize();
    crop.setProperty("InputWorkspace", inputWorkspace);
    crop.setProperty("OutputWorkspace", "dummy");
    crop.setProperty("ComponentNames", componentNames);
    TS_ASSERT_THROWS_EQUALS(
        crop.execute(), const std::runtime_error &e, std::string(e.what()),
        "Some of the requested detectors do not have a corresponding spectrum");
  }

  void test_that_incorrect_component_name_is_not_accepeted() {
    // Arrange
    int numberOfBanks = 4;
    int numberOfPixelsPerBank = 3;

    auto inputWorkspace =
        getSampleWorkspace(numberOfBanks, numberOfPixelsPerBank);
    std::vector<std::string> componentNames = {"wrong_detector_name"};

    // Act
    Mantid::Algorithms::CropToComponent crop;
    crop.setChild(true);
    crop.initialize();
    crop.setRethrows(true);
    crop.setProperty("InputWorkspace", inputWorkspace);
    crop.setProperty("OutputWorkspace", "dummy");
    crop.setProperty("ComponentNames", componentNames);
    TSM_ASSERT_THROWS("Invalid detector names will throw.", crop.execute(),
                      const std::runtime_error &)
  }

  void test_that_det_ids_are_ordered() {
    // Arrange
    Mantid::DataHandling::LoadRaw3 loader;
    loader.initialize();
    loader.setPropertyValue("Filename", "LOQ48097.raw");
    loader.setPropertyValue("OutputWorkspace", "in");
    loader.execute();
    Mantid::API::MatrixWorkspace_sptr workspace =
        Mantid::API::AnalysisDataService::Instance()
            .retrieveWS<Mantid::API::MatrixWorkspace>("in");

    std::vector<std::string> componentNames = {"main-detector-bank"};

    // Act
    Mantid::Algorithms::CropToComponent crop;
    crop.setChild(true);
    crop.initialize();
    crop.setRethrows(true);
    crop.setProperty("InputWorkspace", workspace);
    crop.setProperty("OutputWorkspace", "ordered");
    crop.setProperty("ComponentNames", componentNames);
    crop.execute();
    TS_ASSERT(crop.isExecuted())
    Mantid::API::MatrixWorkspace_sptr orderedWorkspace =
        crop.getProperty("OutputWorkspace");

    // Assert
    // Test the first three spectrum numbers.
    // The ordered workspace should show: 3, 4, 5
    // Without the implemneted ordering we would get 3, 131, 259
    std::array<size_t, 3> indices{{0, 1, 2}};
    std::array<size_t, 3> expectedOrdered{{3, 4, 5}};

    for (auto index : indices) {
      const auto &specOrdered = orderedWorkspace->getSpectrum(index);
      TS_ASSERT_EQUALS(specOrdered.getSpectrumNo(), expectedOrdered[index]);
    }

    // Clean up the ADS
    if (Mantid::API::AnalysisDataService::Instance().doesExist("in")) {
      Mantid::API::AnalysisDataService::Instance().remove("in");
    }
  }

  void test_scanning_workspace() {

    // Create a sample scanning workspace
    Mantid::Algorithms::CreateSampleWorkspace creator;
    creator.setChild(true);
    creator.initialize();
    creator.setProperty("NumBanks", 2);
    creator.setProperty("NumScanPoints", 5);
    creator.setProperty("OutputWorkspace", "__unused_for_child");
    creator.execute();
    Mantid::API::MatrixWorkspace_sptr workspace =
        creator.getProperty("OutputWorkspace");

    // Act
    Mantid::Algorithms::CropToComponent crop;
    crop.setChild(true);
    crop.initialize();
    crop.setRethrows(true);
    crop.setProperty("InputWorkspace", workspace);
    crop.setProperty("OutputWorkspace", "__cropped");
    crop.setProperty("ComponentNames", "bank1");
    TS_ASSERT_THROWS_NOTHING(crop.execute())
    TS_ASSERT(crop.isExecuted())
    Mantid::API::MatrixWorkspace_sptr cropped =
        crop.getProperty("OutputWorkspace");
    TS_ASSERT(cropped)
  }

private:
  Mantid::API::MatrixWorkspace_sptr
  getSampleWorkspace(int numberOfBanks, int numbersOfPixelPerBank) {
    return WorkspaceCreationHelper::create2DWorkspaceWithRectangularInstrument(
        numberOfBanks, numbersOfPixelPerBank, 2);
  }

  void doAsssert(Mantid::API::MatrixWorkspace_sptr workspace,
                 std::vector<Mantid::detid_t> &expectedIDs,
                 size_t expectedNumberOfHistograms) {
    // Assert
    const auto numberOfHistograms = workspace->getNumberHistograms();
    TS_ASSERT_EQUALS(numberOfHistograms, expectedNumberOfHistograms);

    std::vector<size_t> indices(numberOfHistograms);
    std::iota(indices.begin(), indices.end(), 0);

    const auto &spectrumInfo = workspace->spectrumInfo();
    for (const auto index : indices) {
      Mantid::detid_t detectorID = spectrumInfo.detector(index).getID();
      TSM_ASSERT_EQUALS("The detector IDs should match.", expectedIDs[index],
                        detectorID);
    }
  }
};

#endif /* MANTID_ALGORITHMS_CROPTOCOMPONENTTEST_H_ */
