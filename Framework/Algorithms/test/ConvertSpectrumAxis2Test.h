// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef CONVERTSPECTRUMAXIS2TEST_H_
#define CONVERTSPECTRUMAXIS2TEST_H_

#include "MantidAlgorithms/ConvertSpectrumAxis2.h"
#include <cxxtest/TestSuite.h>

#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/Axis.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/SpectrumInfo.h"
#include "MantidAlgorithms/CreateSampleWorkspace.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidKernel/Unit.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"

using namespace Mantid::API;
using namespace Mantid::HistogramData::detail;

class ConvertSpectrumAxis2Test : public CxxTest::TestSuite {
private:
  void do_algorithm_run(std::string target, std::string inputWS,
                        std::string outputWS, bool startYNegative = true,
                        bool isHistogram = true) {
    auto testWS = WorkspaceCreationHelper::create2DWorkspaceWithFullInstrument(
        3, 1, false, startYNegative, isHistogram);
    AnalysisDataService::Instance().addOrReplace(inputWS, testWS);

    Mantid::Algorithms::ConvertSpectrumAxis2 conv;

    conv.initialize();

    TS_ASSERT_THROWS_NOTHING(conv.setPropertyValue("InputWorkspace", inputWS));
    TS_ASSERT_THROWS_NOTHING(
        conv.setPropertyValue("OutputWorkspace", outputWS));
    TS_ASSERT_THROWS_NOTHING(conv.setPropertyValue("Target", target));
    TS_ASSERT_THROWS_NOTHING(conv.setPropertyValue("EFixed", "10.0"));

    TS_ASSERT_THROWS_NOTHING(conv.execute());
    TS_ASSERT(conv.isExecuted());
  }

  void check_output_values_for_signed_theta_conversion(
      std::string outputWSSignedTheta) {
    MatrixWorkspace_const_sptr outputSignedTheta;
    TS_ASSERT_THROWS_NOTHING(
        outputSignedTheta =
            AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(
                outputWSSignedTheta));

    // Check the signed theta axes of the workspaces.
    const Axis *thetaAxis = nullptr;
    TS_ASSERT_THROWS_NOTHING(thetaAxis = outputSignedTheta->getAxis(1));
    TS_ASSERT(thetaAxis->isNumeric());

    // Check axis is correct length for the workspaces.
    TS_ASSERT_THROWS((*thetaAxis)(3),
                     const Mantid::Kernel::Exception::IndexError &);

    // Check the outputs for the workspaces are correct.
    TS_ASSERT_EQUALS(thetaAxis->unit()->caption(), "Scattering angle");
    TS_ASSERT_EQUALS(thetaAxis->unit()->label(), "degrees");
    TS_ASSERT_DELTA((*thetaAxis)(0), -1.1458, 0.0001);
    TS_ASSERT_DELTA((*thetaAxis)(1), 0.0000, 0.0001);
    TS_ASSERT_DELTA((*thetaAxis)(2), 1.1458, 0.0001);
  }

  void check_output_values_for_theta_conversion(std::string inputWSTheta,
                                                std::string outputWSTheta) {
    MatrixWorkspace_const_sptr input, output;
    TS_ASSERT_THROWS_NOTHING(
        input = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(
            inputWSTheta));
    TS_ASSERT_THROWS_NOTHING(
        output = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(
            outputWSTheta));
    // Workspaces should now have a numeric axes up the side, with units of
    // angle.
    const Axis *thetaAxis = nullptr;
    TS_ASSERT_THROWS_NOTHING(thetaAxis = output->getAxis(1));
    TS_ASSERT(thetaAxis->isNumeric());
    TS_ASSERT_EQUALS(thetaAxis->unit()->caption(), "Scattering angle");
    TS_ASSERT_EQUALS(thetaAxis->unit()->label(), "degrees");
    TS_ASSERT_DELTA((*thetaAxis)(0), 0.0000, 0.0001);
    TS_ASSERT_DELTA((*thetaAxis)(1), 1.1458, 0.0001);

    // Data in the workspaces should be swapped over.
    TS_ASSERT_EQUALS(input->x(0), output->x(2));
    TS_ASSERT_EQUALS(input->y(0), output->y(2));
    TS_ASSERT_EQUALS(input->e(0), output->e(2));
    TS_ASSERT_EQUALS(input->x(1), output->x(1));
    TS_ASSERT_EQUALS(input->y(1), output->y(1));
    TS_ASSERT_EQUALS(input->e(1), output->e(1));

    // Check workspace axes are of correct length.
    TS_ASSERT_THROWS((*thetaAxis)(3),
                     const Mantid::Kernel::Exception::IndexError &);
  }

