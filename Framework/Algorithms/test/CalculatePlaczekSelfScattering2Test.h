// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <cxxtest/TestSuite.h>

#include "MantidAPI/Algorithm.h"
#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/Axis.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidAlgorithms/CalculatePlaczekSelfScattering2.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidDataObjects/WorkspaceCreation.h"
#include "MantidFrameworkTestHelpers/WorkspaceCreationHelper.h"
#include "MantidHistogramData/Histogram.h"

using Mantid::MantidVec;
using Mantid::Algorithms::CalculatePlaczekSelfScattering2;
using Mantid::API::Algorithm_sptr;
using Mantid::API::AlgorithmManager;
using Mantid::API::MatrixWorkspace_sptr;

class CalculatePlaczekSelfScattering2Test : public CxxTest::TestSuite {
public:
  // generate incident spectrum data
  std::vector<double> generateIncidentSpectrum(const Mantid::HistogramData::HistogramX &lambda, double phiMax = 6324.0,
                                               double phiEpi = 786.0, double alpha = 0.099, double lambda1 = 0.67143,
                                               double lambda2 = 0.06075, double lambdaT = 1.58) {
    std::vector<double> amplitude;
    const double dx = (lambda[1] - lambda[0]) / 2.0;
    for (double x : lambda) {
      if (x != lambda.back()) {
        double deltaTerm = 1.0 / (1.0 + exp(((x + dx) - lambda1) / lambda2));
        double term1 = phiMax * (pow(lambdaT, 4.0) / pow((x + dx), 5.0)) * exp(-pow((lambdaT / (x + dx)), 2.0));
        double term2 = phiEpi * deltaTerm / (pow((x + dx), (1.0 + 2.0 * alpha)));
        amplitude.emplace_back(term1 + term2);
      }
    }
    return amplitude;
  }

  // generate incident spectrum derivitive
  std::vector<double> generateIncidentSpectrumPrime(const Mantid::HistogramData::HistogramX &lambda,
                                                    double phiMax = 6324.0, double phiEpi = 786.0, double alpha = 0.099,
                                                    double lambda1 = 0.67143, double lambda2 = 0.06075,
                                                    double lambdaT = 1.58) {
    std::vector<double> amplitude;
    const double dx = (lambda[1] - lambda[0]) / 2.0;
    for (double x : lambda) {
      if (x != lambda.back()) {
        double deltaTerm = 1.0 / (1.0 + exp(((x + dx) - lambda1) / lambda2));
        double term1 = phiMax * pow(lambdaT, 4.0) * exp(-pow((lambdaT / (x + dx)), 2.0)) *
                       (-5 * pow((x + dx), -6.0) + 2 * pow((x + dx), -8.0) * lambdaT);
        double term2 = -phiEpi / pow((x + dx), (1.0 + 2.0 * alpha)) * deltaTerm *
                       ((1.0 + 2.0 * alpha) / (x + dx) + (1 / deltaTerm - 1) / lambda2 * deltaTerm);
        amplitude.emplace_back(term1 + term2);
      }
    }
    return amplitude;
  }

  // generate spectrum without detector info
  MatrixWorkspace_sptr generateIncidentSpectrum() {
    const double xStart = 0.2;
    const double xEnd = 4.0;
    const double xInc = 0.01;
    std::vector<double> x;
    std::vector<double> y;
    std::vector<double> yPrime;
    for (int i = 0; i < (xEnd - xStart) / xInc; i++) {
      x.emplace_back(xStart + i * xInc);
    }
    y = generateIncidentSpectrum(x);
    yPrime = generateIncidentSpectrumPrime(x);
    for (double prime : yPrime) {
      y.emplace_back(prime);
    }
    Algorithm_sptr alg = AlgorithmManager::Instance().createUnmanaged("CreateWorkspace");
    alg->initialize();
    alg->setProperty("OutputWorkspace", "incident_spectrum_ws");
    alg->setProperty("DataX", x);
    alg->setProperty("DataY", y);
    alg->setProperty("NSpec", 2);
    alg->setProperty("UnitX", "Wavelength");
    alg->execute();
    // retreve output workspace from ADS
    MatrixWorkspace_sptr outWs =
        Mantid::API::AnalysisDataService::Instance().retrieveWS<Mantid::API::MatrixWorkspace>("incident_spectrum_ws");
    return outWs;
  }

