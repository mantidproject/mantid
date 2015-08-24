#ifndef MANTID_ALGORITHMS_CONVOLUTIONFITSEQUENTIALTEST_H_
#define MANTID_ALGORITHMS_CONVOLUTIONFITSEQUENTIALTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAlgorithms/ConvolutionFitSequential.h"

#include "MantidDataObjects/Workspace2D.h"

#include "MantidTestHelpers/WorkspaceCreationHelper.h"

using Mantid::Algorithms::ConvolutionFitSequential;
using namespace Mantid::API;
using namespace Mantid::DataObjects;

class ConvolutionFitSequentialTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static ConvolutionFitSequentialTest *createSuite() {
    return new ConvolutionFitSequentialTest();
  }
  static void destroySuite(ConvolutionFitSequentialTest *suite) {
    delete suite;
  }

  void test_fit_function_is_valid_for_convolution_fitting() {
    Mantid::Algorithms::ConvolutionFitSequential alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    TS_ASSERT_THROWS_NOTHING(alg.setProperty(
        "Function", "function=test,name=Convolution,name=Resolution"));
  }

  //-------------------------- Failure cases ----------------------------
  void test_empty_function_is_not_allowed() {
    Mantid::Algorithms::ConvolutionFitSequential alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize());

    TS_ASSERT_THROWS(alg.setPropertyValue("Function", ""),
                     std::invalid_argument);
  }

  void test_empty_startX_is_not_allowed() {
    Mantid::Algorithms::ConvolutionFitSequential alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize());

    TS_ASSERT_THROWS(alg.setPropertyValue("Start X", ""),
                     std::invalid_argument);
  }

  void test_empty_endX_is_not_allowed() {
    Mantid::Algorithms::ConvolutionFitSequential alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize());

    TS_ASSERT_THROWS(alg.setPropertyValue("End X", ""), std::invalid_argument);
  }

  void test_empty_specMin_is_not_allowed() {
    Mantid::Algorithms::ConvolutionFitSequential alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize());

    TS_ASSERT_THROWS(alg.setPropertyValue("Spec Min", ""),
                     std::invalid_argument);
  }

  void test_empty_specMax_is_not_allowed() {
    Mantid::Algorithms::ConvolutionFitSequential alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize());

    TS_ASSERT_THROWS(alg.setPropertyValue("Spec Max", ""),
                     std::invalid_argument);
  }

  void test_empty_maxIterations_is_not_allowed() {
    Mantid::Algorithms::ConvolutionFitSequential alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize());

    TS_ASSERT_THROWS(alg.setPropertyValue("Max Iterations", ""),
                     std::invalid_argument);
  }

  void test_empty_temperature_is_not_allowed() {
    Mantid::Algorithms::ConvolutionFitSequential alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize());

    TS_ASSERT_THROWS(alg.setPropertyValue("Temperature", ""),
                     std::invalid_argument);
  }

  void test_spectra_min_or_max_number_can_not_be_negative() {
    Mantid::Algorithms::ConvolutionFitSequential alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    TS_ASSERT_THROWS(alg.setPropertyValue("Spec Min", "-1"),
                     std::invalid_argument);
    TS_ASSERT_THROWS(alg.setPropertyValue("Spec Max", "-1"),
                     std::invalid_argument);
  }

  void test_max_iterations_can_not_be_a_negative_number() {
    Mantid::Algorithms::ConvolutionFitSequential alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    TS_ASSERT_THROWS(alg.setPropertyValue("Max Iterations", "-1"),
                     std::invalid_argument);
  }

  void test_fit_function_that_does_not_contain_resolution_is_not_allowed() {
    Mantid::Algorithms::ConvolutionFitSequential alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    TS_ASSERT_THROWS(
        alg.setProperty("Function", "function=test,name=Convolution"),
        std::invalid_argument);
  }

  void test_fit_function_that_does_not_contain_convolution_is_not_allowed() {
    Mantid::Algorithms::ConvolutionFitSequential alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    TS_ASSERT_THROWS(
        alg.setProperty("Function", "function=test,name=Resolution"),
        std::invalid_argument);
  }

  //------------------------- Execution cases ---------------------------
  void test_exec() {
    Mantid::Algorithms::ConvolutionFitSequential alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    auto resWs = Create2DWorkspace(5, 1);
  }

  //------------------------ Private Functions---------------------------

  Workspace2D_sptr Create2DWorkspace(int xlen, int ylen) {
    boost::shared_ptr<Mantid::MantidVec> x1(new Mantid::MantidVec(xlen, 0.0));
    boost::shared_ptr<Mantid::MantidVec> y1(
        new Mantid::MantidVec(xlen - 1, 3.0));
    boost::shared_ptr<Mantid::MantidVec> e1(
        new Mantid::MantidVec(xlen - 1, sqrt(3.0)));

    Workspace2D_sptr retVal(new Workspace2D);
    retVal->initialize(ylen, xlen, xlen - 1);
    double j = 1.0;

    for (int i = 0; i < xlen; i++) {
      (*x1)[i] = j * 0.5;
      j += 1.5;
    }

    for (int i = 0; i < ylen; i++) {
      retVal->setX(i, x1);
      retVal->setData(i, y1, e1);
    }

    return retVal;
  }
};

#endif /* MANTID_ALGORITHMS_CONVOLUTIONFITSEQUENTIALTEST_H_ */