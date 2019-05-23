// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_ALGORITHMS_RADIUSSUMTEST_H_
#define MANTID_ALGORITHMS_RADIUSSUMTEST_H_

#include "MantidAPI/NumericAxis.h"
#include "MantidAlgorithms/RadiusSum.h"
#include "MantidKernel/Unit.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"
#include "RingProfileTest.h"
#include <boost/shared_ptr.hpp>
#include <cxxtest/TestSuite.h>
using Mantid::Algorithms::RadiusSum;
using namespace Mantid::API;

class RadiusSumTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static RadiusSumTest *createSuite() { return new RadiusSumTest(); }
  static void destroySuite(RadiusSumTest *suite) { delete suite; }

  // released automatically when it goes out of scope
  boost::shared_ptr<RadiusSum> algInstance() {
    boost::shared_ptr<RadiusSum> alg = boost::make_shared<RadiusSum>();
    TS_ASSERT_THROWS_NOTHING(alg->initialize());
    TS_ASSERT(alg->isInitialized());
    return alg;
  }

  void test_wrongInputs() {
    std::string outWSName("RadiusSumTest_OutputWS");

    { // check numbins, min and max radius
      auto alg = algInstance();
      // check numbins is only integer > 1
      TS_ASSERT_THROWS(alg->setProperty("NumBins", -3),
                       const std::invalid_argument &);
      TS_ASSERT_THROWS_NOTHING(alg->setProperty("MinRadius", 1.0));
      TS_ASSERT_THROWS_NOTHING(alg->setProperty("MaxRadius", 0.1));
      MatrixWorkspace_sptr goodWS = RingProfileTest::create_2d_workspace();
      TS_ASSERT_THROWS_NOTHING(alg->setProperty("InputWorkspace", goodWS));
      std::vector<double> twoInputs(2, 0);
      TS_ASSERT_THROWS_NOTHING(alg->setProperty("Centre", twoInputs));
      TS_ASSERT_THROWS_NOTHING(
          alg->setPropertyValue("OutputWorkspace", outWSName));

      TS_ASSERT_EQUALS(alg->execute(), false);
    }

    { // check centre
      auto alg = algInstance();

      // centre must be 2 or 3 values (x,y) or (x,y,z)
      std::vector<double> justOne(1);
      justOne[0] = -0.35;
      TS_ASSERT_THROWS(alg->setProperty("Centre", justOne),
                       const std::invalid_argument &);

      std::vector<double> fourInputs(4, -0.45);
      TS_ASSERT_THROWS(alg->setProperty("Centre", fourInputs),
                       const std::invalid_argument &);

      TS_ASSERT_THROWS_NOTHING(
          alg->setPropertyValue("OutputWorkspace", outWSName));

      // change to a 2d workspace
      MatrixWorkspace_sptr goodWS = RingProfileTest::create_2d_workspace();
      TS_ASSERT_THROWS_NOTHING(alg->setProperty("InputWorkspace", goodWS));

      // centre must be inside the limits of the workspace
      std::vector<double> twoInputs(2, 0);
      // set the centre outside the matrix workspace
      twoInputs[0] = goodWS->x(0)[0] - 3.5;
      twoInputs[1] = goodWS->getAxis(1)->getMin() - 4.5;
      // it is a valid input because it has just two inputs
      TS_ASSERT_THROWS_NOTHING(alg->setProperty("Centre", twoInputs));

      TS_ASSERT_EQUALS(alg->execute(), false);
    }
  }

  void test_radiussum_center_of_numeric_image() {
    auto alg = algInstance();

    int numbins = 3;
    alg->setProperty("MaxRadius", 0.3);
    alg->setProperty("NumBins", numbins);
    alg->setProperty("Centre", std::vector<double>(2, 0));
    std::string outWSName("RadiusSumTest_OutputWS");
    alg->setPropertyValue("OutputWorkspace", outWSName);
    alg->setProperty("InputWorkspace", RingProfileTest::create_2d_workspace());

    TS_ASSERT_EQUALS(alg->execute(), true);

    MatrixWorkspace_sptr outws =
        RingProfileTest::basic_checkup_on_output_workspace((*alg), numbins);
    TS_ASSERT_DELTA(outws->y(0)[0], 0, 0.1);
    TS_ASSERT_DELTA(outws->y(0)[1], 1 + 2 + 3 + 4, 0.1);
    TS_ASSERT_DELTA(outws->y(0)[2], 4 + 1 + 1 + 2 + 2 + 3 + 3 + 4, 0.1);
  }

  void test_radiussum_center_of_numeric_image_normalized() {
    auto alg = algInstance();

    int numbins = 3;
    alg->setProperty("MaxRadius", 0.3);
    alg->setProperty("NumBins", numbins);
    alg->setProperty("Centre", std::vector<double>(2, 0));
    alg->setProperty("NormalizeByRadius", true);
    std::string outWSName("RadiusSumTest_OutputWS");
    alg->setPropertyValue("OutputWorkspace", outWSName);
    alg->setProperty("InputWorkspace", RingProfileTest::create_2d_workspace());

    TS_ASSERT_EQUALS(alg->execute(), true);

    MatrixWorkspace_sptr outws =
        RingProfileTest::basic_checkup_on_output_workspace((*alg), numbins);
    TS_ASSERT_DELTA(outws->y(0)[0], 0, 0.1);
    TS_ASSERT_DELTA(outws->y(0)[1], (1 + 2 + 3 + 4) / 0.15, 0.1);
    TS_ASSERT_DELTA(outws->y(0)[2], (4 + 1 + 1 + 2 + 2 + 3 + 3 + 4) / 0.25,
                    0.1);
  }

  void test_radiussum_horizontal_left_vertical_center_image() {
    auto alg = algInstance();

    int numbins = 5;
    alg->setProperty("MaxRadius", 0.6);
    alg->setProperty("NumBins", numbins);
    std::vector<double> left_center(2, 0);
    left_center[0] = -0.24;
    alg->setProperty("Centre", left_center);

    std::string outWSName("RadiusSumTest_OutputWS");
    alg->setPropertyValue("OutputWorkspace", outWSName);
    alg->setProperty("InputWorkspace", RingProfileTest::create_2d_workspace());

    TS_ASSERT_EQUALS(alg->execute(), true);

    MatrixWorkspace_sptr outws =
        RingProfileTest::basic_checkup_on_output_workspace((*alg), numbins);

    double output[] = {0, 8, 11, 6, 5};

    const auto &y = outws->y(0);
    for (size_t i = 0; i < 5; i++) {
      TS_ASSERT_DELTA(y[i], output[i], 0.1);
    }
  }

  void test_radiussum_center_of_instrument_image() {
    auto alg = algInstance();

    int numbins = 3;
    alg->setProperty("MaxRadius", 0.018);
    alg->setProperty("NumBins", numbins);
    std::vector<double> centre(3, 0.016);
    centre[2] = 0;
    alg->setProperty("Centre", centre);

    std::string outWSName("RadiusSumTest_OutputWS");
    alg->setPropertyValue("OutputWorkspace", outWSName);
    alg->setProperty(
        "InputWorkspace",
        RingProfileTest::create_rectangular_instrument_workspace());

    TS_ASSERT_EQUALS(alg->execute(), true);

    MatrixWorkspace_sptr outws =
        RingProfileTest::basic_checkup_on_output_workspace((*alg), numbins);

    TS_ASSERT_DELTA(outws->y(0)[0], 0, 0.1);
    TS_ASSERT_DELTA(outws->y(0)[1], 1 + 2 + 3 + 4, 0.1);
    TS_ASSERT_DELTA(outws->y(0)[2], 4 + 1 + 1 + 2 + 2 + 3 + 3 + 4, 0.1);
  }

  void test_radiussum_horizontal_left_vertical_center_instrument() {
    auto alg = algInstance();

    int numbins = 5;
    double maxradius = 0.041;
    alg->setProperty("MaxRadius", maxradius);
    alg->setProperty("NumBins", numbins);
    std::vector<double> left_center(3, 0.016);
    left_center[0] = 0;
    left_center[2] = 0;
    alg->setProperty("Centre", left_center);

    std::string outWSName("RadiusSumTest_OutputWS");
    alg->setPropertyValue("OutputWorkspace", outWSName);
    alg->setProperty(
        "InputWorkspace",
        RingProfileTest::create_rectangular_instrument_workspace());

    TS_ASSERT_EQUALS(alg->execute(), true);

    MatrixWorkspace_sptr outws =
        RingProfileTest::basic_checkup_on_output_workspace((*alg), numbins);

    double output[] = {1 + 2 + 2, 0, 11, 7, 7};

    const auto &x = outws->x(0);
    for (int i = 0; i < numbins + 1; i++)
      TS_ASSERT_DELTA(x[i], maxradius / numbins * i, 0.001);

    const auto &y = outws->y(0);
    for (int i = 0; i < numbins; i++)
      TS_ASSERT_DELTA(y[i], output[i], 0.1);

    // check the units of the instrument
    TS_ASSERT_EQUALS(outws->getAxis(0)->unit()->caption(), "Radius");
  }
};

#endif /* MANTID_ALGORITHMS_RADIUSSUMTEST_H_ */
