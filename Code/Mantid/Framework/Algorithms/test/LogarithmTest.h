#ifndef MANTID_ALGORITHM_LOGTEST_H_
#define MANTID_ALGORITHM_LOGTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidTestHelpers/WorkspaceCreationHelper.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidDataObjects/Workspace1D.h"
#include "MantidAPI/WorkspaceProperty.h"
#include "MantidAPI/WorkspaceOpOverloads.h"

#include "MantidAlgorithms/Logarithm.h"

using namespace Mantid::Kernel;
using namespace Mantid::DataObjects;
using namespace Mantid::API;
using namespace Mantid::Algorithms;


class LogarithmTest : public CxxTest::TestSuite
{
public:
  void testInit(void)
  {
      Mantid::Algorithms::Logarithm alg;
      TS_ASSERT_THROWS_NOTHING(alg.initialize());
      TS_ASSERT(alg.isInitialized());

  }

  void testExec1D(void)
  {
      int sizex = 10;

        // Register the workspace in the data service
      MatrixWorkspace_sptr work_in1 = WorkspaceCreationHelper::Create1DWorkspaceFib(sizex);
      AnalysisDataService::Instance().add("test_inLn", work_in1);


      Logarithm alg;

      alg.initialize();

      TS_ASSERT_THROWS_NOTHING(
          alg.setPropertyValue("InputWorkspace","test_inLn");
          alg.setPropertyValue("OutputWorkspace","test_inLn");
          alg.setPropertyValue("Filler","10");
          alg.setPropertyValue("Natural","0");
       );

      alg.execute();

     MatrixWorkspace_sptr work_out1;
     TS_ASSERT_THROWS_NOTHING(work_out1 = boost::dynamic_pointer_cast<MatrixWorkspace>(AnalysisDataService::Instance().retrieve("test_inLn")));

     TS_ASSERT_THROWS_NOTHING(
      AnalysisDataService::Instance().remove("test_outLn");
      AnalysisDataService::Instance().remove("test_inLn");
      );

  }

  void testExec2D(void)
  {

      int sizex = 10,sizey=20;
        // Register the workspace in the data service
        MatrixWorkspace_sptr work_in2 = WorkspaceCreationHelper::Create2DWorkspace154(sizex,sizey);
        Workspace2D_sptr     work_ou2 = WorkspaceCreationHelper::Create2DWorkspace(sizex, sizey);


        Logarithm alg;

        AnalysisDataService::Instance().add("test_inLn2", work_in2);
        AnalysisDataService::Instance().add("test_outLn2", work_ou2);

        alg.initialize();
       TS_ASSERT_THROWS_NOTHING(
        alg.setPropertyValue("InputWorkspace","test_inLn2");
        alg.setPropertyValue("OutputWorkspace","test_outLn2");
        alg.setPropertyValue("Natural","1");

        );

        TS_ASSERT_THROWS_NOTHING(alg.execute());
        TS_ASSERT( alg.isExecuted() );

         MatrixWorkspace_sptr work_out2;
         TS_ASSERT_THROWS_NOTHING(work_out2 = boost::dynamic_pointer_cast<MatrixWorkspace>(AnalysisDataService::Instance().retrieve("test_outLn2")));

   //    checkData(work_in1, work_in2, work_out1);

        AnalysisDataService::Instance().remove("test_inLn2");
        AnalysisDataService::Instance().remove("test_outLn2");

  }

};

#endif           /* MANTID_ALGORITHM_LOGTEST_H_ */
