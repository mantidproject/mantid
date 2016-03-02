#ifndef MANTID_CURVEFITTING_Bk2BkExpConvPVTEST_H_
#define MANTID_CURVEFITTING_Bk2BkExpConvPVTEST_H_

#include "MantidKernel/System.h"
#include "MantidKernel/Timer.h"
#include <cxxtest/TestSuite.h>
#include <fstream>

#include "MantidAPI/WorkspaceFactory.h"
#include "MantidCurveFitting/Algorithms/Fit.h"
#include "MantidCurveFitting/Functions/Bk2BkExpConvPV.h"
#include "MantidDataObjects/Workspace2D.h"

using namespace Mantid;
using namespace Mantid::CurveFitting;
using namespace Mantid::CurveFitting::Algorithms;
using namespace Mantid::CurveFitting::Functions;
using namespace Mantid::API;

class Bk2BkExpConvPVTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static Bk2BkExpConvPVTest *createSuite() { return new Bk2BkExpConvPVTest(); }
  static void destroySuite(Bk2BkExpConvPVTest *suite) { delete suite; }

  /*
   * Experiment data for HKL = (2, 1, 0)
   */
  Workspace_sptr genMockData() {

    const size_t size = 35;

    std::array<double, size> Xs = {
        {54999.094000, 55010.957000, 55022.820000, 55034.684000, 55046.547000,
         55058.410000, 55070.273000, 55082.137000, 55094.000000, 55105.863000,
         55117.727000, 55129.590000, 55141.453000, 55153.320000, 55165.184000,
         55177.047000, 55188.910000, 55200.773000, 55212.637000, 55224.500000,
         55236.363000, 55248.227000, 55260.090000, 55271.953000, 55283.816000,
         55295.680000, 55307.543000, 55319.406000, 55331.270000, 55343.133000,
         55354.996000, 55366.859000, 55378.727000, 55390.590000, 55402.453000}};
    std::array<double, size> Ys = {
        {2.628336, 4.034647, 6.193415, 9.507247, 14.594171, 22.402889,
         34.389721, 52.790192, 81.035973, 124.394840, 190.950440, 293.010220,
         447.602290, 664.847780, 900.438170, 1028.003700, 965.388730,
         787.024410, 603.501770, 456.122890, 344.132350, 259.611210, 195.848420,
         147.746310, 111.458510, 84.083313, 63.431709, 47.852318, 36.099365,
         27.233042, 20.544367, 15.498488, 11.690837, 8.819465, 6.653326}};

    Workspace_sptr ws =
        WorkspaceFactory::Instance().create("Workspace2D", 1, size, size);
    DataObjects::Workspace2D_sptr ws2D =
        boost::dynamic_pointer_cast<DataObjects::Workspace2D>(ws);

    MantidVec &wsX = ws2D->dataX(0);
    MantidVec &wsY = ws2D->dataY(0);
    MantidVec &wsE = ws2D->dataE(0);
    for (size_t i = 0; i < size; ++i) {
      wsX[i] = Xs[i];
      wsY[i] = Ys[i];
      wsE[i] = std::sqrt(fabs(Ys[i]));
    }

    return ws;
  }

  /*
* Test 1
*/
  void test_functionCalculator() {

    // 1. Set peak
    Bk2BkExpConvPV peak;
    peak.initialize();

    // 1. Set parameter
    peak.setParameter("Height", 1000.0);
    peak.setParameter("TOF_h", 55175.79);
    peak.setParameter("Alpha", 0.03613);
    peak.setParameter("Beta", 0.02376);
    peak.setParameter("Sigma2", 187.50514);
    peak.setParameter("Gamma", 0.0);

    // 2. Tie!
    peak.tie("TOF_h", "55175.79");
    peak.tie("Alpha", "0.03613");
    peak.tie("Beta", "0.02376");
    peak.tie("Sigma2", "187.50514");
    peak.tie("Gamma", "0.0");

    // 2. Set workspace
    auto ws = genMockData();
    DataObjects::Workspace2D_sptr ws2D =
        boost::dynamic_pointer_cast<DataObjects::Workspace2D>(ws);

    // put this workspace in the data service
    std::string wsName("Peak210WS");
    TS_ASSERT_THROWS_NOTHING(AnalysisDataService::Instance().add(wsName, ws2D));

    std::cout << "Number of data points to fit = " << ws2D->readX(0).size()
              << std::endl;

    // 3. Set fit
    Algorithms::Fit fitalg;
    fitalg.initialize();
    TS_ASSERT(fitalg.isInitialized());

    // Note: Function must be set before InputWorkspace for Fit
    fitalg.setPropertyValue("Function", peak.asString());
    fitalg.setPropertyValue("InputWorkspace", wsName);
    fitalg.setPropertyValue("WorkspaceIndex", "0");
    fitalg.setProperty("Minimizer", "Levenberg-MarquardtMD");
    fitalg.setProperty("CostFunction", "Least squares");
    fitalg.setProperty("MaxIterations", 100);

    // 4. Execute fit
    TS_ASSERT_THROWS_NOTHING(fitalg.execute());
    TS_ASSERT(fitalg.isExecuted());

    // test the output from fit is what you expect
    double chi2 = fitalg.getProperty("OutputChi2overDoF");
    TS_ASSERT(chi2 < 1.5);
    if (chi2 >= 1.5) {
      std::cout << "Chi^2 = " << chi2 << std::endl;
    }

    std::string fitStatus = fitalg.getProperty("OutputStatus");
    TS_ASSERT_EQUALS(fitStatus, "success");

    // 5. Check result
    IFunction_sptr out = fitalg.getProperty("Function");
    std::vector<std::string> parnames = out->getParameterNames();
    for (size_t ip = 0; ip < parnames.size(); ++ip) {
      if (parnames[ip].compare("TOF_h") == 0) {
        TS_ASSERT_DELTA(out->getParameter("TOF_h"), 55175.79, 1.0E-8);
      } else if (parnames[ip].compare("Height") == 0) {
        TS_ASSERT_DELTA(out->getParameter("Height"), 96000, 100);
      }
    }

    return;
  }
};

#endif /* MANTID_CURVEFITTING_Bk2BkExpConvPVTEST_H_ */
