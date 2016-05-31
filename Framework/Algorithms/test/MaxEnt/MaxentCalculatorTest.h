#ifndef MANTID_ALGORITHMS_MAXENTDATATEST_H_
#define MANTID_ALGORITHMS_MAXENTDATATEST_H_

#include <cxxtest/TestSuite.h>
#include <gmock/gmock.h>

#include "MantidAlgorithms/MaxEnt/MaxentData.h"
#include "MantidAlgorithms/MaxEnt/MaxentEntropy.h"

#include <boost/make_shared.hpp>
#include <boost/shared_ptr.hpp>

using namespace Mantid::Algorithms;
using namespace testing;

class MockEntropy : public MaxentEntropy {

public:
  MOCK_METHOD1(getDerivative, double(double));
  MOCK_METHOD1(getSecondDerivative, double(double));
  MOCK_METHOD2(correctValue, double(double, double));
};
typedef boost::shared_ptr<MockEntropy> MockEntropy_sptr;

class MaxentDataTest : public CxxTest::TestSuite {

public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static MaxentDataTest *createSuite() { return new MaxentDataTest(); }
  static void destroySuite(MaxentDataTest *suite) { delete suite; }

  void test_bad_real_input() {

    MockEntropy *entropy = new NiceMock<MockEntropy>();
    MaxentData_sptr maxentData =
        boost::make_shared<MaxentData>(boost::shared_ptr<MockEntropy>(entropy));

    // Bad image size (should be at least 2*data.size())
    std::vector<double> dat = {0, 1};
    std::vector<double> err = {1, 1};
    std::vector<double> img = {0, 0};
    double bkg = 1;
    TS_ASSERT_THROWS(maxentData->loadReal(dat, err, img, bkg),
                     std::runtime_error);

    // Bad errors size (should be data.size())
    err = std::vector<double>{1};
    img = std::vector<double>{0, 0, 0, 0};
    TS_ASSERT_THROWS(maxentData->loadReal(dat, err, img, bkg),
                     std::runtime_error);

    // Bad background (should be positive)
    err = std::vector<double>{1, 1};
    bkg = 0;
    TS_ASSERT_THROWS(maxentData->loadReal(dat, err, img, bkg),
                     std::runtime_error);
  }
  void test_bad_complex_input() {

    MockEntropy *entropy = new NiceMock<MockEntropy>();
    MaxentData_sptr maxentData =
        boost::make_shared<MaxentData>(boost::shared_ptr<MockEntropy>(entropy));

    // Size mismatch between real and imaginary components
    std::vector<double> datRe = {0, 1};
    std::vector<double> datIm = {0};
    std::vector<double> err = {1, 1};
    std::vector<double> img = {0, 0, 0, 0};
    double bkg = 1;
    TS_ASSERT_THROWS(maxentData->loadComplex(datRe, datIm, err, err, img, bkg),
                     std::runtime_error);

    // Wrong image size
    img = std::vector<double>{0, 0};
    TS_ASSERT_THROWS(maxentData->loadComplex(datRe, datRe, err, err, img, bkg),
                     std::runtime_error);

    // Bad background (should be positive)
    img = std::vector<double>{0, 0, 0, 0};
    bkg = 0;
    TS_ASSERT_THROWS(maxentData->loadComplex(datRe, datIm, err, err, img, bkg),
                     std::runtime_error);
  }
  void test_data_not_loaded() {

    MockEntropy *entropy = new NiceMock<MockEntropy>();
    MaxentData_sptr maxentData =
        boost::make_shared<MaxentData>(boost::shared_ptr<MockEntropy>(entropy));

    // When data were not loaded public methods should throw an exception
    TS_ASSERT_THROWS(maxentData->updateImage(std::vector<double>{0, 1}),
                     std::runtime_error);
    TS_ASSERT_THROWS(maxentData->getReconstructedData(), std::runtime_error);
    TS_ASSERT_THROWS(maxentData->getImage(), std::runtime_error);
    TS_ASSERT_THROWS(maxentData->getQuadraticCoefficients(),
                     std::runtime_error);
    TS_ASSERT_THROWS(maxentData->getAngle(), std::runtime_error);
    TS_ASSERT_THROWS(maxentData->getChisq(), std::runtime_error);
    TS_ASSERT_THROWS(maxentData->calculateQuadraticCoefficients(),
                     std::runtime_error);
  }

