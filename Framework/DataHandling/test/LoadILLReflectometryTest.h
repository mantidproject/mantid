// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <cxxtest/TestSuite.h>

#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/Axis.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/SpectrumInfo.h"
#include "MantidDataHandling/LoadILLReflectometry.h"
#include "MantidDataObjects/TableWorkspace.h"
#include "MantidGeometry/Instrument.h"
#include "MantidGeometry/Instrument/RectangularDetector.h"
#include "MantidKernel/Unit.h"
#include "MantidTypes/Core/DateAndTimeHelpers.h"

using namespace Mantid::API;
using Mantid::DataHandling::LoadILLReflectometry;
using Mantid::Kernel::V3D;

class LoadILLReflectometryTest : public CxxTest::TestSuite {
private:
  const std::string m_d17DirectBeamFile{"ILL/D17/317369.nxs"};
  const std::string m_d17File{"ILL/D17/317370.nxs"};  
  const std::string m_d17File_2018{"ILL/D17/000001.nxs"};
  const std::string m_d17Cycle203File{"ILL/D17/564343.nxs"};
  const std::string m_figaroDirectBeamFile{"ILL/Figaro/709922.nxs"};
  const std::string m_figaroReflectedBeamFile{"ILL/Figaro/709886.nxs"};
  // Name of the default output workspace
  const std::string m_outWSName{"LoadILLReflectometryTest_OutputWS"};

  static void commonProperties(const MatrixWorkspace_sptr &output,
                               const std::string &instrName) {
    TS_ASSERT(output->isHistogramData())
    const auto &spectrumInfo = output->spectrumInfo();
    const auto spectrumInfoSize = spectrumInfo.size();
    TS_ASSERT(spectrumInfo.isMonitor(spectrumInfoSize - 1))
    TS_ASSERT(spectrumInfo.isMonitor(spectrumInfoSize - 2))
    TS_ASSERT_EQUALS(output->getNumberHistograms(), 256 + 2)
    TS_ASSERT_EQUALS(output->blocksize(), 1000)
    TS_ASSERT_EQUALS(output->getInstrument()->getName(), instrName)
    // check the sum of all detector counts against Nexus file entry detsum
    TS_ASSERT_EQUALS(output->run().getPropertyValueAsType<double>("PSD.detsum"),
                     detCounts(output))
    // spectrum number starts with 0
    TS_ASSERT_EQUALS(output->getSpectrum(0).getSpectrumNo(), 0)
    // detector IDs start with 0
    TS_ASSERT_EQUALS(output->getSpectrum(0).getDetectorIDs(),
                     std::set<Mantid::detid_t>{0})
    // sample log entry must exist
    TS_ASSERT(output->run().hasProperty("reduction.line_position"))

    TS_ASSERT(output->run().hasProperty("start_time"));
    TS_ASSERT(Mantid::Types::Core::DateAndTimeHelpers::stringIsISO8601(
        output->run().getProperty("start_time")->value()));
  }

  static double detCounts(const MatrixWorkspace_sptr &output) {
    // sum of detector counts
    double counts{0.0};
    for (size_t i = 0; i < output->getNumberHistograms(); ++i) {
      if (output->spectrumInfo().isMonitor(i)) {
        continue;
      }
      auto &values = output->y(i);
      counts = std::accumulate(values.begin(), values.end(), counts);
    }
    return counts;
  }

  static auto emptyProperties() {
    return std::vector<std::pair<std::string, std::string>>();
  }

  static void getWorkspaceFor(
      MatrixWorkspace_sptr &output, const std::string &fileName,
      const std::string &outFile,
      const std::vector<std::pair<std::string, std::string>> &properties) {
    bool success = loadSpecific(fileName, outFile, properties);
    if (success) {
      output =
          AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(outFile);
      TS_ASSERT(output);
    }
  }

