#ifndef SETSCALINGPSDTEST_H_
#define SETSCALINGPSDTEST_H_

#include "MantidAPI/SpectrumInfo.h"
#include "MantidDataHandling/LoadEmptyInstrument.h"
#include "MantidDataHandling/SetScalingPSD.h"
#include "MantidGeometry/Instrument/ComponentInfo.h"
#include "MantidKernel/ConfigService.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"
#include <Poco/File.h>
#include <Poco/Path.h>
#include <cmath>
#include <cxxtest/TestSuite.h>
#include <fstream>

using namespace Mantid::API;
using namespace Mantid::Geometry;
using Mantid::DataHandling::LoadEmptyInstrument;
using Mantid::DataHandling::SetScalingPSD;
using Mantid::DataObjects::Workspace2D;
using Mantid::DataObjects::Workspace2D_sptr;
using Mantid::Kernel::ConfigService;
using Mantid::Kernel::V3D;

namespace {
bool isDefaultScaleFactor(const V3D &scaleFactor) {
  const V3D defaultScaleFactor(1.0, 1.0, 1.0);
  return (scaleFactor - defaultScaleFactor).norm() < 1e-9;
}
} // namespace

class SetScalingPSDTest : public CxxTest::TestSuite {
public:
  static SetScalingPSDTest *createSuite() { return new SetScalingPSDTest(); }
  static void destroySuite(SetScalingPSDTest *suite) { delete suite; }

  SetScalingPSDTest() : m_y_offset(0.0005) {}

  void test_Input_ASCII_File_Scales_Correctly() {
    const int ndets = 5;
    // Test workspace with 5 detectors, 3 detectors + 2 monitors at the end. 1:1
    // mapping of spectrum:ID
    Workspace2D_sptr testWS =
        WorkspaceCreationHelper::create2DWorkspaceWithFullInstrument(ndets, 1,
                                                                     true);
    const std::string scalingFile = createTestScalingFile(testWS);
    // Needs to be in the ADS for this algorithm
    const std::string wsName("PSDTest");
    AnalysisDataServiceImpl &dataStore = AnalysisDataService::Instance();
    dataStore.add(wsName, testWS);

    // Store the detector positions before the shifts so that we can compare
    std::map<int, V3D> originalPositions;
    const auto &spectrumInfo = testWS->spectrumInfo();
    for (int i = 0; i < ndets; ++i) {
      const auto pos = spectrumInfo.position(i);
      originalPositions.emplace(i, pos);
    }

    IAlgorithm_sptr scaler = createAlgorithm();
    scaler->setPropertyValue("ScalingFilename", scalingFile);
    scaler->setPropertyValue("Workspace", wsName);
    TS_ASSERT_THROWS_NOTHING(scaler->execute());

    // Test values.
    double expectedYPos[5] = {-0.0005, 0.0990, 0.1985, -0.002042, -0.0025133};
    double expectedYScale[3] = {0.995002, 0.995001, 0.995000};

    const auto &spectrumIndexNew = testWS->spectrumInfo();
    const auto &componentInfo = testWS->componentInfo();
    for (int i = 0; i < ndets; ++i) {
      V3D newPos = spectrumIndexNew.position(i);
      V3D oldPos = originalPositions[i];
      TS_ASSERT_DELTA(newPos.Y(), expectedYPos[i], 1e-6);
      TS_ASSERT_DELTA(fabs(oldPos.X() - newPos.X()), 0.0, 1e-05);
      TS_ASSERT_DELTA(fabs(oldPos.Z() - newPos.Z()), 0.0, 1e-05);

      if (spectrumIndexNew.isMonitor(i)) {
        TS_ASSERT(isDefaultScaleFactor(componentInfo.scaleFactor(i)));
      } else {
        TS_ASSERT(!isDefaultScaleFactor(componentInfo.scaleFactor(i)));
        const V3D scaleFactor = componentInfo.scaleFactor(i);
        TS_ASSERT_EQUALS(scaleFactor.X(), 1.0);
        TS_ASSERT_EQUALS(scaleFactor.Z(), 1.0);
        TS_ASSERT_DELTA(scaleFactor.Y(), expectedYScale[i], 1e-06);
      }
    }

    Poco::File(scalingFile).remove();
    dataStore.remove(wsName);
  }

