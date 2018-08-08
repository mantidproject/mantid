#ifndef LOADDAVEGRPTEST_H_
#define LOADDAVEGRPTEST_H_

#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/Axis.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/Run.h"
#include "MantidDataHandling/LoadDaveGrp.h"
#include "MantidKernel/Unit.h"
#include "cxxtest/TestSuite.h"

using namespace Mantid::API;
using namespace Mantid::DataHandling;

class LoadDaveGrpTest : public CxxTest::TestSuite {
public:
  void testLoading() {
    const std::string outputWSName("dave_grp");
    LoadDaveGrp loader;
    loader.initialize();
    TS_ASSERT_THROWS_NOTHING(
        loader.setPropertyValue("Filename", "DaveAscii.grp"));
    TS_ASSERT_THROWS_NOTHING(
        loader.setPropertyValue("OutputWorkspace", outputWSName));
    TS_ASSERT_THROWS_NOTHING(loader.setPropertyValue("XAxisUnits", "DeltaE"));
    TS_ASSERT_THROWS_NOTHING(
        loader.setPropertyValue("YAxisUnits", "MomentumTransfer"));
    TS_ASSERT_THROWS_NOTHING(loader.setProperty<bool>("IsMicroEV", true));
    loader.execute();

    TS_ASSERT_EQUALS(loader.isExecuted(), true);

    // Check the workspace
    AnalysisDataServiceImpl &dataStore = AnalysisDataService::Instance();
    TS_ASSERT_EQUALS(dataStore.doesExist(outputWSName), true);
    Workspace_sptr output;
    TS_ASSERT_THROWS_NOTHING(output = dataStore.retrieve(outputWSName));
    MatrixWorkspace_sptr outputWS =
        boost::dynamic_pointer_cast<MatrixWorkspace>(output);
    if (outputWS) {
      TS_ASSERT_EQUALS(outputWS->getNumberHistograms(), 28);
      TS_ASSERT_EQUALS(outputWS->x(0).size(), 60);
      TS_ASSERT_DELTA(outputWS->x(0)[0], 0.655, 1e-6);
      TS_ASSERT_EQUALS((*(outputWS->getAxis(1)))(1), 0.625);
      TS_ASSERT_DELTA(outputWS->y(0)[1], 0.000106102311091, 1e-6);
      TS_ASSERT_DELTA(outputWS->y(11)[59], 0.0116074689604, 1e-6);
      TS_ASSERT_DELTA(outputWS->e(27)[7], 0.0187950781228, 1e-6);

      TS_ASSERT_EQUALS(outputWS->getAxis(0)->unit()->unitID(), "DeltaE");
      TS_ASSERT_EQUALS(outputWS->getAxis(1)->unit()->unitID(),
                       "MomentumTransfer");

      TS_ASSERT_EQUALS(outputWS->isDistribution(), true);

      // Check if filename is saved
      TS_ASSERT_EQUALS(loader.getPropertyValue("Filename"),
                       outputWS->run().getProperty("Filename")->value());
      dataStore.remove(outputWSName);
    }
  }

  void testHistogramOutput() {
    const std::string outputWSName("dave_grp");
    LoadDaveGrp loader;
    loader.initialize();
    TS_ASSERT_THROWS_NOTHING(
        loader.setPropertyValue("Filename", "DaveAscii.grp"));
    TS_ASSERT_THROWS_NOTHING(
        loader.setPropertyValue("OutputWorkspace", outputWSName));
    TS_ASSERT_THROWS_NOTHING(loader.setPropertyValue("XAxisUnits", "DeltaE"));
    TS_ASSERT_THROWS_NOTHING(
        loader.setPropertyValue("YAxisUnits", "MomentumTransfer"));
    TS_ASSERT_THROWS_NOTHING(loader.setProperty<bool>("IsMicroEV", true));
    TS_ASSERT_THROWS_NOTHING(
        loader.setProperty<bool>("ConvertToHistogram", true));
    loader.execute();

    TS_ASSERT_EQUALS(loader.isExecuted(), true);

    // Check the workspace
    AnalysisDataServiceImpl &dataStore = AnalysisDataService::Instance();
    TS_ASSERT_EQUALS(dataStore.doesExist(outputWSName), true);
    Workspace_sptr output;
    TS_ASSERT_THROWS_NOTHING(output = dataStore.retrieve(outputWSName));
    MatrixWorkspace_sptr outputWS =
        boost::dynamic_pointer_cast<MatrixWorkspace>(output);
    if (outputWS) {
      TS_ASSERT_EQUALS(outputWS->getNumberHistograms(), 28);
      TS_ASSERT_EQUALS(outputWS->x(0).size(), 61);
      TS_ASSERT_EQUALS(outputWS->y(0).size(), 60);
      dataStore.remove(outputWSName);
    }
  }
};

#endif // LOADDAVEGRPTEST_H_
