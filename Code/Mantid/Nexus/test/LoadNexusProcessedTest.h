#ifndef LOADNEXUSPROCESSEDTEST_H_
#define LOADNEXUSPROCESSEDTEST_H_

#include <iostream>
#include <cxxtest/TestSuite.h>
#include "MantidNexus/LoadNexusProcessed.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/Instrument.h"
#include "MantidDataHandling/LoadInstrument.h"

using namespace Mantid::NeXus;
using namespace Mantid::API;

class LoadNexusProcessedTest : public CxxTest::TestSuite
{
public:

  LoadNexusProcessedTest() :
    testFile("../../../../Test/Data/GEM38370_Focussed_Legacy.nxs"),
    output_ws("nxstest")
  {

  }

  ~LoadNexusProcessedTest()
  {
    AnalysisDataService::Instance().clear();
  }

  void testProcessedFile()
  {

    LoadNexusProcessed alg;

    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    TS_ASSERT( alg.isInitialized() );

    alg.setProperty("Filename", testFile);
    alg.setPropertyValue("OutputWorkspace", output_ws);

    TS_ASSERT_THROWS_NOTHING(alg.execute());
    
    //Test some aspects of the file
    Workspace_sptr workspace;
    TS_ASSERT_THROWS_NOTHING( workspace = AnalysisDataService::Instance().retrieve(output_ws) );
    TS_ASSERT( workspace.get() );

    MatrixWorkspace_sptr matrix_ws = boost::dynamic_pointer_cast<MatrixWorkspace>(workspace);
    TS_ASSERT( matrix_ws.get() );


    // Test proton charge from the sample block
    TS_ASSERT_DELTA(matrix_ws->sample().getProtonCharge(), 30.14816, 1e-5)

    //Test history
    const std::vector<AlgorithmHistory>& alghist = matrix_ws->getHistory().getAlgorithmHistories();
    int nalgs = static_cast<int>(alghist.size());
    TS_ASSERT_EQUALS(nalgs, 4)
      
    if( nalgs == 4 )
    {
      TS_ASSERT_EQUALS(alghist[0].name(), "LoadRaw");
      TS_ASSERT_EQUALS(alghist[1].name(), "AlignDetectors");
      TS_ASSERT_EQUALS(alghist[2].name(), "DiffractionFocussing");
      TS_ASSERT_EQUALS(alghist[3].name(), "LoadNexusProcessed");
    }
    
    boost::shared_ptr<Instrument> inst = matrix_ws->getBaseInstrument(); 
    TS_ASSERT_EQUALS(inst->getName(), "GEM")
    TS_ASSERT_EQUALS(inst->getSource()->getPos().Z(), -17)

   }



private:
  LoadNexusProcessed algToBeTested;
  std::string testFile, output_ws;
};

#endif /*LOADNEXUSPROCESSEDTESTRAW_H_*/
