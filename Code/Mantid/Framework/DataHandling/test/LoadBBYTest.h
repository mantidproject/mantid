
#ifndef LOADMCSTASTEST_H_
#define LOADMCSTASTEST_H_

#include <fstream>
#include <cxxtest/TestSuite.h>

#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidDataHandling/LoadBBY.h"
#include "MantidDataObjects/WorkspaceSingleValue.h" 
#include "MantidDataHandling/LoadInstrument.h" 
#include <Poco/Path.h>
#include <Poco/TemporaryFile.h>

using namespace Mantid::API;
using namespace Mantid::Kernel;
using namespace Mantid::DataHandling;
using namespace Mantid::DataObjects;


class LoadBBYTest : public CxxTest::TestSuite
{
public: 
  
  void testInit()
  {
    TS_ASSERT_THROWS_NOTHING(algToBeTested.initialize());
    TS_ASSERT( algToBeTested.isInitialized() );
  }
  

  // test loading of bilby dataset
  void testExec()
  {
    if ( !algToBeTested.isInitialized() ) algToBeTested.initialize();
  
    outputSpace="LoadBBYTest";
    algToBeTested.setPropertyValue("OutputWorkspace", outputSpace);     
    
    // Should fail because mandatory parameter has not been set
    TS_ASSERT_THROWS(algToBeTested.execute(),std::runtime_error);
        
    inputFile = "BBY0000014.tar";
    algToBeTested.setPropertyValue("Filename", inputFile);
    TS_ASSERT_THROWS_NOTHING(algToBeTested.execute());    
    TS_ASSERT( algToBeTested.isExecuted() );

    //  get workspace generated
    MatrixWorkspace_sptr output = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(outputSpace);

    // test it is as expected
    TS_ASSERT_EQUALS( output->getNumberHistograms(), 61440);  
    double sum = 0.0;
    for (size_t i = 0; i < output->getNumberHistograms(); i++)
      sum += output->readY(i)[0];
    sum *= 1.0e22;
    TS_ASSERT_DELTA(sum / 1.0E27,0.9981,0.0001); 
  
  }

 
private:
  LoadBBY algToBeTested;
  std::string inputFile;
  std::string outputSpace;

};

#endif /*LoadBBYTEST_H_*/
