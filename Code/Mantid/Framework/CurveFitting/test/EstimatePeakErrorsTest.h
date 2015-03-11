#ifndef ESTIMATEPEAKERRORSTEST_H_
#define ESTIMATEPEAKERRORSTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidCurveFitting/EstimatePeakErrors.h"

#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/FunctionDomain1D.h"
#include "MantidAPI/FunctionFactory.h"
#include "MantidAPI/FunctionValues.h"
#include "MantidAPI/IFunction.h"
#include "MantidAPI/WorkspaceFactory.h"

#include "MantidTestHelpers/WorkspaceCreationHelper.h"

using namespace Mantid;
using namespace Mantid::API;
using namespace Mantid::CurveFitting;
using namespace WorkspaceCreationHelper;

class EstimatePeakErrorsTest : public CxxTest::TestSuite
{
public:

  void test_on_Gaussian()
  {
    auto fun = FunctionFactory::Instance().createInitialized("name=Gaussian,PeakCentre=0,Height=1,Sigma=2");
    auto ws = createWorkspace(*fun);

    auto fit = AlgorithmManager::Instance().create("Fit");
    fit->setProperty("Function",fun);
    fit->setProperty("InputWorkspace",ws);
    fit->setProperty("CalcErrors",true);
    fit->execute();

    EstimatePeakErrors alg;
    alg.initialize();
    alg.setProperty("Function",fun);
    alg.setPropertyValue("OutputWorkspace","Errors");
    alg.execute();

    auto res = AnalysisDataService::Instance().retrieveWS<ITableWorkspace>("Errors");
    TS_ASSERT_EQUALS( res->rowCount(), 4 );

    TS_ASSERT_EQUALS( res->cell<std::string>(0,0), "Centre" );
    TS_ASSERT_EQUALS( res->cell<std::string>(1,0), "Height" );
    TS_ASSERT_EQUALS( res->cell<std::string>(2,0), "FWHM" );
    TS_ASSERT_EQUALS( res->cell<std::string>(3,0), "Intensity" );

    TS_ASSERT_DELTA( res->cell<double>(0,1), -0.0068, 1e-4 );
    TS_ASSERT_DELTA( res->cell<double>(1,1), 1.0036, 1e-4 );
    TS_ASSERT_DELTA( res->cell<double>(2,1), 4.8046, 1e-4 );
    TS_ASSERT_DELTA( res->cell<double>(3,1), 5.1330, 1e-4 );

    TS_ASSERT_DELTA( res->cell<double>(0,2), 0.7467, 1e-4 );
    TS_ASSERT_DELTA( res->cell<double>(1,2), 0.3172, 1e-4 );
    TS_ASSERT_DELTA( res->cell<double>(2,2), 1.7598, 1e-4 );
    TS_ASSERT_DELTA( res->cell<double>(3,2), 1.6263, 1e-4 );

    AnalysisDataService::Instance().clear();
  }

  void test_on_Gaussian_ties()
  {
    auto fun = FunctionFactory::Instance().createInitialized("name=Gaussian,PeakCentre=0,Height=1,Sigma=2,ties=(Sigma=2)");
    auto ws = createWorkspace(*fun);

    auto fit = AlgorithmManager::Instance().create("Fit");
    fit->setProperty("Function",fun);
    fit->setProperty("InputWorkspace",ws);
    fit->setProperty("CalcErrors",true);
    fit->execute();

    EstimatePeakErrors alg;
    alg.initialize();
    alg.setProperty("Function",fun);
    alg.setPropertyValue("OutputWorkspace","Errors");
    alg.execute();

    auto res = AnalysisDataService::Instance().retrieveWS<ITableWorkspace>("Errors");
    TS_ASSERT_EQUALS( res->rowCount(), 4 );

    TS_ASSERT_EQUALS( res->cell<std::string>(0,0), "Centre" );
    TS_ASSERT_EQUALS( res->cell<std::string>(1,0), "Height" );
    TS_ASSERT_EQUALS( res->cell<std::string>(2,0), "FWHM" );
    TS_ASSERT_EQUALS( res->cell<std::string>(3,0), "Intensity" );

    TS_ASSERT_DELTA( res->cell<double>(0,1), -0.0071, 1e-4 );
    TS_ASSERT_DELTA( res->cell<double>(1,1), 1.0136, 1e-4 );
    TS_ASSERT_DELTA( res->cell<double>(2,1), 4.7096, 1e-4 );
    TS_ASSERT_DELTA( res->cell<double>(3,1), 5.0816, 1e-4 );

    TS_ASSERT_DELTA( res->cell<double>(0,2), 0.7327, 1e-4 );
    TS_ASSERT_DELTA( res->cell<double>(1,2), 0.2625, 1e-4 );
    TS_ASSERT_DELTA( res->cell<double>(2,2), 0.0000, 1e-14 );
    TS_ASSERT_DELTA( res->cell<double>(3,2), 1.3164, 1e-4 );

    AnalysisDataService::Instance().clear();
  }

