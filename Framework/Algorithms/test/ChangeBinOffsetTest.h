#ifndef CHANGEBINOFFSETTEST_H_
#define CHANGEBINOFFSETTEST_H_

#include <cxxtest/TestSuite.h>

#include <sstream>
#include <string>

#include "MantidTestHelpers/WorkspaceCreationHelper.h"

#include "MantidDataObjects/Workspace2D.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidDataHandling/LoadEventPreNexus2.h"
#include "MantidAPI/IAlgorithm.h"
#include "MantidAlgorithms/ChangeBinOffset.h"

using namespace Mantid::Algorithms;
using namespace Mantid::API;
using namespace Mantid::DataObjects;

class ChangeBinOffsetTest : public CxxTest::TestSuite {
public:
  void testExec2D() {
    double offset = 1.0;

    std::ostringstream offsetStr;
    offsetStr << offset;

    Workspace2D_sptr input = makeDummyWorkspace2D();
    input->setDistribution(true);
    AnalysisDataService::Instance().add("input2D", input);

    ChangeBinOffset alg2D;
    TS_ASSERT_THROWS_NOTHING(alg2D.initialize());
    TS_ASSERT(alg2D.isInitialized());

    alg2D.setPropertyValue("InputWorkspace", "input2D");
    alg2D.setPropertyValue("OutputWorkspace", "output2D");
    alg2D.setPropertyValue("Offset", offsetStr.str());

    TS_ASSERT_THROWS_NOTHING(alg2D.execute());
    TS_ASSERT(alg2D.isExecuted());

    MatrixWorkspace_sptr output =
        AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(
            alg2D.getProperty("OutputWorkspace"));

    Mantid::MantidVec &Xold = input->dataX(0);
    Mantid::MantidVec &Xnew = output->dataX(0);

    //		for (int i=0; i < Xnew.size(); ++i)
    //		{
    //		    std::cout << "old value: " << Xold[i] << '\n';
    //		    std::cout << "new value: " << Xnew[i] << '\n';
    //		}

    TS_ASSERT(Xold[0] + offset == Xnew[0]);
    TS_ASSERT(Xold[1] + offset == Xnew[1]);

    // check limits
    alg2D.setPropertyValue("IndexMin", "2");
    alg2D.setPropertyValue("IndexMax", "3");
    alg2D.setPropertyValue("OutputWorkspace", "output2D_lims");
    TS_ASSERT_THROWS_NOTHING(alg2D.execute());
    TS_ASSERT(alg2D.isExecuted());

    output = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(
        "output2D_lims");

    // check hist 0 is unchanged
    Mantid::MantidVec &Xold0 = input->dataX(0);
    Mantid::MantidVec &Xnew0 = output->dataX(0);
    TS_ASSERT(Xold0[0] == Xnew0[0]);
    TS_ASSERT(Xold0[1] == Xnew0[1]);
    // check hist 2 is changed
    Mantid::MantidVec &Xold2 = input->dataX(2);
    Mantid::MantidVec &Xnew2 = output->dataX(2);
    TS_ASSERT(Xold2[0] + offset == Xnew2[0]);
    TS_ASSERT(Xold2[1] + offset == Xnew2[1]);

    AnalysisDataService::Instance().remove("input2D");
  }

  Workspace2D_sptr makeDummyWorkspace2D() {
    Workspace2D_sptr testWorkspace(new Workspace2D);

    testWorkspace->setTitle("input2D");
    testWorkspace->initialize(5, 2, 2);
    int jj = 0;
    for (int i = 0; i < 2; ++i) {
      for (jj = 0; jj < 4; ++jj)
        testWorkspace->dataX(jj)[i] = 1.0 * i;
      testWorkspace->dataY(jj)[i] = 2.0 * i;
    }

    return testWorkspace;
  }

  void setup_Event() {
    this->inputSpace = "eventWS";
    Mantid::DataHandling::LoadEventPreNexus2 loader;
    loader.initialize();
    std::string eventfile("CNCS_7860_neutron_event.dat");
    std::string pulsefile("CNCS_7860_pulseid.dat");
    loader.setPropertyValue("EventFilename", eventfile);
    loader.setPropertyValue("PulseidFilename", pulsefile);
    loader.setPropertyValue("MappingFilename", "CNCS_TS_2008_08_18.dat");
    loader.setPropertyValue("OutputWorkspace", this->inputSpace);
    loader.execute();
    TS_ASSERT(loader.isExecuted());
  }

