// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <cxxtest/TestSuite.h>

#include "MantidAlgorithms/SetInstrumentParameter.h"
#include "MantidFrameworkTestHelpers/WorkspaceCreationHelper.h"
#include "MantidGeometry/Instrument.h"
#include "MantidGeometry/Instrument/DetectorInfo.h"
#include <string>

using Mantid::Algorithms::SetInstrumentParameter;

using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace Mantid::Algorithms;

class SetInstrumentParameterTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static SetInstrumentParameterTest *createSuite() { return new SetInstrumentParameterTest(); }
  static void destroySuite(SetInstrumentParameterTest *suite) { delete suite; }

  void test_Init() {
    SetInstrumentParameter alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
  }

  void test_cmpt_string_value() {
    std::string cmptName = "samplePos";
    std::string detList = "";
    std::string paramName = "TestParam";
    std::string paramValue = "Left";

    MatrixWorkspace_sptr ws = WorkspaceCreationHelper::create2DWorkspaceWithFullInstrument(3, 3);
    ExecuteAlgorithm(ws, cmptName, detList, paramName, paramValue);

    auto cmpt = ws->getInstrument()->getComponentByName(cmptName);
    TS_ASSERT_EQUALS(paramValue, cmpt->getStringParameter(paramName)[0]);
  }

  void test_default_cmpt_string_value() {
    std::string cmptName = "";
    std::string detList = "";
    std::string paramName = "TestParam";
    std::string paramValue = "Left";

    MatrixWorkspace_sptr ws = WorkspaceCreationHelper::create2DWorkspaceWithFullInstrument(3, 3);
    ExecuteAlgorithm(ws, cmptName, detList, paramName, paramValue);

    TS_ASSERT_EQUALS(paramValue, ws->getInstrument()->getStringParameter(paramName)[0]);
  }

  void test_detlist_string_value() {
    std::string cmptName = "a value to ignore";
    std::string detList = "1,2";
    std::string paramName = "TestParam";
    std::string paramValue = "Left";

    MatrixWorkspace_sptr ws = WorkspaceCreationHelper::create2DWorkspaceWithFullInstrument(3, 3);
    ExecuteAlgorithm(ws, cmptName, detList, paramName, paramValue);

    const auto &detectorInfo = ws->detectorInfo();
    const auto index1 = detectorInfo.indexOf(1);
    const auto &cmpt1 = detectorInfo.detector(index1);
    TS_ASSERT_EQUALS(paramValue, cmpt1.getStringParameter(paramName)[0]);
    const auto index2 = detectorInfo.indexOf(2);
    const auto &cmpt2 = detectorInfo.detector(index2);
    TS_ASSERT_EQUALS(paramValue, cmpt2.getStringParameter(paramName)[0]);
  }

  void test_cmpt_int_value() {
    std::string cmptName = "samplePos";
    std::string detList = "";
    std::string paramName = "TestParam";
    std::string paramType = "Number";
    std::string paramValue = "1";

    MatrixWorkspace_sptr ws = WorkspaceCreationHelper::create2DWorkspaceWithFullInstrument(3, 3);
    ExecuteAlgorithm(ws, cmptName, detList, paramName, paramValue, paramType);

    auto cmpt = ws->getInstrument()->getComponentByName(cmptName);
    TS_ASSERT_EQUALS(1, cmpt->getIntParameter(paramName)[0]);
  }

  void test_cmpt_dbl_value() {
    std::string cmptName = "samplePos";
    std::string detList = "";
    std::string paramName = "TestParam";
    std::string paramType = "Number";
    std::string paramValue = "1.12";

    MatrixWorkspace_sptr ws = WorkspaceCreationHelper::create2DWorkspaceWithFullInstrument(3, 3);
    ExecuteAlgorithm(ws, cmptName, detList, paramName, paramValue, paramType);

    auto cmpt = ws->getInstrument()->getComponentByName(cmptName);
    TS_ASSERT_EQUALS(1.12, cmpt->getNumberParameter(paramName)[0]);
  }

  void test_overwrite_dbl_value() {
    std::string cmptName = "samplePos";
    std::string detList = "";
    std::string paramName = "TestParam";
    std::string paramType = "Number";
    std::string paramValue = "1.12";
    std::string paramValue2 = "3.22";

    MatrixWorkspace_sptr ws = WorkspaceCreationHelper::create2DWorkspaceWithFullInstrument(3, 3);
    ExecuteAlgorithm(ws, cmptName, detList, paramName, paramValue, paramType);

    auto cmpt = ws->getInstrument()->getComponentByName(cmptName);
    TS_ASSERT_EQUALS(1.12, cmpt->getNumberParameter(paramName)[0]);

    ExecuteAlgorithm(ws, cmptName, detList, paramName, paramValue2, paramType);

    cmpt = ws->getInstrument()->getComponentByName(cmptName);
    TS_ASSERT_EQUALS(3.22, cmpt->getNumberParameter(paramName)[0]);
  }

  void test_overwrite_diff_type() {
    std::string cmptName = "samplePos";
    std::string detList = "";
    std::string paramName = "TestParam";
    std::string paramType = "Number";
    std::string paramValue = "1.12";
    std::string paramType2 = "String";
    std::string paramValue2 = "A String";

    MatrixWorkspace_sptr ws = WorkspaceCreationHelper::create2DWorkspaceWithFullInstrument(3, 3);
    ExecuteAlgorithm(ws, cmptName, detList, paramName, paramValue, paramType);

    auto cmpt = ws->getInstrument()->getComponentByName(cmptName);
    TS_ASSERT_EQUALS(1.12, cmpt->getNumberParameter(paramName)[0]);

    ExecuteAlgorithm(ws, cmptName, detList, paramName, paramValue2, paramType2);

    cmpt = ws->getInstrument()->getComponentByName(cmptName);
    TS_ASSERT_EQUALS(paramValue2, cmpt->getStringParameter(paramName)[0]);
  }

  void test_bool() {
    const std::string paramName = "TestParam";
    const std::string paramType = "Bool";
    const std::map<std::string, bool> paramValues = {{"true", true},   {"TRUE", true},   {"True", true},
                                                     {"1", true},      {"false", false}, {"FALSE", false},
                                                     {"False", false}, {"0", false}};

    MatrixWorkspace_sptr ws = WorkspaceCreationHelper::create2DWorkspaceWithFullInstrument(3, 3);

    for (const auto &value : paramValues) {
      ExecuteAlgorithm(ws, "", "", paramName, value.first, paramType);
      TS_ASSERT(ws->getInstrument()->getBoolParameter(paramName)[0] == value.second);
    }
  }

  MatrixWorkspace_sptr ExecuteAlgorithm(MatrixWorkspace_sptr testWS, const std::string &cmptName,
                                        const std::string &detList, const std::string &paramName,
                                        const std::string &paramValue, const std::string &paramType = "",
                                        bool fails = false) {
    // add the workspace to the ADS
    AnalysisDataService::Instance().addOrReplace("SetInstrumentParameter_Temporary", testWS);

    // execute algorithm
    SetInstrumentParameter alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    TS_ASSERT(alg.isInitialized())

    alg.setPropertyValue("Workspace", "SetInstrumentParameter_Temporary");
    if (cmptName.length() > 0) {
      alg.setPropertyValue("ComponentName", cmptName);
    }
    if (detList.length() > 0) {
      alg.setPropertyValue("DetectorList", detList);
    }
    if (paramType.length() > 0) {
      alg.setPropertyValue("parameterType", paramType);
    }
    alg.setPropertyValue("ParameterName", paramName);
    alg.setPropertyValue("Value", paramValue);
    TS_ASSERT_THROWS_NOTHING(alg.execute())
    if (fails) {
      TS_ASSERT(!alg.isExecuted())
      return testWS;
    } else {
      TS_ASSERT(alg.isExecuted())
    }

    // check output
    MatrixWorkspace_sptr output =
        AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(alg.getProperty("Workspace"));

    // cleanup
    AnalysisDataService::Instance().remove(output->getName());

    return output;
  }
};
