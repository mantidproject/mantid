#ifndef LORENTZIANTEST_H_
#define LORENTZIANTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidCurveFitting/Lorentzian.h"
#include "MantidAPI/CompositeFunction.h"
#include "MantidCurveFitting/LinearBackground.h"
#include "MantidCurveFitting/BoundaryConstraint.h"
#include "MantidCurveFitting/Fit.h"
#include "MantidKernel/UnitFactory.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidAPI/Algorithm.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidDataHandling/LoadRaw.h"
#include "MantidKernel/Exception.h"
#include "MantidAPI/FunctionFactory.h"

using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace Mantid::CurveFitting;
using namespace Mantid::DataObjects;
using namespace Mantid::DataHandling;


class LorentzianTest : public CxxTest::TestSuite
{
public:

  void getMockData(Mantid::MantidVec& y, Mantid::MantidVec& e)
  {
    y[0] =     4.1733;
    y[1] =     4.3729;
    y[2] =     4.8150;
    y[3] =     5.3402;
    y[4] =     6.0909;
    y[5] =     7.3389;
    y[6] =     9.4883;
    y[7] =    13.6309;
    y[8] =    23.1555;
    y[9] =    48.9471;
    y[10] =  100.4982;
    y[11] =   68.8164;
    y[12] =   30.3590;
    y[13] =   16.4184;
    y[14] =   10.7455;
    y[15] =    8.0570;
    y[16] =    6.5158;
    y[17] =    5.5496;
    y[18] =    5.0087;
    y[19] =    4.5027;

    for (int i = 0; i <=19; i++)
      y[i] -= 3.017;

    e[0] =       2.0429;
    e[1] =       2.0911;
    e[2] =       2.1943;
    e[3] =       2.3109;
    e[4] =       2.4680;
    e[5] =       2.7090;
    e[6] =       3.0803;
    e[7] =       3.6920;
    e[8] =       4.8120;
    e[9] =       6.9962;
    e[10] =     10.0249;
    e[11] =      8.2956;
    e[12] =      5.5099;
    e[13] =      4.0520;
    e[14] =      3.2780;
    e[15] =      2.8385;
    e[16] =      2.5526;
    e[17] =      2.3558;
    e[18] =      2.2380;
    e[19] =      2.1220;
  }

  void testAgainstMockDataConstBackground()
  {
    Fit alg2;
    TS_ASSERT_THROWS_NOTHING(alg2.initialize());
    TS_ASSERT( alg2.isInitialized() );

    // create mock data to test against
    std::string wsName = "LorentzianMockData";
    int histogramNumber = 1;
    int timechannels = 20;
    Workspace_sptr ws = WorkspaceFactory::Instance().create("Workspace2D",histogramNumber,timechannels,timechannels);
    Workspace2D_sptr ws2D = boost::dynamic_pointer_cast<Workspace2D>(ws);
    for (int i = 0; i < 20; i++) ws2D->dataX(0)[i] = i+1;
    Mantid::MantidVec& y = ws2D->dataY(0); // y-values (counts)
    Mantid::MantidVec& e = ws2D->dataE(0); // error values of counts
    getMockData(y, e);

    //put this workspace in the data service
    TS_ASSERT_THROWS_NOTHING(AnalysisDataService::Instance().addOrReplace(wsName, ws2D));

    // create function you want to fit against
    CompositeFunction *fnWithBk = new CompositeFunction();

    LinearBackground *bk = new LinearBackground();
    bk->initialize();

    bk->setParameter("A0",0.0);
    bk->setParameter("A1",0.0);
    bk->removeActive(1);  

    // set up Lorentzian fitting function
    Lorentzian* fn = new Lorentzian();
    fn->initialize();
    fn->setCentre(11.2);
    fn->setHeight(100.7);
    fn->setWidth(2.2);

    fnWithBk->addFunction(fn);
    fnWithBk->addFunction(bk);

    //alg2.setFunction(fnWithBk);    
    alg2.setPropertyValue("Function",*fnWithBk);


    // Set which spectrum to fit against and initial starting values
    alg2.setPropertyValue("InputWorkspace", wsName);
    alg2.setPropertyValue("WorkspaceIndex","0");
    alg2.setPropertyValue("StartX","0");
    alg2.setPropertyValue("EndX","20");

    // execute fit
   TS_ASSERT_THROWS_NOTHING(
      TS_ASSERT( alg2.execute() )
    )

    TS_ASSERT( alg2.isExecuted() );

    // test the output from fit is what you expect
    double dummy = alg2.getProperty("Output Chi^2/DoF");
    TS_ASSERT_DELTA( dummy, 0.0001,0.0001);

    IFunction *out = FunctionFactory::Instance().createInitialized(alg2.getPropertyValue("Function"));
    IPeakFunction *pk = dynamic_cast<IPeakFunction *>(dynamic_cast<CompositeFunction*>(out)->getFunction(0));
    TS_ASSERT_DELTA( pk->height(), 100.6960 ,0.0001);
    TS_ASSERT_DELTA( pk->centre(), 11.1994 ,0.0001);
    TS_ASSERT_DELTA( pk->width(), 2.1975 ,0.0001);
    TS_ASSERT_DELTA( out->getParameter("f1.A0"), -0.0028 ,0.0001);
    TS_ASSERT_DELTA( out->getParameter("f1.A1"), 0.0005 ,0.0001);

    AnalysisDataService::Instance().remove(wsName);

  }