  void test_on_Gaussian_unfitted()
  {
    auto fun = FunctionFactory::Instance().createInitialized("name=Gaussian,PeakCentre=0,Height=1,Sigma=2");

    EstimatePeakErrors alg;
    alg.initialize();
    alg.setProperty("Function",fun);
    alg.setPropertyValue("OutputWorkspace","Errors");
    alg.execute();

    auto res = AnalysisDataService::Instance().retrieveWS<ITableWorkspace>("Errors");
    TS_ASSERT_EQUALS( res->rowCount(), 0 );

    AnalysisDataService::Instance().clear();
  }


  void test_on_Lorentzians()
  {
    std::string funStr = "name=Lorentzian,Amplitude=10,PeakCentre=-4,FWHM=2;"
      "name=Lorentzian,Amplitude=10,PeakCentre=3,FWHM=3;"
      "name=FlatBackground,A0=3";
    auto fun = FunctionFactory::Instance().createInitialized(funStr);
    auto ws = createWorkspace(*fun);

    auto fit = AlgorithmManager::Instance().create("Fit");
    fit->setProperty("Function",fun);
    fit->setProperty("InputWorkspace",ws);
    fit->setProperty("CalcErrors",true);
    fit->execute();

    EstimatePeakErrors alg;
    alg.initialize();
    alg.setProperty("Function",fun);
    alg.setPropertyValue("OutputWorkspace","Errors");
    alg.execute();

    auto res = AnalysisDataService::Instance().retrieveWS<ITableWorkspace>("Errors");
    TS_ASSERT_EQUALS( res->rowCount(), 8 );

    TS_ASSERT_EQUALS( res->cell<std::string>(0,0), "f0.Centre" );
    TS_ASSERT_EQUALS( res->cell<std::string>(1,0), "f0.Height" );
    TS_ASSERT_EQUALS( res->cell<std::string>(2,0), "f0.FWHM" );
    TS_ASSERT_EQUALS( res->cell<std::string>(3,0), "f0.Intensity" );

    TS_ASSERT_DELTA( res->cell<double>(0,1), -3.9865, 1e-4 );
    TS_ASSERT_DELTA( res->cell<double>(1,1), 3.1881, 1e-4 );
    TS_ASSERT_DELTA( res->cell<double>(2,1), 2.0011, 1e-4 );
    TS_ASSERT_DELTA( res->cell<double>(3,1), 9.3859, 1e-4 );

    TS_ASSERT_DELTA( res->cell<double>(0,2), 0.1764, 1e-4 );
    TS_ASSERT_DELTA( res->cell<double>(1,2), 0.5690, 1e-4 );
    TS_ASSERT_DELTA( res->cell<double>(2,2), 0.5969, 1e-4 );
    TS_ASSERT_DELTA( res->cell<double>(3,2), 2.4468, 1e-4 );

    TS_ASSERT_EQUALS( res->cell<std::string>(4,0), "f1.Centre" );
    TS_ASSERT_EQUALS( res->cell<std::string>(5,0), "f1.Height" );
    TS_ASSERT_EQUALS( res->cell<std::string>(6,0), "f1.FWHM" );
    TS_ASSERT_EQUALS( res->cell<std::string>(7,0), "f1.Intensity" );

    TS_ASSERT_DELTA( res->cell<double>(4,1), 3.0064, 1e-4 );
    TS_ASSERT_DELTA( res->cell<double>(5,1), 2.1327, 1e-4 );
    TS_ASSERT_DELTA( res->cell<double>(6,1), 2.9908, 1e-4 );
    TS_ASSERT_DELTA( res->cell<double>(7,1), 9.3838, 1e-4 );

    TS_ASSERT_DELTA( res->cell<double>(4,2), 0.3234, 1e-4 );
    TS_ASSERT_DELTA( res->cell<double>(5,2), 0.4756, 1e-4 );
    TS_ASSERT_DELTA( res->cell<double>(6,2), 1.2002, 1e-4 );
    TS_ASSERT_DELTA( res->cell<double>(7,2), 3.5530, 1e-4 );

    AnalysisDataService::Instance().clear();
  }

