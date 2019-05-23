// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_ALGORITHMS_MAXENTCALCULATORTEST_H_
#define MANTID_ALGORITHMS_MAXENTCALCULATORTEST_H_

#include <cxxtest/TestSuite.h>
#include <gmock/gmock.h>

#include "MantidAlgorithms/MaxEnt/MaxentCalculator.h"
#include "MantidAlgorithms/MaxEnt/MaxentEntropy.h"
#include "MantidAlgorithms/MaxEnt/MaxentTransform.h"
#include "MantidKernel/WarningSuppressions.h"

using namespace Mantid::Algorithms;
using namespace testing;

class MockEntropy : public MaxentEntropy {
  GNU_DIAG_OFF_SUGGEST_OVERRIDE
public:
  MOCK_METHOD2(derivative,
               std::vector<double>(const std::vector<double> &, double));
  MOCK_METHOD2(secondDerivative,
               std::vector<double>(const std::vector<double> &, double));
  MOCK_METHOD2(correctValues,
               std::vector<double>(const std::vector<double> &, double));
};

class MockTransform : public MaxentTransform {

public:
  MOCK_METHOD1(imageToData, std::vector<double>(const std::vector<double> &));
  MOCK_METHOD1(dataToImage, std::vector<double>(const std::vector<double> &));
};
GNU_DIAG_ON_SUGGEST_OVERRIDE
using MockEntropy_sptr = boost::shared_ptr<MockEntropy>;

class MaxentCalculatorTest : public CxxTest::TestSuite {

public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static MaxentCalculatorTest *createSuite() {
    return new MaxentCalculatorTest();
  }
  static void destroySuite(MaxentCalculatorTest *suite) { delete suite; }

  void test_bad_input() {

    MockEntropy *entropy = new NiceMock<MockEntropy>();
    MockTransform *transform = new NiceMock<MockTransform>();
    MaxentCalculator calculator =
        MaxentCalculator(boost::shared_ptr<MockEntropy>(entropy),
                         boost::shared_ptr<MockTransform>(transform));

    std::vector<double> vec = {0, 1};
    std::vector<double> emptyVec = {};
    double bkg = 1;

    // Empty image
    TS_ASSERT_THROWS(
        calculator.iterate(vec, vec, emptyVec, bkg, emptyVec, emptyVec),
        const std::invalid_argument &);
    // Empty errors
    TS_ASSERT_THROWS(
        calculator.iterate(vec, emptyVec, vec, bkg, emptyVec, emptyVec),
        const std::invalid_argument &);
    // Empty data
    TS_ASSERT_THROWS(
        calculator.iterate(emptyVec, vec, vec, bkg, emptyVec, emptyVec),
        const std::invalid_argument &);

    // Bad background (should be positive)
    TS_ASSERT_THROWS(calculator.iterate(vec, vec, vec, 0, emptyVec, emptyVec),
                     const std::invalid_argument &);

    // Size mismatch between data and errors
    std::vector<double> vec2 = {0, 1, 1};
    TS_ASSERT_THROWS(
        calculator.iterate(vec, vec2, vec, bkg, emptyVec, emptyVec),
        const std::invalid_argument &);
  }

  void test_size_mismatch_data_image() {

    // Test the case where there is a mismatch in size between data space and
    // image space

    MockEntropy *entropy = new NiceMock<MockEntropy>();
    MockTransform *transform = new NiceMock<MockTransform>();
    MaxentCalculator calculator =
        MaxentCalculator(boost::shared_ptr<MaxentEntropy>(entropy),
                         boost::shared_ptr<MaxentTransform>(transform));

    // Input data
    // Vector in image space
    std::vector<double> vec1 = {0, 1};
    // Vector in data space
    std::vector<double> vec2 = {1, 1, 1};
    // Background
    double bkg = 1;
    // No adjustments
    std::vector<double> emptyVec = {};

    EXPECT_CALL(*entropy, correctValues(vec2, 1))
        .Times(1)
        .WillOnce(Return(vec1));
    EXPECT_CALL(*transform, imageToData(vec2)).Times(1).WillOnce(Return(vec2));
    EXPECT_CALL(*transform, dataToImage(vec2)).Times(0);
    EXPECT_CALL(*entropy, derivative(vec2, 1)).Times(0);
    EXPECT_CALL(*entropy, secondDerivative(vec2, bkg)).Times(0);
    TS_ASSERT_THROWS(
        calculator.iterate(vec1, vec1, vec2, bkg, emptyVec, emptyVec),
        const std::runtime_error &);

    Mock::VerifyAndClearExpectations(entropy);
    Mock::VerifyAndClearExpectations(transform);
  }

