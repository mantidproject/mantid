#ifndef MANTID_CRYSTAL_PEAKSINREGIONTEST_H_
#define MANTID_CRYSTAL_PEAKSINREGIONTEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidAPI/DetectorInfo.h"
#include "MantidCrystal/PeaksInRegion.h"
#include "MantidDataObjects/PeaksWorkspace.h"
#include "MantidTestHelpers/ComponentCreationHelper.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"
#include <boost/tuple/tuple.hpp>

using namespace Mantid::Crystal;
using namespace Mantid::API;
using namespace Mantid::DataObjects;

/*-------------------------------------------------------------------------------------------------------------------------------------------------------
Functional Tests
-------------------------------------------------------------------------------------------------------------------------------------------------------*/
class PeaksInRegionTest : public CxxTest::TestSuite {

private:
  typedef boost::tuple<PeaksWorkspace_sptr, std::vector<double>>
      PeakWorkspaceWithExtents;

  /**
  Helper function. Creates a peaksworkspace with a single peak
  */
  PeakWorkspaceWithExtents
  createPeaksWorkspace(const std::string coordFrame, double xMinFromPeak,
                       double xMaxFromPeak, double yMinFromPeak,
                       double yMaxFromPeak, double zMinFromPeak,
                       double zMaxFromPeak) {
    PeaksWorkspace_sptr ws = WorkspaceCreationHelper::createPeaksWorkspace(1);
    const auto &detectorIds = ws->detectorInfo().detectorIDs();
    Peak &peak = ws->getPeak(0);
    peak.setDetectorID(detectorIds.front());
    Mantid::Kernel::V3D position;
    if (coordFrame == "Detector space") {
      position = peak.getDetector()->getPos();
    } else if (coordFrame == "Q (lab frame)") {
      position = peak.getQLabFrame();
    } else if (coordFrame == "Q (sample frame)") {
      position = peak.getQSampleFrame();
    } else if (coordFrame == "HKL") {
      position = peak.getHKL();
    } else {
      throw std::runtime_error("Unknown coordinate frame");
    }
    std::vector<double> extents(6);
    extents[0] = position.X() - xMinFromPeak;
    extents[1] = position.X() + xMaxFromPeak;
    extents[2] = position.Y() - yMinFromPeak;
    extents[3] = position.Y() + yMaxFromPeak;
    extents[4] = position.Z() - zMinFromPeak;
    extents[5] = position.Z() + zMaxFromPeak;
    return PeakWorkspaceWithExtents(ws, extents);
  }

public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static PeaksInRegionTest *createSuite() { return new PeaksInRegionTest(); }
  static void destroySuite(PeaksInRegionTest *suite) { delete suite; }

  void test_numberOfFaces() {
    PeaksInRegion alg;
    PeaksIntersection &baseAlg = alg;
    TS_ASSERT_EQUALS(6, baseAlg.numberOfFaces());
  }

  void test_setProperties() {
    PeaksInRegion alg;
    alg.setRethrows(true);
    alg.initialize();
    TS_ASSERT(alg.isInitialized());
    alg.setProperty("InputWorkspace",
                    WorkspaceCreationHelper::createPeaksWorkspace());
    alg.setPropertyValue("CoordinateFrame", "Q (lab frame)");
    alg.setPropertyValue("Extents", "-1,1,-1,1,-1,1");
    alg.setPropertyValue("OutputWorkspace", "OutWS");
  }

  void do_test_extents_throws(const std::string &message,
                              const std::string &extents) {
    PeaksInRegion alg;
    alg.setRethrows(true);
    alg.initialize();
    TS_ASSERT(alg.isInitialized());
    alg.setProperty("InputWorkspace",
                    WorkspaceCreationHelper::createPeaksWorkspace());
    alg.setPropertyValue("CoordinateFrame", "Q (lab frame)");
    alg.setPropertyValue("Extents", extents);
    alg.setPropertyValue("OutputWorkspace", "OutWS");

    TSM_ASSERT_THROWS(message, alg.execute(), std::invalid_argument &);
  }

