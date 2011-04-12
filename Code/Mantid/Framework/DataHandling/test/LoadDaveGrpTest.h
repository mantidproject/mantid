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
    TS_ASSERT_THROWS_NOTHING(loader.setPropertyValue("XAxisUnits", "DeltaE"));
    TS_ASSERT_THROWS_NOTHING(loader.setPropertyValue("YAxisUnits",
        "MomentumTransfer"));
    TS_ASSERT_THROWS_NOTHING(loader.setProperty<bool>("IsMicroEV", true));
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
      TS_ASSERT_EQUALS(outputWS->readX(0).size(), 60);
      TS_ASSERT_DELTA(outputWS->readX(0)[0], 0.655, 1e-6);
      TS_ASSERT_EQUALS((*(outputWS->getAxis(1)))(1), 0.625);
      TS_ASSERT_DELTA(outputWS->readY(0)[1], 0.000106102311091, 1e-6);
      TS_ASSERT_DELTA(outputWS->readY(11)[59], 0.0116074689604, 1e-6);
      TS_ASSERT_DELTA(outputWS->readE(27)[7], 0.0187950781228, 1e-6);

      TS_ASSERT_EQUALS(outputWS->getAxis(0)->unit()->unitID(), "DeltaE");
      TS_ASSERT_EQUALS(outputWS->getAxis(1)->unit()->unitID(), "MomentumTransfer");

      TS_ASSERT_EQUALS(outputWS->isDistribution(), true);

      dataStore.remove(outputWSName);
    }
  }
};

#endif // LOADDAVEGRPTEST_H_
