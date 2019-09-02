// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_ALGORITHMS_REMOVESPECTRATEST_H_
#define MANTID_ALGORITHMS_REMOVESPECTRATEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidAlgorithms/RemoveSpectra.h"
#include "MantidHistogramData/LinearGenerator.h"
#include "MantidTestHelpers/ComponentCreationHelper.h"

using namespace Mantid::API;
using namespace Mantid::Algorithms;
using namespace Mantid;

class RemoveSpectraTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static RemoveSpectraTest *createSuite() { return new RemoveSpectraTest(); }
  static void destroySuite(RemoveSpectraTest *suite) { delete suite; }

  void tearDown() override { AnalysisDataService::Instance().clear(); }

  void test_init() {
    RemoveSpectra alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
  }

  void test_removeWorkspaceIndices() {
    const std::string wsName = "workspace";
    const std::string ouputWsName = "outputWorkspace";
    auto inputWS = createInputWorkspace();
    AnalysisDataService::Instance().addOrReplace(wsName, inputWS);

    RemoveSpectra alg;
    alg.initialize();
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("InputWorkspace", wsName));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("WorkspaceIndices", "0,2,4"));
    TS_ASSERT_THROWS_NOTHING(
        alg.setPropertyValue("OutputWorkspace", ouputWsName));
    TS_ASSERT_THROWS_NOTHING(alg.execute());

    MatrixWorkspace_sptr outputWS =
        AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(
            ouputWsName);
    TS_ASSERT_EQUALS(outputWS->getNumberHistograms(), 2);
    TS_ASSERT_EQUALS(outputWS->y(0).front(), 1.0);
    TS_ASSERT_EQUALS(outputWS->y(1).front(), 3.0);
  }

  void test_removeMaskedSpectra() {
    const std::string wsName = "workspace";
    const std::string ouputWsName = "outputWorkspace";
    auto inputWS = createInputWorkspace();
    AnalysisDataService::Instance().addOrReplace(wsName, inputWS);
    maskWorkspace(wsName);

    RemoveSpectra alg;
    alg.initialize();
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("InputWorkspace", wsName));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("RemoveMaskedSpectra", true));
    TS_ASSERT_THROWS_NOTHING(
        alg.setPropertyValue("OutputWorkspace", ouputWsName));
    TS_ASSERT_THROWS_NOTHING(alg.execute());

    MatrixWorkspace_sptr outputWS =
        AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(
            ouputWsName);
    TS_ASSERT_EQUALS(outputWS->getNumberHistograms(), 2);
    TS_ASSERT_EQUALS(outputWS->y(0).front(), 1.0);
    TS_ASSERT_EQUALS(outputWS->y(1).front(), 3.0);
  }

  void test_removeSpectraWithNoDetector() {
    const std::string wsName = "workspace";
    const std::string ouputWsName = "outputWorkspace";
    setupToscaWorkspace(wsName);

    MatrixWorkspace_sptr inputWS =
        AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(wsName);
    TS_ASSERT_EQUALS(inputWS->x(94).front(), 19900.0);
    TS_ASSERT_EQUALS(inputWS->x(144).front(), 19900.0);

    RemoveSpectra alg;
    alg.initialize();
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("InputWorkspace", wsName));
    TS_ASSERT_THROWS_NOTHING(
        alg.setProperty("RemoveSpectraWithNoDetector", true));
    TS_ASSERT_THROWS_NOTHING(
        alg.setPropertyValue("OutputWorkspace", ouputWsName));
    TS_ASSERT_THROWS_NOTHING(alg.execute());

    MatrixWorkspace_sptr outputWS =
        AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(
            ouputWsName);
    // Removed specs are workspace indices 140 and 144/specNum 141 and 145
    TS_ASSERT_EQUALS(outputWS->getNumberHistograms(), 147);
    TS_ASSERT_DELTA(outputWS->x(93).front(), 0.41157, 0.0001);  // was 93
    TS_ASSERT_DELTA(outputWS->x(94).front(), 0.05484, 0.0001);  // was 95
    TS_ASSERT_DELTA(outputWS->x(95).front(), -0.15111, 0.0001); // was 96
    TS_ASSERT_DIFFERS(outputWS->x(143).front(),
                      19900.0); // Would be 144 if 94 wasn't also removed
  }

private:
  // ---- helper methods ----

  const size_t nSpec = 5;
  const size_t nBins = 6;

  MatrixWorkspace_sptr createInputWorkspace() const {
    // Set up a small workspace for testing
    MatrixWorkspace_sptr space = WorkspaceFactory::Instance().create(
        "Workspace2D", nSpec, nBins + 1, nBins);
    space->setInstrument(
        ComponentCreationHelper::createTestInstrumentCylindrical(1));
    HistogramData::BinEdges edges(nBins + 1,
                                  HistogramData::LinearGenerator(0.0, 1.0));
    for (size_t j = 0; j < nSpec; ++j) {
      const double yVal{static_cast<double>(j)};
      const double eVal{sqrt(yVal)};
      space->setBinEdges(j, edges);
      const std::vector<double> counts(nBins, yVal);
      const std::vector<double> errors(nBins, eVal);
      space->setCounts(j, counts);
      space->setCountStandardDeviations(j, errors);
      space->getSpectrum(j).setDetectorID(detid_t(j + 1));
    }
    return space;
  }

  void maskWorkspace(const std::string &workspaceName) {
    std::vector<int> spectra(3);
    spectra[0] = 1;
    spectra[1] = 3;
    spectra[2] = 5;
    auto alg = AlgorithmManager::Instance().create("MaskDetectors");
    alg->initialize();
    alg->setProperty("Workspace", workspaceName);
    alg->setProperty("SpectraList", spectra);
    alg->execute();
  }

  void setupToscaWorkspace(const std::string &wsName) {
    auto loadAlg = AlgorithmManager::Instance().create("Load");
    loadAlg->initialize();
    // This workspace is a specific example with a spectra with no Detector
    loadAlg->setPropertyValue("Filename", "TSC04970.raw");
    loadAlg->setPropertyValue("OutputWorkspace", wsName);
    loadAlg->execute();

    auto alg = AlgorithmManager::Instance().create("ConvertUnits");
    alg->initialize();
    alg->setPropertyValue("InputWorkspace", wsName);
    alg->setPropertyValue("OutputWorkspace", wsName);
    alg->setPropertyValue("Target", "DeltaE");
    alg->setPropertyValue("EMode", "Indirect");
    alg->execute();
  }
};
#endif /* MANTID_ALGORITHMS_REMOVESPECTRATEST_H_ */