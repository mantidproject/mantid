#ifndef RemoveBinsTest_H_
#define RemoveBinsTest_H_

#include <cxxtest/TestSuite.h>

#include <iostream>
#include <sstream>
#include <string>
#include <stdexcept>

#include "MantidAlgorithms/RemoveBins.h"
#include "MantidNexus/LoadMuonNexus.h"
#include "MantidDataHandling/LoadInstrument.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidKernel/UnitFactory.h"

using namespace Mantid::Algorithms;
using namespace Mantid::API;
using namespace Mantid::DataObjects;

class RemoveBinsTest : public CxxTest::TestSuite
{
public:

	void testName()
	{
		TS_ASSERT_EQUALS( alg.name(), "RemoveBins" )
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
		alg.setPropertyValue("XMin", "0");
		alg.setPropertyValue("XMax", "5");
		
		TS_ASSERT_EQUALS( alg.getPropertyValue("XMin"), "0");
		TS_ASSERT_EQUALS( alg.getPropertyValue("XMax"), "5");
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

		MatrixWorkspace_const_sptr outputWS = boost::dynamic_pointer_cast<MatrixWorkspace>(AnalysisDataService::Instance().retrieve("output"));

		//Should give:
		//10   20   30   40   X
		//     2     5     6       Y
		
		TS_ASSERT_EQUALS(outputWS->dataX(0).size(), 4);
		TS_ASSERT_EQUALS(outputWS->dataY(0).size(), 3);
		TS_ASSERT_EQUALS(outputWS->dataX(0)[0], 10);
		TS_ASSERT_EQUALS(outputWS->dataY(0)[0], 2);

	}
	
	void testRemoveFromBack()
	{
		alg3.initialize();
		TS_ASSERT( alg3.isInitialized() )
		
		
		alg3.setPropertyValue("InputWorkspace", "input2D");
		alg3.setPropertyValue("OutputWorkspace", "output2");
		alg3.setPropertyValue("XMin", "35");
		alg3.setPropertyValue("XMax", "40");
		
		TS_ASSERT_EQUALS( alg3.getPropertyValue("XMin"), "35");
		TS_ASSERT_EQUALS( alg3.getPropertyValue("XMax"), "40");
		
		try 
		{
			TS_ASSERT_EQUALS(alg3.execute(),true);
		}
		catch(std::runtime_error e)
		{
			TS_FAIL(e.what());
		}

		MatrixWorkspace_const_sptr outputWS = boost::dynamic_pointer_cast<MatrixWorkspace>(AnalysisDataService::Instance().retrieve("output2"));

		//0   10   20   30    X
		//   0     2     5        Y
		
		TS_ASSERT_EQUALS(outputWS->dataX(0).size(), 4);
		TS_ASSERT_EQUALS(outputWS->dataY(0).size(), 3);
		TS_ASSERT_EQUALS(outputWS->dataX(0)[0], 0);
		TS_ASSERT_EQUALS(outputWS->dataY(0)[0], 0);
		TS_ASSERT_EQUALS(outputWS->dataX(0)[3], 30);
		TS_ASSERT_EQUALS(outputWS->dataY(0)[2], 5);
		
	}
	
	void testRemoveFromMiddle()
	{
		alg4.initialize();
		TS_ASSERT( alg4.isInitialized() )	
		alg4.setPropertyValue("InputWorkspace", "input2D");
		alg4.setPropertyValue("OutputWorkspace", "output3");
		alg4.setPropertyValue("XMin", "11");
		alg4.setPropertyValue("XMax", "21");
		alg4.setPropertyValue("Interpolation", "Linear");
		
		TS_ASSERT_EQUALS( alg4.getPropertyValue("XMin"), "11");
		TS_ASSERT_EQUALS( alg4.getPropertyValue("XMax"), "21");
		TS_ASSERT_EQUALS( alg4.getPropertyValue("Interpolation"), "Linear");
		
		try 
		{
			TS_ASSERT_EQUALS(alg4.execute(),true);
		}
		catch(std::runtime_error e)
		{
			TS_FAIL(e.what());
		}

		MatrixWorkspace_const_sptr outputWS = boost::dynamic_pointer_cast<MatrixWorkspace>(AnalysisDataService::Instance().retrieve("output3"));

		//0   10   20   30   40   X
		//   0     2     4     6       Y
		
		TS_ASSERT_EQUALS(outputWS->dataX(0).size(), 5);
		TS_ASSERT_EQUALS(outputWS->dataY(0).size(), 4);
		TS_ASSERT_EQUALS(outputWS->dataX(0)[0], 0);
		TS_ASSERT_EQUALS(outputWS->dataX(0)[3], 30);
    TS_ASSERT_EQUALS(outputWS->dataY(0)[0], 0);
    TS_ASSERT_EQUALS(outputWS->dataY(0)[1], 1.5);
		TS_ASSERT_EQUALS(outputWS->dataY(0)[2], 3);
		TS_ASSERT_EQUALS(outputWS->dataY(0)[3], 6);
	}
	
