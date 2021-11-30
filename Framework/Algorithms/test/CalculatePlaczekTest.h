// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidAlgorithms/CalculatePlaczek.h"
#include "MantidFrameworkTestHelpers/WorkspaceCreationHelper.h"
#include <cxxtest/TestSuite.h>

using namespace Mantid::API;
using Mantid::Algorithms::CalculatePlaczek;

class CalculatePlaczekTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static CalculatePlaczekTest *createSuite() { return new CalculatePlaczekTest(); }
  static void destroySuite(CalculatePlaczekTest *suite) { delete suite; }

  void setUp() override { Mantid::API::FrameworkManager::Instance(); }

  void test_Init() {
    CalculatePlaczek alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
  }

  void test_1stOrderPlaczekCorrection() {
    // ---- Create the simple workspace ----
    Mantid::DataObjects::Workspace2D_sptr InputWorkspace =
        WorkspaceCreationHelper::create2DWorkspaceWithRectangularInstrument(5, 100, 380);
    const std::string inwsn = "inws";
    Mantid::API::AnalysisDataService::Instance().addOrReplace(inwsn, InputWorkspace);

    // add sample
    addSampleMaterialToWorkspace(inwsn);

    // ---- Get the incident spectrum ----
    const std::string IncidentSpectaWSN = "incidentSpectrumWS";
    generateIncidentSpectrum(IncidentSpectaWSN);

    CalculatePlaczek alg;
    alg.initialize();
    TS_ASSERT(alg.isInitialized());
    alg.setProperty("InputWorkspace", inwsn);
    alg.setPropertyValue("IncidentSpectra", IncidentSpectaWSN);
    alg.setProperty("Order", 1);
    alg.setProperty("SampleTemperature", 300.0); // K
    alg.setProperty("CrystalDensity", 0.01);
    alg.setPropertyValue("OutputWorkspace", "outws");
    alg.execute();
    TS_ASSERT(alg.isExecuted());

    // check the output against some reference values
    // NOTE: we are using fake incident flux, so these values here are not physically meaningful
    Mantid::API::MatrixWorkspace_sptr outputWS =
        std::dynamic_pointer_cast<MatrixWorkspace>(AnalysisDataService::Instance().retrieve("outws"));
    TS_ASSERT_DELTA(outputWS->readY(0)[0], 10.0, 1e-8);
    TS_ASSERT_DELTA(outputWS->readY(0)[1], 10.0, 1e-8);
  }

  void test_2ndOrderPlaczekCorrection() {
    // ---- Create the simple workspace ----
    Mantid::DataObjects::Workspace2D_sptr InputWorkspace =
        WorkspaceCreationHelper::create2DWorkspaceWithRectangularInstrument(5, 100, 380);
    const std::string inwsn = "inws2";
    Mantid::API::AnalysisDataService::Instance().addOrReplace(inwsn, InputWorkspace);
    // add sample
    addSampleMaterialToWorkspace(inwsn);

    // ---- Get the incident spectrum ----
    const std::string IncidentSpectaWSN = "incidentSpectrumWS";
    generateIncidentSpectrum(IncidentSpectaWSN);

    CalculatePlaczek alg;
    alg.initialize();
    TS_ASSERT(alg.isInitialized());
    alg.setProperty("InputWorkspace", inwsn);
    alg.setPropertyValue("IncidentSpectra", IncidentSpectaWSN);
    alg.setProperty("Order", 2);
    alg.setProperty("SampleTemperature", 300.0); // K
    alg.setProperty("CrystalDensity", 0.01);
    alg.setPropertyValue("OutputWorkspace", "outws");
    alg.execute();
    TS_ASSERT(alg.isExecuted());

    // check the output against some reference values
    // NOTE: we are using fake incident flux, so these values here are not physically meaningful
    Mantid::API::MatrixWorkspace_sptr outputWS =
        std::dynamic_pointer_cast<MatrixWorkspace>(AnalysisDataService::Instance().retrieve("outws"));
    TS_ASSERT_DELTA(outputWS->readY(0)[0], 10.0001512625, 1e-8);
    TS_ASSERT_DELTA(outputWS->readY(0)[1], 10.0002058857, 1e-8);
  }

