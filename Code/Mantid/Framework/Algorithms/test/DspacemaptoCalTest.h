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
    inputFile = loaderDSP.getPropertyValue("Filename");
    wsName = "LoadEmptyInstrumentTestCAL";
    loaderDSP.setPropertyValue("OutputWorkspace", wsName);
    loaderDSP.execute();
    loaderDSP.isExecuted();

    CreateCalFileByNames createrDSP;

    TS_ASSERT_THROWS_NOTHING(createrDSP.initialize());
    TS_ASSERT_THROWS_NOTHING(createrDSP.isInitialized());
    createrDSP.setPropertyValue("InstrumentName", "INES");
    std::string outputFile;
    outputFile = "./INES_test.cal";
    createrDSP.setPropertyValue("GroupingFileName", outputFile);
    outputFile = createrDSP.getPropertyValue("GroupingFileName");
    createrDSP.setPropertyValue("GroupNames", "bank1A,bank2B,bank3C,bank4D,bank5E,bank6F,bank7G,bank8H,bank9I");

    TS_ASSERT_THROWS_NOTHING(createrDSP.execute());

    DspacemaptoCal testerDSP;

    TS_ASSERT_THROWS_NOTHING(testerDSP.initialize());
    TS_ASSERT_THROWS_NOTHING(testerDSP.isInitialized());
    testerDSP.setPropertyValue("InputWorkspace", wsName);
    std::string dspaceFile;
    dspaceFile = "./INES_dspace.dat";
    std::ofstream fout("./INES_dspace.dat", std::ios_base::out|std::ios_base::binary);
    double read = 3.1992498205034756E-6;
    for (int i=0; i<144; i++)
        fout.write( reinterpret_cast<char*>( &read ), sizeof read );
    fout.close();
    testerDSP.setPropertyValue("DspacemapFile", dspaceFile);
    testerDSP.setPropertyValue("CalibrationFile", outputFile);
    TS_ASSERT_THROWS_NOTHING(testerDSP.execute());
    TS_ASSERT_THROWS_NOTHING(testerDSP.isExecuted());


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
    Poco::File(dspaceFile).remove();
    // Remove workspace
    AnalysisDataService::Instance().remove(wsName);

  }


private:
  std::string inputFile;
  std::string wsName;

};

#endif /*DSPACETOCALTEST_H_*/
