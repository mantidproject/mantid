#ifndef CONVERTSPECTRUMAXIS2TEST_H_
#define CONVERTSPECTRUMAXIS2TEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidTestHelpers/WorkspaceCreationHelper.h"
#include "MantidAlgorithms/ConvertSpectrumAxis2.h"
#include "MantidDataHandling/LoadRaw3.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidKernel/UnitFactory.h"

using namespace Mantid::API;

class ConvertSpectrumAxis2Test : public CxxTest::TestSuite {
private:
  void do_algorithm_run(std::string target, std::string inputWS,
                        std::string outputWS, bool startYNegative = true) {
    auto testWS = WorkspaceCreationHelper::create2DWorkspaceWithFullInstrument(
        3, 1, false, startYNegative);
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
    const Axis *thetaAxis = 0;
    TS_ASSERT_THROWS_NOTHING(thetaAxis = outputSignedTheta->getAxis(1));
    TS_ASSERT(thetaAxis->isNumeric());

    // Check axis is correct length for the workspaces.
    TS_ASSERT_THROWS((*thetaAxis)(3), Mantid::Kernel::Exception::IndexError);

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
    const Axis *thetaAxis = 0;
    TS_ASSERT_THROWS_NOTHING(thetaAxis = output->getAxis(1));
    TS_ASSERT(thetaAxis->isNumeric());
    TS_ASSERT_EQUALS(thetaAxis->unit()->caption(), "Scattering angle");
    TS_ASSERT_EQUALS(thetaAxis->unit()->label(), "degrees");
    TS_ASSERT_DELTA((*thetaAxis)(0), 0.0000, 0.0001);
    TS_ASSERT_DELTA((*thetaAxis)(1), 1.1458, 0.0001);

    // Data in the workspaces should be swapped over.
    TS_ASSERT_EQUALS(input->readX(0), output->readX(2));
    TS_ASSERT_EQUALS(input->readY(0), output->readY(2));
    TS_ASSERT_EQUALS(input->readE(0), output->readE(2));
    TS_ASSERT_EQUALS(input->readX(1), output->readX(1));
    TS_ASSERT_EQUALS(input->readY(1), output->readY(1));
    TS_ASSERT_EQUALS(input->readE(1), output->readE(1));

    // Check workspace axes are of correct length.
    TS_ASSERT_THROWS((*thetaAxis)(3), Mantid::Kernel::Exception::IndexError);
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

    do_algorithm_run("signed_theta", inputWS, outputSignedThetaAxisWS);

    // Check output values for the workspace then clean up.
    check_output_values_for_signed_theta_conversion(outputSignedThetaAxisWS);
    clean_up_workspaces(inputWS, outputSignedThetaAxisWS);

    do_algorithm_run("SignedTheta", inputWS, outputSignedThetaAxisWS2);

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
        3, 1, false, true);
    AnalysisDataService::Instance().addOrReplace(inputWS, testWS);

    Mantid::Algorithms::ConvertSpectrumAxis2 conv;
    conv.initialize();
    conv.setRethrows(true);
    TS_ASSERT_THROWS_NOTHING(conv.setPropertyValue("InputWorkspace", inputWS));
    TS_ASSERT_THROWS_NOTHING(
        conv.setPropertyValue("OutputWorkspace", outputWS));
    TS_ASSERT_THROWS_NOTHING(conv.setPropertyValue("Target", target));

    TS_ASSERT_THROWS(conv.execute(), std::invalid_argument);
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
    const Axis *qAxis = 0;
    TS_ASSERT_THROWS_NOTHING(qAxis = output->getAxis(1));
    TS_ASSERT(qAxis->isNumeric());
    TS_ASSERT_EQUALS(qAxis->unit()->unitID(), "MomentumTransfer");

    TS_ASSERT_DELTA((*qAxis)(0), 0.0000, 0.0001);
    TS_ASSERT_DELTA((*qAxis)(1), 0.04394, 1.0000e-4);
    TS_ASSERT_DELTA((*qAxis)(2), 0.0878, 1.0000e-4);

    // Check axis is correct length
    TS_ASSERT_THROWS((*qAxis)(3), Mantid::Kernel::Exception::IndexError);

