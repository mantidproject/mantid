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

// generate incident spectrum data
std::vector<double>
generateIncidentSpectrum(const Mantid::HistogramData::HistogramX &lambda,
                         double phiMax = 6324.0, double phiEpi = 786.0,
                         double alpha = 0.099, double lambda1 = 0.67143,
                         double lambda2 = 0.06075, double lambdaT = 1.58) {
  std::vector<double> amplitude;
  const double dx = (lambda[1] - lambda[0]) / 2.0;
  for (double x : lambda) {
    if (x != lambda.back()) {
      double deltaTerm = 1.0 / (1.0 + exp(((x + dx) - lambda1) / lambda2));
      double term1 = phiMax * (pow(lambdaT, 4.0) / pow((x + dx), 5.0)) *
                     exp(-pow((lambdaT / (x + dx)), 2.0));
      double term2 = phiEpi * deltaTerm / (pow((x + dx), (1.0 + 2.0 * alpha)));
      amplitude.push_back(term1 + term2);
    }
  }
  return amplitude;
}

// generate incident spectrum derivitive
std::vector<double>
generateIncidentSpectrumPrime(const Mantid::HistogramData::HistogramX &lambda,
                              double phiMax = 6324.0, double phiEpi = 786.0,
                              double alpha = 0.099, double lambda1 = 0.67143,
                              double lambda2 = 0.06075, double lambdaT = 1.58) {
  std::vector<double> amplitude;
  const double dx = (lambda[1] - lambda[0]) / 2.0;
  for (double x : lambda) {
    if (x != lambda.back()) {
      double deltaTerm = 1.0 / (1.0 + exp(((x + dx) - lambda1) / lambda2));
      double term1 =
          phiMax * pow(lambdaT, 4.0) * exp(-pow((lambdaT / (x + dx)), 2.0)) *
          (-5 * pow((x + dx), -6.0) + 2 * pow((x + dx), -8.0) * lambdaT);
      double term2 = -phiEpi / pow((x + dx), (1.0 + 2.0 * alpha)) * deltaTerm *
                     ((1.0 + 2.0 * alpha) / (x + dx) +
                      (1 / deltaTerm - 1) / lambda2 * deltaTerm);
      amplitude.push_back(term1 + term2);
    }
  }
  return amplitude;
}

// generate spectrum with detector info
MatrixWorkspace_sptr generateIncidentSpectrumWithDetectorData() {
  const double xStart = 0.2;
  const double xEnd = 4.0;
  const double xInc = 0.01;
  Algorithm_sptr alg =
      AlgorithmManager::Instance().createUnmanaged("CreateSampleWorkspace");
  alg->initialize();
  alg->setProperty("OutputWorkspace", "incident_spectrum_ws");
  alg->setProperty("XMin", xStart);
  alg->setProperty("XMax", xEnd);
  alg->setProperty("BinWidth", xInc);
  alg->setProperty("BankPixelWidth", 1);
  alg->execute();
  // get the output workspace from ADS
  MatrixWorkspace_sptr outWs =
      Mantid::API::AnalysisDataService::Instance()
          .retrieveWS<Mantid::API::MatrixWorkspace>("incident_spectrum_ws");
  Mantid::HistogramData::HistogramX x = outWs->x(0);
  std::vector<double> y = generateIncidentSpectrum(x);
  outWs->setCounts(0, y);
  std::vector<double> yPrime = generateIncidentSpectrumPrime(x);
  outWs->setCounts(1, yPrime);
  return outWs;
}

// generate spectrum without detector info
MatrixWorkspace_sptr generateIncidentSpectrumWithoutDetectorData() {
  const double xStart = 0.2;
  const double xEnd = 4.0;
  const double xInc = 0.01;
  std::vector<double> x;
  std::vector<double> y;
  std::vector<double> yPrime;
  for (int i = 0; i < (xEnd - xStart) / xInc; i++) {
    x.push_back(xStart + i * xInc);
  }
  y = generateIncidentSpectrum(x);
  yPrime = generateIncidentSpectrumPrime(x);
  for (double prime : yPrime) {
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
  MatrixWorkspace_sptr outWs =
      Mantid::API::AnalysisDataService::Instance()
          .retrieveWS<Mantid::API::MatrixWorkspace>("incident_spectrum_ws");
  return outWs;
}

// Add sample to workspace
MatrixWorkspace_sptr addSampleMaterialToWorkspace(MatrixWorkspace_sptr inWs) {
  Algorithm_sptr alg =
      AlgorithmManager::Instance().createUnmanaged("SetSampleMaterial");
  alg->initialize();
  alg->setProperty("InputWorkspace", "incident_spectrum_ws");
  alg->setProperty("ChemicalFormula", "(Li7)2-C-H4-N-Cl6");
  alg->setProperty("SampleNumberDensity", 0.1);
  alg->execute();
  return (inWs);
}

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

  void testInit() {
    CalculatePlaczekSelfScattering alg;
    alg.setRethrows(true);
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
  }

  void testCalculatePlaczekSelfScatteringExecutes() {
    MatrixWorkspace_sptr ws = generateIncidentSpectrumWithDetectorData();
    ws = addSampleMaterialToWorkspace(ws);
    auto alg = makeAlgorithm();
    alg->setProperty("InputWorkspace", "incident_spectrum_ws");
    alg->setProperty("OutputWorkspace", "correction_ws");
    TS_ASSERT_THROWS_NOTHING(alg->execute());
  }

  void testCalculatePlaczekSelfScatteringDoesNotRunWithNoDetectors() {
    MatrixWorkspace_sptr ws = generateIncidentSpectrumWithoutDetectorData();
    ws = addSampleMaterialToWorkspace(ws);

    auto alg = makeAlgorithm();
    alg->setProperty("InputWorkspace", "incident_spectrum_ws");
    alg->setProperty("OutputWorkspace", "correction_ws");
    TS_ASSERT_THROWS(alg->execute(), std::runtime_error)
  }

  void testCalculatePlaczekSelfScatteringDoesNotRunWithNoSample() {
    MatrixWorkspace_sptr ws = generateIncidentSpectrumWithDetectorData();

    auto alg = makeAlgorithm();
    alg->setProperty("InputWorkspace", "incident_spectrum_ws");
    alg->setProperty("OutputWorkspace", "correction_ws");
    TS_ASSERT_THROWS(alg->execute(), std::runtime_error)
  }

private:
  static boost::shared_ptr<CalculatePlaczekSelfScattering> makeAlgorithm() {
    auto a = boost::make_shared<CalculatePlaczekSelfScattering>();
    a->initialize();
    a->setChild(true);
    a->setRethrows(true);
    return a;
  }
};

#endif /* MANTID_ALGORITHMS_CALCULATEPLACZEKSELFSCATTERINGTEST_H_ */