  void clean_up_workspaces(const std::string inputWS,
                           const std::string outputWS) {
    AnalysisDataService::Instance().remove(inputWS);
    AnalysisDataService::Instance().remove(outputWS);
  }

public:
  void testName() {
    Mantid::Algorithms::ConvertSpectrumAxis2 conv;
    TS_ASSERT_EQUALS(conv.name(), "ConvertSpectrumAxis");
  }

  void testVersion() {
    Mantid::Algorithms::ConvertSpectrumAxis2 conv;
    TS_ASSERT_EQUALS(conv.version(), 2);
  }

  void testInit() {
    Mantid::Algorithms::ConvertSpectrumAxis2 conv;
    TS_ASSERT_THROWS_NOTHING(conv.initialize());
    TS_ASSERT(conv.isInitialized());
  }

  void test_Target_SignedTheta_Returns_Correct_Value() {
    const std::string inputWS("inWS");
    const std::string outputSignedThetaAxisWS("outSignedThetaWS");
    const std::string outputSignedThetaAxisWS2("outSignedThetaWS2");

    // Histogram
    do_algorithm_run("signed_theta", inputWS, outputSignedThetaAxisWS);

    // Check output values for the workspace then clean up.
    check_output_values_for_signed_theta_conversion(outputSignedThetaAxisWS);
    clean_up_workspaces(inputWS, outputSignedThetaAxisWS);

    // No histogram
    do_algorithm_run("signed_theta", inputWS, outputSignedThetaAxisWS, true,
                     false);

    // Check output values for the workspace then clean up.
    check_output_values_for_signed_theta_conversion(outputSignedThetaAxisWS);
    clean_up_workspaces(inputWS, outputSignedThetaAxisWS);

    // Histogram
    do_algorithm_run("SignedTheta", inputWS, outputSignedThetaAxisWS2);

    // Check output values for the workspace then clean up.
    check_output_values_for_signed_theta_conversion(outputSignedThetaAxisWS2);
    clean_up_workspaces(inputWS, outputSignedThetaAxisWS2);

    // No histogram
    do_algorithm_run("SignedTheta", inputWS, outputSignedThetaAxisWS2, true,
                     false);

    // Check output values for the workspace then clean up.
    check_output_values_for_signed_theta_conversion(outputSignedThetaAxisWS2);
    clean_up_workspaces(inputWS, outputSignedThetaAxisWS2);
  }

  void test_Target_Theta_Returns_Correct_Value() {
    const std::string inputWS("inWS");
    const std::string outputWS("outWS");
    const std::string outputWS2("outWS2");

    do_algorithm_run("theta", inputWS, outputWS);

    // Check output values for the workspace then clean up.
    check_output_values_for_theta_conversion(inputWS, outputWS);
    clean_up_workspaces(inputWS, outputWS);

    do_algorithm_run("Theta", inputWS, outputWS2);

    // Check output values for the workspace then clean up.
    check_output_values_for_theta_conversion(inputWS, outputWS2);
    clean_up_workspaces(inputWS, outputWS2);
  }