  // Add sample to workspace
  void addSampleMaterialToWorkspace(const std::string wsname) {
    Algorithm_sptr alg = AlgorithmManager::Instance().createUnmanaged("SetSampleMaterial");
    alg->initialize();
    alg->setProperty("InputWorkspace", wsname);
    alg->setProperty("ChemicalFormula", "Si");
    alg->setProperty("SampleNumberDensity", 0.1);
    alg->execute();
    return;
  }

  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static CalculatePlaczekSelfScattering2Test *createSuite() { return new CalculatePlaczekSelfScattering2Test(); }
  static void destroySuite(CalculatePlaczekSelfScattering2Test *suite) { delete suite; }

  void setUp() override { Mantid::API::FrameworkManager::Instance(); }

  void testInit() {
    CalculatePlaczekSelfScattering2 alg;
    alg.setRethrows(true);
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
  }

  void testCalculatePlaczekSelfScatteringExecutes() {
    const std::string wsname = "wstest1";

    MatrixWorkspace_sptr IncidentSpecta = generateIncidentSpectrum();
    auto alg = makeAlgorithm();
    Mantid::DataObjects::Workspace2D_sptr InputWorkspace =
        WorkspaceCreationHelper::create2DWorkspaceWithRectangularInstrument(5, 100, 380);
    Mantid::API::AnalysisDataService::Instance().addOrReplace(wsname, InputWorkspace);
    addSampleMaterialToWorkspace(wsname);
    alg->setProperty("IncidentSpecta", IncidentSpecta);
    alg->setPropertyValue("InputWorkspace", wsname);
    alg->setProperty("OutputWorkspace", "correction_ws");
    TS_ASSERT_THROWS_NOTHING(alg->execute());
  }

  // NOTE:
  // The workspace validator goes off first, makes the following tests irrelevant
  // void testCalculatePlaczekSelfScatteringDoesNotRunWithNoDetectors() {
  //   const std::string wsname = "wstest2";

  //   MatrixWorkspace_sptr IncidentSpecta = generateIncidentSpectrum();
  //   Mantid::DataObjects::Workspace2D_sptr InputWorkspace = WorkspaceCreationHelper::create2DWorkspace(30, 381);
  //   Mantid::API::AnalysisDataService::Instance().addOrReplace(wsname, InputWorkspace);
  //   addSampleMaterialToWorkspace(wsname);
  //   auto alg = makeAlgorithm();
  //   alg->setProperty("IncidentSpecta", IncidentSpecta);
  //   alg->setPropertyValue("InputWorkspace", wsname);
  //   alg->setProperty("OutputWorkspace", "correction_ws");
  //   TS_ASSERT_THROWS(alg->execute(), const std::runtime_error &)
  // }

  // void testCalculatePlaczekSelfScatteringDoesNotRunWithNoSample() {
  //   MatrixWorkspace_sptr IncidentSpecta = generateIncidentSpectrum();
  //   Mantid::DataObjects::Workspace2D_sptr InputWorkspace =
  //       WorkspaceCreationHelper::create2DWorkspaceWithRectangularInstrument(5, 100, 380);
  //   Mantid::API::AnalysisDataService::Instance().addOrReplace("InputWorkspace", InputWorkspace);
  //   auto alg = makeAlgorithm();
  //   alg->setProperty("IncidentSpecta", IncidentSpecta);
  //   alg->setProperty("InputWorkspace", InputWorkspace);
  //   alg->setProperty("OutputWorkspace", "correction_ws");
  //   TS_ASSERT_THROWS(alg->execute(), const std::runtime_error &)
  // }

private:
  static std::shared_ptr<CalculatePlaczekSelfScattering2> makeAlgorithm() {
    auto a = std::make_shared<CalculatePlaczekSelfScattering2>();
    a->initialize();
    a->setChild(true);
    a->setRethrows(true);
    return a;
  }
};
