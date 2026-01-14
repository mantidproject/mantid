// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/IMDEventWorkspace.h"
#include "MantidDataObjects/MDEventFactory.h"
#include "MantidDataObjects/PeakShapeSpherical.h"
#include "MantidDataObjects/PeaksWorkspace.h"
#include "MantidFrameworkTestHelpers/ComponentCreationHelper.h"
#include "MantidGeometry/MDGeometry/HKL.h"
#include "MantidGeometry/MDGeometry/MDHistoDimension.h"
#include "MantidKernel/UnitLabelTypes.h"
#include "MantidMDAlgorithms/CreateMDWorkspace.h"
#include "MantidMDAlgorithms/FakeMDEventData.h"
#include "MantidMDAlgorithms/IntegratePeaksMDHKL.h"

#include <boost/math/distributions/normal.hpp>
#include <boost/math/special_functions/pow.hpp>

#include <cxxtest/TestSuite.h>

using Mantid::Geometry::MDHistoDimension;
using namespace Mantid::API;
using namespace Mantid::DataObjects;
using namespace Mantid::Geometry;
using namespace Mantid::MDAlgorithms;
using Mantid::Kernel::V3D;

class IntegratePeaksMDHKLTest : public CxxTest::TestSuite {
public:
  // This means the constructor isn't called when running other tests
  static IntegratePeaksMDHKLTest *createSuite() { return new IntegratePeaksMDHKLTest(); }
  static void destroySuite(IntegratePeaksMDHKLTest *suite) { delete suite; }

  IntegratePeaksMDHKLTest() { Mantid::API::FrameworkManager::Instance(); }
  ~IntegratePeaksMDHKLTest() override = default;

  void test_Init() {
    IntegratePeaksMDHKL alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
  }

  //-------------------------------------------------------------------------------
  /** Create the (blank) MDEW */
  static void createMDEW(const std::string &mdWS) {
    // ---- Start with empty MDEW ----

    CreateMDWorkspace algC;
    TS_ASSERT_THROWS_NOTHING(algC.initialize())
    TS_ASSERT(algC.isInitialized())
    TS_ASSERT_THROWS_NOTHING(algC.setProperty("Dimensions", "3"));
    TS_ASSERT_THROWS_NOTHING(algC.setProperty("Extents", "0,2,0,2,0,2"));

    TS_ASSERT_THROWS_NOTHING(algC.setProperty("Names", "[H,0,0],[0,K,0],[0,0,L]"));
    std::string units = Mantid::Kernel::Units::Symbol::RLU.ascii() + "," + Mantid::Kernel::Units::Symbol::RLU.ascii() +
                        "," + Mantid::Kernel::Units::Symbol::RLU.ascii();
    TS_ASSERT_THROWS_NOTHING(algC.setProperty("Units", units));
    TS_ASSERT_THROWS_NOTHING(algC.setProperty("SplitInto", "5"));
    TS_ASSERT_THROWS_NOTHING(algC.setProperty("MaxRecursionDepth", "2"));
    std::string frames =
        Mantid::Geometry::HKL::HKLName + "," + Mantid::Geometry::HKL::HKLName + "," + Mantid::Geometry::HKL::HKLName;
    TS_ASSERT_THROWS_NOTHING(algC.setProperty("Frames", frames));
    TS_ASSERT_THROWS_NOTHING(algC.setPropertyValue("OutputWorkspace", mdWS));
    TS_ASSERT_THROWS_NOTHING(algC.execute());
    TS_ASSERT(algC.isExecuted());
  }

  //-------------------------------------------------------------------------------
  /** Add a fake peak */
  static void addPeakAndBackground(const std::string &mdWS, size_t num, double x, double y, double z, double radius,
                                   size_t numBg) {
    std::ostringstream pk_stream;
    pk_stream << num << ", " << x << ", " << y << ", " << z << ", " << radius;
    FakeMDEventData algF;
    TS_ASSERT_THROWS_NOTHING(algF.initialize())
    TS_ASSERT(algF.isInitialized())
    TS_ASSERT_THROWS_NOTHING(algF.setPropertyValue("InputWorkspace", mdWS));
    TS_ASSERT_THROWS_NOTHING(algF.setProperty("PeakParams", pk_stream.str().c_str()));
    TS_ASSERT_THROWS_NOTHING(algF.setProperty("RandomSeed", "63759"));
    if (numBg > 0) {
      std::ostringstream bg_stream;
      bg_stream << numBg;
      TS_ASSERT_THROWS_NOTHING(algF.setProperty("UniformParams", bg_stream.str().c_str()));
    }
    TS_ASSERT_THROWS_NOTHING(algF.execute());
    TS_ASSERT(algF.isExecuted());
  }

