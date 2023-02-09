// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2023 ISIS Rutherford Appleton Laboratory UKRI,
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
#include "MantidDataHandling/LoadILLLagrange.h"
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

class LoadILLLagrangeTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static LoadILLLagrangeTest *createSuite() { return new LoadILLLagrangeTest(); }
  static void destroySuite(LoadILLLagrangeTest *suite) { delete suite; }

  void tearDown() override { AnalysisDataService::Instance().clear(); }

  LoadILLLagrangeTest() {
    ConfigService::Instance().appendDataSearchSubDir("ILL/Lagrange/");
    ConfigService::Instance().setFacility("ILL");
  }

  void test_Init() {
    LoadILLLagrange alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    TS_ASSERT(alg.isInitialized());
  }

  void test_Lagrange() {
    // Tests simple data loading for Lagrange
    LoadILLLagrange alg;
    alg.setChild(true);
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("Filename", "014412"))
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("OutputWorkspace", "_outWS"))
    TS_ASSERT_THROWS_NOTHING(alg.execute())
    TS_ASSERT(alg.isExecuted())

    MatrixWorkspace_sptr outputWS = std::shared_ptr<Mantid::API::MatrixWorkspace>(alg.getProperty("OutputWorkspace"));
    TS_ASSERT(outputWS)

    // check if data is loaded as expected:
    TS_ASSERT_EQUALS(outputWS->x(0).size(), 31)
    TS_ASSERT_DELTA(outputWS->x(0)[0], 35, 0.01)
    TS_ASSERT_DELTA(outputWS->x(0)[30], 50, 0.01)
    TS_ASSERT_EQUALS(outputWS->y(0)[0], 3)
    TS_ASSERT_EQUALS(outputWS->y(0)[30], 3)
    TS_ASSERT_DELTA(outputWS->e(0)[0], 1.73, 0.01)

    // and for the monitor:
    TS_ASSERT_DELTA(outputWS->x(1)[0], 35, 0.01)
    TS_ASSERT_DELTA(outputWS->x(1)[30], 50, 0.01)
    TS_ASSERT_EQUALS(outputWS->y(1)[0], 1)
    TS_ASSERT_EQUALS(outputWS->y(1)[30], 1)
    TS_ASSERT_DELTA(outputWS->e(1)[0], 1.0, 0.01)

    // and whether the monitor flag is properly set
    TS_ASSERT(outputWS->detectorInfo().isMonitor(1))
  }

  void test_Lagrange_close_scans() {
    // Tests loading of synthetic Lagrange data, with two scans being only 2 meV apart
    LoadILLLagrange alg;
    alg.setChild(true);
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("Filename", "014412_close_scans_sample.nxs"))
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("OutputWorkspace", "_outWS"))
    TS_ASSERT_THROWS_NOTHING(alg.execute())
    TS_ASSERT(alg.isExecuted())

    MatrixWorkspace_sptr outputWS = std::shared_ptr<Mantid::API::MatrixWorkspace>(alg.getProperty("OutputWorkspace"));
    TS_ASSERT(outputWS)

    // check if data is loaded as expected:
    TS_ASSERT_EQUALS(outputWS->x(0).size(), 31)
    TS_ASSERT_DELTA(outputWS->x(0)[0], 35, 0.01)
    TS_ASSERT_DELTA(outputWS->x(0)[0], 35.002, 0.01)
    TS_ASSERT_DELTA(outputWS->x(0)[30], 50, 0.01)
    TS_ASSERT_EQUALS(outputWS->y(0)[0], 10)
    TS_ASSERT_EQUALS(outputWS->y(0)[30], 310)
    TS_ASSERT_DELTA(outputWS->e(0)[0], sqrt(10), 0.01)

    // and for the monitor:
    TS_ASSERT_DELTA(outputWS->x(1)[0], 35, 0.01)
    TS_ASSERT_DELTA(outputWS->x(1)[30], 50, 0.01)
    TS_ASSERT_EQUALS(outputWS->y(1)[0], 1)
    TS_ASSERT_EQUALS(outputWS->y(1)[30], 1)
    TS_ASSERT_DELTA(outputWS->e(1)[0], 1.0, 0.01)

    // and whether the monitor flag is properly set
    TS_ASSERT(!outputWS->detectorInfo().isMonitor(0))
    TS_ASSERT(outputWS->detectorInfo().isMonitor(1))
  }

private:
  std::string m_oldFacility;
  std::string m_oldInstrument;
};

class LoadILLagrangeTestPerformance : public CxxTest::TestSuite {
public:
  static LoadILLagrangeTestPerformance *createSuite() { return new LoadILLagrangeTestPerformance(); }
  static void destroySuite(LoadILLagrangeTestPerformance *suite) { delete suite; }

  LoadILLagrangeTestPerformance() {}

  void setUp() override {
    m_alg.initialize();
    m_alg.setChild(true);
    m_alg.setPropertyValue("Filename", "014412");
    m_alg.setPropertyValue("OutputWorkspace", "_outWS");
  }

  void tearDown() override { AnalysisDataService::Instance().clear(); }

  void test_performance() {
    for (int i = 0; i < 50; ++i) {
      TS_ASSERT_THROWS_NOTHING(m_alg.execute());
    }
  }

private:
  LoadILLLagrange m_alg;
  std::string m_oldFacility;
  std::string m_oldInstrument;
};