  void test_Input_RAW_File_Scales_Correctly() {
    // No way around using a raw files here unfortunately as that's what we need
    // to test
    Workspace2D_sptr testWS = loadEmptyMARI();
    TS_ASSERT(testWS);
    if (!testWS) {
      TS_FAIL("Error loading test instrument.");
    }

    IAlgorithm_sptr scaler = createAlgorithm();
    // The most commonly used MARI file in the repository
    scaler->setPropertyValue("ScalingFilename", "MAR11001.raw");
    scaler->setPropertyValue("Workspace", testWS->getName());
    scaler->execute();

    // Test a few detectors
    int testIndices[3] = {6, 7, 8};
    V3D expectedValues[3] = {V3D(-0.08982175, -1.03708771, 3.88495351),
                             V3D(-0.09233499, -1.06610575, 3.87703178),
                             V3D(-0.09484302, -1.09506369, 3.86889169)};
    const auto &spectrumInfo = testWS->spectrumInfo();
    for (int i = 0; i < 3; ++i) {
      const auto pos = spectrumInfo.position(testIndices[i]);
      V3D expectedPos = expectedValues[i];
      TS_ASSERT_DELTA(pos.X(), expectedPos.X(), 1e-8);
      TS_ASSERT_DELTA(pos.Y(), expectedPos.Y(), 1e-8);
      TS_ASSERT_DELTA(pos.Z(), expectedPos.Z(), 1e-8);
    }

    AnalysisDataService::Instance().remove(testWS->getName());
  }

private:
  std::string createTestScalingFile(Workspace2D_sptr testWS) {
    // NOTE: The only values read by the algorithm are the number of detectors
    // and then from each detector line
    // det no., (l2,theta,phi). All other values will be set to -1 here
    const int ndets = static_cast<int>(testWS->getNumberHistograms());
    const std::string filename = "test-setscalingpsd.sca";
    std::ofstream writer(filename.c_str());
    writer << filename << " created by unit test\n";
    writer << ndets << "\t" << -1 << "\n";
    writer << "det no.  offset    l2     code     theta        phi"
           << "         w_x         w_y         w_z         f_x         f_y\n";
    const auto &spectrumInfo = testWS->spectrumInfo();
    for (int i = 0; i < ndets; ++i) {
      auto oldPos = spectrumInfo.position(i);
      // Move in Y only
      oldPos.setY(oldPos.Y() - m_y_offset * (i + 1));
      double l2, theta, phi;
      oldPos.getSpherical(l2, theta, phi);
      const auto &detector = spectrumInfo.detector(i);
      writer << detector.getID() << "\t" << -1 << "\t" << l2 << "\t" << -1
             << "\t" << theta << "\t" << phi << "\n";
    }

    writer.close();
    return filename;
  }

  Workspace2D_sptr loadEmptyMARI() {
    Poco::Path mariIDF(ConfigService::Instance().getInstrumentDirectory());
    mariIDF.resolve("MARI_Definition.xml");
    LoadEmptyInstrument loader;
    loader.initialize();
    loader.setPropertyValue("Filename", mariIDF.toString());
    const std::string outputName("test-emptyMARI");
    loader.setPropertyValue("OutputWorkspace", outputName);
    loader.execute();
    Workspace_sptr result =
        AnalysisDataService::Instance().retrieve(outputName);
    return boost::dynamic_pointer_cast<Workspace2D>(result);
  }

  IAlgorithm_sptr createAlgorithm() {
    IAlgorithm_sptr scaler(new SetScalingPSD);
    scaler->initialize();
    scaler->setRethrows(true);
    return scaler;
  }

  /// Constant shift
  const double m_y_offset;
};

#endif /*SETSCALINGPSDTEST_H_*/