  void test_update_image() {
    // Test the public method updateImage()

    MockEntropy *entropy = new NiceMock<MockEntropy>();
    MaxentData_sptr maxentData =
        boost::make_shared<MaxentData>(boost::shared_ptr<MockEntropy>(entropy));

    std::vector<double> dat = {0, 0};
    std::vector<double> img = {0, 0, 0, 0};
    maxentData->loadComplex(dat, dat, dat, dat, img, 0.1);

    // Trying to update without calculating search directions first
    TS_ASSERT_THROWS(maxentData->updateImage(std::vector<double>{0, 0}),
                     std::runtime_error);

    // Calculate search directions
    maxentData->calculateQuadraticCoefficients();

    // Trying to update using a vector with wrong size
    TS_ASSERT_THROWS(maxentData->updateImage(std::vector<double>{0, 0, 0}),
                     std::invalid_argument);
  }

  void test_chisq_data() {

    MockEntropy *entropy = new NiceMock<MockEntropy>();
    MaxentData_sptr maxentData =
        boost::make_shared<MaxentData>(boost::shared_ptr<MockEntropy>(entropy));

    std::vector<double> dat = {1, 1};
    std::vector<double> err = {1, 1};
    std::vector<double> img = {0, 0, 0, 0};
    double bkg = 1.;

    EXPECT_CALL(*entropy, correctValue(0, bkg)).Times(4);
    TS_ASSERT_THROWS_NOTHING(maxentData->loadReal(dat, err, img, bkg));

    TS_ASSERT_EQUALS(maxentData->getChisq(), 2);
  }

  void test_quadratic_coefficients() {

    MockEntropy *entropy = new NiceMock<MockEntropy>();
    MaxentData_sptr maxentData =
        boost::make_shared<MaxentData>(boost::shared_ptr<MockEntropy>(entropy));

    std::vector<double> dat = {0.5, 0.5};
    std::vector<double> err = {0.1, 0.1};
    std::vector<double> img = {1, 1, 1, 1};
    double bkg = 0.1;

    EXPECT_CALL(*entropy, correctValue(1., bkg))
        .Times(4)
        .WillRepeatedly(Return(1.));
    TS_ASSERT_THROWS_NOTHING(maxentData->loadReal(dat, err, img, bkg));

    EXPECT_CALL(*entropy, getDerivative(1. / 0.1))
        .Times(4)
        .WillRepeatedly(Return(1.));
    EXPECT_CALL(*entropy, getSecondDerivative(1.))
        .Times(4)
        .WillRepeatedly(Return(1.));
    TS_ASSERT_THROWS_NOTHING(maxentData->calculateQuadraticCoefficients());

    auto coeff = maxentData->getQuadraticCoefficients();
    double angle = maxentData->getAngle();
    double chisq = maxentData->getChisq();

    TS_ASSERT_DELTA(angle, 0.5, 1E-6);
    TS_ASSERT_DELTA(chisq, 50, 1E-6);
    // s1, c1
    TS_ASSERT_DELTA(coeff.s1[0][0], 1, 1E-6);
    TS_ASSERT_DELTA(coeff.s1[1][0], 2, 1E-6);
    TS_ASSERT_DELTA(coeff.c1[0][0], 4, 1E-6);
    TS_ASSERT_DELTA(coeff.c1[1][0], 2, 1E-6);
    // s2
    TS_ASSERT_DELTA(coeff.s2[0][0], -10, 1E-6);
    TS_ASSERT_DELTA(coeff.s2[1][0], -5, 1E-6);
    TS_ASSERT_DELTA(coeff.s2[0][1], -5, 1E-6);
    TS_ASSERT_DELTA(coeff.s2[1][1], -10, 1E-6);
    // c2
    TS_ASSERT_DELTA(coeff.c2[0][0], 2, 1E-6);
    TS_ASSERT_DELTA(coeff.c2[1][0], 1, 1E-6);
    TS_ASSERT_DELTA(coeff.c2[0][1], 1, 1E-6);
    TS_ASSERT_DELTA(coeff.c2[1][1], 1, 1E-6);
  }