  void testSingleSpectrum()
  {
    RemoveBins rb;
    TS_ASSERT_THROWS_NOTHING( rb.initialize() )
		TS_ASSERT( rb.isInitialized() )	
		rb.setPropertyValue("InputWorkspace", "input2D");
		rb.setPropertyValue("OutputWorkspace", "output4");
		rb.setPropertyValue("XMin", "0");
		rb.setPropertyValue("XMax", "40");
    rb.setPropertyValue("WorkspaceIndex","0");

    TS_ASSERT( rb.execute() )

    MatrixWorkspace_const_sptr inputWS = boost::dynamic_pointer_cast<MatrixWorkspace>(AnalysisDataService::Instance().retrieve("input2D"));
    MatrixWorkspace_const_sptr outputWS = boost::dynamic_pointer_cast<MatrixWorkspace>(AnalysisDataService::Instance().retrieve("output4"));
    TS_ASSERT_EQUALS( inputWS->readX(0), outputWS->readX(0) )
    TS_ASSERT_EQUALS( inputWS->readX(1), outputWS->readX(1) )
    TS_ASSERT_EQUALS( inputWS->readY(1), outputWS->readY(1) )
    TS_ASSERT_EQUALS( inputWS->readE(1), outputWS->readE(1) )
    for (int i = 0; i < 4; ++i)
    {
      TS_ASSERT_EQUALS( outputWS->readY(0)[i], 0.0 )
      TS_ASSERT_EQUALS( outputWS->readE(0)[i], 0.0 )
    }

    AnalysisDataService::Instance().remove("output4");
  }


	void testRealData()
	{
	
	//This test does not compile on Windows64 as is does not support HDF4 files
#ifndef _WIN64
		Mantid::NeXus::LoadMuonNexus loader;
		loader.initialize();
		loader.setPropertyValue("Filename", "../../../../Test/AutoTestData/emu00006473.nxs");
		loader.setPropertyValue("OutputWorkspace", "EMU6473");
		loader.execute();
		
		//Test removing time bins from the front		
		alg2.initialize();
		TS_ASSERT( alg2.isInitialized() )
		
		alg2.setPropertyValue("InputWorkspace", "EMU6473");
		alg2.setPropertyValue("OutputWorkspace", "result1");
		alg2.setPropertyValue("XMin", "-0.255");
		alg2.setPropertyValue("XMax", "-0.158");
		
		try 
		{
			TS_ASSERT_EQUALS(alg2.execute(),true);
		}
		catch(std::runtime_error e)
		{
			TS_FAIL(e.what());
		}
		
		MatrixWorkspace_const_sptr outputWS = boost::dynamic_pointer_cast<MatrixWorkspace>(AnalysisDataService::Instance().retrieve("result1"));
		
		TS_ASSERT_EQUALS(outputWS->dataX(0).size(), 1994);
#endif /*_WIN64*/
	}


	void makeDummyWorkspace2D()
	{
		Workspace2D_sptr testWorkspace(new Workspace2D);

		testWorkspace->setTitle("input2D");
		testWorkspace->initialize(2,5,4);

		boost::shared_ptr<Mantid::MantidVec> X(new Mantid::MantidVec);
    boost::shared_ptr<Mantid::MantidVec> Y(new Mantid::MantidVec);

		for (int i =0; i < 4; ++i)
		{
			X->push_back(10*i);
			
			if (i == 2)
			{
				Y->push_back(2.0*i + 1);
			}
			else
			{
				Y->push_back(2.0*i);
			}
		}
		X->push_back(40);	// X is one bigger
		
		//0   10   20   30   40   X
		//   0     2     5     6       Y

		testWorkspace->setX(0, X);
		testWorkspace->setX(1, X);
		testWorkspace->setData(0, Y, Y);
		testWorkspace->setData(1, Y, Y);

		testWorkspace->getAxis(0)->unit() = Mantid::Kernel::UnitFactory::Instance().create("TOF");

		AnalysisDataService::Instance().add("input2D", testWorkspace);		
	}

private:
	RemoveBins alg;
	RemoveBins alg2;
	RemoveBins alg3;
	RemoveBins alg4;


};

#endif /*RemoveBinsTest_H_*/
