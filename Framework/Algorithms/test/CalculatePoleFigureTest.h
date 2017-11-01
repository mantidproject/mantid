#ifndef MANTID_ALGORITHMS_CALCULATEPOLEFIGURE_H_
#define MANTID_ALGORITHMS_CALCULATEPOLEFIGURE_H_

#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/Axis.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/IAlgorithm.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidKernel/UnitFactory.h"
#include "MantidAPI/ITableWorkspace.h"
#include "MantidAPI/TableRow.h"
#include <cmath>
#include <cxxtest/TestSuite.h>
#include <numeric>

#include "MantidAlgorithms/CalculatePoleFigure.h"

using namespace Mantid::Algorithms;
using namespace Mantid::Kernel;
using namespace Mantid;

namespace {

//-----------------------------------------------------
/** create a Bragg workspace containg 2 spectra.
 * Each has 100 data points containing a Gaussian peak
 * between d = 1.2 and 1.5
 * @brief createBraggWorkspace
 * @return
 */
Mantid::API::MatrixWorkspace_sptr createBraggWorkspace(const std::string &name) {

  // TODO/NOW - Refer to convertUnitsTest for how to build an instrument

  size_t n = 100;

  Mantid::API::FrameworkManager::Instance();
  Mantid::DataObjects::Workspace2D_sptr ws =
      boost::dynamic_pointer_cast<Mantid::DataObjects::Workspace2D>(
          Mantid::API::WorkspaceFactory::Instance().create("Workspace2D", 2, n,
                                                           n));

  auto &X = ws->mutableX(0);
  auto &Y = ws->mutableY(0);
  auto &E = ws->mutableE(0);

  // create Gaussian peak
  double dx = 0.01;
  for (size_t i = 0; i < n; i++) {
    X[i] = double(i) * dx + 1.2;
    Y[i] = exp(-(X[i]-1.5)*(X[i]-1.5)/(0.02)) * (1.0+double(i));
    E[i] = sqrt(fabs(Y[i]));
  }

  // create unit
  std::string unitlabel("dSpacing");
  ws->getAxis(0)->unit() =
      Mantid::Kernel::UnitFactory::Instance().create(unitlabel);

  Mantid::API::AnalysisDataService::Instance().add(name, ws);

  // run 153144
  double hrot = -0.003857142;
  double omega = 89.998;


  auto instrument = ws->setInstrument();

  // detector position
  // det0 = ws1.getDetector(0)

  //  Out[9]: [-2,0,-3.67394e-16]

  // Out[11]: [2,0,1.22465e-16]

  // source: [0,0,-43.754]


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

  //-------------------------------------------------------------------------------------------
  /**
   * @brief test_Execute
   */
  void test_Execute() {

    API::Workspace_sptr ws =
        createBraggWorkspace("TwoSpecPoleFigure");

    CalculatePoleFigure pfcalculator;
    pfcalculator.initialize();

    // set properties
    pfcalculator.setProperty("InputWorkspace", ws);
    pfcalculator.setProperty("OutputWorkspace", "TwoSpecPoleFigure");
    pfcalculator.setProperty("MinD", 1.3);
    pfcalculator.setProperty("MaxD", 1.5);
    pfcalculator.execute();

    // run
    TS_ASSERT(pfcalculator.isExecuted());

    // check results
    TS_ASSERT(API::AnalysisDataService::Instance().doesExist("TwoSpecPoleFigure"));
    API::ITableWorkspace_sptr outws = boost::dynamic_pointer_cast<API::ITableWorkspace>(API::AnalysisDataService::Instance().retrieve("TwoSpecPoleFigure"));
    TS_ASSERT(outws);
    if (!outws)
        return;

    // shall have 2 rows
    TS_ASSERT_EQUALS(outws->rowCount(), 2);

    // row 0
    API::TableRow row0 = outws->getRow(0);
    int wsindex;
    double r_td, r_nd, intensity;
    row0 >> wsindex >> r_td >> r_nd >> intensity;


  }
};

#endif /* MANTID_ALGORITHMS_CALCULATEPOLEFIGURE_H_ */
