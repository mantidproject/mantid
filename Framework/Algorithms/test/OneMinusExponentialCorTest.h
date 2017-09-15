#ifndef ONEMINUSEXPONENTIALCORTEST_H_
#define ONEMINUSEXPONENTIALCORTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidTestHelpers/WorkspaceCreationHelper.h"
#include "MantidAlgorithms/OneMinusExponentialCor.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidKernel/ArrayProperty.h"

using namespace Mantid::API;
using namespace Mantid::Kernel;
using namespace Mantid::DataObjects;

class OneMinusExponentialCorTest : public CxxTest::TestSuite {
public:
  void testName() { TS_ASSERT_EQUALS(expon.name(), "OneMinusExponentialCor"); }

  void testVersion() { TS_ASSERT_EQUALS(expon.version(), 1); }

  void testCategory() {
    TS_ASSERT_EQUALS(expon.category(), "CorrectionFunctions");
  }

  void testInit() {
    Mantid::Algorithms::OneMinusExponentialCor expon2;
    TS_ASSERT_THROWS_NOTHING(expon2.initialize());
    TS_ASSERT(expon2.isInitialized());

    const std::vector<Property *> props = expon2.getProperties();
    TS_ASSERT_EQUALS(props.size(), 5);

    TS_ASSERT_EQUALS(props[0]->name(), "InputWorkspace");
    TS_ASSERT(props[0]->isDefault());
    TS_ASSERT(dynamic_cast<WorkspaceProperty<MatrixWorkspace> *>(props[0]));

    TS_ASSERT_EQUALS(props[1]->name(), "OutputWorkspace");
    TS_ASSERT(props[1]->isDefault());
    TS_ASSERT(dynamic_cast<WorkspaceProperty<MatrixWorkspace> *>(props[1]));

    TS_ASSERT_EQUALS(props[2]->name(), "C");
    TS_ASSERT(props[2]->isDefault());
    TS_ASSERT(dynamic_cast<PropertyWithValue<double> *>(props[2]));

    TS_ASSERT_EQUALS(props[3]->name(), "C1");
    TS_ASSERT(props[3]->isDefault());
    TS_ASSERT(dynamic_cast<PropertyWithValue<double> *>(props[3]));

    TS_ASSERT_EQUALS(props[4]->name(), "Operation");
    TS_ASSERT(props[4]->isDefault());
    TS_ASSERT_EQUALS(props[4]->value(), "Divide");
    TS_ASSERT(dynamic_cast<PropertyWithValue<std::string> *>(props[4]));
  }

  void testDivide() {
    MatrixWorkspace_sptr inputWS =
        WorkspaceCreationHelper::create2DWorkspaceBinned(2, 3, 0.5);
    AnalysisDataService::Instance().add("InputWS", inputWS);

    Mantid::Algorithms::OneMinusExponentialCor expon3;
    expon3.initialize();
    TS_ASSERT_THROWS_NOTHING(
        expon3.setPropertyValue("InputWorkspace", "InputWS"));
    TS_ASSERT_THROWS_NOTHING(
        expon3.setPropertyValue("OutputWorkspace", "WSCor"));
    TS_ASSERT_THROWS_NOTHING(expon3.setPropertyValue("C", "2.0"));

    TS_ASSERT_THROWS_NOTHING(expon3.execute());
    TS_ASSERT(expon3.isExecuted());

    MatrixWorkspace_sptr result;
    TS_ASSERT_THROWS_NOTHING(
        result = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(
            "WSCor"));
    TS_ASSERT(result);

    for (size_t i = 0; i < result->getNumberHistograms(); ++i) {
      for (int j = 0; j < 3; ++j) {
        double factor = 1.0 - exp(-1.0 * 2.0 * (j + 1.0));
        TS_ASSERT_EQUALS(result->dataX(i)[j], inputWS->dataX(i)[j]);
        TS_ASSERT_DELTA(result->dataY(i)[j], inputWS->dataY(i)[j] / factor,
                        0.0001);
        TS_ASSERT_DELTA(result->dataE(i)[j], inputWS->dataE(i)[j] / factor,
                        0.0001);
      }
    }

    AnalysisDataService::Instance().remove("InputWS");
    AnalysisDataService::Instance().remove("WSCor");
  }

