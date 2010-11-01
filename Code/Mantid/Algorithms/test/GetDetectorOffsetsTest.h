#ifndef GETDETECTOROFFSETSTEST_H_
#define GETDETECTOROFFSETSTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAlgorithms/GetDetectorOffsets.h"
#include "MantidAlgorithms/CrossCorrelate.h"
#include "MantidAlgorithms/Rebin.h"
#include "MantidAlgorithms/ConvertUnits.h"
#include "MantidDataHandling/LoadEventPreNeXus.h"
#include "MantidAPI/AnalysisDataService.h"
#include "Poco/File.h"

using namespace Mantid::API;
using namespace Mantid::Algorithms;
using namespace Mantid::Kernel;
using namespace Mantid::DataHandling;

class GetDetectorOffsetsTest : public CxxTest::TestSuite
{
public:

  void testPG3()
  {

    //Load large diffraction dataset
    LoadEventPreNeXus loaderCAL;
    loaderCAL.initialize();
    loaderCAL.isInitialized();
    loaderCAL.setPropertyValue("EventFilename", "../../../../Test/Data/sns_event_prenexus/PG3_732_neutron_event.dat");
    wsName = "LoadEventPreNeXusTestCAL";
    loaderCAL.setPropertyValue("OutputWorkspace", wsName);
    loaderCAL.execute();
    loaderCAL.isExecuted();

    //Convert to d-Spacing
    ConvertUnits convertCAL;
    convertCAL.initialize();
    convertCAL.isInitialized();
    convertCAL.setPropertyValue("InputWorkspace",wsName);
    convertCAL.setPropertyValue("OutputWorkspace", wsName);
    convertCAL.setPropertyValue("Target","dSpacing");
    convertCAL.execute();
    convertCAL.isExecuted();

    //Rebin data          
    Rebin rebinCAL;
    rebinCAL.initialize();
    rebinCAL.isInitialized();
    rebinCAL.setPropertyValue("InputWorkspace",wsName);
    rebinCAL.setPropertyValue("OutputWorkspace", wsName);
    rebinCAL.setPropertyValue("Params",".2,-.0004,5.");
    rebinCAL.execute();
    rebinCAL.isExecuted();

    //Cross correlate     
    CrossCorrelate ccCAL;
    ccCAL.initialize();
    ccCAL.isInitialized();
    ccCAL.setPropertyValue("InputWorkspace",wsName);
    ccCAL.setPropertyValue("OutputWorkspace", wsName);
    ccCAL.setPropertyValue("ReferenceSpectra","1185");
    ccCAL.setPropertyValue("WorkspaceIndexMin","0");
    ccCAL.setPropertyValue("WorkspaceIndexMax","11857");
    ccCAL.setPropertyValue("XMin","1.08");
    ccCAL.setPropertyValue("XMax","1.15");
    ccCAL.execute();
    ccCAL.isExecuted();

    //Get detector offsets
    GetDetectorOffsets testerCAL;
    TS_ASSERT_THROWS_NOTHING(testerCAL.initialize());
    TS_ASSERT_THROWS_NOTHING(testerCAL.isInitialized());
    testerCAL.setPropertyValue("InputWorkspace",wsName);
    testerCAL.setPropertyValue("OutputWorkspace",wsName);
    testerCAL.setPropertyValue("Step","0.0004");
    testerCAL.setPropertyValue("DReference","1.1109");
    testerCAL.setPropertyValue("XMin","-50");
    testerCAL.setPropertyValue("XMax","50");
    std::string outputFile;
    outputFile = "PG3_732_test.cal";
    testerCAL.setPropertyValue("GroupingFileName", outputFile);
    TS_ASSERT_THROWS_NOTHING(testerCAL.execute());
    TS_ASSERT_THROWS_NOTHING(testerCAL.isExecuted());


    MatrixWorkspace_sptr output;
    output = boost::dynamic_pointer_cast<MatrixWorkspace>(AnalysisDataService::Instance().retrieve(wsName));
    
    // has the algorithm written a file to disk?

    TS_ASSERT( Poco::File(outputFile).exists() );


    // Do a few tests to see if the content of outputFile is what you
    // expect.

    std::ifstream in(outputFile.c_str());

    std::string line;
    int i1,i2,i3,i4;
    double d1;

    for (int i=0; i<1; ++i)
    {
      std::getline (in,line);
    }
    for (int i=0; i<11858; ++i)
    {
      in >> i1 >> i2 >> d1 >> i3 >> i4;
    }

    in.close();

    TS_ASSERT_EQUALS(i1,11857 );
    TS_ASSERT_EQUALS(i2,180937 );
    TS_ASSERT_EQUALS(d1,-0.1230111 );
    TS_ASSERT_EQUALS(i3,1 );
    TS_ASSERT_EQUALS(i4,1 );


    // remove file created by this algorithm
    Poco::File(outputFile).remove();
    // Remove workspace
    AnalysisDataService::Instance().remove(wsName);

  }


private:
  std::string wsName;

};

#endif /*GETDETECTOROFFSETSTEST_H_*/