  void
  test_Target_ElasticQ_Throws_When_No_Efixed_Set_In_Algorithm_And_Not_In_Workspace() {
    std::string inputWS("inWS");
    const std::string outputWS("outWS");
    const std::string target("ElasticQ");

    auto testWS = WorkspaceCreationHelper::create2DWorkspaceWithFullInstrument(
        3, 1, false);
    AnalysisDataService::Instance().addOrReplace(inputWS, testWS);

    Mantid::Algorithms::ConvertSpectrumAxis2 conv;
    conv.initialize();
    conv.setRethrows(true);
    TS_ASSERT_THROWS_NOTHING(conv.setPropertyValue("InputWorkspace", inputWS));
    TS_ASSERT_THROWS_NOTHING(
        conv.setPropertyValue("OutputWorkspace", outputWS));
    TS_ASSERT_THROWS_NOTHING(conv.setPropertyValue("Target", target));

    TS_ASSERT_THROWS(conv.execute(), const std::invalid_argument &);
    TS_ASSERT(!conv.isExecuted());

    // Clean up workspaces.
    clean_up_workspaces(inputWS, outputWS);
  }

  void
  test_Target_ElasticQ_Returns_Correct_Value_When_EFixed_Is_Set_In_Algorithm() {
    std::string inputWS("inWS");
    const std::string outputWS("outWS");

    do_algorithm_run("ElasticQ", inputWS, outputWS, false);

    MatrixWorkspace_const_sptr input, output;
    TS_ASSERT_THROWS_NOTHING(
        input = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(
            inputWS));
    TS_ASSERT_THROWS_NOTHING(
        output = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(
            outputWS));

    // Should now have a numeric axis up the side, with units of Q
    const Axis *qAxis = nullptr;
    TS_ASSERT_THROWS_NOTHING(qAxis = output->getAxis(1));
    TS_ASSERT(qAxis->isNumeric());
    TS_ASSERT_EQUALS(qAxis->unit()->unitID(), "MomentumTransfer");

    TS_ASSERT_DELTA((*qAxis)(0), 0.0000, 0.0001);
    TS_ASSERT_DELTA((*qAxis)(1), 0.04394, 1.0000e-4);
    TS_ASSERT_DELTA((*qAxis)(2), 0.0878, 1.0000e-4);

    // Check axis is correct length
    TS_ASSERT_THROWS((*qAxis)(3),
                     const Mantid::Kernel::Exception::IndexError &);

    TS_ASSERT_EQUALS(input->x(0), output->x(0));
    TS_ASSERT_EQUALS(input->y(0), output->y(0));
    TS_ASSERT_EQUALS(input->e(0), output->e(0));
    TS_ASSERT_EQUALS(input->x(1), output->x(1));
    TS_ASSERT_EQUALS(input->y(1), output->y(1));
    TS_ASSERT_EQUALS(input->e(1), output->e(1));
    TS_ASSERT_EQUALS(input->x(2), output->x(2));
    TS_ASSERT_EQUALS(input->y(2), output->y(2));
    TS_ASSERT_EQUALS(input->e(2), output->e(2));

    // Clean up workspaces.
    clean_up_workspaces(inputWS, outputWS);
  }

  void
  test_Target_ElasticDSpacing_Returns_Correct_Value_When_EFixed_Is_Set_In_Algorithm() {
    const std::string inputWS("inWS");
    const std::string outputWS("outWS");

    do_algorithm_run("ElasticDSpacing", inputWS, outputWS, false);

    MatrixWorkspace_const_sptr input, output;
    TS_ASSERT_THROWS_NOTHING(
        input = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(
            inputWS));
    TS_ASSERT_THROWS_NOTHING(
        output = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(
            outputWS));

    // Should now have a numeric axis up the side, with units of d
    const Axis *qAxis = nullptr;
    TS_ASSERT_THROWS_NOTHING(qAxis = output->getAxis(1));
    TS_ASSERT(qAxis->isNumeric());
    TS_ASSERT_EQUALS(qAxis->unit()->unitID(), "dSpacing");

    TS_ASSERT_DELTA((*qAxis)(0), 71.5464, 1e-4);
    TS_ASSERT_DELTA((*qAxis)(1), 143.0286, 1e-4);
    TS_ASSERT_DELTA((*qAxis)(2), 2 * M_PI / DBL_MIN, 1e-10);

    // Check axis is correct length
    TS_ASSERT_THROWS((*qAxis)(3),
                     const Mantid::Kernel::Exception::IndexError &);

    TS_ASSERT_EQUALS(input->x(0), output->x(0));
    TS_ASSERT_EQUALS(input->y(0), output->y(0));
    TS_ASSERT_EQUALS(input->e(0), output->e(0));
    TS_ASSERT_EQUALS(input->x(1), output->x(1));
    TS_ASSERT_EQUALS(input->y(1), output->y(1));
    TS_ASSERT_EQUALS(input->e(1), output->e(1));
    TS_ASSERT_EQUALS(input->x(2), output->x(2));
    TS_ASSERT_EQUALS(input->y(2), output->y(2));
    TS_ASSERT_EQUALS(input->e(2), output->e(2));

    // Clean up workspaces.
    clean_up_workspaces(inputWS, outputWS);
  }

