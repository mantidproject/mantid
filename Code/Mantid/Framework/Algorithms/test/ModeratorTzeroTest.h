#ifndef MANTID_ALGORITHMS_MODERATORTZEROTEST_H_
#define MANTID_ALGORITHMS_MODERATORTZEROTEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidKernel/Timer.h"
#include "MantidKernel/System.h"
#include "MantidDataHandling/LoadSNSEventNexus.h"
#include "MantidDataHandling/LoadAscii.h"
#include "MantidDataHandling/LoadInstrument.h"
#include "MantidAlgorithms/ModeratorTzero.h"

using namespace Mantid;
using namespace Mantid::Algorithms;
using namespace Mantid::API;

class ModeratorTzeroTest : public CxxTest::TestSuite
{
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static ModeratorTzeroTest *createSuite() { return new ModeratorTzeroTest(); }
  static void destroySuite( ModeratorTzeroTest *suite ) { delete suite; }

  //instruments to test:
  //TOPAZ: no parameters file
  //EQSANS: no deltaE-mode parameter
  //HYSPEC: deltaE-mode='direct'
  //TOSCA: deltaE-mode='indirect', no Moderator.TimeZero parameters
  //BASIS: deltaE-mode='indirect', Moderator.TimeZero parameters found. Will test event and histo files

  //Test several workspace inputs
  void testInit()
  {
    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    TS_ASSERT( alg.isInitialized() );
  }

  void testExec2D()
  {
    //load ASCII histogram file, data
    const std::string inputWStr("inputWS");
    Mantid::DataHandling::LoadAscii loader;
    loader.initialize();
    loader.setPropertyValue("Filename", "BSS_11841_histo.dat");
    loader.setPropertyValue("Unit", "TOF");
    loader.setPropertyValue("OutputWorkspace", inputWStr);
    loader.execute();
    TS_ASSERT(loader.isExecuted() );

    //load the instrument into the workspace
    Mantid::DataHandling::LoadInstrument loaderX;
    loaderX.initialize();
    loaderX.setPropertyValue("InstrumentName", "BASIS");
    loaderX.setPropertyValue("Workspace", inputWStr);
    loaderX.execute();
    TS_ASSERT(loader.isExecuted() );

    if (!alg.isInitialized()) alg.initialize();

    //transform the time-of-flight values
    alg.setPropertyValue("InputWorkspace", inputWStr);
    const std::string outputWStr("outputWS");
    alg.setPropertyValue("OutputWorkspace", outputWStr);
    TS_ASSERT_THROWS_NOTHING( alg.execute() );
    TS_ASSERT( alg.isExecuted() );
  } //end of void testExec2D()

  void testExecEvents()
  {

    //load events file. Input and ouptut are set to be non-equal
    Mantid::DataHandling::LoadEventNexus loader;
    loader.initialize();
    loader.setPropertyValue("Filename", "BSS_11841_event.nxs");
    const std::string inputWStr("inputWS");
    loader.setPropertyValue("OutputWorkspace", inputWStr);
    loader.execute();
    TS_ASSERT(loader.isExecuted() );

    if (!alg.isInitialized()) alg.initialize();

    //transform the time-of-flight values
    alg.setPropertyValue("InputWorkspace", inputWStr);
    const std::string outputWStr("outputWS");
    alg.setPropertyValue("OutputWorkspace", outputWStr);
    TS_ASSERT_THROWS_NOTHING( alg.execute() );
    TS_ASSERT( alg.isExecuted() );

    //retrieve pointers to input and output workspaces
    MatrixWorkspace_sptr inputWS, outputWS;
    TS_ASSERT_THROWS_NOTHING(inputWS=AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(inputWStr));
    TS_ASSERT_THROWS_NOTHING(outputWS=AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(outputWStr));

    //Spectrum index 422 of BSS_11841_event.nxs containing three events
    //std::size_t wkspIndex = 422;
  }

private:
  ModeratorTzero alg;

}; // end of class ModeratorTzeroTest : public CxxTest::TestSuite


#endif /* MANTID_ALGORITHMS_MODERATORTZEROTEST_H_ */
