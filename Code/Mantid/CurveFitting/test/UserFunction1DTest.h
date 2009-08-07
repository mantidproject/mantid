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
            std::vector<double>& X = ws->dataX(i);
            std::vector<double>& Y = ws->dataY(i);
            std::vector<double>& E = ws->dataE(i);
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
        FrameworkManager::Instance().deleteWorkspace("UserFunction1D_params");
        FrameworkManager::Instance().deleteWorkspace("UserFunction1D_result");
        FrameworkManager::Instance().deleteWorkspace("UserFunction1D_params1");
        FrameworkManager::Instance().deleteWorkspace("UserFunction1D_result1");
    }

    void testLinear()
    {
        IAlgorithm* alg = FrameworkManager::Instance().createAlgorithm("UserFunction1D");
        alg->initialize();
        alg->setPropertyValue("InputWorkspace","UserFunction1DWS");
        alg->setPropertyValue("WorkspaceIndex","0");
        alg->setPropertyValue("Function","a+b*x");
        alg->setPropertyValue("OutputParameters","UserFunction1D_params");
        alg->setPropertyValue("OutputWorkspace","UserFunction1D_result");
        TS_ASSERT_THROWS_NOTHING(alg->execute());

        ITableWorkspace_sptr params = boost::dynamic_pointer_cast<ITableWorkspace>(AnalysisDataService::Instance().retrieve("UserFunction1D_params"));

        TS_ASSERT_EQUALS(params->String(0,0),"a");
        TS_ASSERT_EQUALS(params->String(1,0),"b");
        TS_ASSERT_DELTA(params->Double(0,1),2,0.01);
        TS_ASSERT_DELTA(params->Double(1,1),4,0.01);

        IAlgorithm* alg1 = FrameworkManager::Instance().createAlgorithm("UserFunction1D");
        alg1->initialize();
        alg1->setPropertyValue("InputWorkspace","UserFunction1DWS");
        alg1->setPropertyValue("WorkspaceIndex","1");
        alg1->setPropertyValue("Function","a+b*x");
        alg1->setPropertyValue("OutputParameters","UserFunction1D_params1");
        alg1->setPropertyValue("OutputWorkspace","UserFunction1D_result1");
        TS_ASSERT_THROWS_NOTHING(alg1->execute());

        ITableWorkspace_sptr params1 = boost::dynamic_pointer_cast<ITableWorkspace>(AnalysisDataService::Instance().retrieve("UserFunction1D_params1"));

        TS_ASSERT_EQUALS(params1->String(0,0),"a");
        TS_ASSERT_EQUALS(params1->String(1,0),"b");
        TS_ASSERT_DELTA(params1->Double(0,1),4,0.01);
        TS_ASSERT_DELTA(params1->Double(1,1),8,0.01);

    }
};

#endif /*USERFUNCTION1DTEST_H_*/
