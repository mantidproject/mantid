#ifndef SPLINEBACKGROUNDTEST_H_
#define SPLINEBACKGROUNDTEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidCurveFitting/SplineBackground.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidKernel/UnitFactory.h"

#include "MantidTestHelpers/WorkspaceCreationHelper.h"

using namespace Mantid;
using namespace Mantid::API;

class SplineBackgroundTest : public CxxTest::TestSuite
{
private:
  struct SinFunction
  {
    double operator()(double x, int)
    {
      return std::sin(x);
    }
  };

public:

  void testIt()
  {
    auto ws = WorkspaceCreationHelper::Create2DWorkspaceFromFunction(SinFunction(),1,0.1,10.1,0.1,true);
    WorkspaceCreationHelper::addNoise(ws, 0.1);
    // Mask some bins out to test that functionality
    const size_t nbins = 101;
    int toMask = static_cast<int>(0.75*nbins);
    ws->maskBin(0,toMask-1);
    ws->maskBin(0,toMask);
    ws->maskBin(0,toMask+1);
    ws->getAxis(0)->unit() = Kernel::UnitFactory::Instance().create("TOF");
    const std::string wsName = "SplineBackground_points";
    WorkspaceCreationHelper::storeWS(wsName,ws);


    IAlgorithm* alg = Mantid::API::FrameworkManager::Instance().createAlgorithm("SplineBackground");
    alg->initialize();
    alg->setPropertyValue("InputWorkspace",wsName);
    alg->setPropertyValue("OutputWorkspace","SplineBackground_out");
    alg->setPropertyValue("WorkspaceIndex","0");
    alg->execute();

    MatrixWorkspace_sptr outWS = WorkspaceCreationHelper::getWS<MatrixWorkspace>("SplineBackground_out");

    const MantidVec& X = outWS->readX(0);
    const MantidVec& Y = outWS->readY(0);

    for(size_t i=0;i<outWS->blocksize();i++)
    {
      TS_ASSERT_DELTA(Y[i],std::sin(X[i]),0.2);
    }
    TS_ASSERT( outWS->getAxis(0)->unit() == ws->getAxis(0)->unit() );
  }
};

#endif /*SPLINEBACKGROUNDTEST_H_*/
