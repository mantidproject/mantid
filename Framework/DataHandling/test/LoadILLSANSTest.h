// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_DATAHANDLING_LOADILLSANSTEST_H_
#define MANTID_DATAHANDLING_LOADILLSANSTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/Axis.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidDataHandling/LoadILLSANS.h"
#include "MantidGeometry/IComponent.h"
#include "MantidGeometry/Instrument.h"
#include "MantidGeometry/Instrument/DetectorInfo.h"
#include "MantidKernel/ConfigService.h"
#include "MantidKernel/Unit.h"

using Mantid::API::AnalysisDataService;
using Mantid::API::Axis;
using Mantid::API::MatrixWorkspace_const_sptr;
using Mantid::DataHandling::LoadILLSANS;
using Mantid::Geometry::IComponent_const_sptr;
using Mantid::Geometry::Instrument;
using Mantid::Kernel::ConfigService;
using Mantid::Kernel::Unit;
using Mantid::Kernel::V3D;

class LoadILLSANSTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static LoadILLSANSTest *createSuite() { return new LoadILLSANSTest(); }
  static void destroySuite(LoadILLSANSTest *suite) { delete suite; }

  void setUp() override {
    ConfigService::Instance().appendDataSearchSubDir("ILL/D11/");
    ConfigService::Instance().appendDataSearchSubDir("ILL/D22/");
    ConfigService::Instance().appendDataSearchSubDir("ILL/D33/");
    ConfigService::Instance().setFacility("ILL");
  }

  void tearDown() override { AnalysisDataService::Instance().clear(); }

  void test_name() {
    LoadILLSANS alg;
    TS_ASSERT_EQUALS(alg.name(), "LoadILLSANS");
  }

  void test_version() {
    LoadILLSANS alg;
    TS_ASSERT_EQUALS(alg.version(), 1);
  }

  void test_init() {
    LoadILLSANS alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
  }

  void test_D11() {
    LoadILLSANS alg;
    alg.setChild(true);
    alg.initialize();
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("Filename", "010560.nxs"))
    TS_ASSERT_THROWS_NOTHING(
        alg.setPropertyValue("OutputWorkspace", "__unused_for_child"))
    TS_ASSERT_THROWS_NOTHING(alg.execute())
    TS_ASSERT(alg.isExecuted())
    MatrixWorkspace_const_sptr outputWS = alg.getProperty("OutputWorkspace");
    TS_ASSERT(outputWS)
    TS_ASSERT_EQUALS(outputWS->getNumberHistograms(), 128 * 128 + 2)
    TS_ASSERT_EQUALS(outputWS->blocksize(), 1)
    TS_ASSERT(outputWS->detectorInfo().isMonitor(128 * 128))
    TS_ASSERT(outputWS->detectorInfo().isMonitor(128 * 128 + 1))
    TS_ASSERT(outputWS->isHistogramData())
    TS_ASSERT(!outputWS->isDistribution())
    const auto &instrument = outputWS->getInstrument();
    IComponent_const_sptr component =
        instrument->getComponentByName("detector");
    V3D pos = component->getPos();
    TS_ASSERT_DELTA(pos.Z(), 20.007, 1E-3)
    const auto &xAxis = outputWS->x(0).rawData();
    const auto &spec6 = outputWS->y(6).rawData();
    const auto &err6 = outputWS->e(6).rawData();
    TS_ASSERT_EQUALS(xAxis.size(), 2)
    TS_ASSERT_DELTA(xAxis[0], 5.73, 1E-5)
    TS_ASSERT_DELTA(xAxis[1], 6.27, 1E-5)
    TS_ASSERT_EQUALS(spec6[0], 20)
    TS_ASSERT_DELTA(err6[0], sqrt(20), 1E-5)
    const auto unit = outputWS->getAxis(0)->unit()->unitID();
    TS_ASSERT_EQUALS(unit, "Wavelength");
  }

  void test_D22() {
    LoadILLSANS alg;
    alg.setChild(true);
    alg.initialize();
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("Filename", "192068.nxs"))
    TS_ASSERT_THROWS_NOTHING(
        alg.setPropertyValue("OutputWorkspace", "__unused_for_child"))
    TS_ASSERT_THROWS_NOTHING(alg.execute())
    TS_ASSERT(alg.isExecuted())
    MatrixWorkspace_const_sptr outputWS = alg.getProperty("OutputWorkspace");
    TS_ASSERT(outputWS)
    TS_ASSERT_EQUALS(outputWS->getNumberHistograms(), 128 * 256 + 2)
    TS_ASSERT_EQUALS(outputWS->blocksize(), 1)
    TS_ASSERT(outputWS->detectorInfo().isMonitor(128 * 256))
    TS_ASSERT(outputWS->detectorInfo().isMonitor(128 * 256 + 1))
    TS_ASSERT(outputWS->isHistogramData())
    TS_ASSERT(!outputWS->isDistribution())
    const auto &instrument = outputWS->getInstrument();
    IComponent_const_sptr component =
        instrument->getComponentByName("detector");
    V3D pos = component->getPos();
    TS_ASSERT_DELTA(pos.Z(), 8, 0.01)
    TS_ASSERT_DELTA(pos.X(), 0.35, 0.01)
    const auto &xAxis = outputWS->x(0).rawData();
    const auto &spec6 = outputWS->y(6).rawData();
    const auto &err6 = outputWS->e(6).rawData();
    TS_ASSERT_EQUALS(xAxis.size(), 2)
    TS_ASSERT_DELTA(xAxis[0], 4.75, 1E-5)
    TS_ASSERT_DELTA(xAxis[1], 5.25, 1E-5)
    TS_ASSERT_EQUALS(spec6[0], 45)
    TS_ASSERT_DELTA(err6[0], sqrt(45), 1E-5)
    const auto unit = outputWS->getAxis(0)->unit()->unitID();
    TS_ASSERT_EQUALS(unit, "Wavelength");
  }

  void test_D33() {
    LoadILLSANS alg;
    alg.setChild(true);
    alg.initialize();
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("Filename", "002294.nxs"))
    TS_ASSERT_THROWS_NOTHING(
        alg.setPropertyValue("OutputWorkspace", "__unused_for_child"))
    TS_ASSERT_THROWS_NOTHING(alg.execute())
    TS_ASSERT(alg.isExecuted())
    MatrixWorkspace_const_sptr outputWS = alg.getProperty("OutputWorkspace");
    TS_ASSERT(outputWS)
    TS_ASSERT_EQUALS(outputWS->getNumberHistograms(), 256 * 256 + 2)
    TS_ASSERT_EQUALS(outputWS->blocksize(), 1)
    TS_ASSERT(outputWS->detectorInfo().isMonitor(256 * 256))
    TS_ASSERT(outputWS->detectorInfo().isMonitor(256 * 256 + 1))
    TS_ASSERT(outputWS->isHistogramData())
    TS_ASSERT(!outputWS->isDistribution())
    const auto &xAxis = outputWS->x(0).rawData();
    const auto &spec = outputWS->y(15947).rawData();
    const auto &err = outputWS->e(15947).rawData();
    TS_ASSERT_EQUALS(xAxis.size(), 2)
    TS_ASSERT_DELTA(xAxis[0], 9.5, 1E-3)
    TS_ASSERT_DELTA(xAxis[1], 10.5, 1E-3)
    TS_ASSERT_EQUALS(spec[0], 220)
    TS_ASSERT_DELTA(err[0], sqrt(220), 1E-5)
    const auto &instrument = outputWS->getInstrument();
    IComponent_const_sptr back =
        instrument->getComponentByName("back_detector");
    TS_ASSERT_EQUALS(back->getPos(), V3D(0, 0, 10.1128));
    IComponent_const_sptr right =
        instrument->getComponentByName("front_detector_right");
    TS_ASSERT_EQUALS(right->getPos(), V3D(-0.41, 0, 1.4968));
    IComponent_const_sptr left =
        instrument->getComponentByName("front_detector_left");
    TS_ASSERT_EQUALS(left->getPos(), V3D(0.41, 0, 1.4968));
    IComponent_const_sptr top =
        instrument->getComponentByName("front_detector_top");
    TS_ASSERT_EQUALS(top->getPos(), V3D(0, 0.41, 1.3118));
    IComponent_const_sptr bottom =
        instrument->getComponentByName("front_detector_bottom");
    TS_ASSERT_EQUALS(bottom->getPos(), V3D(0, -0.41, 1.3118));
    const auto unit = outputWS->getAxis(0)->unit()->unitID();
    TS_ASSERT_EQUALS(unit, "Wavelength");
  }

  void test_D33_TOF() {
    LoadILLSANS alg;
    alg.setChild(true);
    alg.initialize();
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("Filename", "042610.nxs"))
    TS_ASSERT_THROWS_NOTHING(
        alg.setPropertyValue("OutputWorkspace", "__unused_for_child"))
    TS_ASSERT_THROWS_NOTHING(alg.execute())
    TS_ASSERT(alg.isExecuted())
    MatrixWorkspace_const_sptr outputWS = alg.getProperty("OutputWorkspace");
    TS_ASSERT(outputWS)
    TS_ASSERT_EQUALS(outputWS->getNumberHistograms(), 256 * 256 + 2)
    TS_ASSERT_EQUALS(outputWS->blocksize(), 200)
    TS_ASSERT(outputWS->detectorInfo().isMonitor(256 * 256))
    TS_ASSERT(outputWS->detectorInfo().isMonitor(256 * 256 + 1))
    TS_ASSERT(outputWS->isHistogramData())
    TS_ASSERT(!outputWS->isDistribution())
    const auto &x = outputWS->x(0).rawData();
    TS_ASSERT_DELTA(x[0], 0.04969, 1E-5)
    TS_ASSERT_DELTA(x[1], 0.14873, 1E-5)
    TS_ASSERT_DELTA(x[200], 19.85713, 1E-5)
    const auto &run = outputWS->run();
    TS_ASSERT(run.hasProperty("tof_mode"))
    const auto tof = run.getLogData("tof_mode");
    TS_ASSERT_EQUALS(tof->value(), "TOF");
    const auto unit = outputWS->getAxis(0)->unit()->unitID();
    TS_ASSERT_EQUALS(unit, "Wavelength");
  }
};

class LoadILLSANSTestPerformance : public CxxTest::TestSuite {
public:
  void setUp() override {
    m_alg.initialize();
    m_alg.setChild(true);
    m_alg.setPropertyValue("Filename", "ILL/D33/042610.nxs");
    m_alg.setPropertyValue("OutputWorkspace", "__unused_for_child");
  }
  void testLoadILLSANSPerformance() {
    for (int i = 0; i < 10; ++i) {
      TS_ASSERT_THROWS_NOTHING(m_alg.execute());
    }
  }
  void tearDown() override { AnalysisDataService::Instance().clear(); }

private:
  LoadILLSANS m_alg;
};

#endif /* MANTID_DATAHANDLING_LOADILLSANSTEST_H_ */
