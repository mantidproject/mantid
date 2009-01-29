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
    int m = 3;
    inWS->initialize(n,m+1,m);
    for(int i=0;i<n;i++)
    {
        std::vector<double>& X = inWS->dataX(i);
        std::vector<double>& Y = inWS->dataY(i);
        for(int j=0;j<m;j++)
        {
            X[j] = double(i+j) + .5;
            X[j+1] = double(i+j) + 2.;
            Y[j] = 10.*(i+1+j);
            //std::cerr<<'('<<X[j]<<' '<<X[j+1]<<' '<<Y[j]<<") ";
        }
        //std::cerr<<'\n';
    }

    AnalysisDataService::Instance().add("input",inWS);
    RebinPreserveValue alg;
    alg.initialize();
    alg.setPropertyValue("InputWorkspace","input");
    alg.setPropertyValue("OutputWorkspace","output");
    alg.setPropertyValue("params","0,2,13");
    alg.execute();

    Workspace_sptr outWS = AnalysisDataService::Instance().retrieve("output");
    Workspace2D_sptr WS = boost::dynamic_pointer_cast<Workspace2D>(outWS);

    TS_ASSERT_EQUALS(WS->getNumberHistograms(),10)
    TS_ASSERT_EQUALS(WS->blocksize(),7)


    std::vector<double>& Y0 = WS->dataY(0);
    TS_ASSERT_EQUALS(Y0[0],10)
    TS_ASSERT_EQUALS(Y0[1],20)
    TS_ASSERT_EQUALS(Y0[2],0)

    std::vector<double>& Y1 = WS->dataY(1);
    TS_ASSERT_EQUALS(Y1[0],20)
    TS_ASSERT_EQUALS(Y1[1],20)
    TS_ASSERT_EQUALS(Y1[2],40)

    std::vector<double>& Y2 = WS->dataY(2);
    TS_ASSERT_EQUALS(Y2[0],0)
    TS_ASSERT_EQUALS(Y2[1],30)
    TS_ASSERT_EQUALS(Y2[2],40)


    for(int i=0;i<WS->getNumberHistograms();i++)
    {
        std::vector<double>& X = WS->dataX(i);
        std::vector<double>& Y = WS->dataY(i);
        for(int j=0;j<WS->blocksize();j++)
        {
//            std::cerr<<'('<<X[j]<<','<<Y[j]<<") ";
        }
    }
  }

};
#endif /* REBINPRESERVEVALUETEST_H_ */
