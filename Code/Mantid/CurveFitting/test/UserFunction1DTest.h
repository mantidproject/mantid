#ifndef USERFUNCTION1DTEST_H_
#define USERFUNCTION1DTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidCurveFitting/UserFunction1D.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidAPI/AnalysisDataService.h"

using namespace Mantid::CurveFitting;
using namespace Mantid::API;

class UserFunction1DTest : public CxxTest::TestSuite
{
public:
    UserFunction1DTest()
    {
      FrameworkManager::Instance();
      Mantid::DataObjects::Workspace2D_sptr ws = boost::dynamic_pointer_cast<Mantid::DataObjects::Workspace2D>
        (WorkspaceFactory::Instance().create("Workspace2D",3,10,10));
      for(int i=0;i<3;i++)
      {
        Mantid::MantidVec& X = ws->dataX(i);
        Mantid::MantidVec& Y = ws->dataY(i);
        Mantid::MantidVec& E = ws->dataE(i);
        for(int j=0;j<10;j++)
        {
          X[j] = 1.*j;
          Y[j] = (i+1)*(2. + 4.*X[j]);
          E[j] = 1.;
        }
      }
      AnalysisDataService::Instance().add("UserFunction1DWS",ws);
    }

    ~UserFunction1DTest()
    {
        FrameworkManager::Instance().deleteWorkspace("UserFunction1DWS");
        FrameworkManager::Instance().deleteWorkspace("UserFunction1D_Parameters");
        FrameworkManager::Instance().deleteWorkspace("UserFunction1D_Workspace");
        FrameworkManager::Instance().deleteWorkspace("UserFunction1D1_Parameters");
        FrameworkManager::Instance().deleteWorkspace("UserFunction1D1_Workspace");
    }

    void testLinear()
    {
        IAlgorithm* alg = FrameworkManager::Instance().createAlgorithm("UserFunction1D");
        alg->initialize();
        alg->setPropertyValue("InputWorkspace","UserFunction1DWS");
        alg->setPropertyValue("WorkspaceIndex","0");
        alg->setPropertyValue("Function","a+b*x");
        alg->setPropertyValue("Output","UserFunction1D");
        TS_ASSERT_THROWS_NOTHING(alg->execute());

        ITableWorkspace_sptr params = boost::dynamic_pointer_cast<ITableWorkspace>(AnalysisDataService::Instance().retrieve("UserFunction1D_Parameters"));

        TS_ASSERT_EQUALS(params->String(0,0),"Chi^2/DoF");
        TS_ASSERT_EQUALS(params->String(1,0),"a");
        TS_ASSERT_EQUALS(params->String(2,0),"b");
        TS_ASSERT_DELTA(params->Double(0,1),0,0.01);
        TS_ASSERT_DELTA(params->Double(1,1),2,0.01);
        TS_ASSERT_DELTA(params->Double(2,1),4,0.01);

        IAlgorithm* alg1 = FrameworkManager::Instance().createAlgorithm("UserFunction1D");
        alg1->initialize();
        alg1->setPropertyValue("InputWorkspace","UserFunction1DWS");
        alg1->setPropertyValue("WorkspaceIndex","1");
        alg1->setPropertyValue("Function","a+b*x");
        alg1->setPropertyValue("Output","UserFunction1D1");
        TS_ASSERT_THROWS_NOTHING(alg1->execute());

        ITableWorkspace_sptr params1 = boost::dynamic_pointer_cast<ITableWorkspace>(AnalysisDataService::Instance().retrieve("UserFunction1D1_Parameters"));

        TS_ASSERT_EQUALS(params1->String(0,0),"Chi^2/DoF");
        TS_ASSERT_EQUALS(params1->String(1,0),"a");
        TS_ASSERT_EQUALS(params1->String(2,0),"b");
        TS_ASSERT_DELTA(params1->Double(1,1),4,0.01);
        TS_ASSERT_DELTA(params1->Double(2,1),8,0.01);

    }
};

#endif /*USERFUNCTION1DTEST_H_*/
