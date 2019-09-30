// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef POWERTEST_H_
#define POWERTEST_H_

#include <cmath>
#include <cxxtest/TestSuite.h>

#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/WorkspaceOpOverloads.h"
#include "MantidAPI/WorkspaceProperty.h"
#include "MantidAlgorithms/Power.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"

using namespace Mantid::API;
using namespace Mantid::Kernel;
using namespace Mantid::Algorithms;
using namespace Mantid::DataObjects;

class PowerTest : public CxxTest::TestSuite {
public:
  void testName() {
    Mantid::Algorithms::Power power;
    TSM_ASSERT_EQUALS("Algorithm name should be Power", power.name(), "Power")
  }

  void testVersion() {
    Mantid::Algorithms::Power power;
    TSM_ASSERT_EQUALS("Expected version is 1", power.version(), 1)
  }

  void testInit() {
    Mantid::Algorithms::Power power;
    TS_ASSERT_THROWS_NOTHING(power.initialize())
    TS_ASSERT(power.isInitialized())

    const std::vector<Property *> props = power.getProperties();
    TSM_ASSERT_EQUALS(
        "There should only be 3 properties for this power algorithm",
        props.size(), 3);

    TS_ASSERT_EQUALS(props[0]->name(), "InputWorkspace")
    TS_ASSERT(props[0]->isDefault())
    TS_ASSERT(dynamic_cast<WorkspaceProperty<MatrixWorkspace> *>(props[0]))

    TS_ASSERT_EQUALS(props[1]->name(), "OutputWorkspace")
    TS_ASSERT(props[1]->isDefault())
    TS_ASSERT(dynamic_cast<WorkspaceProperty<MatrixWorkspace> *>(props[1]))

    TS_ASSERT_EQUALS(props[2]->name(), "Exponent")
    TS_ASSERT(props[2]->isDefault())
    TS_ASSERT(dynamic_cast<PropertyWithValue<double> *>(props[2]))
  }

  void testSetProperties() {
    WorkspaceSingleValue_sptr baseWs =
        WorkspaceCreationHelper::createWorkspaceSingleValue(2);
    AnalysisDataService::Instance().add("InputWS", baseWs);

    Power power;
    power.initialize();

    TSM_ASSERT_THROWS_NOTHING(
        "InputWorkspace should be settable",
        power.setPropertyValue("InputWorkspace", "InputWS"))
    TSM_ASSERT_THROWS_NOTHING(
        "OutputWorkspace should be settable",
        power.setPropertyValue("OutputWorkspace", "WSCor"))
    TSM_ASSERT_THROWS_NOTHING("Exponent should be settable",
                              power.setPropertyValue("Exponent", "2.0"))

    AnalysisDataService::Instance().remove("InputWS");
    AnalysisDataService::Instance().remove("WSCor");
  }

  void testNonNumericExponent() {
    Power power;
    power.initialize();
    TSM_ASSERT_THROWS("Exponent cannot be non-numeric",
                      power.setPropertyValue("Exponent", "x"),
                      const std::invalid_argument &)
  }

  void testNegativeExponent() {
    Power power;
    power.initialize();
    TSM_ASSERT_THROWS_NOTHING("Negative exponents are allowed.",
                              power.setPropertyValue("Exponent", "-1"))
  }

  void testdefaultExponent() {
    Power power;
    power.initialize();
    std::string sz_InitalValue = power.getPropertyValue("Exponent");
    TSM_ASSERT_EQUALS("The default exponent value should be 1", "1",
                      sz_InitalValue);
  }

  void testPowerCalculation() {
    WorkspaceSingleValue_sptr baseWs =
        WorkspaceCreationHelper::createWorkspaceSingleValue(2);
    AnalysisDataService::Instance().add("InputWS", baseWs);

    Power power;
    power.initialize();

    power.setPropertyValue("InputWorkspace", "InputWS");
    power.setPropertyValue("OutputWorkspace", "WSCor");
    power.setPropertyValue("Exponent", "2.0");

    power.execute();
    TSM_ASSERT("The Power algorithm did not finish executing",
               power.isExecuted());

    WorkspaceSingleValue_sptr output =
        AnalysisDataService::Instance().retrieveWS<WorkspaceSingleValue>(
            "WSCor");

    Mantid::MantidVec expectedValue(1, 4);
    TSM_ASSERT_EQUALS("Power has not been determined correctly", expectedValue,
                      output->dataY(0));

    AnalysisDataService::Instance().remove("InputWS");
    AnalysisDataService::Instance().remove("WSCor");
  }