  static bool loadSpecific(
      const std::string &fileName, const std::string &outFile,
      const std::vector<std::pair<std::string, std::string>> &properties) {
    LoadILLReflectometry loader;
    loader.setRethrows(true);
    TS_ASSERT_THROWS_NOTHING(loader.initialize())
    TS_ASSERT(loader.isInitialized())
    TS_ASSERT_THROWS_NOTHING(loader.setPropertyValue("Filename", fileName))
    TS_ASSERT_THROWS_NOTHING(
        loader.setPropertyValue("OutputWorkspace", outFile))
    for (const auto &p : properties) {
      loader.setPropertyValue(p.first, p.second);
    }
    TS_ASSERT_THROWS_NOTHING(loader.execute())
    TS_ASSERT(loader.isExecuted())
    return loader.isExecuted();
  }

public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static LoadILLReflectometryTest *createSuite() {
    return new LoadILLReflectometryTest();
  }
  static void destroySuite(LoadILLReflectometryTest *suite) { delete suite; }

  void tearDown() override { AnalysisDataService::Instance().clear(); }

  void testName() {
    LoadILLReflectometry loader;
    TS_ASSERT_THROWS_NOTHING(loader.initialize())
    TS_ASSERT(loader.isInitialized())
    TS_ASSERT_EQUALS(loader.name(), "LoadILLReflectometry")
  }

  void testVersion() {
    LoadILLReflectometry loader;
    TS_ASSERT_THROWS_NOTHING(loader.initialize())
    TS_ASSERT_EQUALS(loader.version(), 1)
  }

  void testExecD17() {
    loadSpecific(m_d17File, m_outWSName, emptyProperties());
  }

  void testExecFigaro() {
    loadSpecific(m_figaroDirectBeamFile, m_outWSName, emptyProperties());
  }

  void testTOFD17() {
    MatrixWorkspace_sptr output;
    auto prop = emptyProperties();
    prop.emplace_back("XUnit", "TimeOfFlight");
    getWorkspaceFor(output, m_d17File, m_outWSName, prop);
    TS_ASSERT_EQUALS(output->getAxis(0)->unit()->unitID(), "TOF")
    const auto &run = output->run();
    const auto channelWidth =
        run.getPropertyValueAsType<double>("PSD.time_of_flight_0");
    const auto channelCount = static_cast<size_t>(
        run.getPropertyValueAsType<double>("PSD.time_of_flight_1"));
    const auto tofDelay =
        run.getPropertyValueAsType<double>("PSD.time_of_flight_2");
    const auto chopper1Speed = run.getPropertyValueAsType<double>(
        "VirtualChopper.chopper1_speed_average");
    const auto chopper1Phase = run.getPropertyValueAsType<double>(
        "VirtualChopper.chopper1_phase_average");
    const auto chopper2Phase = run.getPropertyValueAsType<double>(
        "VirtualChopper.chopper2_phase_average");
    const auto pOffset =
        run.getPropertyValueAsType<double>("VirtualChopper.poff");
    const auto openOffset =
        run.getPropertyValueAsType<double>("VirtualChopper.open_offset");
    const auto chopperWindow =
        run.getPropertyValueAsType<double>("ChopperWindow");
    const auto tof0 =
        tofDelay -
        60.e6 * (pOffset - 45. + chopper2Phase - chopper1Phase + openOffset) /
            (2. * 360. * chopper1Speed);
    TS_ASSERT_EQUALS(output->blocksize(), channelCount)
    for (size_t i = 0; i < output->getNumberHistograms(); ++i) {
      const auto &xs = output->x(i);
      for (size_t j = 0; j < xs.size(); ++j) {
        const auto tof = tof0 + static_cast<double>(j) * channelWidth;
        TS_ASSERT_DELTA(xs[j], tof, 1.e-12)
      }
    }
    TS_ASSERT_EQUALS(run.getProperty("PSD.time_of_flight_0")->units(), "")
    TS_ASSERT_EQUALS(run.getProperty("PSD.time_of_flight_1")->units(), "")
    TS_ASSERT_EQUALS(run.getProperty("PSD.time_of_flight_2")->units(), "")
    TS_ASSERT_EQUALS(
        run.getProperty("VirtualChopper.chopper1_speed_average")->units(), "")
    TS_ASSERT_EQUALS(
        run.getProperty("VirtualChopper.chopper1_phase_average")->units(), "")
    TS_ASSERT_EQUALS(
        run.getProperty("VirtualChopper.chopper2_phase_average")->units(), "")
    TS_ASSERT_EQUALS(run.getProperty("VirtualChopper.poff")->units(), "")
    TS_ASSERT_EQUALS(run.getProperty("VirtualChopper.open_offset")->units(), "")
    TS_ASSERT_EQUALS(chopperWindow, 45.)
  }

