#ifndef INSTRUMENTRAYTRACERTEST_H_
#define INSTRUMENTRAYTRACERTEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidAPI/AnalysisDataService.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidGeometry/Objects/InstrumentRayTracer.h"
#include "MantidTestHelpers/ComponentCreationHelper.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"
#include "MantidAPI/FrameworkManager.h"
#include <boost/make_shared.hpp>

using namespace Mantid::Geometry;
using Mantid::DataObjects::Workspace2D_sptr;
using Mantid::API::AnalysisDataService;
using Mantid::Kernel::V3D;
using Mantid::API::FrameworkManager;

//-------------------------------------------------------------
// Fake test suite to keep cxxtest happy
//-------------------------------------------------------------
class InstrumentRayTracerTest : public CxxTest::TestSuite {
public:
  void testNothing() {}
};

//-------------------------------------------------------------
// Performance test for large rectangular detectors
// TEST IS HERE IN DATAHANDLING BECAUSE IT USES LOADINSTRUMENT
//-------------------------------------------------------------

class InstrumentRayTracerTestPerformance : public CxxTest::TestSuite {
public:
  /// Test instrument
  Instrument_sptr m_inst;
  Workspace2D_sptr topazWS;

  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static InstrumentRayTracerTestPerformance *createSuite() {
    return new InstrumentRayTracerTestPerformance();
  }
  static void destroySuite(InstrumentRayTracerTestPerformance *suite) {
    delete suite;
  }

  InstrumentRayTracerTestPerformance() {
    m_inst = ComponentCreationHelper::createTestInstrumentRectangular(2, 100);

    topazWS = WorkspaceCreationHelper::create2DWorkspace(1, 2);
    AnalysisDataService::Instance().add("TOPAZ_2010", topazWS);
    // Load a small test file
    FrameworkManager::Instance().exec(
        "LoadInstrument", 6, "Filename", "TOPAZ_Definition_2010.xml",
        "Workspace", "TOPAZ_2010", "RewriteSpectraMap", "True");
  }

  ~InstrumentRayTracerTestPerformance() override {
    AnalysisDataService::Instance().remove("TOPAZ_2010");
  }

  void test_RectangularDetector() {
    // Directly in Z+ = towards the detector center
    V3D testDir(0.0, 0.0, 1.0);
    for (size_t i = 0; i < 100; i++) {
      InstrumentRayTracer tracker(m_inst);
      tracker.traceFromSample(testDir);
      Links results = tracker.getResults();
      TS_ASSERT_EQUALS(results.size(), 3);
      // showResults(results, m_inst);
    }
  }

  void test_TOPAZ() {
    bool verbose = false;
    Instrument_const_sptr inst = topazWS->getInstrument();
    // Directly in Z+ = towards the detector center
    for (int azimuth = 0; azimuth < 360; azimuth += 3)
      for (int elev = -89; elev < 89; elev += 3) {
        // Make a vector pointing in every direction
        V3D testDir;
        testDir.spherical(1, double(elev), double(azimuth));
        if (verbose)
          std::cout << testDir << " : ";
        // Track it
        InstrumentRayTracer tracker(inst);
        tracker.traceFromSample(testDir);
        Links results = tracker.getResults();

        if (verbose)
          showResults(results, inst);
      }
  }

private:
  void showResults(Links &results, Instrument_const_sptr inst) {
    Links::const_iterator resultItr = results.begin();
    for (; resultItr != results.end(); resultItr++) {
      IComponent_const_sptr component =
          inst->getComponentByID(resultItr->componentID);
      std::cout << component->getName() << ", ";
    }
    std::cout << "\n";
  }
};

#endif // InstrumentRayTracerTEST_H_
