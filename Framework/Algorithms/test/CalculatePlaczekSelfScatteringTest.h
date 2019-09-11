// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 201p ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_ALGORITHMS_CALCULATEPLACZEKSELFSCATTERINGTEST_H_
#define MANTID_ALGORITHMS_CALCULATEPLACZEKSELFSCATTERINGTEST_H_


#include "MantidAPI/Algorithm.h"
#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidAlgorithms/CalculatePlaczekSelfScattering.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidDataObjects/WorkspaceCreation.h"
#include "MantidHistogramData/Histogram.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"
#include <cxxtest/TestSuite.h>

using Mantid::API::AlgorithmManager;
using Mantid::API::Algorithm_sptr;
using Mantid::API::MatrixWorkspace_sptr;
using Mantid::Algorithms::CalculatePlaczekSelfScattering;
using Mantid::MantidVec;

class CalculatePlaczekSelfScatteringTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static CalculatePlaczekSelfScatteringTest *createSuite() {
    return new CalculatePlaczekSelfScatteringTest();
  }
  static void destroySuite(CalculatePlaczekSelfScatteringTest *suite) {
    delete suite;
  }

  void setUp() override { Mantid::API::FrameworkManager::Instance(); }

  // generate spectrum with detector info
  MatrixWorkspace_sptr generate_incident_spectrum_with_detector_data() {
    Algorithm_sptr alg =
        AlgorithmManager::Instance().createUnmanaged("CreateSampleWorkspace");
    alg->initialize();
    alg->setProperty("OutputWorkspace", "incident_spectrum_ws");
    alg->setProperty("XMin", x_start);
    alg->setProperty("XMax", x_end);
    alg->setProperty("BinWidth", x_inc);
    alg->setProperty("BankPixelWidth", 1);
    alg->execute();
    // get the output workspace from ADS
    MatrixWorkspace_sptr out_ws =
        Mantid::API::AnalysisDataService::Instance()
            .retrieveWS<Mantid::API::MatrixWorkspace>("incident_spectrum_ws");
    Mantid::HistogramData::HistogramX x = out_ws->x(0);
    std::vector<double> y = generate_incident_spectrum(x);
    out_ws->setCounts(0, y);
    std::vector<double> y_prime = generate_incident_spectrum_prime(x);
    out_ws->setCounts(1, y_prime);
    return out_ws;
  }

  // generate spectrum without detector info
  MatrixWorkspace_sptr generate_incident_spectrum_without_detector_data() {
    const double x_start = 0.2;
    const double x_end = 4.0;
    const double x_inc = 0.01;
    std::vector<double> x;
    std::vector<double> y;
    std::vector<double> y_prime;
    for (int i = 0; i < (x_end - x_start) / x_inc; i++) {
      x.push_back(x_start + i * x_inc);
    }
    y = generate_incident_spectrum(x);
    y_prime = generate_incident_spectrum_prime(x);
    for (double prime : y_prime) {
      y.push_back(prime);
    }
    Algorithm_sptr alg =
        AlgorithmManager::Instance().createUnmanaged("CreateWorkspace");
    alg->initialize();
    alg->setProperty("OutputWorkspace", "incident_spectrum_ws");
    alg->setProperty("DataX", x);
    alg->setProperty("DataY", y);
    alg->setProperty("NSpec", 2);
    alg->execute();
    // retreve output workspace from ADS
    MatrixWorkspace_sptr out_ws =
        Mantid::API::AnalysisDataService::Instance()
            .retrieveWS<Mantid::API::MatrixWorkspace>("incident_spectrum_ws");
    return out_ws;
  }

  // generate incident spectrum data
  std::vector<double> generate_incident_spectrum(
	  const Mantid::HistogramData::HistogramX &lambda, double phi_max = 6324.0, 
	  double phi_epi = 786.0, double alpha = 0.099, double lambda_1 = 0.67143, 
	  double lambda_2 = 0.06075, double lambda_T = 1.58) {
    std::vector<double> amplitude;
    const double dx = x_inc / 2.0;
    for (double x : lambda) {
      if (x != lambda.back()) {
        double delta_term = 1.0 / (1.0 + exp(((x + dx) - lambda_1) / lambda_2));
        double term1 = phi_max * (pow(lambda_T, 4.0) / pow((x + dx), 5.0)) *
                       exp(-pow((lambda_T / (x + dx)), 2.0));
        double term2 =
            phi_epi * delta_term / (pow((x + dx), (1.0 + 2.0 * alpha)));
        amplitude.push_back(term1 + term2);
      }
    }
    return amplitude;
  }

  // generate incident spectrum derivitive
  std::vector<double> generate_incident_spectrum_prime(
      const Mantid::HistogramData::HistogramX &lambda, double phi_max = 6324.0,
      double phi_epi = 786.0, double alpha = 0.099, double lambda_1 = 0.67143,
      double lambda_2 = 0.06075, double lambda_T = 1.58) {
    std::vector<double> amplitude;
    const double dx = x_inc / 2.0;
    for (double x : lambda) {
      if (x != lambda.back()) {
        double delta_term = 1.0 / (1.0 + exp(((x + dx) - lambda_1) / lambda_2));
        double term1 =
            phi_max * pow(lambda_T, 4.0) *
            exp(-pow((lambda_T / (x + dx)), 2.0)) *
            (-5 * pow((x + dx), -6.0) + 2 * pow((x + dx), -8.0) * lambda_T);
        double term2 = -phi_epi / pow((x + dx), (1.0 + 2.0 * alpha)) *
                       delta_term *
                       ((1.0 + 2.0 * alpha) / (x + dx) +
                        (1 / delta_term - 1) / lambda_2 * delta_term);
        amplitude.push_back(term1 + term2);
      }
    }
    return amplitude;
  }

  // Add sample to workspace
  MatrixWorkspace_sptr
  add_sample_material_to_workspace(MatrixWorkspace_sptr in_ws) {
    Algorithm_sptr alg =
        AlgorithmManager::Instance().createUnmanaged("SetSampleMaterial");
    alg->initialize();
    alg->setProperty("InputWorkspace", "incident_spectrum_ws");
    alg->setProperty("ChemicalFormula", "(Li7)2-C-H4-N-Cl6");
    alg->setProperty("SampleNumberDensity", 0.1);
    alg->execute();
    return (in_ws);
  }

  void test_Init() {
    std::cout << "test_Init running" << std::endl;
    CalculatePlaczekSelfScattering alg;
    alg.setRethrows(true);
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
  }

  void test_CalculatePlaczekSelfScattering_executes() {
    MatrixWorkspace_sptr ws = generate_incident_spectrum_with_detector_data();
    ws = add_sample_material_to_workspace(ws);
    auto alg = makeAlgorithm();
    alg->setProperty("InputWorkspace", "incident_spectrum_ws");
    alg->setProperty("OutputWorkspace", "correction_ws");
    TS_ASSERT_THROWS_NOTHING(alg->execute());
  }

  void test_CalculatePlaczekSelfScattering_does_not_run_with_no_detectors() {
    MatrixWorkspace_sptr ws =
        generate_incident_spectrum_without_detector_data();
    ws = add_sample_material_to_workspace(ws);

    auto alg = makeAlgorithm();
    alg->setProperty("InputWorkspace", "incident_spectrum_ws");
    alg->setProperty("OutputWorkspace", "correction_ws");
    TS_ASSERT_THROWS(alg->execute(), const std::runtime_error &)
  }

  void test_CalculatePlaczekSelfScattering_does_not_run_with_no_sample() {
    MatrixWorkspace_sptr ws = generate_incident_spectrum_with_detector_data();

    auto alg = makeAlgorithm();
    alg->setProperty("InputWorkspace", "incident_spectrum_ws");
    alg->setProperty("OutputWorkspace", "correction_ws");
    TS_ASSERT_THROWS(alg->execute(), const std::runtime_error &)
  }

private:
  const double x_start = 0.2;
  const double x_end = 4.0;
  const double x_inc = 0.01;
  static boost::shared_ptr<CalculatePlaczekSelfScattering> makeAlgorithm() {
    auto a = boost::make_shared<CalculatePlaczekSelfScattering>();
    a->initialize();
    a->setChild(true);
    a->setRethrows(true);
    return a;
  }
};

#endif /* MANTID_ALGORITHMS_CALCULATEPLACZEKSELFSCATTERINGTEST_H_ */
