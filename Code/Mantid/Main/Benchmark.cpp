#include <iostream>
#include <iomanip>
#include <ctime>
#include "Benchmark.h"
#include "MantidAPI/IAlgorithm.h"

using namespace Mantid::Kernel;
using namespace Mantid::DataObjects;
using namespace Mantid::API;

Workspace1D_sptr Benchmark::Create1DWorkspaceFib(int size)
{
  std::vector<double> x1(size,1),y1,e1;
  //   x1.resize(size);
  //   std::generate(x1.begin(),x1.end(),rand);	
  y1.resize(size);
  std::generate(y1.begin(),y1.end(),FibSeries<double>());
  e1.resize(size);
  Workspace1D_sptr retVal(new Workspace1D);
  retVal->setX(x1);
  retVal->setData(y1,e1);
  return retVal;
}
Workspace2D_sptr Benchmark::Create2DWorkspace123(int xlen, int ylen)
{
  Histogram1D::RCtype x1,y1,e1;
  x1.access().resize(xlen,1);
  y1.access().resize(xlen,2);
  e1.access().resize(xlen,3);
  Workspace2D_sptr retVal(new Workspace2D);
  retVal->initialize(ylen,xlen,xlen);
  for (int i=0; i< ylen; i++)
  {
    retVal->setX(i,x1);     
    retVal->setData(i,y1,e1);
  }

  return retVal;
}

Workspace2D_sptr Benchmark::Create2DWorkspace154(int xlen, int ylen)
{
  Histogram1D::RCtype x1,y1,e1;
  x1.access().resize(xlen,1);
  y1.access().resize(xlen,5);
  e1.access().resize(xlen,4);
  Workspace2D_sptr retVal(new Workspace2D);
  retVal->initialize(ylen,xlen,xlen);
  for (int i=0; i< ylen; i++)
  {
    retVal->setX(i,x1);     
    retVal->setData(i,y1,e1);
  }

  return retVal;
}  
Workspace2D_sptr Benchmark::Create2DWorkspace(int xlen, int ylen)
{
  return Create2DWorkspace123(xlen, ylen);
}
Workspace2D_sptr Benchmark::Create2DWorkspace123Hist(int xlen, int ylen)
{
  Histogram1D::RCtype x1,y1,e1;
  x1.access().resize(xlen+1,1);
  y1.access().resize(xlen,2);
  e1.access().resize(xlen,3);
  Workspace2D_sptr retVal(new Workspace2D);
  retVal->initialize(ylen,xlen+1,xlen);
  for (int i=0; i< ylen; i++)
  {
    retVal->setX(i,x1);     
    retVal->setData(i,y1,e1);
  }

  return retVal;
}  
Workspace2D_sptr Benchmark::Create2DWorkspace154Hist(int xlen, int ylen)
{
  Histogram1D::RCtype x1,y1,e1;
  x1.access().resize(xlen+1,1);
  y1.access().resize(xlen,5);
  e1.access().resize(xlen,4);
  Workspace2D_sptr retVal(new Workspace2D);
  retVal->initialize(ylen,xlen+1,xlen);
  for (int i=0; i< ylen; i++)
  {
    retVal->setX(i,x1);     
    retVal->setData(i,y1,e1);
  }

  return retVal;
}  


void Benchmark::RunPlusTest()
{
//  RunPlusTest(2584,2000);
  RunPlusTest(12584,2000);
}


void Benchmark::RunPlusTest(int detectorCount, int timeBinCount)
{
  int sizex = detectorCount;
  int sizey = timeBinCount;

  MatrixWorkspace_sptr work_in3 = Create2DWorkspace123(sizex,sizey);
  MatrixWorkspace_sptr work_in4 = Create2DWorkspace154(sizex,sizey);

  AnalysisDataService::Instance().add("test_in11", work_in3);
  AnalysisDataService::Instance().add("test_in12", work_in4);

  
  IAlgorithm* alg = FrameworkManager::Instance().createAlgorithm("Plus", "",1);

  //alg.initialize();
  alg->setPropertyValue("InputWorkspace_1","test_in11");
  alg->setPropertyValue("InputWorkspace_2","test_in12");    
  alg->setPropertyValue("OutputWorkspace","test_out1");
  clock_t start = clock();
  alg->execute();
  clock_t end = clock();
  Workspace_sptr work_out1 = AnalysisDataService::Instance().retrieve("test_out1");


  std::cout << double(end - start)/CLOCKS_PER_SEC << std::endl;
  //    ADS->remove("test_out1");
  AnalysisDataService::Instance().remove("test_in11");
  AnalysisDataService::Instance().remove("test_in12");

}
