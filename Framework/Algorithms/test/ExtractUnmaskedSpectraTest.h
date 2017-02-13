#ifndef MANTID_ALGORITHMS_EXTRACTUNMASKEDSPECTRATEST_H_
#define MANTID_ALGORITHMS_EXTRACTUNMASKEDSPECTRATEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAPI/AlgorithmFactory.h"
#include "MantidAPI/MatrixWorkspace_fwd.h"
#include "MantidAPI/SpectrumInfo.h"
#include "MantidAlgorithms/ExtractUnmaskedSpectra.h"
#include "MantidDataObjects/MaskWorkspace.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"

#include <algorithm>

using Mantid::Algorithms::ExtractUnmaskedSpectra;

class ExtractUnmaskedSpectraTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static ExtractUnmaskedSpectraTest *createSuite() {
    return new ExtractUnmaskedSpectraTest();
  }
  static void destroySuite(ExtractUnmaskedSpectraTest *suite) { delete suite; }

  void test_Init() {
    ExtractUnmaskedSpectra alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    TS_ASSERT(alg.isInitialized());
  }

  void test_embeded_mask() {
    auto inputWS = createInputWorkspace(10);
    const auto &spectrumInfoIn = inputWS->spectrumInfo();
    TS_ASSERT_EQUALS(spectrumInfoIn.detector(0).getID(), 1);
    TS_ASSERT_EQUALS(spectrumInfoIn.detector(1).getID(), 2);
    TS_ASSERT_EQUALS(spectrumInfoIn.detector(2).getID(), 3);
    TS_ASSERT_EQUALS(spectrumInfoIn.detector(3).getID(), 4);
    TS_ASSERT_EQUALS(spectrumInfoIn.detector(4).getID(), 5);
    TS_ASSERT_EQUALS(spectrumInfoIn.detector(5).getID(), 6);
    TS_ASSERT_EQUALS(spectrumInfoIn.detector(6).getID(), 7);
    TS_ASSERT_EQUALS(spectrumInfoIn.detector(7).getID(), 8);
    TS_ASSERT_EQUALS(spectrumInfoIn.detector(8).getID(), 9);
    TS_ASSERT_EQUALS(spectrumInfoIn.detector(9).getID(), 10);
    auto outputWS = runAlgorithm(inputWS);
    const auto &spectrumInfoOut = outputWS->spectrumInfo();
    TS_ASSERT_EQUALS(spectrumInfoOut.detector(0).getID(), 2);
    TS_ASSERT_EQUALS(spectrumInfoOut.detector(1).getID(), 4);
    TS_ASSERT_EQUALS(spectrumInfoOut.detector(2).getID(), 6);
    TS_ASSERT_EQUALS(spectrumInfoOut.detector(3).getID(), 8);
    TS_ASSERT_EQUALS(spectrumInfoOut.detector(4).getID(), 10);
    TS_ASSERT_EQUALS(outputWS->getNumberHistograms(), 5);
    TS_ASSERT_EQUALS(outputWS->y(0)[0], 2.0);
    TS_ASSERT_EQUALS(outputWS->y(1)[0], 2.0);
    TS_ASSERT_EQUALS(outputWS->y(2)[0], 2.0);
    TS_ASSERT_EQUALS(outputWS->y(3)[0], 2.0);
    TS_ASSERT_EQUALS(outputWS->y(4)[0], 2.0);
  }

  void test_single_spectrum() {
    auto inputWS = createInputWorkspace(1);
    auto outputWS = runAlgorithm(inputWS);
    TS_ASSERT_EQUALS(outputWS->getNumberHistograms(), 1);
    TS_ASSERT_EQUALS(outputWS->spectrumInfo().detector(0).getID(), 1);
  }

  void test_external_mask() {
    auto inputWS = createInputWorkspace(10, false);
    auto maskedWS = createInputWorkspace(10);
    auto outputWS = runAlgorithm(inputWS, maskedWS);
    TS_ASSERT_EQUALS(outputWS->getNumberHistograms(), 5);
    TS_ASSERT_EQUALS(outputWS->y(0)[0], 2.0);
    TS_ASSERT_EQUALS(outputWS->y(1)[0], 2.0);
    TS_ASSERT_EQUALS(outputWS->y(2)[0], 2.0);
    TS_ASSERT_EQUALS(outputWS->y(3)[0], 2.0);
    TS_ASSERT_EQUALS(outputWS->y(4)[0], 2.0);
    const auto &spectrumInfo = outputWS->spectrumInfo();
    TS_ASSERT_EQUALS(spectrumInfo.detector(0).getID(), 2);
    TS_ASSERT_EQUALS(spectrumInfo.detector(1).getID(), 4);
    TS_ASSERT_EQUALS(spectrumInfo.detector(2).getID(), 6);
    TS_ASSERT_EQUALS(spectrumInfo.detector(3).getID(), 8);
    TS_ASSERT_EQUALS(spectrumInfo.detector(4).getID(), 10);
  }

  void test_external_mask_workspace() {
    auto inputWS = createInputWorkspace(10, false);
    auto maskWS = createMask(inputWS);
    auto outputWS = runAlgorithm(inputWS, maskWS);
    TS_ASSERT_EQUALS(outputWS->getNumberHistograms(), 5);
    TS_ASSERT_EQUALS(outputWS->y(0)[0], 2.0);
    TS_ASSERT_EQUALS(outputWS->y(1)[0], 2.0);
    TS_ASSERT_EQUALS(outputWS->y(2)[0], 2.0);
    TS_ASSERT_EQUALS(outputWS->y(3)[0], 2.0);
    TS_ASSERT_EQUALS(outputWS->y(4)[0], 2.0);
    const auto &spectrumInfo = outputWS->spectrumInfo();
    TS_ASSERT_EQUALS(spectrumInfo.detector(0).getID(), 2);
    TS_ASSERT_EQUALS(spectrumInfo.detector(1).getID(), 4);
    TS_ASSERT_EQUALS(spectrumInfo.detector(2).getID(), 6);
    TS_ASSERT_EQUALS(spectrumInfo.detector(3).getID(), 8);
    TS_ASSERT_EQUALS(spectrumInfo.detector(4).getID(), 10);
  }

