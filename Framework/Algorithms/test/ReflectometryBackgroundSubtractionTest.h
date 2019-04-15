// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_ALGORITHMS_REFLECTOMETRYBACKGROUNDSUBTRACTIONTEST_H_
#define MANTID_ALGORITHMS_REFLECTOMETRYBACKGROUNDSUBTRACTIONTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAPI/FrameworkManager.h"
#include "MantidAlgorithms/ReflectometryBackgroundSubtraction.h"
#include "MantidTestHelpers/ReflectometryHelper.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"

using Mantid::Algorithms::ReflectometryBackgroundSubtraction;
using namespace Mantid::API;

class ReflectometryBackgroundSubtractionTest : public CxxTest::TestSuite {
private:
  MatrixWorkspace_sptr m_multiDetectorWSWithPeak;

public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static ReflectometryBackgroundSubtractionTest *createSuite() {
    return new ReflectometryBackgroundSubtractionTest();
  }
  static void destroySuite(ReflectometryBackgroundSubtractionTest *suite) {
    delete suite;
  }

  ReflectometryBackgroundSubtractionTest() {
    FrameworkManager::Instance();

    // A multi detector ws with a peak in the 3rd spectra (values equal to 5)
    // and background of 2.0
    m_multiDetectorWSWithPeak =
        Mantid::TestHelpers::createREFL_WS(3, 0.0, 30.0, {2, 2, 2, 2, 2, 2, 2});
    const std::vector<double> yValues = {5, 5, 5, 5, 5, 5, 5};
    for (size_t i = 0; i < m_multiDetectorWSWithPeak->y(5).size(); ++i) {
      m_multiDetectorWSWithPeak->mutableY(3)[i] = yValues[i];
    }
  }
  void test_executionPerSpectraAverage() {
    auto alg = setupAlgorithm();
    TS_ASSERT_THROWS_NOTHING(
        alg->setProperty("InputWorkspace", m_multiDetectorWSWithPeak))
    TS_ASSERT_THROWS_NOTHING(
        alg->setProperty("BackgroundCalculationMethod", "Per Detector Average"))
    TS_ASSERT(alg->execute())
  }

  void test_executionPolynomial() {
    auto alg = setupAlgorithm();
    TS_ASSERT_THROWS_NOTHING(
        alg->setProperty("InputWorkspace", m_multiDetectorWSWithPeak))
    TS_ASSERT_THROWS_NOTHING(
        alg->setProperty("BackgroundCalculationMethod", "Polynomial"))
    TS_ASSERT(alg->execute())
  }

  void test_executionAveragePixelFit() {
    auto alg = setupAlgorithm();
    TS_ASSERT_THROWS_NOTHING(
        alg->setProperty("InputWorkspace", m_multiDetectorWSWithPeak))
    TS_ASSERT_THROWS_NOTHING(
        alg->setProperty("BackgroundCalculationMethod", "Average Pixel Fit"))
    TS_ASSERT_THROWS_NOTHING(
        alg->setProperty("InputWorkspaceIndexSet", "0-2,4-6"))
    TS_ASSERT(alg->execute())
  }

  void test_PerSpectraAverageOutput() {
    // test output of perSpectraAverage method
    // InputWorkspaceIndexSet set to spectra containing background
    // output should be 0 for all counts except at peak where values should
    // be 3.0
    auto alg = setupAlgorithm();
    alg->setProperty("InputWorkspace", m_multiDetectorWSWithPeak);
    alg->setProperty("InputWorkspaceIndexSet", "0-2,4-6");
    alg->setProperty("BackgroundCalculationMethod", "Per Detector Average");
    alg->execute();
    MatrixWorkspace_sptr outputWS = alg->getProperty("OutputWorkspace");

    for (size_t i = 0; i != outputWS->getNumberHistograms(); ++i) {
      const auto &output_counts = outputWS->counts(i);
      if (i != 3) {
        for (auto itr = output_counts.begin(); itr != output_counts.end();
             ++itr) {
          TS_ASSERT_DELTA(0.0, *itr, 0.0001)
        }
      } else {
        for (auto itr = output_counts.begin(); itr != output_counts.end();
             ++itr) {
          TS_ASSERT_DELTA(3.0, *itr, 0.0001)
        }
      }
    }

    TS_ASSERT(alg->execute())
  }

  void test_PolynomialOutput() {
    // test output of polynomial method
    // InputWorkspaceIndexSet set to spectra containing background
    // output should be 0 for all counts except at peak where values should
    // be 3.0
    auto alg = setupAlgorithm();
    alg->setProperty("InputWorkspace", m_multiDetectorWSWithPeak);
    alg->setProperty("InputWorkspaceIndexSet", "0-2,4-6");
    alg->setProperty("BackgroundCalculationMethod", "Polynomial");
    alg->setProperty("DegreeOfPolynomial", "0");
    alg->execute();
    MatrixWorkspace_sptr outputWS = alg->getProperty("OutputWorkspace");

    for (size_t i = 0; i != outputWS->getNumberHistograms(); ++i) {
      const auto &output_counts = outputWS->counts(i);
      if (i != 3) {
        for (auto itr = output_counts.begin(); itr != output_counts.end();
             ++itr) {
          TS_ASSERT_DELTA(0.0, *itr, 0.0001)
        }
      } else {
        for (auto itr = output_counts.begin(); itr != output_counts.end();
             ++itr) {
          TS_ASSERT_DELTA(3.0, *itr, 0.0001)
        }
      }
    }
  }

  void test_AveragePixelFitOutput() {
    // test output of Average Pixel Fit method
    // InputWorkspaceIndexSet set to spectra containing background
    // output should be 0 for all counts except at peak where values should
    // be 3.0
    auto alg = setupAlgorithm();
    alg->setProperty("InputWorkspace", m_multiDetectorWSWithPeak);
    alg->setProperty("BackgroundCalculationMethod", "Average Pixel Fit");
    alg->setProperty("InputWorkspaceIndexSet", "0-2,4-6");
    alg->execute();
    MatrixWorkspace_sptr outputWS = alg->getProperty("OutputWorkspace");

    for (size_t i = 0; i != outputWS->getNumberHistograms(); ++i) {
      const auto &output_counts = outputWS->counts(i);
      if (i != 3) {
        for (auto itr = output_counts.begin(); itr != output_counts.end();
             ++itr) {
          TS_ASSERT_DELTA(0.0, *itr, 0.0001)
        }
      } else {
        for (auto itr = output_counts.begin(); itr != output_counts.end();
             ++itr) {
          TS_ASSERT_DELTA(3.0, *itr, 0.0001)
        }
      }
    }
  }

  void test_PolynomialSingleSpectraInputError() {
    // test output of polynomial method returns an error when one spectra is
    // entered
    auto alg = setupAlgorithm();
    alg->setProperty("InputWorkspace", m_multiDetectorWSWithPeak);
    alg->setProperty("InputWorkspaceIndexSet", "2");
    alg->setProperty("BackgroundCalculationMethod", "Polynomial");
    alg->setProperty("DegreeOfPolynomial", "0");
    TS_ASSERT_THROWS_ANYTHING(alg->execute())
  }

private:
  static boost::shared_ptr<ReflectometryBackgroundSubtraction>
  setupAlgorithm() {
    auto alg = boost::make_shared<ReflectometryBackgroundSubtraction>();
    alg->initialize();
    alg->setChild(true);
    alg->setRethrows(true);
    return alg;
  }
};

#endif /* MANTID_ALGORITHMS_REFLECTOMETRYBACKGROUNDSUBTRACTIONTEST_H_ */