  void
  test_Target_ElasticQSquared_Returns_Correct_Value_When_EFixed_Is_Set_In_Algorithm() {
    std::string inputWS("inWS");
    const std::string outputWS("outWS");

    do_algorithm_run("ElasticQSquared", inputWS, outputWS, false);

    MatrixWorkspace_const_sptr input, output;
    TS_ASSERT_THROWS_NOTHING(
        input = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(
            inputWS));
    TS_ASSERT_THROWS_NOTHING(
        output = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(
            outputWS));

    // Should now have a numeric axis up the side, with units of Q^2
    const Axis *q2Axis = nullptr;
    TS_ASSERT_THROWS_NOTHING(q2Axis = output->getAxis(1));
    TS_ASSERT(q2Axis->isNumeric());
    TS_ASSERT_EQUALS(q2Axis->unit()->unitID(), "QSquared");

    TS_ASSERT_DELTA((*q2Axis)(0), 0.0000, 0.0001);
    TS_ASSERT_DELTA((*q2Axis)(1), 0.00193, 1.0000e-5);
    TS_ASSERT_DELTA((*q2Axis)(2), 0.00771, 1.0000e-5);

    // Check axis is correct length
    TS_ASSERT_THROWS((*q2Axis)(3),
                     const Mantid::Kernel::Exception::IndexError &);

    TS_ASSERT_EQUALS(input->x(0), output->x(0));
    TS_ASSERT_EQUALS(input->y(0), output->y(0));
    TS_ASSERT_EQUALS(input->e(0), output->e(0));
    TS_ASSERT_EQUALS(input->x(1), output->x(1));
    TS_ASSERT_EQUALS(input->y(1), output->y(1));
    TS_ASSERT_EQUALS(input->e(1), output->e(1));
    TS_ASSERT_EQUALS(input->x(2), output->x(2));
    TS_ASSERT_EQUALS(input->y(2), output->y(2));
    TS_ASSERT_EQUALS(input->e(2), output->e(2));

    // Clean up workspaces.
    clean_up_workspaces(inputWS, outputWS);
  }

  void
  test_Target_ElasticQ_For_Direct_Uses_Workspace_Ei_If_No_EFixed_Is_Set_In_Algorithm() {
    std::string inputWS("inWS");
    const std::string outputWS("outWS");
    const std::string target("ElasticQ");
    const std::string emode("Direct");
    const double efixed(2.5);

    // Setup a workspace which contains a value for Ei.
    auto testWS = WorkspaceCreationHelper::create2DWorkspaceWithFullInstrument(
        3, 1, false);
    auto &run = testWS->mutableRun();
    run.addProperty("Ei", efixed);
    AnalysisDataService::Instance().addOrReplace(inputWS, testWS);

    Mantid::Algorithms::ConvertSpectrumAxis2 conv;
    conv.initialize();
    TS_ASSERT_THROWS_NOTHING(conv.setPropertyValue("InputWorkspace", inputWS));
    TS_ASSERT_THROWS_NOTHING(
        conv.setPropertyValue("OutputWorkspace", outputWS));
    TS_ASSERT_THROWS_NOTHING(conv.setPropertyValue("Target", target));
    TS_ASSERT_THROWS_NOTHING(conv.setPropertyValue("EMode", emode));

    conv.execute();

    TS_ASSERT(conv.isExecuted());

    MatrixWorkspace_const_sptr input, output;
    TS_ASSERT_THROWS_NOTHING(
        input = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(
            inputWS));
    TS_ASSERT_THROWS_NOTHING(
        output = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(
            outputWS));

    // Should now have a numeric axis up the side, with units of Q
    const Axis *qAxis = nullptr;
    TS_ASSERT_THROWS_NOTHING(qAxis = output->getAxis(1));
    TS_ASSERT(qAxis->isNumeric());
    TS_ASSERT_EQUALS(qAxis->unit()->unitID(), "MomentumTransfer");

    TS_ASSERT_DELTA((*qAxis)(0), 0.000, 0.001);
    TS_ASSERT_DELTA((*qAxis)(1), 0.02196, 1.0000e-5);
    TS_ASSERT_DELTA((*qAxis)(2), 0.0439, 1.0000e-4);

    // Check axis is of correct length.
    TS_ASSERT_THROWS((*qAxis)(3),
                     const Mantid::Kernel::Exception::IndexError &);

    // Clean up workspaces.
    clean_up_workspaces(inputWS, outputWS);
  }