  void testD17Cycle203ChopperWindow() {
    MatrixWorkspace_sptr output;
    auto prop = emptyProperties();
    prop.emplace_back("XUnit", "TimeOfFlight");
    getWorkspaceFor(output, m_d17Cycle203File, m_outWSName, prop);
    TS_ASSERT_EQUALS(
        output->run().getPropertyValueAsType<double>("ChopperWindow"), 20.)
    TS_ASSERT_DELTA(
        output->run().getPropertyValueAsType<double>("Distance.ChopperGap"),
        0.075, 1e-3)
  }

  void testTOFFigaro() {
    MatrixWorkspace_sptr output;
    auto prop = emptyProperties();
    prop.emplace_back("XUnit", "TimeOfFlight");
    getWorkspaceFor(output, m_figaroDirectBeamFile, m_outWSName, prop);
    TS_ASSERT_EQUALS(output->getAxis(0)->unit()->unitID(), "TOF")
    const auto &run = output->run();
    const auto channelWidth =
        run.getPropertyValueAsType<double>("PSD.time_of_flight_0");
    const auto channelCount = static_cast<size_t>(
        run.getPropertyValueAsType<double>("PSD.time_of_flight_1"));
    const auto tofDelay =
        run.getPropertyValueAsType<double>("PSD.time_of_flight_2") +
        run.getPropertyValueAsType<double>("MainParameters.edelay_delay");
    // Using choppers 1 and 4.
    const auto chopper1Speed =
        run.getPropertyValueAsType<double>("chopper1.rotation_speed");
    const double chopper1Phase{0.}; // The value in NeXus is trash.
    const auto chopper2Phase = run.getPropertyValueAsType<double>("chopper2.phase");
    const auto pOffset = run.getPropertyValueAsType<double>("CollAngle.poff");
    const auto openOffset =
        run.getPropertyValueAsType<double>("CollAngle.open_offset");
    const auto tof0 =
        tofDelay -
        60e6 * (pOffset - 45. + chopper2Phase - chopper1Phase + openOffset) /
            (2. * 360. * chopper1Speed);
    TS_ASSERT_EQUALS(output->blocksize(), channelCount)
    for (size_t i = 0; i < output->getNumberHistograms(); ++i) {
      const auto &xs = output->x(i);
      for (size_t j = 0; j < xs.size(); ++j) {
        const auto tof = tof0 + static_cast<double>(j) * channelWidth;
        TS_ASSERT_DELTA(xs[j], tof, 1.e-12)
      }
    }
    TS_ASSERT_EQUALS(run.getProperty("PSD.time_of_flight_0")->units(), "")
    TS_ASSERT_EQUALS(run.getProperty("PSD.time_of_flight_1")->units(), "")
    TS_ASSERT_EQUALS(run.getProperty("PSD.time_of_flight_2")->units(), "")
    TS_ASSERT_EQUALS(run.getProperty("MainParameters.edelay_delay")->units(), "microsec")
    TS_ASSERT_EQUALS(run.getProperty("chopper1.rotation_speed")->units(), "rpm")
    TS_ASSERT_EQUALS(run.getProperty("chopper2.phase")->units(), "degree")
    TS_ASSERT_EQUALS(run.getProperty("CollAngle.poff")->units(), "uu")
    TS_ASSERT_EQUALS(run.getProperty("CollAngle.open_offset")->units(), "degree")
  }

