#ifndef MANTID_ALGORITHMS_CALCULATEPOLEFIGURE_H_
#define MANTID_ALGORITHMS_CALCULATEPOLEFIGURE_H_

#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/Axis.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/IAlgorithm.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidKernel/System.h"
#include "MantidKernel/Timer.h"
#include "MantidKernel/UnitFactory.h"
#include <cmath>
#include <cxxtest/TestSuite.h>
#include <numeric>

#include "MantidAlgorithms/CalculatePoleFigure.h"

using namespace Mantid::Algorithms;
using namespace Mantid::Kernel;
using namespace Mantid;

namespace {
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
          Mantid::API::WorkspaceFactory::Instance().create("Workspace2D", 1, n,
                                                           n));

  auto &X = ws->mutableX(0);
  auto &Y = ws->mutableY(0);
  auto &E = ws->mutableE(0);

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
}

class CalculatePoleFigureTest : public CxxTest::TestSuite {
public:
  void test_Init() {
    CalculatePoleFigure alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
  }

  void test_Init() {

    CalculatePoleFigure pdfft;
    pdfft.initialize();

    TS_ASSERT(pdffit.isInitlialized());
  }

  void test_Execute() {

    API::Workspace_sptr ws =
        createWS(20, 0.1, "TestInput1", "MomentumTransfer");

    CalculatePoleFigure pdfft;
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
};

class CalculatePoleFigureTestPerformance : public CxxTest::TestSuite {

public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static CalculatePoleFigureTestPerformance *createSuite() {
    return new CalculatePoleFigureTestPerformance();
  }

  static void destroySuite(CalculatePoleFigureTestPerformance *suite) {
    delete suite;
  }

  void setUp() override {
    ws = createWS(2000000, 0.1, "inputWS", "MomentumTransfer");
    pdfft = Mantid::API::FrameworkManager::Instance().createAlgorithm(
        "CalculatePoleFigure");

    pdfft->initialize();
    pdfft->setProperty("InputWorkspace", ws);
    pdfft->setProperty("OutputWorkspace", "outputWS");
    pdfft->setProperty("InputSofQType", "S(Q)");
    pdfft->setProperty("Rmax", 20.0);
    pdfft->setProperty("DeltaR", 0.01);
    pdfft->setProperty("Qmin", 0.0);
    pdfft->setProperty("Qmax", 30.0);
    pdfft->setProperty("PDFType", "G(r)");
  }

  void tearDown() override {
    Mantid::API::AnalysisDataService::Instance().remove("outputWS");
  }

  void testPerformanceWS() { pdfft->execute(); }

private:
  Mantid::API::MatrixWorkspace_sptr ws;
  API::IAlgorithm *pdfft;
};

#endif /* MANTID_ALGORITHMS_CALCULATEPOLEFIGURE_H_ */