  void
  test_Target_ElasticQ_For_Indirect_Uses_Detector_If_No_EFixed_Is_Set_In_Algorithm() {
    std::string inputWS("inWS");
    const std::string outputWS("outWS");
    const std::string target("ElasticQ");
    const std::string emode("Indirect");

    // Setup workspace with detectors which have different values for Efixed.
    auto testWS = WorkspaceCreationHelper::create2DWorkspaceWithFullInstrument(
        3, 1, false);
    AnalysisDataService::Instance().addOrReplace(inputWS, testWS);

    auto &pmap = testWS->instrumentParameters();
    const auto &spectrumInfo = testWS->spectrumInfo();
    pmap.addDouble(&spectrumInfo.detector(0), "Efixed", 0.4);
    pmap.addDouble(&spectrumInfo.detector(1), "Efixed", 0.1);
    pmap.addDouble(&spectrumInfo.detector(2), "Efixed", 0.025);

    Mantid::Algorithms::ConvertSpectrumAxis2 conv;
    conv.initialize();

    TS_ASSERT_THROWS_NOTHING(conv.setPropertyValue("InputWorkspace", inputWS));
    TS_ASSERT_THROWS_NOTHING(
        conv.setPropertyValue("OutputWorkspace", outputWS));
    TS_ASSERT_THROWS_NOTHING(conv.setPropertyValue("Target", target));
    TS_ASSERT_THROWS_NOTHING(conv.setPropertyValue("EMode", emode));

    conv.execute();

    MatrixWorkspace_const_sptr input, output;
    TS_ASSERT_THROWS_NOTHING(
        input = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(
            inputWS));
    TS_ASSERT_THROWS_NOTHING(
        output = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(
            outputWS));

    // Should now have a numeric axis up the side, with units of Q.
    const Axis *qAxis = nullptr;
    TS_ASSERT_THROWS_NOTHING(qAxis = output->getAxis(1));
    TS_ASSERT(qAxis->isNumeric());
    TS_ASSERT_EQUALS(qAxis->unit()->unitID(), "MomentumTransfer");

    TS_ASSERT_DELTA((*qAxis)(0), 0.000, 0.001);
    TS_ASSERT_DELTA((*qAxis)(1), 0.004391, 1.0000e-6);
    TS_ASSERT_DELTA((*qAxis)(2), 0.004393, 1.0000e-6);

    // Check axis is of correct length.
    TS_ASSERT_THROWS((*qAxis)(3),
                     const Mantid::Kernel::Exception::IndexError &);

    TS_ASSERT(conv.isExecuted());

    // Clean up workspaces.
    clean_up_workspaces(inputWS, outputWS);
  }

