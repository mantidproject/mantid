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
    // test defaults: same result as testIncoherentScatteringSampleAngleD17()
    TS_ASSERT_EQUALS(output->run().getPropertyValueAsType<double>("stheta"),
                     0.013958706061406229);
    AnalysisDataService::Instance().clear();
  }

  void testInputInputAngleD17() {
    loadSpecificThrows(m_d17File, outWSName, "InputAngle", "user defined");
  }

  void testWavelengthD17() {
    // default "XUnit" = "Wavelength"
    MatrixWorkspace_sptr output;
    getWorkspaceFor(output, m_d17File, outWSName);
    TS_ASSERT_EQUALS(output->getAxis(0)->unit()->unitID(), "Wavelength");
    // Test x values, minimum and maximum, first detector
    TS_ASSERT_EQUALS(output->x(2)[0],
                     -0.23376651299335527); // with offset: -0.23365761888763453
    TS_ASSERT_EQUALS(output->x(2)[1000],
                     30.792960548344681); // with offset: 30.778616441233407
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
    // default InputAngle = "sample angle"
    MatrixWorkspace_sptr output;
    getWorkspaceFor(output, m_d17File, outWSName);
    // Compare angles in rad
    const auto &spectrumInfo = output->spectrumInfo();
    // Check twoTheta between two center detectors 128 and 129 using workspace
    // indices
    double san = output->run().getPropertyValueAsType<double>("san.value");
    double dan = output->run().getPropertyValueAsType<double>("dan.value");
    double offsetAngle = dan / 2. * san;
    TS_ASSERT_LESS_THAN_EQUALS(spectrumInfo.twoTheta(130) * 180. / M_PI,
                               2. * san + offsetAngle);
    AnalysisDataService::Instance().clear();
  }

  void testIncoherentScatteringSampleAngleD17() {
    // this must be the san.value in rad or stheta
    testScatteringAngle(0.013958706061406229, 1e-16, "sample angle", "incoherent",
                        m_d17File);
  }

  void testCoherentScatteringSampleAngleD17() {
    testScatteringAngle(0.013869106563677843, 1e-8, "sample angle", "coherent",
                        m_d17File);
  }

  // small values due to centre angle is zero
  void testIncoherentScatteringDetectorAngleD17() {
    testScatteringAngle(0.0, 1e-16, "detector angle", "incoherent", m_d17File);
  }

  void testCoherentScatteringDetectorAngleD17() {
    testScatteringAngle(-7.116574826901076e-06, 1e-10, "detector angle", "coherent",
                        m_d17File);
  }

  // user defined input angle of 30.0 degree only needs to be converted to
  // radiant
  void testIncoherentScatteringUserAngleD17() {
    testScatteringAngle(30.0 * M_PI / 180., 1e-16, "user defined", "incoherent",
                        m_d17File);
  }

  void testCoherentScatteringUserAngleD17() {
    testScatteringAngle(30.0 * M_PI / 180., 1e-16, "user defined", "coherent",
                        m_d17File);
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
    // test default inputs
    TS_ASSERT_EQUALS(output->run().getPropertyValueAsType<double>("stheta"),
                     0.01085594758122008);
    AnalysisDataService::Instance().clear();
  }

  void testIncoherentScatteringSampleAngleFigaro() {
    testScatteringAngle(0.01085594758122008, 1e-16, "sample angle", "incoherent",
                        m_figaroFile);
  }

  void testCoherentScatteringSampleAngleFigaro() {
    testScatteringAngle(0.017701593089980518, 1e-7, "sample angle", "coherent",
                        m_figaroFile);
  }

  void testIncoherentScatteringDetectorAngleFigaro() {
    testScatteringAngle(-0.009931402389595764, 1e-8, "detector angle", "incoherent",
                        m_figaroFile);
  }

  void testCoherentScatteringDetectorAngleFigaro() {
    testScatteringAngle(0.01770084511622124, 1e-7, "detector angle", "coherent",
                        m_figaroFile);
  }

  void testCoherentScatteringUserAngleFigaro() {
    testScatteringAngle(0.5304444211070592, 1e-7, "user defined", "coherent",
                        m_figaroFile);
  }

  // user defined input angle of 30.0 degree only needs to be converted to
  // radiant
  void testIncoherentScatteringUserAngleFigaro() {
    testScatteringAngle(30.0 * M_PI / 180., 1e-16, "user defined", "incoherent",
                        m_figaroFile);
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

  void commonProperties(MatrixWorkspace_sptr output,
                        const std::string &instrName) {
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

  void testScatteringAngle(const double comparisonValue, const double delta,
                           const std::string &angle,
                           const std::string &scatteringType,
                           const std::string &file) {
    LoadILLReflectometry loader;
    loader.setRethrows(true);
    TS_ASSERT_THROWS_NOTHING(loader.initialize());
    TS_ASSERT(loader.isInitialized());
    TS_ASSERT_THROWS_NOTHING(loader.setPropertyValue("Filename", file));
    TS_ASSERT_THROWS_NOTHING(
        loader.setPropertyValue("OutputWorkspace", outWSName));
    TS_ASSERT_THROWS_NOTHING(loader.setPropertyValue("InputAngle", angle));
    TS_ASSERT_THROWS_NOTHING(
        loader.setPropertyValue("ScatteringType", scatteringType));
    if (angle == "detector angle") {
      // Direct beam is the reflected beam
      TS_ASSERT_THROWS_NOTHING(loader.setPropertyValue("DirectBeam", file));
    }
    if (angle == "user defined") {
      TS_ASSERT_THROWS_NOTHING(loader.setPropertyValue("BraggAngle", "30.0"))
    }
    TS_ASSERT_THROWS_NOTHING(loader.execute(););
    TS_ASSERT(loader.isExecuted());
    if (loader.isExecuted()) {
      MatrixWorkspace_sptr output =
          AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(
              outWSName);
      TS_ASSERT(output);
      TS_ASSERT_DELTA(output->run().getPropertyValueAsType<double>("stheta"),
                       comparisonValue, delta);
    }
    AnalysisDataService::Instance().clear();
  }
};

  class LoadILLReflectometryTestPerformance : public CxxTest::TestSuite {
  public:
    void setUp() override {
      for (int i = 0; i < numberOfIterations; ++i) {
        loadAlgPtrs.emplace_back(setupAlg());
      }
    }

    void testLoadILLReflectometryPerformance() {
      for (auto alg : loadAlgPtrs) {
        TS_ASSERT_THROWS_NOTHING(alg->execute());
      }
    }

    void tearDown() override {
      for (int i = 0; i < numberOfIterations; i++) {
        delete loadAlgPtrs[i];
        loadAlgPtrs[i] = nullptr;
      }
      Mantid::API::AnalysisDataService::Instance().remove(outWSName);
    }

  private:
    std::vector<LoadILLReflectometry *> loadAlgPtrs;

    const int numberOfIterations = 5;

    const std::string inFileName = "ILLD17-161876-Ni.nxs";
    const std::string outWSName = "LoadILLReflectomeryWsOut";

    LoadILLReflectometry *setupAlg() {
      LoadILLReflectometry *loader = new LoadILLReflectometry;
      loader->initialize();
      loader->isInitialized();
      loader->setPropertyValue("Filename", inFileName);
      loader->setPropertyValue("OutputWorkspace", outWSName);

      loader->setRethrows(true);
      return loader;
    }
  };

#endif /* MANTID_DATAHANDLING_LOADILLREFLECTOMETRYTEST_H_ */