  void test_bad_extent_format_too_few() {
    do_test_extents_throws("Too few extents", "-1,1,-1,1,-1,1,-1");
  }

  void test_bad_extent_format_too_many() {
    do_test_extents_throws("Too many extents", "-1,1,-1,1,-1,1,-1,1,-1");
  }

  void test_bad_extent_pairs() {
    do_test_extents_throws("Invalid x extents", "-1,-1.1,-1,1,-1,1");

    do_test_extents_throws("Invalid y extents", "-1,1,-1,-1.1,-1,1");

    do_test_extents_throws("Invalid z extents", "-1,1,-1,1,-1,-1.1");
  }

  void do_test_within_bounds_center_only(const std::string &coordFrame) {
    const std::string outName = "OutWS";
    const double xMinFromPeak = 1;
    const double xMaxFromPeak = 1;
    const double yMinFromPeak = 1;
    const double yMaxFromPeak = 1;
    const double zMinFromPeak = 1;
    const double zMaxFromPeak = 1;

    PeakWorkspaceWithExtents tuple = createPeaksWorkspace(
        coordFrame, xMinFromPeak, xMaxFromPeak, yMinFromPeak, yMaxFromPeak,
        zMinFromPeak, zMaxFromPeak);

    PeaksInRegion alg;
    alg.setRethrows(true);
    alg.initialize();
    alg.setProperty("InputWorkspace", tuple.get<0>());
    alg.setPropertyValue("CoordinateFrame", coordFrame);
    alg.setProperty("Extents", tuple.get<1>());
    alg.setPropertyValue("OutputWorkspace", outName);
    alg.execute();

    ITableWorkspace_sptr outWS =
        AnalysisDataService::Instance().retrieveWS<ITableWorkspace>(outName);

    TS_ASSERT_EQUALS(3, outWS->columnCount());
    TS_ASSERT_EQUALS("PeakIndex", outWS->getColumn(0)->name());
    TS_ASSERT_EQUALS("Intersecting", outWS->getColumn(1)->name());
    TS_ASSERT_EQUALS("Distance", outWS->getColumn(2)->name());

    TS_ASSERT_EQUALS(1, outWS->rowCount());

    TSM_ASSERT_EQUALS("Peak index should be zero", 0, outWS->cell<int>(0, 0));
    TSM_ASSERT_EQUALS("Peak intersect should be true", Boolean(true),
                      outWS->cell<Boolean>(0, 1));
  }

  void
  do_test_out_of_bounds_center_only(const std::string &coordFrame,
                                    double xMinFromPeak, double xMaxFromPeak,
                                    double yMinFromPeak, double yMaxFromPeak,
                                    double zMinFromPeak, double zMaxFromPeak) {
    const std::string outName = "OutWS";

    PeakWorkspaceWithExtents tuple = createPeaksWorkspace(
        coordFrame, xMinFromPeak, xMaxFromPeak, yMinFromPeak, yMaxFromPeak,
        zMinFromPeak, zMaxFromPeak);
    PeaksInRegion alg;
    alg.setRethrows(true);
    alg.initialize();
    alg.setProperty("InputWorkspace", tuple.get<0>());
    alg.setPropertyValue("CoordinateFrame", coordFrame);
    alg.setProperty("Extents", tuple.get<1>());
    alg.setPropertyValue("OutputWorkspace", outName);
    alg.setProperty("CheckPeakExtents", false);
    alg.execute();

    ITableWorkspace_sptr outWS =
        AnalysisDataService::Instance().retrieveWS<ITableWorkspace>(outName);

    TS_ASSERT_EQUALS(3, outWS->columnCount());
    TS_ASSERT_EQUALS("PeakIndex", outWS->getColumn(0)->name());
    TS_ASSERT_EQUALS("Intersecting", outWS->getColumn(1)->name());
    TS_ASSERT_EQUALS("Distance", outWS->getColumn(2)->name());
    TS_ASSERT_EQUALS(1, outWS->rowCount());

    TSM_ASSERT_EQUALS("Peak index should be zero", 0, outWS->cell<int>(0, 0));
    TSM_ASSERT_EQUALS("Peak intersect should be false", Boolean(false),
                      outWS->cell<Boolean>(0, 1));
  }