  void test_Unordered_Axis_With_Scanned_Workspace() {
    FrameworkManager::Instance();

    Mantid::Algorithms::CreateSampleWorkspace creator;
    creator.initialize();
    creator.setChild(true);
    creator.setProperty("NumBanks", 2);
    creator.setProperty("BankPixelWidth", 1);
    creator.setProperty("XMax", 100.);
    creator.setProperty("BinWidth", 50.);
    creator.setProperty("NumScanPoints", 10);
    creator.setProperty("OutputWorkspace", "__unused");
    creator.execute();

    Mantid::Algorithms::ConvertSpectrumAxis2 testee;
    testee.initialize();
    testee.setChild(true);
    testee.setRethrows(true);
    const MatrixWorkspace_sptr ws = creator.getProperty("OutputWorkspace");
    testee.setProperty("InputWorkspace", ws);
    testee.setProperty("OrderAxis", false);
    testee.setProperty("Target", "Theta");
    testee.setProperty("OutputWorkspace", "__unused2");
    testee.execute();
    TS_ASSERT(testee.isExecuted());
    const MatrixWorkspace_sptr output = testee.getProperty("OutputWorkspace");
    TS_ASSERT(output);
    const Axis *axis = output->getAxis(1);
    TS_ASSERT(axis);
    for (size_t i = 0; i < 20; ++i) {
      TS_ASSERT_DELTA((*axis)(i), double(i % 10), 1E-10);
    }
  }

  void test_eventWS() {
    const std::string outputWS("outWS");
    const std::string target("theta");
    auto testWS =
        WorkspaceCreationHelper::createEventWorkspaceWithFullInstrument(500, 3);
    Mantid::Algorithms::ConvertSpectrumAxis2 conv;
    conv.setChild(true);
    conv.initialize();

    conv.setProperty("InputWorkspace", testWS);

    conv.setPropertyValue("OutputWorkspace", outputWS);
    conv.setPropertyValue("Target", target);
    conv.execute();
    TS_ASSERT(conv.isExecuted());
    const MatrixWorkspace_sptr output = conv.getProperty("OutputWorkspace");
    TS_ASSERT_EQUALS(output->getAxis(1)->unit()->unitID(), "Degrees");

    Mantid::DataObjects::EventWorkspace_sptr eventWS =
        boost::dynamic_pointer_cast<Mantid::DataObjects::EventWorkspace>(
            output);
    TS_ASSERT(eventWS);
  }
};

class ConvertSpectrumAxis2TestPerformance : public CxxTest::TestSuite {
public:
  static ConvertSpectrumAxis2TestPerformance *createSuite() {
    return new ConvertSpectrumAxis2TestPerformance();
  }
  static void destroySuite(ConvertSpectrumAxis2TestPerformance *suite) {
    delete suite;
  }

  ConvertSpectrumAxis2TestPerformance() {
    FrameworkManager::Instance();
    m_testWS = WorkspaceCreationHelper::create2DWorkspaceWithFullInstrument(
        20000, 20000);
    m_creator.initialize();
    m_creator.setChild(true);
    m_creator.setProperty("NumBanks", 100);
    m_creator.setProperty("BankPixelWidth", 10);
    m_creator.setProperty("XMax", 100.);
    m_creator.setProperty("BinWidth", 1.);
    m_creator.setProperty("NumScanPoints", 100);
    m_creator.setProperty("OutputWorkspace", "__unused");
    m_creator.execute();
  }

  void test_conversion_to_signed_theta_with_many_entries() {
    Mantid::Algorithms::ConvertSpectrumAxis2 conv;
    conv.initialize();
    conv.setChild(true);
    conv.setProperty("InputWorkspace", m_testWS);
    conv.setPropertyValue("OutputWorkspace", "outputWS");
    conv.setPropertyValue("Target", "SignedTheta");
    conv.setPropertyValue("EFixed", "10.0");
    conv.execute();
  }

  void test_large_scanning_workspace() {
    Mantid::Algorithms::ConvertSpectrumAxis2 conv;
    conv.initialize();
    conv.setChild(true);
    const MatrixWorkspace_sptr ws = m_creator.getProperty("OutputWorkspace");
    conv.setProperty("InputWorkspace", ws);
    conv.setPropertyValue("Target", "Theta");
    conv.setPropertyValue("OutputWorkspace", "outputWS");
    conv.execute();
  }

private:
  MatrixWorkspace_sptr m_testWS;
  Mantid::Algorithms::CreateSampleWorkspace m_creator;
};

#endif /*CONVERTSPECTRUMAXIS2TEST_H_*/
