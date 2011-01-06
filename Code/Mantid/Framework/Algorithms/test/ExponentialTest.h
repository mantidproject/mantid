#ifndef EXPONENTAILTEST_H_
#define EXPONENTIALTEST_H_

#include <cxxtest/TestSuite.h>
#include <cmath>

#include "MantidTestHelpers/WorkspaceCreationHelper.h"
#include "MantidAlgorithms/Exponential.h"
#include "MantidAPI/AnalysisDataService.h"

using namespace Mantid::API;
using namespace Mantid::Kernel;
using namespace Mantid::Algorithms;
using namespace Mantid::DataObjects;

class ExponentialTest : public CxxTest::TestSuite
{
public:

  void testInit()
  {
    Exponential alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    TS_ASSERT(alg.isInitialized())
    //Setting properties to input workspaces that don't exist throws
    TS_ASSERT_THROWS( alg.setPropertyValue("InputWorkspace","test_in21"), std::invalid_argument )
    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("OutputWorkspace","test_out2") )
  }

  void testExec1D()
  {
    int sizex = 10;
    // Register the workspace in the data service
    MatrixWorkspace_sptr work_in1 = WorkspaceCreationHelper::Create1DWorkspaceFib(sizex);

    AnalysisDataService::Instance().add("test_in11", work_in1);
    setError(work_in1);

    Exponential alg;

    alg.initialize();
    TS_ASSERT_THROWS_NOTHING(
      alg.setPropertyValue("InputWorkspace","test_in11");
      alg.setPropertyValue("OutputWorkspace","test_out1");
    )
    alg.execute();

    MatrixWorkspace_sptr work_out1;
    TS_ASSERT_THROWS_NOTHING(work_out1 = boost::dynamic_pointer_cast<MatrixWorkspace>(AnalysisDataService::Instance().retrieve("test_out1")));

    checkData(work_in1, work_out1);

    AnalysisDataService::Instance().remove("test_out1");
    AnalysisDataService::Instance().remove("test_in11");

  }

private:


  // loopOrientation 0=Horizontal, 1=Vertical
  void checkData( MatrixWorkspace_sptr work_in1, MatrixWorkspace_sptr work_out1)
  {

    for (int i = 0; i < work_out1->size(); i++)
    {
      double sig1 = work_in1->dataY(i/work_in1->blocksize())[i%work_in1->blocksize()];
      double sig3 = work_out1->dataY(i/work_in1->blocksize())[i%work_in1->blocksize()];
      TS_ASSERT_DELTA(work_in1->dataX(i/work_in1->blocksize())[i%work_in1->blocksize()],
        work_out1->dataX(i/work_in1->blocksize())[i%work_in1->blocksize()], 1.e-10);
      double expsig3 = exp(sig1);
      TS_ASSERT_DELTA(expsig3, sig3, 1e-10*sig3);
      double err1 = work_in1->dataE(i/work_in1->blocksize())[i%work_in1->blocksize()];
      double err3 = err1 * expsig3;
      TS_ASSERT_DELTA(err3, work_out1->dataE(i/work_in1->blocksize())[i%work_in1->blocksize()], 0.0001);
    }
  }
  // loopOrientation 0=Horizontal, 1=Vertical
  void setError( MatrixWorkspace_sptr work_in1)
  {

    for (int i = 0; i < work_in1->size(); i++)
    {
      double sig1 = work_in1->dataY(i/work_in1->blocksize())[i%work_in1->blocksize()];
      work_in1->dataE(i/work_in1->blocksize())[i%work_in1->blocksize()]=sqrt(sig1);
    }
  }



};

#endif /*EXPONENTIALTEST_H_*/