  void test_on_Lorentzians_ties()
  {
    std::string funStr = "name=Lorentzian,Amplitude=10,FWHM=2,ties=(PeakCentre=-4);"
      "name=Lorentzian,Amplitude=10,PeakCentre=3,FWHM=3;"
      "name=FlatBackground,A0=3;ties=(f1.Amplitude=f0.Amplitude)";
    auto fun = FunctionFactory::Instance().createInitialized(funStr);
    auto ws = createWorkspace(*fun);

    auto fit = AlgorithmManager::Instance().create("Fit");
    fit->setProperty("Function",fun);
    fit->setProperty("InputWorkspace",ws);
    fit->setProperty("CalcErrors",true);
    fit->execute();

    EstimatePeakErrors alg;
    alg.initialize();
    alg.setProperty("Function",fun);
    alg.setPropertyValue("OutputWorkspace","Errors");
    alg.execute();

    auto res = AnalysisDataService::Instance().retrieveWS<ITableWorkspace>("Errors");
    TS_ASSERT_EQUALS( res->rowCount(), 8 );

    TS_ASSERT_EQUALS( res->cell<std::string>(0,0), "f0.Centre" );
    TS_ASSERT_EQUALS( res->cell<std::string>(1,0), "f0.Height" );
    TS_ASSERT_EQUALS( res->cell<std::string>(2,0), "f0.FWHM" );
    TS_ASSERT_EQUALS( res->cell<std::string>(3,0), "f0.Intensity" );

    TS_ASSERT_DELTA( res->cell<double>(0,1), -4.0000, 1e-4 );
    TS_ASSERT_DELTA( res->cell<double>(1,1), 3.1877, 1e-4 );
    TS_ASSERT_DELTA( res->cell<double>(2,1), 2.0012, 1e-4 );
    TS_ASSERT_DELTA( res->cell<double>(3,1), 9.3849, 1e-4 );

    TS_ASSERT_DELTA( res->cell<double>(0,2), 0.0000, 1e-4 );
    TS_ASSERT_DELTA( res->cell<double>(1,2), 0.5609, 1e-4 );
    TS_ASSERT_DELTA( res->cell<double>(2,2), 0.5797, 1e-4 );
    TS_ASSERT_DELTA( res->cell<double>(3,2), 2.2561, 1e-4 );

    TS_ASSERT_EQUALS( res->cell<std::string>(4,0), "f1.Centre" );
    TS_ASSERT_EQUALS( res->cell<std::string>(5,0), "f1.Height" );
    TS_ASSERT_EQUALS( res->cell<std::string>(6,0), "f1.FWHM" );
    TS_ASSERT_EQUALS( res->cell<std::string>(7,0), "f1.Intensity" );

    TS_ASSERT_DELTA( res->cell<double>(4,1), 3.0056, 1e-4 );
    TS_ASSERT_DELTA( res->cell<double>(5,1), 2.1320, 1e-4 );
    TS_ASSERT_DELTA( res->cell<double>(6,1), 2.9921, 1e-4 );
    TS_ASSERT_DELTA( res->cell<double>(7,1), 9.3849, 1e-4 );

    TS_ASSERT_DELTA( res->cell<double>(4,2), 0.3231, 1e-4 );
    TS_ASSERT_DELTA( res->cell<double>(5,2), 0.4668, 1e-4 );
    TS_ASSERT_DELTA( res->cell<double>(6,2), 0.6551, 1e-4 );
    TS_ASSERT_DELTA( res->cell<double>(7,2), 0.0000, 1e-4 );

    AnalysisDataService::Instance().clear();
  }


  void test_no_peaks()
  {
    std::string funStr = "name=FlatBackground,A0=3";
    auto fun = FunctionFactory::Instance().createInitialized(funStr);
    auto ws = createWorkspace(*fun);

    auto fit = AlgorithmManager::Instance().create("Fit");
    fit->setProperty("Function",fun);
    fit->setProperty("InputWorkspace",ws);
    fit->setProperty("CalcErrors",true);
    fit->execute();

    EstimatePeakErrors alg;
    alg.initialize();
    alg.setProperty("Function",fun);
    alg.setPropertyValue("OutputWorkspace","Errors");
    alg.execute();

    auto res = AnalysisDataService::Instance().retrieveWS<ITableWorkspace>("Errors");
    TS_ASSERT_EQUALS( res->rowCount(), 0 );

    AnalysisDataService::Instance().clear();
  }

private:

  MatrixWorkspace_sptr createWorkspace(const IFunction& fun) const
  {
    size_t n = 100;
    auto ws = WorkspaceFactory::Instance().create("Workspace2D", 1, n, n);
    FunctionDomain1DVector x(-10,10,n);
    FunctionValues y(x);
    std::vector<double> e(n,1.0);

    fun.function(x,y);
    ws->setX(0,x.toVector());
    ws->getSpectrum(0)->setData(y.toVector(),e);
    addNoise(ws,0.1);
    return ws;
  }

};

#endif /*CHEBYSHEVTEST_H_*/
