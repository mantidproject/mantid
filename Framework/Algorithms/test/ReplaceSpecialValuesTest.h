#ifndef REPLACESPECIALVALUESTEST_H_
#define REPLACESPECIALVALUESTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAPI/AnalysisDataService.h"
#include "MantidAlgorithms/ReplaceSpecialValues.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"
#include <cmath>
#include <limits>

using namespace Mantid::API;
using namespace Mantid::Kernel;
using namespace Mantid::DataObjects;

class ReplaceSpecialValuesTest : public CxxTest::TestSuite {
public:
  void testName() { TS_ASSERT_EQUALS(alg.name(), "ReplaceSpecialValues"); }

  void testVersion() { TS_ASSERT_EQUALS(alg.version(), 1); }

  void testInit() {
    Mantid::Algorithms::ReplaceSpecialValues alg2;
    TS_ASSERT_THROWS_NOTHING(alg2.initialize());
    TS_ASSERT(alg2.isInitialized());

    const std::vector<Property *> props = alg2.getProperties();
    TS_ASSERT_EQUALS(props.size(), 12);

    TS_ASSERT_EQUALS(props[0]->name(), "InputWorkspace");
    TS_ASSERT(dynamic_cast<WorkspaceProperty<MatrixWorkspace> *>(props[0]));

    TS_ASSERT_EQUALS(props[1]->name(), "OutputWorkspace");
    TS_ASSERT(dynamic_cast<WorkspaceProperty<MatrixWorkspace> *>(props[1]));

    TS_ASSERT_EQUALS(props[2]->name(), "NaNValue");
    TS_ASSERT_EQUALS(props[3]->name(), "NaNError");
    TS_ASSERT_EQUALS(props[4]->name(), "InfinityValue");
    TS_ASSERT_EQUALS(props[5]->name(), "InfinityError");
    TS_ASSERT_EQUALS(props[6]->name(), "BigNumberThreshold");
    TS_ASSERT_EQUALS(props[7]->name(), "BigNumberValue");
    TS_ASSERT_EQUALS(props[8]->name(), "BigNumberError");
    TS_ASSERT_EQUALS(props[9]->name(), "SmallNumberThreshold");
    TS_ASSERT_EQUALS(props[10]->name(), "SmallNumberValue");
    TS_ASSERT_EQUALS(props[11]->name(), "SmallNumberError");

    for (const auto prop : props) {
      assert_property_is_default(prop);
    }
  }

  void testExecCheckBoth() {
    MatrixWorkspace_sptr inputWS = createWorkspace();
    AnalysisDataService::Instance().add("InputWS", inputWS);

    Mantid::Algorithms::ReplaceSpecialValues alg3;
    alg3.initialize();
    TS_ASSERT_THROWS_NOTHING(
        alg3.setPropertyValue("InputWorkspace", "InputWS"));
    TS_ASSERT_THROWS_NOTHING(alg3.setPropertyValue("OutputWorkspace", "WSCor"));
    TS_ASSERT_THROWS_NOTHING(alg3.setPropertyValue("NaNValue", "-99.0"));
    TS_ASSERT_THROWS_NOTHING(alg3.setPropertyValue("NaNError", "-50.0"));
    TS_ASSERT_THROWS_NOTHING(alg3.setPropertyValue("InfinityValue", "999.0"));
    TS_ASSERT_THROWS_NOTHING(alg3.setPropertyValue("InfinityError", "0.00005"));

    TS_ASSERT_THROWS_NOTHING(alg3.execute());
    TS_ASSERT(alg3.isExecuted());

    MatrixWorkspace_sptr result;
    TS_ASSERT_THROWS_NOTHING(
        result = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(
            "WSCor"));
    TS_ASSERT(result);

    checkValues(inputWS, result, 1, 1);

    AnalysisDataService::Instance().remove("InputWS");
    AnalysisDataService::Instance().remove("WSCor");
  }

