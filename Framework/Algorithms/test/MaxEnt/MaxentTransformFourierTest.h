#ifndef MANTID_ALGORITHMS_MAXENTTRANSFORMFOURIERTEST_H_
#define MANTID_ALGORITHMS_MAXENTTRANSFORMFOURIERTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAlgorithms/MaxEnt/MaxentSpaceComplex.h"
#include "MantidAlgorithms/MaxEnt/MaxentSpaceReal.h"
#include "MantidAlgorithms/MaxEnt/MaxentTransformFourier.h"
#include <boost/make_shared.hpp>
#include <cmath>

using Mantid::Algorithms::MaxentSpaceComplex;
using Mantid::Algorithms::MaxentSpaceReal;
using Mantid::Algorithms::MaxentTransformFourier;
using Mantid::Algorithms::MaxentSpace_sptr;

class MaxentTransformFourierTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static MaxentTransformFourierTest *createSuite() {
    return new MaxentTransformFourierTest();
  }
  static void destroySuite(MaxentTransformFourierTest *suite) { delete suite; }

  void test_real_image_to_real_data() {

    MaxentSpace_sptr dataSpace = boost::make_shared<MaxentSpaceReal>();
    MaxentSpace_sptr imageSpace = boost::make_shared<MaxentSpaceReal>();
    MaxentTransformFourier transform(dataSpace, imageSpace);

    // cos (x)
    std::vector<double> realImage = {
        1,         0.951057,  0.809017,  0.587785,     0.309017, 6.12323e-17,
        -0.309017, -0.587785, -0.809017, -0.951057,    -1,       -0.951057,
        -0.809017, -0.587785, -0.309017, -1.83697e-16, 0.309017, 0.587785,
        0.809017,  0.951057};

    TS_ASSERT_THROWS_NOTHING(transform.imageToData(realImage));

    // Perform the transformation
    auto result = transform.imageToData(realImage);

    // Size
    TS_ASSERT_EQUALS(result.size(), realImage.size());
    // Values
    for (size_t i = 0; i < result.size(); i++) {
      if (i == 1 || i == 19) {
        TS_ASSERT_DELTA(result[i], 0.5, 1e-4);
      } else {
        TS_ASSERT(std::fabs(result[i]) < 1e-6);
      }
    }
  }

  void test_real_image_to_complex_data() {

    MaxentSpace_sptr dataSpace = boost::make_shared<MaxentSpaceComplex>();
    MaxentSpace_sptr imageSpace = boost::make_shared<MaxentSpaceReal>();
    MaxentTransformFourier transform(dataSpace, imageSpace);

    // cos (x)
    std::vector<double> realImage = {
        1,         0.951057,  0.809017,  0.587785,     0.309017, 6.12323e-17,
        -0.309017, -0.587785, -0.809017, -0.951057,    -1,       -0.951057,
        -0.809017, -0.587785, -0.309017, -1.83697e-16, 0.309017, 0.587785,
        0.809017,  0.951057};

    TS_ASSERT_THROWS_NOTHING(transform.imageToData(realImage));

    // Perform the transformation
    auto result = transform.imageToData(realImage);

    // Size
    TS_ASSERT_EQUALS(result.size(), realImage.size() * 2);
    // Values
    for (size_t i = 0; i < result.size(); i++) {
      if (i == 2 || i == 38) {
        TS_ASSERT_DELTA(result[i], 0.5, 1e-4);
      } else {
        TS_ASSERT(std::fabs(result[i]) < 1e-6);
      }
    }
  }

  void test_complex_image_to_real_data() {

    MaxentSpace_sptr dataSpace = boost::make_shared<MaxentSpaceReal>();
    MaxentSpace_sptr imageSpace = boost::make_shared<MaxentSpaceComplex>();
    MaxentTransformFourier transform(dataSpace, imageSpace);

    // cos (x) + i sin(x)
    std::vector<double> complexImage = {1.0,
                                        0.0,
                                        0.951056516295,
                                        0.309016994375,
                                        0.809016994375,
                                        0.587785252292,
                                        0.587785252292,
                                        0.809016994375,
                                        0.309016994375,
                                        0.951056516295,
                                        6.12323399574e-17,
                                        1.0,
                                        -0.309016994375,
                                        0.951056516295,
                                        -0.587785252292,
                                        0.809016994375,
                                        -0.809016994375,
                                        0.587785252292,
                                        -0.951056516295,
                                        0.309016994375,
                                        -1.0,
                                        1.22464679915e-16,
                                        -0.951056516295,
                                        -0.309016994375,
                                        -0.809016994375,
                                        -0.587785252292,
                                        -0.587785252292,
                                        -0.809016994375,
                                        -0.309016994375,
                                        -0.951056516295,
                                        -1.83697019872e-16,
                                        -1.0,
                                        0.309016994375,
                                        -0.951056516295,
                                        0.587785252292,
                                        -0.809016994375,
                                        0.809016994375,
                                        -0.587785252292,
                                        0.951056516295,
                                        -0.309016994375};

    TS_ASSERT_THROWS_NOTHING(transform.imageToData(complexImage));

    // Perform the transformation
    auto result = transform.imageToData(complexImage);

    // Size
    TS_ASSERT_EQUALS(result.size(), complexImage.size() / 2);
    // Values
    for (size_t i = 0; i < result.size(); i++) {
      if (i == 19) {
        TS_ASSERT_DELTA(result[i], 1.0, 1e-4);
      } else {
        TS_ASSERT(std::fabs(result[i]) < 1e-10);
      }
    }
  }

  void test_complex_image_to_complex_data() {

    MaxentSpace_sptr dataSpace = boost::make_shared<MaxentSpaceComplex>();
    MaxentSpace_sptr imageSpace = boost::make_shared<MaxentSpaceComplex>();
    MaxentTransformFourier transform(dataSpace, imageSpace);

    // sin (x) + i cos(x)
    std::vector<double> complexImage = {0.0,
                                        1.0,
                                        0.309016994375,
                                        0.951056516295,
                                        0.587785252292,
                                        0.809016994375,
                                        0.809016994375,
                                        0.587785252292,
                                        0.951056516295,
                                        0.309016994375,
                                        1.0,
                                        6.12323399574e-17,
                                        0.951056516295,
                                        -0.309016994375,
                                        0.809016994375,
                                        -0.587785252292,
                                        0.587785252292,
                                        -0.809016994375,
                                        0.309016994375,
                                        -0.951056516295,
                                        1.22464679915e-16,
                                        -1.0,
                                        -0.309016994375,
                                        -0.951056516295,
                                        -0.587785252292,
                                        -0.809016994375,
                                        -0.809016994375,
                                        -0.587785252292,
                                        -0.951056516295,
                                        -0.309016994375,
                                        -1.0,
                                        -1.83697019872e-16,
                                        -0.951056516295,
                                        0.309016994375,
                                        -0.809016994375,
                                        0.587785252292,
                                        -0.587785252292,
                                        0.809016994375,
                                        -0.309016994375,
                                        0.951056516295};

    TS_ASSERT_THROWS_NOTHING(transform.imageToData(complexImage));

    // Perform the transformation
    auto result = transform.imageToData(complexImage);

    // Size
    TS_ASSERT_EQUALS(result.size(), complexImage.size());
    // Values
    for (size_t i = 0; i < result.size(); i++) {
      if (i == 3) {
        TS_ASSERT_DELTA(result[i], 1.0, 1e-4);
      } else {
        TS_ASSERT(std::fabs(result[i]) < 1e-10);
      }
    }
  }

  void test_real_data_to_real_image() {

    MaxentSpace_sptr dataSpace = boost::make_shared<MaxentSpaceReal>();
    MaxentSpace_sptr imageSpace = boost::make_shared<MaxentSpaceReal>();
    MaxentTransformFourier transform(dataSpace, imageSpace);

    // cos(x)
    std::vector<double> realData = {
        1,         0.951057,  0.809017,  0.587785,     0.309017, 6.12323e-17,
        -0.309017, -0.587785, -0.809017, -0.951057,    -1,       -0.951057,
        -0.809017, -0.587785, -0.309017, -1.83697e-16, 0.309017, 0.587785,
        0.809017,  0.951057};

    TS_ASSERT_THROWS_NOTHING(transform.dataToImage(realData));

    // Perform the transformation
    auto result = transform.dataToImage(realData);

    // Size
    TS_ASSERT_EQUALS(result.size(), realData.size());
    // Values
    for (size_t i = 0; i < result.size(); i++) {
      if (i == 1 || i == 19) {
        TS_ASSERT_DELTA(result[i], 10, 1e-4);
      } else {
        TS_ASSERT(std::fabs(result[i]) < 1e-5);
      }
    }
  }

  void test_real_data_to_complex_image() {

    MaxentSpace_sptr dataSpace = boost::make_shared<MaxentSpaceReal>();
    MaxentSpace_sptr imageSpace = boost::make_shared<MaxentSpaceComplex>();
    MaxentTransformFourier transform(dataSpace, imageSpace);

    // cos (x)
    std::vector<double> realData = {
        1,         0.951057,  0.809017,  0.587785,     0.309017, 6.12323e-17,
        -0.309017, -0.587785, -0.809017, -0.951057,    -1,       -0.951057,
        -0.809017, -0.587785, -0.309017, -1.83697e-16, 0.309017, 0.587785,
        0.809017,  0.951057};

    TS_ASSERT_THROWS_NOTHING(transform.dataToImage(realData));

    // Perform the transformation
    auto result = transform.dataToImage(realData);

    // Size
    TS_ASSERT_EQUALS(result.size(), realData.size() * 2);
    // Values
    for (size_t i = 0; i < result.size(); i++) {
      if (i == 2 || i == 38) {
        TS_ASSERT_DELTA(result[i], 10, 1e-4);
      } else {
        TS_ASSERT(std::fabs(result[i]) < 1e-5);
      }
    }
  }

  void test_complex_data_to_real_image() {

    MaxentSpace_sptr dataSpace = boost::make_shared<MaxentSpaceComplex>();
    MaxentSpace_sptr imageSpace = boost::make_shared<MaxentSpaceReal>();
    MaxentTransformFourier transform(dataSpace, imageSpace);

    // cos (x) + i sin(x)
    std::vector<double> complexData = {1.0,
                                       0.0,
                                       0.951056516295,
                                       0.309016994375,
                                       0.809016994375,
                                       0.587785252292,
                                       0.587785252292,
                                       0.809016994375,
                                       0.309016994375,
                                       0.951056516295,
                                       6.12323399574e-17,
                                       1.0,
                                       -0.309016994375,
                                       0.951056516295,
                                       -0.587785252292,
                                       0.809016994375,
                                       -0.809016994375,
                                       0.587785252292,
                                       -0.951056516295,
                                       0.309016994375,
                                       -1.0,
                                       1.22464679915e-16,
                                       -0.951056516295,
                                       -0.309016994375,
                                       -0.809016994375,
                                       -0.587785252292,
                                       -0.587785252292,
                                       -0.809016994375,
                                       -0.309016994375,
                                       -0.951056516295,
                                       -1.83697019872e-16,
                                       -1.0,
                                       0.309016994375,
                                       -0.951056516295,
                                       0.587785252292,
                                       -0.809016994375,
                                       0.809016994375,
                                       -0.587785252292,
                                       0.951056516295,
                                       -0.309016994375};

    TS_ASSERT_THROWS_NOTHING(transform.dataToImage(complexData));

    // Perform the transformation
    auto result = transform.dataToImage(complexData);

    // Size
    TS_ASSERT_EQUALS(result.size(), complexData.size() / 2);
    // Values
    for (size_t i = 0; i < result.size(); i++) {
      if (i == 1) {
        TS_ASSERT_DELTA(result[i], 20.0, 1e-4);
      } else {
        TS_ASSERT(std::fabs(result[i]) < 1e-10);
      }
    }
  }

  void test_complex_data_to_complex_image() {
    MaxentSpace_sptr dataSpace = boost::make_shared<MaxentSpaceComplex>();
    MaxentSpace_sptr imageSpace = boost::make_shared<MaxentSpaceComplex>();
    MaxentTransformFourier transform(dataSpace, imageSpace);

    // sin (x) + i cos(x)
    std::vector<double> complexData = {0.0,
                                       1.0,
                                       0.309016994375,
                                       0.951056516295,
                                       0.587785252292,
                                       0.809016994375,
                                       0.809016994375,
                                       0.587785252292,
                                       0.951056516295,
                                       0.309016994375,
                                       1.0,
                                       6.12323399574e-17,
                                       0.951056516295,
                                       -0.309016994375,
                                       0.809016994375,
                                       -0.587785252292,
                                       0.587785252292,
                                       -0.809016994375,
                                       0.309016994375,
                                       -0.951056516295,
                                       1.22464679915e-16,
                                       -1.0,
                                       -0.309016994375,
                                       -0.951056516295,
                                       -0.587785252292,
                                       -0.809016994375,
                                       -0.809016994375,
                                       -0.587785252292,
                                       -0.951056516295,
                                       -0.309016994375,
                                       -1.0,
                                       -1.83697019872e-16,
                                       -0.951056516295,
                                       0.309016994375,
                                       -0.809016994375,
                                       0.587785252292,
                                       -0.587785252292,
                                       0.809016994375,
                                       -0.309016994375,
                                       0.951056516295};

    TS_ASSERT_THROWS_NOTHING(transform.dataToImage(complexData));

    // Perform the transformation
    auto result = transform.dataToImage(complexData);

    // Size
    TS_ASSERT_EQUALS(result.size(), complexData.size());
    // Values
    for (size_t i = 0; i < result.size(); i++) {
      if (i == 39) {
        TS_ASSERT_DELTA(result[i], 20.0, 1e-4);
      } else {
        TS_ASSERT(std::fabs(result[i]) < 1e-10);
      }
    }
  }

  void test_forward_backward() {

    MaxentSpace_sptr dataSpace = boost::make_shared<MaxentSpaceComplex>();
    MaxentSpace_sptr imageSpace = boost::make_shared<MaxentSpaceComplex>();
    MaxentTransformFourier transform(dataSpace, imageSpace);

    // sin (x) + i cos(x)
    std::vector<double> complexImage = {0.0,
                                        1.0,
                                        0.309016994375,
                                        0.951056516295,
                                        0.587785252292,
                                        0.809016994375,
                                        0.809016994375,
                                        0.587785252292,
                                        0.951056516295,
                                        0.309016994375,
                                        1.0,
                                        6.12323399574e-17,
                                        0.951056516295,
                                        -0.309016994375,
                                        0.809016994375,
                                        -0.587785252292,
                                        0.587785252292,
                                        -0.809016994375,
                                        0.309016994375,
                                        -0.951056516295,
                                        1.22464679915e-16,
                                        -1.0,
                                        -0.309016994375,
                                        -0.951056516295,
                                        -0.587785252292,
                                        -0.809016994375,
                                        -0.809016994375,
                                        -0.587785252292,
                                        -0.951056516295,
                                        -0.309016994375,
                                        -1.0,
                                        -1.83697019872e-16,
                                        -0.951056516295,
                                        0.309016994375,
                                        -0.809016994375,
                                        0.587785252292,
                                        -0.587785252292,
                                        0.809016994375,
                                        -0.309016994375,
                                        0.951056516295};

    auto complexData = transform.imageToData(complexImage);
    auto newImage = transform.dataToImage(complexData);

    TS_ASSERT_EQUALS(complexData.size(), complexImage.size());
    TS_ASSERT_EQUALS(newImage.size(), complexImage.size());
    for (size_t i = 0; i < complexImage.size(); i++) {
      TS_ASSERT_DELTA(newImage[i], complexImage[i], 1e-3);
    }
  }
};

#endif /* MANTID_ALGORITHMS_MAXENTTRANSFORMFOURIERTEST_H_ */