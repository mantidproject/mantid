#ifndef MANTID_ALGORITHMS_MAXENTTRANSFORMMULTIFOURIERTEST_H_
#define MANTID_ALGORITHMS_MAXENTTRANSFORMMULTIFOURIERTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAlgorithms/MaxEnt/MaxentSpaceComplex.h"
#include "MantidAlgorithms/MaxEnt/MaxentSpaceReal.h"
#include "MantidAlgorithms/MaxEnt/MaxentTransformMultiFourier.h"
#include <cmath>
#include <memory>

using Mantid::Algorithms::MaxentSpaceComplex;
using Mantid::Algorithms::MaxentSpaceReal;
using Mantid::Algorithms::MaxentTransformFourier;
using Mantid::Algorithms::MaxentTransformMultiFourier;

using MaxentSpace_sptr = std::shared_ptr<Mantid::Algorithms::MaxentSpace>;
using MaxentSpaceComplex_sptr =
    std::shared_ptr<Mantid::Algorithms::MaxentSpaceComplex>;
using MaxentSpaceReal_sptr =
    std::shared_ptr<Mantid::Algorithms::MaxentSpaceReal>;

class MaxentTransformMultiFourierTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static MaxentTransformMultiFourierTest *createSuite() {
    return new MaxentTransformMultiFourierTest();
  }
  static void destroySuite(MaxentTransformMultiFourierTest *suite) { delete suite; }

  void test_real_data_to_real_image_against_fourier() {

    MaxentSpace_sptr dataSpaceMF = std::make_shared<MaxentSpaceReal>();
    MaxentSpace_sptr imageSpaceMF = std::make_shared<MaxentSpaceReal>();
    MaxentTransformMultiFourier transformMF(dataSpaceMF, imageSpaceMF, 3);
    MaxentSpace_sptr dataSpaceF = std::make_shared<MaxentSpaceReal>();
    MaxentSpace_sptr imageSpaceF = std::make_shared<MaxentSpaceReal>();
    MaxentTransformFourier transformF(dataSpaceF, imageSpaceF);

    // Three square waves that add up to a saw tooth wave
    std::vector<double> realDataMF = {
      1, -1, 1, -1, 1, -1, 1, -1, 2, 2, -2, -2, 2, 2, -2, -2, 4, 4, 4, 4, -4, -4, -4, -4 };
    std::vector<double> realDataF = {
      7, 5, 3, 1, -1, -3, -5, -7 };

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

  void test_real_data_to_complex_image_against_fourier() {

    MaxentSpace_sptr dataSpaceMF = std::make_shared<MaxentSpaceReal>();
    MaxentSpace_sptr imageSpaceMF = std::make_shared<MaxentSpaceComplex>();
    MaxentTransformMultiFourier transformMF(dataSpaceMF, imageSpaceMF, 3);
    MaxentSpace_sptr dataSpaceF = std::make_shared<MaxentSpaceReal>();
    MaxentSpace_sptr imageSpaceF = std::make_shared<MaxentSpaceComplex>();
    MaxentTransformFourier transformF(dataSpaceF, imageSpaceF);

    // Three square waves that add up to a saw tooth wave
    std::vector<double> realDataMF = {
      1, -1, 1, -1, 1, -1, 1, -1, 2, 2, -2, -2, 2, 2, -2, -2, 4, 4, 4, 4, -4, -4, -4, -4 };
    std::vector<double> realDataF = {
      7, 5, 3, 1, -1, -3, -5, -7 };

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

  void test_complex_data_to_real_image_against_fourier() {

    MaxentSpace_sptr dataSpaceMF = std::make_shared<MaxentSpaceComplex>();
    MaxentSpace_sptr imageSpaceMF = std::make_shared<MaxentSpaceReal>();
    MaxentTransformMultiFourier transformMF(dataSpaceMF, imageSpaceMF, 3);
    MaxentSpace_sptr dataSpaceF = std::make_shared<MaxentSpaceComplex>();
    MaxentSpace_sptr imageSpaceF = std::make_shared<MaxentSpaceReal>();
    MaxentTransformFourier transformF(dataSpaceF, imageSpaceF);

    std::vector<double> realDataMF = {
      1, -1, 1, -1, 1, -1, 1, -1, 2, 2, -2, -2, 2, 2, -2, -2, 4, 4, 4, 4, -4, -4, -4, -4 };
    std::vector<double> realDataF = {
      7, 5, 3, 1, -1, -3, -5, -7 };

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

    MaxentSpace_sptr dataSpaceMF = std::make_shared<MaxentSpaceComplex>();
    MaxentSpace_sptr imageSpaceMF = std::make_shared<MaxentSpaceComplex>();
    MaxentTransformMultiFourier transformMF(dataSpaceMF, imageSpaceMF, 3);
    MaxentSpace_sptr dataSpaceF = std::make_shared<MaxentSpaceComplex>();
    MaxentSpace_sptr imageSpaceF = std::make_shared<MaxentSpaceComplex>();
    MaxentTransformFourier transformF(dataSpaceF, imageSpaceF);

    std::vector<double> realDataMF = {
      1, -1, 1, -1, 1, -1, 1, -1, 2, 2, -2, -2, 2, 2, -2, -2, 4, 4, 4, 4, -4, -4, -4, -4 };
    std::vector<double> realDataF = {
      7, 5, 3, 1, -1, -3, -5, -7 };

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

};

#endif /* MANTID_ALGORITHMS_MAXENTTRANSFORMMULTIFOURIERTEST_H_ */