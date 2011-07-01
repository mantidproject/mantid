#ifndef SAVECSVTEST_H_
#define SAVECSVTEST_H_

#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidDataHandling/SaveCSV.h"
#include "MantidDataObjects/Workspace2D.h"
#include <cxxtest/TestSuite.h>
#include <fstream>
#include <Poco/File.h>

using namespace Mantid::API;
using namespace Mantid::Kernel;
using namespace Mantid::DataHandling;
using namespace Mantid::DataObjects;


// Notice, the SaveCSV algorithm currently does not create
// an output workspace and therefore no tests related to the 
// output workspace is performed.

// Notice also that currently no tests have been added to test
// this class when trying to save a 2D workspace with SaveCSV. 

class SaveCSVTest : public CxxTest::TestSuite
{
public: 

  static SaveCSVTest *createSuite() { return new SaveCSVTest(); }
  static void destroySuite(SaveCSVTest *suite) { delete suite; }
  
  SaveCSVTest()
  {
    // create dummy 2D-workspace with one pixel
    Workspace_sptr localWorkspace = WorkspaceFactory::Instance().create("Workspace2D",1,10,10);
    Workspace2D_sptr localWorkspace2D_onePixel = boost::dynamic_pointer_cast<Workspace2D>(localWorkspace);

    double d = 0.0;
    for (int i=0; i<10; ++i,d+=0.1)
    {
      localWorkspace2D_onePixel->dataX(0)[i] = d;
      localWorkspace2D_onePixel->dataY(0)[i] = d+1.0;
      localWorkspace2D_onePixel->dataE(0)[i] = d+2.0;
    }
    
    AnalysisDataService::Instance().add("SAVECSVTEST-testSpace", localWorkspace);
  }
  
  
  void testInit()
  {
    TS_ASSERT_THROWS_NOTHING(algToBeTested.initialize());
    TS_ASSERT( algToBeTested.isInitialized() );
  }
  
  
  void testExec()
  {
    if ( !algToBeTested.isInitialized() ) algToBeTested.initialize();
  
    algToBeTested.setPropertyValue("InputWorkspace", "SAVECSVTEST-testSpace");     
    
    // Should fail because mandatory parameter has not been set
    algToBeTested.execute();
    TS_ASSERT_EQUALS(algToBeTested.isExecuted(), false)
        
    
    // Now set it...
    // specify name of file to save 1D-workspace to
    outputFile = "testOfSaveCSV.csv";
    algToBeTested.setPropertyValue("Filename", outputFile);
    outputFile = algToBeTested.getPropertyValue("Filename");

    std::string result;
    TS_ASSERT_THROWS_NOTHING( result = algToBeTested.getPropertyValue("Filename") )
    TS_ASSERT( ! result.compare(outputFile)); 
    
    TS_ASSERT_THROWS_NOTHING(algToBeTested.execute());    
    TS_ASSERT( algToBeTested.isExecuted() );    
   
    
    // has the algorithm written a file to disk?
    
    TS_ASSERT( Poco::File(outputFile).exists() );
    
    
    // Do a few tests to see if the content of outputFile is what you
    // expect.
     
    std::ifstream in(outputFile.c_str());
    
    std::string Amarker;
    double d1, d2, d3;
    std::string separator;
    std::string number_plus_comma;
    
    in >> Amarker >> d1 >> separator >> d2 >> separator >> d3 >> separator >> number_plus_comma;
    
    in.close();
    
    TS_ASSERT_EQUALS(Amarker,"A" );
    TS_ASSERT_EQUALS(separator,"," );
    TS_ASSERT_DELTA(d1, 0.0, 1e-5);
    TS_ASSERT_DELTA(d2, 0.1, 1e-5);
    TS_ASSERT_DELTA(d3, 0.2, 1e-5);
    TS_ASSERT_EQUALS( number_plus_comma, "0.3," );
    
    
    // remove file created by this algorithm
    Poco::File(outputFile).remove();
    AnalysisDataService::Instance().remove("SAVECSVTEST-testSpace");
  
  }

  
private:
  SaveCSV algToBeTested;
  std::string outputFile;
  
};
  
#endif /*SAVECSVTEST_H_*/
