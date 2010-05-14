#ifndef REBUNCHTEST_H_
#define REBUNCHTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidDataObjects/Workspace1D.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAlgorithms/Rebunch.h"
#include "MantidAPI/WorkspaceProperty.h"

using namespace Mantid::Kernel;
using namespace Mantid::DataObjects;
using namespace Mantid::API;
using namespace Mantid::Algorithms;


class RebunchTest : public CxxTest::TestSuite
{
public:
  void testworkspace1D_pnt_flush()
  {
    Workspace1D_sptr test_in1D = Create1DWorkspacePnt(50);
    AnalysisDataService::Instance().add("test_in1D", test_in1D);

    Rebunch rebunch;
    rebunch.initialize();
    rebunch.setPropertyValue("InputWorkspace","test_in1D");
    rebunch.setPropertyValue("OutputWorkspace","test_out");
    rebunch.setPropertyValue("NBunch", "5");
    rebunch.execute();
    MatrixWorkspace_sptr rebunchdata = boost::dynamic_pointer_cast<MatrixWorkspace>(AnalysisDataService::Instance().retrieve("test_out"));
    const Mantid::MantidVec outX=rebunchdata->dataX(0);
    const Mantid::MantidVec outY=rebunchdata->dataY(0);
    const Mantid::MantidVec outE=rebunchdata->dataE(0);

    TS_ASSERT_DELTA(outX[0],1.5  ,0.000001);
    TS_ASSERT_DELTA(outY[0],3.0 ,0.000001);
    TS_ASSERT_DELTA(outE[0],sqrt(15.0)/5.0  ,0.000001);
    TS_ASSERT_DELTA(outX[4],11.5  ,0.000001);
    TS_ASSERT_DELTA(outY[4],23.0 ,0.000001);
    TS_ASSERT_DELTA(outE[4],sqrt(115.0)/5.0  ,0.000001);
    TS_ASSERT_DELTA(outX[9],24.0  ,0.000001);
    TS_ASSERT_DELTA(outY[9],48.0 ,0.000001);
    TS_ASSERT_DELTA(outE[9],sqrt(240.0)/5.0  ,0.000001);

    AnalysisDataService::Instance().remove("test_in1D");
    AnalysisDataService::Instance().remove("test_out");
  }

  void testworkspace1D_nondist()
  {
    Workspace1D_sptr test_in1D = Create1DWorkspaceHist(50);
    AnalysisDataService::Instance().add("test_in1D", test_in1D);

    Rebunch rebunch;
    rebunch.initialize();
    rebunch.setPropertyValue("InputWorkspace","test_in1D");
    rebunch.setPropertyValue("OutputWorkspace","test_out");
    rebunch.setPropertyValue("NBunch", "7");
    rebunch.execute();
    MatrixWorkspace_sptr rebunchdata = boost::dynamic_pointer_cast<MatrixWorkspace>(AnalysisDataService::Instance().retrieve("test_out"));

    const Mantid::MantidVec outX=rebunchdata->dataX(0);
    const Mantid::MantidVec outY=rebunchdata->dataY(0);
    const Mantid::MantidVec outE=rebunchdata->dataE(0);

    TS_ASSERT_DELTA(outX[0],0.5  ,0.000001);
    TS_ASSERT_DELTA(outY[0],28  ,0.000001);
    TS_ASSERT_DELTA(outE[0],sqrt(28.0)  ,0.000001);
    TS_ASSERT_DELTA(outX[4],21.5  ,0.000001);
    TS_ASSERT_DELTA(outY[4],224.0  ,0.000001);
    TS_ASSERT_DELTA(outE[4],sqrt(224.0)  ,0.000001);
    TS_ASSERT_DELTA(outX[6],32  ,0.000001);
    TS_ASSERT_DELTA(outY[6],322.0 ,0.000001);
    TS_ASSERT_DELTA(outE[6],sqrt(322.0)  ,0.000001);
	bool dist=rebunchdata->isDistribution();
    TS_ASSERT(!dist);

    AnalysisDataService::Instance().remove("test_in1D");
    AnalysisDataService::Instance().remove("test_out");
  }

  void testworkspace2D_dist()
  {
    Workspace2D_sptr test_in2D = Create2DWorkspaceHist(50,20);
    test_in2D->isDistribution(true);
    AnalysisDataService::Instance().add("test_in2D", test_in2D);

    Rebunch rebunch;
    rebunch.initialize();
    rebunch.setPropertyValue("InputWorkspace","test_in2D");
    rebunch.setPropertyValue("OutputWorkspace","test_out");
    rebunch.setPropertyValue("NBunch", "5");
    rebunch.execute();
    MatrixWorkspace_sptr rebunchdata = boost::dynamic_pointer_cast<MatrixWorkspace>(AnalysisDataService::Instance().retrieve("test_out"));

    const Mantid::MantidVec outX=rebunchdata->dataX(5);
    const Mantid::MantidVec outY=rebunchdata->dataY(5);
    const Mantid::MantidVec outE=rebunchdata->dataE(5);

	TS_ASSERT_DELTA(outX[0],0.5  ,0.000001);
    TS_ASSERT_DELTA(outY[0],3 ,0.000001);
	TS_ASSERT_DELTA(outE[0],sqrt(8.4375)/3.75  ,0.000001);
    TS_ASSERT_DELTA(outX[4],15.5  ,0.000001);
    TS_ASSERT_DELTA(outY[4],23,0.000001);
    TS_ASSERT_DELTA(outE[4], sqrt(64.6875)/3.75  ,0.000001);
    TS_ASSERT_DELTA(outX[9],34.25  ,0.000001);
    TS_ASSERT_DELTA(outY[9],47.5,0.000001);
    TS_ASSERT_DELTA(outE[9],sqrt(106.875)/3.0   ,0.000001);

	TS_ASSERT(rebunchdata->isDistribution());

    AnalysisDataService::Instance().remove("test_in2D");
    AnalysisDataService::Instance().remove("test_out");
  }

