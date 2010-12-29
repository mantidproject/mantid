#ifndef SPLINEBACKGROUNDTEST_H_
#define SPLINEBACKGROUNDTEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidCurveFitting/SplineBackground.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidKernel/UnitFactory.h"

using namespace Mantid;
using namespace Mantid::API;

class SplineBackgroundTest : public CxxTest::TestSuite
{
public:

  void testIt()
  {
    MatrixWorkspace_sptr ws = createWS(101,0,"points");

    IAlgorithm* alg = Mantid::API::FrameworkManager::Instance().createAlgorithm("SplineBackground");
    alg->initialize();
    alg->setPropertyValue("InputWorkspace","SplineBackground_points");
    alg->setPropertyValue("OutputWorkspace","SplineBackground_out");
    alg->setPropertyValue("WorkspaceIndex","0");
    alg->execute();

    MatrixWorkspace_sptr outWS = boost::dynamic_pointer_cast<MatrixWorkspace>
      (AnalysisDataService::Instance().retrieve("SplineBackground_out"));

    const MantidVec& X = outWS->readX(0);
    const MantidVec& Y = outWS->readY(0);

    for(int i=0;i<outWS->blocksize();i++)
    {
      TS_ASSERT_DELTA(Y[i],sin(X[i]),0.2);
    }
    TS_ASSERT( outWS->getAxis(0)->unit() == ws->getAxis(0)->unit() );
  }
private:

  MatrixWorkspace_sptr createWS(int n,int dn,const std::string& name)
  {
    FrameworkManager::Instance();
    Mantid::DataObjects::Workspace2D_sptr ws = boost::dynamic_pointer_cast<Mantid::DataObjects::Workspace2D>
      (WorkspaceFactory::Instance().create("Workspace2D",1,n+dn,n));

    Mantid::MantidVec& X = ws->dataX(0);
    Mantid::MantidVec& Y = ws->dataY(0);
    Mantid::MantidVec& E = ws->dataE(0);

    double dx = 0.1;

    for(int k=0;k<n;k++)
    {
      X[k] = dx*k;
      Y[k] = sin(X[k]) + 0.1*(-.5 + double(rand())/RAND_MAX);
      E[k] = 1.;
    }

    if (dn > 0)
      X[n] = X[n-1] + dx;

    // Mask some bins out to test that functionality
    int toMask = static_cast<int>(0.75*n);
    ws->maskBin(0,toMask-1);
    ws->maskBin(0,toMask);
    ws->maskBin(0,toMask+1);
    
    ws->getAxis(0)->unit() = Kernel::UnitFactory::Instance().create("TOF");

    AnalysisDataService::Instance().add("SplineBackground_"+name,ws);

    return ws;
  }

};

#endif /*SPLINEBACKGROUNDTEST_H_*/
