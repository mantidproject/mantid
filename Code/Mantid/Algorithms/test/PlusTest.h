#ifndef PLUSTEST_H_
#define PLUSTEST_H_

#include <cxxtest/TestSuite.h>
#include <cmath>

#include "MantidAlgorithms/Plus.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidDataObjects/Workspace1D.h"

using namespace Mantid::API;
using namespace Mantid::Kernel;
using namespace Mantid::Algorithms;
using namespace Mantid::DataObjects;

class PlusTest : public CxxTest::TestSuite
{
public:

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
  
  
  void testInit()
  {
 

  }
  
  void testExec1D1D()
  {
    int sizex = 10;
    // Register the workspace in the data service
    AnalysisDataService* ADS = AnalysisDataService::Instance();
    Workspace1D* work_in1 = Create1DWorkspaceFib(sizex);
    Workspace1D* work_in2 = Create1DWorkspaceFib(sizex);
    ADS->add("test_in11", work_in1);
    ADS->add("test_in12", work_in2);

    plus plus_alg;

    plus_alg.initialize();
    plus_alg.setProperty("InputWorkspace_1","test_in11");
    plus_alg.setProperty("InputWorkspace_2","test_in12");    
    plus_alg.setProperty("OutputWorkspace","test_out1");
    plus_alg.execute();

    Workspace* work_out1;
    TS_ASSERT_THROWS_NOTHING(work_out1 = ADS->retrieve("test_out1"));

    checkData(work_in1, work_in2, work_out1);

    ADS->remove("test_out1");
    ADS->remove("test_in11");
    ADS->remove("test_in12");

  }

  void testExec2D2D()
  {
    int sizex = 10,sizey=20;
    // Register the workspace in the data service
    AnalysisDataService* ADS = AnalysisDataService::Instance();
    Workspace2D* work_in1 = Create2DWorkspace(sizex,sizey);
    Workspace2D* work_in2 = Create2DWorkspace(sizex,sizey);

    plus plus_alg;

    ADS->add("test_in21", work_in1);
    ADS->add("test_in22", work_in2);
    plus_alg.initialize();
    plus_alg.setProperty("InputWorkspace_1","test_in21");
    plus_alg.setProperty("InputWorkspace_2","test_in22");    
    plus_alg.setProperty("OutputWorkspace","test_out2");
    TS_ASSERT_THROWS_NOTHING(plus_alg.execute());
    TS_ASSERT( plus_alg.isExecuted() );
    Workspace* work_out1;
    TS_ASSERT_THROWS_NOTHING(work_out1 = ADS->retrieve("test_out2"));

    checkData(work_in1, work_in2, work_out1);

    ADS->remove("test_in21");
    ADS->remove("test_in22");
    ADS->remove("test_out2");
   
  }

  void checkData( Workspace* work_in1,  Workspace* work_in2, Workspace* work_out1)
  {
    for (int i = 0; i < work_out1->size(); i++)
    {
      TS_ASSERT_DELTA(work_in1->dataX(i/work_in1->blocksize())[i%work_in1->blocksize()],
                        work_out1->dataX(i/work_in1->blocksize())[i%work_in1->blocksize()], 0.0001);
      TS_ASSERT_DELTA(work_in1->dataY(i/work_in1->blocksize())[i%work_in1->blocksize()] + work_in2->dataY(i/work_in2->blocksize())[i%work_in1->blocksize()],
                        work_out1->dataY(i/work_in1->blocksize())[i%work_in1->blocksize()], 0.0001);
      double err1 = work_in1->dataE(i/work_in1->blocksize())[i%work_in1->blocksize()];
      double err2 = work_in2->dataE(i/work_in2->blocksize())[i%work_in1->blocksize()];
      double err = sqrt((err1 * err1) + (err2 * err2));
      TS_ASSERT_DELTA(err, work_out1->dataE(i/work_in1->blocksize())[i%work_in1->blocksize()], 0.0001);
    }
  }

  void testFinal()
  {
    plus plus_alg;
    if ( !plus_alg.isInitialized() ) plus_alg.initialize();
    
    // The final() method doesn't do anything at the moment, but test anyway
    TS_ASSERT_THROWS_NOTHING( plus_alg.finalize());
    TS_ASSERT( plus_alg.isFinalized() );
  }
  
};

#endif /*PLUSTEST_H_*/
