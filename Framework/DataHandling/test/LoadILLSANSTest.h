// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

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
#include "MantidTypes/Core/DateAndTimeHelpers.h"

using Mantid::API::AnalysisDataService;
using Mantid::API::Axis;
using Mantid::API::MatrixWorkspace_const_sptr;
using Mantid::DataHandling::LoadILLSANS;
using Mantid::Geometry::IComponent_const_sptr;
using Mantid::Geometry::IDetector_const_sptr;
using Mantid::Geometry::Instrument;
using Mantid::Kernel::ConfigService;
using Mantid::Kernel::Unit;
using Mantid::Kernel::V3D;
using detid2det_map = std::map<Mantid::detid_t, IDetector_const_sptr>;

class LoadILLSANSTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static LoadILLSANSTest *createSuite() { return new LoadILLSANSTest(); }
  static void destroySuite(LoadILLSANSTest *suite) { delete suite; }

  LoadILLSANSTest() {
    ConfigService::Instance().appendDataSearchSubDir("ILL/D11/");
    ConfigService::Instance().appendDataSearchSubDir("ILL/D11B/");
    ConfigService::Instance().appendDataSearchSubDir("ILL/D22/");
    ConfigService::Instance().appendDataSearchSubDir("ILL/D22B/");
    ConfigService::Instance().appendDataSearchSubDir("ILL/D33/");
    ConfigService::Instance().appendDataSearchSubDir("ILL/D16/");
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
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("OutputWorkspace", "__unused_for_child"))
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
    IComponent_const_sptr component = instrument->getComponentByName("detector");
    V3D pos = component->getPos();
    TS_ASSERT_DELTA(pos.Z(), 20.007, 1E-3)
    // check for the correct unit
    const auto unit = outputWS->getAxis(0)->unit()->unitID();
    TS_ASSERT_EQUALS(unit, "Wavelength");
    // check loaded data contains expected values
    const auto &xAxis = outputWS->x(0).rawData();
    const auto &spec6 = outputWS->y(6).rawData();
    const auto &err6 = outputWS->e(6).rawData();
    TS_ASSERT_EQUALS(xAxis.size(), 2)
    TS_ASSERT_DELTA(xAxis[0], 5.73, 1E-5)
    TS_ASSERT_DELTA(xAxis[1], 6.27, 1E-5)
    TS_ASSERT_EQUALS(spec6[0], 20)
    TS_ASSERT_DELTA(err6[0], sqrt(20), 1E-5)
    TS_ASSERT_DELTA(outputWS->y(0)[0], 0.0, 1E-5)
    TS_ASSERT_DELTA(outputWS->y(1)[0], 3.0, 1E-5)
    TS_ASSERT_DELTA(outputWS->y(16384)[0], 10418891.0, 1E-5)
    TS_ASSERT_DELTA(outputWS->y(16385)[0], 0.0, 1E-5)

    checkTimeFormat(outputWS);
    checkDuration(outputWS, 1200.);
    checkWavelength(outputWS, 6.);
  }

  void test_D11B() {
    LoadILLSANS alg;
    alg.setChild(true);
    alg.initialize();
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("Filename", "000410.nxs"))
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("OutputWorkspace", "__unused_for_child"))
    TS_ASSERT_THROWS_NOTHING(alg.execute())
    TS_ASSERT(alg.isExecuted())
    MatrixWorkspace_const_sptr outputWS = alg.getProperty("OutputWorkspace");
    TS_ASSERT(outputWS)
    TS_ASSERT_EQUALS(outputWS->getNumberHistograms(), 192 * 256 + 2 * 32 * 256 + 2)
    TS_ASSERT_EQUALS(outputWS->blocksize(), 1)
    TS_ASSERT(outputWS->detectorInfo().isMonitor(192 * 256 + 2 * 32 * 256))
    TS_ASSERT(outputWS->detectorInfo().isMonitor(192 * 256 + 2 * 32 * 256 + 1))
    TS_ASSERT(outputWS->isHistogramData())
    TS_ASSERT(!outputWS->isDistribution())
    const auto &instrument = outputWS->getInstrument();
    const auto &run = outputWS->run();
    TS_ASSERT(run.hasProperty("Detector 1.det_calc"));
    TS_ASSERT(run.hasProperty("L2"));
    const double detCalc = run.getPropertyAsSingleValue("Detector 1.det_calc");
    const double l2 = run.getPropertyAsSingleValue("L2");
    TS_ASSERT_EQUALS(detCalc, l2);
    const double panelOffset = 0.105;
    IComponent_const_sptr component = instrument->getComponentByName("detector_center");
    V3D pos = component->getPos();
    TS_ASSERT_DELTA(pos.Z(), l2, 1E-5)
    component = instrument->getComponentByName("detector_left");
    pos = component->getPos();
    TS_ASSERT_DELTA(pos.Z(), l2 - panelOffset, 1E-5)
    component = instrument->getComponentByName("detector_right");
    pos = component->getPos();
    TS_ASSERT_DELTA(pos.Z(), l2 - panelOffset, 1E-5)
    // check for the correct unit
    const auto unit = outputWS->getAxis(0)->unit()->unitID();
    TS_ASSERT_EQUALS(unit, "Wavelength");
    // check loaded data contains expected values
    const auto &xAxis = outputWS->x(0).rawData();
    TS_ASSERT_EQUALS(outputWS->blocksize(), 1);
    TS_ASSERT_EQUALS(xAxis.size(), 2)
    TS_ASSERT_DELTA(xAxis[0], 5.73, 1E-5)
    TS_ASSERT_DELTA(xAxis[1], 6.27, 1E-5)
    TS_ASSERT_DELTA(outputWS->y(0)[0], 0.0, 1E-5)
    TS_ASSERT_DELTA(outputWS->y(192 * 256 + 2 * 32 * 256)[0], 1.0, 1E-5)
    TS_ASSERT_DELTA(outputWS->y(192 * 256 + 2 * 32 * 256 + 1)[0], 0.0, 1E-5)

    checkTimeFormat(outputWS);
    checkDuration(outputWS, 600.);
    checkWavelength(outputWS, 6.);
  }

  void test_D11B_Kinetic() {
    LoadILLSANS alg;
    alg.setChild(true);
    alg.initialize();
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("Filename", "017177.nxs"))
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("OutputWorkspace", "__unused_for_child"))
    TS_ASSERT_THROWS_NOTHING(alg.execute())
    TS_ASSERT(alg.isExecuted())
    MatrixWorkspace_const_sptr outputWS = alg.getProperty("OutputWorkspace");
    TS_ASSERT(outputWS)
    TS_ASSERT_EQUALS(outputWS->getNumberHistograms(), 192 * 256 + 2 * 32 * 256 + 2)
    TS_ASSERT(!outputWS->detectorInfo().isMonitor(0))
    TS_ASSERT(outputWS->detectorInfo().isMonitor(192 * 256 + 2 * 32 * 256))
    TS_ASSERT(outputWS->detectorInfo().isMonitor(192 * 256 + 2 * 32 * 256 + 1))
    TS_ASSERT(!outputWS->isHistogramData())
    TS_ASSERT(!outputWS->isDistribution())
    TS_ASSERT(outputWS->isCommonBins())
    const auto &instrument = outputWS->getInstrument();
    const auto &run = outputWS->run();
    TS_ASSERT(run.hasProperty("Detector 1.det_calc"));
    TS_ASSERT(run.hasProperty("L2"));
    const double detCalc = run.getPropertyAsSingleValue("Detector 1.det_calc");
    const double l2 = run.getPropertyAsSingleValue("L2");
    TS_ASSERT_EQUALS(detCalc, l2);
    const double panelOffset = 0.105;
    const double lambda = run.getPropertyAsSingleValue("wavelength");
    TS_ASSERT_EQUALS(lambda, 6.);
    IComponent_const_sptr component = instrument->getComponentByName("detector_center");
    V3D pos = component->getPos();
    TS_ASSERT_DELTA(pos.Z(), l2, 1E-5)
    component = instrument->getComponentByName("detector_left");
    pos = component->getPos();
    TS_ASSERT_DELTA(pos.Z(), l2 - panelOffset, 1E-5)
    component = instrument->getComponentByName("detector_right");
    pos = component->getPos();
    TS_ASSERT_DELTA(pos.Z(), l2 - panelOffset, 1E-5)
    const size_t nExpectedFrames = 85;
    TS_ASSERT_EQUALS(outputWS->blocksize(), nExpectedFrames)
    const auto &xAxis = outputWS->x(0).rawData();
    TS_ASSERT_EQUALS(outputWS->blocksize(), nExpectedFrames);
    TS_ASSERT_EQUALS(xAxis.size(), nExpectedFrames)
    TS_ASSERT_DELTA(xAxis[0], 0, 1E-9)
    TS_ASSERT_DELTA(xAxis[13], 13, 1E-9)
    // check loaded data contains expected values
    const auto &mon1 = outputWS->y(192 * 256 + 2 * 32 * 256).rawData();
    const auto &mon1err = outputWS->e(192 * 256 + 2 * 32 * 256).rawData();
    TS_ASSERT_EQUALS(mon1.size(), nExpectedFrames)
    TS_ASSERT_EQUALS(mon1[0], 367)
    TS_ASSERT_EQUALS(mon1err.size(), nExpectedFrames)
    TS_ASSERT_DELTA(mon1err[0], std::sqrt(367), 1E-9)
    const auto &mon2 = outputWS->y(192 * 256 + 2 * 32 * 256 + 1).rawData();
    const auto &mon2err = outputWS->e(192 * 256 + 2 * 32 * 256 + 1).rawData();
    TS_ASSERT_EQUALS(mon2.size(), nExpectedFrames)
    TS_ASSERT_DELTA(mon2[0], 0.05, 1E-3)
    TS_ASSERT_EQUALS(mon2err.size(), nExpectedFrames)
    TS_ASSERT_DELTA(mon2err[0], 0, 1E-9)
    TS_ASSERT_DELTA(outputWS->y(0)[0], 0, 1E-5)
    TS_ASSERT_DELTA(outputWS->y(0)[84], 1.0, 1E-5)
    // check for the correct unit
    const auto unit = outputWS->getAxis(0)->unit()->unitID();
    TS_ASSERT_EQUALS(unit, "Empty");

    checkTimeFormat(outputWS);
    checkDuration(outputWS, 0.);
    checkWavelength(outputWS, 6.);
  }

  void test_D22() {
    LoadILLSANS alg;
    alg.setChild(true);
    alg.initialize();
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("Filename", "192068.nxs"))
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("OutputWorkspace", "__unused_for_child"))
    TS_ASSERT_THROWS_NOTHING(alg.execute())
    TS_ASSERT(alg.isExecuted())
    MatrixWorkspace_const_sptr outputWS = alg.getProperty("OutputWorkspace");
    TS_ASSERT(outputWS)
    TS_ASSERT_EQUALS(outputWS->getNumberHistograms(), 128 * 256 + 2)
    TS_ASSERT_EQUALS(outputWS->blocksize(), 1)
    TS_ASSERT(!outputWS->detectorInfo().isMonitor(0))
    TS_ASSERT(outputWS->detectorInfo().isMonitor(128 * 256))
    TS_ASSERT(outputWS->detectorInfo().isMonitor(128 * 256 + 1))
    TS_ASSERT(outputWS->isHistogramData())
    TS_ASSERT(!outputWS->isDistribution())
    const auto &instrument = outputWS->getInstrument();
    IComponent_const_sptr component = instrument->getComponentByName("detector");
    V3D pos = component->getPos();
    TS_ASSERT_DELTA(pos.Z(), 8, 0.01)
    TS_ASSERT_DELTA(pos.X(), -0.35, 0.01)
    // check loaded data contains expected values
    const auto &xAxis = outputWS->x(0).rawData();
    const auto &spec6 = outputWS->y(6).rawData();
    const auto &err6 = outputWS->e(6).rawData();
    TS_ASSERT_EQUALS(xAxis.size(), 2)
    TS_ASSERT_DELTA(xAxis[0], 4.75, 1E-5)
    TS_ASSERT_DELTA(xAxis[1], 5.25, 1E-5)
    TS_ASSERT_EQUALS(spec6[0], 45)
    TS_ASSERT_DELTA(err6[0], sqrt(45), 1E-5)
    TS_ASSERT_DELTA(outputWS->y(128 * 256)[0], 245681.0, 1E-5)
    TS_ASSERT_DELTA(outputWS->e(128 * 256)[0], 495.66218, 1E-5)
    TS_ASSERT_DELTA(outputWS->y(128 * 256 + 1)[0], 0.0, 1E-5)
    TS_ASSERT_DELTA(outputWS->e(128 * 256 + 1)[0], 0.0, 1E-5)
    // check for the correct unit
    const auto unit = outputWS->getAxis(0)->unit()->unitID();
    TS_ASSERT_EQUALS(unit, "Wavelength");
    checkTimeFormat(outputWS);
    checkDuration(outputWS, 120.);
    checkWavelength(outputWS, 5.);
  }

  void test_D22B_Cycle211() {
    // During cycle 211 the front detector was detector 1, back detector was detector 2
    // This wasn't consistent with the rest of the instruments, so was swapped from 212 on
    LoadILLSANS alg;
    alg.setChild(true);
    alg.initialize();
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("Filename", "000180.nxs"))
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("OutputWorkspace", "__unused_for_child"))
    TS_ASSERT_THROWS_NOTHING(alg.execute())
    TS_ASSERT(alg.isExecuted())
    MatrixWorkspace_const_sptr outputWS = alg.getProperty("OutputWorkspace");
    TS_ASSERT(outputWS)
    TS_ASSERT_EQUALS(outputWS->getNumberHistograms(), 128 * 256 + 96 * 256 + 2)
    TS_ASSERT_EQUALS(outputWS->blocksize(), 1)
    TS_ASSERT(!outputWS->detectorInfo().isMonitor(0))
    TS_ASSERT(outputWS->detectorInfo().isMonitor(128 * 256 + 96 * 256))
    TS_ASSERT(outputWS->detectorInfo().isMonitor(128 * 256 + 96 * 256 + 1))
    TS_ASSERT(outputWS->isHistogramData())
    TS_ASSERT(!outputWS->isDistribution())
    TS_ASSERT_EQUALS(outputWS->getAxis(0)->unit()->unitID(), "Wavelength");
    const auto &instrument = outputWS->getInstrument();
    const auto &run = outputWS->run();
    IComponent_const_sptr comp = instrument->getComponentByName("detector_back");
    V3D pos = comp->getPos();
    TS_ASSERT(run.hasProperty("Detector 2.det2_calc"))
    double det2_calc = run.getLogAsSingleValue("Detector 2.det2_calc");
    TS_ASSERT(run.hasProperty("Detector 2.dtr2_actual"))
    double dtr2_act = run.getLogAsSingleValue("Detector 2.dtr2_actual");
    TS_ASSERT_DELTA(pos.Z(), det2_calc, 1E-6)
    TS_ASSERT_DELTA(pos.X(), -dtr2_act / 1000., 1E-6)
    TS_ASSERT(run.hasProperty("L2"))
    double l2 = run.getLogAsSingleValue("L2");
    TS_ASSERT_DELTA(l2, det2_calc, 1E-6)
    comp = instrument->getComponentByName("detector_front");
    pos = comp->getPos();
    TS_ASSERT(run.hasProperty("Detector 1.det1_calc"))
    double det1_calc = run.getLogAsSingleValue("Detector 1.det1_calc");
    TS_ASSERT(run.hasProperty("Detector 1.dtr1_actual"))
    double dtr1_act = run.getLogAsSingleValue("Detector 1.dtr1_actual");
    TS_ASSERT_DELTA(pos.Z(), det1_calc, 1E-6)
    TS_ASSERT_DELTA(pos.X(), -dtr1_act / 1000., 1E-6)
    TS_ASSERT(run.hasProperty("Detector 1.dan1_actual"))
    double dan1_act = run.getLogAsSingleValue("Detector 1.dan1_actual");
    double angle, qx, qy, qz;
    comp->getRotation().getAngleAxis(angle, qx, qy, qz);
    TS_ASSERT_DELTA(angle, dan1_act, 1E-6)
    TS_ASSERT_EQUALS(qx, 0.)
    TS_ASSERT_DELTA(std::fabs(qy), 1., 1E-6)
    TS_ASSERT_EQUALS(qz, 0.)
    // check loaded data contains expected values
    const auto &xAxis = outputWS->x(0).rawData();
    TS_ASSERT_DELTA(xAxis[0], 5.7, 1E-5)
    TS_ASSERT_DELTA(xAxis[1], 6.3, 1E-5)
    TS_ASSERT_DELTA(outputWS->y(0)[0], 0.0, 1E-5)
    TS_ASSERT_DELTA(outputWS->e(0)[0], 0.0, 1E-5)
    TS_ASSERT_DELTA(outputWS->y(128 * 256 + 96 * 256)[0], 0.0, 1E-5)
    TS_ASSERT_DELTA(outputWS->e(128 * 256 + 96 * 256)[0], 0.0, 1E-5)
    checkTimeFormat(outputWS);
    checkDuration(outputWS, 60.);
    checkWavelength(outputWS, 6.);
  }

  void test_D22B() {
    LoadILLSANS alg;
    alg.setChild(true);
    alg.initialize();
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("Filename", "046600.nxs"))
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("OutputWorkspace", "__unused_for_child"))
    TS_ASSERT_THROWS_NOTHING(alg.execute())
    TS_ASSERT(alg.isExecuted())
    MatrixWorkspace_const_sptr outputWS = alg.getProperty("OutputWorkspace");
    TS_ASSERT(outputWS)
    TS_ASSERT_EQUALS(outputWS->getNumberHistograms(), 128 * 256 + 96 * 256 + 2)
    TS_ASSERT_EQUALS(outputWS->blocksize(), 1)
    TS_ASSERT(!outputWS->detectorInfo().isMonitor(0))
    TS_ASSERT(outputWS->detectorInfo().isMonitor(128 * 256 + 96 * 256))
    TS_ASSERT(outputWS->detectorInfo().isMonitor(128 * 256 + 96 * 256 + 1))
    TS_ASSERT(outputWS->isHistogramData())
    TS_ASSERT(!outputWS->isDistribution())
    TS_ASSERT_EQUALS(outputWS->getAxis(0)->unit()->unitID(), "Wavelength");
    const auto &instrument = outputWS->getInstrument();
    const auto &run = outputWS->run();
    IComponent_const_sptr comp = instrument->getComponentByName("detector_back");
    V3D pos = comp->getPos();
    TS_ASSERT(run.hasProperty("Detector 1.det1_calc"))
    double det2_calc = run.getLogAsSingleValue("Detector 1.det1_calc");
    TS_ASSERT(run.hasProperty("Detector 1.dtr1_actual"))
    double dtr2_act = run.getLogAsSingleValue("Detector 1.dtr1_actual");
    TS_ASSERT_DELTA(pos.Z(), det2_calc, 1E-6)
    TS_ASSERT_DELTA(pos.X(), -dtr2_act / 1000., 1E-6)
    TS_ASSERT(run.hasProperty("L2"))
    double l2 = run.getLogAsSingleValue("L2");
    TS_ASSERT_DELTA(l2, det2_calc, 1E-6)
    comp = instrument->getComponentByName("detector_front");
    pos = comp->getPos();
    TS_ASSERT(run.hasProperty("Detector 2.det2_calc"))
    double det1_calc = run.getLogAsSingleValue("Detector 2.det2_calc");
    TS_ASSERT(run.hasProperty("Detector 2.dtr2_actual"))
    double dtr1_act = run.getLogAsSingleValue("Detector 2.dtr2_actual");
    TS_ASSERT_DELTA(pos.Z(), det1_calc, 1E-6)
    TS_ASSERT_DELTA(pos.X(), -dtr1_act / 1000., 1E-6)
    TS_ASSERT(run.hasProperty("Detector 2.dan2_actual"))
    double dan1_act = run.getLogAsSingleValue("Detector 2.dan2_actual");
    double angle, qx, qy, qz;
    comp->getRotation().getAngleAxis(angle, qx, qy, qz);
    TS_ASSERT_DELTA(angle, dan1_act, 1E-6)
    TS_ASSERT_EQUALS(qx, 0.)
    TS_ASSERT_DELTA(std::fabs(qy), 1., 1E-6)
    TS_ASSERT_EQUALS(qz, 0.)
    // check loaded data contains expected values
    const auto &xAxis = outputWS->x(0).rawData();
    TS_ASSERT_DELTA(xAxis[0], 10.4595, 1E-5)
    TS_ASSERT_DELTA(xAxis[1], 11.5605, 1E-5)
    TS_ASSERT_DELTA(outputWS->y(17263)[0], 100.0, 1E-5)
    TS_ASSERT_DELTA(outputWS->e(17263)[0], 10.0, 1E-5)
    TS_ASSERT_DELTA(outputWS->y(128 * 256 + 96 * 256)[0], 74361.0, 1E-5)
    TS_ASSERT_DELTA(outputWS->e(128 * 256 + 96 * 256)[0], 272.69213, 1E-5)
    TS_ASSERT_DELTA(outputWS->y(128 * 256 + 96 * 256 + 1)[0], 0.0, 1E-5)
    TS_ASSERT_DELTA(outputWS->e(128 * 256 + 96 * 256 + 1)[0], 0.0, 1E-5)
    checkTimeFormat(outputWS);
    checkDuration(outputWS, 60.);
    checkWavelength(outputWS, 11.01);
  }

  void test_D22B_Kinetic() {
    LoadILLSANS alg;
    alg.setChild(true);
    alg.initialize();
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("Filename", "089120.nxs"))
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("OutputWorkspace", "__unused_for_child"))
    TS_ASSERT_THROWS_NOTHING(alg.execute())
    TS_ASSERT(alg.isExecuted())
    MatrixWorkspace_const_sptr outputWS = alg.getProperty("OutputWorkspace");
    TS_ASSERT(outputWS)
    TS_ASSERT_EQUALS(outputWS->getNumberHistograms(), 128 * 256 + 96 * 256 + 2)
    TS_ASSERT(!outputWS->detectorInfo().isMonitor(0))
    TS_ASSERT(outputWS->detectorInfo().isMonitor(128 * 256 + 96 * 256 + 1))
    TS_ASSERT(outputWS->detectorInfo().isMonitor(128 * 256 + 96 * 256))
    TS_ASSERT(!outputWS->isHistogramData())
    TS_ASSERT(!outputWS->isDistribution())
    TS_ASSERT(outputWS->isCommonBins())
    TS_ASSERT_EQUALS(outputWS->blocksize(), 400)
    const auto &instrument = outputWS->getInstrument();
    const auto &run = outputWS->run();
    TS_ASSERT(run.hasProperty("Detector 1.det1_calc"));
    TS_ASSERT(run.hasProperty("L2"));
    const double detCalc = run.getPropertyAsSingleValue("Detector 1.det1_calc");
    const double l2 = run.getPropertyAsSingleValue("L2");
    TS_ASSERT_EQUALS(detCalc, l2);
    IComponent_const_sptr comp = instrument->getComponentByName("detector_back");
    V3D pos = comp->getPos();
    TS_ASSERT(run.hasProperty("Detector 1.det1_calc"))
    double det2_calc = run.getLogAsSingleValue("Detector 1.det1_calc");
    TS_ASSERT(run.hasProperty("Detector 1.dtr1_actual"))
    double dtr2_act = run.getLogAsSingleValue("Detector 1.dtr1_actual");
    TS_ASSERT_DELTA(pos.Z(), det2_calc, 1E-6)
    TS_ASSERT_DELTA(pos.X(), -dtr2_act / 1000., 1E-6)
    comp = instrument->getComponentByName("detector_front");
    pos = comp->getPos();
    TS_ASSERT(run.hasProperty("Detector 2.det2_calc"))
    double det1_calc = run.getLogAsSingleValue("Detector 2.det2_calc");
    TS_ASSERT(run.hasProperty("Detector 2.dtr2_actual"))
    double dtr1_act = run.getLogAsSingleValue("Detector 2.dtr2_actual");
    TS_ASSERT_DELTA(pos.Z(), det1_calc, 1E-6)
    TS_ASSERT_DELTA(pos.X(), -dtr1_act / 1000., 1E-6)
    TS_ASSERT(run.hasProperty("Detector 2.dan2_actual"))
    double dan1_act = run.getLogAsSingleValue("Detector 2.dan2_actual");
    double angle, qx, qy, qz;
    comp->getRotation().getAngleAxis(angle, qx, qy, qz);
    TS_ASSERT_DELTA(angle, dan1_act, 1E-6)
    TS_ASSERT_EQUALS(qx, 0.)
    TS_ASSERT_DELTA(std::fabs(qy), 1., 1E-6)
    TS_ASSERT_EQUALS(qz, 0.)
    // check loaded data contains expected values
    const auto &xAxis = outputWS->x(0).rawData();
    TS_ASSERT_DELTA(xAxis[0], 0.0, 1E-5)
    TS_ASSERT_DELTA(xAxis[5], 5.0, 1E-5)
    TS_ASSERT_DELTA(xAxis[399], 399.0, 1E-5)
    TS_ASSERT_DELTA(outputWS->y(51192)[155], 1.0, 1E-5)
    TS_ASSERT_DELTA(outputWS->e(51192)[155], 1.0, 1E-5)
    TS_ASSERT_DELTA(outputWS->y(128 * 256 + 96 * 256)[0], 173.0, 1E-5)
    TS_ASSERT_DELTA(outputWS->e(128 * 256 + 96 * 256)[0], 13.15295, 1E-5)
    TS_ASSERT_DELTA(outputWS->y(128 * 256 + 96 * 256 + 1)[0], 0.05, 1E-5)
    TS_ASSERT_DELTA(outputWS->e(128 * 256 + 96 * 256 + 1)[0], 0.0, 1E-5)
    checkTimeFormat(outputWS);
    checkDuration(outputWS, 20.);
    checkWavelength(outputWS, 6.);
  }

  void test_D16_GAMMA() {
    LoadILLSANS alg;
    alg.setChild(true);
    alg.initialize();
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("Filename", "218356.nxs"));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("OutputWorkspace", "__unused_for_child"));
    TS_ASSERT_THROWS_NOTHING(alg.execute());
    TS_ASSERT(alg.isExecuted());
    MatrixWorkspace_const_sptr outputWS = alg.getProperty("OutputWorkspace");
    TS_ASSERT(outputWS);
    TS_ASSERT(outputWS->isHistogramData())
    TS_ASSERT(!outputWS->detectorInfo().isMonitor(0));
    TS_ASSERT(outputWS->detectorInfo().isMonitor(320 * 320));
    TS_ASSERT(outputWS->detectorInfo().isMonitor(320 * 320 + 1));
    const auto &instrument = outputWS->getInstrument();
    IComponent_const_sptr component = instrument->getComponentByName("detector");
    V3D pos = component->getPos();
    V3D origin(0, 0, 0);
    TS_ASSERT_DELTA(pos.distance(origin), 1, 1E-5);
    TS_ASSERT_DELTA(pos.X(), -0.17365, 1E-5); // sin(10)
    TS_ASSERT_DELTA(pos.Z(), 0.98481, 1E-5);  // cos(10)
    Mantid::detid_t bl_id, tr_id;
    instrument->getMinMaxDetectorIDs(bl_id, tr_id);
    detid2det_map det_map;
    instrument->getDetectors(det_map);
    IDetector_const_sptr bottom_right_pixel = det_map[bl_id];
    IDetector_const_sptr top_left_pixel = det_map[320 * 320 - 1];
    V3D br_pos = bottom_right_pixel->getPos();
    V3D tl_pos = top_left_pixel->getPos();
    // check the detector has 10 degrees angle
    TS_ASSERT_DELTA(br_pos.distance(origin), 1.02512, 1E-5);
    TS_ASSERT_DELTA(tl_pos.distance(origin), 1.02512, 1E-5);
    TS_ASSERT_DELTA(tl_pos.X(), -0.33073, 1E-5);
    TS_ASSERT_DELTA(tl_pos.Z(), 0.95711, 1E-5);
    TS_ASSERT_DELTA(br_pos.X(), -0.01657, 1E-5);
    TS_ASSERT_DELTA(br_pos.Z(), 1.01250, 1E-5);
    // check loaded data contains expected values
    const auto &xAxis = outputWS->x(0).rawData();
    const auto &spec = outputWS->y(51972).rawData();
    const auto &err = outputWS->e(51972).rawData();
    TS_ASSERT_EQUALS(xAxis.size(), 2)
    TS_ASSERT_DELTA(xAxis[0], 6.965, 1E-3)
    TS_ASSERT_DELTA(xAxis[1], 7.035, 1E-3)
    TS_ASSERT_EQUALS(spec[0], 17)
    TS_ASSERT_DELTA(err[0], sqrt(17), 1E-5)
    TS_ASSERT_DELTA(outputWS->y(320 * 320)[0], 0.0, 1E-5)
    TS_ASSERT_DELTA(outputWS->e(320 * 320)[0], 0.0, 1E-5)
    TS_ASSERT_DELTA(outputWS->y(320 * 320 + 1)[0], 124744.0, 1E-5)
    TS_ASSERT_DELTA(outputWS->e(320 * 320 + 1)[0], 353.19117, 1E-5)
    checkTimeFormat(outputWS);
    checkDuration(outputWS, 30.);
    checkWavelength(outputWS, 7.);
  }

  void test_D16_OMEGA_SCAN_SINGLE_POINT() {
    // test d16 scan data in the format where every file is a point
    LoadILLSANS alg;
    alg.setChild(true);
    alg.initialize();
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("Filename", "023583.nxs"));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("OutputWorkspace", "__unused_for_child"));
    TS_ASSERT_THROWS_NOTHING(alg.execute());
    TS_ASSERT(alg.isExecuted());
    MatrixWorkspace_const_sptr outputWS = alg.getProperty("OutputWorkspace");
    TS_ASSERT(outputWS);
    TS_ASSERT(outputWS->isHistogramData())
    TS_ASSERT(!outputWS->detectorInfo().isMonitor(0));
    TS_ASSERT(outputWS->detectorInfo().isMonitor(320 * 320));
    TS_ASSERT(outputWS->detectorInfo().isMonitor(320 * 320 + 1));
    TS_ASSERT_EQUALS(outputWS->blocksize(), 1)
    TS_ASSERT_EQUALS(outputWS->getNumberHistograms(), 320 * 320 + 2);
    // check loaded data contains expected values
    const auto &xAxis = outputWS->x(0).rawData();
    TS_ASSERT_EQUALS(xAxis.size(), 2)
    TS_ASSERT_DELTA(xAxis[0], 4.776, 1E-3)
    TS_ASSERT_DELTA(xAxis[1], 4.824, 1E-3)
    TS_ASSERT_DELTA(outputWS->y(0)[0], 3.0, 1E-3)
    TS_ASSERT_DELTA(outputWS->e(0)[0], 1.732, 1E-3)
    TS_ASSERT_DELTA(outputWS->y(320 * 320)[0], 0.0, 1E-3)
    TS_ASSERT_DELTA(outputWS->e(320 * 320)[0], 0.0, 1E-3) // apparently, the error is 0
    TS_ASSERT_DELTA(outputWS->y(320 * 320 + 1)[0], 213094.0, 1E-3)
    TS_ASSERT_DELTA(outputWS->e(320 * 320 + 1)[0], 461.621, 1E-3)
    checkTimeFormat(outputWS);
    checkDuration(outputWS, 5.);
    checkWavelength(outputWS, 4.8);
  }

  void test_D16_OMEGA_CONCATENATED_SCAN() {
    // test d16 scan data in a scan file, i.e. with multiple points in a single file
    LoadILLSANS alg;
    alg.setChild(true);
    alg.initialize();
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("Filename", "025786.nxs"));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("OutputWorkspace", "__unused_for_child"));
    TS_ASSERT_THROWS_NOTHING(alg.execute());
    TS_ASSERT(alg.isExecuted());
    MatrixWorkspace_const_sptr outputWS = alg.getProperty("OutputWorkspace");
    TS_ASSERT(outputWS);
    TS_ASSERT(!outputWS->isHistogramData())
    TS_ASSERT(outputWS->detectorInfo().isMonitor(320 * 320));
    TS_ASSERT_EQUALS(outputWS->blocksize(), 261)
    TS_ASSERT_EQUALS(outputWS->getNumberHistograms(), 320 * 320 + 2);
    checkTimeFormat(outputWS);
    checkDuration(outputWS, 15.);
    checkWavelength(outputWS, 4.45);
  }

  void test_D16B() {
    LoadILLSANS alg;
    alg.setChild(true);
    alg.initialize();
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("Filename", "066321.nxs"));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("OutputWorkspace", "__unused_for_child"));
    TS_ASSERT_THROWS_NOTHING(alg.execute());
    TS_ASSERT(alg.isExecuted());
    MatrixWorkspace_const_sptr outputWS = alg.getProperty("OutputWorkspace");
    TS_ASSERT(outputWS);
    TS_ASSERT(!outputWS->isHistogramData())
    TS_ASSERT(!outputWS->detectorInfo().isMonitor(0));
    TS_ASSERT(outputWS->detectorInfo().isMonitor(192 * 1152));
    TS_ASSERT_EQUALS(outputWS->blocksize(), 6)
    TS_ASSERT_EQUALS(outputWS->getNumberHistograms(), 192 * 1152 + 1);
    // check loaded data contains expected values
    TS_ASSERT_DELTA(outputWS->x(0)[0], 5.0, 1E-3)
    TS_ASSERT_DELTA(outputWS->x(0)[5], 6.0, 1E-3)
    TS_ASSERT_DELTA(outputWS->y(0)[0], 3304.0, 1E-3)
    TS_ASSERT_DELTA(outputWS->e(0)[0], 57.480, 1E-3)
    TS_ASSERT_DELTA(outputWS->y(192 * 1152 - 1)[5], 1131.0, 1E-3)
    TS_ASSERT_DELTA(outputWS->e(192 * 1152 - 1)[5], 33.630, 1E-3)
    TS_ASSERT_DELTA(outputWS->y(192 * 1152)[0], 0.0, 1E-3)
    TS_ASSERT_DELTA(outputWS->e(192 * 1152)[0], 0.0, 1E-3)
    checkTimeFormat(outputWS);
    checkDuration(outputWS, 3.);
    checkWavelength(outputWS, 4.45);
  }

  void test_D33_MONO() {
    LoadILLSANS alg;
    alg.setChild(true);
    alg.initialize();
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("Filename", "002294.nxs"))
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("OutputWorkspace", "__unused_for_child"))
    TS_ASSERT_THROWS_NOTHING(alg.execute())
    TS_ASSERT(alg.isExecuted())
    MatrixWorkspace_const_sptr outputWS = alg.getProperty("OutputWorkspace");
    TS_ASSERT(outputWS)
    TS_ASSERT_EQUALS(outputWS->getNumberHistograms(), 256 * 256 + 2)
    TS_ASSERT_EQUALS(outputWS->blocksize(), 1)
    TS_ASSERT(!outputWS->detectorInfo().isMonitor(0))
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
    IComponent_const_sptr back = instrument->getComponentByName("back_detector");
    TS_ASSERT_EQUALS(back->getPos(), V3D(0, 0, 10.1128));
    IComponent_const_sptr right = instrument->getComponentByName("front_detector_right");
    TS_ASSERT_EQUALS(right->getPos(), V3D(-0.41, 0, 1.4968));
    IComponent_const_sptr left = instrument->getComponentByName("front_detector_left");
    TS_ASSERT_EQUALS(left->getPos(), V3D(0.41, 0, 1.4968));
    IComponent_const_sptr top = instrument->getComponentByName("front_detector_top");
    TS_ASSERT_EQUALS(top->getPos(), V3D(0, 0.41, 1.3118));
    IComponent_const_sptr bottom = instrument->getComponentByName("front_detector_bottom");
    TS_ASSERT_EQUALS(bottom->getPos(), V3D(0, -0.41, 1.3118));
    const auto unit = outputWS->getAxis(0)->unit()->unitID();
    TS_ASSERT_EQUALS(unit, "Wavelength");
    checkTimeFormat(outputWS);
    checkDuration(outputWS, 41.5);
    checkWavelength(outputWS, 10.);
  }

  void test_D33_LTOF() {
    LoadILLSANS alg;
    alg.setChild(true);
    alg.initialize();
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("Filename", "042610.nxs"))
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("OutputWorkspace", "__unused_for_child"))
    TS_ASSERT_THROWS_NOTHING(alg.execute())
    TS_ASSERT(alg.isExecuted())
    MatrixWorkspace_const_sptr outputWS = alg.getProperty("OutputWorkspace");
    TS_ASSERT(outputWS)
    TS_ASSERT_EQUALS(outputWS->getNumberHistograms(), 256 * 256 + 2)
    TS_ASSERT_EQUALS(outputWS->blocksize(), 200)
    TS_ASSERT(!outputWS->detectorInfo().isMonitor(0))
    TS_ASSERT(outputWS->detectorInfo().isMonitor(256 * 256))
    TS_ASSERT(outputWS->detectorInfo().isMonitor(256 * 256 + 1))
    TS_ASSERT(outputWS->isHistogramData())
    TS_ASSERT(!outputWS->isDistribution())
    TS_ASSERT(!outputWS->isCommonBins())
    const auto &run = outputWS->run();
    TS_ASSERT(run.hasProperty("tof_mode"))
    const auto tof = run.getLogData("tof_mode");
    TS_ASSERT_EQUALS(tof->value(), "TOF");
    const auto unit = outputWS->getAxis(0)->unit()->unitID();
    TS_ASSERT_EQUALS(unit, "Wavelength");
    // check loaded data contains expected values
    TS_ASSERT_DELTA(outputWS->x(0)[0], 0.04969, 1E-5)
    TS_ASSERT_DELTA(outputWS->x(0)[1], 0.14873, 1E-5)
    TS_ASSERT_DELTA(outputWS->x(0)[200], 19.85713, 1E-5)
    TS_ASSERT_DELTA(outputWS->y(0)[124], 1.0, 1E-3)
    TS_ASSERT_DELTA(outputWS->e(0)[124], 1.0, 1E-3)
    TS_ASSERT_DELTA(outputWS->y(256 * 256)[0], 0.0, 1E-3)
    TS_ASSERT_DELTA(outputWS->e(256 * 256)[0], 0.0, 1E-3)
    TS_ASSERT_DELTA(outputWS->y(256 * 256)[0], 0.0, 1E-3)
    TS_ASSERT_DELTA(outputWS->e(256 * 256)[0], 0.0, 1E-3)
    checkTimeFormat(outputWS);
    checkDuration(outputWS, 30.);
  }

  void test_D33_VTOF() {
    LoadILLSANS alg;
    alg.setChild(true);
    alg.initialize();
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("Filename", "093410.nxs"))
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("OutputWorkspace", "__unused_for_child"))
    TS_ASSERT_THROWS_NOTHING(alg.execute())
    TS_ASSERT(alg.isExecuted())
    MatrixWorkspace_const_sptr outputWS = alg.getProperty("OutputWorkspace");
    TS_ASSERT(outputWS)
    TS_ASSERT_EQUALS(outputWS->getNumberHistograms(), 256 * 256 + 2)
    TS_ASSERT_EQUALS(outputWS->blocksize(), 30)
    TS_ASSERT(!outputWS->detectorInfo().isMonitor(0))
    TS_ASSERT(outputWS->detectorInfo().isMonitor(256 * 256))
    TS_ASSERT(outputWS->detectorInfo().isMonitor(256 * 256 + 1))
    TS_ASSERT(outputWS->isHistogramData())
    TS_ASSERT(!outputWS->isDistribution())
    TS_ASSERT(!outputWS->isCommonBins())
    const auto &run = outputWS->run();
    TS_ASSERT(run.hasProperty("tof_mode"))
    const auto tof = run.getLogData("tof_mode");
    TS_ASSERT_EQUALS(tof->value(), "TOF");
    const auto unit = outputWS->getAxis(0)->unit()->unitID();
    TS_ASSERT_EQUALS(unit, "Wavelength");
    // check loaded data contains expected values
    TS_ASSERT_DELTA(outputWS->x(0)[0], 0.0, 1E-5)
    TS_ASSERT_DELTA(outputWS->x(0)[1], 0.1998, 1E-5)
    TS_ASSERT_DELTA(outputWS->x(0)[2], 0.3996, 1E-5)
    TS_ASSERT_DELTA(outputWS->y(0)[23], 1.0, 1E-3)
    TS_ASSERT_DELTA(outputWS->e(0)[23], 1.0, 1E-3)
    TS_ASSERT_DELTA(outputWS->y(256 * 256)[0], 0.0, 1E-3)
    TS_ASSERT_DELTA(outputWS->e(256 * 256)[0], 0.0, 1E-3)
    TS_ASSERT_DELTA(outputWS->y(256 * 256)[0], 0.0, 1E-3)
    TS_ASSERT_DELTA(outputWS->e(256 * 256)[0], 0.0, 1E-3)
    checkTimeFormat(outputWS);
    checkDuration(outputWS, 120.);
  }

  void checkTimeFormat(MatrixWorkspace_const_sptr outputWS) {
    TS_ASSERT(outputWS->run().hasProperty("start_time"));
    TS_ASSERT(
        Mantid::Types::Core::DateAndTimeHelpers::stringIsISO8601(outputWS->run().getProperty("start_time")->value()));
  }
  void checkDuration(MatrixWorkspace_const_sptr outputWS, const double val) {
    TS_ASSERT(outputWS->run().hasProperty("duration"));
    TS_ASSERT_DELTA(outputWS->run().getPropertyAsSingleValue("duration"), val, 0.1);
  }
  void checkWavelength(MatrixWorkspace_const_sptr outputWS, const double val) {
    TS_ASSERT(outputWS->run().hasProperty("wavelength"));
    TS_ASSERT_DELTA(outputWS->run().getPropertyAsSingleValue("wavelength"), val, 0.1);
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