  void testSampleAndSourcePositionsD17() {
    MatrixWorkspace_sptr output;
    auto prop = emptyProperties();
    prop.emplace_back("Xunit", "TimeOfFlight");
    getWorkspaceFor(output, m_d17File, m_outWSName, prop);
    const auto &run = output->run();
    const auto chopperCentre =
        run.getPropertyValueAsType<double>("VirtualChopper.dist_chop_samp");
    const auto chopperSeparation =
        run.getPropertyValueAsType<double>("Distance.ChopperGap");
    const auto sourceSample = chopperCentre - 0.5 * chopperSeparation;
    const auto &spectrumInfo = output->spectrumInfo();
    const auto l1 = spectrumInfo.l1();
    TS_ASSERT_DELTA(sourceSample, l1, 1e-12)
    const auto samplePos = spectrumInfo.samplePosition();
    const auto sourcePos = spectrumInfo.sourcePosition();
    for (size_t i = 0; i < 3; ++i) {
      TS_ASSERT_EQUALS(samplePos[i], 0.)
    }
    TS_ASSERT_EQUALS(sourcePos.X(), 0.)
    TS_ASSERT_EQUALS(sourcePos.Y(), 0.)
    TS_ASSERT_EQUALS(sourcePos.Z(), -sourceSample)
    TS_ASSERT_EQUALS(run.getProperty("VirtualChopper.dist_chop_samp")->units(),
                     "meter")
    TS_ASSERT_EQUALS(run.getProperty("Distance.ChopperGap")->units(), "meter")
    TS_ASSERT_DELTA(chopperSeparation, 0.082, 1e-3)
  }

  void testSampleAndSourcePositionsFigaro() {
    MatrixWorkspace_sptr output;
    auto prop = emptyProperties();
    prop.emplace_back("Xunit", "TimeOfFlight");
    getWorkspaceFor(output, m_figaroDirectBeamFile, m_outWSName, prop);
    const auto &run = output->run();
    const auto chopperCentre =
        run.getPropertyValueAsType<double>(
            "ChopperSetting.chopperpair_sample_distance") *
        1.e-3;
    const auto incomingDeflectionAngle =
        run.getPropertyValueAsType<double>("CollAngle.actual_coll_angle");
    const auto sampleZOffset =
        run.getPropertyValueAsType<double>("Theta.sample_horizontal_offset") *
        1.e-3;
    const auto sourceSample =
        chopperCentre +
        sampleZOffset / std::cos(incomingDeflectionAngle / 180. * M_PI);
    const auto &spectrumInfo = output->spectrumInfo();
    const auto l1 = spectrumInfo.l1();
    TS_ASSERT_DELTA(sourceSample, l1, 1.e-12)
    const auto samplePos = spectrumInfo.samplePosition();
    const auto sourcePos = spectrumInfo.sourcePosition();
    TS_ASSERT_EQUALS(samplePos.X(), 0.)
    TS_ASSERT_EQUALS(samplePos.Y(), 0.)
    TS_ASSERT_EQUALS(samplePos.Z(), 0.)
    TS_ASSERT_EQUALS(sourcePos.X(), 0.)
    TS_ASSERT_EQUALS(sourcePos.Y(), 0.)
    TS_ASSERT_EQUALS(sourcePos.Z(), -sourceSample)
    TS_ASSERT_EQUALS(run.getProperty("CollAngle.actual_coll_angle")->units(),
                     "uu")
    TS_ASSERT_EQUALS(run.getProperty("Theta.sample_horizontal_offset")->units(),
                     "mm")
    TS_ASSERT_EQUALS(
        run.getProperty("ChopperSetting.chopperpair_sample_distance")->units(),
        "mm")
  }

  void testAngleReflectedBeamD17() {
    MatrixWorkspace_sptr output;
    auto prop = emptyProperties();
    prop.emplace_back("Measurement", "ReflectedBeam");
    prop.emplace_back("BraggAngle", "1.5");
    getWorkspaceFor(output, m_d17File, m_outWSName, prop);
    const auto &spectrumInfo = output->spectrumInfo();
    const auto &run = output->run();
    const double centre =
        run.getPropertyValueAsType<double>("reduction.line_position");
    TS_ASSERT_DELTA(centre, 201.674, 0.001);
    const double centreAngle =
        (spectrumInfo.twoTheta(201) + spectrumInfo.twoTheta(202)) / 2;
    TS_ASSERT_DELTA(centreAngle * 180 / M_PI, 3., 0.1);
  }