  void testExecEvents() {
    this->setup_Event();
    std::string outputSpace = "eventWS_out";

    ChangeBinOffset alg;
    if (!alg.isInitialized())
      alg.initialize();
    TS_ASSERT(alg.isInitialized());

    // Set all the properties
    alg.setPropertyValue("InputWorkspace", this->inputSpace);
    alg.setPropertyValue("Offset", "100.0");
    alg.setPropertyValue("OutputWorkspace", outputSpace);

    TS_ASSERT_THROWS_NOTHING(alg.execute());
    TS_ASSERT(alg.isExecuted());

    EventWorkspace_sptr WSI =
        AnalysisDataService::Instance().retrieveWS<EventWorkspace>(
            this->inputSpace);
    TS_ASSERT(WSI);
    EventWorkspace_sptr WSO =
        AnalysisDataService::Instance().retrieveWS<EventWorkspace>(outputSpace);
    TS_ASSERT(WSO);

    std::size_t wkspIndex = 4348; // a good workspace index (with events)
    TS_ASSERT_DELTA(WSI->getSpectrum(wkspIndex).getEvents()[0].tof() + 100,
                    WSO->getSpectrum(wkspIndex).getEvents()[0].tof(), 0.001);
    TS_ASSERT_DELTA(WSI->getSpectrum(wkspIndex).dataX()[1] + 100.,
                    WSO->getSpectrum(wkspIndex).dataX()[1], 0.001);

    alg.setPropertyValue("IndexMin", "4349");
    alg.setPropertyValue("IndexMax", "4350");
    TS_ASSERT_THROWS_NOTHING(alg.execute());
    TS_ASSERT(alg.isExecuted());
    WSO =
        AnalysisDataService::Instance().retrieveWS<EventWorkspace>(outputSpace);
    TS_ASSERT(WSO);
    TS_ASSERT_DELTA(WSI->getSpectrum(wkspIndex).getEvents()[0].tof(),
                    WSO->getSpectrum(wkspIndex).getEvents()[0].tof(),
                    0.001); // should be unchanged
    TS_ASSERT_DELTA(WSI->getSpectrum(wkspIndex).dataX()[1],
                    WSO->getSpectrum(wkspIndex).dataX()[1],
                    0.001); // should be unchanged
    TS_ASSERT_DELTA(WSI->getSpectrum(wkspIndex + 1).getEvents()[0].tof() + 100,
                    WSO->getSpectrum(wkspIndex + 1).getEvents()[0].tof(),
                    0.001); // should change
    TS_ASSERT_DELTA(WSI->getSpectrum(wkspIndex + 1).dataX()[1] + 100.,
                    WSO->getSpectrum(wkspIndex + 1).dataX()[1],
                    0.001); // should change
  }

private:
  std::string inputSpace;
};

class ChangeBinOffsetTestPerformance : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static ChangeBinOffsetTestPerformance *createSuite() {
    return new ChangeBinOffsetTestPerformance();
  }
  static void destroySuite(ChangeBinOffsetTestPerformance *suite) {
    AnalysisDataService::Instance().clear();
    delete suite;
  }

  ChangeBinOffsetTestPerformance() {
    input = WorkspaceCreationHelper::create2DWorkspaceBinned(10000, 1000);
    inputEvent =
        WorkspaceCreationHelper::createEventWorkspace(10000, 1000, 5000);
  }

  void testExec2D() {
    ChangeBinOffset alg;
    alg.initialize();
    alg.setProperty("InputWorkspace", input);
    alg.setProperty("Offset", 100.0);
    alg.setPropertyValue("OutputWorkspace", "output");
    alg.execute();
  }

  void testExecEvent() {
    ChangeBinOffset alg;
    alg.initialize();
    alg.setProperty("InputWorkspace", inputEvent);
    alg.setProperty("Offset", 100.0);
    alg.setPropertyValue("OutputWorkspace", "output2");
    alg.execute();
  }

private:
  MatrixWorkspace_sptr input;
  EventWorkspace_sptr inputEvent;
};

#endif /*CHANGEBINOFFSETTEST_H_*/
