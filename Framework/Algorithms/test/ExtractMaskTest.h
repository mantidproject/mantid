#ifndef EXTRACTMASKINGTEST_H_
#define EXTRACTMASKINGTEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidAPI/SpectrumInfo.h"
#include "MantidAlgorithms/ExtractMask.h"
#include "MantidDataObjects/MaskWorkspace.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"

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
    TS_ASSERT_EQUALS(properties.size(), 3);
    if (properties.size() == 3) {
      TS_ASSERT_EQUALS(properties[0]->name(), "InputWorkspace");
      TS_ASSERT_EQUALS(properties[1]->name(), "OutputWorkspace");
    }
  }

  // Commenting out test because I am not sure that this is indeed the correct
  // behaviour.
  void
  xtest_That_Input_Masked_Spectra_Are_Assigned_Zero_And_Remain_Masked_On_Output() {
    // Create a simple test workspace
    const int nvectors(50), nbins(10);
    Workspace2D_sptr inputWS =
        WorkspaceCreationHelper::create2DWorkspace(nvectors, nbins);
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

    Workspace_sptr workspace =
        AnalysisDataService::Instance().retrieve(outputName);
    if (workspace) {
      // output should be a MaskWorkspace
      MaskWorkspace_sptr outputWS =
          boost::dynamic_pointer_cast<MaskWorkspace>(workspace);
      return outputWS;
    } else {
      return MaskWorkspace_sptr();
    }
  }

  void doTest(MatrixWorkspace_const_sptr inputWS,
              MaskWorkspace_const_sptr outputWS) {
    TS_ASSERT_EQUALS(outputWS->blocksize(), 1);
    size_t nOutputHists(outputWS->getNumberHistograms());
    TS_ASSERT_EQUALS(nOutputHists, inputWS->getNumberHistograms());
    const auto &iSpecInfo = inputWS->spectrumInfo();
    const auto &oSpecInfo = outputWS->spectrumInfo();
    for (size_t i = 0; i < nOutputHists; ++i) {
      // Sizes
      TS_ASSERT_EQUALS(outputWS->readX(i).size(), 1);
      TS_ASSERT_EQUALS(outputWS->readY(i).size(), 1);
      TS_ASSERT_EQUALS(outputWS->readE(i).size(), 1);
      // Data
      double expectedValue(-1.0);
      bool outputMasked(false);
      if (!iSpecInfo.hasDetectors(i) || !oSpecInfo.hasDetectors(i)) {
        expectedValue = 1.0;
      }

      if (iSpecInfo.hasDetectors(i) && iSpecInfo.isMasked(i)) {
        expectedValue = 1.0;
        outputMasked = true;
      } else {
        expectedValue = 0.0;
        outputMasked = false;
      }

      TS_ASSERT_EQUALS(outputWS->dataY(i)[0], expectedValue);
      TS_ASSERT_EQUALS(outputWS->dataE(i)[0], expectedValue);
      TS_ASSERT_EQUALS(outputWS->dataX(i)[0], 0.0);
      if (iSpecInfo.hasDetectors(i)) {
        TS_ASSERT_EQUALS(oSpecInfo.isMasked(i), outputMasked);
      }
    }
  }
};

#endif // EXTRACTMASKINGTEST_H_
