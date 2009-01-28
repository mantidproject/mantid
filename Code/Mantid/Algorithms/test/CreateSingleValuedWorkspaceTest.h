#ifndef CREATESINGLEVALUEDWORKSPACETEST_H_
#define CREATESINGLEVALUEDWORKSPACETEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidAlgorithms/CreateSingleValuedWorkspace.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidDataObjects/WorkspaceSingleValue.h"

class CreateSingleValuedWorkspaceTest : public CxxTest::TestSuite
{

public:

  void testInitNoErr()
  {
    TS_ASSERT_THROWS_NOTHING( algNoErr.initialize() );
    TS_ASSERT( algNoErr.isInitialized() );

  }
  
  void testExecNoErr()
  {
    if( ! algNoErr.isInitialized() ) TS_ASSERT_THROWS_NOTHING( algNoErr.initialize() );

    //First with no Error
    //Running algorithm here should throw
    TS_ASSERT_THROWS( algNoErr.execute(), std::runtime_error );

    //Set some properties
    std::string outputSpace("NoError");
    TS_ASSERT_THROWS_NOTHING( algNoErr.setPropertyValue("OutputWorkspace", outputSpace) );
    TS_ASSERT_THROWS_NOTHING( algNoErr.setPropertyValue("DataValue", "3.0") );

    //Run the algorithm
    TS_ASSERT_THROWS_NOTHING( algNoErr.execute() );

    Mantid::API::Workspace_sptr ws;
    TS_ASSERT_THROWS_NOTHING( ws = Mantid::API::AnalysisDataService::Instance().retrieve(outputSpace) );

    Mantid::DataObjects::WorkspaceSingleValue_sptr single = 
      boost::dynamic_pointer_cast<Mantid::DataObjects::WorkspaceSingleValue>(ws);

    TS_ASSERT( ws.get() != 0 );

    //Test the data
    TS_ASSERT_DELTA( single->dataX(0)[0], 0.0, 1e-08 );
    TS_ASSERT_DELTA( single->dataY(0)[0], 3.0, 1e-08 );
    TS_ASSERT_DELTA( single->dataE(0)[0], 0.0, 1e-08 );

  }

  void testInitWithErr()
  {

    TS_ASSERT_THROWS_NOTHING( algWithErr.initialize() );
    TS_ASSERT( algWithErr.isInitialized() );

  }
  
  void testExecWithErr()
  {
    if( ! algWithErr.isInitialized() ) TS_ASSERT_THROWS_NOTHING( algWithErr.initialize() );

    //First with no Error
    //Running algorithm here should throw
    TS_ASSERT_THROWS( algWithErr.execute(), std::runtime_error );

    //Set some properties
    std::string outputSpace("WithError");
    TS_ASSERT_THROWS_NOTHING( algWithErr.setPropertyValue("OutputWorkspace", outputSpace) );
    TS_ASSERT_THROWS_NOTHING( algWithErr.setPropertyValue("DataValue", "5.0") );
    TS_ASSERT_THROWS_NOTHING( algWithErr.setPropertyValue("ErrorValue", "2.0") );

    //Run the algorithm
    TS_ASSERT_THROWS_NOTHING( algWithErr.execute() );

    //Get the workspace out
    Mantid::API::Workspace_sptr ws;
    TS_ASSERT_THROWS_NOTHING( ws = Mantid::API::AnalysisDataService::Instance().retrieve(outputSpace) );

    Mantid::DataObjects::WorkspaceSingleValue_sptr single = 
      boost::dynamic_pointer_cast<Mantid::DataObjects::WorkspaceSingleValue>(ws);

    TS_ASSERT( ws.get() != 0 );

    //Test the data
    TS_ASSERT_DELTA( single->dataX(0)[0], 0.0, 1e-08 );
    TS_ASSERT_DELTA( single->dataY(0)[0], 5.0, 1e-08 );
    TS_ASSERT_DELTA( single->dataE(0)[0], 2.0, 1e-08 );

  }

  
private:
  Mantid::Algorithms::CreateSingleValuedWorkspace algNoErr, algWithErr;
};


#endif //CREATESINGLEVALUEDWORKSPACETEST_H_
