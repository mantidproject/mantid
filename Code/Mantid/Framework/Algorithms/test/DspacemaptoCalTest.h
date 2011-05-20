#ifndef DSPACETOCALTEST_H_
#define DSPACETOCALTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAlgorithms/DspacemaptoCal.h"
#include "MantidAlgorithms/CreateCalFileByNames.h"
#include "MantidDataHandling/LoadEmptyInstrument.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidGeometry/Instrument/Instrument.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidKernel/Exception.h"
#include "MantidKernel/ConfigService.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/Workspace.h"
#include "MantidAPI/Algorithm.h"
#include "MantidAPI/SpectraDetectorMap.h"
#include "MantidGeometry/Instrument/Component.h"
#include "MantidGeometry/Instrument/FitParameter.h"
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

class DspacemaptoCalTest : public CxxTest::TestSuite
{
public:

  void testINES()
  {
    LoadEmptyInstrument loaderDSP;

    loaderDSP.initialize();
    loaderDSP.isInitialized();
    loaderDSP.setPropertyValue("Filename", ConfigService::Instance().getString(
        "instrumentDefinition.directory")+"/INES_Definition.xml");
    const std::string wsName = "LoadEmptyInstrumentTestCAL";
    loaderDSP.setPropertyValue("OutputWorkspace", wsName);
    loaderDSP.execute();
    loaderDSP.isExecuted();

    CreateCalFileByNames createrDSP;

    TS_ASSERT_THROWS_NOTHING(createrDSP.initialize());
    TS_ASSERT_THROWS_NOTHING(createrDSP.isInitialized());
    createrDSP.setPropertyValue("InstrumentWorkspace", wsName);
    std::string outputFile = "./INES_DspacemaptoCalTest.cal";
    createrDSP.setPropertyValue("GroupingFileName", outputFile);
    outputFile = createrDSP.getPropertyValue("GroupingFileName");
    createrDSP.setPropertyValue("GroupNames", "bank1A,bank2B,bank3C,bank4D,bank5E,bank6F,bank7G,bank8H,bank9I");

    TS_ASSERT_THROWS_NOTHING(createrDSP.execute());

    DspacemaptoCal testerDSP;

    TS_ASSERT_THROWS_NOTHING(testerDSP.initialize());
    TS_ASSERT_THROWS_NOTHING(testerDSP.isInitialized());
    testerDSP.setPropertyValue("InputWorkspace", wsName);
    std::string dspaceFile = "./INES_DspacemaptoCalTest.dat";
    std::ofstream fout("./INES_DspacemaptoCalTest.dat", std::ios_base::out|std::ios_base::binary);
    double read = 3.1992498205034756E-6;
    for (int i=0; i<147; i++)
        fout.write( reinterpret_cast<char*>( &read ), sizeof read );
    fout.close();
    testerDSP.setPropertyValue("DspacemapFile", dspaceFile);
    testerDSP.setPropertyValue("CalibrationFile", outputFile);
    TS_ASSERT_THROWS_NOTHING(testerDSP.execute());
    TS_ASSERT(testerDSP.isExecuted());


    // Remove workspace
    AnalysisDataService::Instance().remove(wsName);
    
    // has the algorithm written a file to disk?
    bool fileExists = false;
    TS_ASSERT( fileExists = Poco::File(outputFile).exists() );

    if ( fileExists )
    {
      // Do a few tests to see if the content of outputFile is what you
      // expect.

      std::ifstream in(outputFile.c_str());

      std::string line;
      int i1,i2,i3,i4;
      double d1;

      for (int i=0; i<2; ++i)
      {
        std::getline (in,line);
      }
      for (int i=0; i<1; ++i)
      {
        in >> i1 >> i2 >> d1 >> i3 >> i4;
      }

      in.close();

      TS_ASSERT_EQUALS(i1,0 );
      TS_ASSERT_EQUALS(i2,1 );
      TS_ASSERT_DELTA( d1, -0.6162, 0.0001 )
      TS_ASSERT_EQUALS(i3,1 );
      TS_ASSERT_EQUALS(i4,1 );


      // remove file created by this algorithm
      Poco::File(outputFile).remove();
      Poco::File dsf(dspaceFile);
      if (dsf.exists()) dsf.remove();
    }
  }



  void doTestVulcan(std::string dspaceFile, std::string fileType)
  {
    LoadEmptyInstrument loaderDSP;

    loaderDSP.initialize();
    loaderDSP.isInitialized();
    loaderDSP.setPropertyValue("Filename", ConfigService::Instance().getString(
        "instrumentDefinition.directory")+"/VULCAN_Definition.xml");
    const std::string wsName = "LoadEmptyInstrumentTestCAL";
    loaderDSP.setPropertyValue("OutputWorkspace", wsName);
    loaderDSP.execute();
    TS_ASSERT( loaderDSP.isExecuted() );

    std::string outputFile = "./VULCAN_dspacemaptocal_test.cal";

    CreateCalFileByNames createrDSP;
    TS_ASSERT_THROWS_NOTHING(createrDSP.initialize());
    TS_ASSERT_THROWS_NOTHING(createrDSP.isInitialized());
    createrDSP.setPropertyValue("InstrumentWorkspace", wsName);
    createrDSP.setPropertyValue("GroupingFileName", outputFile);
    outputFile = createrDSP.getPropertyValue("GroupingFileName");
    createrDSP.setPropertyValue("GroupNames", "");
    createrDSP.execute();
    TS_ASSERT( createrDSP.isExecuted() );


    DspacemaptoCal testerDSP;
    TS_ASSERT_THROWS_NOTHING(testerDSP.initialize());
    TS_ASSERT_THROWS_NOTHING(testerDSP.isInitialized());
    testerDSP.setPropertyValue("InputWorkspace", wsName);
    testerDSP.setPropertyValue("DspacemapFile", dspaceFile);
    testerDSP.setPropertyValue("FileType", fileType);
    testerDSP.setPropertyValue("CalibrationFile", outputFile);
    TS_ASSERT_THROWS_NOTHING(testerDSP.execute());
    TS_ASSERT(testerDSP.isExecuted());


    // Remove workspace
    AnalysisDataService::Instance().remove(wsName);

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

      for (int i=0; i<4; ++i)
      {
        std::getline (in,line);
      }
      for (int i=0; i<1; ++i)
      {
        in >> i1 >> i2 >> d1 >> i3 >> i4;
      }

      in.close();

      TS_ASSERT_EQUALS(i1,2 );
      TS_ASSERT_EQUALS(i2,26250 );
      TS_ASSERT_DELTA( d1, 0.0938575, 0.0001 )
      TS_ASSERT_EQUALS(i3,1 );
      TS_ASSERT_EQUALS(i4,1 );


      // remove file created by this algorithm
      //Poco::File(outputFile).remove();
    }
  }

  void xtestVulcan_ASCII()
  {
    doTestVulcan("pid_offset_vulcan_new.dat", "VULCAN-ASCII");
  }

  void xtestVulcan_Binary()
  {
    doTestVulcan("pid_offset_vulcan_new.dat.bin", "VULCAN-Binary");
  }




};

#endif /*DSPACETOCALTEST_H_*/
