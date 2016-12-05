#ifndef CREATEPSDBLEEDMASKSTEST_H_
#define CREATEPSDBLEEDMASKSTEST_H_

#include "MantidAlgorithms/CreatePSDBleedMask.h"
#include <cxxtest/TestSuite.h>

#include "MantidTestHelpers/WorkspaceCreationHelper.h"
#include "MantidTestHelpers/ComponentCreationHelper.h"
#include "MantidGeometry/Instrument.h"
#include "MantidGeometry/Instrument/CompAssembly.h"
#include "MantidGeometry/Instrument/Detector.h"

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
    TS_ASSERT_THROWS(diagnostic.execute(), std::invalid_argument);

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
    const int nTubes = 3;
    const int nPixelsPerTube = 50;
    const int nBins(5);
    // YLength = nTubes * nPixelsPerTube
    const int nSpectra(nTubes * nPixelsPerTube);
    Workspace2D_sptr testWS =
        WorkspaceCreationHelper::create2DWorkspaceBinned(nSpectra, nBins);
    testWS->setInstrument(createTestInstrument(nTubes, nPixelsPerTube));
    // Set a spectra to have high count such that the fail the test
    const int failedTube(1);
    // Set a high value to tip that tube over the max count rate
    testWS->mutableY(failedTube * nPixelsPerTube + 1)[0] = 100.0;
    return testWS;
  }

  Mantid::Geometry::Instrument_sptr
  createTestInstrument(const int nTubes = 3, const int nPixelsPerTube = 50) {
    using Mantid::Geometry::Instrument_sptr;
    using Mantid::Geometry::Instrument;
    using Mantid::Geometry::CompAssembly;
    using Mantid::Geometry::Detector;
    using Mantid::Geometry::Object_sptr;
    using Mantid::Kernel::V3D;

    // Need a tube based instrument.
    // pixels
    // Pixels will be numbered simply from 1->nTubes*nPixelsPerTube with a 1:1
    // mapping
    Instrument_sptr testInst(new Instrument("Merlin-like"));

    // Pixel shape
    const double pixelRadius(0.01);
    const double pixelHeight(0.003);
    Object_sptr pixelShape = ComponentCreationHelper::createCappedCylinder(
        pixelRadius, pixelHeight, V3D(0.0, -0.5 * pixelHeight, 0.0),
        V3D(0.0, 1.0, 0.0), "pixelShape");
    for (int i = 0; i < nTubes; ++i) {
      std::ostringstream lexer;
      lexer << "tube-" << i;
      CompAssembly *tube = new CompAssembly(lexer.str());
      tube->setPos(V3D(i * 2.0 * pixelRadius, 0.0, 0.0));
      for (int j = 0; j < nPixelsPerTube; ++j) {
        lexer.str("");
        lexer << "pixel-" << i *nPixelsPerTube + j;
        Detector *pixel = new Detector(lexer.str(), i * nPixelsPerTube + j + 1,
                                       pixelShape, tube);
        const double xpos = 0.0;
        const double ypos = j * pixelHeight;
        pixel->setPos(xpos, ypos, 0.0);
        tube->add(pixel);
        testInst->markAsDetector(pixel);
      }
      testInst->add(tube);
    }
    return testInst;
  }

  Mantid::Algorithms::CreatePSDBleedMask diagnostic;
};

#endif // CREATEPSDBLEEDMASKSTEST_H_
