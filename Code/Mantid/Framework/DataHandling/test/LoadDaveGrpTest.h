#ifndef LOADDAVEGRPTEST_H_
#define LOADDAVEGRPTEST_H_

#include "cxxtest/TestSuite.h"
#include "MantidDataHandling/LoadDaveGrp.h"
#include "MantidAPI/AnalysisDataService.h"

using namespace Mantid::API;
using namespace Mantid::DataHandling;

class LoadDaveGrpTest : public CxxTest::TestSuite
{
public:
  void testLoading()
  {
    const std::string outputWSName("dave_grp");
    LoadDaveGrp loader;
    loader.initialize();
    TS_ASSERT_THROWS_NOTHING(loader.setPropertyValue("Filename",
        "DaveAscii.grp"));
    TS_ASSERT_THROWS_NOTHING(loader.setPropertyValue("OutputWorkspace",
        outputWSName));
    loader.execute();

    TS_ASSERT_EQUALS(loader.isExecuted(), true);

    // Check the workspace
    AnalysisDataServiceImpl &dataStore = AnalysisDataService::Instance();
    TS_ASSERT_EQUALS( dataStore.doesExist(outputWSName), true);
    Workspace_sptr output;
    TS_ASSERT_THROWS_NOTHING(output = dataStore.retrieve(outputWSName));
    MatrixWorkspace_sptr outputWS = boost::dynamic_pointer_cast<MatrixWorkspace>(output);
    if(outputWS)
    {
      TS_ASSERT_EQUALS(outputWS->getNumberHistograms(), 28);
      dataStore.remove(outputWSName);
    }
  }
};

#endif // LOADDAVEGRPTEST_H_
