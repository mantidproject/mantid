#ifndef EXPONENTIALCORRECTIONTEST_H_
#define EXPONENTIALCORRECTIONTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAPI/AnalysisDataService.h"
#include "MantidAlgorithms/ExponentialCorrection.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"

using namespace Mantid::API;
using namespace Mantid::Kernel;
using namespace Mantid::DataObjects;

class ExponentialCorrectionTest : public CxxTest::TestSuite {
public:
  void testName() { TS_ASSERT_EQUALS(expon.name(), "ExponentialCorrection") }

  void testVersion() { TS_ASSERT_EQUALS(expon.version(), 1) }

  void testInit() {
    Mantid::Algorithms::ExponentialCorrection expon2;
    TS_ASSERT_THROWS_NOTHING(expon2.initialize())
    TS_ASSERT(expon2.isInitialized())

    const std::vector<Property *> props = expon2.getProperties();
    TS_ASSERT_EQUALS(props.size(), 5)

    TS_ASSERT_EQUALS(props[0]->name(), "InputWorkspace")
    TS_ASSERT(props[0]->isDefault())
    TS_ASSERT(dynamic_cast<WorkspaceProperty<MatrixWorkspace> *>(props[0]))

    TS_ASSERT_EQUALS(props[1]->name(), "OutputWorkspace")
    TS_ASSERT(props[1]->isDefault())
    TS_ASSERT(dynamic_cast<WorkspaceProperty<MatrixWorkspace> *>(props[1]))

    TS_ASSERT_EQUALS(props[2]->name(), "C0")
    TS_ASSERT(props[2]->isDefault())
    TS_ASSERT(dynamic_cast<PropertyWithValue<double> *>(props[2]))

    TS_ASSERT_EQUALS(props[3]->name(), "C1")
    TS_ASSERT(props[3]->isDefault())
    TS_ASSERT(dynamic_cast<PropertyWithValue<double> *>(props[3]))

    TS_ASSERT_EQUALS(props[4]->name(), "Operation")
    TS_ASSERT(props[4]->isDefault())
    TS_ASSERT_EQUALS(props[4]->value(), "Divide")
    TS_ASSERT(dynamic_cast<PropertyWithValue<std::string> *>(props[4]))
  }

  void testDivide() {
    MatrixWorkspace_sptr inputWS =
        WorkspaceCreationHelper::create2DWorkspaceBinned(2, 3, 0.5);
    AnalysisDataService::Instance().add("InputWS", inputWS);

    Mantid::Algorithms::ExponentialCorrection expon3;
    expon3.initialize();
    TS_ASSERT_THROWS_NOTHING(
        expon3.setPropertyValue("InputWorkspace", "InputWS"))
    TS_ASSERT_THROWS_NOTHING(
        expon3.setPropertyValue("OutputWorkspace", "WSCor"))
    TS_ASSERT_THROWS_NOTHING(expon3.setPropertyValue("c0", "2.0"))
    TS_ASSERT_THROWS_NOTHING(expon3.setPropertyValue("c1", "1.0"))

    TS_ASSERT_THROWS_NOTHING(expon3.execute())
    TS_ASSERT(expon3.isExecuted())

    MatrixWorkspace_sptr result;
    TS_ASSERT_THROWS_NOTHING(
        result = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(
            "WSCor"))
    TS_ASSERT(result)

    for (size_t i = 0; i < result->getNumberHistograms(); ++i) {
      for (int j = 0; j < 3; ++j) {
        double factor = 2.0 * exp(-1.0 * (j + 1.0));
        TS_ASSERT_EQUALS(result->dataX(i)[j], inputWS->dataX(i)[j])
        TS_ASSERT_DELTA(result->dataY(i)[j], inputWS->dataY(i)[j] / factor,
                        0.0001)
        TS_ASSERT_DELTA(result->dataE(i)[j], inputWS->dataE(i)[j] / factor,
                        0.0001)
      }
    }

    AnalysisDataService::Instance().remove("InputWS");
    AnalysisDataService::Instance().remove("WSCor");
  }

  void testMultiply() {
    MatrixWorkspace_sptr inputWS =
        WorkspaceCreationHelper::create2DWorkspaceBinned(2, 3, 0.5);
    AnalysisDataService::Instance().add("InputWS", inputWS);

    Mantid::Algorithms::ExponentialCorrection expon3;
    expon3.initialize();
    TS_ASSERT_THROWS_NOTHING(
        expon3.setPropertyValue("InputWorkspace", "InputWS"))
    TS_ASSERT_THROWS_NOTHING(
        expon3.setPropertyValue("OutputWorkspace", "WSCor"))
    TS_ASSERT_THROWS_NOTHING(expon3.setPropertyValue("c0", "2.0"))
    TS_ASSERT_THROWS_NOTHING(expon3.setPropertyValue("c1", "1.0"))
    TS_ASSERT_THROWS_NOTHING(expon3.setPropertyValue("Operation", "Multiply"))

    TS_ASSERT_THROWS_NOTHING(expon3.execute())
    TS_ASSERT(expon3.isExecuted())

    MatrixWorkspace_sptr result;
    TS_ASSERT_THROWS_NOTHING(
        result = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(
            "WSCor"))
    TS_ASSERT(result)

    for (size_t i = 0; i < result->getNumberHistograms(); ++i) {
      for (int j = 0; j < 3; ++j) {
        double factor = 2.0 * exp(-1.0 * (j + 1.0));
        TS_ASSERT_EQUALS(result->dataX(i)[j], inputWS->dataX(i)[j])
        TS_ASSERT_DELTA(result->dataY(i)[j], inputWS->dataY(i)[j] * factor,
                        0.0001)
        TS_ASSERT_DELTA(result->dataE(i)[j], inputWS->dataE(i)[j] * factor,
                        0.0001)
      }
    }

    AnalysisDataService::Instance().remove("InputWS");
    AnalysisDataService::Instance().remove("WSCor");
  }

  void testEvents() {
    EventWorkspace_sptr evin = WorkspaceCreationHelper::createEventWorkspace(
                            1, 5, 10, 0, 1, 3),
                        evout;
    AnalysisDataService::Instance().add("test_ev_ec", evin);

    Mantid::Algorithms::ExponentialCorrection alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    TS_ASSERT(alg.isInitialized());
    TS_ASSERT_THROWS_NOTHING(
        alg.setPropertyValue("InputWorkspace", "test_ev_ec"));
    TS_ASSERT_THROWS_NOTHING(
        alg.setPropertyValue("OutputWorkspace", "test_ev_ec_out"));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("C0", "3"));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("C1", "2"));

    alg.execute();
    TS_ASSERT(alg.isExecuted());

    TS_ASSERT_THROWS_NOTHING(
        evout = boost::dynamic_pointer_cast<EventWorkspace>(
            AnalysisDataService::Instance().retrieve("test_ev_ec_out")));

    TS_ASSERT(evout); // should be an event workspace
    for (size_t i = 0; i < 5; ++i) {
      double t = static_cast<double>(i) + 0.5;
      double w = 1. / 3. / std::exp(-2. * t);
      TS_ASSERT_DELTA(evout->getSpectrum(0).getEvent(i).m_weight, w, w * 1e-6);
    }

    AnalysisDataService::Instance().remove("test_ev_ec");
    AnalysisDataService::Instance().remove("test_ev_ec_out");
  }

private:
  Mantid::Algorithms::ExponentialCorrection expon;
};

#endif /*EXPONENTIALCORRECTIONTEST_H_*/