  void testAgainstMockDataWithConstraint()
  {
    Fit alg2;
    TS_ASSERT_THROWS_NOTHING(alg2.initialize());
    TS_ASSERT( alg2.isInitialized() );

    // create mock data to test against
    std::string wsName = "LorentzianMockData";
    int histogramNumber = 1;
    int timechannels = 20;
    Workspace_sptr ws = WorkspaceFactory::Instance().create("Workspace2D",histogramNumber,timechannels,timechannels);
    Workspace2D_sptr ws2D = boost::dynamic_pointer_cast<Workspace2D>(ws);
    for (int i = 0; i < 20; i++) ws2D->dataX(0)[i] = i+1;
    Mantid::MantidVec& y = ws2D->dataY(0); // y-values (counts)
    Mantid::MantidVec& e = ws2D->dataE(0); // error values of counts
    getMockData(y, e);

    //put this workspace in the data service
    TS_ASSERT_THROWS_NOTHING(AnalysisDataService::Instance().add(wsName, ws2D));

    // set up Lorentzian fitting function
    Lorentzian* fn = new Lorentzian();
    fn->initialize();
    fn->setCentre(11.2);
    fn->setHeight(100.7);
    fn->setWidth(2.2);

    // add constraint to function
    BoundaryConstraint* bc = new BoundaryConstraint(fn,"PeakCentre",11.3, 12.0);
    fn->addConstraint(bc);

    //void setFunction(API::IFunction* fun);
    //alg2.setFunction(fn);
    alg2.setPropertyValue("Function",*fn);


    // Set which spectrum to fit against and initial starting values
    alg2.setPropertyValue("InputWorkspace", wsName);
    alg2.setPropertyValue("WorkspaceIndex","0");
    alg2.setPropertyValue("StartX","0");
    alg2.setPropertyValue("EndX","20");

    // execute fit
    TS_ASSERT_THROWS_NOTHING(
      TS_ASSERT( alg2.execute() )
    )

    TS_ASSERT( alg2.isExecuted() );

    // test the output from fit is what you expect
    double dummy = alg2.getProperty("Output Chi^2/DoF");
    TS_ASSERT_DELTA( dummy, 0.08,0.01);

    IFunction *out = FunctionFactory::Instance().createInitialized(alg2.getPropertyValue("Function"));
    IPeakFunction *pk = dynamic_cast<IPeakFunction *>(out);

    TS_ASSERT_DELTA( pk->height(), 100.7 ,0.0001);
    TS_ASSERT_DELTA( pk->centre(), 11.3 ,0.01);
    TS_ASSERT_DELTA( pk->width(), 2.1999 ,0.0001);

    AnalysisDataService::Instance().remove(wsName);
  }

  void testAgainstMockDataWithConstraintAndConstBackground()
  {
    Fit alg2;
    TS_ASSERT_THROWS_NOTHING(alg2.initialize());
    TS_ASSERT( alg2.isInitialized() );

    // create mock data to test against
    std::string wsName = "LorentzianMockData";
    int histogramNumber = 1;
    int timechannels = 20;
    Workspace_sptr ws = WorkspaceFactory::Instance().create("Workspace2D",histogramNumber,timechannels,timechannels);
    Workspace2D_sptr ws2D = boost::dynamic_pointer_cast<Workspace2D>(ws);
    for (int i = 0; i < 20; i++) ws2D->dataX(0)[i] = i+1;
    Mantid::MantidVec& y = ws2D->dataY(0); // y-values (counts)
    Mantid::MantidVec& e = ws2D->dataE(0); // error values of counts
    getMockData(y, e);

    //put this workspace in the data service
    TS_ASSERT_THROWS_NOTHING(AnalysisDataService::Instance().add(wsName, ws2D));

    // create function you want to fit against
    CompositeFunction *fnWithBk = new CompositeFunction();

    LinearBackground *bk = new LinearBackground();
    bk->initialize();

    bk->setParameter("A0",0.0);
    bk->setParameter("A1",0.0);
    //bk->removeActive(0);  
    bk->removeActive(1);

    // set up Lorentzian fitting function
    Lorentzian* fn = new Lorentzian();
    fn->initialize();
    fn->setCentre(11.2);
    fn->setHeight(100.7);
    fn->setWidth(2.2);

    // add constraint to function
    BoundaryConstraint* bc = new BoundaryConstraint(fn,"PeakCentre",11.3, 12.0);
    fn->addConstraint(bc);

    fnWithBk->addFunction(fn);
    fnWithBk->addFunction(bk);

    //alg2.setFunction(fnWithBk);
    alg2.setPropertyValue("Function",*fnWithBk);


    // Set which spectrum to fit against and initial starting values
    alg2.setPropertyValue("InputWorkspace", wsName);
    alg2.setPropertyValue("WorkspaceIndex","0");
    alg2.setPropertyValue("StartX","0");
    alg2.setPropertyValue("EndX","20");

    // execute fit
    TS_ASSERT_THROWS_NOTHING(
      TS_ASSERT( alg2.execute() )
    )

    TS_ASSERT( alg2.isExecuted() );

    // test the output from fit is what you expect
    double dummy = alg2.getProperty("Output Chi^2/DoF");
    TS_ASSERT_DELTA( dummy, 0.09,0.01);

    IFunction *out = FunctionFactory::Instance().createInitialized(alg2.getPropertyValue("Function"));
    IPeakFunction *pk = dynamic_cast<IPeakFunction *>(dynamic_cast<CompositeFunction*>(out)->getFunction(0));
    TS_ASSERT_DELTA( pk->height(), 100.7 ,0.0001);
    TS_ASSERT_DELTA( pk->centre(), 11.3 ,0.01);
    TS_ASSERT_DELTA( pk->width(), 2.1999 ,0.0001);
    TS_ASSERT_DELTA( out->getParameter("f1.A0"), 0.0 ,0.01);
    TS_ASSERT_DELTA( out->getParameter("f1.A1"), 0.0 ,0.01);

    AnalysisDataService::Instance().remove(wsName);

  }


};

#endif /*LORENTZIANTEST_H_*/