  void testAngleDirectBeamD17() {
    MatrixWorkspace_sptr output;
    auto prop = emptyProperties();
    prop.emplace_back("Measurement", "DirectBeam");
    getWorkspaceFor(output, m_d17File, m_outWSName, prop);
    const auto &spectrumInfo = output->spectrumInfo();
    const auto &run = output->run();
    const double centre =
        run.getPropertyValueAsType<double>("reduction.line_position");
    TS_ASSERT_DELTA(centre, 201.674, 0.001);
    const double centreAngle =
        (spectrumInfo.twoTheta(201) + spectrumInfo.twoTheta(202)) / 2;
    TS_ASSERT_DELTA(centreAngle * 180 / M_PI, 0., 0.1);
  }

  void testAngleReflectedBeamFigaro() {
    MatrixWorkspace_sptr output;
    auto prop = emptyProperties();
    prop.emplace_back("Measurement", "ReflectedBeam");
    prop.emplace_back("BraggAngle", "1.5");
    getWorkspaceFor(output, m_figaroReflectedBeamFile, m_outWSName, prop);
    const auto &spectrumInfo = output->spectrumInfo();
    const auto &run = output->run();
    const double centre =
        run.getPropertyValueAsType<double>("reduction.line_position");
    TS_ASSERT_DELTA(centre, 173.38, 0.001);
    const double centreAngle =
        (spectrumInfo.twoTheta(173) + spectrumInfo.twoTheta(174)) / 2;
    TS_ASSERT_DELTA(centreAngle * 180 / M_PI, 3., 0.1);
  }

  void testAngleDirectBeamFigaro() {
    MatrixWorkspace_sptr output;
    auto prop = emptyProperties();
    prop.emplace_back("Measurement", "DirectBeam");
    getWorkspaceFor(output, m_figaroDirectBeamFile, m_outWSName, prop);
    const auto &spectrumInfo = output->spectrumInfo();
    const auto &run = output->run();
    const double centre =
        run.getPropertyValueAsType<double>("reduction.line_position");
    TS_ASSERT_DELTA(centre, 173.38, 0.001);
    const double centreAngle =
        (spectrumInfo.twoTheta(173) + spectrumInfo.twoTheta(174)) / 2;
    TS_ASSERT_DELTA(centreAngle * 180 / M_PI, 0., 0.1);
  }

  void testPropertiesD17() {
    MatrixWorkspace_sptr output;
    getWorkspaceFor(output, m_d17File, m_outWSName, emptyProperties());
    commonProperties(output, "D17");
    AnalysisDataService::Instance().clear();
  }

  void testPropertiesFigaro() {
    MatrixWorkspace_sptr output;
    getWorkspaceFor(output, m_figaroDirectBeamFile, m_outWSName, emptyProperties());
    commonProperties(output, "FIGARO");
  }

  // Following tests are introduced after Nexus file changes.
  // Except of edelay, all new variables can be computed from still existing
  // variables.

  void testMovedNexusEntries() {
    LoadILLReflectometry loader;
    loader.setRethrows(true);
    TS_ASSERT_THROWS_NOTHING(loader.initialize())
    TS_ASSERT(loader.isInitialized())
    TS_ASSERT_THROWS_NOTHING(
        loader.setPropertyValue("Filename", this->m_figaroDirectBeamFile))
    TS_ASSERT_THROWS_NOTHING(
        loader.setPropertyValue("OutputWorkspace", this->m_outWSName))
    TS_ASSERT_THROWS_NOTHING(loader.execute())
    TS_ASSERT(loader.isExecuted())
    const auto output =
        AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(
            this->m_outWSName);
    const auto &run = output->run();
    TS_ASSERT(output)
    TS_ASSERT_EQUALS(run.getProperty("Distance.edelay_delay")->units(),
                     "microsec") // a time in the distance field!
    TS_ASSERT_EQUALS(run.getProperty("Distance.inter-slit_distance")->units(),
                     "mm")
  }

