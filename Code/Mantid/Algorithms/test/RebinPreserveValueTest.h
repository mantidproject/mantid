#ifndef REBINPRESERVEVALUETEST_H_
#define REBINPRESERVEVALUETEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidDataObjects/Workspace1D.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAlgorithms/RebinPreserveValue.h"
#include "MantidAPI/WorkspaceProperty.h"

using namespace Mantid::Kernel;
using namespace Mantid::DataObjects;
using namespace Mantid::API;
using namespace Mantid::Algorithms;


class RebinPreserveValueTest : public CxxTest::TestSuite
{
public:
  void testIt()
  {
    Workspace2D_sptr inWS(new Workspace2D());
    int n = 10;
    int m = 1;
    inWS->initialize(n,m+1,m);
    for(int i=0;i<n;i++)
    {
        std::vector<double>& X = inWS->dataX(i);
        std::vector<double>& Y = inWS->dataY(i);
        X[0] = double(i) + .5;
        X[1] = double(i) + 4.;
        Y[0] = 10.*(i+1);
    }

    AnalysisDataService::Instance().add("input",inWS);
    RebinPreserveValue alg;
    TS_ASSERT_THROWS_NOTHING( alg.initialize() )
    TS_ASSERT( alg.isInitialized() )
    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("InputWorkspace","input") )
    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("OutputWorkspace","output") )
    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("params","0,2,13") )
    TS_ASSERT_THROWS_NOTHING( alg.execute() )
    TS_ASSERT( alg.isExecuted() )

    Workspace_sptr outWS = AnalysisDataService::Instance().retrieve("output");
    Workspace2D_sptr WS = boost::dynamic_pointer_cast<Workspace2D>(outWS);

    TS_ASSERT_EQUALS(WS->getNumberHistograms(),10)
    TS_ASSERT_EQUALS(WS->blocksize(),7)


    std::vector<double>& Y0 = WS->dataY(0);
    TS_ASSERT_EQUALS(Y0[0],7.5)
    TS_ASSERT_EQUALS(Y0[1],10)
    TS_ASSERT_EQUALS(Y0[2],0)

    std::vector<double>& Y1 = WS->dataY(1);
    TS_ASSERT_EQUALS(Y1[0],5)
    TS_ASSERT_EQUALS(Y1[1],20)
    TS_ASSERT_EQUALS(Y1[2],10)

    std::vector<double>& Y2 = WS->dataY(2);
    TS_ASSERT_EQUALS(Y2[0],0)
    TS_ASSERT_EQUALS(Y2[1],22.5)
    TS_ASSERT_EQUALS(Y2[2],30)


  }

};
#endif /* REBINPRESERVEVALUETEST_H_ */
