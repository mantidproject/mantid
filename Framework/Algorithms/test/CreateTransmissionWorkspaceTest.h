// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
/*
 * CreateTransmissionWorkspaceTest.h
 *
 *  Created on: Jul 29, 2014
 *      Author: spu92482
 */

#ifndef ALGORITHMS_TEST_CREATETRANSMISSIONWORKSPACETEST_H_
#define ALGORITHMS_TEST_CREATETRANSMISSIONWORKSPACETEST_H_

#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/Axis.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidGeometry/Instrument/ReferenceFrame.h"
#include "MantidKernel/Unit.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"
#include <algorithm>
#include <cxxtest/TestSuite.h>

using namespace Mantid;
using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace WorkspaceCreationHelper;

class CreateTransmissionWorkspaceTest : public CxxTest::TestSuite {
private:
  MatrixWorkspace_sptr m_tinyReflWS;
  MatrixWorkspace_sptr m_TOF;
  MatrixWorkspace_sptr m_NotTOF;

private:
  IAlgorithm_sptr construct_standard_algorithm() {
    auto alg = AlgorithmManager::Instance().create(
        "CreateTransmissionWorkspaceAuto", 1);
    alg->initialize();
    alg->setChild(true);
    alg->setProperty("FirstTransmissionRun", m_TOF);
    alg->setProperty("WavelengthMin", 0.0);
    alg->setProperty("WavelengthMax", 1.0);
    alg->setProperty("I0MonitorIndex", 0);
    alg->setPropertyValue("ProcessingInstructions", "0, 1");
    alg->setProperty("MonitorBackgroundWavelengthMin", 0.0);
    alg->setProperty("MonitorBackgroundWavelengthMax", 1.0);
    alg->setProperty("MonitorIntegrationWavelengthMin", 0.0);
    alg->setProperty("MonitorIntegrationWavelengthMax", 1.0);
    alg->setPropertyValue("OutputWorkspace", "demo_ws");
    alg->setRethrows(true);
    return alg;
  }

public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static CreateTransmissionWorkspaceTest *createSuite() {
    return new CreateTransmissionWorkspaceTest();
  }
  static void destroySuite(CreateTransmissionWorkspaceTest *suite) {
    delete suite;
  }

  CreateTransmissionWorkspaceTest() {
    m_tinyReflWS = create2DWorkspaceWithReflectometryInstrument();

    FrameworkManager::Instance();
    MantidVec xData = {0, 1, 2, 3};
    MantidVec yData = {0, 0, 0};

    auto createWorkspace =
        AlgorithmManager::Instance().createUnmanaged("CreateWorkspace");
    createWorkspace->setChild(true);
    createWorkspace->initialize();
    createWorkspace->setProperty("UnitX", "1/q");
    createWorkspace->setProperty("DataX", xData);
    createWorkspace->setProperty("DataY", yData);
    createWorkspace->setProperty("NSpec", 1);
    createWorkspace->setPropertyValue("OutputWorkspace", "UnitWS");
    createWorkspace->execute();
    m_NotTOF = createWorkspace->getProperty("OutputWorkspace");

    createWorkspace->setProperty("UnitX", "TOF");
    createWorkspace->setProperty("DataX", xData);
    createWorkspace->setProperty("DataY", yData);
    createWorkspace->setProperty("NSpec", 1);
    createWorkspace->execute();
    m_TOF = createWorkspace->getProperty("OutputWorkspace");
  }

  void test_check_first_transmission_workspace_not_tof_or_wavelength_throws() {
    auto alg = construct_standard_algorithm();
    TS_ASSERT_THROWS(alg->setProperty("FirstTransmissionRun", m_NotTOF),
                     const std::invalid_argument &);
  }

  void test_check_second_transmission_workspace_not_tof_throws() {
    auto alg = construct_standard_algorithm();
    TS_ASSERT_THROWS(alg->setProperty("SecondTransmissionRun", m_NotTOF),
                     const std::invalid_argument &);
  }

