#ifndef RUNPARAMTEST_H_
#define RUNPARAMTEST_H_

// ONLY BARE BONES OF POSSIBLE TEST FILE, MAINLY STILL TOBYFITTEST

#include <cxxtest/TestSuite.h>
#include <cmath>
#include <iostream>
#include <boost/scoped_ptr.hpp> 
#include "MantidGeometry/Instrument/Goniometer.h"
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
  boost::shared_ptr<Mantid::MDAlgorithms::RunParam> rParam3;
  boost::shared_ptr<Mantid::MDAlgorithms::RunParam> rParam4;


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

    TS_ASSERT_DELTA(rParam2->getWa(),66.67*1.e-3,1e-5);
    TS_ASSERT_DELTA(rParam2->getHa(),66.67*1.e-3,1e-5);

    TS_ASSERT_DELTA(rParam2->getS1(),13.55314,1e-5);
    TS_ASSERT_DELTA(rParam2->getS2(),50.,1e-5);
    TS_ASSERT_DELTA(rParam2->getS3(),0.,1e-5);
    TS_ASSERT_DELTA(rParam2->getS4(),0.,1e-5);
    TS_ASSERT_DELTA(rParam2->getS5(),0.,1e-5);
    TS_ASSERT_DELTA(rParam2->getThetam(),26.7*M_PI/180.,1e-5);
    TS_ASSERT_EQUALS(rParam2->getModModel(),1);

    // note these are scaled from mm (in) to m (internal).
    TS_ASSERT_DELTA(rParam2->getPslit(),2.28e-3,1e-5);
    TS_ASSERT_DELTA(rParam2->getRadius(),49.e-3,1e-5);
    TS_ASSERT_DELTA(rParam2->getRho(),1300.e-3,1e-5);
    TS_ASSERT_DELTA(rParam2->getAngVel(),150.*2*M_PI,1e-5);
    TS_ASSERT_DELTA(rParam2->getTjit(),0.,1e-5);

    TS_ASSERT_DELTA(rParam2->getAs(),3.87,1e-5);
    TS_ASSERT_DELTA(rParam2->getBs(),3.87,1e-5);
    TS_ASSERT_DELTA(rParam2->getCs(),3.87,1e-5);

    TS_ASSERT_DELTA(rParam2->getAa(),M_PI*0.5,1e-5);
    TS_ASSERT_DELTA(rParam2->getBb(),M_PI*0.5,1e-5);
    TS_ASSERT_DELTA(rParam2->getCc(),M_PI*0.5,1e-5);

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

    TS_ASSERT_DELTA(rParam2->getSx(),10.e-3,1e-8);
    TS_ASSERT_DELTA(rParam2->getSy(),14.e-3,1e-8);
    TS_ASSERT_DELTA(rParam2->getSz(),18.e-3,1e-8);

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

    // test lookup table for time - look up is order of magnitude faster and accurate apart
    // from end points. Table uses 1001 points so each .0005 is a mid point
    double t0 =rParam2->areaToTIK(0.5005,rParam2->getS1(),rParam2->getS2(),rParam2->getS3());
    double tl = rParam2->moderatorTimeLookUp(0.5005);
    TS_ASSERT_DELTA((t0-tl),0,1e-5);
    t0 =rParam2->areaToTIK(0.9995,rParam2->getS1(),rParam2->getS2(),rParam2->getS3());
    tl = rParam2->moderatorTimeLookUp(0.9995);
    TS_ASSERT_DELTA((t0-tl),0,0.095); // this last mid-point is 11% out
    t0 =rParam2->areaToTIK(0.0005,rParam2->getS1(),rParam2->getS2(),rParam2->getS3());
    tl = rParam2->moderatorTimeLookUp(0.0005);
    TS_ASSERT_DELTA((t0-tl),0,0.02); // this first mid point is 59% out but is rarely used

    TS_ASSERT_DELTA(rParam2->tridev(0.00),-1.00              ,1e-10);
    TS_ASSERT_DELTA(rParam2->tridev(0.25),-0.2928932188134524,1e-10);
    TS_ASSERT_DELTA(rParam2->tridev(0.50),0.00               ,1e-10);
    TS_ASSERT_DELTA(rParam2->tridev(0.75), 0.2928932188134524,1e-10);
    TS_ASSERT_DELTA(rParam2->tridev(1.00),1.00               ,1e-10);

    TS_ASSERT_DELTA(rParam2->tChopVariance(), 1.027298e-10  ,1e-15);

    double pw,ph;
    rParam2->getAperturePoint(0.,0.,pw,ph);
    TS_ASSERT_DELTA(pw,-0.0333350,1e-6)
    TS_ASSERT_DELTA(ph,-0.0333350,1e-6)
    rParam2->getAperturePoint(0.5,0.5,pw,ph);
    TS_ASSERT_DELTA(pw,0.0,1e-6)
    TS_ASSERT_DELTA(ph,0.0,1e-6)
    rParam2->getAperturePoint(1.0,1.0,pw,ph);
    TS_ASSERT_DELTA(pw,0.0333350,1e-6)
    TS_ASSERT_DELTA(ph,0.0333350,1e-6)

    double eta2,eta3;
    rParam2->getEta23( 0.25, 0.75, eta2, eta3);
    TS_ASSERT_DELTA(eta2,0.0000000,    1e-7)
    TS_ASSERT_DELTA(eta3,-0.0061706707,1e-7)
    rParam2->getEta23( 0.5, 0.5, eta2, eta3);
    TS_ASSERT_DELTA(eta2,-0.004363323,  1e-7)
    TS_ASSERT_DELTA(eta3,0.000000000,  1e-7)
    rParam2->getEta23( 0.9375, 0.0625,eta2, eta3);
    TS_ASSERT_DELTA(eta2,0.001230069,  1e-6)
    TS_ASSERT_DELTA(eta3,0.00050951129,1e-6)
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

  void testModChopTimes()
  {
    rParam3 = boost::shared_ptr<RunParam> (new RunParam(
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
    TS_ASSERT_DELTA(rParam3->getTauModeratorAverageUs(),40.65942,1e-5);
    TS_ASSERT_DELTA(rParam3->getTauModeratorSignal(),2.34747e-5,1e-9);
    TS_ASSERT_DELTA(rParam3->getTauModeratorMean(),4.065942e-5,1e-9);
    TS_ASSERT_DELTA(rParam3->energyResolutionModChop(12.25, 2.512),1.381791378,4e-8 )
    TS_ASSERT_DELTA(rParam3->energyResolutionModChop(13.25, 2.512),1.346621610,4e-8 )
    rParam4 = boost::shared_ptr<RunParam> (new RunParam(
           447., -90., 0., 0., 0., // ei, psi, elo, ehi, de
           10.1, 8.11, 1.9, 70.13, 70.13, //x0,xa,x1,wa,ha
           3.31, 0., 0., 0., 0., 32., //s1-5, theta
           1, //imod
           2.899, 49., 1300., 600., 0.0, //pslit,radius,rho,hz,tjit
           2.507, 2.507, 4.069, 90., 90., 120., //as,ab,cs,aa,bb,cc
           1., 0., 0., 0., 0., 1., //uhkl,vhkl
           0., 0., 0., 0.511967229831443, // omega, gs,gl,dpsi
           0., 0., 0., 0., 0., 0., //xhkjl,yhkl
           0., 0., 0., 0, //sxyz, isam
           10., 0.0 //temp,eta
       ));
    TS_ASSERT_DELTA(rParam4->getTauModeratorAverageUs(),9.930,1e-5);
    TS_ASSERT_DELTA(rParam4->energyResolutionModChop(195., 6.034),7.1854280755,4e-7 )
    TS_ASSERT_DELTA(rParam4->energyResolutionModChop(197., 6.034),7.1622982277,4e-7 )
  }

  void testUBmatrix()
  {
    // TF demo data with the U,V vectors rotated to Mantid coords (beam z, y up)
    rParam3 = boost::shared_ptr<RunParam> (new RunParam(
        45., 45., 5., 42.,
        0.5, 10., 7.19, 1.82,
        66.67, 66.67, 13.55314, 50.,
        0., 0., 0., 26.7,
        1, 2.28, 49., 1300.,
        150., 0., 3.87, 3.87,
        3.87, 90., 90., 90.,
        1., 0., 0., 0., 1., 0.,
        0., 0., 0., 0.,
        1., 1.,0., -1., 1., 0.,
        10., 14., 18., 1,
        10., 0.5
    ));
    rParam4 = boost::shared_ptr<RunParam> (new RunParam(
        447., -90., 0., 0., 0., // ei, psi, elo, ehi, de
                   10.1, 8.11, 1.9, 70.13, 70.13, //x0,xa,x1,wa,ha
                   3.31, 0., 0., 0., 0., 32., //s1-5, theta
                   1, //imod
                   2.899, 49., 1300., 600., 0.0, //pslit,radius,rho,hz,tjit
                   2.507, 2.507, 4.069, 90., 90., 120., //as,ab,cs,aa,bb,cc
                   //1., 0., 0., 0., 0., 1., //uhkl,vhkl
                   0., 0., 1., 0., 1., 0., // rot to MTF coords
                   0., 0., 0., 0.511967229831443, // omega, gs,gl,dpsi
                   0., 0., 0., 0., 0., 0., //xhkjl,yhkl
                   0., 0., 0., 0, //sxyz, isam
                   10., 0.0 //temp
    ));
    rParam3->setTransforms();
    TS_ASSERT_DELTA(rParam3->getCubInvMat()[0][0],0.435528,1e-5)
    TS_ASSERT_DELTA(rParam3->getCubInvMat()[0][2],0.435528,1e-5)
    TS_ASSERT_DELTA(rParam3->getCubInvMat()[2][1],0.61593,1e-5)
    TS_ASSERT_DELTA(rParam3->getSMat()[0][2],1.,1e-7)
    TS_ASSERT_DELTA(rParam3->getSMat()[1][1],1.,1e-7)
    TS_ASSERT_DELTA(rParam3->getSMat()[2][0],-1.,1e-7)
    rParam4->setTransforms();
    TS_ASSERT_DELTA(rParam4->getCubInvMat()[0][0],0.,1e-7)
    TS_ASSERT_DELTA(rParam4->getCubInvMat()[0][1],-0.399001,1e-5)
    TS_ASSERT_DELTA(rParam4->getSMat()[0][0],1.,1e-7)
    /*
    std::cout << "\ncubinvmat " << rParam3->getCubInvMat() <<"\n";
    std::cout << "smat " << rParam3->getSMat() <<"\n";
    std::cout << "cubinvmat " << rParam4->getCubInvMat() <<"\n";
    std::cout << "smat " << rParam4->getSMat() <<"\n";
    */

  }
  void testUPrivate()
  {
    // TF demo data with the U,V vectors rotated to Mantid coords (beam z, y up)
    rParam3 = boost::shared_ptr<RunParam> (new RunParam(
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
    rParam4 = boost::shared_ptr<RunParam> (new RunParam(
        45., 45., 5., 42.,
        0.5, 10., 7.19, 1.82,
        66.67, 66.67, 13.55314, 50.,
        0., 0., 0., 26.7,
        1, 2.28, 49., 1300.,
        150., 0., 3.87, 3.87,
        3.87, 90., 90., 90.,
        0., 0., 1., 1.,
        0., 0., 0., 0.,
        0., 0., 1., 0.,
        1., 1., 0., -1.,
        10., 14., 18., 1,
        10., 0.5
    ));

    // experimental testing to be replaced later
    rParam3->setTransforms();
    rParam4->setTransforms();
    OrientedLattice myLat;
    boost::shared_ptr<OrientedLattice> oLptr = boost::shared_ptr<OrientedLattice> (new OrientedLattice(3.87,3.87,3.87,90.,90.,90.));
    DblMatrix ub=myLat.getUB(); //ub.print();
    DblMatrix ub2=oLptr->getUB(); //ub2.print();
    V3D u(1.,0.,0.);
    V3D v(0.,1.,0.);
    oLptr->setUFromVectors(u,v);
    DblMatrix ub3=oLptr->getUB(); //ub3.print();
    rParam3->setRunLatticeMatrices(oLptr);
    TS_ASSERT_DELTA(rParam3->getAs(),3.87,1e-5);
    TS_ASSERT_DELTA(rParam3->getBs(),3.87,1e-5);
    TS_ASSERT_DELTA(rParam3->getCs(),3.87,1e-5);

    TS_ASSERT_DELTA(rParam3->getAa(),M_PI*0.5,1e-5);
    TS_ASSERT_DELTA(rParam3->getBb(),M_PI*0.5,1e-5);
    TS_ASSERT_DELTA(rParam3->getCc(),M_PI*0.5,1e-5);
    V3D u1(0.,0.,1.);
    V3D v1(1.,0.,0.);
    oLptr->setUFromVectors(u1,v1);
    DblMatrix ub4=oLptr->getUB();
    //ub4.print();
    //oLptr->getB().print();
    //oLptr->getU().print();
    Goniometer x;
    x.makeUniversalGoniometer();
    x.setRotationAngle(1,90.);
    x.setRotationAngle(2,rParam3->getPsi());
    //std::cout << "rot\n";
    //x.getR().print();
    rParam3->setTransforms();
  }
  void testDetectorInfo()
  {
    rParam3 = boost::shared_ptr<RunParam> (new RunParam(
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
    std::pair<V3D, V3D> det0(V3D(1,2,3), V3D(4,5,6));
    V3D d1(1,2,3); V3D d2(4,5,6);
    V3D detPos; V3D detDim;
    double deps;
    rParam3->setDetInfo(0,d1,d2,1.0);
    rParam3->setDetInfo(1,V3D(11,12,13),V3D(14,15,16),1.0);
    rParam3->setDetInfo(10,V3D(20,21,22),V3D(23,24,25),1.0);
    rParam3->getDetInfo(0,detPos,detDim,deps);
    TS_ASSERT_DELTA(detDim[2],6.,1e-10);
    TS_ASSERT_DELTA(detPos[1],2.,1e-10);
    TS_ASSERT_DELTA(deps,1.,1e-10);
    rParam3->getDetInfo(10,detPos,detDim,deps);
    TS_ASSERT_DELTA(detDim[0],23.,1e-10);
    TS_ASSERT_DELTA(detPos[2],22.,1e-10);
  }

  void testTidyUp()
  {
  }

};

#endif /*RUNPARAMTEST_H_*/
