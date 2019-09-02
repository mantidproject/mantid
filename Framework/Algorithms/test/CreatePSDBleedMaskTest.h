// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef CREATEPSDBLEEDMASKSTEST_H_
#define CREATEPSDBLEEDMASKSTEST_H_

#include "MantidAlgorithms/CreatePSDBleedMask.h"
#include <cxxtest/TestSuite.h>

#include "MantidDataObjects/WorkspaceCreation.h"
#include "MantidGeometry/Instrument.h"
#include "MantidGeometry/Instrument/CompAssembly.h"
#include "MantidGeometry/Instrument/Detector.h"
#include "MantidHistogramData/LinearGenerator.h"
#include "MantidIndexing/IndexInfo.h"
#include "MantidTestHelpers/ComponentCreationHelper.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"

using Mantid::DataObjects::Workspace2D_sptr;
using Mantid::Geometry::Instrument_sptr;

class CreatePSDBleedMaskTest : public CxxTest::TestSuite {

public:
  void test_Name() {
    TS_ASSERT_EQUALS(diagnostic.name(), "CreatePSDBleedMask");
  }

  void test_Category() {
    TS_ASSERT_EQUALS(diagnostic.category(), "Diagnostics");
  }

  void test_That_Tube_Based_Detector_Gives_Expected_Masking() {
    using Mantid::API::MatrixWorkspace_sptr;
    using Mantid::Geometry::IDetector_const_sptr;

    Workspace2D_sptr testWS = createTestWorkspace();

    if (!diagnostic.isInitialized()) {
      diagnostic.initialize();
    }

    TS_ASSERT(diagnostic.isInitialized());

    diagnostic.setProperty<Mantid::API::MatrixWorkspace_sptr>("InputWorkspace",
                                                              testWS);
    const std::string outputName("PSDBleedMask-Test");
    diagnostic.setPropertyValue("OutputWorkspace", outputName);
    // Based on test setup: Passing tubes should have a framerate 9.2 and the
    // failing tube 19.0
    diagnostic.setProperty("MaxTubeFramerate", 10.0);
    diagnostic.setProperty("NIgnoredCentralPixels", 4);

    diagnostic.setRethrows(true);

    // First test that a workspace not containing the number of good frames
    // fails
    TS_ASSERT_THROWS(diagnostic.execute(), const std::invalid_argument &);

    // Set the number of frames
    testWS->mutableRun().addProperty("goodfrm", 10);

    TS_ASSERT_THROWS_NOTHING(diagnostic.execute());

    Mantid::API::AnalysisDataServiceImpl &dataStore =
        Mantid::API::AnalysisDataService::Instance();
    bool ws_found = dataStore.doesExist(outputName);
    TS_ASSERT(ws_found);
    if (!ws_found)
      return;
    MatrixWorkspace_sptr outputWS =
        boost::dynamic_pointer_cast<Mantid::API::MatrixWorkspace>(
            dataStore.retrieve(outputName));
    TS_ASSERT(outputWS);
    if (!outputWS) {
      TS_FAIL("Cannot find output workspace");
    }

    const size_t numSpectra = outputWS->getNumberHistograms();
    TS_ASSERT_EQUALS(numSpectra, testWS->getNumberHistograms());
    TS_ASSERT_EQUALS(outputWS->blocksize(), 1);
    const int numMasked = diagnostic.getProperty("NumberOfFailures");
    TS_ASSERT_EQUALS(numMasked, 50);

    // Test masking
    int failedIndexStart(50), failedIndexEnd(99);
    for (int i = failedIndexStart; i <= failedIndexEnd; ++i) {
      TS_ASSERT_EQUALS(outputWS->y(i)[0], 1.);
    }

    for (int i = 0; i <= 49; ++i) {
      TS_ASSERT_EQUALS(outputWS->y(i)[0], 0.);
    }

    dataStore.remove(outputName);
  }

private:
  Workspace2D_sptr createTestWorkspace() {
    using namespace Mantid::DataObjects;
    using namespace Mantid::HistogramData;
    using namespace Mantid::Indexing;
    const int nTubes = 3;
    const int nPixelsPerTube = 50;
    const int nBins(5);
    // YLength = nTubes * nPixelsPerTube
    const int nSpectra(nTubes * nPixelsPerTube);
    auto testWS = create<Workspace2D>(
        ComponentCreationHelper::createInstrumentWithPSDTubes(nTubes,
                                                              nPixelsPerTube),
        IndexInfo(nSpectra),
        Histogram(BinEdges(nBins + 1, LinearGenerator(0.0, 1.0)),
                  Counts(nBins, 2.0)));
    // Set a spectra to have high count such that the fail the test
    const int failedTube(1);
    // Set a high value to tip that tube over the max count rate
    testWS->mutableY(failedTube * nPixelsPerTube + 1)[0] = 100.0;
    return std::move(testWS);
  }

  Mantid::Algorithms::CreatePSDBleedMask diagnostic;
};

#endif // CREATEPSDBLEEDMASKSTEST_H_
