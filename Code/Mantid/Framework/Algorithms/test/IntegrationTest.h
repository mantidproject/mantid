#ifndef INTEGRATIONTEST_H_
#define INTEGRATIONTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAlgorithms/Integration.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"

using namespace Mantid::API;
using namespace Mantid::Kernel;
using namespace Mantid::Algorithms;
using namespace Mantid::DataObjects;

class IntegrationTest : public CxxTest::TestSuite
{
public:

  IntegrationTest()
  {
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
    // Register the workspace in the data service
    AnalysisDataService::Instance().add("testSpace", space);

  }

  ~IntegrationTest()
  {}

  void testInit()
  {
    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    TS_ASSERT( alg.isInitialized() );

    // Set the properties
    alg.setPropertyValue("InputWorkspace","testSpace");
    outputSpace = "IntegrationOuter";
    alg.setPropertyValue("OutputWorkspace",outputSpace);

    alg.setPropertyValue("RangeLower","0.1");
    alg.setPropertyValue("RangeUpper","4.0");
    alg.setPropertyValue("StartWorkspaceIndex","2");
    alg.setPropertyValue("EndWorkspaceIndex","4");

    TS_ASSERT_THROWS_NOTHING( alg2.initialize());
    TS_ASSERT( alg.isInitialized() );

    // Set the properties
    alg2.setPropertyValue("InputWorkspace","testSpace");
    alg2.setPropertyValue("OutputWorkspace","out2");

    TS_ASSERT_THROWS_NOTHING( alg3.initialize());
    TS_ASSERT( alg3.isInitialized() );

    // Set the properties
    alg3.setPropertyValue("InputWorkspace","testSpace");
    alg3.setPropertyValue("OutputWorkspace","out3");
    alg3.setPropertyValue("RangeLower","0.1");
    alg3.setPropertyValue("RangeUpper","4.5");
    alg3.setPropertyValue("StartWorkspaceIndex","2");
    alg3.setPropertyValue("EndWorkspaceIndex","4");
    alg3.setPropertyValue("IncludePartialBins","1");
  }

  void testRangeNoPartialBins()
  {
    if ( !alg.isInitialized() ) alg.initialize();
    TS_ASSERT_THROWS_NOTHING( alg.execute());
    TS_ASSERT( alg.isExecuted() );

    // Get back the saved workspace
    Workspace_sptr output;
    TS_ASSERT_THROWS_NOTHING(output = AnalysisDataService::Instance().retrieve(outputSpace));

    Workspace2D_sptr output2D = boost::dynamic_pointer_cast<Workspace2D>(output);
    int max;
    TS_ASSERT_EQUALS( max = output2D->getNumberHistograms(), 3);
    double yy[3] = {36,51,66};
    for (int i = 0; i < max; ++i)
    {
      Mantid::MantidVec &x = output2D->dataX(i);
      Mantid::MantidVec &y = output2D->dataY(i);
      Mantid::MantidVec &e = output2D->dataE(i);

      TS_ASSERT_EQUALS( x.size(), 2 );
      TS_ASSERT_EQUALS( y.size(), 1 );
      TS_ASSERT_EQUALS( e.size(), 1 );

      TS_ASSERT_EQUALS( x[0], 1.0 );
      TS_ASSERT_EQUALS( x[1], 4.0 );
      TS_ASSERT_EQUALS( y[0], yy[i] );
      TS_ASSERT_DELTA( e[0], sqrt(yy[i]), 0.001 );
    }
  }

  void testNoRangeNoPartialBins()
  {
    if ( !alg2.isInitialized() ) alg2.initialize();

    // Check setting of invalid property value causes failure
    TS_ASSERT_THROWS( alg2.setPropertyValue("StartWorkspaceIndex","-1"), std::invalid_argument) ;

    TS_ASSERT_THROWS_NOTHING( alg2.execute());
    TS_ASSERT( alg2.isExecuted() );

    // Get back the saved workspace
    Workspace_sptr output;
    TS_ASSERT_THROWS_NOTHING(output = AnalysisDataService::Instance().retrieve("out2"));

    Workspace2D_sptr output2D = boost::dynamic_pointer_cast<Workspace2D>(output);
    TS_ASSERT_EQUALS( output2D->getNumberHistograms(), 5);
    TS_ASSERT_EQUALS( output2D->dataX(0)[0], 0 );
    TS_ASSERT_EQUALS( output2D->dataX(0)[1], 5 );
    TS_ASSERT_EQUALS( output2D->dataY(0)[0], 10 );
    TS_ASSERT_EQUALS( output2D->dataY(4)[0], 110 );
    TS_ASSERT_DELTA ( output2D->dataE(2)[0], 7.746, 0.001 );
  }

