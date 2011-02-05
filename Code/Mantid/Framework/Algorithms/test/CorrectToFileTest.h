#ifndef CORRECTTOFILE_H_
#define CORRECTTOFILE_H_

//-------------------
// Includes
//--------------------
#include <cxxtest/TestSuite.h>
#include "MantidAlgorithms/CorrectToFile.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidDataHandling/LoadRKH.h"
#include "MantidKernel/UnitFactory.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"
#include <Poco/Path.h>

using Mantid::API::AnalysisDataService;
using Mantid::API::MatrixWorkspace_sptr;

class CorrectToFileTest : public CxxTest::TestSuite
{
public:

  CorrectToFileTest() : inputFile("DIRECT.041")
  {}

  void testInit()
  {
    TS_ASSERT_THROWS_NOTHING( correctToFile.initialize() );
    TS_ASSERT( correctToFile.isInitialized() );
  }

  // This algorithm really just runs LoadRKH, RebinToWorkspace and then Divide
  // so given that each of those has its own test, this test does not need to be
  // that complicated
  void testExec2D()
  {
    //Need a workspace to correct
    MatrixWorkspace_sptr testInput = 
      WorkspaceCreationHelper::Create2DWorkspaceBinned(10, 102, 1.5);
    testInput->getAxis(0)->unit() =  Mantid::Kernel::UnitFactory::Instance().create("Wavelength");

    TS_ASSERT_DELTA( testInput->readY(0)[0], 2.0 , 0.0001 );

    MatrixWorkspace_sptr data = executeAlgorithm(testInput, "Wavelength", "Divide");

    TS_ASSERT(data);
    TS_ASSERT_EQUALS( data->getNumberHistograms(), 10 );

    //Sizes are correct
    TS_ASSERT_EQUALS( static_cast<int>(data->readX(0).size()), 103);
    TS_ASSERT_EQUALS( static_cast<int>(data->readY(0).size()), 102);
    TS_ASSERT_EQUALS( static_cast<int>(data->readE(0).size()), 102);

    //value at a single point
    TS_ASSERT_DELTA( data->readY(0)[0], 0.6986 , 0.0001 );

    //cleanup the output workspace
    AnalysisDataService::Instance().remove(data->getName());
  }

  void testExecEvent()
  {
    //Need a workspace to correct
    MatrixWorkspace_sptr testInput = 
      WorkspaceCreationHelper::CreateEventWorkspace(10, 102, 100, 1.5);
    testInput->getAxis(0)->unit() =  Mantid::Kernel::UnitFactory::Instance().create("Wavelength");

    TS_ASSERT_DELTA( testInput->readY(1)[0], 1.0 , 0.0001 );

    MatrixWorkspace_sptr data = executeAlgorithm(testInput, "Wavelength", "Divide");

    TS_ASSERT(data);
    TS_ASSERT_EQUALS( data->getNumberHistograms(), 10 );

    //Sizes are correct
    TS_ASSERT_EQUALS( static_cast<int>(data->readX(0).size()), 103);
    TS_ASSERT_EQUALS( static_cast<int>(data->readY(0).size()), 102);
    TS_ASSERT_EQUALS( static_cast<int>(data->readE(0).size()), 102);

    //value at a single point
    TS_ASSERT_DELTA( data->readY(1)[0], 0.3493 , 0.0001 );


    //cleanup the output workspace
    AnalysisDataService::Instance().remove(data->getName());
  }

  void testSpectraDivide()
  {    //Need a workspace to correct
    MatrixWorkspace_sptr testInput = 
      WorkspaceCreationHelper::Create2DWorkspaceBinned(102, 32, 1.5);

    MatrixWorkspace_sptr data = executeAlgorithm(testInput, "SpectrumNumber", "Divide");

    // the tests aren't extensive because the algorithm just calls the LoadRKH and Divide algorithms and these already have tests
    TS_ASSERT(data);
    TS_ASSERT_EQUALS( data->getNumberHistograms(), 102 );

    //Sizes are correct
    TS_ASSERT_EQUALS( static_cast<int>(data->readX(0).size()), 33);
    TS_ASSERT_EQUALS( static_cast<int>(data->readY(0).size()), 32);
    TS_ASSERT_EQUALS( static_cast<int>(data->readE(0).size()), 32);

    //value at a single point
    TS_ASSERT_DELTA( data->readY(1)[13], 8.7000, 0.0001 );
    TS_ASSERT_DELTA( data->readY(1)[13], 8.7000, 0.0001 );

    //cleanup the output workspace
    AnalysisDataService::Instance().remove(data->getName());
  }