  void testExecCheckNaN() {
    MatrixWorkspace_sptr inputWS = createWorkspace();
    AnalysisDataService::Instance().add("InputWS", inputWS);

    Mantid::Algorithms::ReplaceSpecialValues alg3;
    alg3.initialize();
    TS_ASSERT_THROWS_NOTHING(
        alg3.setPropertyValue("InputWorkspace", "InputWS"));
    TS_ASSERT_THROWS_NOTHING(alg3.setPropertyValue("OutputWorkspace", "WSCor"));
    TS_ASSERT_THROWS_NOTHING(alg3.setPropertyValue("NaNValue", "-99.0"));
    TS_ASSERT_THROWS_NOTHING(alg3.setPropertyValue("NaNError", "-50.0"));

    TS_ASSERT_THROWS_NOTHING(alg3.execute());
    TS_ASSERT(alg3.isExecuted());

    MatrixWorkspace_sptr result;
    TS_ASSERT_THROWS_NOTHING(
        result = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(
            "WSCor"));
    TS_ASSERT(result);

    checkValues(inputWS, result, 1, 0);

    AnalysisDataService::Instance().remove("InputWS");
    AnalysisDataService::Instance().remove("WSCor");
  }

  void testExecCheckInf() {
    MatrixWorkspace_sptr inputWS = createWorkspace();
    AnalysisDataService::Instance().add("InputWS", inputWS);

    Mantid::Algorithms::ReplaceSpecialValues alg3;
    alg3.initialize();
    TS_ASSERT_THROWS_NOTHING(
        alg3.setPropertyValue("InputWorkspace", "InputWS"));
    TS_ASSERT_THROWS_NOTHING(alg3.setPropertyValue("OutputWorkspace", "WSCor"));
    TS_ASSERT_THROWS_NOTHING(alg3.setPropertyValue("InfinityValue", "999.0"));
    TS_ASSERT_THROWS_NOTHING(alg3.setPropertyValue("InfinityError", "0.00005"));

    TS_ASSERT_THROWS_NOTHING(alg3.execute());
    TS_ASSERT(alg3.isExecuted());

    MatrixWorkspace_sptr result;
    TS_ASSERT_THROWS_NOTHING(
        result = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(
            "WSCor"));
    TS_ASSERT(result);

    checkValues(inputWS, result, 0, 1);

    AnalysisDataService::Instance().remove("InputWS");
    AnalysisDataService::Instance().remove("WSCor");
  }

  void testExecCheckBig() {
    MatrixWorkspace_sptr inputWS = createWorkspace();
    // Put in some 'big' values
    inputWS->mutableY(0)[0] = 1.0E12;
    inputWS->mutableY(0)[1] = 1.000001E10;
    AnalysisDataService::Instance().add("InputWS", inputWS);

    Mantid::Algorithms::ReplaceSpecialValues alg3;
    alg3.initialize();
    TS_ASSERT_THROWS_NOTHING(
        alg3.setPropertyValue("InputWorkspace", "InputWS"));
    TS_ASSERT_THROWS_NOTHING(alg3.setPropertyValue("OutputWorkspace", "WSCor"));
    TS_ASSERT_THROWS_NOTHING(
        alg3.setPropertyValue("BigNumberThreshold", "1.0E10"));
    TS_ASSERT_THROWS_NOTHING(alg3.setPropertyValue("BigNumberValue", "999"));
    TS_ASSERT_THROWS_NOTHING(
        alg3.setPropertyValue("BigNumberError", "0.00005"));

    TS_ASSERT_THROWS_NOTHING(alg3.execute());
    TS_ASSERT(alg3.isExecuted());

    MatrixWorkspace_sptr result;
    TS_ASSERT_THROWS_NOTHING(
        result = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(
            "WSCor"));
    TS_ASSERT(result);

    for (size_t i = 0; i < result->getNumberHistograms(); ++i) {
      for (int j = 0; j < 4; ++j) {
        if ((i == 0 && j != 3) || (i == 1 && j == 0)) {
          TS_ASSERT_EQUALS(result->y(i)[j], 999);
          TS_ASSERT_EQUALS(result->e(i)[j], 0.00005);
        } else if (inputWS->y(i)[j] != inputWS->y(i)[j]) // NaN
        {
          TS_ASSERT_DIFFERS(result->y(i)[j], result->y(i)[j]);
        } else {
          TS_ASSERT_EQUALS(result->y(i)[j], inputWS->y(i)[j]);
        }
      }
    }

    AnalysisDataService::Instance().remove("InputWS");
    AnalysisDataService::Instance().remove("WSCor");
  }

