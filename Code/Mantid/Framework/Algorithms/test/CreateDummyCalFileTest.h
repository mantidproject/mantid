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
#include "Poco/File.h"
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
    loaderCAL.setPropertyValue("Filename", "../../../Instrument/INES_Definition.xml");
    inputFile = loaderCAL.getPropertyValue("Filename");
    wsName = "LoadEmptyInstrumentTestCAL";
    loaderCAL.setPropertyValue("OutputWorkspace", wsName);
    loaderCAL.execute();
    loaderCAL.isExecuted();

    CreateDummyCalFile testerCAL;

    TS_ASSERT_THROWS_NOTHING(testerCAL.initialize());
    TS_ASSERT_THROWS_NOTHING(testerCAL.isInitialized());
    testerCAL.setPropertyValue("InputWorkspace", wsName);
    std::string outputFile;
    outputFile = "./INES_test.cal";
    testerCAL.setPropertyValue("CalFilename", outputFile);
    outputFile = testerCAL.getPropertyValue("CalFilename");

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

    for (int i=0; i<4; ++i)
    {
      std::getline (in,line);
    }
    for (int i=0; i<145; ++i)
    {
      in >> i1 >> i2 >> d1 >> i3 >> i4;
    }

    in.close();

    TS_ASSERT_EQUALS(i1,144 );
    TS_ASSERT_EQUALS(i2,144 );
    TS_ASSERT_EQUALS(d1,0.000000 );
    TS_ASSERT_EQUALS(i3,1 );
    TS_ASSERT_EQUALS(i4,1 );


    // remove file created by this algorithm
    Poco::File(outputFile).remove();
    // Remove workspace
    AnalysisDataService::Instance().remove(wsName);

  }


private:
  std::string inputFile;
  std::string wsName;

};

#endif /*CREATEDUMMYCALFILETEST_H_*/
