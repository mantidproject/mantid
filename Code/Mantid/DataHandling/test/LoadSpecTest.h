#ifndef LOADSPECTEST_H
#define LOADSPECTEST_H

//------------------------------------------------
// Includes
//------------------------------------------------

#include <cxxtest/TestSuite.h>

#include "MantidDataHandling/LoadSpec.h"
#include "MantidDataObjects/Workspace1D.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidAPI/IInstrument.h"
#include "MantidGeometry/Instrument/ParameterMap.h"
#include "MantidGeometry/Instrument/Parameter.h"
#include "Poco/Path.h"
#include <vector>

class LoadSpecTest : public CxxTest::TestSuite
{
public:
  LoadSpecTest()
  {
     inputFile = Poco::Path(Poco::Path::current()).resolve("../../../../Test/Data/spec_example.txt").toString();
  }
  void testInit()
  {
    TS_ASSERT_THROWS_NOTHING( loader.initialize());
    TS_ASSERT( loader.isInitialized() );
  }

  void testExec()
  {
    if ( !loader.isInitialized() ) loader.initialize();

    //No parameters have been set yet, so it should throw
    TS_ASSERT_THROWS(loader.execute(), std::runtime_error);

    //Set the file name
    loader.setPropertyValue("Filename", inputFile);
    
    std::string outputSpace = "out_spec_ws";
    //Set an output workspace
    loader.setPropertyValue("OutputWorkspace", outputSpace);
    
    //check that retrieving the filename gets the correct value
    std::string result;
    TS_ASSERT_THROWS_NOTHING( result = loader.getPropertyValue("Filename") )
    TS_ASSERT( result.compare(inputFile) == 0 );

    TS_ASSERT_THROWS_NOTHING( result = loader.getPropertyValue("OutputWorkspace") )
    TS_ASSERT( result == outputSpace );

    //Should now throw nothing
    TS_ASSERT_THROWS_NOTHING( loader.execute() );
    TS_ASSERT( loader.isExecuted() );

   //Now need to test the resultant workspace, first retrieve it
    Mantid::API::Workspace_sptr ws;
    TS_ASSERT_THROWS_NOTHING( ws = Mantid::API::AnalysisDataService::Instance().retrieve(outputSpace) );
    Mantid::DataObjects::Workspace2D_sptr ws2d = boost::dynamic_pointer_cast<Mantid::DataObjects::Workspace2D>(ws);

    TS_ASSERT_EQUALS( ws2d->getNumberHistograms(), 1 );

    TS_ASSERT_EQUALS( (ws2d->dataX(0).size()), 51);
    TS_ASSERT_EQUALS( (ws2d->dataY(0).size()), 51);
    TS_ASSERT_EQUALS( (ws2d->dataE(0).size()), 51);


    double tolerance(1e-04);
    TS_ASSERT_DELTA( ws2d->dataX(0)[0], 0.0323820562087, tolerance );
    TS_ASSERT_DELTA( ws2d->dataX(0)[10], 0.0376905900134, tolerance );
    TS_ASSERT_DELTA( ws2d->dataX(0)[50], 0.109482190039, tolerance );

    TS_ASSERT_DELTA( ws2d->dataY(0)[0], 0.0, tolerance );
    TS_ASSERT_DELTA( ws2d->dataY(0)[10], 2.59507483034, tolerance );
    TS_ASSERT_DELTA( ws2d->dataY(0)[50], 0.0, tolerance );

    TS_ASSERT_DELTA( ws2d->dataE(0)[0], 0.0, tolerance );
    TS_ASSERT_DELTA( ws2d->dataE(0)[10], 0.0124309835217, tolerance );
    TS_ASSERT_DELTA( ws2d->dataE(0)[50], 0.0, tolerance );

    Mantid::API::AnalysisDataService::Instance().remove(outputSpace);
  }

private:
  std::string inputFile;
  Mantid::DataHandling::LoadSpec loader;

};
#endif