  void testExecCheckSmall() {
    MatrixWorkspace_sptr inputWS = createWorkspace();
    // Put in some small values
    inputWS->dataY(0)[0] = 2.0E-7;
    inputWS->dataY(0)[1] = 1.99E-7;
    AnalysisDataService::Instance().add("InputWS", inputWS);

    Mantid::Algorithms::ReplaceSpecialValues alg3;
    alg3.initialize();
    TS_ASSERT_THROWS_NOTHING(
        alg3.setPropertyValue("InputWorkspace", "InputWS"));
    TS_ASSERT_THROWS_NOTHING(alg3.setPropertyValue("OutputWorkspace", "WSCor"));
    TS_ASSERT_THROWS_NOTHING(
        alg3.setPropertyValue("SmallNumberThreshold", "2.0E-7"));
    TS_ASSERT_THROWS_NOTHING(
        alg3.setPropertyValue("SmallNumberValue", "0.123"));
    TS_ASSERT_THROWS_NOTHING(
        alg3.setPropertyValue("SmallNumberError", "0.456"));

    TS_ASSERT_THROWS_NOTHING(alg3.execute());
    TS_ASSERT(alg3.isExecuted());

    MatrixWorkspace_sptr result;
    TS_ASSERT_THROWS_NOTHING(
        result = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(
            "WSCor"));
    TS_ASSERT(result);

    TS_ASSERT_EQUALS(result->y(0)[1], 0.123);
    TS_ASSERT_EQUALS(result->e(0)[1], 0.456);

    for (size_t i = 0; i < result->getNumberHistograms(); ++i) {
      for (int j = 0; j < 4; ++j) {
        if ((i == 0 && j == 1) || !std::isnormal(inputWS->y(i)[j])) {
          // Skip our changed one or any we can't compare
          continue;
        } else {
          TS_ASSERT_EQUALS(result->y(i)[j], inputWS->y(i)[j]);
          TS_ASSERT_EQUALS(result->e(i)[j], inputWS->e(i)[j]);
        }
      }
    }

    AnalysisDataService::Instance().remove("InputWS");
    AnalysisDataService::Instance().remove("WSCor");
  }

  void testExecCheckNone() {
    MatrixWorkspace_sptr inputWS = createWorkspace();
    AnalysisDataService::Instance().add("InputWS", inputWS);

    Mantid::Algorithms::ReplaceSpecialValues alg3;
    alg3.initialize();
    TS_ASSERT_THROWS_NOTHING(
        alg3.setPropertyValue("InputWorkspace", "InputWS"));
    TS_ASSERT_THROWS_NOTHING(alg3.setPropertyValue("OutputWorkspace", "WSCor"));

    TS_ASSERT_THROWS_NOTHING(alg3.execute());
    TS_ASSERT(!alg3.isExecuted());

    MatrixWorkspace_sptr result;

    AnalysisDataService::Instance().remove("InputWS");
  }

