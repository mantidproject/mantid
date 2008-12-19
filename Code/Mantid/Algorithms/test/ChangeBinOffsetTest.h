#ifndef CHANGEBINOFFSETTEST_H_
#define CHANGEBINOFFSETTEST_H_

#include <cxxtest/TestSuite.h>

#include <iostream>
#include <sstream>
#include <string>

#include "MantidDataObjects/Workspace1D.h"
#include "MantidDataObjects/Workspace2D.h"
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
		TS_ASSERT( alg1D.isInitialized() )

		alg1D.setPropertyValue("InputWorkspace", "input1D");
		alg1D.setPropertyValue("OutputWorkspace", "output1D");
		alg1D.setPropertyValue("Offset", offsetStr.str());

		TS_ASSERT_THROWS_NOTHING(alg1D.execute());
		TS_ASSERT( alg1D.isExecuted() )

		Workspace_sptr output = AnalysisDataService::Instance(). retrieve(alg1D.getProperty("OutputWorkspace"));

		std::vector<double>& Xold = input->dataX(0);
		std::vector<double>& Xnew = output->dataX(0);

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
		TS_ASSERT( alg2D.isInitialized() )

		alg2D.setPropertyValue("InputWorkspace", "input2D");
		alg2D.setPropertyValue("OutputWorkspace", "output2D");
		alg2D.setPropertyValue("Offset", offsetStr.str());

		TS_ASSERT_THROWS_NOTHING(alg2D.execute());
		TS_ASSERT( alg2D.isExecuted() )

		Workspace_sptr output = AnalysisDataService::Instance().retrieve(alg2D.getProperty("OutputWorkspace"));

		std::vector<double>& Xold = input->dataX(0);
		std::vector<double>& Xnew = output->dataX(0);

		for (int i=0; i < Xnew.size(); ++i)
		{
		    std::cout << "old value: " << Xold[i] << std::endl;
		    std::cout << "new value: " << Xnew[i] << std::endl;
		}

		TS_ASSERT(Xold[0] + offset == Xnew[0]);
		TS_ASSERT(Xold[1] + offset == Xnew[1]);

		AnalysisDataService::Instance().remove("input2D");
	}

	Workspace1D_sptr makeDummyWorkspace1D()
	{
		Workspace1D_sptr testWorkspace(new Workspace1D);

		testWorkspace->setTitle("input1D");
		testWorkspace->initialize(1,2,2);

		std::vector<double> X;
		std::vector<double> Y;

		for (int i =0; i < 2; ++i)
		{
			X.push_back(1.0*i);
			Y.push_back(2.0*i);
		}

		testWorkspace->setX(X);
		testWorkspace->setData(Y);

		return testWorkspace;
	}

	Workspace2D_sptr makeDummyWorkspace2D()
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

		return testWorkspace;
	}

private:


};

#endif /*CHANGEBINOFFSETTEST_H_*/