  void testSourceAndSampleLocationsFigaro() {
    // In the following, all distance units are millimeter (proposed by NeXus)!
    using namespace NeXus;
    LoadILLReflectometry loader;
    loader.setRethrows(true);
    TS_ASSERT_THROWS_NOTHING(loader.initialize())
    TS_ASSERT(loader.isInitialized())
    TS_ASSERT_THROWS_NOTHING(
        loader.setPropertyValue("Filename", this->m_figaroDirectBeamFile))
    TS_ASSERT_THROWS_NOTHING(
        loader.setPropertyValue("OutputWorkspace", this->m_outWSName))
    TS_ASSERT_THROWS_NOTHING(loader.execute())
    TS_ASSERT(loader.isExecuted())
    const auto output =
        AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(
            this->m_outWSName);
    TS_ASSERT(output)
    const auto &run = output->run();
    TS_ASSERT_EQUALS(run.getProperty("Distance.D1")->units(), "mm")
    TS_ASSERT_EQUALS(run.getProperty("Distance.D0")->units(), "mm")
    TS_ASSERT_EQUALS(run.getProperty("Distance.dist_chop_samp")->units(), "mm")
  }

  void testSourceAndSampleLocationsD17() {
    // In the following, all distance units are in m (proposed by NeXus)!
    LoadILLReflectometry loader;
    loader.setRethrows(true);
    TS_ASSERT_THROWS_NOTHING(loader.initialize())
    TS_ASSERT(loader.isInitialized())
    TS_ASSERT_THROWS_NOTHING(
        loader.setPropertyValue("Filename", this->m_d17File_2018))
    TS_ASSERT_THROWS_NOTHING(
        loader.setPropertyValue("OutputWorkspace", this->m_outWSName))
    TS_ASSERT_THROWS_NOTHING(loader.execute())
    TS_ASSERT(loader.isExecuted())
    const auto output =
        AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(
            this->m_outWSName);
    const auto &run = output->run();
    TS_ASSERT(output)
    TS_ASSERT_EQUALS(run.getProperty("Distance.D1")->units(), "")
    TS_ASSERT_EQUALS(run.getProperty("Distance.D0")->units(), "")
  }

  void testCurrentDoubleDefinitionsAndUnusedVariablesFigaro() {
    LoadILLReflectometry loader;
    loader.setRethrows(true);
    TS_ASSERT_THROWS_NOTHING(loader.initialize())
    TS_ASSERT(loader.isInitialized())
    TS_ASSERT_THROWS_NOTHING(
        loader.setPropertyValue("Filename", this->m_figaroDirectBeamFile))
    TS_ASSERT_THROWS_NOTHING(
        loader.setPropertyValue("OutputWorkspace", this->m_outWSName))
    TS_ASSERT_THROWS_NOTHING(loader.execute())
    TS_ASSERT(loader.isExecuted())
    const auto output =
        AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(
            this->m_outWSName);
    TS_ASSERT(output)
    const auto v1 = loader.doubleFromRun("Theta.sample_horizontal_offset");
    const auto v2 = loader.doubleFromRun("Distance.sample_changer_horizontal_offset");
    TS_ASSERT_EQUALS(v1, v2)
    // Unused variables -> if used in future they may simplify the loader
    TS_ASSERT_EQUALS(loader.doubleFromRun("Theta.actual_directDan"), 0.)
    TS_ASSERT_EQUALS(loader.doubleFromRun("Theta.actual_directDh"), 0.)
    TS_ASSERT_EQUALS(loader.doubleFromRun("Theta.actual_reflectedDan"), 0.)
    TS_ASSERT_EQUALS(loader.doubleFromRun("Theta.actual_reflectedDh"), 0.)
  }

  void testCurrentDoubleDefinitionsD17() {
    AnalysisDataService::Instance().clear();
    LoadILLReflectometry loader;
    loader.setRethrows(true);
    TS_ASSERT_THROWS_NOTHING(loader.initialize())
    TS_ASSERT(loader.isInitialized())
    TS_ASSERT_THROWS_NOTHING(
        loader.setPropertyValue("Filename", this->m_d17File_2018))
    TS_ASSERT_THROWS_NOTHING(
        loader.setPropertyValue("OutputWorkspace", this->m_outWSName))
    TS_ASSERT_THROWS_NOTHING(loader.execute())
    TS_ASSERT(loader.isExecuted())
    const auto output =
        AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(
            this->m_outWSName);
    const auto run = output->run();
    TS_ASSERT(output)
    const auto v7 = loader.doubleFromRun("VirtualChopper.dist_chop_samp");
    double v8;
    if (run.hasProperty("Distance.dist_chop_samp")) {
      v8 = loader.doubleFromRun("Distance.dist_chop_samp");
      TS_ASSERT_EQUALS(run.getProperty("Distance.dist_chop_samp")->units(), "")
    } else {
      v8 = loader.doubleFromRun("VirtualChopper.dist_chop_samp");
      TS_ASSERT_EQUALS(
          run.getProperty("VirtualChopper.dist_chop_samp")->units(), "meter")
    }
    TS_ASSERT_EQUALS(v7, v8)
  }