  void test_detectorSpace_with_peak_in_bounds() {
    do_test_within_bounds_center_only("Detector space");
  }

  void test_qLab_with_peak_in_bounds() {
    do_test_within_bounds_center_only("Q (lab frame)");
  }

  void test_qSample_with_peak_in_bounds() {
    do_test_within_bounds_center_only("Q (sample frame)");
  }

  void test_HKL_with_peak_in_bounds() {
    do_test_within_bounds_center_only("HKL");
  }

  void test_detectorSpace_with_peaks_out_of_bounds() {
    const std::string coordinateFrame = "Detector space";
    do_test_out_of_bounds_center_only(coordinateFrame, -0.5, 1, 1, 1, 1,
                                      1); // outside xmin
    do_test_out_of_bounds_center_only(coordinateFrame, 1, -0.5, 1, 1, 1,
                                      1); // outside xmax
    do_test_out_of_bounds_center_only(coordinateFrame, 1, 1, -0.5, 1, 1,
                                      1); // outside ymin
    do_test_out_of_bounds_center_only(coordinateFrame, 1, 1, 1, -0.5, 1,
                                      1); // outside ymax
    do_test_out_of_bounds_center_only(coordinateFrame, 1, 1, 1, 1, -0.5,
                                      1); // outside zmin
    do_test_out_of_bounds_center_only(coordinateFrame, 1, 1, 1, 1, 1,
                                      -0.5); // outside zmax
  }

  void test_qLab_with_peaks_out_of_bounds() {
    const std::string coordinateFrame = "Q (lab frame)";
    do_test_out_of_bounds_center_only(coordinateFrame, -0.5, 1, 1, 1, 1,
                                      1); // outside xmin
    do_test_out_of_bounds_center_only(coordinateFrame, 1, -0.5, 1, 1, 1,
                                      1); // outside xmax
    do_test_out_of_bounds_center_only(coordinateFrame, 1, 1, -0.5, 1, 1,
                                      1); // outside ymin
    do_test_out_of_bounds_center_only(coordinateFrame, 1, 1, 1, -0.5, 1,
                                      1); // outside ymax
    do_test_out_of_bounds_center_only(coordinateFrame, 1, 1, 1, 1, -0.5,
                                      1); // outside zmin
    do_test_out_of_bounds_center_only(coordinateFrame, 1, 1, 1, 1, 1,
                                      -0.5); // outside zmax
  }

  void test_qSample_with_peaks_out_of_bounds() {
    const std::string coordinateFrame = "Q (sample frame)";
    do_test_out_of_bounds_center_only(coordinateFrame, -0.5, 1, 1, 1, 1,
                                      1); // outside xmin
    do_test_out_of_bounds_center_only(coordinateFrame, 1, -0.5, 1, 1, 1,
                                      1); // outside xmax
    do_test_out_of_bounds_center_only(coordinateFrame, 1, 1, -0.5, 1, 1,
                                      1); // outside ymin
    do_test_out_of_bounds_center_only(coordinateFrame, 1, 1, 1, -0.5, 1,
                                      1); // outside ymax
    do_test_out_of_bounds_center_only(coordinateFrame, 1, 1, 1, 1, -0.5,
                                      1); // outside zmin
    do_test_out_of_bounds_center_only(coordinateFrame, 1, 1, 1, 1, 1,
                                      -0.5); // outside zmax
  }

