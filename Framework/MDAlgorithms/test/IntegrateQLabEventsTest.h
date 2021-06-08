// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidDataObjects/PeakShapeEllipsoid.h"
#include "MantidKernel/V3D.h"
#include "MantidMDAlgorithms/IntegrateQLabEvents.h"

#include <cxxtest/TestSuite.h>
#include <random>

using namespace Mantid;
using namespace Mantid::DataObjects;
using namespace Mantid::Kernel;
using namespace Mantid::MDAlgorithms;

class IntegrateQLabEventsTest : public CxxTest::TestSuite {
public:
  /* Test support class for integration of events using ellipsoids aligned
   * with the principal axes of the events near a peak.  This test
   * generates some poorly distributed synthetic data, and checks that
   * expected integration results are obtained using either fixed size
   * ellipsoids, or using ellipsoids with axis half-lengths set to
   * three standard deviations. */
  void test_integrateMainPeaksWithFixedRadiiandDefaultScaledRadii() {
    /* This test is an adaptation of the test with the same name in
     * suite IntegrateQ3DEventsTest */
    double inti_all[] = {755, 704, 603};
    double sigi_all[] = {27.4773, 26.533, 24.5561};

    double inti_some[] = {692, 649, 603};
    double sigi_some[] = {27.4590, 26.5141, 24.5561};

    // synthesize three peaks

    V3D peak_1(10, 0, 0);
    V3D peak_2(0, 5, 0);
    V3D peak_3(0, 0, 4);
    std::vector<std::pair<std::pair<double, double>, V3D>> peak_q_list{
        {std::make_pair(1., 1.), peak_1}, {std::make_pair(1., 1.), peak_2}, {std::make_pair(1., 1.), peak_3}};

    /* synthesize events around the peaks. All events with in one unit of the
     * peak. 755 events around peak 1, 704 events around peak 2, and
     * 603 events around peak 3.*/
    std::vector<std::pair<std::pair<double, double>, V3D>> event_Qs;
    for (int i = -100; i <= 100; i++) {
      event_Qs.emplace_back(std::make_pair(std::make_pair(2., 1.), V3D(peak_1 + V3D((double)i / 100.0, 0, 0))));
      event_Qs.emplace_back(std::make_pair(std::make_pair(2., 1.), V3D(peak_2 + V3D((double)i / 100.0, 0, 0))));
      event_Qs.emplace_back(std::make_pair(std::make_pair(2., 1.), V3D(peak_3 + V3D((double)i / 100.0, 0, 0))));

      event_Qs.emplace_back(std::make_pair(std::make_pair(2., 1.), V3D(peak_1 + V3D(0, (double)i / 200.0, 0))));
      event_Qs.emplace_back(std::make_pair(std::make_pair(2., 1.), V3D(peak_2 + V3D(0, (double)i / 200.0, 0))));
      event_Qs.emplace_back(std::make_pair(std::make_pair(2., 1.), V3D(peak_3 + V3D(0, (double)i / 200.0, 0))));

      event_Qs.emplace_back(std::make_pair(std::make_pair(2., 1.), V3D(peak_1 + V3D(0, 0, (double)i / 300.0))));
      event_Qs.emplace_back(std::make_pair(std::make_pair(2., 1.), V3D(peak_2 + V3D(0, 0, (double)i / 300.0))));
      event_Qs.emplace_back(std::make_pair(std::make_pair(2., 1.), V3D(peak_3 + V3D(0, 0, (double)i / 300.0))));
    }

    for (int i = -50; i <= 50; i++) {
      event_Qs.emplace_back(std::make_pair(std::make_pair(2., 1.), V3D(peak_1 + V3D(0, (double)i / 147.0, 0))));
      event_Qs.emplace_back(std::make_pair(std::make_pair(2., 1.), V3D(peak_2 + V3D(0, (double)i / 147.0, 0))));
    }

    for (int i = -25; i <= 25; i++) {
      event_Qs.emplace_back(std::make_pair(std::make_pair(2., 1.), V3D(peak_1 + V3D(0, 0, (double)i / 61.0))));
    }

    double radius = 1.3;
    IntegrateQLabEvents integrator(peak_q_list, radius);

    integrator.addEvents(event_Qs);
    integrator.populateCellsWithPeaks();

    // With fixed size ellipsoids, all the events are counted.
    bool specify_size = true;
    double peak_radius = 1.2;
    double back_inner_radius = 1.2;
    double back_outer_radius = 1.3;
    std::vector<double> new_sigma;
    std::vector<Kernel::V3D> E1Vec;
    double inti;
    double sigi;
    for (size_t i = 0; i < peak_q_list.size(); i++) {
      auto shape = integrator.ellipseIntegrateEvents(E1Vec, peak_q_list[i].second, specify_size, peak_radius,
                                                     back_inner_radius, back_outer_radius, new_sigma, inti, sigi);
      TS_ASSERT_DELTA(inti, 2 * inti_all[i], 0.1);
      TS_ASSERT_DELTA(sigi, sigi_all[i], 0.01);

      auto ellipsoid_shape = std::dynamic_pointer_cast<const Mantid::DataObjects::PeakShapeEllipsoid>(shape);
      TSM_ASSERT("Expect to get back an ellipsoid shape", ellipsoid_shape);
    }

    // The test data is not normally distributed, so with 3 sigma half-axis
    // sizes, we miss some counts
    specify_size = false;
    for (size_t i = 0; i < peak_q_list.size(); i++) {
      integrator.ellipseIntegrateEvents(E1Vec, peak_q_list[i].second, specify_size, peak_radius, back_inner_radius,
                                        back_outer_radius, new_sigma, inti, sigi);
      TS_ASSERT_DELTA(inti, 2 * inti_some[i], 0.1);
      TS_ASSERT_DELTA(sigi, sigi_some[i], 0.01);
    }
  }
};
