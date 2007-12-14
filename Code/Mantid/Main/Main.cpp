#include <iostream>
#include <iomanip>
#include "MantidAPI/AnalysisDataService.h"
#include "MantidDataHandling/LoadInstrument.h"
#include "MantidAPI/Instrument.h"
#include "MantidAlgorithms/SimpleIntegration.h"
#include "MantidAlgorithms/Plus.h"
#include "MantidKernel/Exception.h"
#include "MantidKernel/PropertyWithValue.h"
#include "MantidKernel/BoundedValidator.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidAPI/Workspace.h"
#include "MantidAPI/Algorithm.h"
#include "MantidAPI/TripleRef.h" 
#include "MantidAPI/TripleIterator.h" 
#include "MantidDataObjects/Workspace1D.h" 
#include "MantidDataObjects/Workspace2D.h" 

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

  Workspace1D* Create1DWorkspaceFib(int size)
  {
    std::vector<double> x1,y1,e1;
    x1.resize(size);
    std::generate(x1.begin(),x1.end(),rand);	
    y1.resize(size);
    std::generate(y1.begin(),y1.end(),FibSeries<double>());
    e1.resize(size);
    Workspace1D* retVal = new Workspace1D;
    retVal->setX(x1);
    retVal->setData(y1,e1);
    return retVal;
  }
  Workspace2D* Create2DWorkspace(int xlen, int ylen)
  {
    std::vector<double> x1(xlen,1),y1(xlen,2),e1(xlen,3);
    Workspace2D* retVal = new Workspace2D;
    retVal->setHistogramNumber(ylen);
    for (int i=0; i< ylen; i++)
    {
      retVal->setX(i,x1);     
      retVal->setData(i,y1,e1);
    }

    return retVal;
  }
int main()
{
#if defined _DEBUG
	//NOTE:  Any code in here is temporory for debugging purposes only, nothing is safe!
    int sizex = 1500,sizey=2584;
    // Register the workspace in the data service
    AnalysisDataService* ADS = AnalysisDataService::Instance();
//    Workspace1D* work_in1 = Create1DWorkspaceFib(sizex);
//    Workspace1D* work_in2 = Create1DWorkspaceFib(sizex);
    Workspace2D* work_in3 = Create2DWorkspace(sizex,sizey);
    Workspace2D* work_in4 = Create2DWorkspace(sizex,sizey);
//    ADS->add("test_in11", work_in1);
//    ADS->add("test_in12", work_in2);

    plus plus_alg;
/*
    plus_alg.initialize();
    plus_alg.setProperty("InputWorkspace_1","test_in11");
    plus_alg.setProperty("InputWorkspace_2","test_in12");    
    plus_alg.setProperty("OutputWorkspace","test_out1");
    plus_alg.execute();
    Workspace* work_out1 = ADS->retrieve("test_out1");
    ADS->remove("test_out1");
    ADS->remove("test_in11");
    ADS->remove("test_in12");
*/
    ADS->add("test_in21", work_in3);
    ADS->add("test_in22", work_in4);
    plus_alg.initialize();
    plus_alg.setProperty("InputWorkspace_1","test_in21");
    plus_alg.setProperty("InputWorkspace_2","test_in22");    
    plus_alg.setProperty("OutputWorkspace","test_out2");
    plus_alg.execute();
    Workspace* work_out2 = ADS->retrieve("test_out2");



    //one would then have to dynamically cast this to a workspace1D
#endif
	exit(0);
}