  void test_qHKL_with_peaks_out_of_bounds() {
    const std::string coordinateFrame = "HKL";
    do_test_out_of_bounds_center_only(coordinateFrame, -0.5, 1, 1, 1, 1,
                                      1); // outside xmin
    do_test_out_of_bounds_center_only(coordinateFrame, 1, -0.5, 1, 1, 1,
                                      1); // outside xmax
    do_test_out_of_bounds_center_only(coordinateFrame, 1, 1, -0.5, 1, 1,
                                      1); // outside ymin
    do_test_out_of_bounds_center_only(coordinateFrame, 1, 1, 1, -0.5, 1,
                                      1); // outside ymax
    do_test_out_of_bounds_center_only(coordinateFrame, 1, 1, 1, 1, -0.5,
                                      1); // outside zmin
    do_test_out_of_bounds_center_only(coordinateFrame, 1, 1, 1, 1, 1,
                                      -0.5); // outside zmax
  }

  void do_test_bounds_check_extents(const std::string coordFrame,
                                    double xMinFromPeak, double xMaxFromPeak,
                                    double yMinFromPeak, double yMaxFromPeak,
                                    double zMinFromPeak, double zMaxFromPeak,
                                    double radius, bool expectation) {
    const std::string outName = "OutWS";

    PeakWorkspaceWithExtents tuple = createPeaksWorkspace(
        coordFrame, xMinFromPeak, xMaxFromPeak, yMinFromPeak, yMaxFromPeak,
        zMinFromPeak, zMaxFromPeak);
    PeaksInRegion alg;
    alg.setRethrows(true);
    alg.initialize();
    alg.setProperty("InputWorkspace", tuple.get<0>());
    alg.setPropertyValue("CoordinateFrame", coordFrame);
    alg.setProperty("Extents", tuple.get<1>());
    alg.setPropertyValue("OutputWorkspace", outName);
    alg.setProperty("CheckPeakExtents", true);
    alg.setProperty("PeakRadius", radius);
    alg.execute();

    ITableWorkspace_sptr outWS =
        AnalysisDataService::Instance().retrieveWS<ITableWorkspace>(outName);

    TS_ASSERT_EQUALS(1, outWS->rowCount());
    TSM_ASSERT_EQUALS("Peak index should be zero", 0, outWS->cell<int>(0, 0));
    TSM_ASSERT_EQUALS("Peak intersect calculated incorrectly",
                      Boolean(expectation), outWS->cell<Boolean>(0, 1));
  }

  void test_peak_intersects_xmin_boundary_when_radius_large_enough() {
    const std::string coordinateFrame = "Detector space";
    const double wallDistanceFromPeakCenter = 0.5;

    double peakRadius = 0.49; // not enough for the sphere to penetrate the
                              // bounding box. Expect failure
    do_test_bounds_check_extents(coordinateFrame, -wallDistanceFromPeakCenter,
                                 1, 1, 1, 1, 1, peakRadius,
                                 peakRadius > wallDistanceFromPeakCenter);

    peakRadius = 0.51; // just enough for the sphere to penetrate the bounding
                       // box. Expect pass.
    do_test_bounds_check_extents(coordinateFrame, -wallDistanceFromPeakCenter,
                                 1, 1, 1, 1, 1, peakRadius,
                                 peakRadius > wallDistanceFromPeakCenter);
  }

  void test_peak_intersects_xmax_boundary_when_radius_large_enough() {
    const std::string coordinateFrame = "Detector space";
    const double wallDistanceFromPeakCenter = 0.5;

    double peakRadius = 0.49; // not enough for the sphere to penetrate the
                              // bounding box. Expect failure
    do_test_bounds_check_extents(
        coordinateFrame, 1, -wallDistanceFromPeakCenter, 1, 1, 1, 1, peakRadius,
        peakRadius > wallDistanceFromPeakCenter);

    peakRadius = 0.51; // just enough for the sphere to penetrate the bounding
                       // box. Expect pass.
    do_test_bounds_check_extents(
        coordinateFrame, 1, -wallDistanceFromPeakCenter, 1, 1, 1, 1, peakRadius,
        peakRadius > wallDistanceFromPeakCenter);
  }

