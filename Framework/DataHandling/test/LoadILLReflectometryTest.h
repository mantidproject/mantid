#ifndef MANTID_DATAHANDLING_LOADILLREFLECTOMETRYTEST_H_
#define MANTID_DATAHANDLING_LOADILLREFLECTOMETRYTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/Axis.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/SpectrumInfo.h"
#include "MantidDataHandling/LoadEmptyInstrument.h"
#include "MantidDataHandling/LoadILLReflectometry.h"
#include "MantidGeometry/Instrument.h"
#include "MantidKernel/Unit.h"


using namespace Mantid::API;
using Mantid::DataHandling::LoadILLReflectometry;
using Mantid::DataHandling::LoadEmptyInstrument;

class LoadILLReflectometryTest : public CxxTest::TestSuite {
private:
  const std::string m_d17File{"ILLD17-161876-Ni.nxs"};
  const std::string m_figaroFile{"ILL/Figaro/598488.nxs"};

public:
  // Name of the output workspace
  const std::string outWSName{"LoadILLReflectometryTest_OutputWS"};
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static LoadILLReflectometryTest *createSuite() {
    return new LoadILLReflectometryTest();
  }
  static void destroySuite(LoadILLReflectometryTest *suite) { delete suite; }

  void testName() {
    LoadILLReflectometry loader;
    TS_ASSERT_THROWS_NOTHING(loader.initialize());
    TS_ASSERT(loader.isInitialized());
    TS_ASSERT_EQUALS(loader.name(), "LoadILLReflectometry");
  }

  void testVersion() {
    LoadILLReflectometry loader;
    TS_ASSERT_THROWS_NOTHING(loader.initialize());
    TS_ASSERT_EQUALS(loader.version(), 1);
  }

  void testInstrumentD17() { checkInstrument("D17", outWSName); }

  void testInstrumentd17() { checkInstrument("d17", outWSName); }

  void testInstrumentFigaro() { checkInstrument("Figaro", outWSName); }

  void testInstrumentfigaro() { checkInstrument("figaro", outWSName); }

  void testExecD17() { loadSpecific(m_d17File, outWSName); }

  void testExecFigaro() { loadSpecific(m_figaroFile, outWSName); }

  // D17

  void testPropertiesD17() {
    MatrixWorkspace_sptr output;
    getWorkspaceFor(output, m_d17File, outWSName);
    commonProperties(output, "D17");
    TS_ASSERT_EQUALS(
        output->run().getPropertyValueAsType<double>("PSD.time_of_flight_0"),
        57.0);
    TS_ASSERT_EQUALS(output->run().getPropertyValueAsType<double>("dan.value"),
                     3.1909999847412109);
    TS_ASSERT_EQUALS(output->run().getPropertyValueAsType<double>("stheta"),
                     0.013958706061406229);
    TS_ASSERT_EQUALS(output->run().getPropertyValueAsType<double>("stheta"),
                     0.013958706061406229);
    AnalysisDataService::Instance().clear();
  }

  void testInputBraggAngleIsD17() {
    loadSpecificThrows(m_d17File, outWSName, "BraggAngleIs", "user defined");
  }

  void testWavelengthD17() {
    // default "XUnit" = "Wavelength"
    MatrixWorkspace_sptr output;
    getWorkspaceFor(output, m_d17File, outWSName);
    TS_ASSERT_EQUALS(output->getAxis(0)->unit()->unitID(), "Wavelength");
    // Test x values, minimum and maximum, first detector
    TS_ASSERT_EQUALS(output->x(2)[0], -0.23365761888763453);
    TS_ASSERT_EQUALS(output->x(2)[1000], 30.778616441233407);
    AnalysisDataService::Instance().clear();
  }

  void testTOFD17() {
    MatrixWorkspace_sptr output;
    getWorkspaceFor(output, m_d17File, outWSName, "XUnit", "TimeOfFlight");
    TS_ASSERT_EQUALS(output->getAxis(0)->unit()->unitID(), "TOF");
    // Test x values, minimum and maximum, first detector
    TS_ASSERT_EQUALS(output->x(2)[0], -429.45848636496885);
    TS_ASSERT_EQUALS(output->x(2)[1000], 56570.541513635035);
    AnalysisDataService::Instance().clear();
  }

  void test2ThetaD17() {
    // default BraggAngleIs = "sample angle"
    MatrixWorkspace_sptr output;
    getWorkspaceFor(output, m_d17File, outWSName);
    // Compare angles in rad
    const auto &spectrumInfo = output->spectrumInfo();
    // Check twoTheta between two center detectors 128 and 129 using workspace
    // indices
    double san = output->run().getPropertyValueAsType<double>("san.value");
    double dan = output->run().getPropertyValueAsType<double>("dan.value");
    double offsetAngle = dan / 2. * san;
    TS_ASSERT_LESS_THAN_EQUALS(spectrumInfo.twoTheta(130) * 180.0 / M_PI,
                               2. * san + offsetAngle);
    AnalysisDataService::Instance().clear();
  }

  void testDirectBeamD17() {
    LoadILLReflectometry loader;
    loader.setRethrows(true);
    TS_ASSERT_THROWS_NOTHING(loader.initialize());
    TS_ASSERT(loader.isInitialized());
    TS_ASSERT_THROWS_NOTHING(loader.setPropertyValue("Filename", m_d17File));
    TS_ASSERT_THROWS_NOTHING(
        loader.setPropertyValue("OutputWorkspace", outWSName));
    TS_ASSERT_THROWS_NOTHING(
        loader.setPropertyValue("BraggAngleIs", "detector angle"));
    // Direct beam is the reflected beam
    TS_ASSERT_THROWS_NOTHING(loader.setPropertyValue("DirectBeam", m_d17File));
    TS_ASSERT_EQUALS(
        loader.fitReflectometryPeak("DirectBeam", "detector angle"),
        std::vector<double>(0.0, 200.463));
    TS_ASSERT_THROWS_NOTHING(loader.execute(););
    TS_ASSERT(loader.isExecuted());
  }