    TS_ASSERT_EQUALS(input->readX(0), output->readX(0));
    TS_ASSERT_EQUALS(input->readY(0), output->readY(0));
    TS_ASSERT_EQUALS(input->readE(0), output->readE(0));
    TS_ASSERT_EQUALS(input->readX(1), output->readX(1));
    TS_ASSERT_EQUALS(input->readY(1), output->readY(1));
    TS_ASSERT_EQUALS(input->readE(1), output->readE(1));
    TS_ASSERT_EQUALS(input->readX(2), output->readX(2));
    TS_ASSERT_EQUALS(input->readY(2), output->readY(2));
    TS_ASSERT_EQUALS(input->readE(2), output->readE(2));

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
    const Axis *q2Axis = 0;
    TS_ASSERT_THROWS_NOTHING(q2Axis = output->getAxis(1));
    TS_ASSERT(q2Axis->isNumeric());
    TS_ASSERT_EQUALS(q2Axis->unit()->unitID(), "QSquared");

    TS_ASSERT_DELTA((*q2Axis)(0), 0.0000, 0.0001);
    TS_ASSERT_DELTA((*q2Axis)(1), 0.00193, 1.0000e-5);
    TS_ASSERT_DELTA((*q2Axis)(2), 0.00771, 1.0000e-5);

    // Check axis is correct length
    TS_ASSERT_THROWS((*q2Axis)(3), Mantid::Kernel::Exception::IndexError);

    TS_ASSERT_EQUALS(input->readX(0), output->readX(0));
    TS_ASSERT_EQUALS(input->readY(0), output->readY(0));
    TS_ASSERT_EQUALS(input->readE(0), output->readE(0));
    TS_ASSERT_EQUALS(input->readX(1), output->readX(1));
    TS_ASSERT_EQUALS(input->readY(1), output->readY(1));
    TS_ASSERT_EQUALS(input->readE(1), output->readE(1));
    TS_ASSERT_EQUALS(input->readX(2), output->readX(2));
    TS_ASSERT_EQUALS(input->readY(2), output->readY(2));
    TS_ASSERT_EQUALS(input->readE(2), output->readE(2));

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
    const Axis *qAxis = 0;
    TS_ASSERT_THROWS_NOTHING(qAxis = output->getAxis(1));
    TS_ASSERT(qAxis->isNumeric());
    TS_ASSERT_EQUALS(qAxis->unit()->unitID(), "MomentumTransfer");

    TS_ASSERT_DELTA((*qAxis)(0), 0.000, 0.001);
    TS_ASSERT_DELTA((*qAxis)(1), 0.02196, 1.0000e-5);
    TS_ASSERT_DELTA((*qAxis)(2), 0.0439, 1.0000e-4);

    // Check axis is of correct length.
    TS_ASSERT_THROWS((*qAxis)(3), Mantid::Kernel::Exception::IndexError);

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

    auto det0 = testWS->getDetector(0);
    pmap.addDouble(det0.get(), "Efixed", 0.4);

    auto det1 = testWS->getDetector(1);
    pmap.addDouble(det1.get(), "Efixed", 0.1);

    auto det2 = testWS->getDetector(2);
    pmap.addDouble(det2.get(), "Efixed", 0.025);

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
    const Axis *qAxis = 0;
    TS_ASSERT_THROWS_NOTHING(qAxis = output->getAxis(1));
    TS_ASSERT(qAxis->isNumeric());
    TS_ASSERT_EQUALS(qAxis->unit()->unitID(), "MomentumTransfer");

    TS_ASSERT_DELTA((*qAxis)(0), 0.000, 0.001);
    TS_ASSERT_DELTA((*qAxis)(1), 0.004391, 1.0000e-6);
    TS_ASSERT_DELTA((*qAxis)(2), 0.004393, 1.0000e-6);

    // Check axis is of correct length.
    TS_ASSERT_THROWS((*qAxis)(3), Mantid::Kernel::Exception::IndexError);

    TS_ASSERT(conv.isExecuted());

    // Clean up workspaces.
    clean_up_workspaces(inputWS, outputWS);
  }
};

#endif /*CONVERTSPECTRUMAXIS2TEST_H_*/
