#include <iostream>
#include <iomanip>
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAlgorithms/Plus.h"
#include "MantidAlgorithms/Multiply.h"
#include "MantidAPI/Workspace.h"
#include "MantidAPI/Algorithm.h"
#include "MantidDataObjects/Workspace1D.h" 
#include "MantidDataObjects/Workspace2D.h" 
#include <ctime>
using namespace Mantid::Kernel;
using namespace Mantid::DataObjects;
using namespace Mantid::API;
using namespace Mantid::Algorithms;

  template<typename T>
  class FibSeries
  {
  private:
    T x1;  /// Initial value 1;
    T x2;  /// Initial value 2;

  public:

    FibSeries() : x1(1),x2(1) {}
    T operator()() { const T out(x1+x2); x1=x2; x2=out;  return out; }
  };

  static Workspace1D_sptr Create1DWorkspaceFib(int size)
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
static Workspace2D_sptr Create2DWorkspace123(int xlen, int ylen)
  {
    std::vector<double> x1(xlen,1),y1(xlen,2),e1(xlen,3);
    Workspace2D_sptr retVal(new Workspace2D);
    retVal->setHistogramNumber(ylen);
    for (int i=0; i< ylen; i++)
    {
      retVal->setX(i,x1);     
      retVal->setData(i,y1,e1);
    }

    return retVal;
  }

  static Workspace2D_sptr Create2DWorkspace154(int xlen, int ylen)
  {
    std::vector<double> x1(xlen,1),y1(xlen,5),e1(xlen,4);
    Workspace2D_sptr retVal(new Workspace2D);
    retVal->setHistogramNumber(ylen);
    for (int i=0; i< ylen; i++)
    {
      retVal->setX(i,x1);     
      retVal->setData(i,y1,e1);
    }

    return retVal;
  }  
  static Workspace2D_sptr Create2DWorkspace(int xlen, int ylen)
  {
    return Create2DWorkspace123(xlen, ylen);
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



void benchmark()
{

	//NOTE:  Any code in here is temporory for debugging purposes only, nothing is safe!
    int sizex = 2000,sizey=2584;
    // Register the workspace in the data service
    AnalysisDataService* ADS = AnalysisDataService::Instance();

    Workspace2D_sptr work_in3 = Create2DWorkspace123(sizex,sizey);
    Workspace2D_sptr work_in4 = Create2DWorkspace154(sizex,sizey);

    ADS->add("test_in11", work_in3);
    ADS->add("test_in12", work_in4);

    Plus plus_alg;

    plus_alg.initialize();
    plus_alg.setPropertyValue("InputWorkspace_1","test_in11");
    plus_alg.setPropertyValue("InputWorkspace_2","test_in12");    
    plus_alg.setPropertyValue("OutputWorkspace","test_out1");
    
    clock_t start = clock();
    plus_alg.execute();
    clock_t end = clock();

    Workspace_sptr work_out1 = ADS->retrieve("test_out1");

    
    std::cout << double(end - start)/CLOCKS_PER_SEC << std::endl;
//    ADS->remove("test_out1");
    ADS->remove("test_in11");
    ADS->remove("test_in12");

}
