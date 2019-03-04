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
    // A multi detector ws
    m_multiDetectorWSWithPeak = WorkspaceCreationHelper::
        create2DWorkspaceWithReflectometryInstrumentMultiDetector();
  }

  void test_executionPerSpectraAverage() {
    auto alg = setupAlgorithm();
    TS_ASSERT_THROWS_NOTHING(
        alg->setProperty("InputWorkspace", m_multiDetectorWSWithPeak))
    TS_ASSERT_THROWS_NOTHING(
        alg->setProperty("TypeOfBackgroundSubtraction", "Per Spectra Average"))
    TS_ASSERT_THROWS_NOTHING(alg->setProperty("BottomBackgroundRange", "0,2"))
    TS_ASSERT_THROWS_NOTHING(alg->setProperty("TopBackgroundRange", "3,4"))
    TS_ASSERT(alg->execute())
  }

  void test_executionPolynomial() {
    auto alg = setupAlgorithm();
    TS_ASSERT_THROWS_NOTHING(
        alg->setProperty("InputWorkspace", m_multiDetectorWSWithPeak))
    TS_ASSERT_THROWS_NOTHING(
        alg->setProperty("TypeOfBackgroundSubtraction", "Polynomial"))
    TS_ASSERT_THROWS_NOTHING(alg->setProperty("DegreeOfPolynomial", "0"))
    TS_ASSERT_THROWS_NOTHING(alg->setProperty("XRanges", "0,1.5,2.5,3"))
    TS_ASSERT(alg->execute())
  }

  void test_outputPerSpectraAverage() {
    auto alg = setupAlgorithm();
    alg->setProperty("InputWorkspace", m_multiDetectorWSWithPeak);
    alg->setProperty("TypeOfBackgroundSubtraction", "Per Spectra Average");
    alg->setProperty("BottomBackgroundRange", "0,2");
    alg->setProperty("TopBackgroundRange", "3,4");
    alg->execute();
    MatrixWorkspace_sptr outputWS = alg->getProperty("OutputWorkspace");


    const auto &output_counts = outputWS->counts(0);
    for (auto itr = output_counts.begin(); itr != output_counts.end(); ++itr) {
		TS_ASSERT_DELTA(0.0, *itr, 0.0001)
    }
  }

  void test_outputPolynomial() {
    auto alg = setupAlgorithm();
    alg->setProperty("InputWorkspace", m_multiDetectorWSWithPeak);
    alg->setProperty("TypeOfBackgroundSubtraction", "Polynomial");
    alg->setProperty("DegreeOfPolynomial", "0");
    alg->setProperty("XRanges", "0,1.5,2.5,3");
    alg->execute();
    MatrixWorkspace_sptr outputWS = alg->getProperty("OutputWorkspace");

    const auto &output_counts = outputWS->counts(0);
    for (auto itr = output_counts.begin(); itr != output_counts.end(); ++itr) {
        TS_ASSERT_DELTA(0.0, *itr, 0.0001)
    }
  }

  void test_default_OutputWorkspace() {
    ReflectometryBackgroundSubtraction alg;
    auto inputWS = MatrixWorkspace_sptr(m_multiDetectorWSWithPeak->clone());
    for (size_t i = 0; i < inputWS->getNumberHistograms(); ++i) {
      auto &y = inputWS->mutableY(i);
      for (size_t j = 0; j < y.size(); ++j) {
        y[j] += double(j + 1) * double(i + 1);
      }
    }

    alg.initialize();
    alg.setRethrows(true);
    alg.setProperty("InputWorkspace", inputWS);
    alg.setProperty("BottomBackgroundRange", "2,3");
    alg.setProperty("TopBackgroundRange", "3,4");
    alg.execute();

    TS_ASSERT(AnalysisDataService::Instance().doesExist("_Background"));
    AnalysisDataService::Instance().clear();
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