#ifndef REGROUPTEST_H_
#define REGROUPTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidDataObjects/Workspace1D.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAlgorithms/Regroup.h"
#include "MantidAPI/WorkspaceProperty.h"

#include <iostream>

using namespace Mantid::Kernel;
using namespace Mantid::DataObjects;
using namespace Mantid::API;
using namespace Mantid::Algorithms;


class RegroupTest : public CxxTest::TestSuite
{
public:
  void testworkspace1D_dist()
  {
    Workspace1D_sptr test_in1D = Create1DWorkspace(50);
    test_in1D->isDistribution(true);
    AnalysisDataService::Instance().add("test_in1D", test_in1D);

    Regroup regroup;
    regroup.initialize();
    regroup.setChild(true);
    regroup.setPropertyValue("InputWorkspace","test_in1D");
    regroup.setPropertyValue("OutputWorkspace","test_out");
    // Check it fails if "params" property not set
    TS_ASSERT_THROWS( regroup.execute(), std::runtime_error )
    TS_ASSERT( ! regroup.isExecuted() )
    // Trying to set the property with an error fails
    TS_ASSERT_THROWS(regroup.setPropertyValue("Params", "1.5,2.0,20,-0.1,15,1.0,35"), std::invalid_argument)
	// Now set the property
    TS_ASSERT_THROWS_NOTHING( regroup.setPropertyValue("Params", "1.5,1,19,-0.1,30,1,35") )

    TS_ASSERT(regroup.execute())
    TS_ASSERT( regroup.isExecuted())

    MatrixWorkspace_sptr rebindata = regroup.getProperty("OutputWorkspace");
    const Mantid::MantidVec outX=rebindata->dataX(0);

    TS_ASSERT_DELTA(outX[7],12.5  ,0.000001);
    TS_ASSERT_DELTA(outX[12],20.75  ,0.000001);


    /*const std::vector<double> outY=rebindata->dataY(0);
    const std::vector<double> outE=rebindata->dataE(0);

    TS_ASSERT_DELTA(outX[7],15.5  ,0.000001);
    TS_ASSERT_DELTA(outY[7],3.0 ,0.000001);
    TS_ASSERT_DELTA(outE[7], sqrt(4.5)/2.0  ,0.000001);

    TS_ASSERT_DELTA(outX[12],24.2 ,0.000001);
    TS_ASSERT_DELTA(outY[12],3.0 ,0.000001);
    TS_ASSERT_DELTA(outE[12],sqrt(5.445)/2.42 ,0.000001);

    TS_ASSERT_DELTA(outX[17],32.0  ,0.000001);
    TS_ASSERT_DELTA(outY[17],3.0 ,0.000001);
    TS_ASSERT_DELTA(outE[17],sqrt(2.25) ,0.000001);
    bool dist=rebindata->isDistribution();
    TS_ASSERT(dist);
    //*/
    AnalysisDataService::Instance().remove("test_in1D");
    AnalysisDataService::Instance().remove("test_out");
  }

private:
  Workspace1D_sptr Create1DWorkspace(int size)
  {
    boost::shared_ptr<Mantid::MantidVec> y1(new Mantid::MantidVec(size-1,3.0));
    boost::shared_ptr<Mantid::MantidVec> e1(new Mantid::MantidVec(size-1,sqrt(3.0)));
    Workspace1D_sptr retVal(new Workspace1D);
    retVal->initialize(1,size,size-1);
    double j=1.0;
    for (int i=0; i<size; i++)
    {
      retVal->dataX()[i]=j*0.5;
      j+=1.5;
    }
    retVal->setData(y1,e1);
    return retVal;
  }
  
  Workspace2D_sptr Create2DWorkspace(int xlen, int ylen)
  {
    boost::shared_ptr<Mantid::MantidVec> x1(new Mantid::MantidVec(xlen,0.0));
    boost::shared_ptr<Mantid::MantidVec> y1(new Mantid::MantidVec(xlen-1,3.0));
    boost::shared_ptr<Mantid::MantidVec> e1(new Mantid::MantidVec(xlen-1,sqrt(3.0)));

    Workspace2D_sptr retVal(new Workspace2D);
    retVal->initialize(ylen,xlen,xlen-1);
    double j=1.0;

    for (int i=0; i<xlen; i++)
    {
      (*x1)[i]=j*0.5;
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
#endif /* REGROUPTEST */