private:
  Mantid::API::MatrixWorkspace_sptr createInputWorkspace(int n,
                                                         bool isMasked = true) {
    auto workspace =
        WorkspaceCreationHelper::create2DWorkspaceWithFullInstrument(n, 3);
    if (isMasked) {
      size_t nMask = n > 1 ? n / 2 : 1;
      std::vector<size_t> indices(nMask);
      {
        // mask spectra with even indices
        size_t i = 0;
        std::generate(indices.begin(), indices.end(),
                      [&i]() { return 2 * i++; });
      }
      auto alg =
          Mantid::API::AlgorithmFactory::Instance().create("MaskDetectors", -1);
      alg->setChild(true);
      alg->initialize();
      alg->setProperty(
          "Workspace",
          boost::dynamic_pointer_cast<Mantid::API::Workspace>(workspace));
      alg->setProperty("WorkspaceIndexList", indices);
      alg->execute();
    }
    return workspace;
  }

  Mantid::API::MatrixWorkspace_sptr
  createMask(Mantid::API::MatrixWorkspace_sptr inputWS) {
    auto maskWS = Mantid::DataObjects::MaskWorkspace_sptr(
        new Mantid::DataObjects::MaskWorkspace(inputWS));
    size_t n = inputWS->getNumberHistograms();
    size_t nMask = n > 1 ? n / 2 : 1;
    for (size_t i = 0; i < nMask; ++i) {
      auto j = 2 * i;
      maskWS->setMaskedIndex(j, true);
    }
    return maskWS;
  }

  Mantid::API::MatrixWorkspace_sptr
  runAlgorithm(Mantid::API::MatrixWorkspace_sptr inputWS,
               Mantid::API::MatrixWorkspace_sptr maskedWS =
                   Mantid::API::MatrixWorkspace_sptr()) {
    ExtractUnmaskedSpectra alg;
    alg.setChild(true);
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("InputWorkspace", inputWS));
    if (maskedWS) {
      TS_ASSERT_THROWS_NOTHING(alg.setProperty("MaskWorkspace", maskedWS));
    }
    TS_ASSERT_THROWS_NOTHING(
        alg.setPropertyValue("OutputWorkspace", "_unused_for_child"));
    TS_ASSERT_THROWS_NOTHING(alg.execute());
    TS_ASSERT(alg.isExecuted());

    Mantid::API::MatrixWorkspace_sptr outputWS =
        alg.getProperty("OutputWorkspace");
    TS_ASSERT(outputWS);
    return outputWS;
  }
};

#endif /* MANTID_ALGORITHMS_EXTRACTUNMASKEDSPECTRATEST_H_ */