  void test_peak_intersects_ymin_boundary_when_radius_large_enough() {
    const std::string coordinateFrame = "Detector space";
    const double wallDistanceFromPeakCenter = 0.5;

    double peakRadius = 0.49; // not enough for the sphere to penetrate the
                              // bounding box. Expect failure
    do_test_bounds_check_extents(
        coordinateFrame, 1, 1, -wallDistanceFromPeakCenter, 1, 1, 1, peakRadius,
        peakRadius > wallDistanceFromPeakCenter);

    peakRadius = 0.51; // just enough for the sphere to penetrate the bounding
                       // box. Expect pass.
    do_test_bounds_check_extents(
        coordinateFrame, 1, 1, -wallDistanceFromPeakCenter, 1, 1, 1, peakRadius,
        peakRadius > wallDistanceFromPeakCenter);
  }

  void test_peak_intersects_ymax_boundary_when_radius_large_enough() {
    const std::string coordinateFrame = "Detector space";
    const double wallDistanceFromPeakCenter = 0.5;

    double peakRadius = 0.49; // not enough for the sphere to penetrate the
                              // bounding box. Expect failure
    do_test_bounds_check_extents(coordinateFrame, 1, 1, 1,
                                 -wallDistanceFromPeakCenter, 1, 1, peakRadius,
                                 peakRadius > wallDistanceFromPeakCenter);

    peakRadius = 0.51; // just enough for the sphere to penetrate the bounding
                       // box. Expect pass.
    do_test_bounds_check_extents(coordinateFrame, 1, 1, 1,
                                 -wallDistanceFromPeakCenter, 1, 1, peakRadius,
                                 peakRadius > wallDistanceFromPeakCenter);
  }

  void test_peak_intersects_zmin_boundary_when_radius_large_enough() {
    const std::string coordinateFrame = "Detector space";
    const double wallDistanceFromPeakCenter = 0.5;

    double peakRadius = 0.49; // not enough for the sphere to penetrate the
                              // bounding box. Expect failure
    do_test_bounds_check_extents(coordinateFrame, 1, 1, 1, 1,
                                 -wallDistanceFromPeakCenter, 1, peakRadius,
                                 peakRadius > wallDistanceFromPeakCenter);

    peakRadius = 0.51; // just enough for the sphere to penetrate the bounding
                       // box. Expect pass.
    do_test_bounds_check_extents(coordinateFrame, 1, 1, 1, 1,
                                 -wallDistanceFromPeakCenter, 1, peakRadius,
                                 peakRadius > wallDistanceFromPeakCenter);
  }

  void test_peak_intersects_zmax_boundary_when_radius_large_enough() {
    const std::string coordinateFrame = "Detector space";
    const double wallDistanceFromPeakCenter = 0.5;

    double peakRadius = 0.49; // not enough for the sphere to penetrate the
                              // bounding box. Expect failure
    do_test_bounds_check_extents(coordinateFrame, 1, 1, 1, 1, 1,
                                 -wallDistanceFromPeakCenter, peakRadius,
                                 peakRadius > wallDistanceFromPeakCenter);

    peakRadius = 0.51; // just enough for the sphere to penetrate the bounding
                       // box. Expect pass.
    do_test_bounds_check_extents(coordinateFrame, 1, 1, 1, 1, 1,
                                 -wallDistanceFromPeakCenter, peakRadius,
                                 peakRadius > wallDistanceFromPeakCenter);
  }