  void testPowerCalculationWithNegativeExponent() {
    WorkspaceSingleValue_sptr baseWs =
        WorkspaceCreationHelper::createWorkspaceSingleValue(2);
    AnalysisDataService::Instance().add("InputWS", baseWs);

    Power power;
    power.initialize();

    power.setPropertyValue("InputWorkspace", "InputWS");
    power.setPropertyValue("OutputWorkspace", "WSCor");
    power.setPropertyValue("Exponent", "-2.0");

    power.execute();
    TSM_ASSERT("The Power algorithm did not finish executing",
               power.isExecuted());

    WorkspaceSingleValue_sptr output =
        AnalysisDataService::Instance().retrieveWS<WorkspaceSingleValue>(
            "WSCor");

    Mantid::MantidVec expectedValue(1, 0.25);
    TSM_ASSERT_EQUALS("Power has not been determined correctly", expectedValue,
                      output->dataY(0));

    Mantid::MantidVec expectedError(1, 0.35355);
    TS_ASSERT_DELTA(0.353553391, output->dataE(0)[0], 0.001);

    AnalysisDataService::Instance().remove("InputWS");
    AnalysisDataService::Instance().remove("WSCor");
  }

  void testPowerErrorCalculation() {
    // Workspace creation helper creates input error as sqrt of input value. So
    // input error = 2.

    // if x = p ^ y, then err_x = y * x * err_p / p

    WorkspaceSingleValue_sptr baseWs =
        WorkspaceCreationHelper::createWorkspaceSingleValue(4);
    AnalysisDataService::Instance().add("InputWS", baseWs);

    Power power;
    power.initialize();

    power.setPropertyValue("InputWorkspace", "InputWS");
    power.setPropertyValue("OutputWorkspace", "WSCor");
    power.setPropertyValue("Exponent", "2.0");

    power.execute();

    WorkspaceSingleValue_sptr output =
        AnalysisDataService::Instance().retrieveWS<WorkspaceSingleValue>(
            "WSCor");

    Mantid::MantidVec expectedError(1, 16);
    TSM_ASSERT_EQUALS("Error has not been determined correctly", expectedError,
                      output->dataE(0));

    AnalysisDataService::Instance().remove("InputWS");
    AnalysisDataService::Instance().remove("WSCor");
  }

  void testEvents() {
    // evin has 0 events per bin in pixel0, 1 in pixel 1, 2 in pixel2, ...
    EventWorkspace_sptr evin = WorkspaceCreationHelper::createEventWorkspace(
                            5, 3, 1000, 0, 1, 4),
                        evout;
    AnalysisDataService::Instance().add("test_ev_pow", evin);

    Power alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    TS_ASSERT(alg.isInitialized());
    TS_ASSERT_THROWS_NOTHING(
        alg.setPropertyValue("InputWorkspace", "test_ev_pow"));
    TS_ASSERT_THROWS_NOTHING(
        alg.setPropertyValue("OutputWorkspace", "test_ev_pow_out"));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("Exponent", "2.0"));
    alg.execute();
    TS_ASSERT(alg.isExecuted());

    TS_ASSERT_THROWS_NOTHING(
        evout = boost::dynamic_pointer_cast<EventWorkspace>(
            AnalysisDataService::Instance().retrieve("test_ev_pow_out")));

    TS_ASSERT(!evout); // should not be an event workspace

    MatrixWorkspace_sptr histo_out;
    TS_ASSERT_THROWS_NOTHING(
        histo_out = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(
            "test_ev_pow_out"));
    TS_ASSERT(histo_out); // this should be a 2d workspace

    for (size_t i = 0; i < 5; ++i) {
      TS_ASSERT_DELTA(histo_out->readY(i)[0], static_cast<double>(i * i),
                      1e-10);
    }
    AnalysisDataService::Instance().remove("test_pow_log");
    AnalysisDataService::Instance().remove("test_ev_pow_out");
  }
};

#endif
