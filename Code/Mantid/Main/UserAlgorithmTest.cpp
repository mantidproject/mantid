#include <iostream>
#include <iomanip>
#include <ctime>
#include "Benchmark.h"
#include "UserAlgorithmTest.h"
#include "MantidAPI/IAlgorithm.h"

using namespace Mantid::Kernel;
using namespace Mantid::DataObjects;
using namespace Mantid::API;

Workspace1D_sptr UserAlgorithmTest::Create1DWorkspace(int size)
{
  MantidVecPtr x1,y1,e1;
  x1.access().resize(size,1);
  y1.access().resize(size);
  std::generate(y1.access().begin(),y1.access().end(),FibSeries<double>());
  e1.access().resize(size);
  Workspace1D_sptr retVal(new Workspace1D);
  retVal->setX(x1);
  retVal->setData(y1,e1);
  return retVal;
}
Workspace2D_sptr UserAlgorithmTest::Create2DWorkspace(int xlen, int ylen)
{
  std::vector<double> x1(xlen),y1(xlen),e1(xlen);
  Workspace2D_sptr retVal(new Workspace2D);
  retVal->initialize(ylen,xlen,xlen);
  for (int i=0; i< ylen; i++)
  {
    Mantid::MantidVec& x1 = retVal->dataX(i);
    Mantid::MantidVec& y1 = retVal->dataY(i);
    Mantid::MantidVec& e1 = retVal->dataE(i);
    for(int j=0;j<xlen;j++)
    {
        x1[j] = double(i)+0.1*j;
        y1[j] = x1[j]*10;
        e1[j] = x1[j]/10;
    }
  }

  return retVal;
}

void UserAlgorithmTest::RunPropertyAlgorithmTest()
{

  IAlgorithm* alg = FrameworkManager::Instance().createAlgorithm("PropertyAlgorithm", "",1);

  alg->execute();

}

void UserAlgorithmTest::RunWorkspaceAlgorithmTest()
{

  MatrixWorkspace_sptr work = Create1DWorkspace(10);

  AnalysisDataService::Instance().add("test", work);
  
  IAlgorithm* alg = FrameworkManager::Instance().createAlgorithm("WorkspaceAlgorithm", "",1);

  alg->setPropertyValue("Workspace","test");

  alg->execute();

  AnalysisDataService::Instance().remove("test");

}

void UserAlgorithmTest::RunModifyDataTest()
{

  MatrixWorkspace_sptr inW = Create2DWorkspace(4,2);
  MatrixWorkspace_sptr outW = Create2DWorkspace(2,2);

  AnalysisDataService::Instance().add("inTest", inW);
  AnalysisDataService::Instance().add("outTest", outW);
  
  IAlgorithm* alg = FrameworkManager::Instance().createAlgorithm("ModifyData", "",1);

  alg->setPropertyValue("InputWorkspace","inTest");
  alg->setPropertyValue("OutputWorkspace","outTest");
  alg->setPropertyValue("UseVectors","0");

  alg->execute();

  AnalysisDataService::Instance().remove("outTest");
  AnalysisDataService::Instance().remove("inTest");

}


void UserAlgorithmTest::RunAllTests()
{
  RunPropertyAlgorithmTest();
  RunWorkspaceAlgorithmTest();
  RunModifyDataTest();
}
