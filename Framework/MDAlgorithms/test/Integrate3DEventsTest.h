#ifndef MANTID_MDEVENTS_INTEGRATE_3D_EVENTS_TEST_H_
#define MANTID_MDEVENTS_INTEGRATE_3D_EVENTS_TEST_H_

#include "MantidMDAlgorithms/Integrate3DEvents.h"
#include "MantidKernel/V3D.h"
#include "MantidDataObjects/PeakShapeEllipsoid.h"

#include <cxxtest/TestSuite.h>
#include <iostream>
#include <random>

using namespace Mantid;
using namespace Mantid::DataObjects;
using namespace Mantid::Kernel;
using namespace Mantid::MDAlgorithms;

class Integrate3DEventsTest : public CxxTest::TestSuite {
public:
  // Test support class for integration of events using ellipsoids aligned
  // with the principal axes of the events near a peak.  This test
  // generates some poorly distributed synthetic data, and checks that
  // expected integration results are obtained using either fixed size
  // ellipsoids, or using ellipsoids with axis half-lengths set to
  // three standard deviations.
  void test_1() {
    double inti_all[] = {755, 704, 603};
    double sigi_all[] = {27.4773, 26.533, 24.5561};

    double inti_some[] = {692, 649, 603};
    double sigi_some[] = {27.4590, 26.5141, 24.5561};

    // synthesize three peaks

    V3D peak_1(10, 0, 0);
    V3D peak_2(0, 5, 0);
    V3D peak_3(0, 0, 4);
    std::vector<std::pair<double, V3D>> peak_q_list{
        {1., peak_1}, {1., peak_2}, {1., peak_3}};

    // synthesize a UB-inverse to map
    DblMatrix UBinv(3, 3, false); // Q to h,k,l
    UBinv.setRow(0, V3D(.1, 0, 0));
    UBinv.setRow(1, V3D(0, .2, 0));
    UBinv.setRow(2, V3D(0, 0, .25));

    // synthesize events around the
    // peaks.  All events with in one
    // unit of the peak.  755 events
    // around peak 1, 704 events around
    // peak 2, and 603 events around
    // peak 3.
    std::vector<std::pair<double, V3D>> event_Qs;
    for (int i = -100; i <= 100; i++) {
      event_Qs.push_back(
          std::make_pair(1., V3D(peak_1 + V3D((double)i / 100.0, 0, 0))));
      event_Qs.push_back(
          std::make_pair(1., V3D(peak_2 + V3D((double)i / 100.0, 0, 0))));
      event_Qs.push_back(
          std::make_pair(1., V3D(peak_3 + V3D((double)i / 100.0, 0, 0))));

      event_Qs.push_back(
          std::make_pair(1., V3D(peak_1 + V3D(0, (double)i / 200.0, 0))));
      event_Qs.push_back(
          std::make_pair(1., V3D(peak_2 + V3D(0, (double)i / 200.0, 0))));
      event_Qs.push_back(
          std::make_pair(1., V3D(peak_3 + V3D(0, (double)i / 200.0, 0))));

      event_Qs.push_back(
          std::make_pair(1., V3D(peak_1 + V3D(0, 0, (double)i / 300.0))));
      event_Qs.push_back(
          std::make_pair(1., V3D(peak_2 + V3D(0, 0, (double)i / 300.0))));
      event_Qs.push_back(
          std::make_pair(1., V3D(peak_3 + V3D(0, 0, (double)i / 300.0))));
    }

    for (int i = -50; i <= 50; i++) {
      event_Qs.push_back(
          std::make_pair(1., V3D(peak_1 + V3D(0, (double)i / 147.0, 0))));
      event_Qs.push_back(
          std::make_pair(1., V3D(peak_2 + V3D(0, (double)i / 147.0, 0))));
    }

    for (int i = -25; i <= 25; i++) {
      event_Qs.push_back(
          std::make_pair(1., V3D(peak_1 + V3D(0, 0, (double)i / 61.0))));
    }

    double radius = 1.3;
    Integrate3DEvents integrator(peak_q_list, UBinv, radius);

    integrator.addEvents(event_Qs, false);

    // With fixed size ellipsoids, all the
    // events are counted.
    bool specify_size = true;
    double peak_radius = 1.2;
    double back_inner_radius = 1.2;
    double back_outer_radius = 1.3;
    std::vector<double> new_sigma;
    std::vector<Kernel::V3D> E1Vec;
    double inti;
    double sigi;
    for (size_t i = 0; i < peak_q_list.size(); i++) {
      auto shape = integrator.ellipseIntegrateEvents(
          E1Vec, peak_q_list[i].second, specify_size, peak_radius,
          back_inner_radius, back_outer_radius, new_sigma, inti, sigi);
      TS_ASSERT_DELTA(inti, inti_all[i], 0.1);
      TS_ASSERT_DELTA(sigi, sigi_all[i], 0.01);

      auto ellipsoid_shape = boost::dynamic_pointer_cast<
          const Mantid::DataObjects::PeakShapeEllipsoid>(shape);
      TSM_ASSERT("Expect to get back an ellipsoid shape", ellipsoid_shape);
    }

    // The test data is not normally distributed,
    // so with 3 sigma half-axis sizes, we miss
    // some counts
    specify_size = false;
    for (size_t i = 0; i < peak_q_list.size(); i++) {
      integrator.ellipseIntegrateEvents(
          E1Vec, peak_q_list[i].second, specify_size, peak_radius,
          back_inner_radius, back_outer_radius, new_sigma, inti, sigi);
      TS_ASSERT_DELTA(inti, inti_some[i], 0.1);
      TS_ASSERT_DELTA(sigi, sigi_some[i], 0.01);
    }
  }

