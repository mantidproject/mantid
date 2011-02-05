#ifndef CREATEDUMMYCALFILETEST_H_
#define CREATEDUMMYCALFILETEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAlgorithms/CreateDummyCalFile.h"
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
#include "MantidKernel/ConfigService.h"
#include <Poco/File.h>
#include <fstream>
#include <cstring>
#include <vector>

using namespace Mantid::API;
using namespace Mantid::Algorithms;
using namespace Mantid::Kernel;
using namespace Mantid::Geometry;
using namespace Mantid::DataHandling;
using namespace Mantid::DataObjects;

class CreateDummyCalFileTest : public CxxTest::TestSuite
{
public:

  void testINES()
  {
    LoadEmptyInstrument loaderCAL;

    loaderCAL.initialize();
    loaderCAL.isInitialized();
    loaderCAL.setPropertyValue("Filename",  ConfigService::Instance().getString(
        "instrumentDefinition.directory")+"/INES_Definition.xml");
    const std::string wsName = "LoadEmptyInstrumentTestCAL";
    loaderCAL.setPropertyValue("OutputWorkspace", wsName);
    loaderCAL.execute();
    loaderCAL.isExecuted();

    CreateDummyCalFile testerCAL;

    TS_ASSERT_THROWS_NOTHING(testerCAL.initialize());
    TS_ASSERT_THROWS_NOTHING(testerCAL.isInitialized());
    testerCAL.setPropertyValue("InputWorkspace", wsName);
    std::string outputFile;
    outputFile = "./INES_CreateDummyCalFileTest.cal";
    testerCAL.setPropertyValue("CalFilename", outputFile);
    outputFile = testerCAL.getPropertyValue("CalFilename");

    TS_ASSERT_THROWS_NOTHING(testerCAL.execute());
    TS_ASSERT_THROWS_NOTHING(testerCAL.isExecuted());

    // has the algorithm written a file to disk?
    bool fileExists;
    TS_ASSERT( fileExists = Poco::File(outputFile).exists() );

    if ( fileExists )
    {
      // Do a few tests to see if the content of outputFile is what you
      // expect.

      std::ifstream in(outputFile.c_str());

      std::string line;
      int i1,i2,i3,i4;
      double d1;

      // Skip header
      for (int i=0; i<4; ++i)
      {
        std::getline (in,line);
      }
      // Check the first line, middle and last lines
      // First line
      in >> i1 >> i2 >> d1 >> i3 >> i4;
      TS_ASSERT_EQUALS(i1,0 );
      TS_ASSERT_EQUALS(i2,145 );
      TS_ASSERT_EQUALS(d1,0.000000 );
      TS_ASSERT_EQUALS(i3,1 );
      TS_ASSERT_EQUALS(i4,1 );

      for (int i=0; i<72; ++i)
      {
        in >> i1 >> i2 >> d1 >> i3 >> i4;
      }
      TS_ASSERT_EQUALS(i1, 72 );
      TS_ASSERT_EQUALS(i2, 71 );
      TS_ASSERT_EQUALS(d1,0.000000 );
      TS_ASSERT_EQUALS(i3,1 );
      TS_ASSERT_EQUALS(i4,1);

      // Final line
      for( int i=0; i<73; ++i )
      {
        in >> i1 >> i2 >> d1 >> i3 >> i4;
      }
      TS_ASSERT_EQUALS(i1,145 );
      TS_ASSERT_EQUALS(i2,144 );
      TS_ASSERT_EQUALS(d1,0.000000 );
      TS_ASSERT_EQUALS(i3,1 );
      TS_ASSERT_EQUALS(i4,1 );

      in.close();

      // remove file created by this algorithm
      Poco::File(outputFile).remove();
      // Remove workspace
      AnalysisDataService::Instance().remove(wsName);
    }
  }

};

#endif /*CREATEDUMMYCALFILETEST_H_*/
