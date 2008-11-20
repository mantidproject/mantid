#ifndef MUONASYMMETRYCALCTEST_H_
#define MUONASYMMETRYCALCTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidNexus/LoadMuonNexus.h"
#include "MantidAPI/IAlgorithm.h"
#include "MantidAlgorithms/MuonAsymmetryCalc.h"
#include "MantidAPI/Workspace.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidAPI/AnalysisDataService.h"

using namespace Mantid::Algorithms;
using namespace Mantid::API;

class MuonAsymmetryCalcTest : public CxxTest::TestSuite
{
public:
	
	void testName()
	{
		TS_ASSERT_EQUALS( asymCalc.name(), "MuonAsymmetryCalc" )
	}

	void testCategory()
	{
	TS_ASSERT_EQUALS( asymCalc.category(), "Muon" )
	}

	void testInit()
	{
	  asymCalc.initialize();
	  TS_ASSERT( asymCalc.isInitialized() )
	}

	void testLoadNexusAndSetProperties()
	{
		loader.initialize();
		loader.setPropertyValue("Filename", "../../../../Test/Nexus/emu00006473.nxs");
		loader.setPropertyValue("OutputWorkspace", "EMU6473");
		TS_ASSERT_THROWS_NOTHING( loader.execute() )
		
		asymCalc.setPropertyValue("InputWorkspace", "EMU6473");
		asymCalc.setPropertyValue("OutputWorkspace", "Result");
		asymCalc.setPropertyValue("Alpha", "1.0");
		asymCalc.setPropertyValue("ForwardSpectra", "0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15");
		asymCalc.setPropertyValue("BackwardSpectra", "16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31");
	}
	
	void testProperties()
	{
		TS_ASSERT_EQUALS( asymCalc.getPropertyValue("Alpha"), "1");
		//TS_ASSERT_EQUALS( asymCalc.getPropertyValue("ForwardSpectra"), "0");
		//TS_ASSERT_EQUALS( asymCalc.getPropertyValue("BackwardSpectra"), "16");
	}
	
	void testExecute()
	{
		TS_ASSERT_THROWS_NOTHING( asymCalc.execute() );
		
		Workspace_const_sptr outputWS = AnalysisDataService::Instance().retrieve("Result");
		
		//Use a range as cxxtest seems to complain about the accuracy
		TS_ASSERT(outputWS->dataY(0)[100] >= 0.296);
		TS_ASSERT(outputWS->dataY(0)[100] <= 0.297);

	}

private:
	MuonAsymmetryCalc asymCalc;
	Mantid::NeXus::LoadMuonNexus loader;

};

#endif /*MUONASYMMETRYCALCTEST_H_*/
