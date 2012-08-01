#ifndef MANTID_CURVEFITTING_Bk2BkExpConvPVTEST_H_
#define MANTID_CURVEFITTING_Bk2BkExpConvPVTEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidKernel/Timer.h"
#include "MantidKernel/System.h"
#include <iostream>
#include <iomanip>
#include <fstream>

#include "MantidCurveFitting/Bk2BkExpConvPV.h"
#include "MantidCurveFitting/Fit.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidDataObjects/Workspace2D.h"

using namespace Mantid;
using namespace Mantid::CurveFitting;
using namespace Mantid::API;

class Bk2BkExpConvPVTest : public CxxTest::TestSuite
{
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static Bk2BkExpConvPVTest *createSuite() { return new Bk2BkExpConvPVTest(); }
  static void destroySuite( Bk2BkExpConvPVTest *suite ) { delete suite; }

  /*
   * Experiment data for HKL = (2, 1, 0)
   */
  void getMockData(std::vector<double>& xvalues, std::vector<double>& yvalues)
  {
    xvalues.clear();
    yvalues.clear();

    xvalues.push_back(54999.094);       yvalues.push_back(2.6283364);
    xvalues.push_back(55010.957);       yvalues.push_back(4.0346470);
    xvalues.push_back(55022.820);       yvalues.push_back(6.1934152);
    xvalues.push_back(55034.684);       yvalues.push_back(9.5072470);
    xvalues.push_back(55046.547);       yvalues.push_back(14.594171);
    xvalues.push_back(55058.410);       yvalues.push_back(22.402889);
    xvalues.push_back(55070.273);       yvalues.push_back(34.389721);
    xvalues.push_back(55082.137);       yvalues.push_back(52.790192);
    xvalues.push_back(55094.000);       yvalues.push_back(81.035973);
    xvalues.push_back(55105.863);       yvalues.push_back(124.39484);
    xvalues.push_back(55117.727);       yvalues.push_back(190.95044);
    xvalues.push_back(55129.590);       yvalues.push_back(293.01022);
    xvalues.push_back(55141.453);       yvalues.push_back(447.60229);
    xvalues.push_back(55153.320);       yvalues.push_back(664.84778);
    xvalues.push_back(55165.184);       yvalues.push_back(900.43817);
    xvalues.push_back(55177.047);       yvalues.push_back(1028.0037);
    xvalues.push_back(55188.910);       yvalues.push_back(965.38873);
    xvalues.push_back(55200.773);       yvalues.push_back(787.02441);
    xvalues.push_back(55212.637);       yvalues.push_back(603.50177);
    xvalues.push_back(55224.500);       yvalues.push_back(456.12289);
    xvalues.push_back(55236.363);       yvalues.push_back(344.13235);
    xvalues.push_back(55248.227);       yvalues.push_back(259.61121);
    xvalues.push_back(55260.090);       yvalues.push_back(195.84842);
    xvalues.push_back(55271.953);       yvalues.push_back(147.74631);
    xvalues.push_back(55283.816);       yvalues.push_back(111.45851);
    xvalues.push_back(55295.680);       yvalues.push_back(84.083313);
    xvalues.push_back(55307.543);       yvalues.push_back(63.431709);
    xvalues.push_back(55319.406);       yvalues.push_back(47.852318);
    xvalues.push_back(55331.270);       yvalues.push_back(36.099365);
    xvalues.push_back(55343.133);       yvalues.push_back(27.233042);
    xvalues.push_back(55354.996);       yvalues.push_back(20.544367);
    xvalues.push_back(55366.859);       yvalues.push_back(15.498488);
    xvalues.push_back(55378.727);       yvalues.push_back(11.690837);
    xvalues.push_back(55390.590);       yvalues.push_back(8.8194647);
    xvalues.push_back(55402.453);       yvalues.push_back(6.6533256);

    return;
  }

  /*
   * Test 1
   */
  void test_functionCalculator()
  {

    // 1. Set peak
    Bk2BkExpConvPV peak;
    peak.initialize();

    // 1. Set parameter
    peak.setParameter("height", 1000.0);
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
    std::vector<double> Xs;
    std::vector<double> Ys;
    getMockData(Xs, Ys);

    size_t histogramNumber = 1;
    size_t timechannels = Xs.size();
    Workspace_sptr ws = WorkspaceFactory::Instance().create("Workspace2D", histogramNumber, timechannels, timechannels);
    DataObjects::Workspace2D_sptr ws2D = boost::dynamic_pointer_cast<DataObjects::Workspace2D>(ws);

    MantidVec& wsX = ws2D->dataX(0);
    MantidVec& wsY = ws2D->dataY(0);
    MantidVec& wsE = ws2D->dataE(0);
    for (size_t i = 0; i < timechannels; ++i)
    {
      wsX[i] = Xs[i];
      wsY[i] = Ys[i];
      wsE[i] = std::sqrt(fabs(Ys[i]));
    }

    //put this workspace in the data service
    std::string wsName("Peak210WS");
    TS_ASSERT_THROWS_NOTHING(AnalysisDataService::Instance().add(wsName, ws2D));

    std::cout << "Number of data points to fit = " << ws2D->readX(0).size() << std::endl;

    // 3. Set fit
    Fit fitalg;
    fitalg.initialize();
    TS_ASSERT(fitalg.isInitialized());

    // Note: Function must be set before InputWorkspace for Fit
    fitalg.setPropertyValue("Function", peak.asString());
    fitalg.setPropertyValue("InputWorkspace", wsName);
    fitalg.setPropertyValue("WorkspaceIndex","0");
    fitalg.setProperty("StartX", wsX[0]);
    fitalg.setProperty("EndX", wsX.back());
    fitalg.setProperty("Minimizer", "Levenberg-MarquardtMD");
    fitalg.setProperty("CostFunction", "Least squares");
    fitalg.setProperty("MaxIterations", 100);

    // 4. Execute fit
    TS_ASSERT_THROWS_NOTHING(fitalg.execute());
    TS_ASSERT(fitalg.isExecuted());

    // test the output from fit is what you expect
    double chi2 = fitalg.getProperty("OutputChi2overDoF");
    TS_ASSERT(chi2 < 1.5);
    if (chi2 >= 1.5)
    {
      std::cout << "Chi^2 = " << chi2 << std::endl;
    }

    std::string fitStatus = fitalg.getProperty("OutputStatus");
    TS_ASSERT_EQUALS(fitStatus, "success");

    // 5. Check result
    IFunction_sptr out = fitalg.getProperty("Function");
    std::vector<std::string> parnames = out->getParameterNames();
    for (size_t ip = 0; ip < parnames.size(); ++ip)
    {
      if (parnames[ip].compare("TOF_h") == 0)
      {
        TS_ASSERT_DELTA(out->getParameter("TOF_h"), 55175.79, 1.0E-8);
      }
      else if (parnames[ip].compare("height") == 0)
      {
        TS_ASSERT_DELTA(out->getParameter("height"), 96000, 100);
      }
    }


    return;
  }


};


#endif /* MANTID_CURVEFITTING_Bk2BkExpConvPVTEST_H_ */
