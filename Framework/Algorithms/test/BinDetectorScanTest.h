#ifndef MANTID_ALGORITHMS_BINDETECTORSCANTEST_H_
#define MANTID_ALGORITHMS_BINDETECTORSCANTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAlgorithms/BinDetectorScan.h"

#include "MantidAPI/Axis.h"
#include "MantidTestHelpers/ComponentCreationHelper.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"

using Mantid::Algorithms::BinDetectorScan;

using namespace Mantid::API;

class BinDetectorScanTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static BinDetectorScanTest *createSuite() {
    return new BinDetectorScanTest();
  }
  static void destroySuite(BinDetectorScanTest *suite) { delete suite; }

  void test_normal_operation() {
    const size_t nTubes = 5;
    const size_t nPixelsPerTube = 10;
    const size_t nSpectra = nTubes * nPixelsPerTube;
    const size_t nBins = 1;

    MatrixWorkspace_sptr testWS =
        WorkspaceCreationHelper::create2DWorkspaceBinned(nSpectra, nBins);
    testWS->setInstrument(ComponentCreationHelper::createInstrumentWithPSDTubes(
        nTubes, nPixelsPerTube, true));
    AnalysisDataService::Instance().add("testWS", testWS);

    BinDetectorScan alg;
    alg.initialize();
    alg.setProperty("InputWorkspaces", "testWS");
    alg.setProperty("OutputWorkspace", "outWS");
    alg.setProperty("ScatteringAngleBinning", "22.5");
    alg.setProperty("ComponentForHeightAxis", "tube-1");
    //    alg.setProperty("HeightBinning", "-1.0, 0.1, 1.0");
    TS_ASSERT_THROWS_NOTHING(alg.execute());

    MatrixWorkspace_sptr outWS =
        boost::dynamic_pointer_cast<Mantid::API::MatrixWorkspace>(
            AnalysisDataService::Instance().retrieve("outWS"));

    // Check x-axis goes from -90 -> 0 with 180 bins
    const auto &xAxis = outWS->getAxis(0);
    TS_ASSERT_EQUALS(xAxis->length(), nTubes + 1)
    TS_ASSERT_DELTA(xAxis->getValue(0), -90.0 - 22.5 / 2.0, 1e-6)
    TS_ASSERT_DELTA(xAxis->getValue(nTubes), 0.0 + 22.5 / 2.0, 1e-6)

    // Check y-axis goes from 0 to 0.027 with 10 points
    const auto &yAxis = outWS->getAxis(1);
    TS_ASSERT_EQUALS(yAxis->length(), nPixelsPerTube)
    TS_ASSERT_DELTA(yAxis->getValue(0), 0.0, 1e-6)
    TS_ASSERT_DELTA(yAxis->getValue(nPixelsPerTube - 1), 0.027, 1e-6)

    for (size_t i = 0; i < nSpectra; ++i)
      TS_ASSERT_DELTA(testWS->getSpectrum(i).y()[0], 2.0, 1e-6)

    for (size_t i = 0; i < nTubes; ++i) {
      for (size_t j = 0; j < nPixelsPerTube; ++j) {
        std::cout << i << "==" << j << std::endl;
        TS_ASSERT_DELTA(outWS->getSpectrum(j).y()[i], 2.0, 1e-6)
      }
    }
  }

  void test_non_existent_component() {}
};

#endif /* MANTID_ALGORITHMS_BINDETECTORSCANTEST_H_ */
