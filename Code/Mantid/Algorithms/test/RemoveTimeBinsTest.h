#ifndef REMOVETIMEBINSTEST_H_
#define REMOVETIMEBINSTEST_H_

#include <cxxtest/TestSuite.h>

#include <iostream>
#include <sstream>
#include <string>
#include <stdexcept>

#include "MantidNexus/LoadMuonNexus.h"
#include "MantidDataObjects/Workspace1D.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidAPI/IAlgorithm.h"
#include "MantidAPI/AlgorithmManager.h"
#include "MantidAlgorithms/RemoveTimeBins.h"

using namespace Mantid::Algorithms;
using namespace Mantid::API;
using namespace Mantid::DataObjects;

class RemoveTimeBinsTest : public CxxTest::TestSuite
{
public:

	void testName()
	{
		TS_ASSERT_EQUALS( alg.name(), "RemoveTimeBins" )
	}

	void testCategory()
	{
	TS_ASSERT_EQUALS( alg.category(), "General" )
	}

	void testInit()
	{
		alg.initialize();
		TS_ASSERT( alg.isInitialized() )
	}

	void testSetProperties()
	{
		makeDummyWorkspace2D();
		
		alg.setPropertyValue("InputWorkspace", "input2D");
		alg.setPropertyValue("OutputWorkspace", "output");
		alg.setPropertyValue("StartTimeBin", "0");
		alg.setPropertyValue("EndTimeBin", "0");
		
		TS_ASSERT_EQUALS( alg.getPropertyValue("StartTimeBin"), "0");
		TS_ASSERT_EQUALS( alg.getPropertyValue("EndTimeBin"), "0");
	}
	
	void testExec()
	{
		try 
		{
			TS_ASSERT_EQUALS(alg.execute(),true);
		}
		catch(std::runtime_error e)
		{
			TS_FAIL(e.what());
		}

		Workspace_const_sptr outputWS = AnalysisDataService::Instance().retrieve("output");
		
		TS_ASSERT_EQUALS(outputWS->dataX(0).size(), 1);
		TS_ASSERT_EQUALS(outputWS->dataX(0)[0], 1);
		TS_ASSERT_EQUALS(outputWS->dataY(0)[0], 2);
		
	}
	
	void testRealData()
	{
		Mantid::NeXus::LoadMuonNexus loader;
		loader.initialize();
		loader.setPropertyValue("Filename", "../../../../Test/Nexus/emu00006473.nxs");
		loader.setPropertyValue("OutputWorkspace", "EMU6473");
		loader.execute();
				
		alg2.initialize();
		TS_ASSERT( alg.isInitialized() )
		
		alg2.setPropertyValue("InputWorkspace", "EMU6473");
		alg2.setPropertyValue("OutputWorkspace", "result");
		alg2.setPropertyValue("StartTimeBin", "0");
		alg2.setPropertyValue("EndTimeBin", "6");
		
		try 
		{
			TS_ASSERT_EQUALS(alg2.execute(),true);
		}
		catch(std::runtime_error e)
		{
			TS_FAIL(e.what());
		}
		
		Workspace_const_sptr outputWS = AnalysisDataService::Instance().retrieve("result");
		
		TS_ASSERT_EQUALS(outputWS->dataX(0).size(), 1994);
	}

	void makeDummyWorkspace2D()
	{
		Workspace2D_sptr testWorkspace(new Workspace2D);

		testWorkspace->setTitle("input2D");
		testWorkspace->initialize(2,2,2);

		std::vector<double> X;
		std::vector<double> Y;

		for (int i =0; i < 2; ++i)
		{
			X.push_back(1.0*i);
			Y.push_back(2.0*i);
		}

		testWorkspace->setX(0, X);
		testWorkspace->setX(1, X);
		testWorkspace->setData(0, Y);
		testWorkspace->setData(1, Y);

		AnalysisDataService::Instance().add("input2D", testWorkspace);
	}

private:
	RemoveTimeBins alg;
	RemoveTimeBins alg2;

};

#endif /*REMOVETIMEBINSTEST_H_*/