  void testGaussianFitD17() {
    LoadILLReflectometry loader;
    loader.setRethrows(true);
    TS_ASSERT_THROWS_NOTHING(loader.initialize());
    TS_ASSERT(loader.isInitialized());
    TS_ASSERT_THROWS_NOTHING(loader.setPropertyValue("Filename", m_d17File));
    TS_ASSERT_THROWS_NOTHING(
        loader.setPropertyValue("OutputWorkspace", outWSName));
    TS_ASSERT_THROWS_NOTHING(
        loader.setPropertyValue("BraggAngleIs", "detector angle"));
    // Direct beam is the reflected beam
    TS_ASSERT_THROWS_NOTHING(loader.setPropertyValue("DirectBeam", m_d17File));
    TS_ASSERT_THROWS_NOTHING(loader.execute(););
    TS_ASSERT(loader.isExecuted());
    if (loader.isExecuted()) {
      MatrixWorkspace_sptr output =
          AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(
              outWSName);
      TS_ASSERT(output);
    }
    AnalysisDataService::Instance().clear();
  }

  // Figaro

  void testPropertiesFigaro() {
    MatrixWorkspace_sptr output;
    getWorkspaceFor(output, m_figaroFile, outWSName);
    commonProperties(output, "Figaro");
    TS_ASSERT_EQUALS(
        output->run().getPropertyValueAsType<double>("PSD.time_of_flight_0"),
        40.0);
    TS_ASSERT_DELTA(output->run().getPropertyValueAsType<double>("san.value"),
                    1.3877788e-17, 1e-16);
    TS_ASSERT_EQUALS(output->run().getPropertyValueAsType<double>("stheta"),
                     2.4221309013948832e-19);
    TS_ASSERT_EQUALS(output->run().getPropertyValueAsType<double>("stheta"),
                     2.4221309013948832e-19);
    AnalysisDataService::Instance().clear();
  }

  // helpers

  void checkInstrument(const std::string &instrName,
                       const std::string outFile) {
    LoadEmptyInstrument instr;
    instr.setRethrows(true);
    instr.initialize();
    TS_ASSERT(instr.isInitialized());
    instr.setProperty("OutputWorkspace", outFile);
    instr.setPropertyValue("InstrumentName", instrName);
    TS_ASSERT_THROWS_NOTHING(instr.execute());
    TS_ASSERT(instr.isExecuted());
  }

  bool loadSpecific(const std::string fileName, const std::string outFile,
                    std::string property = "", std::string value = "") {
    LoadILLReflectometry loader;
    loader.setRethrows(true);
    TS_ASSERT_THROWS_NOTHING(loader.initialize());
    TS_ASSERT(loader.isInitialized());
    TS_ASSERT_THROWS_NOTHING(loader.setPropertyValue("Filename", fileName));
    TS_ASSERT_THROWS_NOTHING(
        loader.setPropertyValue("OutputWorkspace", outFile));
    if (property != "" && value != "") {
      loader.setPropertyValue(property, value);
    }
    TS_ASSERT_THROWS_NOTHING(loader.execute(););
    TS_ASSERT(loader.isExecuted());
    return loader.isExecuted();
  }

  void getWorkspaceFor(MatrixWorkspace_sptr &output, const std::string fileName,
                       const std::string outFile, std::string property = "",
                       std::string value = "") {
    bool success = loadSpecific(fileName, outFile, property, value);
    if (success) {
      output =
          AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(outFile);
      TS_ASSERT(output);
    }
  }

  void loadSpecificThrows(const std::string fileName, const std::string outFile,
                          std::string property = "", std::string value = "") {
    LoadILLReflectometry loader;
    loader.setRethrows(true);
    TS_ASSERT_THROWS_NOTHING(loader.initialize());
    TS_ASSERT(loader.isInitialized());
    TS_ASSERT_THROWS_NOTHING(loader.setPropertyValue("Filename", fileName));
    TS_ASSERT_THROWS_NOTHING(
        loader.setPropertyValue("OutputWorkspace", outFile));
    if (property != "" && value != "") {
      TS_ASSERT_THROWS_NOTHING(loader.setPropertyValue(property, value));
    }
    TS_ASSERT_THROWS_ANYTHING(loader.execute(););
  }

  void commonProperties(MatrixWorkspace_sptr output, const std::string& instrName) {
    TS_ASSERT(output->isHistogramData());
    TS_ASSERT(output->spectrumInfo().isMonitor(0));
    TS_ASSERT(output->spectrumInfo().isMonitor(1));
    TS_ASSERT_EQUALS(output->getNumberHistograms(), 256 + 2);
    TS_ASSERT_EQUALS(output->blocksize(), 1000);
    TS_ASSERT_EQUALS(output->run().getProperty("Facility")->value(), "ILL");
    TS_ASSERT_EQUALS(output->getInstrument()->getName(), instrName);
    // check the sum of all detector counts against Nexus file entry detsum
    TS_ASSERT_EQUALS(output->run().getPropertyValueAsType<double>("PSD.detsum"),
                     detCounts(output));
  }

  double detCounts(MatrixWorkspace_sptr output) {
    // sum of detector counts
    double counts{0.0};
    for (size_t i = 2; i < output->getNumberHistograms(); ++i) {
      auto &values = output->y(i);
      counts = std::accumulate(values.begin(), values.end(), counts);
    }
    return counts;
  }
};

#endif /* MANTID_DATAHANDLING_LOADILLREFLECTOMETRYTEST_H_ */
