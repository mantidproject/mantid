// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/SpectrumInfo.h"
#include "MantidAlgorithms/ExtractMask.h"
#include "MantidDataObjects/MaskWorkspace.h"
#include "MantidFrameworkTestHelpers/WorkspaceCreationHelper.h"
#include <cxxtest/TestSuite.h>

using namespace Mantid::API;
using namespace Mantid::DataObjects;
using namespace Mantid::Geometry;
using Mantid::Algorithms::ExtractMask;
using Mantid::Kernel::Property;

class ExtractMaskTest : public CxxTest::TestSuite {

public:
  void test_Init_Gives_An_Input_And_An_Output_Workspace_Property() {
    ExtractMask maskExtractor;
    maskExtractor.initialize();
    std::vector<Property *> properties = maskExtractor.getProperties();
    TS_ASSERT_EQUALS(properties.size(), 4);
    if (properties.size() == 4) {
      TS_ASSERT_EQUALS(properties[0]->name(), "InputWorkspace");
      TS_ASSERT_EQUALS(properties[1]->name(), "InstrumentDonor");
      TS_ASSERT_EQUALS(properties[2]->name(), "OutputWorkspace");
    }
  }

  // Commenting out test because I am not sure that this is indeed the correct
  // behaviour.
  void test_That_Input_Masked_Spectra_Are_Assigned_Zero_And_Remain_Masked_On_Output() {
    // Create a simple test workspace
    const int nvectors(50), nbins(10);
    Workspace2D_sptr inputWS = WorkspaceCreationHelper::create2DWorkspace(nvectors, nbins);
    // Mask every 10th spectra
    std::set<int64_t> maskedIndices;
    for (int i = 0; i < 50; i += 10) {
      maskedIndices.insert(i);
    }
    // A few randoms
    maskedIndices.insert(5);
    maskedIndices.insert(23);
    maskedIndices.insert(37);
    inputWS = WorkspaceCreationHelper::maskSpectra(inputWS, maskedIndices);

    const std::string inputName("inputWS");
    AnalysisDataService::Instance().add(inputName, inputWS);
    MaskWorkspace_sptr outputWS;
    TS_ASSERT_THROWS_NOTHING(outputWS = runExtractMask(inputName));
    TS_ASSERT(outputWS);
    if (outputWS) {
      doTest(inputWS, outputWS);
    }

    AnalysisDataService::Instance().remove(inputName);
    AnalysisDataService::Instance().remove(outputWS->getName());
  }

  void test_That_Masked_Detector_List_Populated_When_Passed_A_Mask_Workspace() {
    // Create a simple test workspace
    const int nvectors(50), nbins(10);
    Workspace2D_sptr inputWS = WorkspaceCreationHelper::create2DWorkspace(nvectors, nbins);
    // Mask every 10th spectra
    std::set<int64_t> maskedIndices;
    for (int i = 0; i < 50; i += 10) {
      maskedIndices.insert(i);
    }
    // A few randoms
    maskedIndices.insert(5);
    maskedIndices.insert(23);
    maskedIndices.insert(37);
    inputWS = WorkspaceCreationHelper::maskSpectra(inputWS, maskedIndices);
    const std::string inputName("inputWSMask");
    AnalysisDataService::Instance().add(inputName, inputWS);
    MaskWorkspace_sptr inputWSMask = runExtractMask(inputName);
    std::vector<Mantid::detid_t> expectedDetectorList = {1, 6, 11, 21, 24, 31, 38, 41};

    std::vector<Mantid::detid_t> detectorList;

    TS_ASSERT_THROWS_NOTHING(detectorList = runExtractMaskReturnList(inputWSMask->getName()));
    TS_ASSERT_EQUALS(detectorList, expectedDetectorList)

    AnalysisDataService::Instance().remove(inputName);
    AnalysisDataService::Instance().remove(inputWSMask->getName());
  }

