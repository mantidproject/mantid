#ifndef CREATECALFILEBYNAMESTEST_H_
#define CREATECALFILEBYNAMESTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAlgorithms/CreateCalFileByNames.h"
#include "MantidDataHandling/LoadEmptyInstrument.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidGeometry/Instrument/Instrument.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidKernel/Exception.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/Workspace.h"
#include "MantidAPI/Algorithm.h"
#include "MantidAPI/SpectraDetectorMap.h"
#include "MantidGeometry/Instrument/Component.h"
#include "MantidGeometry/Instrument/FitParameter.h"
#include "Poco/File.h"
#include <vector>

using namespace Mantid::API;
using namespace Mantid::Algorithms;
using namespace Mantid::Kernel;
using namespace Mantid::Geometry;
using namespace Mantid::DataHandling;
using namespace Mantid::DataObjects;

class CreateCalFileByNamesTest : public CxxTest::TestSuite
{
public:

  void testSNAP()
  {
    LoadEmptyInstrument loaderCAL;

    loaderCAL.initialize();
    loaderCAL.isInitialized();
    loaderCAL.setPropertyValue("Filename", "../../../../Test/Instrument/SNAP_Definition.xml");
    inputFile = loaderCAL.getPropertyValue("Filename");
    wsName = "LoadEmptyInstrumentTestCAL";
    loaderCAL.setPropertyValue("OutputWorkspace", wsName);
    loaderCAL.execute();
    loaderCAL.isExecuted();

    CreateCalFileByNames testerCAL;

    TS_ASSERT_THROWS_NOTHING(testerCAL.initialize());
    TS_ASSERT_THROWS_NOTHING(testerCAL.isInitialized());
    testerCAL.setPropertyValue("InstrumentName", "SNAP");
    std::string outputFile;
    outputFile = "SNAP_test.cal";
    testerCAL.setPropertyValue("GroupingFileName", outputFile);
    testerCAL.setPropertyValue("GroupNames", "E1,E2,E3,E4,E5,E6,E7,E8,E9,W1,W2,W3,W4,W5,W6,W7,W8,W9");

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

    for (int i=0; i<5; ++i)
    {
      std::getline (in,line);
    }
    for (int i=0; i<1179648; ++i)
    {
      in >> i1 >> i2 >> d1 >> i3 >> i4;
    }

    in.close();

    TS_ASSERT_EQUALS(i1,1179648 );
    TS_ASSERT_EQUALS(i2,65535 );
    TS_ASSERT_EQUALS(d1,0.000000 );
    TS_ASSERT_EQUALS(i3,1 );
    TS_ASSERT_EQUALS(i4,18 );


    // remove file created by this algorithm
    Poco::File(outputFile).remove();

  }


private:
  std::string inputFile;
  std::string wsName;

};

#endif /*CREATECALFILEBYNAMESTEST_H_*/
