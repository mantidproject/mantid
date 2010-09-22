#ifndef LOADRKHTEST_H_
#define LOADRKHTEST_H_

//-----------------
// Includes
//-----------------
#include <cxxtest/TestSuite.h>

#include "MantidDataHandling/LoadRKH.h"
#include "MantidDataObjects/Workspace1D.h"
#include "Poco/Path.h"

class LoadRKHTest : public CxxTest::TestSuite
{
public:

  // A sample file is in the repository
  LoadRKHTest() : inputFile("")
  {    
    inputFile = Poco::Path(Poco::Path::current()).resolve("../../../../Test/AutoTestData/DIRECT.041").toString();
  }

  void testInit()
  {
    TS_ASSERT_THROWS_NOTHING( loadrkh.initialize());
    TS_ASSERT( loadrkh.isInitialized() );
  }

  void testExec()
  {
    if ( !loadrkh.isInitialized() ) loadrkh.initialize();

    //No parameters have been set yet, so it should throw
    TS_ASSERT_THROWS(loadrkh.execute(), std::runtime_error);

    //Set the file name
    loadrkh.setPropertyValue("Filename", inputFile);
    
    std::string outputSpace = "outer";
    //Set an output workspace
    loadrkh.setPropertyValue("OutputWorkspace", outputSpace);
    
    //check that retrieving the filename gets the correct value
    std::string result;
    TS_ASSERT_THROWS_NOTHING( result = loadrkh.getPropertyValue("Filename") )
    TS_ASSERT( result.compare(inputFile) == 0 );

    TS_ASSERT_THROWS_NOTHING( result = loadrkh.getPropertyValue("OutputWorkspace") )
    TS_ASSERT( result == outputSpace );

    //Should now throw nothing
    TS_ASSERT_THROWS_NOTHING( loadrkh.execute() );
    TS_ASSERT( loadrkh.isExecuted() );
    
    using namespace Mantid::API;
    using namespace Mantid::DataObjects;
    //Now need to test the resultant workspace, first retrieve it
    Workspace_sptr rkhspace;
    TS_ASSERT_THROWS_NOTHING( rkhspace = AnalysisDataService::Instance().retrieve(outputSpace) );
    Workspace1D_sptr data = boost::dynamic_pointer_cast<Workspace1D>(rkhspace);
    
    //The data in the 2D workspace does not match the file data directly because the
    //file contains bin-centered values and the algorithm adjusts the x values so that 
    //they are bin edge values
    
    //Single histogram
    TS_ASSERT_EQUALS( data->getNumberHistograms(), 1 );

    //Test the size of the data vectors (there should be 102 data points so x have 103)
    TS_ASSERT_EQUALS( static_cast<int>(data->dataX(0).size()), 102);
    TS_ASSERT_EQUALS( static_cast<int>(data->dataY(0).size()), 102);
    TS_ASSERT_EQUALS( static_cast<int>(data->dataE(0).size()), 102);

    //Test first 3 bin edges for the correct values
    double tolerance(1e-06);
    TS_ASSERT_DELTA( data->dataX(0)[0], 1.34368, tolerance );
    TS_ASSERT_DELTA( data->dataX(0)[1], 1.37789, tolerance );
    TS_ASSERT_DELTA( data->dataX(0)[2], 1.41251, tolerance );
    //Test a couple of random ones
    TS_ASSERT_DELTA( data->dataX(0)[20], 2.20313, tolerance );
    TS_ASSERT_DELTA( data->dataX(0)[45], 4.08454, tolerance );
    TS_ASSERT_DELTA( data->dataX(0)[87], 11.52288, tolerance );
    //Test the last 3
    TS_ASSERT_DELTA( data->dataX(0)[100], 15.88747, tolerance );
    TS_ASSERT_DELTA( data->dataX(0)[101], 16.28282, tolerance );

    //Now Y values
    TS_ASSERT_DELTA( data->dataY(0)[0], 0.168419, tolerance );
    TS_ASSERT_DELTA( data->dataY(0)[25], 2.019846, tolerance );
    TS_ASSERT_DELTA( data->dataY(0)[99], 0.0, tolerance );

    //Now E values
    TS_ASSERT_DELTA( data->dataE(0)[0], 0.122346, tolerance );
    TS_ASSERT_DELTA( data->dataE(0)[25], 0.018345, tolerance );
    TS_ASSERT_DELTA( data->dataE(0)[99], 0.0, tolerance );

  }

private:
  Mantid::DataHandling::LoadRKH loadrkh;
  std::string inputFile;
};


#endif //LOADRKHTEST_H_
