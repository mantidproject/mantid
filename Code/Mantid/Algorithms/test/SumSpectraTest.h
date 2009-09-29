#ifndef SUMSPECTRATEST_H_
#define SUMSPECTRATEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAlgorithms/SumSpectra.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidDataObjects/Workspace2D.h"

using namespace Mantid::API;
using namespace Mantid::Kernel;
using namespace Mantid::Algorithms;
using namespace Mantid::DataObjects;

class SumSpectraTest : public CxxTest::TestSuite
{
public:

  SumSpectraTest()
  {
		outputSpace1 = "SumSpectraOut1";
	  outputSpace2 = "SumSpectraOut2";
	  inputSpace = "SumSpectraIn";

    // Set up a small workspace for testing
    Workspace_sptr space = WorkspaceFactory::Instance().create("Workspace2D",5,6,5);
    Workspace2D_sptr space2D = boost::dynamic_pointer_cast<Workspace2D>(space);
    double *a = new double[25];
    double *e = new double[25];
    for (int i = 0; i < 25; ++i)
    {
      a[i]=i;
      e[i]=sqrt(double(i));
    }
    for (int j = 0; j < 5; ++j) {
      for (int k = 0; k < 6; ++k) {
        space2D->dataX(j)[k] = k;
      }
      space2D->setData(j, boost::shared_ptr<Mantid::MantidVec>(new Mantid::MantidVec(a+(5*j), a+(5*j)+5)),
          boost::shared_ptr<Mantid::MantidVec>(new Mantid::MantidVec(e+(5*j), e+(5*j)+5)));
    }
    delete[] a;
    delete[] e;
    // Register the workspace in the data service
    AnalysisDataService::Instance().add(inputSpace, space);

  }

  ~SumSpectraTest()
  {}

  void testInit()
  {
    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    TS_ASSERT( alg.isInitialized() );

    // Set the properties
    alg.setPropertyValue("InputWorkspace",inputSpace);
    alg.setPropertyValue("OutputWorkspace",outputSpace1);

    alg.setPropertyValue("StartWorkspaceIndex","2");
    alg.setPropertyValue("EndWorkspaceIndex","4");
  }


  void xtestExecWithLimits()
  {
    if ( !alg.isInitialized() ) alg.initialize();
    TS_ASSERT_THROWS_NOTHING( alg.execute());
    TS_ASSERT( alg.isExecuted() );

    // Get back the input workspace
    Workspace_sptr input;
    TS_ASSERT_THROWS_NOTHING(input = AnalysisDataService::Instance().retrieve(inputSpace));
    Workspace2D_sptr input2D = boost::dynamic_pointer_cast<Workspace2D>(input);

		// Get back the saved workspace
    Workspace_sptr output;
    TS_ASSERT_THROWS_NOTHING(output = AnalysisDataService::Instance().retrieve(outputSpace1));

    Workspace2D_sptr output2D = boost::dynamic_pointer_cast<Workspace2D>(output);
    int max;
    TS_ASSERT_EQUALS( max = input2D->blocksize(), output2D->blocksize())
    TS_ASSERT_EQUALS( output2D->getNumberHistograms(), 1)
    double yy[5] = {45,48,51,54,57};

    Mantid::MantidVec &x = output2D->dataX(0);
    Mantid::MantidVec &y = output2D->dataY(0);
    Mantid::MantidVec &e = output2D->dataE(0);
    TS_ASSERT_EQUALS( x.size(), 6 );
    TS_ASSERT_EQUALS( y.size(), 5 );
    TS_ASSERT_EQUALS( e.size(), 5 );

    for (int i = 0; i < max; ++i)
    {
			TS_ASSERT_EQUALS( x[i], i );
      TS_ASSERT_EQUALS( y[i], yy[i] );
      TS_ASSERT_DELTA( e[i], sqrt(yy[i]), 0.001 );
    }

	}
	
	void xtestExecWithoutLimits()
  {
		
		SumSpectra alg2;
		TS_ASSERT_THROWS_NOTHING( alg2.initialize());
    TS_ASSERT( alg2.isInitialized() );

    // Set the properties
    alg2.setPropertyValue("InputWorkspace",inputSpace);
    alg2.setPropertyValue("OutputWorkspace",outputSpace2);
    if ( !alg2.isInitialized() ) alg2.initialize();

    // Check setting of invalid property value causes failure
    TS_ASSERT_THROWS( alg2.setPropertyValue("StartWorkspaceIndex","-1"), std::invalid_argument) ;

    TS_ASSERT_THROWS_NOTHING( alg2.execute());
    TS_ASSERT( alg2.isExecuted() );

    // Get back the saved workspace
    Workspace_sptr output;
    TS_ASSERT_THROWS_NOTHING(output = AnalysisDataService::Instance().retrieve(outputSpace2));
		Workspace2D_sptr output2D = boost::dynamic_pointer_cast<Workspace2D>(output);

    int max = output2D->blocksize();
    TS_ASSERT_EQUALS( output2D->getNumberHistograms(), 1);
    double yy[5] = {50,55,60,65,70};

    Mantid::MantidVec &x = output2D->dataX(0);
    Mantid::MantidVec &y = output2D->dataY(0);
    Mantid::MantidVec &e = output2D->dataE(0);
    TS_ASSERT_EQUALS( x.size(), 6 );
    TS_ASSERT_EQUALS( y.size(), 5 );
    TS_ASSERT_EQUALS( e.size(), 5 );

    for (int i = 0; i < max; ++i)
    {
			TS_ASSERT_EQUALS( x[i], i );
      TS_ASSERT_EQUALS( y[i], yy[i] );
      TS_ASSERT_DELTA( e[i], sqrt(yy[i]), 0.001 );
    }
  }

private:
  SumSpectra alg;   // Test with range limits
  std::string outputSpace1;
	std::string outputSpace2;
	std::string inputSpace;
};

#endif /*SUMSPECTRATEST_H_*/
