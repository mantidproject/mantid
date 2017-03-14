#ifndef MANTID_DATAHANDLING_LOADILLREFLECTOMETRYTEST_H_
#define MANTID_DATAHANDLING_LOADILLREFLECTOMETRYTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidDataHandling/LoadILLReflectometry.h"
#include "MantidDataHandling/LoadEmptyInstrument.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/Axis.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/SpectrumInfo.h"
#include "MantidKernel/Unit.h"

using namespace Mantid::API;
using Mantid::DataHandling::LoadILLReflectometry;
using Mantid::DataHandling::LoadEmptyInstrument;

class LoadILLReflectometryTest : public CxxTest::TestSuite {
private:
  const std::string m_d17File{"ILLD17-161876-Ni.nxs"};
  const std::string m_figaroFile{"ILL/Figaro/598488.nxs"};
  LoadILLReflectometry loader;
  LoadEmptyInstrument instr;

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

  void testInstrumentD17() { checkInstrument("D17", outWSName); }

  void testInstrumentd17() { checkInstrument("d17", outWSName); }

  void testInstrumentFigaro() { checkInstrument("Figaro", outWSName); }

  void testInstrumentfigaro() { checkInstrument("figaro", outWSName); }

  void testExecD17() { loadSpecific(m_d17File, outWSName); }

  void testExecFigaro() { loadSpecific(m_figaroFile, outWSName); }

  // D17

  void testPropertiesD17() {
    MatrixWorkspace_sptr output = getWorkspaceFor(m_d17File, outWSName);
    commonProperties(output);
    TS_ASSERT_EQUALS(
        output->run().getPropertyValueAsType<double>("channel_width"), 57.0);
    TS_ASSERT_EQUALS(output->run().getPropertyValueAsType<double>("dan.value"),
                     3.1909999847412109);
    TS_ASSERT_EQUALS(output->run().getPropertyValueAsType<double>("stheta"),
                     0.013958706061406229);
    AnalysisDataService::Instance().clear();
  }

  // void testInputThetaD17() { loadSpecificThrows(m_d17File, outWSName,
  // "BraggAngleIs", "theta");
  // }

  // void testThetaUserDefinedD17() {
  //  loadSpecificThrows(m_d17File, outWSName, "BraggAngle", "0.5");
  //}

  void testWavelengthD17() {
    // default "XUnit" = "Wavelength"
    MatrixWorkspace_sptr output = getWorkspaceFor(m_d17File, outWSName);
    TS_ASSERT_EQUALS(output->getAxis(0)->unit()->unitID(), "Wavelength");
    // Test x values, minimum and maximum, first detector
    TS_ASSERT_DELTA(output->x(2)[0], -0.22089473, 1e-6);
    TS_ASSERT_DELTA(output->x(2)[1000], 30.79137933, 1e-6);
    AnalysisDataService::Instance().clear();
  }

  void testTOFD17() {
    MatrixWorkspace_sptr output =
        getWorkspaceFor(m_d17File, outWSName, "XUnit", "TimeOfFlight");
    TS_ASSERT_EQUALS(output->getAxis(0)->unit()->unitID(), "TOF");
    // Test x values, minimum and maximum, first detector
    TS_ASSERT_DELTA(output->x(2)[0], -406.00052788, 1e-6);
    TS_ASSERT_DELTA(output->x(2)[1000], 56593.99947212, 1e-6);
    AnalysisDataService::Instance().clear();
  }

  void test2ThetaD17() {
    // default BraggAngleIs = "sample angle"
    MatrixWorkspace_sptr output = getWorkspaceFor(m_d17File, outWSName);
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

  // Figaro

  void testPropertiesFigaro() {
    MatrixWorkspace_sptr output = getWorkspaceFor(m_figaroFile, outWSName);
    commonProperties(output);
    TS_ASSERT_EQUALS(
        output->run().getPropertyValueAsType<double>("channel_width"), 40.0);
    TS_ASSERT_DELTA(output->run().getPropertyValueAsType<double>("san.value"),
                    1.3877788e-17, 1e-16);
    TS_ASSERT_EQUALS(output->run().getPropertyValueAsType<double>("stheta"),
                     2.4221309013948832e-19);
    AnalysisDataService::Instance().clear();
  }

  // helpers

  void checkInstrument(const std::string &instrName,
                       const std::string outFile) {
    instr.setRethrows(true);
    instr.initialize();
    TS_ASSERT(instr.isInitialized());
    instr.setProperty("OutputWorkspace", outFile);
    instr.setPropertyValue("InstrumentName", instrName);
    TS_ASSERT_THROWS_NOTHING(instr.execute());
    TS_ASSERT(instr.isExecuted());
  }

  void loadSpecific(const std::string fileName, const std::string outFile,
                    std::string property = "", std::string value = "") {
    loader.setRethrows(true);
    TS_ASSERT_THROWS_NOTHING(loader.setPropertyValue("Filename", fileName));
    TS_ASSERT_THROWS_NOTHING(
        loader.setPropertyValue("OutputWorkspace", outFile));
    if (property != "" && value != "") {
      loader.setPropertyValue(property, value);
    }
    loader.initialize();
    TS_ASSERT_THROWS_NOTHING(loader.execute(););
    TS_ASSERT(loader.isExecuted());
  }

  MatrixWorkspace_sptr getWorkspaceFor(const std::string fileName,
                                       const std::string outFile,
                                       std::string property = "",
                                       std::string value = "") {
    loadSpecific(fileName, outFile, property, value);
    MatrixWorkspace_sptr output =
        AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(outFile);
    TS_ASSERT(output);
    return output;
  }

  void loadSpecificThrows(const std::string fileName, const std::string outFile,
                          std::string property = "", std::string value = "") {
    loader.setRethrows(true);
    TS_ASSERT_THROWS_NOTHING(loader.setPropertyValue("Filename", fileName));
    TS_ASSERT_THROWS_NOTHING(
        loader.setPropertyValue("OutputWorkspace", outFile));
    if (property != "" && value != "") {
      loader.setPropertyValue(property, value);
    }
    loader.initialize();
    TS_ASSERT_THROWS_ANYTHING(loader.execute(););
    TS_ASSERT(loader.isExecuted());
  }

  void commonProperties(MatrixWorkspace_sptr output) {
    TS_ASSERT(output->isHistogramData());
    TS_ASSERT(output->spectrumInfo().isMonitor(0));
    TS_ASSERT(output->spectrumInfo().isMonitor(1));
    TS_ASSERT_EQUALS(output->getNumberHistograms(), 256 + 2);
    TS_ASSERT_EQUALS(output->blocksize(), 1000);
    // check the sum of all detector counts against Nexus file entry detsum
    TS_ASSERT_EQUALS(output->run().getPropertyValueAsType<double>("PSD.detsum"),
                     detCounts(output));
  }

  double detCounts(MatrixWorkspace_sptr output) {
    // sum of detector counts
    double counts{0.0};
    for (size_t i = 2; i < output->getNumberHistograms(); ++i) {
      auto &values = output->y(i);
      for (size_t k = 0; k < output->blocksize(); ++k) {
        counts += values[k];
      }
    }
    return counts;
  }
};

#endif /* MANTID_DATAHANDLING_LOADILLREFLECTOMETRYTEST_H_ */