  void test_That_Masked_Detector_List_Populated_When_Passed_A_Workspace() {
    // Create a simple test workspace
    const int nvectors(50), nbins(10);
    Workspace2D_sptr inputWS = WorkspaceCreationHelper::create2DWorkspace(nvectors, nbins);
    // Mask every 10th spectra
    std::set<int64_t> maskedIndices;
    for (int i = 0; i < 50; i += 10) {
      maskedIndices.insert(i);
    }
    // A few randoms
    maskedIndices.insert(5);
    maskedIndices.insert(23);
    maskedIndices.insert(37);
    inputWS = WorkspaceCreationHelper::maskSpectra(inputWS, maskedIndices);
    const std::string inputName("inputWS");
    AnalysisDataService::Instance().add(inputName, inputWS);

    std::vector<Mantid::detid_t> expectedDetectorList = {1, 6, 11, 21, 24, 31, 38, 41};

    std::vector<Mantid::detid_t> detectorList;

    TS_ASSERT_THROWS_NOTHING(detectorList = runExtractMaskReturnList(inputName));
    TS_ASSERT_EQUALS(detectorList, expectedDetectorList)

    AnalysisDataService::Instance().remove(inputName);
  }

private:
  // The input workspace should be in the analysis data service
  MaskWorkspace_sptr runExtractMask(const std::string &inputName) {
    ExtractMask maskExtractor;
    maskExtractor.initialize();
    maskExtractor.setPropertyValue("InputWorkspace", inputName);
    const std::string outputName("masking");
    maskExtractor.setPropertyValue("OutputWorkspace", outputName);
    maskExtractor.setRethrows(true);
    maskExtractor.execute();

    Workspace_sptr workspace = AnalysisDataService::Instance().retrieve(outputName);
    if (workspace) {
      // output should be a MaskWorkspace
      MaskWorkspace_sptr outputWS = std::dynamic_pointer_cast<MaskWorkspace>(workspace);
      return outputWS;
    } else {
      return MaskWorkspace_sptr();
    }
  }

  std::vector<Mantid::detid_t> runExtractMaskReturnList(const std::string &inputName) {
    ExtractMask maskExtractor;
    maskExtractor.initialize();
    maskExtractor.setPropertyValue("InputWorkspace", inputName);
    const std::string outputName("masking");
    maskExtractor.setPropertyValue("OutputWorkspace", outputName);
    maskExtractor.setRethrows(true);
    maskExtractor.execute();
    std::vector<Mantid::detid_t> detectorList = maskExtractor.getProperty("DetectorList");
    AnalysisDataService::Instance().remove(outputName);
    return detectorList;
  }

  void doTest(const MatrixWorkspace_const_sptr &inputWS, const MaskWorkspace_const_sptr &outputWS) {
    TS_ASSERT_EQUALS(outputWS->blocksize(), 1);
    size_t nOutputHists(outputWS->getNumberHistograms());
    TS_ASSERT_EQUALS(nOutputHists, inputWS->getNumberHistograms());
    const auto &iSpecInfo = inputWS->spectrumInfo();
    const auto &oSpecInfo = outputWS->spectrumInfo();
    for (size_t i = 0; i < nOutputHists; ++i) {
      // Sizes
      TS_ASSERT_EQUALS(outputWS->x(i).size(), 1);
      TS_ASSERT_EQUALS(outputWS->y(i).size(), 1);
      TS_ASSERT_EQUALS(outputWS->e(i).size(), 1);
      // Data
      double expectedValue(-1.0);
      if (!iSpecInfo.hasDetectors(i) || !oSpecInfo.hasDetectors(i)) {
        expectedValue = 1.0;
      }

      if (iSpecInfo.hasDetectors(i) && iSpecInfo.isMasked(i)) {
        expectedValue = 1.0;
      } else {
        expectedValue = 0.0;
      }

      TS_ASSERT_EQUALS(outputWS->y(i)[0], expectedValue);
      TS_ASSERT_EQUALS(outputWS->e(i)[0], 0.0);
      TS_ASSERT_EQUALS(outputWS->x(i)[0], 1.0);
      // Detectors never masked since masking information is carried by Y.
      if (oSpecInfo.hasDetectors(i)) {
        TS_ASSERT_EQUALS(oSpecInfo.isMasked(i), false);
      }
    }
  }
};
