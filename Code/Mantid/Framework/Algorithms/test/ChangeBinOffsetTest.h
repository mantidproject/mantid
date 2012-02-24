#ifndef CHANGEBINOFFSETTEST_H_
#define CHANGEBINOFFSETTEST_H_

#include <cxxtest/TestSuite.h>

#include <iostream>
#include <sstream>
#include <string>

#include "MantidDataObjects/Workspace2D.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidDataHandling/LoadEventPreNexus.h"
#include "MantidAPI/IAlgorithm.h"
#include "MantidAlgorithms/ChangeBinOffset.h"

using namespace Mantid::Algorithms;
using namespace Mantid::API;
using namespace Mantid::DataObjects;

class ChangeBinOffsetTest : public CxxTest::TestSuite
{
public:

	void testExec2D()
	{
		double offset = 1.0;

		std::ostringstream offsetStr;
		offsetStr << offset;

		Workspace2D_sptr input = makeDummyWorkspace2D();
		input->isDistribution(true);
		AnalysisDataService::Instance().add("input2D", input);

		ChangeBinOffset alg2D;
		TS_ASSERT_THROWS_NOTHING(alg2D.initialize());
		TS_ASSERT( alg2D.isInitialized() );

		alg2D.setPropertyValue("InputWorkspace", "input2D");
		alg2D.setPropertyValue("OutputWorkspace", "output2D");
		alg2D.setPropertyValue("Offset", offsetStr.str());


		TS_ASSERT_THROWS_NOTHING(alg2D.execute());
		TS_ASSERT( alg2D.isExecuted() );

		MatrixWorkspace_sptr output = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(alg2D.getProperty("OutputWorkspace"));

		Mantid::MantidVec& Xold = input->dataX(0);
		Mantid::MantidVec& Xnew = output->dataX(0);

//		for (int i=0; i < Xnew.size(); ++i)
//		{
//		    std::cout << "old value: " << Xold[i] << std::endl;
//		    std::cout << "new value: " << Xnew[i] << std::endl;
//		}

		TS_ASSERT(Xold[0] + offset == Xnew[0]);
		TS_ASSERT(Xold[1] + offset == Xnew[1]);

    // check limits
    alg2D.setPropertyValue("IndexMin", "2");
    alg2D.setPropertyValue("IndexMax", "3");
    alg2D.setPropertyValue("OutputWorkspace", "output2D_lims");
    TS_ASSERT_THROWS_NOTHING(alg2D.execute());
		TS_ASSERT( alg2D.isExecuted() );

		output = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>("output2D_lims");

    //check hist 0 is unchanged
		Mantid::MantidVec& Xold0 = input->dataX(0);
		Mantid::MantidVec& Xnew0 = output->dataX(0);
		TS_ASSERT(Xold0[0]  == Xnew0[0]);
		TS_ASSERT(Xold0[1]  == Xnew0[1]);
    //check hist 2 is changed
		Mantid::MantidVec& Xold2 = input->dataX(2);
		Mantid::MantidVec& Xnew2 = output->dataX(2);
		TS_ASSERT(Xold2[0] + offset == Xnew2[0]);
		TS_ASSERT(Xold2[1] + offset == Xnew2[1]);
   

		AnalysisDataService::Instance().remove("input2D");
	}

	Workspace2D_sptr makeDummyWorkspace2D()
	{
		Workspace2D_sptr testWorkspace(new Workspace2D);

		testWorkspace->setTitle("input2D");
		testWorkspace->initialize(5,2,2);
    int jj=0;
		for (int i =0; i < 2; ++i)
		{
      for (jj=0; jj<4; ++jj)
		  testWorkspace->dataX(jj)[i] = 1.0*i;
		  testWorkspace->dataY(jj)[i] = 2.0*i;
		}

		return testWorkspace;
	}

  void setup_Event()
  {
    this->inputSpace = "eventWS";
    Mantid::DataHandling::LoadEventPreNexus loader;
    loader.initialize();
    std::string eventfile( "CNCS_7860_neutron_event.dat" );
    std::string pulsefile( "CNCS_7860_pulseid.dat" );
    loader.setPropertyValue("EventFilename", eventfile);
    loader.setPropertyValue("PulseidFilename", pulsefile);
    loader.setPropertyValue("MappingFilename", "CNCS_TS_2008_08_18.dat");
    loader.setPropertyValue("OutputWorkspace", this->inputSpace);
    loader.execute();
    TS_ASSERT (loader.isExecuted() );
  }

	void testExecEvents()
	{
	  this->setup_Event();
	  std::string outputSpace = "eventWS_out";

	  ChangeBinOffset alg;
    if ( !alg.isInitialized() ) alg.initialize();
    TS_ASSERT( alg.isInitialized() );

    //Set all the properties
    alg.setPropertyValue("InputWorkspace", this->inputSpace);
    alg.setPropertyValue("Offset", "100.0");
    alg.setPropertyValue("OutputWorkspace", outputSpace);

    TS_ASSERT_THROWS_NOTHING( alg.execute() );
    TS_ASSERT( alg.isExecuted() );

    EventWorkspace_sptr WSI = AnalysisDataService::Instance().retrieveWS<EventWorkspace>(this->inputSpace);
    TS_ASSERT(WSI);
    EventWorkspace_sptr WSO = AnalysisDataService::Instance().retrieveWS<EventWorkspace>(outputSpace);
    TS_ASSERT(WSO);

    std::size_t wkspIndex = 4348; // a good workspace index (with events)
    TS_ASSERT_DELTA(WSI->getEventList(wkspIndex).getEvents()[0].tof()+100,
        WSO->getEventList(wkspIndex).getEvents()[0].tof(),0.001);
    TS_ASSERT_DELTA(WSI->getEventList(wkspIndex).dataX()[1]+100.,
        WSO->getEventList(wkspIndex).dataX()[1],0.001);

    alg.setPropertyValue("IndexMin", "4349");
    alg.setPropertyValue("IndexMax", "4350");
    TS_ASSERT_THROWS_NOTHING( alg.execute() );
    TS_ASSERT( alg.isExecuted() );
    WSO = AnalysisDataService::Instance().retrieveWS<EventWorkspace>(outputSpace);
    TS_ASSERT(WSO);
    TS_ASSERT_DELTA(WSI->getEventList(wkspIndex).getEvents()[0].tof(),
        WSO->getEventList(wkspIndex).getEvents()[0].tof(),0.001); //should be unchanged
    TS_ASSERT_DELTA(WSI->getEventList(wkspIndex).dataX()[1],
        WSO->getEventList(wkspIndex).dataX()[1],0.001);//should be unchanged
    TS_ASSERT_DELTA(WSI->getEventList(wkspIndex+1).getEvents()[0].tof()+100,
        WSO->getEventList(wkspIndex+1).getEvents()[0].tof(),0.001);//should change
    TS_ASSERT_DELTA(WSI->getEventList(wkspIndex+1).dataX()[1]+100.,
        WSO->getEventList(wkspIndex+1).dataX()[1],0.001);//should change
	}

private:
	std::string inputSpace;

};

#endif /*CHANGEBINOFFSETTEST_H_*/
