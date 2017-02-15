#ifndef MANTID_DATAHANDLING_LOADILLREFLECTOMETRYTEST_H_
#define MANTID_DATAHANDLING_LOADILLREFLECTOMETRYTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidDataHandling/LoadILLReflectometry.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/Axis.h"
// Attention: LoadHelper includes "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/SpectrumInfo.h" // Attention: LoadHelper will include this header?? No
#include "MantidKernel/Unit.h"

using namespace Mantid::API;
using Mantid::DataHandling::LoadILLReflectometry;

class LoadILLReflectometryTest : public CxxTest::TestSuite {
private:
  std::string m_dataFile{"ILLD17-161876-Ni.nxs"};
  LoadILLReflectometry loader;
  Mantid::DataHandling::LoadHelper m_loader;

public:
  // Name of the output workspace
  const std::string outWSName{"LoadILLReflectometryTest_OutputWS"};

  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static LoadILLReflectometryTest *createSuite() {
    return new LoadILLReflectometryTest();
  }
  static void destroySuite(LoadILLReflectometryTest *suite) { delete suite; }

  void testInit() {
    TS_ASSERT_THROWS_NOTHING(loader.initialize());
    TS_ASSERT(loader.isInitialized());
  }

  void testName() { TS_ASSERT_EQUALS(loader.name(), "LoadILLReflectometry"); }

  void testVersion() { TS_ASSERT_EQUALS(loader.version(), 1); }

  void testExecD17() {
    loader.setRethrows(true);
    TS_ASSERT_THROWS_NOTHING(loader.setPropertyValue("Filename", m_dataFile));
    TS_ASSERT_THROWS_NOTHING(
        loader.setPropertyValue("OutputWorkspace", outWSName));
    TS_ASSERT_THROWS_NOTHING(loader.execute(););
    TS_ASSERT(loader.isExecuted());
  }

  void testPropertiesD17() {
    loader.initialize();
    loader.setPropertyValue("Filename", m_dataFile);
    loader.setPropertyValue("OutputWorkspace", outWSName);
    loader.execute();
    MatrixWorkspace_sptr output =
        AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(outWSName);
    TS_ASSERT(output);
    TS_ASSERT_EQUALS(output->getNumberHistograms(), 256 + 2);
    double channelWidth =
        m_loader.getPropertyFromRun<double>(output, "channel_width");
    TS_ASSERT_EQUALS(channelWidth, 57.0);
    double analyserAngle =
        m_loader.getPropertyFromRun<double>(output, "dan.value");
    TS_ASSERT_EQUALS(analyserAngle, 3.1909999847412109);
    // Remove workspace from the data service.
    AnalysisDataService::Instance().clear();
  }

  void testInputThetaD17() {
    loader.initialize();
    loader.setPropertyValue("Filename", m_dataFile);
    loader.setPropertyValue("OutputWorkspace", outWSName);
    loader.setPropertyValue("Theta", "theta");
    TS_ASSERT_THROWS_ANYTHING(loader.execute(););
  }

  void testThetaUserDefinedD17() {
    loader.initialize();
    loader.setPropertyValue("Filename", m_dataFile);
    loader.setPropertyValue("OutputWorkspace", outWSName);
    loader.setPropertyValue("Theta", "san");
    loader.setProperty("ThetaUserDefined", 0.5);
    TS_ASSERT_THROWS_ANYTHING(loader.execute(););
  }

  void testWavelengthD17() {
    loader.initialize();
    loader.setPropertyValue("Filename", m_dataFile);
    loader.setPropertyValue("OutputWorkspace", outWSName);
    loader.setPropertyValue("XUnit", "Wavelength");
    TS_ASSERT_THROWS_NOTHING(loader.execute(););
    MatrixWorkspace_sptr output =
        AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(outWSName);
    TS_ASSERT(output);
    TS_ASSERT_EQUALS(output->getAxis(0)->unit()->unitID(), "Wavelength");
    // Test x values, minimum and maximum
    double minimum_wavelength = output->x(2)[0];
    TS_ASSERT_DELTA(minimum_wavelength, -0.23369886776335402, 1e-6);
    double maximum_wavelength = output->x(2)[1000];
    TS_ASSERT_DELTA(maximum_wavelength, 30.784049961143634, 1e-6);
    // Remove workspace from the data service.
    AnalysisDataService::Instance().clear();
  }

  void testTOFD17() {
    loader.initialize();
    loader.setPropertyValue("Filename", m_dataFile);
    loader.setPropertyValue("OutputWorkspace", outWSName);
    loader.setPropertyValue("XUnit", "TimeOfFlight");
    TS_ASSERT_THROWS_NOTHING(loader.execute(););
    MatrixWorkspace_sptr output =
        AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(outWSName);
    TS_ASSERT(output);
    TS_ASSERT_EQUALS(output->getAxis(0)->unit()->unitID(), "TOF");
    // Test x values, minimum and maximum
    double minimum_wavelength = output->x(2)[0];
    TS_ASSERT_DELTA(minimum_wavelength, -429.4584, 1e-6);
    double maximum_wavelength = output->x(2)[1000];
    TS_ASSERT_DELTA(maximum_wavelength, 56570.5415, 1e-6);
    // Remove workspace from the data service.
    AnalysisDataService::Instance().clear();
  }

  void test2ThetaD17() {
    loader.initialize();
    loader.setPropertyValue("Filename", m_dataFile);
    loader.setPropertyValue("OutputWorkspace", outWSName);
    loader.setPropertyValue("Theta", "san");
    loader.execute();
    MatrixWorkspace_sptr output =
        AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(outWSName);
    TS_ASSERT(output);
    // Compare angles in rad
    double sampleAngle =
        m_loader.getPropertyFromRun<double>(output, "san.value");
    const auto &spectrumInfo = output->spectrumInfo();
    // Check twoTheta between two center detectors 128 and 129 using workspace
    // indices
    TS_ASSERT_LESS_THAN_EQUALS(spectrumInfo.twoTheta(130) * 180.0 / M_PI,
                               2. * sampleAngle);
    // Remove workspace from the data service.
    AnalysisDataService::Instance().clear();
  }
};

#endif /* MANTID_DATAHANDLING_LOADILLREFLECTOMETRYTEST_H_ */
