// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 201p ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_ALGORITHMS_WEIGHTEDSUMDETECTORTEST_H_
#define MANTID_ALGORITHMS_WEIGHTEDSUMDETECTORTEST_H_

#include "MantidAPI/Algorithm.h"
#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidAlgorithms/WeightedSumDetector.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidDataObjects/WorkspaceCreation.h"
#include "MantidHistogramData/Histogram.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"
#include <cxxtest/TestSuite.h>

using Mantid::API::AlgorithmManager;
using Mantid::API::Algorithm_sptr;
using Mantid::API::MatrixWorkspace_sptr;
using Mantid::Algorithms::WeightedSumDetector;
using Mantid::MantidVec;

class WeightedSumDetectorTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static WeightedSumDetectorTest *createSuite() {
    return new WeightedSumDetectorTest();
  }
  static void destroySuite(WeightedSumDetectorTest *suite) {
    delete suite;
  }

  void setUp() override { Mantid::API::FrameworkManager::Instance(); }

  // generate spectrum
  MatrixWorkspace_sptr generate_DCS_data() {
    const double x_start = 0.2;
    const double x_end = 60.0;
    const double x_inc = 0.01;
    const int n_spec = 8;

    std::vector<double> x;
    std::vector<double> y;
    for (int i = 0; i < (x_end - x_start) / x_inc; i++) {
      x.push_back(x_start + i * x_inc);
    }
    for (int j = 0; j < n_spec; j++) {
      for (int i = 0; i < (x_end - x_start) / x_inc; i++) {
        y.push_back((1+j)*std::exp(-i));
      }
    }
    Algorithm_sptr alg =
        AlgorithmManager::Instance().createUnmanaged("CreateWorkspace");
    alg->initialize();
    alg->setProperty("OutputWorkspace", "incident_spectrum_ws");
    alg->setProperty("DataX", x);
    alg->setProperty("DataY", y);
    alg->setProperty("NSpec", n_spec);
    alg->execute();
    // retreve output workspace from ADS
    MatrixWorkspace_sptr out_ws =
        Mantid::API::AnalysisDataService::Instance()
            .retrieveWS<Mantid::API::MatrixWorkspace>("incident_spectrum_ws");
    return out_ws;
  }

  // generate spectrum
  MatrixWorkspace_sptr generate_SLF_data() {
    const double x_start = 0.2;
    const double x_end = 60.0;
    const double x_inc = 0.01;
    const int n_spec = 8;

    std::vector<double> x;
    std::vector<double> y;
    for (int i = 0; i < (x_end - x_start) / x_inc; i++) {
      x.push_back(x_start + i * x_inc);
    }
    for (int j = 0; j < n_spec; j++) {
      for (int i = 0; i < (x_end - x_start) / x_inc; i++) {
        y.push_back((1 + j) * std::exp(-2 * i));
      }
    }
    Algorithm_sptr alg =
        AlgorithmManager::Instance().createUnmanaged("CreateWorkspace");
    alg->initialize();
    alg->setProperty("OutputWorkspace", "incident_spectrum_ws");
    alg->setProperty("DataX", x);
    alg->setProperty("DataY", y);
    alg->setProperty("NSpec", n_spec);
    alg->execute();
    // retreve output workspace from ADS
    MatrixWorkspace_sptr out_ws =
        Mantid::API::AnalysisDataService::Instance()
            .retrieveWS<Mantid::API::MatrixWorkspace>("incident_spectrum_ws");
    return out_ws;
  }

  void test_Init() {
    WeightedSumDetector alg;
    alg.setRethrows(true);
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
  }

  void test_WeightedSumDetector_runs_without_correction_files() {
    MatrixWorkspace_sptr DCSws = generate_DCS_data();
    MatrixWorkspace_sptr SLFws = generate_SLF_data();
    auto alg = makeAlgorithm();

    alg->setProperty("DCSWorkspace", DCSws);
    alg->setProperty("SLFWorkspace", DCSws);
    alg->setProperty("OutputWorkspace", "merged_workspace");
    TS_ASSERT_THROWS_NOTHING(alg->execute());
  }

  void test_WeightedSumDetector_runs_with_alf_file() {
    MatrixWorkspace_sptr DCSws = generate_DCS_data();
    MatrixWorkspace_sptr SLFws = generate_SLF_data();

    auto alg = makeAlgorithm();

    alg->setProperty("DCSWorkspace", DCSws);
    alg->setProperty("SLFWorkspace", DCSws);
    alg->setProperty("OutputWorkspace", "merged_workspace");
    TS_ASSERT_THROWS_NOTHING(alg->execute());
  }

private:
  static boost::shared_ptr<WeightedSumDetector> makeAlgorithm() {
    auto a = boost::make_shared<WeightedSumDetector>();
    a->initialize();
    a->setChild(true);
    a->setRethrows(true);
    return a;
  }
};

#endif /* MANTID_ALGORITHMS_WEIGHTEDSUMDETECTORTEST_H_ */