  void testSpectraMultip()
  {    //Need a workspace to correct
    MatrixWorkspace_sptr testInput = 
      WorkspaceCreationHelper::Create2DWorkspaceBinned(102, 32, 1.5);

    MatrixWorkspace_sptr data = executeAlgorithm(testInput, "SpectrumNumber", "Multiply", false);

    // the tests aren't extensive because the algorithm just calls the LoadRKH and Multiply algorithms and these already have tests
    TS_ASSERT(data);
    TS_ASSERT_EQUALS( data->getNumberHistograms(), 102 );

    //Sizes are correct
    TS_ASSERT_EQUALS( static_cast<int>(data->readX(0).size()), 33);
    TS_ASSERT_EQUALS( static_cast<int>(data->readY(0).size()), 32);
    TS_ASSERT_EQUALS( static_cast<int>(data->readE(0).size()), 32);

    //value at a single point
    TS_ASSERT_DELTA( data->readY(7)[5], 0.2791, 0.0001 );
    TS_ASSERT_DELTA( data->readY(7)[5], 0.2791, 0.0001 );

    //cleanup the output workspace
    AnalysisDataService::Instance().remove(data->getName());
  }

  MatrixWorkspace_sptr executeAlgorithm(MatrixWorkspace_sptr testInput, const std::string & unit, const std::string & operation, bool newWksp = true)
  {

    if( !correctToFile.isInitialized() ) correctToFile.initialize();

    //Executing now should throw since none of the properties have been set
    TS_ASSERT_THROWS( correctToFile.execute(), std::runtime_error );

    //Register this with the service
    using namespace Mantid::API;
    TS_ASSERT_THROWS_NOTHING( AnalysisDataService::Instance().add("CorrectThis", testInput) );

    //Set the properties
    TS_ASSERT_THROWS_NOTHING(correctToFile.setPropertyValue("WorkspaceToCorrect", "CorrectThis"));
    TS_ASSERT_THROWS_NOTHING(correctToFile.setPropertyValue("Filename", inputFile));
    TS_ASSERT_THROWS_NOTHING(correctToFile.setPropertyValue("FirstColumnValue", unit));
    TS_ASSERT_THROWS_NOTHING(correctToFile.setPropertyValue("WorkspaceOperation", operation));
    std::string outputSpace("CorrectToFileOutputTest");
    if ( ! newWksp ) outputSpace = correctToFile.getPropertyValue("WorkspaceToCorrect");
    TS_ASSERT_THROWS_NOTHING(correctToFile.setPropertyValue("OutputWorkspace", outputSpace));

    //check that retrieving the output workspace gets the correct value
    std::string result;
    TS_ASSERT_THROWS_NOTHING( result = correctToFile.getPropertyValue("OutputWorkspace") )
    TS_ASSERT( result == outputSpace );

    //Should now not throw anything
    TS_ASSERT_THROWS_NOTHING( correctToFile.execute() );
    TS_ASSERT( correctToFile.isExecuted() );
    
    //Now need to test the resultant workspace, first retrieve it
    Workspace_sptr wkspOut;
    TS_ASSERT_THROWS_NOTHING( wkspOut = AnalysisDataService::Instance().retrieve(outputSpace) );
    MatrixWorkspace_sptr data = boost::dynamic_pointer_cast<MatrixWorkspace>(wkspOut);

    //cleanup the input workspace
    AnalysisDataService::Instance().remove(testInput->getName());

    return data;
  }
  
private:
  Mantid::Algorithms::CorrectToFile correctToFile;
  std::string inputFile;
};

#endif //CORRECTTOFILE_H_
