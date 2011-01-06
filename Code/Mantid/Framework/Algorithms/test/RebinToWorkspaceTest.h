#ifndef REBINTOWORKSPACETEST_H_
#define REBINTOWORKSPACETEST_H_

//-------------------
// Includes
//--------------------
#include <cxxtest/TestSuite.h>
#include "MantidAlgorithms/RebinToWorkspace.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"


class RebinToWorkspaceTest : public CxxTest::TestSuite
{
public:

  void testInit()
  {
    TS_ASSERT_THROWS_NOTHING( rebinToWS.initialize() );
    TS_ASSERT( rebinToWS.isInitialized() );
  }

  void testExec()
  {
    if( !rebinToWS.isInitialized() ) rebinToWS.initialize();

    //No properties have been set so should throw if executed
    TS_ASSERT_THROWS( rebinToWS.execute(), std::runtime_error );
    
    //Need to input workspaces to test this
    using namespace Mantid::DataObjects;
    Workspace2D_sptr rebinThis = 
      WorkspaceCreationHelper::Create2DWorkspaceBinned(10, 50, 5.0, 1.0);
    Workspace2D_sptr matchToThis = 
      WorkspaceCreationHelper::Create2DWorkspaceBinned(15, 30, 3.0, 2.5);
    //Register them with the DataService
    using namespace Mantid::API;
    TS_ASSERT_THROWS_NOTHING(AnalysisDataService::Instance().add("rbThis", rebinThis));
    TS_ASSERT_THROWS_NOTHING(AnalysisDataService::Instance().add("matThis", matchToThis));

    //Set the properties for the algorithm
    rebinToWS.setPropertyValue("WorkspaceToRebin", "rbThis");
    rebinToWS.setPropertyValue("WorkspaceToMatch", "matThis");
    std::string outputSpace("testOutput");
    rebinToWS.setPropertyValue("OutputWorkspace", outputSpace);
    
    //Test that the properties are set correctly
    std::string result;
    TS_ASSERT_THROWS_NOTHING( result = rebinToWS.getPropertyValue("WorkspaceToRebin") )
    TS_ASSERT( result == "rbThis" );
    
    TS_ASSERT_THROWS_NOTHING( result = rebinToWS.getPropertyValue("WorkspaceToMatch") )
    TS_ASSERT( result == "matThis" );
    
    TS_ASSERT_THROWS_NOTHING( result = rebinToWS.getPropertyValue("OutputWorkspace") )
    TS_ASSERT( result == outputSpace );

    //Execute the algorithm, testing that it does not throw
    TS_ASSERT_THROWS_NOTHING( rebinToWS.execute() );
    TS_ASSERT( rebinToWS.isExecuted() );

    //Retrieved rebinned workspace
    Workspace_sptr workspace;
    TS_ASSERT_THROWS_NOTHING( workspace = AnalysisDataService::Instance().retrieve(outputSpace) );
    Workspace2D_sptr output2D = boost::dynamic_pointer_cast<Workspace2D>(workspace);

    //Test x-vectors from this and "matchToThis" are the same
    TS_ASSERT_EQUALS(output2D->dataX(0).size(), matchToThis->dataX(0).size() );    
    TS_ASSERT_DIFFERS(output2D->dataX(0).size(), rebinThis->dataX(0).size() );    

    //Test a random x bin for matching value
    TS_ASSERT_EQUALS( output2D->dataX(0)[22], matchToThis->dataX(0)[22] );
  }
  
private:
  Mantid::Algorithms::RebinToWorkspace rebinToWS;
};

#endif //REBINTOWORKSPACETEST_H_
