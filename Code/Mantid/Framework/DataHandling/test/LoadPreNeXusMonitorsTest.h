#ifndef LOADPRENEXUSMONITORSTEST_H_
#define LOADPRENEXUSMONITORSTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidDataHandling/LoadPreNeXusMonitors.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/MatrixWorkspace.h"

#include "Poco/Path.h"

class LoadPreNeXusMonitorsTest: public CxxTest::TestSuite
{
public:
  static LoadPreNeXusMonitorsTest *createSuite() { return new LoadPreNeXusMonitorsTest(); }
  static void destroySuite(LoadPreNeXusMonitorsTest *suite) { delete suite; }

  LoadPreNeXusMonitorsTest()
  {
    // Path to test input file assumes Test directory checked out from SVN
	// You will need to make sure the bmon* files are in the same directory
    runinfoFile = "CNCS_7860_runinfo.xml";
  }

  void testInit()
  {
    TS_ASSERT_THROWS_NOTHING(loader.initialize());
    TS_ASSERT(loader.isInitialized());
  }

  void testExec()
  {
    if (!loader.isInitialized())
    {
      loader.initialize();
    }

    std::string outWS("outWS");

    // Check we can set the properties
    TS_ASSERT_THROWS_NOTHING(loader.setPropertyValue("RunInfoFilename", runinfoFile));
    TS_ASSERT_THROWS_NOTHING(loader.setPropertyValue("OutputWorkspace", outWS));
    // Actually run it and test is has been run.
    TS_ASSERT_THROWS_NOTHING(loader.execute());
    TS_ASSERT(loader.isExecuted());

    // Get back the saved workspace
    Mantid::API::MatrixWorkspace_const_sptr ws;
    TS_ASSERT_THROWS_NOTHING(ws = boost::dynamic_pointer_cast<Mantid::API::MatrixWorkspace>(
        Mantid::API::AnalysisDataService::Instance().retrieve(outWS)));

    TS_ASSERT_EQUALS(ws->getNumberHistograms(), 3);
    TS_ASSERT_EQUALS(ws->blocksize(), 200001);

    // Check all the X axes are the same
    TS_ASSERT( (ws->dataX(0)) == (ws->dataX(1)) );

    // Check a particular value
    TS_ASSERT_EQUALS( ws->dataY(1)[3424], 858);

    Mantid::API::AnalysisDataService::Instance().remove(outWS);

  }

private:
  Mantid::DataHandling::LoadPreNeXusMonitors loader;
  std::string runinfoFile;
};

#endif /* LOADPRENEXUSMONITORSTEST_H_ */