  void test_false_intersection_when_check_peak_extents() {
    std::vector<double> extents = {
        0, 1, 0, 1, 0, 1}; // Extents go from 0, 1 in each dimension.

    PeaksWorkspace_sptr ws = WorkspaceCreationHelper::createPeaksWorkspace(1);
    Peak &peak = ws->getPeak(0);
    peak.setHKL(Mantid::Kernel::V3D(2, 0, 0)); // This point is actually on the
                                               // y = 0 plane, i.e. satisfies
                                               // the plane equation. aX + bY +
                                               // cZ = 0, but is outside the
                                               // box.

    const std::string outName = "OutWS";

    PeaksInRegion alg;
    alg.setRethrows(true);
    alg.initialize();
    alg.setProperty("InputWorkspace", ws);
    alg.setPropertyValue("CoordinateFrame", "HKL");
    alg.setProperty("Extents", extents);
    alg.setPropertyValue("OutputWorkspace", outName);
    alg.setProperty("CheckPeakExtents",
                    true); // This wouldn't happen if CheckPeak extents = false.
    alg.setProperty("PeakRadius", 0.1);
    alg.execute();

    ITableWorkspace_sptr outWS =
        AnalysisDataService::Instance().retrieveWS<ITableWorkspace>(outName);

    TS_ASSERT_EQUALS(1, outWS->rowCount());
    TSM_ASSERT_EQUALS("Peak index should be zero", 0, outWS->cell<int>(0, 0));
    TSM_ASSERT_EQUALS("Peak does NOT intersect the box", Boolean(false),
                      outWS->cell<Boolean>(0, 1));
  }
};

/*-------------------------------------------------------------------------------------------------------------------------------------------------------
Perfomance Tests
-------------------------------------------------------------------------------------------------------------------------------------------------------*/
class PeaksInRegionTestPerformance : public CxxTest::TestSuite {

private:
  Mantid::API::IPeaksWorkspace_sptr inputWS;

public:
  static PeaksInRegionTestPerformance *createSuite() {
    return new PeaksInRegionTestPerformance();
  }
  static void destroySuite(PeaksInRegionTestPerformance *suite) {
    delete suite;
  }

  PeaksInRegionTestPerformance() {
    int numPeaks = 4000;
    inputWS = boost::make_shared<PeaksWorkspace>();
    Mantid::Geometry::Instrument_sptr inst =
        ComponentCreationHelper::createTestInstrumentRectangular2(1, 200);
    inputWS->setInstrument(inst);

    for (int i = 0; i < numPeaks; ++i) {
      Peak peak(inst, i, i + -0.5);
      inputWS->addPeak(peak);
    }
  }

  void test_performance_peak_centers_only() {
    const std::string outName = "OutPerfWS";

    PeaksInRegion alg;
    alg.setRethrows(true);
    alg.initialize();
    TS_ASSERT(alg.isInitialized());
    alg.setProperty("InputWorkspace", inputWS);
    alg.setPropertyValue("CoordinateFrame", "Detector space");
    alg.setPropertyValue("Extents", "-1,1,-1,1,-1,1");
    alg.setPropertyValue("OutputWorkspace", outName);
    alg.setProperty("CheckPeakExtents", false);
    alg.execute();

    Mantid::API::ITableWorkspace_sptr outWS =
        AnalysisDataService::Instance().retrieveWS<ITableWorkspace>(outName);

    TS_ASSERT_EQUALS(3, outWS->columnCount());
    TS_ASSERT_EQUALS(inputWS->rowCount(), outWS->rowCount());
  }

  void test_performance_peak_extents_checking() {
    const std::string outName = "OutPerfWS";

    PeaksInRegion alg;
    alg.setRethrows(true);
    alg.initialize();
    TS_ASSERT(alg.isInitialized());
    alg.setProperty("InputWorkspace", inputWS);
    alg.setPropertyValue("CoordinateFrame", "Detector space");
    alg.setPropertyValue("Extents", "0.5,1,-1,1,-1,1");
    alg.setPropertyValue("OutputWorkspace", outName);
    alg.setProperty("CheckPeakExtents", true);
    alg.setProperty("PeakRadius", 0.4);
    alg.execute();

    Mantid::API::ITableWorkspace_sptr outWS =
        AnalysisDataService::Instance().retrieveWS<ITableWorkspace>(outName);

    TS_ASSERT_EQUALS(3, outWS->columnCount());
    TS_ASSERT_EQUALS(inputWS->rowCount(), outWS->rowCount());
  }
};

#endif /* MANTID_CRYSTAL_PEAKSINREGIONTEST_H_ */