  void test_end_overlap_must_be_greater_than_start_overlap_or_throw() {
    auto alg = construct_standard_algorithm();
    alg->setProperty("FirstTransmissionRun", m_TOF);
    alg->setProperty("SecondTransmissionRun", m_TOF);
    MantidVec params = {0.0, 0.1, 1.0};
    alg->setProperty("Params", params);
    alg->setProperty("StartOverlap", 0.6);
    alg->setProperty("EndOverlap", 0.4);
    TS_ASSERT_THROWS(alg->execute(), const std::invalid_argument &);
  }

  void test_must_provide_wavelengths() {
    auto alg =
        AlgorithmManager::Instance().create("CreateTransmissionWorkspace", 1);
    alg->initialize();
    alg->setChild(true);
    alg->setProperty("FirstTransmissionRun", m_TOF);
    alg->setProperty("SecondTransmissionRun", m_TOF);
    alg->setPropertyValue("OutputWorkspace", "demo_ws");
    alg->setRethrows(true);
    TS_ASSERT_THROWS(alg->execute(), const std::runtime_error &);

    alg->setProperty("FirstTransmissionRun", m_TOF);
    alg->setProperty("SecondTransmissionRun", m_TOF);
    alg->setProperty("WavelengthMin", 1.0);
    alg->setRethrows(true);
    TS_ASSERT_THROWS(alg->execute(), const std::runtime_error &);
  }

  void test_wavelength_min_greater_wavelength_max_throws() {
    auto alg = construct_standard_algorithm();
    alg->setProperty("WavelengthMin", 1.0);
    alg->setProperty("WavelengthMax", 0.0);
    TS_ASSERT_THROWS(alg->execute(), const std::invalid_argument &);
  }

  void
  test_monitor_background_wavelength_min_greater_monitor_background_wavelength_max_throws() {
    auto alg = construct_standard_algorithm();
    alg->setProperty("MonitorBackgroundWavelengthMin", 1.0);
    alg->setProperty("MonitorBackgroundWavelengthMax", 0.0);
    TS_ASSERT_THROWS(alg->execute(), const std::invalid_argument &);
  }

  void
  test_monitor_integration_wavelength_min_greater_monitor_integration_wavelength_max_throws() {
    auto alg = construct_standard_algorithm();
    alg->setProperty("MonitorIntegrationWavelengthMin", 1.0);
    alg->setProperty("MonitorIntegrationWavelengthMax", 0.0);
    TS_ASSERT_THROWS(alg->execute(), const std::invalid_argument &);
  }

  void test_execute_one_tranmission() {

    IAlgorithm_sptr alg =
        AlgorithmManager::Instance().create("CreateTransmissionWorkspace", 1);

    alg->setChild(true);
    alg->initialize();

    alg->setProperty("FirstTransmissionRun", m_tinyReflWS);
    alg->setProperty("WavelengthMin", 1.0);
    alg->setProperty("WavelengthMax", 15.0);
    alg->setProperty("I0MonitorIndex", 1);
    alg->setProperty("MonitorBackgroundWavelengthMin", 14.0);
    alg->setProperty("MonitorBackgroundWavelengthMax", 15.0);
    alg->setProperty("MonitorIntegrationWavelengthMin", 4.0);
    alg->setProperty("MonitorIntegrationWavelengthMax", 10.0);
    alg->setPropertyValue("ProcessingInstructions", "0");
    alg->setPropertyValue("OutputWorkspace", "demo_ws");
    alg->execute();

    MatrixWorkspace_sptr outWS = alg->getProperty("OutputWorkspace");
    TS_ASSERT_EQUALS("Wavelength", outWS->getAxis(0)->unit()->unitID());
  }
};

#endif /* ALGORITHMS_TEST_CREATETRANSMISSIONWORKSPACETEST_H_ */
