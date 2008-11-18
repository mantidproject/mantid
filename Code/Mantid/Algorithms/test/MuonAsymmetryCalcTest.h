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
		asymCalc.setPropertyValue("ForwardSpectrum", "0");
		asymCalc.setPropertyValue("BackwardSpectrum", "16");
	}
	
	void testProperties()
	{
		TS_ASSERT_EQUALS( asymCalc.getPropertyValue("Alpha"), "1");
		TS_ASSERT_EQUALS( asymCalc.getPropertyValue("ForwardSpectrum"), "0");
		TS_ASSERT_EQUALS( asymCalc.getPropertyValue("BackwardSpectrum"), "16");
	}
	
	void testExecute()
	{
		TS_ASSERT_THROWS_NOTHING( asymCalc.execute() );
		
		Workspace_const_sptr outputWS = AnalysisDataService::Instance().retrieve("Result");
		
		//Use a range as cxxtest seems to complain about the accuracy
		TS_ASSERT(outputWS->dataY(0)[10] < -0.0578);
		TS_ASSERT(outputWS->dataY(0)[10] > -0.0579);

	}

private:
	MuonAsymmetryCalc asymCalc;
	Mantid::NeXus::LoadMuonNexus loader;

};

#endif /*MUONASYMMETRYCALCTEST_H_*/
