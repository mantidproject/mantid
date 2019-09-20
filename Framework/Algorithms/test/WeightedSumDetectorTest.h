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
#include "MantidTestHelpers/ScopedFileHelper.h"
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
  MatrixWorkspace_sptr generate_data(double x_start, double x_end, double x_inc,
                                     int n_spec, double decay) {
    std::vector<double> x;
    std::vector<double> y;
    for (int i = 0; i < (x_end - x_start) / x_inc; i++) {
      x.push_back(x_start + i * x_inc);
    }
    for (int j = 0; j < n_spec; j++) {
      for (int i = 0; i < (x_end - x_start) / x_inc; i++) {
        y.push_back((1.0 + j) * std::exp(-i * decay));
      }
    }
    Algorithm_sptr alg =
        AlgorithmManager::Instance().createUnmanaged("CreateWorkspace");
    alg->initialize();
    alg->setProperty("OutputWorkspace", "ws");
    alg->setProperty("DataX", x);
    alg->setProperty("DataY", y);
    alg->setProperty("NSpec", n_spec);
    alg->execute();
    // retreve output workspace from ADS
    MatrixWorkspace_sptr out_ws =
        Mantid::API::AnalysisDataService::Instance()
            .retrieveWS<Mantid::API::MatrixWorkspace>("ws");
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
        y.push_back((1.0 + j) * std::exp(-2 * i));
      }
    }
    Algorithm_sptr alg =
        AlgorithmManager::Instance().createUnmanaged("CreateWorkspace");
    alg->initialize();
    alg->setProperty("OutputWorkspace", "ws");
    alg->setProperty("DataX", x);
    alg->setProperty("DataY", y);
    alg->setProperty("NSpec", n_spec);
    alg->execute();
    // retreve output workspace from ADS
    MatrixWorkspace_sptr out_ws =
        Mantid::API::AnalysisDataService::Instance()
            .retrieveWS<Mantid::API::MatrixWorkspace>("ws");
    return out_ws;
  }

  void test_Init() {
    WeightedSumDetector alg;
    alg.setRethrows(true);
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
  }

  void test_WeightedSumDetector_runs_with_correction_files() {
    MatrixWorkspace_sptr DCSws = generate_data(0.2, 60.0, 0.01, 8, 1.0);
    MatrixWorkspace_sptr SLFws = generate_data(0.2, 60.0, 0.01, 8, 2.0);
    auto alg = makeAlgorithm();
    ScopedFileHelper::ScopedFile alf_file = gen_valid_alf();
    ScopedFileHelper::ScopedFile lim_file = gen_valid_lim();
    ScopedFileHelper::ScopedFile lin_file = gen_valid_lin();

    alg->setProperty("DCSWorkspace", DCSws);
    alg->setProperty("SLFWorkspace", SLFws);
    alg->setProperty("OutputWorkspace", "merged_workspace");
    alg->setProperty(".alf file", alf_file.getFileName());
    alg->setProperty(".lim file", lim_file.getFileName());
    alg->setProperty(".lin file", lin_file.getFileName());
    TS_ASSERT_THROWS_NOTHING(alg->execute());
  }

  void test_WeightedSumDetector_throws_with_diff_n_spec_file() {
    MatrixWorkspace_sptr DCSws = generate_data(0.2, 60.0, 0.01, 8, 1.0);
    MatrixWorkspace_sptr SLFws = generate_data(0.2, 60.0, 0.01, 6, 2.0);
    auto alg = makeAlgorithm();
    ScopedFileHelper::ScopedFile alf_file = gen_valid_alf();
    ScopedFileHelper::ScopedFile lim_file = gen_valid_lim();
    ScopedFileHelper::ScopedFile lin_file = gen_valid_lin();
    alg->setProperty("DCSWorkspace", DCSws);
    alg->setProperty("SLFWorkspace", SLFws);
    alg->setProperty("OutputWorkspace", "merged_workspace");
    alg->setProperty(".alf file", alf_file.getFileName());
    alg->setProperty(".lim file", lim_file.getFileName());
    alg->setProperty(".lin file", lin_file.getFileName());
    TS_ASSERT_THROWS(alg->execute(), const std::runtime_error &);
  }

  void test_WeightedSumDetector_throws_with_invalid_alf_file() {
    MatrixWorkspace_sptr DCSws = generate_data(0.2, 60.0, 0.01, 8, 1.0);
    MatrixWorkspace_sptr SLFws = generate_data(0.2, 60.0, 0.01, 8, 2.0);
    auto alg = makeAlgorithm();
    ScopedFileHelper::ScopedFile alf_file = gen_invalid_alf();
    ScopedFileHelper::ScopedFile lim_file = gen_valid_lim();
    ScopedFileHelper::ScopedFile lin_file = gen_valid_lin();
    alg->setProperty("DCSWorkspace", DCSws);
    alg->setProperty("SLFWorkspace", SLFws);
    alg->setProperty("OutputWorkspace", "merged_workspace");
    alg->setProperty(".alf file", alf_file.getFileName());
    alg->setProperty(".lim file", lim_file.getFileName());
    alg->setProperty(".lin file", lin_file.getFileName());
    TS_ASSERT_THROWS(alg->execute(), const std::runtime_error &);
  }

  void test_WeightedSumDetector_throws_with_invalid_lim_file() {
    MatrixWorkspace_sptr DCSws = generate_data(0.2, 60.0, 0.01, 8, 1.0);
    MatrixWorkspace_sptr SLFws = generate_data(0.2, 60.0, 0.01, 8, 2.0);
    auto alg = makeAlgorithm();
    ScopedFileHelper::ScopedFile alf_file = gen_valid_alf();
    ScopedFileHelper::ScopedFile lim_file = gen_invalid_lim();
    ScopedFileHelper::ScopedFile lin_file = gen_valid_lin();
    alg->setProperty("DCSWorkspace", DCSws);
    alg->setProperty("SLFWorkspace", SLFws);
    alg->setProperty("OutputWorkspace", "merged_workspace");
    alg->setProperty(".alf file", alf_file.getFileName());
    alg->setProperty(".lim file", lim_file.getFileName());
    alg->setProperty(".lin file", lin_file.getFileName());
    TS_ASSERT_THROWS(alg->execute(), const std::runtime_error &);
  }

  void test_WeightedSumDetector_throws_with_invalid_lin_file() {
    MatrixWorkspace_sptr DCSws = generate_data(0.2, 60.0, 0.01, 8, 1.0);
    MatrixWorkspace_sptr SLFws = generate_data(0.2, 60.0, 0.01, 8, 2.0);
    auto alg = makeAlgorithm();
    ScopedFileHelper::ScopedFile alf_file = gen_valid_alf();
    ScopedFileHelper::ScopedFile lim_file = gen_valid_lim();
    ScopedFileHelper::ScopedFile lin_file = gen_invalid_lin();
    alg->setProperty("DCSWorkspace", DCSws);
    alg->setProperty("SLFWorkspace", SLFws);
    alg->setProperty("OutputWorkspace", "merged_workspace");
    alg->setProperty(".alf file", alf_file.getFileName());
    alg->setProperty(".lim file", lim_file.getFileName());
    alg->setProperty(".lin file", lin_file.getFileName());
    TS_ASSERT_THROWS(alg->execute(), const std::runtime_error &);
  }

