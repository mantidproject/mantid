#ifndef SIMPLEINTEGRATIONTEST_H_
#define SIMPLEINTEGRATIONTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAlgorithms/SimpleIntegration.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidDataObjects/Workspace1D.h"

using namespace Mantid::API;
using namespace Mantid::Kernel;
using namespace Mantid::Algorithms;
using namespace Mantid::DataObjects;

class SimpleIntegrationTest : public CxxTest::TestSuite
{
public:
  
  SimpleIntegrationTest()
  {    
    // Set up a small workspace for testing
    Workspace_sptr space = WorkspaceFactory::Instance().create("Workspace2D",5,25,25);
    Workspace2D_sptr space2D = boost::dynamic_pointer_cast<Workspace2D>(space);
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
	AnalysisDataService::Instance().add("testSpace", space);
    
  }
  
  ~SimpleIntegrationTest()
  {
    
  }
  
  void testInit()
  {
    TS_ASSERT_THROWS_NOTHING(alg.initialize());    
    TS_ASSERT( alg.isInitialized() );
    
    // Set the properties
    alg.setPropertyValue("InputWorkspace","testSpace");
    outputSpace = "outer";
    alg.setPropertyValue("OutputWorkspace",outputSpace);

    alg.setPropertyValue("StartX","1");
    alg.setPropertyValue("EndX","4");
    alg.setPropertyValue("StartY","2");
    alg.setPropertyValue("EndY","4");
    
    TS_ASSERT_THROWS_NOTHING( alg2.initialize());    
    TS_ASSERT( alg.isInitialized() );
    
    // Set the properties
    alg2.setPropertyValue("InputWorkspace","testSpace");
    alg2.setPropertyValue("OutputWorkspace","out2");

  }
  
  void testExec()
  {
    if ( !alg.isInitialized() ) alg.initialize();
    TS_ASSERT_THROWS_NOTHING( alg.execute());
    TS_ASSERT( alg.isExecuted() );
    
    // Get back the saved workspace
    Workspace_sptr output;
    TS_ASSERT_THROWS_NOTHING(output = AnalysisDataService::Instance().retrieve(outputSpace));
    
    Workspace1D_sptr output1D = boost::dynamic_pointer_cast<Workspace1D>(output);
    std::vector<double> y = output1D->dataY();
    std::vector<double> e = output1D->dataE();
    unsigned int two = 2;
    TS_ASSERT_EQUALS( y.size(), two );
    TS_ASSERT_EQUALS( e.size(), two );
    
    TS_ASSERT_EQUALS( y[0], 36);
    TS_ASSERT_EQUALS( y[1], 51);
    TS_ASSERT_EQUALS( e[0], 6);

    if ( !alg2.isInitialized() ) alg2.initialize();
    
    // Check setting of invalid property value causes failure
    alg2.setPropertyValue("StartY","-1");
    TS_ASSERT_THROWS( alg2.execute(),std::runtime_error);
    // Set back to default value
    alg2.setPropertyValue("StartY","0");
    
    TS_ASSERT_THROWS_NOTHING( alg2.execute());
    TS_ASSERT( alg2.isExecuted() );
    
    // Get back the saved workspace
    TS_ASSERT_THROWS_NOTHING(output = AnalysisDataService::Instance().retrieve("out2"));
    
    output1D = boost::dynamic_pointer_cast<Workspace1D>(output);
    y = output1D->dataY();
    e = output1D->dataE();
    unsigned int five = 5;
    TS_ASSERT_EQUALS( y.size(), five );
    TS_ASSERT_EQUALS( e.size(), five );
    
    TS_ASSERT_EQUALS( y[0], 10 );
    TS_ASSERT_EQUALS( y[4], 110 );
    TS_ASSERT_DELTA ( e[2], 7.746, 0.001 );

  }
  
private:
  SimpleIntegration alg;   // Test with range limits
  SimpleIntegration alg2;  // Test without limits
  std::string outputSpace;
};

#endif /*SIMPLEINTEGRATIONTEST_H_*/
