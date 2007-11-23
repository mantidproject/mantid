#ifndef SIMPLEINTEGRATIONTEST_H_
#define SIMPLEINTEGRATIONTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAlgorithms/SimpleIntegration.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidDataObjects/Workspace1D.h"

using namespace Mantid::Kernel;
using namespace Mantid::Algorithms;
using namespace Mantid::DataObjects;

class SimpleIntegrationTest : public CxxTest::TestSuite
{
public:
  
  SimpleIntegrationTest()
  { 
    WorkspaceFactory *factory = WorkspaceFactory::Instance();
    
    // Set up a small workspace for testing
    Workspace *space = factory->create("Workspace2D");
    Workspace2D *space2D = dynamic_cast<Workspace2D*>(space);
    space2D->setHistogramNumber(5);
    double *a = new double[25];
    double *e = new double[25];
    for (int i = 0; i < 25; ++i)
    {
      a[i]=i;
      e[i]=sqrt(double(i));
    }
    for (int j = 0; j < 5; ++j) {
      space2D->setData(j, *(new std::vector<double>(a+(5*j), a+(5*j)+5)),
          *(new std::vector<double>(e+(5*j), e+(5*j)+5)));
    }
      
    // Register the workspace in the data service
    AnalysisDataService *data = AnalysisDataService::Instance();
    data->add("testSpace", space);
    
  }
  
  ~SimpleIntegrationTest()
  {
    
  }
  
  void testInit()
  {
    StatusCode status = alg.initialize();
    TS_ASSERT( ! status.isFailure() );
    TS_ASSERT( alg.isInitialized() );
    
    // Set the properties
    alg.setProperty("InputWorkspace","testSpace");
    outputSpace = "outer";
    alg.setProperty("OutputWorkspace",outputSpace);

    alg.setProperty("StartX","1");
    alg.setProperty("EndX","4");
    alg.setProperty("StartY","2");
    alg.setProperty("EndY","4");
    
    status = alg2.initialize();
    TS_ASSERT( ! status.isFailure() );
    TS_ASSERT( alg.isInitialized() );
    
    // Set the properties
    alg2.setProperty("InputWorkspace","testSpace");
    alg2.setProperty("OutputWorkspace","out2");

  }
  
  void testExec()
  {
    if ( !alg.isInitialized() ) alg.initialize();
    StatusCode status = alg.execute();
    TS_ASSERT( ! status.isFailure() );
    TS_ASSERT( alg.isExecuted() );
    
    // Get back the saved workspace
    AnalysisDataService *data = AnalysisDataService::Instance();
    Workspace *output;
    status = data->retrieve(outputSpace, output);
    TS_ASSERT( ! status.isFailure() );
    
    Workspace1D *output1D = dynamic_cast<Workspace1D*>(output);
    std::vector<double> y = output1D->getY();
    std::vector<double> e = output1D->getE();
    TS_ASSERT_EQUALS( y.size(), 2 );
    TS_ASSERT_EQUALS( e.size(), 2 );
    
    TS_ASSERT_EQUALS( y[0], 36);
    TS_ASSERT_EQUALS( y[1], 51);
    TS_ASSERT_EQUALS( e[0], 6);

    if ( !alg2.isInitialized() ) alg2.initialize();
    status = alg2.execute();
    TS_ASSERT( ! status.isFailure() );
    TS_ASSERT( alg2.isExecuted() );
    
    // Get back the saved workspace
    status = data->retrieve("out2", output);
    TS_ASSERT( ! status.isFailure() );
    
    output1D = dynamic_cast<Workspace1D*>(output);
    y = output1D->getY();
    e = output1D->getE();
    TS_ASSERT_EQUALS( y.size(), 5 );
    TS_ASSERT_EQUALS( e.size(), 5 );
    
    TS_ASSERT_EQUALS( y[0], 10 );
    TS_ASSERT_EQUALS( y[4], 110 );
    TS_ASSERT_DELTA ( e[2], 7.746, 0.001 );

  }
  
  void testFinal()
  {
    if ( !alg.isInitialized() ) alg.initialize();
    
    // The final() method doesn't do anything at the moment, but test anyway
    StatusCode status = alg.finalize();
    TS_ASSERT( ! status.isFailure() );
    TS_ASSERT( alg.isFinalized() );
  }
  
private:
  SimpleIntegration alg;   // Test with range limits
  SimpleIntegration alg2;  // Test without limits
  std::string outputSpace;
};

#endif /*SIMPLEINTEGRATIONTEST_H_*/
