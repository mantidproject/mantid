#ifndef MANTID_ALGORITHMS_PDFFOURIERTRANSFORMTEST_H_
#define MANTID_ALGORITHMS_PDFFOURIERTRANSFORMTEST_H_

#include <cxxtest/TestSuite.h>
#include <numeric>
#include <cmath>
#include "MantidKernel/Timer.h"
#include "MantidKernel/System.h"
#include "MantidKernel/UnitFactory.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/Axis.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/IAlgorithm.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/WorkspaceFactory.h"

#include "MantidAlgorithms/PDFFourierTransform.h"

using namespace Mantid::Algorithms;
using namespace Mantid::Kernel;
using namespace Mantid;

class PDFFourierTransformTest : public CxxTest::TestSuite {
public:
  void test_Init() {
    PDFFourierTransform alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
  }

  void test_Execute() {

    API::Workspace_sptr ws =
        createWS(20, 0.1, "TestInput1", "MomentumTransfer");

    PDFFourierTransform pdfft;
    pdfft.initialize();
    pdfft.setProperty("InputWorkspace", ws);
    pdfft.setProperty("OutputWorkspace", "PDFGofR");
    pdfft.setProperty("InputSofQType", "S(Q)");
    pdfft.setProperty("Rmax", 20.0);
    pdfft.setProperty("DeltaR", 0.01);
    pdfft.setProperty("Qmin", 0.0);
    pdfft.setProperty("Qmax", 30.0);
    pdfft.setProperty("PDFType", "G(r)");

    pdfft.execute();

    TS_ASSERT(pdfft.isExecuted());
  }

  void test_CheckResult() {

    API::Workspace_sptr ws =
        createWS(20, 0.1, "CheckResult", "MomentumTransfer");

    // 1. Run PDFFT
    API::IAlgorithm *pdfft =
        Mantid::API::FrameworkManager::Instance().createAlgorithm(
            "PDFFourierTransform");

    pdfft->initialize();
    pdfft->setProperty("InputWorkspace", ws);
    pdfft->setProperty("OutputWorkspace", "PDFGofR");
    pdfft->setProperty("InputSofQType", "S(Q)");
    pdfft->setProperty("Rmax", 20.0);
    pdfft->setProperty("DeltaR", 0.01);
    pdfft->setProperty("Qmin", 0.0);
    pdfft->setProperty("Qmax", 30.0);
    pdfft->setProperty("PDFType", "G(r)");

    pdfft->execute();

    DataObjects::Workspace2D_sptr pdfws =
        boost::dynamic_pointer_cast<DataObjects::Workspace2D>(
            API::AnalysisDataService::Instance().retrieve("PDFGofR"));
    MantidVec &R = pdfws->dataX(0);
    MantidVec &GofR = pdfws->dataY(0);

    TS_ASSERT_DELTA(R[0], 0.01, 0.0001);
    TS_ASSERT_DELTA(R[249], 2.5, 0.0001);
    TS_ASSERT_DELTA(GofR[0], 0.0186, 0.0001);
    TS_ASSERT_DELTA(GofR[249], -0.3867, 0.0001);
  }

  void test_CheckNan() {

    API::Workspace_sptr ws =
        createWS(20, 0.1, "CheckNan", "MomentumTransfer", true);

    // 1. Run PDFFT
    API::IAlgorithm *pdfft =
        Mantid::API::FrameworkManager::Instance().createAlgorithm(
            "PDFFourierTransform");

    pdfft->initialize();
    pdfft->setProperty("InputWorkspace", ws);
    pdfft->setProperty("OutputWorkspace", "PDFGofR");
    pdfft->setProperty("InputSofQType", "S(Q)");
    pdfft->setProperty("Rmax", 20.0);
    pdfft->setProperty("DeltaR", 0.01);
    pdfft->setProperty("Qmin", 0.0);
    pdfft->setProperty("Qmax", 30.0);
    pdfft->setProperty("PDFType", "G(r)");

    pdfft->execute();

    DataObjects::Workspace2D_sptr pdfws =
        boost::dynamic_pointer_cast<DataObjects::Workspace2D>(
            API::AnalysisDataService::Instance().retrieve("PDFGofR"));
    MantidVec &R = pdfws->dataX(0);
    MantidVec &GofR = pdfws->dataY(0);

    TS_ASSERT_DELTA(R[0], 0.01, 0.0001);
    TS_ASSERT_DELTA(R[249], 2.5, 0.0001);
    // make sure that nan didn' slip in
    TS_ASSERT(std::find_if(GofR.begin(), GofR.end(),
                           static_cast<bool (*)(double)>(std::isnan)) ==
              GofR.end());
  }

  void test_filter() {
    API::MatrixWorkspace_sptr ws =
        createWS(200, 0.1, "filter", "MomentumTransfer");
    MantidVec &SofQ = ws->dataY(0);
    for (size_t i = 0; i < SofQ.size(); i++) {
      SofQ[i] = 1.0;
    }

    // 1. Run PDFFT
    API::IAlgorithm *pdfft =
        Mantid::API::FrameworkManager::Instance().createAlgorithm(
            "PDFFourierTransform");

    pdfft->initialize();
    pdfft->setProperty("InputWorkspace", ws);
    pdfft->setProperty("OutputWorkspace", "PDFGofR");
    pdfft->setProperty("InputSofQType", "S(Q)-1");
    pdfft->setProperty("Qmax", 20.0);
    pdfft->setProperty("PDFType", "G(r)");
    pdfft->setProperty("Filter", true);

    pdfft->execute();

    DataObjects::Workspace2D_sptr pdfws =
        boost::dynamic_pointer_cast<DataObjects::Workspace2D>(
            API::AnalysisDataService::Instance().retrieve("PDFGofR"));
    MantidVec &GofR = pdfws->dataY(0);

    TS_ASSERT(GofR[0] > 40.0);
    for (size_t i = 1; i < GofR.size(); i++) {
      TS_ASSERT_LESS_THAN(fabs(GofR[i]), 1e-12);
    }

    Mantid::API::AnalysisDataService::Instance().clear();
  }

private:
  /**
   * Create Workspace from 0 to N*dx
   */
  Mantid::API::MatrixWorkspace_sptr createWS(size_t n, double dx,
                                             const std::string &name,
                                             const std::string unitlabel,
                                             const bool withBadValues = false) {

    Mantid::API::FrameworkManager::Instance();
    Mantid::DataObjects::Workspace2D_sptr ws =
        boost::dynamic_pointer_cast<Mantid::DataObjects::Workspace2D>(
            Mantid::API::WorkspaceFactory::Instance().create("Workspace2D", 1,
                                                             n, n));

    Mantid::MantidVec &X = ws->dataX(0);
    Mantid::MantidVec &Y = ws->dataY(0);
    Mantid::MantidVec &E = ws->dataE(0);

    for (size_t i = 0; i < n; i++) {
      X[i] = double(i) * dx;
      Y[i] = X[i] + 1.0;
      E[i] = sqrt(fabs(X[i]));
    }

    if (withBadValues) {
      Y[0] = std::numeric_limits<double>::quiet_NaN();
      Y[Y.size() - 1] = std::numeric_limits<double>::quiet_NaN();
    }

    ws->getAxis(0)->unit() =
        Mantid::Kernel::UnitFactory::Instance().create(unitlabel);

    Mantid::API::AnalysisDataService::Instance().add(name, ws);

    return ws;
  }
};

#endif /* MANTID_ALGORITHMS_PDFFOURIERTRANSFORMTEST_H_ */