  void test_size_complex_data_real_image() {

    // As data and image spaces can be real/complex spaces independently, the
    // following situations are not considered size mismatches:
    // data.size() = 2 * image.size()
    // 2 * data.size() = image.size()

    MockEntropy *entropy = new NiceMock<MockEntropy>();
    MockTransform *transform = new NiceMock<MockTransform>();
    MaxentCalculator calculator =
        MaxentCalculator(boost::shared_ptr<MaxentEntropy>(entropy),
                         boost::shared_ptr<MaxentTransform>(transform));

    // Input data
    // Vector in data space
    std::vector<double> vec1 = {0, 1};
    // Vector in image space
    std::vector<double> vec2 = {1, 1, 1, 1};
    // Background
    double bkg = 1;
    // No adjustments
    std::vector<double> emptyVec = {};

    EXPECT_CALL(*entropy, correctValues(vec2, 1))
        .Times(1)
        .WillOnce(Return(vec2));
    EXPECT_CALL(*transform, imageToData(_))
        .Times(3)
        .WillRepeatedly(Return(vec1));
    EXPECT_CALL(*transform, dataToImage(_)).Times(1).WillOnce(Return(vec2));
    EXPECT_CALL(*entropy, derivative(vec2, 1)).Times(1).WillOnce(Return(vec2));
    EXPECT_CALL(*entropy, secondDerivative(vec2, bkg))
        .Times(1)
        .WillOnce(Return(vec2));
    TS_ASSERT_THROWS_NOTHING(
        calculator.iterate(vec1, vec1, vec2, bkg, emptyVec, emptyVec));

    Mock::VerifyAndClearExpectations(entropy);
    Mock::VerifyAndClearExpectations(transform);
  }

  void test_size_resolution_factor() {

    // As we may be applying a resolution factor != 1, the following is
    // not considered a size mismatch:
    // data.size() = N * image.size() (where N is an integer number)
    // However the opposite:
    // N * data.size() = image.size() (where N is an integer number)
    // is a size mismatch

    MockEntropy *entropy = new NiceMock<MockEntropy>();
    MockTransform *transform = new NiceMock<MockTransform>();
    MaxentCalculator calculator =
        MaxentCalculator(boost::shared_ptr<MaxentEntropy>(entropy),
                         boost::shared_ptr<MaxentTransform>(transform));

    // Input data
    // Vector in data space
    std::vector<double> vec1 = {0, 1};
    // Vector in image space
    std::vector<double> vec2 = {1, 1, 1, 1, 1, 1, 1, 1};
    // Background
    double bkg = 1;
    // No adjustments
    std::vector<double> emptyVec = {};

    EXPECT_CALL(*entropy, correctValues(vec2, 1))
        .Times(1)
        .WillOnce(Return(vec2));
    EXPECT_CALL(*transform, imageToData(_))
        .Times(3)
        .WillRepeatedly(Return(vec1));
    EXPECT_CALL(*transform, dataToImage(_)).Times(1).WillOnce(Return(vec2));
    EXPECT_CALL(*entropy, derivative(vec2, 1)).Times(1).WillOnce(Return(vec2));
    EXPECT_CALL(*entropy, secondDerivative(vec2, bkg))
        .Times(1)
        .WillOnce(Return(vec2));
    // This is OK: data.size() = N * image.size()
    TS_ASSERT_THROWS_NOTHING(
        calculator.iterate(vec1, vec1, vec2, bkg, emptyVec, emptyVec));

    EXPECT_CALL(*entropy, correctValues(vec1, 1))
        .Times(1)
        .WillOnce(Return(vec2));
    EXPECT_CALL(*transform, imageToData(_))
        .Times(1)
        .WillRepeatedly(Return(vec1));
    EXPECT_CALL(*transform, dataToImage(_)).Times(0);
    EXPECT_CALL(*entropy, derivative(_, _)).Times(0);
    EXPECT_CALL(*entropy, secondDerivative(_, _)).Times(0);
    // But this is not: N * data.size() = image.size()
    TS_ASSERT_THROWS(
        calculator.iterate(vec2, vec2, vec1, bkg, emptyVec, emptyVec),
        const std::runtime_error &);

    Mock::VerifyAndClearExpectations(entropy);
    Mock::VerifyAndClearExpectations(transform);
  }

  void test_data_not_loaded() {

    MockEntropy *entropy = new NiceMock<MockEntropy>();
    MockTransform *transform = new NiceMock<MockTransform>();
    MaxentCalculator calculator =
        MaxentCalculator(boost::shared_ptr<MockEntropy>(entropy),
                         boost::shared_ptr<MockTransform>(transform));

    // When data were not loaded public methods should throw an exception
    TS_ASSERT_THROWS(calculator.getReconstructedData(),
                     const std::runtime_error &);
    TS_ASSERT_THROWS(calculator.getImage(), const std::runtime_error &);
    TS_ASSERT_THROWS(calculator.getSearchDirections(),
                     const std::runtime_error &);
    TS_ASSERT_THROWS(calculator.getQuadraticCoefficients(),
                     const std::runtime_error &);
    TS_ASSERT_THROWS(calculator.getAngle(), const std::runtime_error &);
    TS_ASSERT_THROWS(calculator.getChisq(), const std::runtime_error &);
  }