  void testDivideWithPrefactor() {
    MatrixWorkspace_sptr inputWS =
        WorkspaceCreationHelper::create2DWorkspaceBinned(2, 3, 0.5);
    AnalysisDataService::Instance().add("InputWS", inputWS);

    Mantid::Algorithms::OneMinusExponentialCor expon3;
    expon3.initialize();
    TS_ASSERT_THROWS_NOTHING(
        expon3.setPropertyValue("InputWorkspace", "InputWS"));
    TS_ASSERT_THROWS_NOTHING(
        expon3.setPropertyValue("OutputWorkspace", "WSCor"));
    const double prefactor = 2.0;
    TS_ASSERT_THROWS_NOTHING(expon3.setProperty<double>("C1", prefactor));
    TS_ASSERT_THROWS_NOTHING(expon3.setPropertyValue("C", "2.0"));

    TS_ASSERT_THROWS_NOTHING(expon3.execute());
    TS_ASSERT(expon3.isExecuted());

    MatrixWorkspace_sptr result;
    TS_ASSERT_THROWS_NOTHING(
        result = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(
            "WSCor"));
    TS_ASSERT(result);

    for (size_t i = 0; i < result->getNumberHistograms(); ++i) {
      for (int j = 0; j < 3; ++j) {
        double factor = prefactor * (1.0 - exp(-1.0 * 2.0 * (j + 1.0)));
        TS_ASSERT_EQUALS(result->dataX(i)[j], inputWS->dataX(i)[j]);
        TS_ASSERT_DELTA(result->dataY(i)[j], inputWS->dataY(i)[j] / factor,
                        0.0001);
        TS_ASSERT_DELTA(result->dataE(i)[j], inputWS->dataE(i)[j] / factor,
                        0.0001);
      }
    }

    AnalysisDataService::Instance().remove("InputWS");
    AnalysisDataService::Instance().remove("WSCor");
  }

  void testMultiply() {
    MatrixWorkspace_sptr inputWS =
        WorkspaceCreationHelper::create2DWorkspaceBinned(2, 3, 0.5);
    AnalysisDataService::Instance().add("InputWS", inputWS);

    Mantid::Algorithms::OneMinusExponentialCor expon3;
    expon3.initialize();
    TS_ASSERT_THROWS_NOTHING(
        expon3.setPropertyValue("InputWorkspace", "InputWS"));
    TS_ASSERT_THROWS_NOTHING(
        expon3.setPropertyValue("OutputWorkspace", "WSCor"));
    TS_ASSERT_THROWS_NOTHING(expon3.setPropertyValue("C", "2.0"));
    TS_ASSERT_THROWS_NOTHING(expon3.setPropertyValue("Operation", "Multiply"));

    TS_ASSERT_THROWS_NOTHING(expon3.execute());
    TS_ASSERT(expon3.isExecuted());

    MatrixWorkspace_sptr result;
    TS_ASSERT_THROWS_NOTHING(
        result = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(
            "WSCor"));
    TS_ASSERT(result);

    for (size_t i = 0; i < result->getNumberHistograms(); ++i) {
      for (int j = 0; j < 3; ++j) {
        double factor = 1.0 - exp(-1.0 * 2.0 * (j + 1.0));
        TS_ASSERT_EQUALS(result->dataX(i)[j], inputWS->dataX(i)[j]);
        TS_ASSERT_DELTA(result->dataY(i)[j], inputWS->dataY(i)[j] * factor,
                        0.0001);
        TS_ASSERT_DELTA(result->dataE(i)[j], inputWS->dataE(i)[j] * factor,
                        0.0001);
      }
    }

    AnalysisDataService::Instance().remove("InputWS");
    AnalysisDataService::Instance().remove("WSCor");
  }

