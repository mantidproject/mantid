// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidTestHelpers/WorkspaceCreationHelper.h"
#include <cxxtest/TestSuite.h>

#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/Axis.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidKernel/UnitFactory.h"
#include "MantidReflectometry/ConvertSingleSpectrumLambdaToQ.h"

using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace Mantid::Reflectometry;
using namespace Mantid::DataObjects;

namespace {
/// Create a single-spectrum lambda workspace and register in the ADS
void setUpSingleSpectrumLambdaWs(std::string &inputWs) {
  const auto nspecs{1};
  const auto nbins{4};
  auto space2D = createWorkspace<Workspace2D>(nspecs, nbins + 1, nbins);
  space2D->getAxis(0)->unit() = UnitFactory::Instance().create("Wavelength");
  AnalysisDataService::Instance().addOrReplace(inputWs, space2D);
}

/// Create a multi-spectra lambda workspace and register in the ADS
void setUpMultiSpectraLambdaWs(std::string &inputWs) {
  const auto nspecs{3};
  const auto nbins{4};
  auto space2D = createWorkspace<Workspace2D>(nspecs, nbins + 1, nbins);
  space2D->getAxis(0)->unit() = UnitFactory::Instance().create("Wavelength");
  AnalysisDataService::Instance().addOrReplace(inputWs, space2D);
}

/// Create a single spectrum momentum workspace and register in the ADS
void setUpSingleSpectrumMomentumWs(std::string &inputWs) {
  const auto nspecs{1};
  const auto nbins{4};
  auto space2D = createWorkspace<Workspace2D>(nspecs, nbins + 1, nbins);
  space2D->getAxis(0)->unit() =
      UnitFactory::Instance().create("MomentumTransfer");
  AnalysisDataService::Instance().addOrReplace(inputWs, space2D);
}
} // namespace

class ConvertSingleSpectrumLambdaToQTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static ConvertSingleSpectrumLambdaToQTest *createSuite() {
    return new ConvertSingleSpectrumLambdaToQTest();
  }
  static void destroySuite(ConvertSingleSpectrumLambdaToQTest *suite) {
    delete suite;
  }

  void setUp() override { inputWs = "testWorkspace"; }
  void testInit() {
    ConvertSingleSpectrumLambdaToQ alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    TS_ASSERT(alg.isInitialized());
  }

  /// Tests the execution of the algorithm with a single-spectrum lambda
  /// workspace
  void testExecSingleSpectrumLambdaInput() {
    setUpSingleSpectrumLambdaWs(inputWs);
    ConvertSingleSpectrumLambdaToQ alg;
    alg.setChild(true);
    alg.setRethrows(true);
    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    TS_ASSERT(alg.isInitialized());
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue(
        "InputWorkspace", inputWs));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue(
        "OutputWorkspace", "outputWs"));
    TS_ASSERT_THROWS_NOTHING(
        alg.setPropertyValue("ThetaIn", "1.5"));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue(
        "Target", "MomentumTransfer"));
    TS_ASSERT_THROWS_NOTHING(alg.execute());
    TS_ASSERT(alg.isExecuted());

    MatrixWorkspace_const_sptr input = alg.getProperty("InputWorkspace");

    MatrixWorkspace_const_sptr output = alg.getProperty("OutputWorkspace");

    // Check that the output unit is correct
    TS_ASSERT_EQUALS(output->getAxis(0)->unit()->unitID(), "MomentumTransfer");

    // Check that X has been correctly converted
    TS_ASSERT_DELTA(output->x(0)[0], 0.0657898467, 1e-9);
    TS_ASSERT_DELTA(output->x(0)[1], 0.0822373084, 1e-9);
    TS_ASSERT_DELTA(output->x(0)[2], 0.1096497446, 1e-9);

    // Check that Y and E were correctly reversed
    TS_ASSERT_DELTA(output->y(0)[0], input->y(0)[2], 1e-6);
    TS_ASSERT_DELTA(output->y(0)[1], input->y(0)[1], 1e-6);
    TS_ASSERT_DELTA(output->y(0)[2], input->y(0)[0], 1e-6);
    TS_ASSERT_DELTA(output->e(0)[0], input->e(0)[2], 1e-6);
    TS_ASSERT_DELTA(output->e(0)[1], input->e(0)[1], 1e-6);
    TS_ASSERT_DELTA(output->e(0)[2], input->e(0)[0], 1e-6);

    AnalysisDataService::Instance().remove("outputWs");
  }

  /// Tests the execution of the algorithm with a multi-spectra lambda
  /// workspace
  void testExecMultiSpectraLambdaInputThrows() {
    setUpMultiSpectraLambdaWs(inputWs);
    ConvertSingleSpectrumLambdaToQ alg;
    alg.setChild(true);
    alg.setRethrows(true);
    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    TS_ASSERT(alg.isInitialized());
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue(
        "InputWorkspace", inputWs));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue(
        "OutputWorkspace", "outputWs"));
    TS_ASSERT_THROWS_NOTHING(
        alg.setPropertyValue("ThetaIn", "1.5"));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue(
        "Target", "MomentumTransfer"));
    TS_ASSERT_THROWS_ANYTHING(alg.execute());
    TS_ASSERT(!alg.isExecuted());

    MatrixWorkspace_const_sptr input = alg.getProperty("InputWorkspace");

    MatrixWorkspace_const_sptr output = alg.getProperty("OutputWorkspace");
    std::cout << output->getName();

    // Check that the unit remains unchanged
    TS_ASSERT_EQUALS(input->getAxis(0)->unit()->unitID(), "Wavelength");
  }

  /// Tests the execution of the algorithm with a single-spectrum momentum
  /// workspace
  void testExecSingleSpectrumMomentumInputThrows() {
    setUpSingleSpectrumMomentumWs(inputWs);
    ConvertSingleSpectrumLambdaToQ alg;
    alg.setChild(true);
    alg.setRethrows(true);
    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    TS_ASSERT(alg.isInitialized());
    TS_ASSERT_THROWS(alg.setPropertyValue(
                         "InputWorkspace", inputWs),
                     std::invalid_argument);
    TS_ASSERT_THROWS_NOTHING(
        alg.setPropertyValue("OutputWorkspace", "outputWs"));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("ThetaIn", "1.5"));
    TS_ASSERT_THROWS_NOTHING(
        alg.setPropertyValue("Target", "MomentumTransfer"));
    TS_ASSERT_THROWS_ANYTHING(alg.execute());
    TS_ASSERT(!alg.isExecuted());

    MatrixWorkspace_const_sptr input = alg.getProperty("InputWorkspace");

    //TS_ASSERT_THROWS_ANYTHING(MatrixWorkspace_const_sptr output =
    //                             alg.getProperty("OutputWorkspace"));

    // Check that the unit remains unchanged
    TS_ASSERT_EQUALS(input->getAxis(0)->unit()->unitID(), "MomentumTransfer");
  }

private:
  std::string inputWs;
  std::string outputWs;
};

// class ConvertSingleSpectrumLambdaToQTestPerformance
//    : public CxxTest::TestSuite {
// public:
//  void setUp() override {
//    inputWs = "testWorkspace";
//    setUpSingleSpectrumLambdaWs(inputWs);
//  }
//
//  void tearDown() override {
//    AnalysisDataService::Instance().remove("outputWs");
//  }
//
//  void
//  test_exec_single_spectrum_lambda_input() {
//    alg.initialize();
//    alg.setPropertyValue("InputWorkspace", "inputWs");
//    alg.setPropertyValue("OutputWorkspace", "outputWs");
//    alg.setPropertyValue("ThetaIn", "1.5");
//    alg.setPropertyValue("Target", "MomentumTransfer");
//    TS_ASSERT_THROWS_NOTHING(alg.execute());
//  }
//
// private:
//  ConvertSingleSpectrumLambdaToQ alg;
//  std::string inputWs;
//  std::string outputWs;
//};
