#ifndef LOADCANSAS1DTEST_H
#define LOADCANSAS1DTEST_H

//------------------------------------------------
// Includes
//------------------------------------------------

#include <cxxtest/TestSuite.h>

#include"MantidDataHandling/LoadCanSAS1D.h"
#include "MantidDataObjects/Workspace1D.h"
#include "MantidDataObjects/Workspace2D.h"
#include "Poco/Path.h"

class LoadCanSAS1dTest : public CxxTest::TestSuite
{
public:
	LoadCanSAS1dTest()
	{
		 inputFile = Poco::Path(Poco::Path::current()).resolve("../../../../Test/Data/LOQ_CANSAS1D.xml").toString();
	}
	 void testInit()
  {
    TS_ASSERT_THROWS_NOTHING( cansas1d.initialize());
    TS_ASSERT( cansas1d.isInitialized() );
  }

  void testExec()
  {
	  if ( !cansas1d.isInitialized() ) cansas1d.initialize();

    //No parameters have been set yet, so it should throw
    TS_ASSERT_THROWS(cansas1d.execute(), std::runtime_error);

    //Set the file name
    cansas1d.setPropertyValue("Filename", inputFile);
    
    std::string outputSpace = "outws";
    //Set an output workspace
    cansas1d.setPropertyValue("OutputWorkspace", outputSpace);
    
    //check that retrieving the filename gets the correct value
    std::string result;
    TS_ASSERT_THROWS_NOTHING( result = cansas1d.getPropertyValue("Filename") )
    TS_ASSERT( result.compare(inputFile) == 0 );

    TS_ASSERT_THROWS_NOTHING( result = cansas1d.getPropertyValue("OutputWorkspace") )
    TS_ASSERT( result == outputSpace );

    //Should now throw nothing
    TS_ASSERT_THROWS_NOTHING( cansas1d.execute() );
    TS_ASSERT( cansas1d.isExecuted() );

	 //Now need to test the resultant workspace, first retrieve it
	Mantid::API::Workspace_sptr ws;
	TS_ASSERT_THROWS_NOTHING( ws = Mantid::API::AnalysisDataService::Instance().retrieve(outputSpace) );
	Mantid::DataObjects::Workspace2D_sptr ws2d = boost::dynamic_pointer_cast<Mantid::DataObjects::Workspace2D>(ws);
	  //Single histogram
   TS_ASSERT_EQUALS( ws2d->getNumberHistograms(), 1 );

   //Test the size of the data vectors (there should be 102 data points so x have 103)
    TS_ASSERT_EQUALS( (ws2d->dataX(0).size()), 102);
    TS_ASSERT_EQUALS( (ws2d->dataY(0).size()), 102);
    TS_ASSERT_EQUALS( (ws2d->dataE(0).size()), 102);


    double tolerance(1e-06);
    TS_ASSERT_DELTA( ws2d->dataX(0)[0], 0.0604703, tolerance );
    TS_ASSERT_DELTA( ws2d->dataX(0)[1], 0.0620232, tolerance );
    TS_ASSERT_DELTA( ws2d->dataX(0)[2], 0.0635737, tolerance );
    ////Test a couple of random ones
    TS_ASSERT_DELTA( ws2d->dataX(0)[20], 0.0991537, tolerance );
    TS_ASSERT_DELTA( ws2d->dataX(0)[64], 0.293873, tolerance );
    
   
    TS_ASSERT_DELTA( ws2d->dataX(0)[100], 0.714858, tolerance );
    TS_ASSERT_DELTA( ws2d->dataX(0)[101], 0.732729, tolerance );


    TS_ASSERT_DELTA( ws2d->dataY(0)[0], 12, tolerance );
    TS_ASSERT_DELTA( ws2d->dataY(0)[25], 4674, tolerance );
    TS_ASSERT_DELTA( ws2d->dataY(0)[99], 1, tolerance );


    TS_ASSERT_DELTA( ws2d->dataE(0)[0], 3.4641, tolerance );
    TS_ASSERT_DELTA( ws2d->dataE(0)[25], 68.3667, tolerance );
    TS_ASSERT_DELTA( ws2d->dataE(0)[99], 1, tolerance );
    
    
  }
private:
	std::string inputFile;
	Mantid::DataHandling::LoadCanSAS1D cansas1d;

};
#endif