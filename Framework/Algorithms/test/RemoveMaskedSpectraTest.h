#ifndef MANTID_ALGORITHMS_REMOVEMASKEDSPECTRATEST_H_
#define MANTID_ALGORITHMS_REMOVEMASKEDSPECTRATEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAlgorithms/RemoveMaskedSpectra.h"
#include "MantidAPI/AlgorithmManager.h"
#include "MantidTestHelpers/ComponentCreationHelper.h"

using Mantid::Algorithms::RemoveMaskedSpectra;
using namespace Mantid::API;
using namespace Mantid;

class RemoveMaskedSpectraTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static RemoveMaskedSpectraTest *createSuite() {
    return new RemoveMaskedSpectraTest();
  }
  static void destroySuite(RemoveMaskedSpectraTest *suite) {
    AnalysisDataService::Instance().clear();
    delete suite;
  }

  RemoveMaskedSpectraTest() : nSpec(5), nBins(6) {}

  void test_Init() {
    RemoveMaskedSpectra alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
  }

  void test_workspace2D_mask() {
    auto inputWS = createInputWorkspace();
    auto maskedWS = createInputWorkspace();
    TS_ASSERT_DIFFERS(inputWS, maskedWS);
    maskWorkspace(maskedWS);
    auto output = runAlgorithm(inputWS, maskedWS);
    TS_ASSERT_EQUALS(output->getNumberHistograms(), 2);
    TS_ASSERT_EQUALS(output->readY(0).front(), 1.0);
    TS_ASSERT_EQUALS(output->readY(1).front(), 3.0);
  }

  void test_mask_workspace_mask() {
    auto inputWS = createInputWorkspace();
    auto secondWS = createInputWorkspace();
    maskWorkspace(secondWS);

    auto alg = AlgorithmManager::Instance().create("ExtractMask");
    alg->initialize();
    alg->setProperty("InputWorkspace", secondWS);
    alg->setPropertyValue("OutputWorkspace", "RemoveMaskedSpectraTest_MaskWS");
    alg->execute();
    auto maskedWS = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(
        "RemoveMaskedSpectraTest_MaskWS");

    auto output = runAlgorithm(inputWS, maskedWS);
    TS_ASSERT_EQUALS(output->getNumberHistograms(), 2);
    TS_ASSERT_EQUALS(output->readY(0).front(), 1.0);
    TS_ASSERT_EQUALS(output->readY(1).front(), 3.0);
  }

  void test_self_mask() {
    auto inputWS = createInputWorkspace();
    maskWorkspace(inputWS);
    auto output = runAlgorithm(inputWS);
    TS_ASSERT_EQUALS(output->getNumberHistograms(), 2);
    TS_ASSERT_EQUALS(output->readY(0).front(), 1.0);
    TS_ASSERT_EQUALS(output->readY(1).front(), 3.0);
  }

private:
  // ---- helper methods ----

  const size_t nSpec;
  const size_t nBins;

  MatrixWorkspace_sptr createInputWorkspace() const {
    // Set up a small workspace for testing
    MatrixWorkspace_sptr space = WorkspaceFactory::Instance().create(
        "Workspace2D", nSpec, nBins + 1, nBins);
    space->setInstrument(
        ComponentCreationHelper::createTestInstrumentCylindrical(1));
    for (size_t j = 0; j < nSpec; ++j) {
      for (size_t k = 0; k <= nBins; ++k) {
        space->dataX(j)[k] = double(k);
      }
      space->dataY(j).assign(nBins, double(j));
      space->dataE(j).assign(nBins, sqrt(double(j)));
      space->getSpectrum(j)->setDetectorID(detid_t(j + 1));
    }
    return space;
  }

  void maskWorkspace(MatrixWorkspace_sptr ws) {
    std::vector<int> spectra(3);
    spectra[0] = 1;
    spectra[1] = 3;
    spectra[2] = 5;
    auto alg = AlgorithmManager::Instance().create("MaskDetectors");
    alg->initialize();
    alg->setProperty("Workspace", ws);
    alg->setProperty("SpectraList", spectra);
    alg->execute();
  }

  MatrixWorkspace_sptr
  runAlgorithm(MatrixWorkspace_sptr inputWS,
               MatrixWorkspace_sptr maskedWS = MatrixWorkspace_sptr()) {
    // Name of the output workspace.
    std::string outWSName("RemoveMaskedSpectraTest_OutputWS");

    RemoveMaskedSpectra alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("InputWorkspace", inputWS));
    if (maskedWS) {
      TS_ASSERT_THROWS_NOTHING(alg.setProperty("MaskedWorkspace", maskedWS));
    }
    TS_ASSERT_THROWS_NOTHING(
        alg.setPropertyValue("OutputWorkspace", outWSName));
    TS_ASSERT_THROWS_NOTHING(alg.execute(););
    TS_ASSERT(alg.isExecuted());
    if (!alg.isExecuted())
      return MatrixWorkspace_sptr();

    // Retrieve the workspace from data service. TODO: Change to your desired
    // type
    MatrixWorkspace_sptr ws;
    TS_ASSERT_THROWS_NOTHING(
        ws = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(
            outWSName));
    TS_ASSERT(ws);
    return ws;
  }
};

#endif /* MANTID_ALGORITHMS_REMOVEMASKEDSPECTRATEST_H_ */