private:
  static ScopedFileHelper::ScopedFile gen_valid_alf() {
    const std::string content = " 8             \n"
                                " 1             1.5 \n"
                                " 2             1.1 \n"
                                " 3             1.2 \n"
                                " 4             1 \n"
                                " 5             0.8 \n"
                                " 6             0.8 \n"
                                " 7             0.8 \n"
                                " 8             0.8 ";
    return ScopedFileHelper::ScopedFile(content, "gem61910.alf");
  }
  static ScopedFileHelper::ScopedFile gen_invalid_alf() {
    const std::string content = " 6             \n"
                                " 1             1.5 \n"
                                " 2             1.1 \n"
                                " 3             1.2 \n"
                                " 4             1 \n"
                                " 5             0.8 \n"
                                " 6             0.8 ";
    return ScopedFileHelper::ScopedFile(content, "gem61910.alf");
  }
  static ScopedFileHelper::ScopedFile gen_valid_lim() {
    const std::string content =
        " 8                                        \n"
        " 1             1             1             30 \n"
        " 2             1             1             7 \n"
        " 3             1             0.9           7.3 \n"
        " 4             1             2.3           9.8 \n"
        " 5             1             6.2           13.2 \n"
        " 6             1                           "
        " 7             1             6.2           13.2 \n"
        " 8             1             10            14 \n";
    return ScopedFileHelper::ScopedFile(content, "gem61910.lim");
  }
  static ScopedFileHelper::ScopedFile gen_invalid_lim() {
    const std::string content =
        " 6                                        \n"
        " 1             1             1             30 \n"
        " 2             1             1             7 \n"
        " 3             1             0.9           7.3 \n"
        " 4             1             2.3           9.8 \n"
        " 5             1             6.2           13.2 \n"
        " 6             1                           ";
    return ScopedFileHelper::ScopedFile(content, "gem61910.lim");
  }
  static ScopedFileHelper::ScopedFile gen_valid_lin() {
    const std::string content =
        " 8                                       \n"
        " 1             0                           \n"
        " 2             1             0             0.045 \n"
        " 3             1             0             0.04 \n"
        " 4             1             0             0.045 \n"
        " 5             1             0             0.047 \n"
        " 6             1             0             0.044 \n"
        " 7             1             0             0.047 \n"
        " 8             1             0             0.044 ";
    return ScopedFileHelper::ScopedFile(content, "gem61910.lin");
  }
  static ScopedFileHelper::ScopedFile gen_invalid_lin() {
    const std::string content =
        " 6                                       \n"
        " 1             0                           \n"
        " 2             1             0             0.045 \n"
        " 3             1             0             0.04 \n"
        " 4             1             0             0.045 \n"
        " 5             1             0             0.047 \n"
        " 6             1             0             0.044 ";
    return ScopedFileHelper::ScopedFile(content, "gem61910.lin");
  }
  static boost::shared_ptr<WeightedSumDetector> makeAlgorithm() {
    auto a = boost::make_shared<WeightedSumDetector>();
    a->initialize();
    a->setChild(true);
    a->setRethrows(true);
    return a;
  }
};

#endif /* MANTID_ALGORITHMS_WEIGHTEDSUMDETECTORTEST_H_ */