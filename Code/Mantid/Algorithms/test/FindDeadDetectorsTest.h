#ifndef FINDDEADDETECTORSTEST_H_
#define FINDDEADDETECTORSTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAlgorithms/FindDeadDetectors.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidDataObjects/Workspace2D.h"
#include "WorkspaceCreationHelper.hh"

using namespace Mantid::API;
using namespace Mantid::Kernel;
using namespace Mantid::Algorithms;
using namespace Mantid::DataObjects;

class FindDeadDetectorsTest : public CxxTest::TestSuite
{
public:

  FindDeadDetectorsTest()
  {
  }

  ~FindDeadDetectorsTest()
  {}

  void testInit()
  {
    FindDeadDetectors alg;

    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    TS_ASSERT( alg.isInitialized() );

    // Set the properties
    alg.setPropertyValue("InputWorkspace","testSpace");
    std::string outputSpace = "IntegrationOuter";
    alg.setPropertyValue("OutputWorkspace",outputSpace);

    alg.setPropertyValue("DeadThreshold","1");
    alg.setPropertyValue("LiveValue","3");
    alg.setPropertyValue("DeadValue","2");
  }

  void testExec()
  {
    int sizex = 10,sizey=20;
    // Register the workspace in the data service
    Workspace2D_sptr work_in = WorkspaceCreationHelper::Create2DWorkspace154(sizex,sizey,1);
    //set some dead detectors
    std::vector<double> yDead(sizex,0);
    for (int i=0; i< sizey; i++)
    {
      if (i%2==0)
      {
        work_in->setData(i,yDead,yDead);
      }
    }

    FindDeadDetectors alg;

    AnalysisDataService::Instance().add("testdead_in", work_in);
    alg.initialize();
    alg.setPropertyValue("InputWorkspace","testdead_in");
    alg.setPropertyValue("OutputWorkspace","testdead_out");
    alg.setPropertyValue("DeadThreshold","0");
    alg.setPropertyValue("LiveValue","1");
    alg.setPropertyValue("DeadValue","2");
    TS_ASSERT_THROWS_NOTHING(alg.execute());
    TS_ASSERT( alg.isExecuted() );
    Workspace_sptr work_out;
    TS_ASSERT_THROWS_NOTHING(work_out = AnalysisDataService::Instance().retrieve("testdead_out"));

    for (int i=0; i< sizey; i++)
    {
      const double val = work_out->readY(i)[0];
      double valExpected = 1;
      if (i%2==0)
      {
          valExpected = 2;
      }
      TS_ASSERT_DELTA(val,valExpected,1e-9);
    }

    AnalysisDataService::Instance().remove("testdead_in");
    AnalysisDataService::Instance().remove("testdead_out");

  }

};

#endif /*FINDDEADDETECTORSTEST_H_*/