  void testRangeWithPartialBins()
  {
    if ( !alg3.isInitialized() ) alg3.initialize();
    TS_ASSERT_THROWS_NOTHING( alg3.execute());
    TS_ASSERT( alg3.isExecuted() );

    // Get back the saved workspace
    Workspace_sptr output;
    TS_ASSERT_THROWS_NOTHING(output = AnalysisDataService::Instance().retrieve("out3"));

    Workspace2D_sptr output2D = boost::dynamic_pointer_cast<Workspace2D>(output);
    int max;
    TS_ASSERT_EQUALS( max = output2D->getNumberHistograms(), 3);
    const double yy[3] = {52.,74.,96.};
    const double ee[3] = {6.899,8.240,9.391};
    for (int i = 0; i < max; ++i)
    {
      Mantid::MantidVec &x = output2D->dataX(i);
      Mantid::MantidVec &y = output2D->dataY(i);
      Mantid::MantidVec &e = output2D->dataE(i);

      TS_ASSERT_EQUALS( x.size(), 2 );
      TS_ASSERT_EQUALS( y.size(), 1 );
      TS_ASSERT_EQUALS( e.size(), 1 );

      TS_ASSERT_EQUALS( x[0], 0.1 );
      TS_ASSERT_EQUALS( x[1], 4.5 );
      TS_ASSERT_EQUALS( y[0], yy[i] );
      TS_ASSERT_DELTA( e[0], ee[i], 0.001 );
    }

    //Test that the same values occur for a distribution
    Workspace_sptr input;
    TS_ASSERT_THROWS_NOTHING(input = AnalysisDataService::Instance().retrieve("testSpace"));
    Workspace2D_sptr input2D = boost::dynamic_pointer_cast<Workspace2D>(output);
    input2D->isDistribution(true);
    //Replace workspace
    AnalysisDataService::Instance().addOrReplace("testSpace", input2D);
    
    alg3.execute();
    //Retest
    TS_ASSERT_THROWS_NOTHING(output = AnalysisDataService::Instance().retrieve("out3"));

    output2D = boost::dynamic_pointer_cast<Workspace2D>(output);
    TS_ASSERT_EQUALS( max = output2D->getNumberHistograms(), 3);
    for (int i = 0; i < max; ++i)
    {
      Mantid::MantidVec &x = output2D->dataX(i);
      Mantid::MantidVec &y = output2D->dataY(i);
      Mantid::MantidVec &e = output2D->dataE(i);

      TS_ASSERT_EQUALS( x.size(), 2 );
      TS_ASSERT_EQUALS( y.size(), 1 );
      TS_ASSERT_EQUALS( e.size(), 1 );

      TS_ASSERT_EQUALS( x[0], 0.1 );
      TS_ASSERT_EQUALS( x[1], 4.5 );
      TS_ASSERT_EQUALS( y[0], yy[i] );
      TS_ASSERT_DELTA( e[0], ee[i], 0.001 );
    }
  }


  void doTestEvent(std::string inName, std::string outName, int StartWorkspaceIndex, int EndWorkspaceIndex)
  {
    int numPixels = 100;
    int numBins = 50;
    EventWorkspace_sptr inWS = WorkspaceCreationHelper::CreateEventWorkspace(numPixels,numBins,numBins,0.0, 1.0, 2);
    AnalysisDataService::Instance().addOrReplace(inName, inWS);

    Integration integ;
    integ.initialize();
    integ.setPropertyValue("InputWorkspace",inName);
    integ.setPropertyValue("OutputWorkspace",outName);
    integ.setPropertyValue("RangeLower","9.9");
    integ.setPropertyValue("RangeUpper","20.1");
    integ.setProperty("StartWorkspaceIndex",StartWorkspaceIndex);
    integ.setProperty("EndWorkspaceIndex",EndWorkspaceIndex);

    integ.execute();
    TS_ASSERT( integ.isExecuted() );

    EventWorkspace_sptr output;
    TS_ASSERT_THROWS_NOTHING(output = boost::dynamic_pointer_cast<EventWorkspace>( AnalysisDataService::Instance().retrieve(outName) ) );
    //Yep, it is an event
    TS_ASSERT( output );
    if (!output) return;

    TS_ASSERT_EQUALS( output->getNumberHistograms(), EndWorkspaceIndex -StartWorkspaceIndex+1 );

    for (int i=0; i< output->getNumberHistograms(); i++)
    {
      MantidVec X = output->readX(i);
      MantidVec Y = output->readY(i);
      MantidVec E = output->readE(i);
      TS_ASSERT_EQUALS( X.size(), 2);
      TS_ASSERT_EQUALS( Y.size(), 1);
      TS_ASSERT_DELTA( Y[0], 20.0, 1e-6);
      TS_ASSERT_DELTA( E[0], sqrt(20.0), 1e-6);
    }

    AnalysisDataService::Instance().remove(inName);
    AnalysisDataService::Instance().remove(outName);
  }

  void testEvent_AllHistograms()
  {
    doTestEvent("inWS", "outWS", 0, 99);
  }

  void testEvent_SomeHistograms()
  {
    doTestEvent("inWS", "outWS", 10, 39);
  }

  void testEvent_InPlace_AllHistograms()
  {
    doTestEvent("inWS", "inWS", 0, 99);
  }

  void testEvent_InPlace_SomeHistograms()
  {
    doTestEvent("inWS", "inWS", 10, 29);
  }


private:
  Integration alg;   // Test with range limits
  Integration alg2;  // Test without limits
  Integration alg3; // Test with range and partial bins
  std::string outputSpace;
};

#endif /*INTEGRATIONTEST_H_*/
