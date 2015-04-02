#ifndef MANTID_MDEVENTS_SAVEISAWQVECTORTEST_H_
#define MANTID_MDEVENTS_SAVEISAWQVECTORTEST_H_

#include "MantidAPI/AnalysisDataService.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidMDAlgorithms/SaveIsawQvector.h"
#include "MantidTestHelpers/MDEventsTestHelper.h"

#include <cxxtest/TestSuite.h>
#include <Poco/File.h>

using Mantid::API::AnalysisDataService;
using Mantid::MDAlgorithms::SaveIsawQvector;

class SaveIsawQvectorTest : public CxxTest::TestSuite
{
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static SaveIsawQvectorTest *createSuite() { return new SaveIsawQvectorTest(); }
  static void destroySuite( SaveIsawQvectorTest *suite ) { delete suite; }


  void test_Init()
  {
    SaveIsawQvector alg;
    TS_ASSERT_THROWS_NOTHING( alg.initialize() )
    TS_ASSERT( alg.isInitialized() )
  }

  void test_exec()
  {
    // Name of the output workspace.
    std::string inWSName("SaveIsawQvectorTest_InputWS");
    std::string outfile = "./SaveIsawQvectorTest.bin";

    // create the test workspace
    int numEventsPer = 100;
    Mantid::DataObjects::EventWorkspace_sptr inputW = Mantid::DataObjects::MDEventsTestHelper::createDiffractionEventWorkspace(numEventsPer);
    AnalysisDataService::Instance().addOrReplace(inWSName, inputW);
    size_t nevents = inputW->getNumberEvents();

    // run the actual algorithm
    SaveIsawQvector alg;
    TS_ASSERT_THROWS_NOTHING( alg.initialize() )
    TS_ASSERT( alg.isInitialized() )
    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("InputWorkspace", inWSName) );
    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("Filename", outfile) );
    TS_ASSERT_THROWS_NOTHING( alg.execute(); );
    TS_ASSERT( alg.isExecuted() );

    // Get the file
    outfile = alg.getPropertyValue("Filename");
    Poco::File poco_file(outfile);
    TS_ASSERT( poco_file.exists() );
    std::size_t bytes = poco_file.getSize();
    TS_ASSERT_EQUALS(bytes / (3*4), nevents); // 3 floats (Qx,Qy,Qz) for each event

    if (Poco::File(outfile).exists())
      Poco::File(outfile).remove();

    // Remove workspace from the data service.
    AnalysisDataService::Instance().remove(inWSName);
  }

};


#endif /* MANTID_MDEVENTS_SAVEISAWQVECTORTEST_H_ */
