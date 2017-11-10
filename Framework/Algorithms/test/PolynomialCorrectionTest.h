#ifndef POLYNOMIALCORRECTIONTEST_H_
#define POLYNOMIALCORRECTIONTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidTestHelpers/WorkspaceCreationHelper.h"
#include "MantidAlgorithms/PolynomialCorrection.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidKernel/ArrayProperty.h"

using namespace Mantid::API;
using namespace Mantid::Kernel;
using namespace Mantid::DataObjects;

class PolynomialCorrectionTest : public CxxTest::TestSuite {
public:
  void testName() { TS_ASSERT_EQUALS(poly.name(), "PolynomialCorrection"); }

  void testVersion() { TS_ASSERT_EQUALS(poly.version(), 1); }

  void testCategory() {
    TS_ASSERT_EQUALS(poly.category(), "CorrectionFunctions");
  }

  void testInit() {
    Mantid::Algorithms::PolynomialCorrection poly2;
    TS_ASSERT_THROWS_NOTHING(poly2.initialize());
    TS_ASSERT(poly2.isInitialized());

    const std::vector<Property *> props = poly2.getProperties();
    TS_ASSERT_EQUALS(props.size(), 4);

    TS_ASSERT_EQUALS(props[0]->name(), "InputWorkspace");
    TS_ASSERT(props[0]->isDefault());
    TS_ASSERT(dynamic_cast<WorkspaceProperty<MatrixWorkspace> *>(props[0]));

    TS_ASSERT_EQUALS(props[1]->name(), "OutputWorkspace");
    TS_ASSERT(props[1]->isDefault());
    TS_ASSERT(dynamic_cast<WorkspaceProperty<MatrixWorkspace> *>(props[1]));

    TS_ASSERT_EQUALS(props[2]->name(), "Coefficients");
    TS_ASSERT(props[2]->isDefault());
    TS_ASSERT(dynamic_cast<ArrayProperty<double> *>(props[2]));

    TS_ASSERT_EQUALS(props[3]->name(), "Operation");
    TS_ASSERT(props[3]->isDefault());
  }

  void testExecMultiply() {
    const std::string wsName = "PolynomialCorrectionTest_inputWS";
    const std::string wsNameOut = "PolynomialCorrectionTest_outputWS";
    MatrixWorkspace_sptr inputWS =
        WorkspaceCreationHelper::create2DWorkspaceBinned(2, 3, 0.5);
    AnalysisDataService::Instance().add(wsName, inputWS);

    Mantid::Algorithms::PolynomialCorrection poly3;
    poly3.initialize();
    TS_ASSERT_THROWS_NOTHING(poly3.setPropertyValue("InputWorkspace", wsName));
    TS_ASSERT_THROWS_NOTHING(
        poly3.setPropertyValue("OutputWorkspace", wsNameOut));
    TS_ASSERT_THROWS_NOTHING(
        poly3.setPropertyValue("Coefficients", "3.0,2.0,1.0"));

    TS_ASSERT_THROWS_NOTHING(poly3.execute());
    TS_ASSERT(poly3.isExecuted());

    MatrixWorkspace_sptr result;
    TS_ASSERT_THROWS_NOTHING(
        result = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(
            wsNameOut));
    TS_ASSERT(result);

    for (size_t i = 0; i < result->getNumberHistograms(); ++i) {
      for (int j = 1; j < 4; ++j) {
        double factor = 3.0 + j * 2.0 + j * j * 1.0;
        TS_ASSERT_EQUALS(result->dataX(i)[j - 1], inputWS->dataX(i)[j - 1]);
        TS_ASSERT_EQUALS(result->dataY(i)[j - 1],
                         factor * inputWS->dataY(i)[j - 1]);
        TS_ASSERT_EQUALS(result->dataE(i)[j - 1],
                         factor * inputWS->dataE(i)[j - 1]);
      }
    }

    AnalysisDataService::Instance().remove(wsName);
    AnalysisDataService::Instance().remove(wsNameOut);
  }

  void testExecDivide() {
    const std::string wsName = "PolynomialCorrectionTest_inputWS";
    const std::string wsNameOut = "PolynomialCorrectionTest_outputWS";
    MatrixWorkspace_sptr inputWS =
        WorkspaceCreationHelper::create2DWorkspaceBinned(2, 3, 0.5);
    AnalysisDataService::Instance().add(wsName, inputWS);

    Mantid::Algorithms::PolynomialCorrection poly3;
    poly3.initialize();
    TS_ASSERT_THROWS_NOTHING(poly3.setPropertyValue("InputWorkspace", wsName));
    TS_ASSERT_THROWS_NOTHING(
        poly3.setPropertyValue("OutputWorkspace", wsNameOut));
    TS_ASSERT_THROWS_NOTHING(
        poly3.setPropertyValue("Coefficients", "3.0,2.0,1.0"));
    TS_ASSERT_THROWS_NOTHING(poly3.setPropertyValue("Operation", "Divide"));

    TS_ASSERT_THROWS_NOTHING(poly3.execute());
    TS_ASSERT(poly3.isExecuted());

    MatrixWorkspace_sptr result;
    TS_ASSERT_THROWS_NOTHING(
        result = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(
            wsNameOut));
    TS_ASSERT(result);

    for (size_t i = 0; i < result->getNumberHistograms(); ++i) {
      for (int j = 1; j < 4; ++j) {
        double factor = 3.0 + j * 2.0 + j * j * 1.0;
        TS_ASSERT_EQUALS(result->dataX(i)[j - 1], inputWS->dataX(i)[j - 1]);
        TS_ASSERT_EQUALS(result->dataY(i)[j - 1],
                         inputWS->dataY(i)[j - 1] / factor);
        TS_ASSERT_EQUALS(result->dataE(i)[j - 1],
                         inputWS->dataE(i)[j - 1] / factor);
      }
    }

    AnalysisDataService::Instance().remove(wsName);
    AnalysisDataService::Instance().remove(wsNameOut);
  }

  void testEvents() {
    EventWorkspace_sptr evin = WorkspaceCreationHelper::createEventWorkspace(
                            1, 5, 10, 0, 1, 3),
                        evout;
    AnalysisDataService::Instance().add("test_ev_polyc", evin);

    Mantid::Algorithms::PolynomialCorrection alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    TS_ASSERT(alg.isInitialized());
    TS_ASSERT_THROWS_NOTHING(
        alg.setPropertyValue("InputWorkspace", "test_ev_polyc"));
    TS_ASSERT_THROWS_NOTHING(
        alg.setPropertyValue("OutputWorkspace", "test_ev_polyc_out"));
    TS_ASSERT_THROWS_NOTHING(
        alg.setPropertyValue("Coefficients", "3.0,2.0,1.0"));

    alg.execute();
    TS_ASSERT(alg.isExecuted());

    TS_ASSERT_THROWS_NOTHING(
        evout = boost::dynamic_pointer_cast<EventWorkspace>(
            AnalysisDataService::Instance().retrieve("test_ev_polyc_out")));

    TS_ASSERT(evout); // should be an event workspace
    for (size_t i = 0; i < 5; ++i) {
      double t = static_cast<double>(i) + 0.5;
      double w = 3.0 + t * 2.0 + t * t;
      TS_ASSERT_DELTA(evout->getSpectrum(0).getEvent(i).m_weight, w, w * 1e-6);
    }

    AnalysisDataService::Instance().remove("test_ev_polyc");
    AnalysisDataService::Instance().remove("test_ev_polyc_out");
  }

private:
  Mantid::Algorithms::PolynomialCorrection poly;
};

#endif /*POLYNOMIALCORRECTIONTEST_H_*/
