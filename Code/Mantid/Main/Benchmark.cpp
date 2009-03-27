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
  std::vector<double> x1(xlen,1),y1(xlen,2),e1(xlen,3);
//  Workspace2D_sptr retVal(new ManagedWorkspace2D);
//  retVal->setTitle("ws123");
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
  std::vector<double> x1(xlen,1),y1(xlen,5),e1(xlen,4);
//  Workspace2D_sptr retVal(new ManagedWorkspace2D);
//  retVal->setTitle("ws154");
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
  std::vector<double> x1(xlen+1,1);
  std::vector<double> y1(xlen,2),e1(xlen,3);
//  Workspace2D_sptr retVal(new ManagedWorkspace2D);
//  retVal->setTitle("ws154");
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
  std::vector<double> x1(xlen+1,1);
  std::vector<double> y1(xlen,5),e1(xlen,4);
//  Workspace2D_sptr retVal(new ManagedWorkspace2D);
//  retVal->setTitle("ws154");
  Workspace2D_sptr retVal(new Workspace2D);
  retVal->initialize(ylen,xlen+1,xlen);
  for (int i=0; i< ylen; i++)
  {
    retVal->setX(i,x1);     
    retVal->setData(i,y1,e1);
  }

  return retVal;
}  

/*
This code runs on jdmc windows workstation (4Gb, AMD Athlon X2 Dual Core Processor 3800+ 1.99Ghz) 

release configuration
---------------------
using full iterator method  : ~8 seconds
using handcrafted looping without use of iterators: ~1.5 seconds

debug configuration
-------------------
using full iterator method  : ~52 seconds

An identical fortran operation using LIBISIS takes ~0.7 seconds

*/
void Benchmark::RunPlusTest()
{
//  RunPlusTest(2800,41476);
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
