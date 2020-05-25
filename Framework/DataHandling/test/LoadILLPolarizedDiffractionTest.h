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
#include "MantidAPI/Run.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidDataHandling/Load.h"
#include "MantidDataHandling/LoadILLPolarizedDiffraction.h"
#include "MantidGeometry/Instrument/ComponentHelper.h"
#include "MantidGeometry/Instrument/ComponentInfo.h"
#include "MantidGeometry/Instrument/DetectorInfo.h"
#include "MantidKernel/ConfigService.h"
#include "MantidKernel/FacilityInfo.h"
#include "MantidKernel/Unit.h"
#include "MantidKernel/V3D.h"

using namespace Mantid::API;
using namespace Mantid::Kernel;
using namespace Mantid::DataHandling;
using namespace Mantid::Geometry;

class LoadILLPolarizedDiffractionTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static LoadILLPolarizedDiffractionTest *createSuite() {
    return new LoadILLPolarizedDiffractionTest();
  }
  static void destroySuite(LoadILLPolarizedDiffractionTest *suite) {
    delete suite;
  }

  void setUp() override {
    ConfigService::Instance().appendDataSearchSubDir("/ILL/D7");

    m_oldFacility = ConfigService::Instance().getFacility().name();
    ConfigService::Instance().setFacility("ILL");

    m_oldInstrument = ConfigService::Instance().getInstrument().name();
    ConfigService::Instance().setString("default.instrument", "D7");
  }

  void tearDown() override {
    if (!m_oldFacility.empty()) {
      ConfigService::Instance().setFacility(m_oldFacility);
    }
    if (!m_oldInstrument.empty()) {
      ConfigService::Instance().setString("default.instrument",
                                          m_oldInstrument);
    }
  }

  void test_Init() {
    LoadILLPolarizedDiffraction alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    TS_ASSERT(alg.isInitialized());
  }

  void test_D7() {
    LoadILLPolarizedDiffraction alg;
    // Don't put output in ADS by default
    alg.setChild(true);
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("Filename", "ILL/D7/401800.nxs"))
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("OutputWorkspace", "401800"))
    TS_ASSERT_THROWS_NOTHING(
        alg.setPropertyValue("PositionCalibration", "None"))
    TS_ASSERT_THROWS_NOTHING(alg.execute())
    TS_ASSERT(alg.isExecuted())

    WorkspaceGroup_sptr outputWS = alg.getProperty("OutputWorkspace");
    TS_ASSERT(outputWS)
    TS_ASSERT_EQUALS(outputWS->isGroup(), 1)
    TS_ASSERT_EQUALS(outputWS->getNumberOfEntries(), 6)
    MatrixWorkspace_sptr workspaceEntry1 =
        std::dynamic_pointer_cast<Mantid::API::MatrixWorkspace>(
            outputWS->getItem(0));
    TS_ASSERT(workspaceEntry1)
    TS_ASSERT_EQUALS(workspaceEntry1->getNumberHistograms(), 134)
    TS_ASSERT_EQUALS(workspaceEntry1->blocksize(), 1)

    Instrument_const_sptr instrument = workspaceEntry1->getInstrument();
    TS_ASSERT(instrument)

    V3D sample(0, 0, 0);
    V3D zAxis(0, 0, 1);

    const auto &pixel1 = instrument->getDetector(1);
    TS_ASSERT_DELTA(RAD_2_DEG * pixel1->getTwoTheta(sample, zAxis), 12.66, 0.01)

    const auto &pixel44 = instrument->getDetector(44);
    TS_ASSERT_DELTA(RAD_2_DEG * pixel44->getTwoTheta(sample, zAxis), 55.45,
                    0.01)

    const auto &pixel45 = instrument->getDetector(45);
    TS_ASSERT_DELTA(RAD_2_DEG * pixel45->getTwoTheta(sample, zAxis), 58.79,
                    0.01)

    const auto &pixel88 = instrument->getDetector(88);
    TS_ASSERT(pixel88)
    TS_ASSERT_DELTA(RAD_2_DEG * pixel88->getTwoTheta(sample, zAxis), 101.58,
                    0.01)

    const auto &pixel89 = instrument->getDetector(89);
    TS_ASSERT(pixel89)
    TS_ASSERT_DELTA(RAD_2_DEG * pixel89->getTwoTheta(sample, zAxis), 100.78,
                    0.01)

    const auto &pixel132 = instrument->getDetector(132);
    TS_ASSERT(pixel132)
    TS_ASSERT_DELTA(RAD_2_DEG * pixel132->getTwoTheta(sample, zAxis), 143.57,
                    0.01)
  }

  void test_D7_monochromatic() {
    LoadILLPolarizedDiffraction alg;
    // Don't put output in ADS by default
    alg.setChild(true);
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("Filename", "ILL/D7/401800.nxs"))
    TS_ASSERT_THROWS_NOTHING(
        alg.setPropertyValue("OutputWorkspace", "_outWS"))
    TS_ASSERT_THROWS_NOTHING(
        alg.setPropertyValue("PositionCalibration", "None"))
    TS_ASSERT_THROWS_NOTHING(alg.execute())
    TS_ASSERT(alg.isExecuted())