private:
  // generate incident spectrum data
  // NOTE:
  //  Since the actual spectrum (flux) and its derivative should be provided by the user,
  //  the testing here is using a mocked spectrum, x denotes lambda
  //        -(x-0.05)(x-2)(x-2.2)(x-3)  x \in (0.05, 2)
  //  fig@https://www.wolframalpha.com/input/?i=-%28x-0.05%29%28x-2%29%28x-2.2%29%28x-3%29
  std::vector<double> generateIncidentSpectrum(const Mantid::HistogramData::HistogramX &lambda) {
    std::vector<double> amplitude;
    std::transform(lambda.begin(), lambda.end(), std::back_inserter(amplitude),
                   [](double x) { return -(x - 0.05) * (x - 2) * (x - 2.2) * (x - 3); });
    return amplitude;
  }

  // generate incident spectrum derivitive
  // NOTE:
  //  The first order derivative of the mocked spectrum above is
  //        -4(-3.5125 + 8.68 x - 5.4375 x^2 + x^3)  x \in (0.05, 2)
  //  fig@https://www.wolframalpha.com/input/?i=-4.%28-3.5125+%2B+8.68+x+-+5.4375+x%5E2+%2B+x%5E3%29
  std::vector<double> generateIncidentSpectrumPrime(const Mantid::HistogramData::HistogramX &lambda) {
    std::vector<double> amplitude;
    std::transform(lambda.begin(), lambda.end(), std::back_inserter(amplitude),
                   [](double x) { return -4 * (-3.5125 + 8.68 * x - 5.4375 * x * x + x * x * x); });
    return amplitude;
  }

  // generate second order derivative of the incident spectrum
  // NOTE:
  //  The second order derivative of the mocked spectrum above is
  //        -34.72 + 43.5 x - 12 x^2  x \in (0.05, 2)
  //  fig@https://www.wolframalpha.com/input/?i=-34.72+%2B+43.5+x+-+12+x%5E2
  std::vector<double> generateIncidentSpectrumPrimePrime(const Mantid::HistogramData::HistogramX &lambda) {
    std::vector<double> amplitude;
    std::transform(lambda.begin(), lambda.end(), std::back_inserter(amplitude),
                   [](double x) { return -34.72 + 43.5 * x - 12 * x * x; });
    return amplitude;
  }

  /**
   * @brief generate a worksapce with the incident spectrum using given name
   *
   * @param wsname
   */
  void generateIncidentSpectrum(const std::string &wsname) {
    const double xMin = 0.06;
    const double xMax = 2.0;
    const double xStep = 0.01;
    std::vector<double> xVec, yVec, yPrimeVec, yPrimePrimeVec;

    // make the lambda vector
    for (double x = xMin; x < xMax; x += xStep) {
      xVec.emplace_back(x);
    }

    // make the amplitude vector
    yVec = generateIncidentSpectrum(xVec);

    // make the first order derivitive vector
    yPrimeVec = generateIncidentSpectrumPrime(xVec);

    // make the second order derivitive vector
    yPrimePrimeVec = generateIncidentSpectrumPrimePrime(xVec);

    // extend the y vector to contain the other two dimension
    for (double yp : yPrimeVec) {
      yVec.emplace_back(yp);
    }
    for (double ypp : yPrimePrimeVec) {
      yVec.emplace_back(ypp);
    }

    // create the workspace
    Algorithm_sptr alg = AlgorithmManager::Instance().createUnmanaged("CreateWorkspace");
    alg->initialize();
    alg->setProperty("OutputWorkspace", wsname);
    alg->setProperty("DataX", xVec);
    alg->setProperty("DataY", yVec);
    alg->setProperty("NSpec", 3);
    alg->setProperty("UnitX", "Wavelength");
    alg->execute();
  }

  void addSampleMaterialToWorkspace(const std::string &wsname) {
    Algorithm_sptr alg = AlgorithmManager::Instance().createUnmanaged("SetSampleMaterial");
    alg->initialize();
    alg->setProperty("InputWorkspace", wsname);
    alg->setProperty("ChemicalFormula", "Si");
    alg->setProperty("SampleNumberDensity", 0.1);
    alg->execute();
  }

  // FIN
};