  void testSlitConfigurationD17() {
    MatrixWorkspace_sptr output;
    getWorkspaceFor(output, m_d17File, m_outWSName, emptyProperties());
    auto instrument = output->getInstrument();
    auto slit1 = instrument->getComponentByName("slit2");
    auto slit2 = instrument->getComponentByName("slit3");
    // cppcheck-suppress unreadVariable
    const double S2z =
        -output->run().getPropertyValueAsType<double>("Distance.S2toSample") *
        1e-3;
    TS_ASSERT_EQUALS(slit1->getPos(), V3D(0.0, 0.0, S2z))
    // cppcheck-suppress unreadVariable
    const double S3z =
        -output->run().getPropertyValueAsType<double>("Distance.S3toSample") *
        1e-3;
    TS_ASSERT_EQUALS(slit2->getPos(), V3D(0.0, 0.0, S3z))
    const auto &run = output->run();
    TS_ASSERT_EQUALS(run.getProperty("Distance.S2toSample")->units(), "")
    TS_ASSERT_EQUALS(run.getProperty("Distance.S3toSample")->units(), "")
  }

  void testSlitConfigurationFigaro() {
    MatrixWorkspace_sptr output;
    getWorkspaceFor(output, m_figaroDirectBeamFile, m_outWSName, emptyProperties());
    auto instrument = output->getInstrument();
    auto slit1 = instrument->getComponentByName("slit2");
    auto slit2 = instrument->getComponentByName("slit3");
    const auto &run = output->run();
    // The S3 position is missing in the NeXus file; use a hard-coded value.
    const double collimationAngle =
        run.getPropertyValueAsType<double>("CollAngle.actual_coll_angle") /
        180. * M_PI;
    const double sampleOffset = output->run().getPropertyValueAsType<double>(
                                    "Theta.sample_horizontal_offset") *
                                1e-3;
    const double slitZOffset = sampleOffset / std::cos(collimationAngle);
    const double S3z = -0.368 - slitZOffset;
    const double slitSeparation =
        run.getPropertyValueAsType<double>("Distance.S2_S3") * 1e-3;
    const double S2z = S3z - slitSeparation;
    TS_ASSERT_EQUALS(slit1->getPos(), V3D(0.0, 0.0, S2z))
    TS_ASSERT_EQUALS(slit2->getPos(), V3D(0.0, 0.0, S3z))
    TS_ASSERT_EQUALS(run.getProperty("Distance.S2_S3")->units(),
                     "mm")
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
      TS_ASSERT_THROWS_NOTHING(alg->execute())
    }
  }

  void tearDown() override {
    for (int i = 0; i < numberOfIterations; i++) {
      delete loadAlgPtrs[i];
      loadAlgPtrs[i] = nullptr;
    }
    Mantid::API::AnalysisDataService::Instance().remove(m_outWSName);
  }

private:
  std::vector<LoadILLReflectometry *> loadAlgPtrs;

  const int numberOfIterations = 10;

  const std::string inFileName = "ILL/D17/317370.nxs";
  const std::string m_outWSName = "LoadILLReflectomeryWsOut";

  LoadILLReflectometry *setupAlg() {
    LoadILLReflectometry *loader = new LoadILLReflectometry;
    loader->initialize();
    loader->isInitialized();
    loader->setPropertyValue("Filename", inFileName);
    loader->setPropertyValue("OutputWorkspace", m_outWSName);

    loader->setRethrows(true);
    return loader;
  }
};