  void test_integrateWithStrongPeak() {
    // synthesize two peaks
    V3D peak_1(20, 0, 0);
    V3D peak_2(0, 20, 0);
    std::vector<std::pair<double, V3D>> peak_q_list{{1., peak_1}, {1., peak_2}};

    // synthesize a UB-inverse to map
    DblMatrix UBinv(3, 3, false); // Q to h,k,l
    UBinv.setRow(0, V3D(.1, 0, 0));
    UBinv.setRow(1, V3D(0, .2, 0));
    UBinv.setRow(2, V3D(0, 0, .25));

    std::vector<std::pair<double, V3D>> event_Qs;
    generatePeak(event_Qs, peak_1, 1, 10000, 1); // strong peak
    generatePeak(event_Qs, peak_2, 1, 100, 1);   // weak peak

    IntegrationParameters params(peak_1, 1.2, 0.1, 0.2, true);

    // Create integraton region + events & UB
    Integrate3DEvents integrator(peak_q_list, UBinv, params.regionRadius);
    integrator.addEvents(event_Qs, false);

    double strong_inti, strong_sigi;
    auto result = integrator.integrateStrongPeak(params, strong_inti, strong_sigi);
    const auto shape = boost::dynamic_pointer_cast<const PeakShapeEllipsoid>(result.first);
    const auto frac = result.second;

    TS_ASSERT_DELTA(frac, 0.2948, 0.0001);
    TS_ASSERT_DELTA(strong_inti, 2616.18, 0.01);

    double inti, sigi;
    std::vector<double> sigmas;

    integrator.integrateWeakPeak(params, shape, frac, peak_2, inti, sigi);

    TS_ASSERT_DELTA(inti, 78.0140, 0.001);
  }

  void test_estimateSignalToNoiseRatio() {
    // synthesize two peaks
    V3D peak_1(20, 0, 0);
    V3D peak_2(0, 20, 0);
    std::vector<std::pair<double, V3D>> peak_q_list{{1., peak_1}, {1., peak_2}};

    // synthesize a UB-inverse to map
    DblMatrix UBinv(3, 3, false); // Q to h,k,l
    UBinv.setRow(0, V3D(.1, 0, 0));
    UBinv.setRow(1, V3D(0, .2, 0));
    UBinv.setRow(2, V3D(0, 0, .25));

    std::vector<std::pair<double, V3D>> event_Qs;
    generatePeak(event_Qs, peak_1, 1, 10000, 1); // strong peak
    generatePeak(event_Qs, peak_2, 1, 10, 1);   // weak peak
    generateNoise(event_Qs, -40, 40, 10000);

    IntegrationParameters params(peak_1, 1.2, 0.1, 0.2, true);

    // Create integraton region + events & UB
    Integrate3DEvents integrator(peak_q_list, UBinv, params.regionRadius);
    integrator.addEvents(event_Qs, false);

    const auto ratio1 = integrator.estimateSignalToNoiseRatio(params, peak_1);
    const auto ratio2 = integrator.estimateSignalToNoiseRatio(params, peak_2);

    TS_ASSERT_DELTA(ratio1, 1.9679, 0.0001);
    TS_ASSERT_DELTA(ratio2, 0.0842, 0.0001);
  }

  /** Generate a symmetric Gaussian peak
    *
    * @param event_Qs :: vector of event Qs
    * @param center :: location of the center of the peak
    * @param sigma :: standard deviation of the peak
    * @param numSamples :: number of samples to draw
    * @param seed :: the seed to the pseudo-random number generator
    */
   void generatePeak(std::vector<std::pair<double, V3D>>& event_Qs, V3D center, double sigma = 5, size_t numSamples = 1000, int seed = 1) {

    std::mt19937 gen;
    std::normal_distribution<> d(0,sigma);
    gen.seed(seed);

    for (size_t i = 0; i < numSamples; ++i) {
      V3D offset(d(gen), d(gen), d(gen));
      event_Qs.push_back(std::make_pair(1., center+offset));
    }
  }

   void generateNoise(std::vector<std::pair<double, V3D>>& event_Qs, double lower, double upper, size_t numSamples = 1000, int seed = 1) {

    std::mt19937 gen;
    std::uniform_real_distribution<> d(lower,upper);
    gen.seed(seed);

    for (size_t i = 0; i < numSamples; ++i) {
      V3D point(d(gen), d(gen), d(gen));
      event_Qs.push_back(std::make_pair(1., point));
    }
  }
};

#endif /* MANTID_MDEVENTS_INTEGRATE_3D_EVENTS_TEST_H_ */
