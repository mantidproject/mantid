#ifndef TOFTOWAVELENGTHTEST_H_
#define TOFTOWAVELENGTHTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAlgorithms/TOFtoWavelength.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidDataHandling/LoadInstrument.h"

using namespace Mantid::API;
using namespace Mantid::Algorithms;
using Mantid::DataObjects::Workspace2D;

class TOFtoWavelengthTest : public CxxTest::TestSuite
{
public:
  
  TOFtoWavelengthTest()
  {
    WorkspaceFactory *factory = WorkspaceFactory::Instance();
    
    // Set up a small workspace for testing
    Workspace *space = factory->create("Workspace2D");
    Workspace2D *space2D = dynamic_cast<Workspace2D*>(space);
    space2D->setHistogramNumber(2584);
    std::vector<double> x(11);
    for (int i = 0; i < 11; ++i) 
    {
      x[i]=i*1000;
    }
    std::vector<double> a(10);
    std::vector<double> e(10);
    for (int i = 0; i < 10; ++i)
    {
      a[i]=i;
      e[i]=sqrt(double(i));
    }
    for (int j = 0; j < 2584; ++j) {
      space2D->setX(j, x);
      space2D->setData(j, a, e);
    }
    
    // Register the workspace in the data service
    AnalysisDataService *data = AnalysisDataService::Instance();
    inputSpace = "testWorkspace";
    data->add(inputSpace, space);

    // Load the instrument data
    Mantid::DataHandling::LoadInstrument loader;
    loader.initialize();
    // Path to test input file assumes Test directory checked out from SVN
    std::string inputFile = "../../../../Test/Instrument/HET_Definition.txt";
    loader.setProperty("Filename", inputFile);
    loader.setProperty("Workspace", inputSpace);
    loader.execute();
    loader.finalize();
    
  }
  
  void testInit()
  {
    TS_ASSERT_THROWS_NOTHING(alg.initialize());    
    TS_ASSERT( alg.isInitialized() );    
    
    // Set the properties
    alg.setProperty("InputWorkspace",inputSpace);
    outputSpace = "outWorkspace";
    alg.setProperty("OutputWorkspace",outputSpace);
  }
  
  void testExec()
  {
    if ( !alg.isInitialized() ) alg.initialize();
    TS_ASSERT_THROWS_NOTHING( alg.execute());
    TS_ASSERT( alg.isExecuted() );

    // Get back the saved workspace
    AnalysisDataService *data = AnalysisDataService::Instance();
    Workspace *output;
    TS_ASSERT_THROWS_NOTHING(output = data->retrieve(outputSpace));
    Workspace *input;
    TS_ASSERT_THROWS_NOTHING(input = data->retrieve(inputSpace));
    
    Workspace2D *output2D = dynamic_cast<Workspace2D*>(output);
    Workspace2D *input2D = dynamic_cast<Workspace2D*>(input);
    // Test that y & e data is unchanged
    std::vector<double> y = output2D->getY(777);
    std::vector<double> e = output2D->getE(777);
    TS_ASSERT_EQUALS( y.size(), 10 );
    TS_ASSERT_EQUALS( e.size(), 10 );
    std::vector<double> yIn = input2D->getY(777);
    std::vector<double> eIn = input2D->getE(777);
    TS_ASSERT_EQUALS( y[0], yIn[0] );
    TS_ASSERT_EQUALS( y[4], yIn[4] );
    TS_ASSERT_EQUALS( e[1], eIn[1] );
    // Test that spectra that should have been zeroed have been
    std::vector<double> x = output2D->getX(1);
    y = output2D->getY(134);
    e = output2D->getE(382);
    TS_ASSERT_EQUALS( x[7], 0 );
    TS_ASSERT_EQUALS( y[1], 0 );
    TS_ASSERT_EQUALS( e[9], 0 );
    // Check that the data has truly been copied (i.e. isn't a reference to the same
    //    vector in both workspaces)
    double test[10] = {11, 22, 33, 44, 55, 66, 77, 88, 99, 1010};
    std::vector<double> tester(test, test+10);
    output2D->setData(1837, tester);
    y = output2D->getY(1837);
    TS_ASSERT_EQUALS( y[3], 44.0);
    yIn = input2D->getY(1837);
    TS_ASSERT_EQUALS( yIn[3], 3.0);
    
    // Check that a couple of x bin boundaries have been correctly converted
    x = output2D->getX(500);
    TS_ASSERT_DELTA( x[5], 0.914, 0.001 );
    TS_ASSERT_DELTA( x[10], 1.828, 0.001 );
    // Just check that an input bin boundary is unchanged
    std::vector<double> xIn = input2D->getX(2066);
    TS_ASSERT_EQUALS( xIn[4], 4000.0 );
  }
  
  void testFinal()
  {
    if ( !alg.isInitialized() ) alg.initialize();
    
    // The final() method doesn't do anything at the moment, but test anyway
    TS_ASSERT_THROWS_NOTHING(alg.finalize());
    TS_ASSERT( alg.isFinalized() );
  }
  
private:
  TOFtoWavelength alg;
  std::string inputSpace;
  std::string outputSpace;
};

#endif /*TOFTOWAVELENGTHTEST_H_*/
