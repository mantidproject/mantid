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
#include "MantidTypes/Core/DateAndTimeHelpers.h"

#include <tuple>

using namespace Mantid::API;
using namespace Mantid::Kernel;
using namespace Mantid::DataHandling;
using namespace Mantid::Geometry;

std::tuple<std::string, std::string> commonSetUp() {
  ConfigService::Instance().appendDataSearchSubDir("ILL/D7/");

  auto oldFacility = ConfigService::Instance().getFacility().name();
  ConfigService::Instance().setFacility("ILL");

  auto oldInstrument = ConfigService::Instance().getInstrument().name();
  ConfigService::Instance().setString("default.instrument", "D7");

  return std::make_tuple(oldFacility, oldInstrument);
}

void commonTearDown(const std::string &oldFacility, const std::string &oldInstrument) {
  if (!oldFacility.empty()) {
    ConfigService::Instance().setFacility(oldFacility);
  }
  if (!oldInstrument.empty()) {
    ConfigService::Instance().setString("default.instrument", oldInstrument);
  }
}

class LoadILLPolarizedDiffractionTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static LoadILLPolarizedDiffractionTest *createSuite() { return new LoadILLPolarizedDiffractionTest(); }
  static void destroySuite(LoadILLPolarizedDiffractionTest *suite) { delete suite; }

  void setUp() override { std::tie(m_oldFacility, m_oldInstrument) = commonSetUp(); }

  void tearDown() override { commonTearDown(m_oldFacility, m_oldInstrument); }

  void test_Init() {
    LoadILLPolarizedDiffraction alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    TS_ASSERT(alg.isInitialized());
  }

  void test_D7_monochromatic() {
    // Tests monochromatic data loading for D7
    LoadILLPolarizedDiffraction alg;
    alg.setChild(true);
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("Filename", "401800"))
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("OutputWorkspace", "_outWS"))
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("PositionCalibration", "None"))
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("ConvertToScatteringAngle", false))
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("TransposeMonochromatic", false))
    TS_ASSERT_THROWS_NOTHING(alg.execute())
    TS_ASSERT(alg.isExecuted())

    WorkspaceGroup_sptr outputWS = std::shared_ptr<Mantid::API::WorkspaceGroup>(alg.getProperty("OutputWorkspace"));
    TS_ASSERT(outputWS)
    TS_ASSERT_EQUALS(outputWS->getNumberOfEntries(), 6)
    do_test_general_features(outputWS, "monochromatic");

    MatrixWorkspace_sptr workspaceEntry1 =
        std::dynamic_pointer_cast<Mantid::API::MatrixWorkspace>(outputWS->getItem(0));
    TS_ASSERT(workspaceEntry1)

    TS_ASSERT_DELTA(workspaceEntry1->x(0)[0], 3.13, 0.01)
    TS_ASSERT_DELTA(workspaceEntry1->x(0)[1], 3.19, 0.01)
    TS_ASSERT_EQUALS(workspaceEntry1->y(0)[0], 11)
    TS_ASSERT_DELTA(workspaceEntry1->e(0)[0], 3.31, 0.01)

    TS_ASSERT_DELTA(workspaceEntry1->x(1)[0], 3.13, 0.01)
    TS_ASSERT_DELTA(workspaceEntry1->x(1)[1], 3.19, 0.01)
    TS_ASSERT_EQUALS(workspaceEntry1->y(1)[0], 12)
    TS_ASSERT_DELTA(workspaceEntry1->e(1)[0], 3.46, 0.01)

    TS_ASSERT_DELTA(workspaceEntry1->x(130)[0], 3.13, 0.01)
    TS_ASSERT_DELTA(workspaceEntry1->x(130)[1], 3.19, 0.01)
    TS_ASSERT_EQUALS(workspaceEntry1->y(130)[0], 4)
    TS_ASSERT_DELTA(workspaceEntry1->e(130)[0], 2.00, 0.01)

    TS_ASSERT_DELTA(workspaceEntry1->x(131)[0], 3.13, 0.01)
    TS_ASSERT_DELTA(workspaceEntry1->x(131)[1], 3.19, 0.01)
    TS_ASSERT_EQUALS(workspaceEntry1->y(131)[0], 17)
    TS_ASSERT_DELTA(workspaceEntry1->e(131)[0], 4.12, 0.01)

    TS_ASSERT_DELTA(workspaceEntry1->x(132)[0], 3.13, 0.01)
    TS_ASSERT_DELTA(workspaceEntry1->x(132)[1], 3.19, 0.01)
    TS_ASSERT_EQUALS(workspaceEntry1->y(132)[0], 167943)
    TS_ASSERT_DELTA(workspaceEntry1->e(132)[0], 409.80, 0.01)

    TS_ASSERT_DELTA(workspaceEntry1->x(133)[0], 3.13, 0.01)
    TS_ASSERT_DELTA(workspaceEntry1->x(133)[1], 3.19, 0.01)
    TS_ASSERT_EQUALS(workspaceEntry1->y(133)[0], 2042)
    TS_ASSERT_DELTA(workspaceEntry1->e(133)[0], 45.18, 0.01)
    checkTimeFormat(workspaceEntry1);
  }

  void test_D7_timeOfFlight() {
    // Tests loading TOF data for D7, indirectly tests for sorting SF and NSF entries
    LoadILLPolarizedDiffraction alg;
    alg.setChild(true);
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("Filename", "395850"))
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("OutputWorkspace", "_outWS"))
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("PositionCalibration", "None"))
    TS_ASSERT_THROWS_NOTHING(alg.execute())
    TS_ASSERT(alg.isExecuted())

    WorkspaceGroup_sptr outputWS = alg.getProperty("OutputWorkspace");
    TS_ASSERT(outputWS)
    TS_ASSERT(outputWS->isGroup())
    TS_ASSERT_EQUALS(outputWS->getNumberOfEntries(), 2)
    do_test_general_features(outputWS, "TOF");

    MatrixWorkspace_sptr workspaceEntry1 =
        std::dynamic_pointer_cast<Mantid::API::MatrixWorkspace>(outputWS->getItem(1));
    TS_ASSERT(workspaceEntry1)
    TS_ASSERT_EQUALS(workspaceEntry1->getAxis(0)->unit()->unitID(), "TOF")
    TS_ASSERT_EQUALS(workspaceEntry1->getAxis(0)->unit()->caption(), "Time-of-flight")

    TS_ASSERT_DELTA(workspaceEntry1->x(0)[0], 180.00, 0.01)
    TS_ASSERT_DELTA(workspaceEntry1->x(0)[1], 186.64, 0.01)
    TS_ASSERT_EQUALS(workspaceEntry1->y(0)[0], 0)
    TS_ASSERT_DELTA(workspaceEntry1->e(0)[0], 1.00, 0.01)

    TS_ASSERT_DELTA(workspaceEntry1->x(0)[511], 3573.04, 0.01)
    TS_ASSERT_DELTA(workspaceEntry1->x(0)[512], 3579.68, 0.01)
    TS_ASSERT_EQUALS(workspaceEntry1->y(0)[511], 0)
    TS_ASSERT_DELTA(workspaceEntry1->e(0)[511], 1.00, 0.01)

    TS_ASSERT_DELTA(workspaceEntry1->x(1)[0], 180.00, 0.01)
    TS_ASSERT_DELTA(workspaceEntry1->x(1)[1], 186.64, 0.01)
    TS_ASSERT_EQUALS(workspaceEntry1->y(1)[0], 0)
    TS_ASSERT_DELTA(workspaceEntry1->e(1)[0], 1.00, 0.01)

    TS_ASSERT_DELTA(workspaceEntry1->x(1)[511], 3573.04, 0.01)
    TS_ASSERT_DELTA(workspaceEntry1->x(1)[512], 3579.68, 0.01)
    TS_ASSERT_EQUALS(workspaceEntry1->y(1)[511], 0)
    TS_ASSERT_DELTA(workspaceEntry1->e(1)[511], 1.00, 0.01)

    TS_ASSERT_DELTA(workspaceEntry1->x(130)[0], 180.00, 0.01)
    TS_ASSERT_DELTA(workspaceEntry1->x(130)[1], 186.64, 0.01)
    TS_ASSERT_EQUALS(workspaceEntry1->y(130)[0], 0)
    TS_ASSERT_DELTA(workspaceEntry1->e(130)[0], 1.00, 0.01)

    TS_ASSERT_DELTA(workspaceEntry1->x(130)[365], 2603.60, 0.01)
    TS_ASSERT_DELTA(workspaceEntry1->x(130)[366], 2610.24, 0.01)
    TS_ASSERT_EQUALS(workspaceEntry1->y(130)[365], 0)
    TS_ASSERT_DELTA(workspaceEntry1->e(130)[365], 1.00, 0.01)

    TS_ASSERT_DELTA(workspaceEntry1->x(131)[0], 180.00, 0.01)
    TS_ASSERT_DELTA(workspaceEntry1->x(131)[1], 186.64, 0.01)
    TS_ASSERT_EQUALS(workspaceEntry1->y(131)[0], 0)
    TS_ASSERT_DELTA(workspaceEntry1->e(131)[0], 1.00, 0.01)

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
    TS_ASSERT_DELTA(workspaceEntry1->e(133)[0], 1.00, 0.01)

    TS_ASSERT_DELTA(workspaceEntry1->x(133)[511], 3573.04, 0.01)
    TS_ASSERT_DELTA(workspaceEntry1->x(133)[512], 3579.68, 0.01)
    TS_ASSERT_EQUALS(workspaceEntry1->y(133)[511], 0)
    TS_ASSERT_DELTA(workspaceEntry1->e(133)[511], 1.00, 0.01)
    checkTimeFormat(workspaceEntry1);
  }

  void test_D7_timeOfFlight_timechannels() {
    // Tests loading TOF data for D7, indirectly tests for sorting of SF and NSF entries
    LoadILLPolarizedDiffraction alg;
    alg.setChild(true);
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("Filename", "395850"))
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("OutputWorkspace", "_outWS"))
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("PositionCalibration", "None"))
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("TOFUnits", "TimeChannels"))
    TS_ASSERT_THROWS_NOTHING(alg.execute())
    TS_ASSERT(alg.isExecuted())

    WorkspaceGroup_sptr outputWS = alg.getProperty("OutputWorkspace");
    TS_ASSERT(outputWS)
    TS_ASSERT(outputWS->isGroup())
    TS_ASSERT_EQUALS(outputWS->getNumberOfEntries(), 2)
    do_test_general_features(outputWS, "TOF");

    MatrixWorkspace_sptr workspaceEntry1 =
        std::dynamic_pointer_cast<Mantid::API::MatrixWorkspace>(outputWS->getItem(1));
    TS_ASSERT(workspaceEntry1)
    TS_ASSERT_EQUALS(workspaceEntry1->getAxis(0)->unit()->unitID(), "Label")
    TS_ASSERT_EQUALS(workspaceEntry1->getAxis(0)->unit()->caption(), "Time channel")

    TS_ASSERT_EQUALS(workspaceEntry1->x(0)[0], 0)
    TS_ASSERT_EQUALS(workspaceEntry1->x(0)[1], 1)
    TS_ASSERT_EQUALS(workspaceEntry1->y(0)[0], 0)
    TS_ASSERT_DELTA(workspaceEntry1->e(0)[0], 1.00, 0.01)

    TS_ASSERT_EQUALS(workspaceEntry1->x(0)[511], 511)
    TS_ASSERT_EQUALS(workspaceEntry1->x(0)[512], 512)
    TS_ASSERT_EQUALS(workspaceEntry1->y(0)[511], 0)
    TS_ASSERT_DELTA(workspaceEntry1->e(0)[511], 1.00, 0.01)

    TS_ASSERT_EQUALS(workspaceEntry1->x(1)[0], 0)
    TS_ASSERT_EQUALS(workspaceEntry1->x(1)[1], 1)
    TS_ASSERT_EQUALS(workspaceEntry1->y(1)[0], 0)
    TS_ASSERT_DELTA(workspaceEntry1->e(1)[0], 1.00, 0.01)

    TS_ASSERT_EQUALS(workspaceEntry1->x(1)[511], 511)
    TS_ASSERT_EQUALS(workspaceEntry1->x(1)[512], 512)
    TS_ASSERT_EQUALS(workspaceEntry1->y(1)[511], 0)
    TS_ASSERT_DELTA(workspaceEntry1->e(1)[511], 1.00, 0.01)

    TS_ASSERT_EQUALS(workspaceEntry1->x(130)[0], 0)
    TS_ASSERT_EQUALS(workspaceEntry1->x(130)[1], 1)
    TS_ASSERT_EQUALS(workspaceEntry1->y(130)[0], 0)
    TS_ASSERT_DELTA(workspaceEntry1->e(130)[0], 1.00, 0.01)

    TS_ASSERT_EQUALS(workspaceEntry1->x(130)[365], 365)
    TS_ASSERT_EQUALS(workspaceEntry1->x(130)[366], 366)
    TS_ASSERT_EQUALS(workspaceEntry1->y(130)[365], 0)
    TS_ASSERT_DELTA(workspaceEntry1->e(130)[365], 1.00, 0.01)

    TS_ASSERT_EQUALS(workspaceEntry1->x(131)[0], 0)
    TS_ASSERT_EQUALS(workspaceEntry1->x(131)[1], 1)
    TS_ASSERT_EQUALS(workspaceEntry1->y(131)[0], 0)
    TS_ASSERT_DELTA(workspaceEntry1->e(131)[0], 1.00, 0.01)

    TS_ASSERT_EQUALS(workspaceEntry1->x(131)[365], 365)
    TS_ASSERT_EQUALS(workspaceEntry1->x(131)[366], 366)
    TS_ASSERT_EQUALS(workspaceEntry1->y(131)[365], 1)
    TS_ASSERT_DELTA(workspaceEntry1->e(131)[365], 1.00, 0.01)

    TS_ASSERT_EQUALS(workspaceEntry1->x(132)[0], 0)
    TS_ASSERT_EQUALS(workspaceEntry1->x(132)[1], 1)
    TS_ASSERT_EQUALS(workspaceEntry1->y(132)[0], 5468)
    TS_ASSERT_DELTA(workspaceEntry1->e(132)[0], 73.94, 0.01)

    TS_ASSERT_EQUALS(workspaceEntry1->x(132)[511], 511)
    TS_ASSERT_EQUALS(workspaceEntry1->x(132)[512], 512)
    TS_ASSERT_EQUALS(workspaceEntry1->y(132)[511], 5394)
    TS_ASSERT_DELTA(workspaceEntry1->e(132)[511], 73.44, 0.01)

    TS_ASSERT_EQUALS(workspaceEntry1->x(133)[0], 0)
    TS_ASSERT_EQUALS(workspaceEntry1->x(133)[1], 1)
    TS_ASSERT_EQUALS(workspaceEntry1->y(133)[0], 0)
    TS_ASSERT_DELTA(workspaceEntry1->e(133)[0], 1.00, 0.01)

    TS_ASSERT_EQUALS(workspaceEntry1->x(133)[511], 511)
    TS_ASSERT_EQUALS(workspaceEntry1->x(133)[512], 512)
    TS_ASSERT_EQUALS(workspaceEntry1->y(133)[511], 0)
    TS_ASSERT_DELTA(workspaceEntry1->e(133)[511], 1.00, 0.01)
  }

  void test_D7_multifile_sum() {
    // Tests loading and adding 2 files for D7 with the generic Load on ADS
    // This tests indirectly the confidence method

    Load alg;
    alg.initialize();
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("Filename", "401800+401801"))
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("OutputWorkspace", "_outWS"))
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("PositionCalibration", "None"))
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("ConvertToScatteringAngle", false))
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("TransposeMonochromatic", false))
    TS_ASSERT_THROWS_NOTHING(alg.execute())
    TS_ASSERT(alg.isExecuted())

    WorkspaceGroup_sptr outputWS = AnalysisDataService::Instance().retrieveWS<WorkspaceGroup>("_outWS");
    TS_ASSERT(outputWS)
    TS_ASSERT(outputWS->isGroup())
    TS_ASSERT_EQUALS(outputWS->getNumberOfEntries(), 6)
    do_test_general_features(outputWS, "monochromatic");

    MatrixWorkspace_sptr workspaceEntry1 = std::static_pointer_cast<Mantid::API::MatrixWorkspace>(outputWS->getItem(0));
    TS_ASSERT(workspaceEntry1)

    TS_ASSERT_DELTA(workspaceEntry1->x(0)[0], 3.13, 0.01)
    TS_ASSERT_DELTA(workspaceEntry1->x(0)[1], 3.19, 0.01)
    TS_ASSERT_EQUALS(workspaceEntry1->y(0)[0], 24)
    TS_ASSERT_DELTA(workspaceEntry1->e(0)[0], 4.89, 0.01)

    TS_ASSERT_DELTA(workspaceEntry1->x(1)[0], 3.13, 0.01)
    TS_ASSERT_DELTA(workspaceEntry1->x(1)[1], 3.19, 0.01)
    TS_ASSERT_EQUALS(workspaceEntry1->y(1)[0], 19)
    TS_ASSERT_DELTA(workspaceEntry1->e(1)[0], 4.35, 0.01)

    TS_ASSERT_DELTA(workspaceEntry1->x(130)[0], 3.13, 0.01)
    TS_ASSERT_DELTA(workspaceEntry1->x(130)[1], 3.19, 0.01)
    TS_ASSERT_EQUALS(workspaceEntry1->y(130)[0], 8)
    TS_ASSERT_DELTA(workspaceEntry1->e(130)[0], 2.82, 0.01)

    TS_ASSERT_DELTA(workspaceEntry1->x(131)[0], 3.13, 0.01)
    TS_ASSERT_DELTA(workspaceEntry1->x(131)[1], 3.19, 0.01)
    TS_ASSERT_EQUALS(workspaceEntry1->y(131)[0], 38)
    TS_ASSERT_DELTA(workspaceEntry1->e(131)[0], 6.16, 0.01)

    TS_ASSERT_DELTA(workspaceEntry1->x(132)[0], 3.13, 0.01)
    TS_ASSERT_DELTA(workspaceEntry1->x(132)[1], 3.19, 0.01)
    TS_ASSERT_EQUALS(workspaceEntry1->y(132)[0], 335686)
    TS_ASSERT_DELTA(workspaceEntry1->e(132)[0], 579.38, 0.01)

    TS_ASSERT_DELTA(workspaceEntry1->x(133)[0], 3.13, 0.01)
    TS_ASSERT_DELTA(workspaceEntry1->x(133)[1], 3.19, 0.01)
    TS_ASSERT_EQUALS(workspaceEntry1->y(133)[0], 4109)
    TS_ASSERT_DELTA(workspaceEntry1->e(133)[0], 64.10, 0.01)
    checkTimeFormat(workspaceEntry1);
  }

  void test_D7_multifile_list() {
    // Tests loading 2 files as a list for D7 with the generic Load on ADS
    // This tests indirectly the confidence method

    Load alg;
    alg.initialize();
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("Filename", "401800,401801"))
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("OutputWorkspace", "_outWS"))
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("PositionCalibration", "None"))
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("ConvertToScatteringAngle", false))
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("TransposeMonochromatic", false))
    TS_ASSERT_THROWS_NOTHING(alg.execute())
    TS_ASSERT(alg.isExecuted())

    WorkspaceGroup_sptr outputWS = AnalysisDataService::Instance().retrieveWS<WorkspaceGroup>("_outWS");
    TS_ASSERT(outputWS)
    TS_ASSERT(outputWS->isGroup())
    TS_ASSERT_EQUALS(outputWS->getNumberOfEntries(), 12)
    do_test_general_features(outputWS, "monochromatic");

    MatrixWorkspace_sptr workspaceEntry1 = std::static_pointer_cast<Mantid::API::MatrixWorkspace>(outputWS->getItem(0));
    TS_ASSERT_DELTA(workspaceEntry1->x(0)[0], 3.13, 0.01)
    TS_ASSERT_DELTA(workspaceEntry1->x(0)[1], 3.19, 0.01)
    TS_ASSERT_EQUALS(workspaceEntry1->y(0)[0], 11)
    TS_ASSERT_DELTA(workspaceEntry1->e(0)[0], 3.31, 0.01)

    TS_ASSERT_DELTA(workspaceEntry1->x(1)[0], 3.13, 0.01)
    TS_ASSERT_DELTA(workspaceEntry1->x(1)[1], 3.19, 0.01)
    TS_ASSERT_EQUALS(workspaceEntry1->y(1)[0], 12)
    TS_ASSERT_DELTA(workspaceEntry1->e(1)[0], 3.46, 0.01)

    TS_ASSERT_DELTA(workspaceEntry1->x(130)[0], 3.13, 0.01)
    TS_ASSERT_DELTA(workspaceEntry1->x(130)[1], 3.19, 0.01)
    TS_ASSERT_EQUALS(workspaceEntry1->y(130)[0], 4)
    TS_ASSERT_DELTA(workspaceEntry1->e(130)[0], 2.00, 0.01)

    TS_ASSERT_DELTA(workspaceEntry1->x(131)[0], 3.13, 0.01)
    TS_ASSERT_DELTA(workspaceEntry1->x(131)[1], 3.19, 0.01)
    TS_ASSERT_EQUALS(workspaceEntry1->y(131)[0], 17)
    TS_ASSERT_DELTA(workspaceEntry1->e(131)[0], 4.12, 0.01)

    TS_ASSERT_DELTA(workspaceEntry1->x(132)[0], 3.13, 0.01)
    TS_ASSERT_DELTA(workspaceEntry1->x(132)[1], 3.19, 0.01)
    TS_ASSERT_EQUALS(workspaceEntry1->y(132)[0], 167943)
    TS_ASSERT_DELTA(workspaceEntry1->e(132)[0], 409.80, 0.01)

    TS_ASSERT_DELTA(workspaceEntry1->x(133)[0], 3.13, 0.01)
    TS_ASSERT_DELTA(workspaceEntry1->x(133)[1], 3.19, 0.01)
    TS_ASSERT_EQUALS(workspaceEntry1->y(133)[0], 2042)
    TS_ASSERT_DELTA(workspaceEntry1->e(133)[0], 45.18, 0.01)

    MatrixWorkspace_sptr workspaceEntry12 =
        std::static_pointer_cast<Mantid::API::MatrixWorkspace>(outputWS->getItem(11));
    TS_ASSERT_DELTA(workspaceEntry12->x(0)[0], 3.13, 0.01)
    TS_ASSERT_DELTA(workspaceEntry12->x(0)[1], 3.19, 0.01)
    TS_ASSERT_EQUALS(workspaceEntry12->y(0)[0], 14)
    TS_ASSERT_DELTA(workspaceEntry12->e(0)[0], 3.74, 0.01)

    TS_ASSERT_DELTA(workspaceEntry12->x(1)[0], 3.13, 0.01)
    TS_ASSERT_DELTA(workspaceEntry12->x(1)[1], 3.19, 0.01)
    TS_ASSERT_EQUALS(workspaceEntry12->y(1)[0], 15)
    TS_ASSERT_DELTA(workspaceEntry12->e(1)[0], 3.87, 0.01)

    TS_ASSERT_DELTA(workspaceEntry12->x(130)[0], 3.13, 0.01)
    TS_ASSERT_DELTA(workspaceEntry12->x(130)[1], 3.19, 0.01)
    TS_ASSERT_EQUALS(workspaceEntry12->y(130)[0], 5)
    TS_ASSERT_DELTA(workspaceEntry12->e(130)[0], 2.23, 0.01)

    TS_ASSERT_DELTA(workspaceEntry12->x(131)[0], 3.13, 0.01)
    TS_ASSERT_DELTA(workspaceEntry12->x(131)[1], 3.19, 0.01)
    TS_ASSERT_EQUALS(workspaceEntry12->y(131)[0], 15)
    TS_ASSERT_DELTA(workspaceEntry12->e(131)[0], 3.87, 0.01)

    TS_ASSERT_DELTA(workspaceEntry12->x(132)[0], 3.13, 0.01)
    TS_ASSERT_DELTA(workspaceEntry12->x(132)[1], 3.19, 0.01)
    TS_ASSERT_EQUALS(workspaceEntry12->y(132)[0], 167220)
    TS_ASSERT_DELTA(workspaceEntry12->e(132)[0], 408.92, 0.01)

    TS_ASSERT_DELTA(workspaceEntry12->x(133)[0], 3.13, 0.01)
    TS_ASSERT_DELTA(workspaceEntry12->x(133)[1], 3.19, 0.01)
    TS_ASSERT_EQUALS(workspaceEntry12->y(133)[0], 108504)
    TS_ASSERT_DELTA(workspaceEntry12->e(133)[0], 329.39, 0.01)
    checkTimeFormat(workspaceEntry1);
  }

  void test_D7_default_alignment() {
    // Tests default pixel position alignment coming from the IDF file
    LoadILLPolarizedDiffraction alg;
    alg.setChild(true);
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("Filename", "401800"))
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("OutputWorkspace", "_outWS"))
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("PositionCalibration", "None"))
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("ConvertToScatteringAngle", false))
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("TransposeMonochromatic", false))
    TS_ASSERT_THROWS_NOTHING(alg.execute())
    TS_ASSERT(alg.isExecuted())

    WorkspaceGroup_sptr outputWS = alg.getProperty("OutputWorkspace");
    TS_ASSERT(outputWS)
    TS_ASSERT(outputWS->isGroup())
    TS_ASSERT_EQUALS(outputWS->getNumberOfEntries(), 6)
    do_test_general_features(outputWS, "monochromatic");

    for (auto entry_no = 0; entry_no < outputWS->getNumberOfEntries(); ++entry_no) {
      MatrixWorkspace_sptr workspaceEntry =
          std::dynamic_pointer_cast<Mantid::API::MatrixWorkspace>(outputWS->getItem(entry_no));
      TS_ASSERT(workspaceEntry)

      TS_ASSERT_DELTA(workspaceEntry->detectorInfo().twoTheta(0) * RAD_2_DEG, 12.66, 0.01)
      TS_ASSERT_DELTA(workspaceEntry->detectorInfo().twoTheta(43) * RAD_2_DEG, 55.45, 0.01)
      TS_ASSERT_DELTA(workspaceEntry->detectorInfo().twoTheta(44) * RAD_2_DEG, 58.79, 0.01)
      TS_ASSERT_DELTA(workspaceEntry->detectorInfo().twoTheta(87) * RAD_2_DEG, 101.58, 0.01)
      TS_ASSERT_DELTA(workspaceEntry->detectorInfo().twoTheta(88) * RAD_2_DEG, 100.78, 0.01)
      TS_ASSERT_DELTA(workspaceEntry->detectorInfo().twoTheta(131) * RAD_2_DEG, 143.57, 0.01)
    }
  }

  void test_D7_nexus_alignment() {
    // Tests pixel position alignment coming from the NeXuS file
    LoadILLPolarizedDiffraction alg;
    alg.setChild(true);
    alg.initialize();
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("Filename", "401800"))
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("OutputWorkspace", "_outWS"))
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("PositionCalibration", "Nexus"))
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("ConvertToScatteringAngle", false))
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("TransposeMonochromatic", false))
    TS_ASSERT_THROWS_NOTHING(alg.execute())
    TS_ASSERT(alg.isExecuted())

    WorkspaceGroup_sptr outputWS = alg.getProperty("OutputWorkspace");
    TS_ASSERT(outputWS)
    TS_ASSERT(outputWS->isGroup())
    TS_ASSERT_EQUALS(outputWS->getNumberOfEntries(), 6)
    do_test_general_features(outputWS, "monochromatic");

    for (auto entry_no = 0; entry_no < outputWS->getNumberOfEntries(); ++entry_no) {
      MatrixWorkspace_sptr workspaceEntry =
          std::dynamic_pointer_cast<Mantid::API::MatrixWorkspace>(outputWS->getItem(entry_no));
      TS_ASSERT(workspaceEntry)

      TS_ASSERT_DELTA(workspaceEntry->detectorInfo().twoTheta(0) * RAD_2_DEG, 10.86, 0.01)
      TS_ASSERT_DELTA(workspaceEntry->detectorInfo().twoTheta(43) * RAD_2_DEG, 53.81, 0.01)
      TS_ASSERT_DELTA(workspaceEntry->detectorInfo().twoTheta(44) * RAD_2_DEG, 57.06, 0.01)
      TS_ASSERT_DELTA(workspaceEntry->detectorInfo().twoTheta(87) * RAD_2_DEG, 99.45, 0.01)
      TS_ASSERT_DELTA(workspaceEntry->detectorInfo().twoTheta(88) * RAD_2_DEG, 101.38, 0.01)
      TS_ASSERT_DELTA(workspaceEntry->detectorInfo().twoTheta(131) * RAD_2_DEG, 144.17, 0.01)
    }
  }

  void test_D7_yigfile_alignment() {
    // Tests pixel position alignment coming from the YIG calibration IPF file
    LoadILLPolarizedDiffraction alg;
    alg.setChild(true);
    alg.initialize();
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("Filename", "401800"))
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("OutputWorkspace", "_outWS"))
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("PositionCalibration", "YIGFile"))
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("YIGFilename", "D7_YIG_calibration.xml"))
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("ConvertToScatteringAngle", false))
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("TransposeMonochromatic", false))
    TS_ASSERT_THROWS_NOTHING(alg.execute())
    TS_ASSERT(alg.isExecuted())

    WorkspaceGroup_sptr outputWS = alg.getProperty("OutputWorkspace");
    TS_ASSERT(outputWS)
    TS_ASSERT(outputWS->isGroup())
    TS_ASSERT_EQUALS(outputWS->getNumberOfEntries(), 6)
    do_test_general_features(outputWS, "monochromatic");

    for (auto entry_no = 0; entry_no < outputWS->getNumberOfEntries(); ++entry_no) {
      MatrixWorkspace_sptr workspaceEntry =
          std::static_pointer_cast<Mantid::API::MatrixWorkspace>(outputWS->getItem(entry_no));
      TS_ASSERT(workspaceEntry)

      TS_ASSERT_DELTA(workspaceEntry->detectorInfo().twoTheta(0) * RAD_2_DEG, 10.86, 0.01)
      TS_ASSERT_DELTA(workspaceEntry->detectorInfo().twoTheta(43) * RAD_2_DEG, 53.81, 0.01)
      TS_ASSERT_DELTA(workspaceEntry->detectorInfo().twoTheta(44) * RAD_2_DEG, 57.06, 0.01)
      TS_ASSERT_DELTA(workspaceEntry->detectorInfo().twoTheta(87) * RAD_2_DEG, 99.45, 0.01)
      TS_ASSERT_DELTA(workspaceEntry->detectorInfo().twoTheta(88) * RAD_2_DEG, 101.38, 0.01)
      TS_ASSERT_DELTA(workspaceEntry->detectorInfo().twoTheta(131) * RAD_2_DEG, 144.17, 0.01)
    }
    // check for the correct wavelength value from the IPF
    MatrixWorkspace_sptr ws = std::static_pointer_cast<Mantid::API::MatrixWorkspace>(outputWS->getItem(0));
    double wavelength = stod(ws->mutableRun().getLogData("monochromator.wavelength")->value());
    TS_ASSERT_DELTA(wavelength, 3.09, 0.01)
    checkTimeFormat(ws);
  }

  void test_D7_transpose() {
    // Tests monochromatic data loading for D7
    LoadILLPolarizedDiffraction alg;
    alg.setChild(true);
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("Filename", "401800"))
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("OutputWorkspace", "_outWS"))
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("PositionCalibration", "None"))
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("ConvertToScatteringAngle", false))
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("TransposeMonochromatic", true))
    TS_ASSERT_THROWS_NOTHING(alg.execute())
    TS_ASSERT(alg.isExecuted())

    WorkspaceGroup_sptr outputWS = alg.getProperty("OutputWorkspace");
    TS_ASSERT(outputWS)
    TS_ASSERT(outputWS->isGroup())
    TS_ASSERT_EQUALS(outputWS->getNumberOfEntries(), 6)

    for (auto entry_no = 0; entry_no < outputWS->getNumberOfEntries(); entry_no++) {
      MatrixWorkspace_sptr workspaceEntry =
          std::static_pointer_cast<Mantid::API::MatrixWorkspace>(outputWS->getItem(entry_no));
      TS_ASSERT(workspaceEntry)
      TS_ASSERT_EQUALS(workspaceEntry->getNumberHistograms(), 1)
      TS_ASSERT_EQUALS(workspaceEntry->blocksize(), 134)
      TS_ASSERT(!workspaceEntry->isHistogramData())
      TS_ASSERT(!workspaceEntry->isDistribution())
      TS_ASSERT_EQUALS(workspaceEntry->YUnitLabel(), "Counts")
      TS_ASSERT_EQUALS(workspaceEntry->getAxis(0)->unit()->caption(), "Spectrum")
    }
  }

  void test_D7_convert_spectral_axis() {
    // Tests monochromatic data loading for D7
    LoadILLPolarizedDiffraction alg;
    alg.setChild(true);
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("Filename", "401800"))
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("OutputWorkspace", "_outWS"))
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("PositionCalibration", "None"))
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("ConvertToScatteringAngle", true))
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("TransposeMonochromatic", false))
    TS_ASSERT_THROWS_NOTHING(alg.execute())
    TS_ASSERT(alg.isExecuted())

    WorkspaceGroup_sptr outputWS = alg.getProperty("OutputWorkspace");
    TS_ASSERT(outputWS)
    TS_ASSERT(outputWS->isGroup())
    TS_ASSERT_EQUALS(outputWS->getNumberOfEntries(), 6)

    for (auto entry_no = 0; entry_no < outputWS->getNumberOfEntries(); entry_no++) {
      MatrixWorkspace_sptr workspaceEntry =
          std::static_pointer_cast<Mantid::API::MatrixWorkspace>(outputWS->getItem(entry_no));
      TS_ASSERT(workspaceEntry)
      TS_ASSERT_EQUALS(workspaceEntry->getNumberHistograms(), 134)
      TS_ASSERT_EQUALS(workspaceEntry->blocksize(), 1)
      TS_ASSERT(workspaceEntry->isHistogramData())
      TS_ASSERT(!workspaceEntry->isDistribution())
      TS_ASSERT_EQUALS(workspaceEntry->YUnitLabel(), "Counts")
      TS_ASSERT_EQUALS(workspaceEntry->getAxis(0)->unit()->unitID(), "Wavelength")
      TS_ASSERT_DELTA(workspaceEntry->getAxis(1)->getValue(0), 12.66, 0.01)
      TS_ASSERT_DELTA(workspaceEntry->getAxis(1)->getValue(1), 13.45, 0.01)
      TS_ASSERT_DELTA(workspaceEntry->getAxis(1)->getValue(2), 14.66, 0.01)
      TS_ASSERT_DELTA(workspaceEntry->getAxis(1)->getValue(3), 15.45, 0.01)
      TS_ASSERT_DELTA(workspaceEntry->getAxis(1)->getValue(132), 0.00, 0.01)
      TS_ASSERT_DELTA(workspaceEntry->getAxis(1)->getValue(133), 0.00, 0.01)
    }
  }

  void test_D7_sign_TwoTheta() {
    // Tests pixel position alignment coming from the YIG calibration IPF file
    LoadILLPolarizedDiffraction alg;
    alg.setChild(true);
    alg.initialize();
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("Filename", "394458"))
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("OutputWorkspace", "_outWS"))
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("PositionCalibration", "None"))
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("ConvertToScatteringAngle", true))
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("TransposeMonochromatic", false))
    TS_ASSERT_THROWS_NOTHING(alg.execute())
    TS_ASSERT(alg.isExecuted())

    WorkspaceGroup_sptr outputWS = alg.getProperty("OutputWorkspace");
    TS_ASSERT(outputWS)
    TS_ASSERT(outputWS->isGroup())
    TS_ASSERT_EQUALS(outputWS->getNumberOfEntries(), 1)
    do_test_general_features(outputWS, "monochromatic");

    for (auto entry_no = 0; entry_no < outputWS->getNumberOfEntries(); ++entry_no) {
      MatrixWorkspace_sptr workspaceEntry =
          std::static_pointer_cast<Mantid::API::MatrixWorkspace>(outputWS->getItem(entry_no));
      TS_ASSERT(workspaceEntry)
      auto axis = workspaceEntry->getAxis(1);
      TS_ASSERT(!axis->isSpectra())
      TS_ASSERT_DELTA(axis->getValue(0), -88.87, 0.01)
      TS_ASSERT_DELTA(axis->getValue(43), -46.08, 0.01)
      TS_ASSERT_DELTA(axis->getValue(44), -42.65, 0.01)
      TS_ASSERT_DELTA(axis->getValue(87), 0.13, 0.01)
      TS_ASSERT_DELTA(axis->getValue(88), -0.80, 0.01)
      TS_ASSERT_DELTA(axis->getValue(131), 41.99, 0.01)
    }
  }

  void test_D7_polarisation_order() {
    // Tests loading and sorting polarisation with XYZ measurement
    LoadILLPolarizedDiffraction alg;
    alg.setChild(true);
    alg.initialize();
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("Filename", "401800"))
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("OutputWorkspace", "_outWS"))
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("PositionCalibration", "None"))
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("ConvertToScatteringAngle", false))
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("TransposeMonochromatic", false))
    TS_ASSERT_THROWS_NOTHING(alg.execute())
    TS_ASSERT(alg.isExecuted())

    WorkspaceGroup_sptr outputWS = alg.getProperty("OutputWorkspace");
    TS_ASSERT(outputWS)
    TS_ASSERT(outputWS->isGroup())
    TS_ASSERT_EQUALS(outputWS->getNumberOfEntries(), 6)
    do_test_general_features(outputWS, "monochromatic");

    for (auto entry_no = 0; entry_no < outputWS->getNumberOfEntries(); ++entry_no) {
      MatrixWorkspace_sptr workspaceEntry =
          std::static_pointer_cast<Mantid::API::MatrixWorkspace>(outputWS->getItem(entry_no));
      TS_ASSERT(workspaceEntry)
      auto polarisation = workspaceEntry->mutableRun().getLogData("POL.actual_state")->value();
      auto expected_polarisation = "ZPO";
      if (entry_no < 2) {
        expected_polarisation = "ZPO";
      } else if (entry_no < 4) {
        expected_polarisation = "XPO";
      } else if (entry_no < 6) {
        expected_polarisation = "YPO";
      }
      TS_ASSERT_EQUALS(polarisation, expected_polarisation)
    }
  }

  void do_test_general_features(WorkspaceGroup_sptr outputWS, std::string measurementMode) {
    for (auto entry_no = 0; entry_no < outputWS->getNumberOfEntries(); entry_no++) {
      MatrixWorkspace_sptr workspaceEntry =
          std::dynamic_pointer_cast<Mantid::API::MatrixWorkspace>(outputWS->getItem(entry_no));
      TS_ASSERT(workspaceEntry)
      TS_ASSERT_EQUALS(workspaceEntry->getNumberHistograms(), 134)
      TS_ASSERT(workspaceEntry->detectorInfo().isMonitor(132))
      TS_ASSERT(workspaceEntry->detectorInfo().isMonitor(133))
      TS_ASSERT(workspaceEntry->isHistogramData())
      TS_ASSERT(!workspaceEntry->isDistribution())
      TS_ASSERT_EQUALS(workspaceEntry->YUnitLabel(), "Counts")
      checkTimeFormat(workspaceEntry);
      if (measurementMode == "monochromatic") {
        TS_ASSERT_EQUALS(workspaceEntry->blocksize(), 1)
        TS_ASSERT_EQUALS(workspaceEntry->getAxis(0)->unit()->unitID(), "Wavelength")
      } else if (measurementMode == "TOF") {
        {
          TS_ASSERT_EQUALS(workspaceEntry->blocksize(), 512)
        }
      }
    }
  }

  void checkTimeFormat(MatrixWorkspace_const_sptr outputWS) {
    TS_ASSERT(outputWS->run().hasProperty("start_time"));
    TS_ASSERT(
        Mantid::Types::Core::DateAndTimeHelpers::stringIsISO8601(outputWS->run().getProperty("start_time")->value()));
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
  static void destroySuite(LoadILLPolarizedDiffractionTestPerformance *suite) { delete suite; }

  LoadILLPolarizedDiffractionTestPerformance() {}

  void setUp() override {
    std::tie(m_oldFacility, m_oldInstrument) = commonSetUp();

    m_alg.initialize();
    m_alg.setChild(true);
    m_alg.setPropertyValue("Filename", "395850");
    m_alg.setPropertyValue("OutputWorkspace", "_outWS");
    m_alg.setPropertyValue("PositionCalibration", "Nexus");
  }

  void tearDown() override { commonTearDown(m_oldFacility, m_oldInstrument); }

  void test_performance() {
    for (int i = 0; i < 50; ++i) {
      TS_ASSERT_THROWS_NOTHING(m_alg.execute());
    }
  }

private:
  LoadILLPolarizedDiffraction m_alg;
  std::string m_oldFacility;
  std::string m_oldInstrument;
};