  void test_update_image_updates_dataCalc() {

    MockEntropy *entropy = new NiceMock<MockEntropy>();
    MaxentData_sptr maxentData =
        boost::make_shared<MaxentData>(boost::shared_ptr<MockEntropy>(entropy));

    std::vector<double> dat = {0.5, 0.5};
    std::vector<double> err = {0.1, 0.1};
    std::vector<double> img = {1, 1, 1, 1};
    double bkg = 0.1;

    EXPECT_CALL(*entropy, correctValue(1., bkg))
        .Times(4)
        .WillRepeatedly(Return(1.));
    TS_ASSERT_THROWS_NOTHING(maxentData->loadReal(dat, err, img, bkg));

    // Get the calculated data
    auto dataCalc = maxentData->getReconstructedData();

    // Calculate quad coeffs and search directions
    EXPECT_CALL(*entropy, getDerivative(1. / 0.1))
        .Times(4)
        .WillRepeatedly(Return(1.));
    EXPECT_CALL(*entropy, getSecondDerivative(1.))
        .Times(4)
        .WillRepeatedly(Return(1.));
    TS_ASSERT_THROWS_NOTHING(maxentData->calculateQuadraticCoefficients());

    // Update the image
    EXPECT_CALL(*entropy, correctValue(_, _))
        .Times(4)
        .WillRepeatedly(Return(0.5));
    TS_ASSERT_THROWS_NOTHING(
        maxentData->updateImage(std::vector<double>{1, 1}));

    // Get the calculated data
    auto newDataCalc = maxentData->getReconstructedData();
    // Should have been updated
    TS_ASSERT_DIFFERS(dataCalc, newDataCalc);
  }

  void test_extended_image() {
    // The input image may be F times the size of the
    // experimental data and errors

    MockEntropy *entropy = new NiceMock<MockEntropy>();
    MaxentData_sptr maxentData =
        boost::make_shared<MaxentData>(boost::shared_ptr<MockEntropy>(entropy));

    // Real case

    // Bad image size (should be F*2*dat.size() with F an integer number)
    std::vector<double> dat = {0, 1};
    std::vector<double> err = {1, 1};
    std::vector<double> img = {0, 0, 0, 0, 0};
    double bkg = 1;
    TS_ASSERT_THROWS(maxentData->loadReal(dat, err, img, bkg),
                     std::runtime_error);

    // OK, image is 2 * (2 * dat.size())
    img = std::vector<double>{0, 0, 0, 0, 0, 0, 0, 0};
    TS_ASSERT_THROWS_NOTHING(maxentData->loadReal(dat, err, img, bkg));

    // Complex case

    // Bad image size (should be F*2*dat.size() with F an integer number)
    img = std::vector<double>{0, 0, 0, 0, 0};
    TS_ASSERT_THROWS(maxentData->loadComplex(dat, dat, err, err, img, bkg),
                     std::runtime_error);

    // OK, image is 3 * (2 * dat.size())
    img = std::vector<double>{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

    TS_ASSERT_THROWS_NOTHING(
        maxentData->loadComplex(dat, dat, err, err, img, bkg));
  }
};

#endif /* MANTID_ALGORITHMS_MAXENTDATATEST_H_ */