  void test_chisq_angle() {

    MockEntropy *entropy = new NiceMock<MockEntropy>();
    MockTransform *transform = new NiceMock<MockTransform>();
    MaxentCalculator calculator =
        MaxentCalculator(boost::shared_ptr<MaxentEntropy>(entropy),
                         boost::shared_ptr<MaxentTransform>(transform));

    // Input data
    std::vector<double> dat = {1, 1};
    std::vector<double> err = {1, 1};
    std::vector<double> img = {0, 0, 0, 0};
    double bkg = 1.;
    // No adjustments
    std::vector<double> emptyVec = {};
    // Calc data
    std::vector<double> datC = {0, 0};

    EXPECT_CALL(*entropy, correctValues(img, bkg))
        .Times(1)
        .WillOnce(Return(img));
    EXPECT_CALL(*transform, imageToData(_))
        .Times(3)
        .WillRepeatedly(Return(datC));
    EXPECT_CALL(*transform, dataToImage(_)).Times(1).WillOnce(Return(img));
    EXPECT_CALL(*entropy, derivative(img, bkg)).Times(1).WillOnce(Return(img));
    EXPECT_CALL(*entropy, secondDerivative(img, bkg))
        .Times(1)
        .WillOnce(Return(img));

    TS_ASSERT_THROWS_NOTHING(
        calculator.iterate(dat, err, img, bkg, emptyVec, emptyVec));
    TS_ASSERT_DELTA(calculator.getChisq(), 1, 1e-8);
    TS_ASSERT_DELTA(calculator.getAngle(), 0.7071, 1e-4);

    Mock::VerifyAndClearExpectations(entropy);
    Mock::VerifyAndClearExpectations(transform);
  }

  void test_dirs_coefficients() {

    MockEntropy *entropy = new NiceMock<MockEntropy>();
    MockTransform *transform = new NiceMock<MockTransform>();
    MaxentCalculator calculator =
        MaxentCalculator(boost::shared_ptr<MaxentEntropy>(entropy),
                         boost::shared_ptr<MaxentTransform>(transform));

    // Input data
    std::vector<double> dat = {1, 1};
    std::vector<double> err = {1, 1};
    std::vector<double> img = {1, 1, 1, 1};
    double bkg = 1.;
    // No adjustments
    std::vector<double> emptyVec = {};
    // Calc data
    std::vector<double> datC = {0, 0};

    EXPECT_CALL(*entropy, correctValues(img, bkg))
        .Times(1)
        .WillOnce(Return(img));
    EXPECT_CALL(*transform, imageToData(_))
        .Times(3)
        .WillRepeatedly(Return(datC));
    EXPECT_CALL(*transform, dataToImage(_)).Times(1).WillOnce(Return(img));
    EXPECT_CALL(*entropy, derivative(img, bkg)).Times(1).WillOnce(Return(img));
    EXPECT_CALL(*entropy, secondDerivative(img, bkg))
        .Times(1)
        .WillOnce(Return(img));

    TS_ASSERT_THROWS_NOTHING(
        calculator.iterate(dat, err, img, bkg, emptyVec, emptyVec));

    auto dirs = calculator.getSearchDirections();
    TS_ASSERT_EQUALS(dirs[0].size(), 4);
    TS_ASSERT_EQUALS(dirs[1].size(), 4);
    for (size_t i = 0; i < 4; i++) {
      TS_ASSERT_DELTA(dirs[0][i], 0.5, 1E-6);
      TS_ASSERT_DELTA(dirs[1][i], 0.5, 1E-6);
    }

    auto coeff = calculator.getQuadraticCoefficients();
    // s1, c1
    TS_ASSERT_DELTA(coeff.s1[0][0], 2, 1E-6);
    TS_ASSERT_DELTA(coeff.s1[1][0], 2, 1E-6);
    TS_ASSERT_DELTA(coeff.c1[0][0], 1, 1E-6);
    TS_ASSERT_DELTA(coeff.c1[1][0], 1, 1E-6);
    // s2
    TS_ASSERT_DELTA(coeff.s2[0][0], -1, 1E-6);
    TS_ASSERT_DELTA(coeff.s2[1][0], -1, 1E-6);
    TS_ASSERT_DELTA(coeff.s2[0][1], -1, 1E-6);
    TS_ASSERT_DELTA(coeff.s2[1][1], -1, 1E-6);
    // c2
    TS_ASSERT_DELTA(coeff.c2[0][0], 0, 1E-6);
    TS_ASSERT_DELTA(coeff.c2[1][0], 0, 1E-6);
    TS_ASSERT_DELTA(coeff.c2[0][1], 0, 1E-6);
    TS_ASSERT_DELTA(coeff.c2[1][1], 0, 1E-6);

    Mock::VerifyAndClearExpectations(entropy);
    Mock::VerifyAndClearExpectations(transform);
  }
};

#endif /* MANTID_ALGORITHMS_MAXENTCALCULATORTEST_H_ */
