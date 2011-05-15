#ifndef MANTID_GEOMETRY_UNITCELLTEST_H_
#define MANTID_GEOMETRY_UNITCELLTEST_H_

#include <cxxtest/TestSuite.h>
#include <MantidKernel/Timer.h>
#include <MantidKernel/System.h>
#include <iostream>
#include <iomanip>
#include <MantidGeometry/Math/Matrix.h>

#include <MantidGeometry/Crystal/UnitCell.h>

using namespace Mantid::Geometry;

class UnitCellTest : public CxxTest::TestSuite
{
public:

  void test_Simple()
  {
    // test constructors, access to some of the variables
    UnitCell u1,u2(3,4,5),u3(2,3,4,85.,95.,100),u4;
    u4=u2;
    TS_ASSERT_EQUALS(u1.a1(),1);
    TS_ASSERT_EQUALS(u1.alpha(),90);
    TS_ASSERT_DELTA(u2.b1(),1./3.,1e-10);
    TS_ASSERT_DELTA(u2.alphastar(),90,1e-10);
    TS_ASSERT_DELTA(u4.volume(),1./u2.recVolume(),1e-10);
    u2.seta(3);
    TS_ASSERT_DELTA(u2.a(),3,1e-10);
  }

  void checkCell(UnitCell & u)
  {
    TS_ASSERT_DELTA(u.a(),2.5,1e-10);
    TS_ASSERT_DELTA(u.b(),6,1e-10);
    TS_ASSERT_DELTA(u.c(),8,1e-10);
    TS_ASSERT_DELTA(u.alpha(),93,1e-10);
    TS_ASSERT_DELTA(u.beta(),88,1e-10);
    TS_ASSERT_DELTA(u.gamma(),97,1e-10);

    // get the some elements of the B matrix
    TS_ASSERT_DELTA(u.getB()[0][0],0.403170877311,1e-10);
    TS_ASSERT_DELTA(u.getB()[2][0],0.0,1e-10);
    TS_ASSERT_DELTA(u.getB()[0][2],-0.00360329991666,1e-10);
    TS_ASSERT_DELTA(u.getB()[2][2],0.125,1e-10);

    // Inverse B matrix
    MantidMat I = u.getB() * u.getBinv();
    TS_ASSERT_EQUALS( I, Matrix<double>(3,3,true));

    // d spacing for direct lattice at (1,1,1) (will automatically check dstar)
    TS_ASSERT_DELTA(u.d(1.,1.,1.),2.1227107587,1e-10);
    TS_ASSERT_DELTA(u.d(V3D(1.,1.,1.)),2.1227107587,1e-10);
    // angle
    TS_ASSERT_DELTA(u.recAngle(1,1,1,1,0,0,angRadians),0.471054990614,1e-10);

    // Convert to and from HKL
    V3D hkl = u.hklFromQ(V3D(1.0, 2.0, 3.0));
    double dstar = u.dstar(hkl[0], hkl[1], hkl[2]);
    TS_ASSERT_DELTA( dstar, sqrt(1+4.0+9.0), 1e-4); // The d-spacing after a round trip matches the Q we put in
  }

  void test_Advanced()
  {
    // test more advanced calculations
    // the new Gstar shold yield a=2.5, b=6, c=8, alpha=93, beta=88, gamma=97.
    MantidMat newGstar(3,3);
    newGstar[0][0]=0.162546756312;
    newGstar[0][1]=0.00815256992072;
    newGstar[0][2]=-0.00145274558861;
    newGstar[1][0]=newGstar[0][1];
    newGstar[1][1]=0.028262965555;
    newGstar[1][2]=0.00102046431298;
    newGstar[2][0]=newGstar[0][2];
    newGstar[2][1]=newGstar[1][2];
    newGstar[2][2]=0.0156808990098;

    UnitCell u;
    u.recalculateFromGstar(newGstar);

    // Check the directly-created one
    checkCell(u);

    // Check if copy constructor is also good.
    UnitCell u2 = u;
    checkCell(u2);

  }


