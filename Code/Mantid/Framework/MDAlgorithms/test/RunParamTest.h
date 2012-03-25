#ifndef RUNPARAMTEST_H_
#define RUNPARAMTEST_H_

// ONLY BARE BONES OF POSSIBLE TEST FILE, MAINLY STILL TOBYFITTEST

#include <cxxtest/TestSuite.h>
#include <cmath>
#include <iostream>
#include <boost/scoped_ptr.hpp> 
#include "MantidMDAlgorithms/RunParam.h"

using namespace Mantid::API;
using namespace Mantid::Kernel;
using namespace Mantid::Geometry;
using namespace Mantid::MDAlgorithms;



class RunParamTest : public CxxTest::TestSuite, public RunParam
{
private:
  
  boost::shared_ptr<Mantid::MDAlgorithms::RunParam> rParam;
  boost::shared_ptr<Mantid::MDAlgorithms::RunParam> rParam2;


public:
  
  // set up a simple RunParam object and test
  void testInit()
  {
    using namespace Mantid::MDAlgorithms;
    rParam = boost::shared_ptr<RunParam> (new RunParam() ) ;
    rParam->setEi(10.);
    TS_ASSERT_EQUALS(rParam->getEi(),10.);
    // create an RunParam with the values used in Tobyfit demo example
    rParam2 = boost::shared_ptr<RunParam> (new RunParam(
        45., 45., 5., 42.,
        0.5, 10., 7.19, 1.82,
        66.67, 66.67, 13.55314, 50.,
        0., 0., 0., 26.7,
        1, 2.28, 49., 1300.,
        150., 0., 3.87, 3.87,
        3.87, 90., 90., 90.,
        1., 0., 0., 0.,
        1., 0., 0., 0.,
        0., 0., 1., 1.,
        0., -1., 1., 0.,
        10., 14., 18., 1,
        10., 0.5
        ));


    // check simple get's on preset parameters
    // In general will not want to provide these gets as a public interface, need instead to provide the
    // the computed values that are needed later on, e.g. reciprocal lattice lengths, not real lattice values.
    TS_ASSERT_EQUALS(rParam2->getEi(),45.);
    TS_ASSERT_EQUALS(rParam2->getPsi(),45.);

    TS_ASSERT_EQUALS(rParam2->getElo(),5.);
    TS_ASSERT_EQUALS(rParam2->getEhi(),42.);

    TS_ASSERT_EQUALS(rParam2->getDe(),0.5);

    TS_ASSERT_EQUALS(rParam2->getX0(),10.);
    TS_ASSERT_DELTA(rParam2->getXa(),7.19,1e-5);
    TS_ASSERT_DELTA(rParam2->getX1(),1.82,1e-5);

    TS_ASSERT_DELTA(rParam2->getWa(),66.67,1e-5);
    TS_ASSERT_DELTA(rParam2->getHa(),66.67,1e-5);

    TS_ASSERT_DELTA(rParam2->getS1(),13.55314,1e-5);
    TS_ASSERT_DELTA(rParam2->getS2(),50.,1e-5);
    TS_ASSERT_DELTA(rParam2->getS3(),0.,1e-5);
    TS_ASSERT_DELTA(rParam2->getS4(),0.,1e-5);
    TS_ASSERT_DELTA(rParam2->getS5(),0.,1e-5);
    TS_ASSERT_DELTA(rParam2->getThetam(),26.7,1e-5);
    TS_ASSERT_EQUALS(rParam2->getModModel(),1);

    // note these are scaled from mm (in) to m (internal).
    TS_ASSERT_DELTA(rParam2->getPslit(),2.28e-3,1e-5);
    TS_ASSERT_DELTA(rParam2->getRadius(),49.e-3,1e-5);
    TS_ASSERT_DELTA(rParam2->getRho(),1300.e-3,1e-5);
    TS_ASSERT_DELTA(rParam2->getHz(),150.,1e-5);
    TS_ASSERT_DELTA(rParam2->getTjit(),0.,1e-5);

    TS_ASSERT_DELTA(rParam2->getAs(),3.87,1e-5);
    TS_ASSERT_DELTA(rParam2->getBs(),3.87,1e-5);
    TS_ASSERT_DELTA(rParam2->getCs(),3.87,1e-5);

    TS_ASSERT_DELTA(rParam2->getAa(),90.,1e-5);
    TS_ASSERT_DELTA(rParam2->getBb(),90.,1e-5);
    TS_ASSERT_DELTA(rParam2->getCc(),90.,1e-5);

    TS_ASSERT_DELTA(rParam2->getUh(),1.,1e-5);
    TS_ASSERT_DELTA(rParam2->getUk(),0.,1e-5);
    TS_ASSERT_DELTA(rParam2->getUl(),0.,1e-5);

    TS_ASSERT_DELTA(rParam2->getVh(),0.,1e-5);
    TS_ASSERT_DELTA(rParam2->getVk(),1.,1e-5);
    TS_ASSERT_DELTA(rParam2->getVl(),0.,1e-5);

    TS_ASSERT_DELTA(rParam2->getOmega(),0.,1e-5);
    TS_ASSERT_DELTA(rParam2->getGs(),0.,1e-5);
    TS_ASSERT_DELTA(rParam2->getGl(),0.,1e-5);
    TS_ASSERT_DELTA(rParam2->getDpsi(),0.,1e-5);

    TS_ASSERT_DELTA(rParam2->getXh(),1.,1e-5);
    TS_ASSERT_DELTA(rParam2->getXk(),1.,1e-5);
    TS_ASSERT_DELTA(rParam2->getXl(),0.,1e-5);

    TS_ASSERT_DELTA(rParam2->getYh(),-1.,1e-5);
    TS_ASSERT_DELTA(rParam2->getYk(),1.,1e-5);
    TS_ASSERT_DELTA(rParam2->getYl(),0.,1e-5);

    TS_ASSERT_DELTA(rParam2->getSx(),10.,1e-5);
    TS_ASSERT_DELTA(rParam2->getSy(),14.,1e-5);
    TS_ASSERT_DELTA(rParam2->getSz(),18.,1e-5);

    TS_ASSERT_EQUALS(rParam2->getIsam(),1);

    TS_ASSERT_EQUALS(rParam2->getTemp(),10.);

    TS_ASSERT_DELTA(rParam2->getEta(),0.5,1e-5)

    // test two points from Tobyfit for "demo.tf" HET with SMOD1 and SMOD2 set
    TS_ASSERT_DELTA(rParam2->areaToTIK(0.001001001001001001,13.55314,50,0.),0.059738,1e-5);
    TS_ASSERT_DELTA(rParam2->areaToTIK(0.6666666666666666,13.55314,50,0.),0.5336866686596371,1e-5);
    // test fot MAPS case with SMOD1=3.3, rest zero, identical results to above
    TS_ASSERT_DELTA(rParam2->areaToTIK(0.6666666666666666,32.,0.,0.),0.5336866686596371,1e-5);
    //  test with non-zero SMOD3 which does change table values - not sure if physical
    TS_ASSERT_DELTA(rParam2->areaToTIK(0.6666666666666666,13.,50,0.1),0.52255474006789071,1e-5);
    //
    TS_ASSERT_DELTA(rParam2->moderatorDepartTime(0.5),-4.4175090026406118e-6,1e-10);
    TS_ASSERT_DELTA(rParam2->moderatorDepartTime(0.25),-1.7249099699136883e-5,1e-10);
    TS_ASSERT_DELTA(rParam2->moderatorDepartTime(0.75),1.24743490059619e-5,1e-10);

    TS_ASSERT_DELTA(rParam2->tridev(0.00),-1.00              ,1e-10);
    TS_ASSERT_DELTA(rParam2->tridev(0.25),-0.2928932188134524,1e-10);
    TS_ASSERT_DELTA(rParam2->tridev(0.50),0.00               ,1e-10);
    TS_ASSERT_DELTA(rParam2->tridev(0.75), 0.2928932188134524,1e-10);
    TS_ASSERT_DELTA(rParam2->tridev(1.00),1.00               ,1e-10);

    TS_ASSERT_DELTA(rParam2->tChopVariance(), 1.027298e-10  ,1e-15);
    /*double sum=0.;
     int n=10000000;
     std::cout << "start\n";
     for(int i=0;i<n;i++)
     {
         double x=i/(n+0.0);
         sum+= rParam2->moderatorDepartTime(x);
         //std::cout << "i=" << i << " x=" << x << "  dt=" << rParam2->moderatorDepartTime(x) << "\n";
     }
     std::cout << "sum=" << sum << " stop\n";
     */
  }

  void testTidyUp()
  {
  }

};

#endif /*RUNPARAMTEST_H_*/