  void testworkspace2D_pnt_remainder()
  {
    Workspace2D_sptr test_in2D = Create2DWorkspacePnt(50,20);
    AnalysisDataService::Instance().add("test_in2D", test_in2D);

    Rebunch rebunch;
    rebunch.initialize();
    rebunch.setPropertyValue("InputWorkspace","test_in2D");
    rebunch.setPropertyValue("OutputWorkspace","test_out");
    rebunch.setPropertyValue("NBunch", "7");
    rebunch.execute();
    MatrixWorkspace_sptr rebunchdata = boost::dynamic_pointer_cast<MatrixWorkspace>(AnalysisDataService::Instance().retrieve("test_out"));
    const Mantid::MantidVec outX=rebunchdata->dataX(5);
    const Mantid::MantidVec outY=rebunchdata->dataY(5);
    const Mantid::MantidVec outE=rebunchdata->dataE(5);

    TS_ASSERT_DELTA(outX[0],2.75  ,0.000001);
    TS_ASSERT_DELTA(outY[0],5.5 ,0.000001);
    TS_ASSERT_DELTA(outE[0],sqrt(38.5)/7.0  ,0.000001);
    TS_ASSERT_DELTA(outX[2],13.25 ,0.000001);
    TS_ASSERT_DELTA(outY[2],26.5 ,0.000001);
    TS_ASSERT_DELTA(outE[2],sqrt(185.5)/7.0  ,0.000001);
    TS_ASSERT_DELTA(outX[7],37.25  ,0.000001);
    TS_ASSERT_DELTA(outY[7],74.5 ,0.000001);
    TS_ASSERT_DELTA(outE[7],sqrt(74.5)  ,0.000001);

    AnalysisDataService::Instance().remove("test_in2D");
    AnalysisDataService::Instance().remove("test_out");
  }

private:
  Workspace1D_sptr Create1DWorkspaceHist(int size)
  {
    Workspace1D_sptr retVal(new Workspace1D);
    retVal->initialize(1,size,size-1);
    double j=1.0;
    for (int i=0; i<size; i++)
    {
      retVal->dataX()[i]=j*0.5;
      j+=1.5;
    }
    j=1.0;
    for (int i=0; i<size-1; i++)
    {
      retVal->dataY()[i]=j;
      retVal->dataE()[i]=sqrt(j);
      j+=1;
    }

    return retVal;
  }
  
  Workspace1D_sptr Create1DWorkspacePnt(int size)
  {
    Workspace1D_sptr retVal(new Workspace1D);
    retVal->initialize(1,size,size);
    double j=1.0;
    for (int i=0; i<size; i++)
    {
      retVal->dataX()[i]=j*0.5;
      retVal->dataY()[i]=j;
      retVal->dataE()[i]=sqrt(j);
      j+=1.0;
    }

    return retVal;
  }
   
  Workspace2D_sptr Create2DWorkspaceHist(int xlen, int ylen)
  {
    boost::shared_ptr<Mantid::MantidVec> x1(new Mantid::MantidVec(xlen,0.0));
    boost::shared_ptr<Mantid::MantidVec> y1(new Mantid::MantidVec(xlen-1,0.0));
    boost::shared_ptr<Mantid::MantidVec> e1(new Mantid::MantidVec(xlen-1,0.0));
    boost::shared_ptr<Mantid::MantidVec> e2(new Mantid::MantidVec(xlen-1,0.0));

    Workspace2D_sptr retVal(new Workspace2D);
    retVal->initialize(ylen,xlen,xlen-1);
    double j=1.0;

    for (int i=0; i<xlen; i++)
    {
      (*x1)[i]=j*0.5;
      j+=1.5;
    }
    j=1.0;
    for (int i=0; i<xlen-1; i++)
    {
      (*y1)[i]=j;
      (*e1)[i]=sqrt(j);
      j+=1;
    }

    for (int i=0; i< ylen; i++)
    {
      retVal->setX(i,x1);
      retVal->setData(i,y1,e1);

    }

    return retVal;
  }
  
  Workspace2D_sptr Create2DWorkspacePnt(int xlen, int ylen)
  {
    boost::shared_ptr<Mantid::MantidVec> x1(new Mantid::MantidVec(xlen,0.0));
    boost::shared_ptr<Mantid::MantidVec> y1(new Mantid::MantidVec(xlen,0.0));
    boost::shared_ptr<Mantid::MantidVec> e1(new Mantid::MantidVec(xlen,0.0));
    boost::shared_ptr<Mantid::MantidVec> e2(new Mantid::MantidVec(xlen,0.0));

    Workspace2D_sptr retVal(new Workspace2D);
    retVal->initialize(ylen,xlen,xlen);
    double j=1.0;

    for (int i=0; i<xlen; i++)
    {
      (*x1)[i]=j*0.5;
      (*y1)[i]=j;
      (*e1)[i]=sqrt(j);
      j+=1.5;
    }

    for (int i=0; i< ylen; i++)
    {
      retVal->setX(i,x1);
      retVal->setData(i,y1,e1);

    }

    return retVal;
  }
};
#endif /* REBUNCHTEST */