  void testUnitRotation(){
	  UnitCell theCell;
	  MantidMat rot;
	  TSM_ASSERT_THROWS_NOTHING("The unit transformation should not throw",rot=theCell.getUBmatrix(V3D(1,0,0),V3D(0,1,0)));

	  std::vector<double> Rot = rot.get_vector();
	  std::vector<double> rez(9,0);
	  rez[0]=1;
	  rez[4]=1;
	  rez[8]=1;
      double err=0;
	  for(int i=0;i<9;i++){
		  err += (rez[i]-Rot[i])*(rez[i]-Rot[i]);
	  }
	  TSM_ASSERT_DELTA("This should produce proper permutation matrix defined as a vector",0,err,1.e-6);

  }
  void testParallelProjThrows(){
		 UnitCell theCell;
		 MantidMat rot;
		 TSM_ASSERT_THROWS("The transformation to plain defined by two parallel vectors should throw",
			rot=theCell.getUBmatrix(V3D(0,1,0),V3D(0,1,0)),std::invalid_argument);
  }
  void testPermutations(){
	  UnitCell theCell;
	  MantidMat rot;
	  TSM_ASSERT_THROWS_NOTHING("The permutation transformation should not throw",rot=theCell.getUBmatrix(V3D(0,1,0),V3D(1,0,0)));

	  std::vector<double> Rot = rot.get_vector();
	  std::vector<double> rez(9,0);
	  rez[1]=1;
	  rez[3]=1;
	  rez[8]=-1;
	  double err=0;
	  for(int i=0;i<9;i++){
		  err += (rez[i]-Rot[i])*(rez[i]-Rot[i]);
	  }
	  TSM_ASSERT_DELTA("This should produce proper permutation matrix defined as a vector",0,err,1.e-6);

  }
	
 void testRotations2D(){
	  UnitCell theCell;
	  MantidMat rot;
	  TSM_ASSERT_THROWS_NOTHING("The permutation transformation should not throw",rot=theCell.getUBmatrix(V3D(1,1,0),V3D(1,-1,0)));
	  V3D dir0(sqrt(2.),0,0);
	
	  std::vector<double> Rot = rot.get_vector();
	  double x = Rot[0]*dir0.X()+Rot[3]*dir0.Y()+Rot[6]*dir0.Z();
	  double y = Rot[1]*dir0.X()+Rot[4]*dir0.Y()+Rot[7]*dir0.Z();
	  double z = Rot[2]*dir0.X()+Rot[5]*dir0.Y()+Rot[8]*dir0.Z();

	 TSM_ASSERT_DELTA("X-coord shoud be 1",1,x,1.e-5);
	 TSM_ASSERT_DELTA("Y-coord shoud be 1",1,y,1.e-5);
	 TSM_ASSERT_DELTA("Z-coord shoud be 0",0,z,1.e-5);
  }
   void testRotations3D(){
	  UnitCell theCell;
	  MantidMat rot;
	  // two orthogonal vectors
	  V3D ort1(sqrt(2.),-1,-1);
	  V3D ort2(sqrt(2.),1,1);
	  TSM_ASSERT_THROWS_NOTHING("The permutation transformation should not throw",rot=theCell.getUBmatrix(ort1,ort2));

	  V3D dir(1,0,0);
	  V3D xx = ort1.cross_prod(ort2);
	  //double pp = xx.scalar_prod(dir); // dir should belong to ort1,ort2 plain
     
	  double p1=dir.scalar_prod(ort1)/ort1.norm();
	  double p2=dir.scalar_prod(ort2)/ort2.norm();

	  
	
	  std::vector<double> Rot = rot.get_vector();
	  double x = Rot[0]*dir.X()+Rot[3]*dir.Y()+Rot[6]*dir.Z();
	  double y = Rot[1]*dir.X()+Rot[4]*dir.Y()+Rot[7]*dir.Z();
	  double z = Rot[2]*dir.X()+Rot[5]*dir.Y()+Rot[8]*dir.Z();

	 TSM_ASSERT_DELTA("X-coord should be 1/sqrt(2)",p1,x,1.e-5);
	 TSM_ASSERT_DELTA("Y-coord shlule be 1/sqrt(2)",p2,y,1.e-5);
	 TSM_ASSERT_DELTA("Z-coord should be 0"  ,0, z,1.e-5);
  }
  void testRotations3DNonOrthogonal(){
	  UnitCell theCell(1,2,3,30,60,45);
	  MantidMat rot;
	  TSM_ASSERT_THROWS_NOTHING("The permutation transformation should not throw",rot=theCell.getUBmatrix(V3D(1,0,0),V3D(0,1,0)));

	  V3D dir(1,1,1);
 
	  std::vector<double> Rot = rot.get_vector();
	  double x = Rot[0]*dir.X()+Rot[3]*dir.Y()+Rot[6]*dir.Z();
	  double y = Rot[1]*dir.X()+Rot[4]*dir.Y()+Rot[7]*dir.Z();
	  double z = Rot[2]*dir.X()+Rot[5]*dir.Y()+Rot[8]*dir.Z();
	// this freeses the interface but unclear how to propelry indentify the 
	 TSM_ASSERT_DELTA("X-coord should be specified correctly",1.4915578672621419,x,1.e-5);
	 TSM_ASSERT_DELTA("Y-coord should be specified correctly",0.18234563931714265,y,1.e-5);
	 TSM_ASSERT_DELTA("Z-coord should be specified correctly",-0.020536948488997286,z,1.e-5);
  }

};


#endif /* MANTID_GEOMETRY_UNITCELLTEST_H_ */

