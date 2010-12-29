#ifndef SAVERKHTEST_H_
#define SAVERKHTEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidDataHandling/SaveRKH.h"
#include "../../Algorithms/test/WorkspaceCreationHelper.hh"

#include <fstream>
#include "Poco/File.h"

class SaveRKHTest : public CxxTest::TestSuite
{
public:
  ///Constructor
  SaveRKHTest() : outputFile("SAVERKH.out")
  {} 

  ~SaveRKHTest()
  {
    //Remove the file
    Poco::File(outputFile).remove();
  }

  void testInit()
  {
    using namespace Mantid::DataHandling;
    TS_ASSERT_THROWS_NOTHING(testAlgorithm1.initialize());
    TS_ASSERT_EQUALS(testAlgorithm1.isInitialized(), true);
 
    TS_ASSERT_THROWS_NOTHING(testAlgorithm2.initialize());
    TS_ASSERT_EQUALS(testAlgorithm2.isInitialized(), true);
 
  }

  void testExecHorizontal()
  {
    if ( !testAlgorithm1.isInitialized() ) testAlgorithm1.initialize();
    
    //No parameters have been set yet, so it should throw
    TS_ASSERT_THROWS(testAlgorithm1.execute(), std::runtime_error);
    
    //Need a test workspace to use as input
    using namespace Mantid::API;
    MatrixWorkspace_sptr inputWS1 = WorkspaceCreationHelper::Create2DWorkspaceBinned(1, 10, 1.0);
    //Register workspace
    AnalysisDataService::Instance().add("testInputOne", inputWS1);

    testAlgorithm1.setPropertyValue("InputWorkspace", "testInputOne");
    testAlgorithm1.setPropertyValue("Filename", outputFile);
    outputFile = testAlgorithm1.getPropertyValue("Filename"); //get absolute path
    testAlgorithm2.setProperty<bool>("Append", false);

    //Execute the algorithm
    TS_ASSERT_THROWS_NOTHING( testAlgorithm1.execute() );
    TS_ASSERT( testAlgorithm1.isExecuted() );
    
    //Check if the file exists
    TS_ASSERT( Poco::File(outputFile).exists() );

    //Open it and check a few lines
    std::ifstream file(outputFile.c_str());

    TS_ASSERT(file);

    //Bury the first 5 lines and then read the next
    int count(0);
    std::string fileline;
    while( ++count < 7 )
    {
      getline(file, fileline);
    }
    std::istringstream strReader(fileline);
    double x(0.0), y(0.0), err(0.0);
    strReader >> x >> y >> err;

    //Test values
    TS_ASSERT_DELTA(x, 1.5, 1e-08);
    TS_ASSERT_DELTA(y, 2.0, 1e-08);
    TS_ASSERT_DELTA(err, 1.414214, 1e-06);

    //Read some other lines
    count = 0;
    while( ++count < 6 )
    {
      getline(file, fileline);
    }

    x = 0.0;
    y = 0.0;
    err = 0.0;
    strReader.clear();
    strReader.str(fileline.c_str());
    strReader >> x >> y >> err;

    //Test values
    TS_ASSERT_DELTA(x, 6.5, 1e-08);
    TS_ASSERT_DELTA(y, 2.0, 1e-08);
    TS_ASSERT_DELTA(err, 1.414214, 1e-06);
    
    file.close();
  }

  void xtestExecVertical()
  {
    //Now a workspace of the other kind
    if ( !testAlgorithm2.isInitialized() ) testAlgorithm2.initialize();
    
    TS_ASSERT_THROWS(testAlgorithm2.execute(), std::runtime_error);

    using namespace Mantid::API;
    MatrixWorkspace_sptr inputWS2 = WorkspaceCreationHelper::Create2DWorkspaceBinned(10, 1, 0.0);
    //Register workspace
    AnalysisDataService::Instance().add("testInputTwo", inputWS2);

    testAlgorithm2.setPropertyValue("InputWorkspace", "testInputTwo");
    testAlgorithm2.setPropertyValue("Filename", outputFile);
    outputFile = testAlgorithm2.getPropertyValue("Filename"); //get absolute path
    testAlgorithm2.setProperty<bool>("Append", false);

    //Execute the algorithm
    TS_ASSERT_THROWS_NOTHING( testAlgorithm2.execute() );
    TS_ASSERT( testAlgorithm2.isExecuted() );
    
    //Check if the file exists  
    TS_ASSERT( Poco::File(outputFile).exists() );

    //Open it and check a few lines
    std::ifstream file2(outputFile.c_str());

    TS_ASSERT(file2);

    //Bury the first 5 lines and then read the next
    int count = 0;
    std::string fileline;
    while( ++count < 7 )
    {
      getline(file2, fileline);
    }
    std::istringstream strReader(fileline);
    strReader.clear();
    strReader.str(fileline.c_str());
    double x(0.0), y(0.0), err(0.0);
    strReader >> x >> y >> err;

    //Test values
    TS_ASSERT_DELTA(x, 0.0, 1e-08);
    TS_ASSERT_DELTA(y, 2.0, 1e-08);
    TS_ASSERT_DELTA(err, 1.414214, 1e-06);

    //Read some other lines
    count = 0;
    while( ++count < 6 )
    {
      getline(file2, fileline);
    }

    x = 0.0;
    y = 0.0;
    err = 0.0;
    strReader.clear();
    strReader.str(fileline.c_str());
    strReader >> x >> y >> err;

    //Test values
    TS_ASSERT_DELTA(x, 0.0, 1e-08);
    TS_ASSERT_DELTA(y, 2.0, 1e-08);
    TS_ASSERT_DELTA(err, 1.414214, 1e-06);
    
    file2.close();
  }

private:
  ///The algorithm object
  Mantid::DataHandling::SaveRKH testAlgorithm1, testAlgorithm2;

  ///The name of the file to use as output
  std::string outputFile;
};

#endif  //SAVERKHTEST_H_
