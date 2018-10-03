// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_MDEVENTS_MDCENTROIDPEAKS2TEST_H_
#define MANTID_MDEVENTS_MDCENTROIDPEAKS2TEST_H_

#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/IMDEventWorkspace.h"
#include "MantidDataObjects/MDEventFactory.h"
#include "MantidDataObjects/PeaksWorkspace.h"
#include "MantidGeometry/MDGeometry/HKL.h"
#include "MantidGeometry/MDGeometry/MDHistoDimension.h"
#include "MantidGeometry/MDGeometry/QLab.h"
#include "MantidGeometry/MDGeometry/QSample.h"
#include "MantidKernel/UnitLabelTypes.h"
#include "MantidMDAlgorithms/CentroidPeaksMD2.h"
#include "MantidMDAlgorithms/CreateMDWorkspace.h"
#include "MantidMDAlgorithms/FakeMDEventData.h"
#include "MantidTestHelpers/ComponentCreationHelper.h"

#include <boost/math/distributions/normal.hpp>
#include <boost/math/special_functions/pow.hpp>

#include <cxxtest/TestSuite.h>

using Mantid::API::AnalysisDataService;
using Mantid::Geometry::MDHistoDimension;
using namespace Mantid::API;
using namespace Mantid::DataObjects;
using namespace Mantid::Geometry;
using namespace Mantid::MDAlgorithms;
using Mantid::Kernel::V3D;

class CentroidPeaksMD2Test : public CxxTest::TestSuite {
public:
  void test_Init() {
    CentroidPeaksMD2 alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
  }

  //-------------------------------------------------------------------------------
  /** Create the (blank) MDEW */
  static void createMDEW(std::string CoordinatesToUse) {
    // ---- Start with empty MDEW ----
    std::string frames;
    if (CoordinatesToUse == "Q (lab frame)") {
      frames = Mantid::Geometry::QLab::QLabName + "," +
               Mantid::Geometry::QLab::QLabName + "," +
               Mantid::Geometry::QLab::QLabName;
    } else if (CoordinatesToUse == "Q (sample frame)") {
      frames = Mantid::Geometry::QSample::QSampleName + "," +
               Mantid::Geometry::QSample::QSampleName + "," +
               Mantid::Geometry::QSample::QSampleName;
    } else if (CoordinatesToUse == "HKL") {
      frames = Mantid::Geometry::HKL::HKLName + "," +
               Mantid::Geometry::HKL::HKLName + "," +
               Mantid::Geometry::HKL::HKLName;
    }
    CreateMDWorkspace algC;
    TS_ASSERT_THROWS_NOTHING(algC.initialize())
    TS_ASSERT(algC.isInitialized())
    TS_ASSERT_THROWS_NOTHING(algC.setProperty("Dimensions", "3"));
    TS_ASSERT_THROWS_NOTHING(
        algC.setProperty("Extents", "-10,10,-10,10,-10,10"));
    TS_ASSERT_THROWS_NOTHING(algC.setProperty("Names", "h,k,l"));
    std::string units = Mantid::Kernel::Units::Symbol::RLU.ascii() + "," +
                        Mantid::Kernel::Units::Symbol::RLU.ascii() + "," +
                        Mantid::Kernel::Units::Symbol::RLU.ascii();
    TS_ASSERT_THROWS_NOTHING(algC.setProperty("Units", units));
    TS_ASSERT_THROWS_NOTHING(algC.setProperty("Frames", frames));
    TS_ASSERT_THROWS_NOTHING(algC.setProperty("SplitInto", "5"));
    TS_ASSERT_THROWS_NOTHING(algC.setProperty("MaxRecursionDepth", "2"));
    TS_ASSERT_THROWS_NOTHING(
        algC.setPropertyValue("OutputWorkspace", "CentroidPeaksMD2Test_MDEWS"));
    TS_ASSERT_THROWS_NOTHING(algC.execute());
    TS_ASSERT(algC.isExecuted());
  }

  //-------------------------------------------------------------------------------
  /** Add a fake "peak"*/
  static void addPeak(size_t num, double x, double y, double z, double radius) {
    std::ostringstream mess;
    mess << num << ", " << x << ", " << y << ", " << z << ", " << radius;
    FakeMDEventData algF;
    TS_ASSERT_THROWS_NOTHING(algF.initialize())
    TS_ASSERT(algF.isInitialized())
    TS_ASSERT_THROWS_NOTHING(
        algF.setPropertyValue("InputWorkspace", "CentroidPeaksMD2Test_MDEWS"));
    TS_ASSERT_THROWS_NOTHING(
        algF.setProperty("PeakParams", mess.str().c_str()));
    TS_ASSERT_THROWS_NOTHING(algF.setProperty("RandomSeed", "1234"));
    TS_ASSERT_THROWS_NOTHING(algF.execute());
    TS_ASSERT(algF.isExecuted());
  }