  void testMultiplyWithPrefactor() {
    MatrixWorkspace_sptr inputWS =
        WorkspaceCreationHelper::create2DWorkspaceBinned(2, 3, 0.5);
    AnalysisDataService::Instance().add("InputWS", inputWS);

    Mantid::Algorithms::OneMinusExponentialCor expon3;
    expon3.initialize();
    TS_ASSERT_THROWS_NOTHING(
        expon3.setPropertyValue("InputWorkspace", "InputWS"));
    TS_ASSERT_THROWS_NOTHING(
        expon3.setPropertyValue("OutputWorkspace", "WSCor"));
    TS_ASSERT_THROWS_NOTHING(expon3.setPropertyValue("C", "2.0"));
    const double prefactor = 2.0;
    TS_ASSERT_THROWS_NOTHING(expon3.setProperty<double>("C1", prefactor));
    TS_ASSERT_THROWS_NOTHING(expon3.setPropertyValue("Operation", "Multiply"));

    TS_ASSERT_THROWS_NOTHING(expon3.execute());
    TS_ASSERT(expon3.isExecuted());

    MatrixWorkspace_sptr result;
    TS_ASSERT_THROWS_NOTHING(
        result = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(
            "WSCor"));
    TS_ASSERT(result);

    for (size_t i = 0; i < result->getNumberHistograms(); ++i) {
      for (int j = 0; j < 3; ++j) {
        double factor = prefactor * (1.0 - exp(-1.0 * 2.0 * (j + 1.0)));
        TS_ASSERT_EQUALS(result->dataX(i)[j], inputWS->dataX(i)[j]);
        TS_ASSERT_DELTA(result->dataY(i)[j], inputWS->dataY(i)[j] * factor,
                        0.0001);
        TS_ASSERT_DELTA(result->dataE(i)[j], inputWS->dataE(i)[j] * factor,
                        0.0001);
      }
    }

    AnalysisDataService::Instance().remove("InputWS");
    AnalysisDataService::Instance().remove("WSCor");
  }

  void testEvents() {
    EventWorkspace_sptr evin = WorkspaceCreationHelper::createEventWorkspace(
                            1, 5, 10, 0, 1, 3),
                        evout;
    AnalysisDataService::Instance().add("test_ev_omec", evin);

    Mantid::Algorithms::OneMinusExponentialCor alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    TS_ASSERT(alg.isInitialized());
    TS_ASSERT_THROWS_NOTHING(
        alg.setPropertyValue("InputWorkspace", "test_ev_omec"));
    TS_ASSERT_THROWS_NOTHING(
        alg.setPropertyValue("OutputWorkspace", "test_ev_omec_out"));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("C", "3"));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("C1", "2"));

    alg.execute();
    TS_ASSERT(alg.isExecuted());

    TS_ASSERT_THROWS_NOTHING(
        evout = boost::dynamic_pointer_cast<EventWorkspace>(
            AnalysisDataService::Instance().retrieve("test_ev_omec_out")));

    TS_ASSERT(evout); // should be an event workspace
    for (size_t i = 0; i < 5; ++i) {
      double t = static_cast<double>(i) + 0.5;
      double w = 0.5 / (1 - std::exp(-3. * t));
      TS_ASSERT_DELTA(evout->getSpectrum(0).getEvent(i).m_weight, w, 1e-6);
    }

    AnalysisDataService::Instance().remove("test_ev_omec");
    AnalysisDataService::Instance().remove("test_ev_omec_out");
  }

private:
  Mantid::Algorithms::OneMinusExponentialCor expon;
};

#endif /*ONEMINUSEXPONENTIALCORTEST_H_*/
