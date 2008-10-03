#ifndef MUONASYMMETRYCALCTEST_H_
#define MUONASYMMETRYCALCTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidNexus/LoadMuonNexus.h"
#include "MantidAPI/IAlgorithm.h"
#include "MantidAlgorithms/MuonAsymmetryCalc.h"

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
		asymCalc.setPropertyValue("OutputWorkspace", "AsymCalc");
	}

private:
	MuonAsymmetryCalc asymCalc;
	Mantid::NeXus::LoadMuonNexus loader;

};

#endif /*MUONASYMMETRYCALCTEST_H_*/
