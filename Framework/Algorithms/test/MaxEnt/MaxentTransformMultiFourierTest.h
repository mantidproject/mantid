// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_ALGORITHMS_MAXENTTRANSFORMMULTIFOURIERTEST_H_
#define MANTID_ALGORITHMS_MAXENTTRANSFORMMULTIFOURIERTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAlgorithms/MaxEnt/MaxentSpaceComplex.h"
#include "MantidAlgorithms/MaxEnt/MaxentSpaceReal.h"
#include "MantidAlgorithms/MaxEnt/MaxentTransformMultiFourier.h"
#include <boost/make_shared.hpp>
#include <boost/shared_ptr.hpp>
#include <cmath>

using Mantid::Algorithms::MaxentSpaceComplex;
using Mantid::Algorithms::MaxentSpaceComplex_sptr;
using Mantid::Algorithms::MaxentSpaceReal;
using Mantid::Algorithms::MaxentSpace_sptr;
using Mantid::Algorithms::MaxentTransformFourier;
using Mantid::Algorithms::MaxentTransformMultiFourier;

class MaxentTransformMultiFourierTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static MaxentTransformMultiFourierTest *createSuite() {
    return new MaxentTransformMultiFourierTest();
  }
  static void destroySuite(MaxentTransformMultiFourierTest *suite) {
    delete suite;
  }

  void test_complex_data_to_real_image_against_fourier() {

    MaxentSpaceComplex_sptr dataSpaceMF =
        boost::make_shared<MaxentSpaceComplex>();
    MaxentSpace_sptr imageSpaceMF = boost::make_shared<MaxentSpaceReal>();
    MaxentTransformMultiFourier transformMF(dataSpaceMF, imageSpaceMF, 3);
    MaxentSpace_sptr dataSpaceF = boost::make_shared<MaxentSpaceComplex>();
    MaxentSpace_sptr imageSpaceF = boost::make_shared<MaxentSpaceReal>();
    MaxentTransformFourier transformF(dataSpaceF, imageSpaceF);

    // Three square waves that add up to a saw tooth wave
    std::vector<double> realDataMF = {1, -1, 1,  -1, 1,  -1, 1,  -1,
                                      2, 2,  -2, -2, 2,  2,  -2, -2,
                                      4, 4,  4,  4,  -4, -4, -4, -4};
    std::vector<double> realDataF = {7, 5, 3, 1, -1, -3, -5, -7};

    // Perform the transformation
    auto result = transformMF.dataToImage(realDataMF);
    // Perform Fourier transformation for comparison
    auto resultF = transformF.dataToImage(realDataF);

    // Check both results are equal
    TS_ASSERT_EQUALS(result.size(), resultF.size());
    if (result.size() == resultF.size()) {
      for (size_t i = 0; i < result.size(); i++) {
        TS_ASSERT_DELTA(result[i], resultF[i], 1e-4);
      }
    }
  }

  void test_complex_data_to_complex_image_against_fourier() {

    MaxentSpaceComplex_sptr dataSpaceMF =
        boost::make_shared<MaxentSpaceComplex>();
    MaxentSpace_sptr imageSpaceMF = boost::make_shared<MaxentSpaceComplex>();
    MaxentTransformMultiFourier transformMF(dataSpaceMF, imageSpaceMF, 3);
    MaxentSpace_sptr dataSpaceF = boost::make_shared<MaxentSpaceComplex>();
    MaxentSpace_sptr imageSpaceF = boost::make_shared<MaxentSpaceComplex>();
    MaxentTransformFourier transformF(dataSpaceF, imageSpaceF);

    std::vector<double> realDataMF = {1, -1, 1,  -1, 1,  -1, 1,  -1,
                                      2, 2,  -2, -2, 2,  2,  -2, -2,
                                      4, 4,  4,  4,  -4, -4, -4, -4};
    std::vector<double> realDataF = {7, 5, 3, 1, -1, -3, -5, -7};

    // Perform the transformation
    auto result = transformMF.dataToImage(realDataMF);
    // Perform Fourier transformation for comparison
    auto resultF = transformF.dataToImage(realDataF);

    // Check both results are equal
    TS_ASSERT_EQUALS(result.size(), resultF.size());
    if (result.size() == resultF.size()) {
      for (size_t i = 0; i < result.size(); i++) {
        TS_ASSERT_DELTA(result[i], resultF[i], 1e-4);
      }
    }
  }

  void test_image_to_data_repeats_fourier() {
    MaxentSpaceComplex_sptr dataSpaceMF =
        boost::make_shared<MaxentSpaceComplex>();
    MaxentSpace_sptr imageSpaceMF = boost::make_shared<MaxentSpaceComplex>();
    MaxentTransformMultiFourier transformMF(dataSpaceMF, imageSpaceMF, 3);
    MaxentSpace_sptr dataSpaceF = boost::make_shared<MaxentSpaceComplex>();
    MaxentSpace_sptr imageSpaceF = boost::make_shared<MaxentSpaceComplex>();
    MaxentTransformFourier transformF(dataSpaceF, imageSpaceF);

    std::vector<double> complexImage = {4.0, 3.0, 2.0, 1.0, 0.0, 0.0, 0.0, 0.0};

    // Perform the transformation
    auto result = transformMF.imageToData(complexImage);
    // Perform Fourier transformation for comparison
    auto resultF = transformF.imageToData(complexImage);

    // Check that result is a triple repetition of fourier
    TS_ASSERT_EQUALS(result.size(), 3 * resultF.size());
    if (result.size() == 3 * resultF.size()) {
      size_t n = resultF.size();
      for (size_t i = 0; i < n; i++) {
        TS_ASSERT_DELTA(result[i], resultF[i], 1e-4);
        TS_ASSERT_DELTA(result[i + n], resultF[i], 1e-4);
        TS_ASSERT_DELTA(result[i + 2 * n], resultF[i], 1e-4);
      }
    }
  }

  void test_image_to_data_with_real_adjustments() {
    MaxentSpaceComplex_sptr dataSpaceMF =
        boost::make_shared<MaxentSpaceComplex>();
    MaxentSpace_sptr imageSpaceMF = boost::make_shared<MaxentSpaceComplex>();
    MaxentTransformMultiFourier transformMF(dataSpaceMF, imageSpaceMF, 3);
    MaxentSpace_sptr dataSpaceF = boost::make_shared<MaxentSpaceComplex>();
    MaxentSpace_sptr imageSpaceF = boost::make_shared<MaxentSpaceComplex>();
    MaxentTransformFourier transformF(dataSpaceF, imageSpaceF);

    std::vector<double> complexImage = {4.0, 3.0, 2.0, 1.0, 0.0, 0.0, 0.0, 0.0};
    std::vector<double> linearAdjustments = {
        1.0, 0.0, 2.0, 0.0, 3.0, 0.0, 4.0, 0.0, 0.0, 0.0, 0.0, 0.0,
        0.0, 0.0, 0.0, 0.0, 1.0, 0.0, 2.0, 0.0, 3.0, 0.0, 4.0, 0.0};
    std::vector<double> constAdjustments = {
        0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0,  0.0, 1.0, 0.0, 2.0,  0.0,
        3.0, 0.0, 4.0, 0.0, 1.0, 0.0, -1.0, 0.0, 1.0, 0.0, -1.0, 0.0};

    // Set Adjustments
    transformMF.setAdjustments(linearAdjustments, constAdjustments);

    // Perform the transformation
    auto result = transformMF.imageToData(complexImage);
    // Perform Fourier transformation for comparison
    auto resultF = transformF.imageToData(complexImage);

    // Check that result is a triple repetition of fourier
    // but with the adjustments applied
    TS_ASSERT_EQUALS(result.size(), 3 * resultF.size());
    TS_ASSERT_EQUALS(result.size(), linearAdjustments.size());
    TS_ASSERT_EQUALS(result.size(), constAdjustments.size());

    if (result.size() == 3 * resultF.size()) {
      size_t n = resultF.size();
      for (size_t i = 0; i < n; i++) {
        TS_ASSERT_DELTA(result[i],
                        linearAdjustments[i - i % 2] * resultF[i] +
                            constAdjustments[i],
                        1e-4);
        TS_ASSERT_DELTA(result[i + n],
                        linearAdjustments[i + n - i % 2] * resultF[i] +
                            constAdjustments[i + n],
                        1e-4);
        TS_ASSERT_DELTA(result[i + 2 * n],
                        linearAdjustments[i + 2 * n - i % 2] * resultF[i] +
                            constAdjustments[i + 2 * n],
                        1e-4);
      }
    }
  }

  void test_image_to_data_with_imaginary_adjustments() {
    MaxentSpaceComplex_sptr dataSpaceMF =
        boost::make_shared<MaxentSpaceComplex>();
    MaxentSpace_sptr imageSpaceMF = boost::make_shared<MaxentSpaceComplex>();
    MaxentTransformMultiFourier transformMF(dataSpaceMF, imageSpaceMF, 3);
    MaxentSpace_sptr dataSpaceF = boost::make_shared<MaxentSpaceComplex>();
    MaxentSpace_sptr imageSpaceF = boost::make_shared<MaxentSpaceComplex>();
    MaxentTransformFourier transformF(dataSpaceF, imageSpaceF);

    std::vector<double> complexImage = {4.0, 3.0, 2.0, 1.0, 0.0, 0.0, 0.0, 0.0};
    std::vector<double> linearAdjustments = {
        0.0, 1.0, 0.0, 2.0, 0.0, 3.0, 0.0, 4.0, 0.0, 0.0, 0.0, 0.0,
        0.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0, 2.0, 0.0, 3.0, 0.0, 4.0};
    std::vector<double> constAdjustments = {
        0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0,  0.0, 1.0, 0.0, 2.0,
        0.0, 3.0, 0.0, 4.0, 0.0, 1.0, 0.0, -1.0, 0.0, 1.0, 0.0, -1.0};

    // Set Adjustments
    transformMF.setAdjustments(linearAdjustments, constAdjustments);

    // Perform the transformation
    auto result = transformMF.imageToData(complexImage);
    // Perform Fourier transformation for comparison
    auto resultF = transformF.imageToData(complexImage);

    // Check that result is a triple repetition of fourier
    // but with the adjustments applied
    TS_ASSERT_EQUALS(result.size(), 3 * resultF.size());
    TS_ASSERT_EQUALS(result.size(), linearAdjustments.size());
    TS_ASSERT_EQUALS(result.size(), constAdjustments.size());

    if (result.size() == 3 * resultF.size()) {
      size_t n = resultF.size();
      for (size_t i = 0; i < n; i++) {
        if (i % 2 == 0) { // Real part
          TS_ASSERT_DELTA(result[i],
                          -linearAdjustments[i + 1] * resultF[i + 1] +
                              constAdjustments[i],
                          1e-4);
          TS_ASSERT_DELTA(result[i + n],
                          -linearAdjustments[i + n + 1] * resultF[i + 1] +
                              constAdjustments[i + n],
                          1e-4);
          TS_ASSERT_DELTA(result[i + 2 * n],
                          -linearAdjustments[i + 2 * n + 1] * resultF[i + 1] +
                              constAdjustments[i + 2 * n],
                          1e-4);
        } else { // Imaginary part
          TS_ASSERT_DELTA(result[i],
                          linearAdjustments[i] * resultF[i - 1] +
                              constAdjustments[i],
                          1e-4);
          TS_ASSERT_DELTA(result[i + n],
                          linearAdjustments[i + n] * resultF[i - 1] +
                              constAdjustments[i + n],
                          1e-4);
          TS_ASSERT_DELTA(result[i + 2 * n],
                          linearAdjustments[i + 2 * n] * resultF[i - 1] +
                              constAdjustments[i + 2 * n],
                          1e-4);
        }
      }
    }
  }
};

#endif /* MANTID_ALGORITHMS_MAXENTTRANSFORMMULTIFOURIERTEST_H_ */