  void checkValues(MatrixWorkspace_sptr inputWS, MatrixWorkspace_sptr result,
                   bool naNCheck, bool infCheck) {

    for (size_t i = 0; i < result->getNumberHistograms(); ++i) {
      for (int j = 1; j < 5; ++j) {
        TS_ASSERT_EQUALS(result->x(i)[j - 1], inputWS->x(i)[j - 1]);

        if (infCheck && std::isinf(inputWS->y(i)[j - 1])) {
          if (std::isinf(result->y(i)[j - 1])) {
            TS_FAIL("Infinity detected that should have been replaced");
          } else {
            TS_ASSERT_DELTA(result->y(i)[j - 1], 999.0, 1e-8);
            TS_ASSERT_DELTA(result->e(i)[j - 1], 0.00005, 1e-8);
          }
        } else if (naNCheck && std::isnan(inputWS->y(i)[j - 1])) {
          TS_ASSERT_DELTA(result->y(i)[j - 1], -99.0, 1e-8);
          TS_ASSERT_DELTA(result->e(i)[j - 1], -50.0, 1e-8);
        } else {
          if (!naNCheck && std::isnan(inputWS->y(i)[j - 1])) {
            TS_ASSERT_DIFFERS(result->y(i)[j - 1], result->y(i)[j - 1]);
          } else {
            TS_ASSERT_EQUALS(result->y(i)[j - 1], inputWS->y(i)[j - 1]);
          }
          TS_ASSERT_EQUALS(result->e(i)[j - 1], inputWS->e(i)[j - 1]);
        }
      }
    }
  }

  MatrixWorkspace_sptr createWorkspace() {
    MatrixWorkspace_sptr inputWS =
        WorkspaceCreationHelper::create2DWorkspaceBinned(4, 4, 0.5);
    // put some infinities and NaNs in there
    double inf = std::numeric_limits<double>::infinity();
    inputWS->dataY(0)[2] = inf;
    inputWS->dataY(1)[0] = -inf;
    double nan = std::numeric_limits<double>::quiet_NaN();
    inputWS->dataY(2)[3] = nan;
    inputWS->dataY(3)[1] = nan;

    return inputWS;
  }

  void testEvents() {
    EventWorkspace_sptr evin = WorkspaceCreationHelper::createEventWorkspace(
                            1, 5, 10, 0, 1, 3),
                        evout;
    AnalysisDataService::Instance().add("test_ev_rep", evin);
    auto &evlist = evin->getSpectrum(0);
    evlist.switchTo(WEIGHTED);
    evlist.getWeightedEvents().at(0).m_weight = static_cast<float>(0.01);
    evlist.getWeightedEvents().at(1).m_weight =
        std::numeric_limits<float>::infinity();
    evlist.getWeightedEvents().at(2).m_weight =
        std::numeric_limits<float>::quiet_NaN();

    Mantid::Algorithms::ReplaceSpecialValues alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    TS_ASSERT(alg.isInitialized());
    TS_ASSERT_THROWS_NOTHING(
        alg.setPropertyValue("InputWorkspace", "test_ev_rep"));
    TS_ASSERT_THROWS_NOTHING(
        alg.setPropertyValue("OutputWorkspace", "test_ev_rep_out"));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("NaNValue", "7"));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("NaNError", "8"));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("InfinityValue", "9"));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("InfinityError", "10"));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("BigNumberThreshold", "0.1"));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("BigNumberValue", "-11"));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("BigNumberError", "-12"));

    alg.execute();
    TS_ASSERT(alg.isExecuted());

    TS_ASSERT_THROWS_NOTHING(
        evout = boost::dynamic_pointer_cast<EventWorkspace>(
            AnalysisDataService::Instance().retrieve("test_ev_rep_out")));

    TS_ASSERT(evout); // should be an event workspace
    TS_ASSERT_DELTA(evout->getSpectrum(0).getEvent(0).m_weight, 0.01, 1e-8);
    TS_ASSERT_EQUALS(evout->getSpectrum(0).getEvent(1).m_weight, 9);
    TS_ASSERT_EQUALS(evout->getSpectrum(0).getEvent(2).m_weight, 7);
    TS_ASSERT_EQUALS(evout->getSpectrum(0).getEvent(3).m_weight, -11);
    AnalysisDataService::Instance().remove("test_ev_rep");
    AnalysisDataService::Instance().remove("test_ev_rep_out");
  }

private:
  Mantid::Algorithms::ReplaceSpecialValues alg;

  void assert_property_is_default(const Property *propToCheck) {
    TS_ASSERT(propToCheck->isDefault());
  }
};

#endif /*REPLACESPECIALVALUESTEST_H_*/
