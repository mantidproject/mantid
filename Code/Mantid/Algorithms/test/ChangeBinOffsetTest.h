#ifndef CHANGEBINOFFSETTEST_H_
#define CHANGEBINOFFSETTEST_H_

#include <cxxtest/TestSuite.h>

#include <iostream>
#include <sstream>
#include <string>

#include "MantidDataObjects/Workspace1D.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidDataHandling/LoadEventPreNeXus.h"
#include "MantidAPI/IAlgorithm.h"
#include "MantidAlgorithms/ChangeBinOffset.h"

using namespace Mantid::Algorithms;
using namespace Mantid::API;
using namespace Mantid::DataObjects;

class ChangeBinOffsetTest : public CxxTest::TestSuite
{
public:

	void testExec1D()
	{
		double offset = 2.0;

		std::ostringstream offsetStr;
		offsetStr << offset;

		Workspace1D_sptr input = makeDummyWorkspace1D();
		input->isDistribution(true);
		AnalysisDataService::Instance().add("input1D", input);

		ChangeBinOffset alg1D;
		TS_ASSERT_THROWS_NOTHING(alg1D.initialize());
		TS_ASSERT( alg1D.isInitialized() );

		alg1D.setPropertyValue("InputWorkspace", "input1D");
		alg1D.setPropertyValue("OutputWorkspace", "output1D");
		alg1D.setPropertyValue("Offset", offsetStr.str());

		TS_ASSERT_THROWS_NOTHING(alg1D.execute());
		TS_ASSERT( alg1D.isExecuted() );

    MatrixWorkspace_sptr output = boost::dynamic_pointer_cast<MatrixWorkspace>(AnalysisDataService::Instance(). retrieve(alg1D.getProperty("OutputWorkspace")));

		Mantid::MantidVec& Xold = input->dataX(0);
		Mantid::MantidVec& Xnew = output->dataX(0);

//		for (int i=0; i < Xnew.size(); ++i)
//		{
//		    std::cout << "old value: " << Xold[i] << std::endl;
//		    std::cout << "new value: " << Xnew[i] << std::endl;
//		}

		TS_ASSERT(Xold[0] + offset == Xnew[0]);
		TS_ASSERT(Xold[1] + offset == Xnew[1]);

	}

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

		MatrixWorkspace_sptr output = boost::dynamic_pointer_cast<MatrixWorkspace>(AnalysisDataService::Instance().retrieve(alg2D.getProperty("OutputWorkspace")));

		Mantid::MantidVec& Xold = input->dataX(0);
		Mantid::MantidVec& Xnew = output->dataX(0);

//		for (int i=0; i < Xnew.size(); ++i)
//		{
//		    std::cout << "old value: " << Xold[i] << std::endl;
//		    std::cout << "new value: " << Xnew[i] << std::endl;
//		}

		TS_ASSERT(Xold[0] + offset == Xnew[0]);
		TS_ASSERT(Xold[1] + offset == Xnew[1]);

		AnalysisDataService::Instance().remove("input2D");
	}

	Workspace1D_sptr makeDummyWorkspace1D()
	{
		Workspace1D_sptr testWorkspace(new Workspace1D);

		testWorkspace->setTitle("input1D");
		testWorkspace->initialize(1,2,2);

		for (int i =0; i < 2; ++i)
		{
		  testWorkspace->dataX()[i] = 1.0*i;
		  testWorkspace->dataY()[i] = 2.0*i;
		}

		return testWorkspace;
	}

	Workspace2D_sptr makeDummyWorkspace2D()
	{
		Workspace2D_sptr testWorkspace(new Workspace2D);

		testWorkspace->setTitle("input2D");
		testWorkspace->initialize(2,2,2);

		for (int i =0; i < 2; ++i)
		{
		  testWorkspace->dataX(1)[i] = testWorkspace->dataX(0)[i] = 1.0*i;
		  testWorkspace->dataY(1)[i] = testWorkspace->dataY(0)[i] = 2.0*i;
		}

		return testWorkspace;
	}

  void setup_Event()
  {
    this->inputSpace = "eventWS";
    Mantid::DataHandling::LoadEventPreNeXus loader;
    loader.initialize();
    std::string eventfile( "../../../../Test/AutoTestData/CNCS_11514_neutron_event.dat" );
    std::string pulsefile( "../../../../Test/AutoTestData/CNCS_11514_pulseid.dat" );
    loader.setPropertyValue("EventFilename", eventfile);
    loader.setProperty("PulseidFilename", pulsefile);
    loader.setPropertyValue("MappingFilename", "../../../../Test/AutoTestData/CNCS_TS_2008_08_18.dat");
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

    EventWorkspace_sptr WSI = boost::dynamic_pointer_cast<EventWorkspace>(AnalysisDataService::Instance().retrieve(this->inputSpace));
    TS_ASSERT(WSI);
    EventWorkspace_sptr WSO = boost::dynamic_pointer_cast<EventWorkspace>(AnalysisDataService::Instance().retrieve(outputSpace));
    TS_ASSERT(WSO);

    TS_ASSERT_DIFFERS(WSI->getEventList(0).getEvents()[0].tof(),
        WSO->getEventList(0).getEvents()[0].tof());

    TS_ASSERT_DIFFERS(WSI->getEventList(0).dataX()[1],
        WSO->getEventList(0).dataX()[1]);
	}

private:
	std::string inputSpace;

};

#endif /*CHANGEBINOFFSETTEST_H_*/