  static void doRun(const std::string &peakWS, const std::string &mdWS, const bool doBg = false) {
    IntegratePeaksMDHKL alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("InputWorkspace", mdWS));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("PeaksWorkspace", peakWS));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("OutputWorkspace", peakWS));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("DeltaHKL", "1.0"));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("GridPoints", "21"));
    if (doBg) {
      TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("BackgroundInnerRadius", "0.5"));
      TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("BackgroundouterRadius", "0.65"));
    }
    TS_ASSERT_THROWS_NOTHING(alg.execute());
    TS_ASSERT(alg.isExecuted());
  }

  //-------------------------------------------------------------------------------
  /** Full test using faked-out peak data */
  void test_exec() {
    // --- Fake workspace with 3 peaks ------
    const std::string mdName = "IntegratePeaksMDHKLTest_MDEWS_nobg";
    createMDEW(mdName);
    const size_t neventsPeak = 10000;
    const size_t neventsBg = 0;
    addPeakAndBackground(mdName, neventsPeak, 1., 1., 1., 0.5, neventsBg); // bg has 1 event

    auto mdews = AnalysisDataService::Instance().retrieveWS<MDEventWorkspace3Lean>(mdName);
    auto &frame = mdews->getDimension(0)->getMDFrame();
    TSM_ASSERT_EQUALS("Should be HKL", Mantid::Geometry::HKL::HKLName, frame.name());
    TS_ASSERT_EQUALS(mdews->getNPoints(), neventsPeak + neventsBg);
    TS_ASSERT_DELTA(mdews->getBox()->getSignal(), neventsPeak + neventsBg, 1); // randomised signal but approx nevents

    // Make a fake instrument - doesn't matter, we won't use it really
    Instrument_sptr inst = ComponentCreationHelper::createTestInstrumentRectangular(1, 100, 0.05);

    // --- Make a fake PeaksWorkspace ---
    auto peakWS = std::make_shared<PeaksWorkspace>();
    peakWS->setInstrument(inst);
    Peak Pin(inst, 15050, 1.0);
    Pin.setHKL(V3D(1, 1, 1));
    peakWS->addPeak(Pin);

    TS_ASSERT_EQUALS(peakWS->getPeak(0).getIntensity(), 0.0);
    const std::string peaksName = "IntegratePeaksMDHKLTest_peaks_nobg";
    AnalysisDataService::Instance().add(peaksName, peakWS);

    doRun(peaksName, mdName, false);

    TS_ASSERT_DELTA(peakWS->getPeak(0).getIntensity(), neventsPeak, 50.0);
    TS_ASSERT_DELTA(peakWS->getPeak(0).getSigmaIntensity(), sqrt(neventsPeak), 1.0);
  }

  //-------------------------------------------------------------------------------
  /// Integrate background between start/end background radius
  void test_exec_shellBackground() {

    // --- Fake workspace with 3 peaks ------
    const std::string mdName = "IntegratePeaksMDHKLTest_MDEWS_bg";
    createMDEW(mdName);
    size_t neventsPeak = 10000;
    auto neventsBg =
        static_cast<size_t>(0.5 * static_cast<double>(neventsPeak) * 8 / (4 * M_PI * pow(0.5, 3) / 3)); // half density
    addPeakAndBackground(mdName, neventsPeak, 1., 1., 1., 0.5, neventsBg);

    auto mdews = AnalysisDataService::Instance().retrieveWS<MDEventWorkspace3Lean>(mdName);
    auto &frame = mdews->getDimension(0)->getMDFrame();
    TSM_ASSERT_EQUALS("Should be HKL", Mantid::Geometry::HKL::HKLName, frame.name());
    TS_ASSERT_EQUALS(mdews->getNPoints(), neventsPeak + neventsBg);
    TS_ASSERT_DELTA(mdews->getBox()->getSignal(), neventsPeak + neventsBg, 1); // randomised signal but approx nevents

    // Make a fake instrument - doesn't matter, we won't use it really
    Instrument_sptr inst = ComponentCreationHelper::createTestInstrumentRectangular(1, 100, 0.05);

    // --- Make a fake PeaksWorkspace ---
    auto peakWS = std::make_shared<PeaksWorkspace>();
    peakWS->setInstrument(inst);
    Peak Pin(inst, 15050, 1.0);
    Pin.setHKL(V3D(1, 1, 1));
    peakWS->addPeak(Pin);

    TS_ASSERT_EQUALS(peakWS->getPeak(0).getIntensity(), 0.0);
    const std::string peaksName = "IntegratePeaksMDHKLTest_peaks_bg";
    AnalysisDataService::Instance().add(peaksName, peakWS);

    doRun(peaksName, mdName, true);

    TS_ASSERT_DELTA(peakWS->getPeak(0).getIntensity(), neventsPeak, 500.0);
    // Error is larger, since it is quadrature sum of error of peak and bg (scaled by vol.)
    TS_ASSERT_DELTA(peakWS->getPeak(0).getSigmaIntensity(), 537.1, 1.0);
  }
};