//    WorkspaceGroup_sptr outputWS = alg.getProperty("OutputWorkspace");
    WorkspaceGroup_sptr outputWS = std::shared_ptr<Mantid::API::WorkspaceGroup>(
                alg.getProperty("OutputWorkspace"));
    TS_ASSERT(outputWS)
    TS_ASSERT_EQUALS(outputWS->getNumberOfEntries(), 6)
    MatrixWorkspace_sptr workspaceEntry1 =
        std::dynamic_pointer_cast<Mantid::API::MatrixWorkspace>(
            outputWS->getItem(0));
    TS_ASSERT(workspaceEntry1)
    TS_ASSERT_EQUALS(workspaceEntry1->getNumberHistograms(), 134)
    TS_ASSERT_EQUALS(workspaceEntry1->blocksize(), 1)
    TS_ASSERT(workspaceEntry1->detectorInfo().isMonitor(132))
    TS_ASSERT(workspaceEntry1->detectorInfo().isMonitor(133))
    TS_ASSERT(workspaceEntry1->isHistogramData())
    TS_ASSERT(!workspaceEntry1->isDistribution())

    TS_ASSERT_EQUALS(workspaceEntry1->getAxis(0)->unit()->caption(),
                     "Wavelength")
    TS_ASSERT_EQUALS(workspaceEntry1->YUnitLabel(), "Counts")

    TS_ASSERT_DELTA(workspaceEntry1->x(0)[0], 2.84, 0.01)
    TS_ASSERT_DELTA(workspaceEntry1->x(0)[1], 3.47, 0.01)
    TS_ASSERT_EQUALS(workspaceEntry1->y(0)[0], 11)
    TS_ASSERT_DELTA(workspaceEntry1->e(0)[0], 3.31, 0.01)

    TS_ASSERT_DELTA(workspaceEntry1->x(1)[0], 2.84, 0.01)
    TS_ASSERT_DELTA(workspaceEntry1->x(1)[1], 3.47, 0.01)
    TS_ASSERT_EQUALS(workspaceEntry1->y(1)[0], 12)
    TS_ASSERT_DELTA(workspaceEntry1->e(1)[0], 3.46, 0.01)

    TS_ASSERT_DELTA(workspaceEntry1->x(130)[0], 2.84, 0.01)
    TS_ASSERT_DELTA(workspaceEntry1->x(130)[1], 3.47, 0.01)
    TS_ASSERT_EQUALS(workspaceEntry1->y(130)[0], 4)
    TS_ASSERT_DELTA(workspaceEntry1->e(130)[0], 2.00, 0.01)

    TS_ASSERT_DELTA(workspaceEntry1->x(131)[0], 2.84, 0.01)
    TS_ASSERT_DELTA(workspaceEntry1->x(131)[1], 3.47, 0.01)
    TS_ASSERT_EQUALS(workspaceEntry1->y(131)[0], 17)
    TS_ASSERT_DELTA(workspaceEntry1->e(131)[0], 4.12, 0.01)

    TS_ASSERT_DELTA(workspaceEntry1->x(132)[0], 2.84, 0.01)
    TS_ASSERT_DELTA(workspaceEntry1->x(132)[1], 3.47, 0.01)
    TS_ASSERT_EQUALS(workspaceEntry1->y(132)[0], 167943)
    TS_ASSERT_DELTA(workspaceEntry1->e(132)[0], 409.80, 0.01)

    TS_ASSERT_DELTA(workspaceEntry1->x(133)[0], 2.84, 0.01)
    TS_ASSERT_DELTA(workspaceEntry1->x(133)[1], 3.47, 0.01)
    TS_ASSERT_EQUALS(workspaceEntry1->y(133)[0], 2042)
    TS_ASSERT_DELTA(workspaceEntry1->e(133)[0], 45.18, 0.01)

    MatrixWorkspace_sptr workspaceEntry5 =
        std::dynamic_pointer_cast<Mantid::API::MatrixWorkspace>(
            outputWS->getItem(5));
    TS_ASSERT(workspaceEntry5)
    TS_ASSERT_EQUALS(workspaceEntry5->getNumberHistograms(), 134)
    TS_ASSERT_EQUALS(workspaceEntry5->blocksize(), 1)
    TS_ASSERT(workspaceEntry5->detectorInfo().isMonitor(132))
    TS_ASSERT(workspaceEntry5->detectorInfo().isMonitor(133))
    TS_ASSERT(workspaceEntry5->isHistogramData())
    TS_ASSERT(!workspaceEntry5->isDistribution())
  }

  void test_D7_timeOfFlight() {
    LoadILLPolarizedDiffraction alg;
    // Don't put output in ADS by default
    alg.setChild(true);
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("Filename", "ILL/D7/395850.nxs"))
    TS_ASSERT_THROWS_NOTHING(
        alg.setPropertyValue("OutputWorkspace", "_unused_for_child"))
    TS_ASSERT_THROWS_NOTHING(
        alg.setPropertyValue("PositionCalibration", "None"))
    TS_ASSERT_THROWS_NOTHING(alg.execute())
    TS_ASSERT(alg.isExecuted())

    WorkspaceGroup_sptr outputWS = alg.getProperty("OutputWorkspace");
    TS_ASSERT(outputWS)
    TS_ASSERT_EQUALS(outputWS->getNumberOfEntries(), 2)
    MatrixWorkspace_sptr workspaceEntry1 =
        std::dynamic_pointer_cast<Mantid::API::MatrixWorkspace>(
            outputWS->getItem(0));
    TS_ASSERT(workspaceEntry1)
    TS_ASSERT_EQUALS(workspaceEntry1->getNumberHistograms(), 134)
    TS_ASSERT_EQUALS(workspaceEntry1->blocksize(), 512)
    TS_ASSERT(workspaceEntry1->detectorInfo().isMonitor(132))
    TS_ASSERT(workspaceEntry1->detectorInfo().isMonitor(133))
    TS_ASSERT(workspaceEntry1->isHistogramData())
    TS_ASSERT(!workspaceEntry1->isDistribution())

    TS_ASSERT_EQUALS(workspaceEntry1->getAxis(0)->unit()->caption(), "Time")
    TS_ASSERT_EQUALS(workspaceEntry1->YUnitLabel(), "Counts")

    TS_ASSERT_DELTA(workspaceEntry1->x(0)[0], 180.00, 0.01)
    TS_ASSERT_DELTA(workspaceEntry1->x(0)[1], 186.64, 0.01)
    TS_ASSERT_EQUALS(workspaceEntry1->y(0)[0], 0)
    TS_ASSERT_DELTA(workspaceEntry1->e(0)[0], 0.00, 0.01)

    TS_ASSERT_DELTA(workspaceEntry1->x(0)[511], 3573.04, 0.01)
    TS_ASSERT_DELTA(workspaceEntry1->x(0)[512], 3579.68, 0.01)
    TS_ASSERT_EQUALS(workspaceEntry1->y(0)[511], 0)
    TS_ASSERT_DELTA(workspaceEntry1->e(0)[511], 0.00, 0.01)

    TS_ASSERT_DELTA(workspaceEntry1->x(1)[0], 180.00, 0.01)
    TS_ASSERT_DELTA(workspaceEntry1->x(1)[1], 186.64, 0.01)
    TS_ASSERT_EQUALS(workspaceEntry1->y(1)[0], 0)
    TS_ASSERT_DELTA(workspaceEntry1->e(1)[0], 0.00, 0.01)

    TS_ASSERT_DELTA(workspaceEntry1->x(1)[511], 3573.04, 0.01)
    TS_ASSERT_DELTA(workspaceEntry1->x(1)[512], 3579.68, 0.01)
    TS_ASSERT_EQUALS(workspaceEntry1->y(1)[511], 0)
    TS_ASSERT_DELTA(workspaceEntry1->e(1)[511], 0.00, 0.01)

    TS_ASSERT_DELTA(workspaceEntry1->x(130)[0], 180.00, 0.01)
    TS_ASSERT_DELTA(workspaceEntry1->x(130)[1], 186.64, 0.01)
    TS_ASSERT_EQUALS(workspaceEntry1->y(130)[0], 0)
    TS_ASSERT_DELTA(workspaceEntry1->e(130)[0], 0.00, 0.01)

    TS_ASSERT_DELTA(workspaceEntry1->x(130)[365], 2603.60, 0.01)
    TS_ASSERT_DELTA(workspaceEntry1->x(130)[366], 2610.24, 0.01)
    TS_ASSERT_EQUALS(workspaceEntry1->y(130)[365], 0)
    TS_ASSERT_DELTA(workspaceEntry1->e(130)[365], 0.00, 0.01)

    TS_ASSERT_DELTA(workspaceEntry1->x(131)[0], 180.00, 0.01)
    TS_ASSERT_DELTA(workspaceEntry1->x(131)[1], 186.64, 0.01)
    TS_ASSERT_EQUALS(workspaceEntry1->y(131)[0], 0)
    TS_ASSERT_DELTA(workspaceEntry1->e(131)[0], 0.00, 0.01)

    TS_ASSERT_DELTA(workspaceEntry1->x(131)[365], 2603.60, 0.01)
    TS_ASSERT_DELTA(workspaceEntry1->x(131)[366], 2610.24, 0.01)
    TS_ASSERT_EQUALS(workspaceEntry1->y(131)[365], 1)
    TS_ASSERT_DELTA(workspaceEntry1->e(131)[365], 1.00, 0.01)

    TS_ASSERT_DELTA(workspaceEntry1->x(132)[0], 180.00, 0.01)
    TS_ASSERT_DELTA(workspaceEntry1->x(132)[1], 186.64, 0.01)
    TS_ASSERT_EQUALS(workspaceEntry1->y(132)[0], 5468)
    TS_ASSERT_DELTA(workspaceEntry1->e(132)[0], 73.94, 0.01)

    TS_ASSERT_DELTA(workspaceEntry1->x(132)[511], 3573.04, 0.01)
    TS_ASSERT_DELTA(workspaceEntry1->x(132)[512], 3579.68, 0.01)
    TS_ASSERT_EQUALS(workspaceEntry1->y(132)[511], 5394)
    TS_ASSERT_DELTA(workspaceEntry1->e(132)[511], 73.44, 0.01)

    TS_ASSERT_DELTA(workspaceEntry1->x(133)[0], 180.00, 0.01)
    TS_ASSERT_DELTA(workspaceEntry1->x(133)[1], 186.64, 0.01)
    TS_ASSERT_EQUALS(workspaceEntry1->y(133)[0], 0)
    TS_ASSERT_DELTA(workspaceEntry1->e(133)[0], 0.00, 0.01)

    TS_ASSERT_DELTA(workspaceEntry1->x(133)[511], 3573.04, 0.01)
    TS_ASSERT_DELTA(workspaceEntry1->x(133)[512], 3579.68, 0.01)
    TS_ASSERT_EQUALS(workspaceEntry1->y(133)[511], 0)
    TS_ASSERT_DELTA(workspaceEntry1->e(133)[512], 0.00, 0.01)

    MatrixWorkspace_sptr workspaceEntry2 =
        std::dynamic_pointer_cast<Mantid::API::MatrixWorkspace>(
            outputWS->getItem(1));
    TS_ASSERT(workspaceEntry2)
    TS_ASSERT_EQUALS(workspaceEntry2->getNumberHistograms(), 134)
    TS_ASSERT_EQUALS(workspaceEntry2->blocksize(), 512)
    TS_ASSERT(workspaceEntry2->detectorInfo().isMonitor(132))
    TS_ASSERT(workspaceEntry2->detectorInfo().isMonitor(133))
    TS_ASSERT(workspaceEntry2->isHistogramData())
    TS_ASSERT(!workspaceEntry2->isDistribution())

    TS_ASSERT_EQUALS(workspaceEntry2->getAxis(0)->unit()->caption(), "Time")
    TS_ASSERT_EQUALS(workspaceEntry2->YUnitLabel(), "Counts")
  }

  void test_D7_multifile() {
    // Tests 2 files for D7 with the generic Load on ADS
    // This tests indirectly the confidence method
    // (and NexusDescriptor issue therein)

    Load alg;
    alg.initialize();
    TS_ASSERT_THROWS_NOTHING(
        alg.setPropertyValue("Filename", "ILL/D7/401800+401801.nxs"))
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("OutputWorkspace", "_outWS"))
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("PositionCalibration", "None"))
    TS_ASSERT_THROWS_NOTHING(alg.execute())
    TS_ASSERT(alg.isExecuted())

    WorkspaceGroup_sptr outputWS =
        AnalysisDataService::Instance().retrieveWS<WorkspaceGroup>("_outWS");
    TS_ASSERT(outputWS)
    TS_ASSERT_EQUALS(outputWS->getNumberOfEntries(), 6)
    MatrixWorkspace_sptr workspaceEntry1 =
        std::dynamic_pointer_cast<Mantid::API::MatrixWorkspace>(
            outputWS->getItem(0));
    TS_ASSERT(workspaceEntry1)
    TS_ASSERT_EQUALS(workspaceEntry1->getNumberHistograms(), 134)
    TS_ASSERT_EQUALS(workspaceEntry1->blocksize(), 1)
    TS_ASSERT(workspaceEntry1->detectorInfo().isMonitor(132))
    TS_ASSERT(workspaceEntry1->detectorInfo().isMonitor(133))
    TS_ASSERT(workspaceEntry1->isHistogramData())
    TS_ASSERT(!workspaceEntry1->isDistribution())

    TS_ASSERT_EQUALS(workspaceEntry1->getAxis(0)->unit()->caption(),
                             "Wavelength")
    TS_ASSERT_EQUALS(workspaceEntry1->YUnitLabel(), "Counts")

    TS_ASSERT_DELTA(workspaceEntry1->x(0)[0], 2.84, 0.01)
    TS_ASSERT_DELTA(workspaceEntry1->x(0)[1], 3.47, 0.01)
    TS_ASSERT_EQUALS(workspaceEntry1->y(0)[0], 24)
    TS_ASSERT_DELTA(workspaceEntry1->e(0)[0], 4.89, 0.01)

    TS_ASSERT_DELTA(workspaceEntry1->x(1)[0], 2.84, 0.01)
    TS_ASSERT_DELTA(workspaceEntry1->x(1)[1], 3.47, 0.01)
    TS_ASSERT_EQUALS(workspaceEntry1->y(1)[0], 19)
    TS_ASSERT_DELTA(workspaceEntry1->e(1)[0], 4.35, 0.01)

    TS_ASSERT_DELTA(workspaceEntry1->x(130)[0], 2.84, 0.01)
    TS_ASSERT_DELTA(workspaceEntry1->x(130)[1], 3.47, 0.01)
    TS_ASSERT_EQUALS(workspaceEntry1->y(130)[0], 8)
    TS_ASSERT_DELTA(workspaceEntry1->e(130)[0], 2.82, 0.01)

    TS_ASSERT_DELTA(workspaceEntry1->x(131)[0], 2.84, 0.01)
    TS_ASSERT_DELTA(workspaceEntry1->x(131)[1], 3.47, 0.01)
    TS_ASSERT_EQUALS(workspaceEntry1->y(131)[0], 38)
    TS_ASSERT_DELTA(workspaceEntry1->e(131)[0], 6.16, 0.01)

    TS_ASSERT_DELTA(workspaceEntry1->x(132)[0], 2.84, 0.01)
    TS_ASSERT_DELTA(workspaceEntry1->x(132)[1], 3.47, 0.01)
    TS_ASSERT_EQUALS(workspaceEntry1->y(132)[0], 335686)
    TS_ASSERT_DELTA(workspaceEntry1->e(132)[0], 579.38, 0.01)

    TS_ASSERT_DELTA(workspaceEntry1->x(133)[0], 2.84, 0.01)
    TS_ASSERT_DELTA(workspaceEntry1->x(133)[1], 3.47, 0.01)
    TS_ASSERT_EQUALS(workspaceEntry1->y(133)[0], 4109)
    TS_ASSERT_DELTA(workspaceEntry1->e(133)[0], 64.10, 0.01)
  }

  void test_D7_nexus_alignment() {

    LoadILLPolarizedDiffraction alg;
    alg.setChild(true);
    alg.initialize();
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("Filename", "ILL/D7/401800.nxs"))
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("OutputWorkspace", "__outWS"))
    TS_ASSERT_THROWS_NOTHING(
        alg.setPropertyValue("PositionCalibration", "Nexus"))
    TS_ASSERT_THROWS_NOTHING(alg.execute())
    TS_ASSERT(alg.isExecuted())

    WorkspaceGroup_sptr outputWS = alg.getProperty("OutputWorkspace");
    TS_ASSERT(outputWS)
    TS_ASSERT_EQUALS(outputWS->getNumberOfEntries(), 6)
    MatrixWorkspace_sptr workspaceEntry1 =
        std::dynamic_pointer_cast<Mantid::API::MatrixWorkspace>(
            outputWS->getItem(0));
    TS_ASSERT(workspaceEntry1)

    Instrument_const_sptr instrument = workspaceEntry1->getInstrument();
    TS_ASSERT(instrument)

    V3D sample(0, 0, 0);
    V3D zAxis(0, 0, 1);

    const auto &pixel1 = instrument->getDetector(1);
    TS_ASSERT(pixel1)
    TS_ASSERT_DELTA(RAD_2_DEG * pixel1->getTwoTheta(sample, zAxis), 10.86, 0.01)

    const auto &pixel44 = instrument->getDetector(44);
    TS_ASSERT(pixel44)
    TS_ASSERT_DELTA(RAD_2_DEG * pixel44->getTwoTheta(sample, zAxis), 53.81,
                    0.01)

    const auto &pixel45 = instrument->getDetector(45);
    TS_ASSERT(pixel45)
    TS_ASSERT_DELTA(RAD_2_DEG * pixel45->getTwoTheta(sample, zAxis), 57.06,
                    0.01)

    const auto &pixel88 = instrument->getDetector(88);
    TS_ASSERT(pixel88)
    TS_ASSERT_DELTA(RAD_2_DEG * pixel88->getTwoTheta(sample, zAxis), 99.45,
                    0.01)

    const auto &pixel89 = instrument->getDetector(89);
    TS_ASSERT(pixel89)
    TS_ASSERT_DELTA(RAD_2_DEG * pixel89->getTwoTheta(sample, zAxis), 101.38,
                    0.01)

    const auto &pixel132 = instrument->getDetector(132);
    TS_ASSERT(pixel132)
    TS_ASSERT_DELTA(RAD_2_DEG * pixel132->getTwoTheta(sample, zAxis), 144.17,
                    0.01)
  }

  void test_D7_yigfile_alignment() {

    LoadILLPolarizedDiffraction alg;
    alg.setChild(true);
    alg.initialize();
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("Filename", "ILL/D7/401800.nxs"))
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("OutputWorkspace", "__outWS"))
    TS_ASSERT_THROWS_NOTHING(
        alg.setPropertyValue("PositionCalibration", "YIGFile"))
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("YIGFilename", "ILL/D7/YIG_IPF.xml"))
    TS_ASSERT_THROWS_NOTHING(alg.execute())
    TS_ASSERT(alg.isExecuted())

    WorkspaceGroup_sptr outputWS = alg.getProperty("OutputWorkspace");
    TS_ASSERT(outputWS)
    TS_ASSERT_EQUALS(outputWS->getNumberOfEntries(), 6)
    MatrixWorkspace_sptr workspaceEntry1 =
        std::dynamic_pointer_cast<Mantid::API::MatrixWorkspace>(
            outputWS->getItem(0));
    TS_ASSERT(workspaceEntry1)

    Instrument_const_sptr instrument = workspaceEntry1->getInstrument();
    TS_ASSERT(instrument)

    V3D sample(0, 0, 0);
    V3D zAxis(0, 0, 1);

    const auto &pixel1 = instrument->getDetector(1);
    TS_ASSERT(pixel1)
    TS_ASSERT_DELTA(RAD_2_DEG * pixel1->getTwoTheta(sample, zAxis), 10.86, 0.01)

    const auto &pixel44 = instrument->getDetector(44);
    TS_ASSERT(pixel44)
    TS_ASSERT_DELTA(RAD_2_DEG * pixel44->getTwoTheta(sample, zAxis), 53.81,
                    0.01)

    const auto &pixel45 = instrument->getDetector(45);
    TS_ASSERT(pixel45)
    TS_ASSERT_DELTA(RAD_2_DEG * pixel45->getTwoTheta(sample, zAxis), 57.06,
                    0.01)

    const auto &pixel88 = instrument->getDetector(88);
    TS_ASSERT(pixel88)
    TS_ASSERT_DELTA(RAD_2_DEG * pixel88->getTwoTheta(sample, zAxis), 99.45,
                    0.01)

    const auto &pixel89 = instrument->getDetector(89);
    TS_ASSERT(pixel89)
    TS_ASSERT_DELTA(RAD_2_DEG * pixel89->getTwoTheta(sample, zAxis), 101.38,
                    0.01)

    const auto &pixel132 = instrument->getDetector(132);
    TS_ASSERT(pixel132)
    TS_ASSERT_DELTA(RAD_2_DEG * pixel132->getTwoTheta(sample, zAxis), 144.17,
                    0.01)
  }

private:
  const double RAD_2_DEG = 180.0 / M_PI;
  std::string m_oldFacility;
  std::string m_oldInstrument;
};

class LoadILLPolarizedDiffractionTestPerformance : public CxxTest::TestSuite {
public:
  static LoadILLPolarizedDiffractionTestPerformance *createSuite() {
    return new LoadILLPolarizedDiffractionTestPerformance();
  }
  static void destroySuite(LoadILLPolarizedDiffractionTestPerformance *suite) {
    delete suite;
  }

  LoadILLPolarizedDiffractionTestPerformance() {}

  void setUp() override {
    m_alg.initialize();
    m_alg.setChild(true);
    m_alg.setPropertyValue("Filename", "ILL/D7/395850");
    m_alg.setPropertyValue("OutputWorkspace", "__");
    m_alg.setPropertyValue("PositionCalibration", "None");
  }

  void test_performance() {
    for (int i = 0; i < 5; ++i) {
      TS_ASSERT_THROWS_NOTHING(m_alg.execute());
    }
  }

private:
  LoadILLPolarizedDiffraction m_alg;
};