  //-------------------------------------------------------------------------------
  /** Run the CentroidPeaksMD2 with the given peak radius param */
  void doRun(V3D startPos, double PeakRadius, double binCount,
             V3D expectedResult, std::string message,
             std::string OutputWorkspace = "CentroidPeaksMD2Test_Peaks") {
    // Make a fake instrument - doesn't matter, we won't use it really
    Instrument_sptr inst =
        ComponentCreationHelper::createTestInstrumentCylindrical(5);

    // --- Make a fake PeaksWorkspace in the given coordinate space ---
    PeaksWorkspace_sptr peakWS(new PeaksWorkspace());

    Peak pIn(inst, 1, 1.0, startPos);
    if (CoordinatesToUse == "Q (lab frame)")
      pIn.setQLabFrame(startPos, 1 /*sample to detector distance*/);
    else if (CoordinatesToUse == "Q (sample frame)")
      pIn.setQSampleFrame(startPos, 1 /*sample to detector distance*/);
    else if (CoordinatesToUse == "HKL")
      pIn.setHKL(startPos);
    peakWS->addPeak(pIn);

    TS_ASSERT_EQUALS(peakWS->getPeak(0).getIntensity(), 0.0);
    AnalysisDataService::Instance().addOrReplace("CentroidPeaksMD2Test_Peaks",
                                                 peakWS);

    CentroidPeaksMD2 alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
    TS_ASSERT_THROWS_NOTHING(
        alg.setPropertyValue("InputWorkspace", "CentroidPeaksMD2Test_MDEWS"));
    TS_ASSERT_THROWS_NOTHING(
        alg.setPropertyValue("PeaksWorkspace", "CentroidPeaksMD2Test_Peaks"));
    TS_ASSERT_THROWS_NOTHING(
        alg.setPropertyValue("OutputWorkspace", OutputWorkspace));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("PeakRadius", PeakRadius));
    TS_ASSERT_THROWS_NOTHING(alg.execute());
    TS_ASSERT(alg.isExecuted());

    peakWS = boost::dynamic_pointer_cast<PeaksWorkspace>(
        AnalysisDataService::Instance().retrieve(OutputWorkspace));
    TS_ASSERT(peakWS);
    if (!peakWS)
      return;

    // Compare the result to the expectation
    V3D result;
    IPeak &p = peakWS->getPeak(0);
    if (CoordinatesToUse == "Q (lab frame)")
      result = p.getQLabFrame();
    else if (CoordinatesToUse == "Q (sample frame)") {
      std::cerr << p.getGoniometerMatrix() << '\n';
      result = p.getQSampleFrame();
    } else if (CoordinatesToUse == "HKL")
      result = p.getHKL();
    TSM_ASSERT_DELTA(message, p.getBinCount(), binCount, 0.05);

    for (size_t i = 0; i < 3; i++)
      TSM_ASSERT_DELTA(message, result[i], expectedResult[i], 0.05);

    AnalysisDataService::Instance().remove("CentroidPeaksMD2Test_Peaks");
  }

  //-------------------------------------------------------------------------------
  /** Full test using faked-out peak data */
  void do_test_exec() {
    // --- Fake workspace with 3 peaks ------
    createMDEW(CoordinatesToUse);
    addPeak(1000, 0, 0., 0., 1.0);
    addPeak(1000, 2., 3., 4., 0.5);
    addPeak(1000, 6., 6., 6., 2.0);

    MDEventWorkspace3Lean::sptr mdews =
        AnalysisDataService::Instance().retrieveWS<MDEventWorkspace3Lean>(
            "CentroidPeaksMD2Test_MDEWS");
    TS_ASSERT_EQUALS(mdews->getNPoints(), 3000);
    TS_ASSERT_DELTA(mdews->getBox()->getSignal(), 3000.0, 1e-2);

    if (CoordinatesToUse == "HKL") {
      mdews->setCoordinateSystem(Mantid::Kernel::HKL);
      doRun(V3D(0., 0., 0.), 1.0, 1000., V3D(0., 0., 0.),
            "Start at the center, get the center");

      doRun(V3D(0.2, 0.2, 0.2), 1.8, 1000., V3D(0., 0., 0.),
            "Somewhat off center");
    } else if (CoordinatesToUse == "Q (lab frame)") {
      mdews->setCoordinateSystem(Mantid::Kernel::QLab);
    } else if (CoordinatesToUse == "Q (sample frame)") {
      mdews->setCoordinateSystem(Mantid::Kernel::QSample);
    }

    doRun(V3D(2., 3., 4.), 1.0, 1000., V3D(2., 3., 4.),
          "Start at the center, get the center");

    doRun(V3D(1.5, 2.5, 3.5), 3.0, 1000., V3D(2., 3., 4.), "Pretty far off");

    doRun(V3D(1.0, 1.5, 2.0), 4.0, 2000., V3D(1.0, 1.5, 2.0),
          "Include two peaks, get the centroid of the two");

    doRun(V3D(8.0, 0.0, 1.0), 1.0, 0., V3D(8.0, 0.0, 1.0),
          "Include no events, get no change");

    doRun(V3D(6., 6., 6.), 0.1, 0., V3D(6., 6., 6.),
          "Small radius still works");

    AnalysisDataService::Instance().remove("CentroidPeaksMD2Test_MDEWS");
  }

  void test_exec_HKL() {
    CoordinatesToUse = "HKL";
    do_test_exec();
  }

  void test_exec_QSampleFrame() {
    CoordinatesToUse = "Q (sample frame)";
    do_test_exec();
  }

  void test_exec_QLabFrame() {
    CoordinatesToUse = "Q (lab frame)";
    do_test_exec();
  }

  void test_exec_HKL_NotInPlace() {
    CoordinatesToUse = "HKL";
    createMDEW(CoordinatesToUse);
    addPeak(1000, 0, 0., 0., 1.0);
    doRun(V3D(0., 0., 0.), 1.0, 1000., V3D(0., 0., 0.),
          "Start at the center, get the center",
          "CentroidPeaksMD2Test_MDEWS_outputCopy");
  }

private:
  std::string CoordinatesToUse;
};

#endif /* MANTID_MDEVENTS_MDCENTROIDPEAKS2TEST_H_ */
