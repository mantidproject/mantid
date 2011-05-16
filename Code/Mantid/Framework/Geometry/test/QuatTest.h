#ifndef MANTID_TESTQUAT__
#define MANTID_TESTQUAT__

#include <cxxtest/TestSuite.h>
#include <cmath>
#include <ostream>
#include <float.h>
#include "MantidGeometry/V3D.h"
#include "MantidGeometry/Quat.h" 

using namespace Mantid::Geometry;

class QuatTest : public CxxTest::TestSuite
{
private:

  Mantid::Geometry::Quat q,p;

public:
  void testoperatorbracket()
  {
    p[0]=0;
    p[1]=1;
    p[2]=2;
    p[3]=3;
    TS_ASSERT_EQUALS(p[0],0.0);
    TS_ASSERT_EQUALS(p[1],1.0);
    TS_ASSERT_EQUALS(p[2],2.0);
    TS_ASSERT_EQUALS(p[3],3.0);
  }

  void testEmptyConstructor()
  {
    TS_ASSERT_EQUALS(q[0],1.0);
    TS_ASSERT_EQUALS(q[1],0.0);
    TS_ASSERT_EQUALS(q[2],0.0);
    TS_ASSERT_EQUALS(q[3],0.0);
  }

  void testValueConstructor()
  {
    Mantid::Geometry::Quat q1(1,2,3,4);
    TS_ASSERT_EQUALS(q1[0],1.0);
    TS_ASSERT_EQUALS(q1[1],2.0);
    TS_ASSERT_EQUALS(q1[2],3.0);
    TS_ASSERT_EQUALS(q1[3],4.0);
  }

  void testAngleAxisConstructor()
  {
    Mantid::Geometry::V3D v(1,1,1);
    // Construct quaternion to represent rotation
    // of 45 degrees around the 111 axis.
    Mantid::Geometry::Quat q1(90.0,v);
    double c=1.0/sqrt(2.0);
    double s=c/sqrt(3.0);
    TS_ASSERT_DELTA(q1[0],c,0.000001);
    TS_ASSERT_DELTA(q1[1],s,0.000001);
    TS_ASSERT_DELTA(q1[2],s,0.000001);
    TS_ASSERT_DELTA(q1[3],s,0.000001);
  }

  void testoperatorassignmentfromdouble()
  {
    q(2,3,4,5);
    TS_ASSERT_EQUALS(q[0],2.0);
    TS_ASSERT_EQUALS(q[1],3.0);
    TS_ASSERT_EQUALS(q[2],4.0);
    TS_ASSERT_EQUALS(q[3],5.0);
  }

  void testoperatorassignmentfromangleaxis()
  {
    Mantid::Geometry::V3D v(1,1,1);
    q(90.0,v);
    double c=1.0/sqrt(2.0);
    double s=c/sqrt(3.0);
    TS_ASSERT_DELTA(q[0],c,0.000001);
    TS_ASSERT_DELTA(q[1],s,0.000001);
    TS_ASSERT_DELTA(q[2],s,0.000001);
    TS_ASSERT_DELTA(q[3],s,0.000001);

    //Now rotate 45 degrees around y
    q(45, V3D(0,1,0));
    V3D X(1,0,0);
    q.rotate(X);
    double a = sqrt(2.0)/2;
    TS_ASSERT(X==V3D(a,0,-a));
    //Now rotate -45 degrees around y
    q(-45, V3D(0,1,0));
    X(1,0,0);
    q.rotate(X);
    TS_ASSERT(X==V3D(a,0,a));
  }

  void testoperatorequal()
  {
    q=p;
    TS_ASSERT_EQUALS(q[0],p[0]);
    TS_ASSERT_EQUALS(q[1],p[1]);
    TS_ASSERT_EQUALS(q[2],p[2]);
    TS_ASSERT_EQUALS(q[3],p[3]);
  }

  void testlenmethod()
  {
    q(1,2,3,4);
    TS_ASSERT_EQUALS(q.len(),sqrt(30.0));
  }

  void testlen2method()
  {
    q(1,2,3,4);
    TS_ASSERT_EQUALS(q.len2(),30.0);
  }

  void testinitmehtod()
  {
    q.init();
    TS_ASSERT_EQUALS(q[0],1);
    TS_ASSERT_EQUALS(q[1],0);
    TS_ASSERT_EQUALS(q[2],0);
    TS_ASSERT_EQUALS(q[3],0);
  }

  void testnormalizemethod()
  {
    q(2,2,2,2);
    q.normalize();
    TS_ASSERT_DELTA(q[0],0.5,0.000001);
    TS_ASSERT_DELTA(q[1],0.5,0.000001);
    TS_ASSERT_DELTA(q[2],0.5,0.000001);
    TS_ASSERT_DELTA(q[3],0.5,0.000001);
  }

  void testconjugatemethod()
  {
    q(1,1,1,1);
    q.conjugate();
    TS_ASSERT_EQUALS(q[0],1);
    TS_ASSERT_EQUALS(q[1],-1);
    TS_ASSERT_EQUALS(q[2],-1);
    TS_ASSERT_EQUALS(q[3],-1);
  }

  void testinversemethod()
  {
    q(2,3,4,5);
    Mantid::Geometry::Quat qinv(q);
    qinv.inverse();
    q*=qinv;
    TS_ASSERT_DELTA(q[0],1,0.000001);
    TS_ASSERT_DELTA(q[1],0,0.000001);
    TS_ASSERT_DELTA(q[2],0,0.000001);
    TS_ASSERT_DELTA(q[3],0,0.000001);
  }

  void testoperatorplus()
  {
    q(1,1,1,1);
    p(-1,2,1,3);
    Mantid::Geometry::Quat res;
    res=p+q;
    TS_ASSERT_EQUALS(res[0],0);
    TS_ASSERT_EQUALS(res[1],3);
    TS_ASSERT_EQUALS(res[2],2);
    TS_ASSERT_EQUALS(res[3],4);
  }

  void testoperatorminus()
  {
    q(1,1,1,1);
    p(-1,2,1,3);
    Mantid::Geometry::Quat res;
    res=p-q;
    TS_ASSERT_EQUALS(res[0],-2);
    TS_ASSERT_EQUALS(res[1],1);
    TS_ASSERT_EQUALS(res[2],0);
    TS_ASSERT_EQUALS(res[3],2);
  }

  void testoperatortimes()
  {
    q(1,1,1,1);
    p(-1,2,1,3);
    Mantid::Geometry::Quat res;
    res=p*q;
    TS_ASSERT_EQUALS(res[0],-7);
    TS_ASSERT_EQUALS(res[1],-1);
    TS_ASSERT_EQUALS(res[2],1);
    TS_ASSERT_EQUALS(res[3],3);
  }

  void testoperatordoublequal()
  {
    p=q;
    TS_ASSERT(p==q);
    q(1,4,5,6);
    TS_ASSERT(p!=q);
  }

  void testoperatornotequal()
  {
    q(1,2,3,4);
    TS_ASSERT(p!=q);
    p=q;
    TS_ASSERT(!(p!=q));
  }

  void testRotateVector()
  {
    double a = sqrt(2.0)/2;

    //Trivial
    p(1,0,0,0);//Identity quaternion
    V3D v(1,0,0);
    V3D orig_v = v;
    p.rotate(v);
    TS_ASSERT(orig_v==v);

    //Now do more angles
    v = V3D(1,0,0);
    p(90., V3D(0,1,0)); //90 degrees, right-handed, around y
    p.rotate(v);
    TS_ASSERT(v==V3D(0,0,-1));

    v = V3D(1,0,0);
    p(45., V3D(0,0,1));
    p.rotate(v);
    TS_ASSERT(v==V3D(a, a, 0));

    v = V3D(1,0,0);
    p(-45., V3D(0,0,1));
    p.rotate(v);
    TS_ASSERT(v==V3D(a, -a, 0));

    v = V3D(1,0,0);
    p(30., V3D(0,0,1));
    p.rotate(v);
    TS_ASSERT(v==V3D(sqrt(3.0)/2, 0.5, 0));

    v = V3D(1,0,0);
    p(125., V3D(1,0,0));
    p.rotate(v);
    TS_ASSERT(v==V3D(1,0,0));

    //90 deg around +Z
    p(90, V3D(0,0,1));
    v = V3D(1,0,0);    p.rotate(v);    TS_ASSERT(v==V3D(0,1,0));
    v = V3D(0,1,0);    p.rotate(v);    TS_ASSERT(v==V3D(-1,0,0));

    //std::cout << "Rotated v is" << v << "\n";
  }

 void testGetRotation()
  {
	V3D some(1,0.5,1);
	V3D target(1,2,-1);
	//V3D some(1,0,0);
	//V3D target(0,1,0);


    V3D rotAxis = some.cross_prod(target);
	rotAxis.normalize();

	double targ_norm=target.norm();
	double some_norm=some.norm();
	double cc = some.scalar_prod(target)/some_norm/targ_norm;
	double rotAngle = acos(cc)*180/M_PI;

	// rotator will be unit quaternion as it is build by the constructor this way;
	Quat rotator(rotAngle,rotAxis);	


	std::vector<double> rotMatrix;
    TSM_ASSERT_THROWS_NOTHING("The rotator quaternion has to be a unit quaternion",rotMatrix= rotator.getRotation(true));

	// Kroniker Deltas valid for valid rotational matrix; a_ij*a_jk=delta_jk 
	double cron00= rotMatrix[0]*rotMatrix[0]+rotMatrix[1]*rotMatrix[1]+rotMatrix[2]*rotMatrix[2];	TSM_ASSERT_DELTA("delta_00 should be 1",1.0,cron00,FLT_EPSILON);
	double cron11= rotMatrix[3]*rotMatrix[3]+rotMatrix[4]*rotMatrix[4]+rotMatrix[5]*rotMatrix[5];	TSM_ASSERT_DELTA("delta_11 should be 1",1.0,cron11,FLT_EPSILON);
	double cron22= rotMatrix[6]*rotMatrix[6]+rotMatrix[7]*rotMatrix[7]+rotMatrix[8]*rotMatrix[8];   TSM_ASSERT_DELTA("delta_22 should be 1",1.0,cron22,FLT_EPSILON);

	double cron01= rotMatrix[0]*rotMatrix[1]+rotMatrix[3]*rotMatrix[4]+rotMatrix[6]*rotMatrix[7];	TSM_ASSERT_DELTA("delta_01 should be 0",0.0,cron01,FLT_EPSILON);
	double cron02= rotMatrix[0]*rotMatrix[2]+rotMatrix[3]*rotMatrix[5]+rotMatrix[6]*rotMatrix[8];	TSM_ASSERT_DELTA("delta_02 should be 0",0.0,cron02,FLT_EPSILON);
	double cron12= rotMatrix[1]*rotMatrix[2]+rotMatrix[4]*rotMatrix[5]+rotMatrix[7]*rotMatrix[8];	TSM_ASSERT_DELTA("delta_12 should be 0",0.0,cron12,FLT_EPSILON);


	double det =  rotMatrix[0]*(rotMatrix[4]*rotMatrix[8]-rotMatrix[5]*rotMatrix[7])
	            + rotMatrix[1]*(rotMatrix[5]*rotMatrix[6]-rotMatrix[3]*rotMatrix[8])
		        + rotMatrix[2]*(rotMatrix[3]*rotMatrix[7]-rotMatrix[4]*rotMatrix[6]);

	TSM_ASSERT_DELTA("Determinant for the proper rotation matrix has to be equal to 1 ",1.0,det,FLT_EPSILON);

	double x1=(rotMatrix[0]*some.X()+rotMatrix[3]*some.Y()+rotMatrix[6]*some.Z())*targ_norm/some_norm;
	TSM_ASSERT_DELTA("X -coordinate obtained using the rotation matxis have to coinside with the one obtained by rotation via quat",x1,target.X(),FLT_EPSILON);

	double y1=(rotMatrix[1]*some.X()+rotMatrix[4]*some.Y()+rotMatrix[7]*some.Z())*targ_norm/some_norm;
	TSM_ASSERT_DELTA("Y -coordinate obtained using the rotation matxis have to coinside with the one obtained by rotation via quat",y1,target.Y(),FLT_EPSILON);

 	double z1=(rotMatrix[2]*some.X()+rotMatrix[5]*some.Y()+rotMatrix[8]*some.Z())*targ_norm/some_norm;
	TSM_ASSERT_DELTA("Z -coordinate obtained using the rotation matxis have to coinside with the one obtained by rotation via quat",z1,target.Z(),FLT_EPSILON);

	// if the vectors are not notmalized (not equal), the angle between the vectors calculated by the constructor below would not be equal to the one, calculated 
	// above. 
	some *=(targ_norm/some_norm);
 	Quat rot2(some,target);	
  
	std::vector<double> rotMatrix2;
	TSM_ASSERT_THROWS_NOTHING("The rotator quaternion has to be a unit quaternion",rotMatrix2	=rot2.getRotation(true));

	for(int i=0;i<9;i++){
		TSM_ASSERT_DELTA("Elements of the rotation matrix obtained quat on 2 vectors have to be equivalent",rotMatrix[i],rotMatrix2[i],FLT_EPSILON);
	}

	x1=(rotMatrix2[0]*some.X()+rotMatrix2[3]*some.Y()+rotMatrix2[6]*some.Z());
	TSM_ASSERT_DELTA("X -coordinate obtained using the rotation matxis have to coinside with the one obtained by rotation via quat",x1,target.X(),FLT_EPSILON);

	y1=(rotMatrix2[1]*some.X()+rotMatrix2[4]*some.Y()+rotMatrix2[7]*some.Z());
	TSM_ASSERT_DELTA("Y -coordinate obtained using the rotation matxis have to coinside with the one obtained by rotation via quat",y1,target.Y(),FLT_EPSILON);

 	z1=(rotMatrix2[2]*some.X()+rotMatrix2[5]*some.Y()+rotMatrix2[8]*some.Z());
	TSM_ASSERT_DELTA("Z -coordinate obtained using the rotation matxis have to coinside with the one obtained by rotation via quat",z1,target.Z(),FLT_EPSILON);


 }
 void testUnitQuatFromUnitRotMatrix(){
	 MantidMat Rot(3,3);
	 Rot[0][0]=1;
	 Rot[1][1]=1;
	 Rot[2][2]=1;

	 Quat Test;
	 Test.setQuat(Rot);

	 std::vector<double> rez = Test.getRotation();
	 std::vector<double> rot = Rot.get_vector();
	 TSM_ASSERT_EQUALS("This operation should return rotation matrix",rot,rez);

 }

 void testQuatFromRotMatrix(){
	 MantidMat Rot(3,3);
	 int Nx(5),Ny(5),Nz(3);
	 double Phi=M_PI/2/Nx;
	 double Tht=M_PI/2/Ny;
	 double Psi=M_PI/2/Ny;
	 Quat Test;
	 std::vector<double> rez;
	 std::vector<double> rot;

	 for(int i=0;i<=Nx;i++){
		 double cT=cos(Tht*i);
		 double sT=sin(Tht*i);
		 for(int j=0;j<=Ny;j++){
			 double cF = cos(j*Phi);
			 double sF = sin(j*Phi);
			 for (int k=0;k<=Nz;k++){
				Rot.zeroMatrix();
				double cP = cos(k*Psi);
				double sP = sin(k*Psi);
	
				Rot[0][0]=cT*cP;   Rot[1][0]=-cF*sP+sF*sT*cP;    Rot[2][0]=sF*sP+cF*sT*cP;
		        Rot[0][1]=cT*sP;   Rot[1][1]= cF*cP+sF*sT*sP;    Rot[2][1]=-sF*cP+cF*sT*sP;
		        Rot[0][2]=-sT;     Rot[1][2]= sF*cT;             Rot[2][2]= cT*cF;
			    V3D e1(1,0,0);
				V3D e2(0,1,0);
				V3D e3(0,0,1);

				V3D d1= Rot*e1;
				V3D d2= Rot*e2;
				V3D d3= Rot*e3;

			   Test.setQuat(Rot);
			   rez = Test.getRotation();
		       rot = Rot.get_vector();
			   for(int ii=0;ii<9;ii++){
					TSM_ASSERT_DELTA("This operation should return initial rotation matrix",rot[i],rez[i],1e-4);
			   }
			 }
		 }
	 }

 }

 void testSetFromDirectionCosineMatrix_trival()
  {
    Mantid::Geometry::V3D rX(1,0,0);
    Mantid::Geometry::V3D rY(0,1,0);
    Mantid::Geometry::V3D rZ(0,0,1);
    q(rX,rY,rZ);
    p(1,0,0,0); //Identity quaternion
    TS_ASSERT(p==q); //Trivial rotation
  }

  void testSetFromDirectionCosineMatrix2()
  {
    //Rotate 90 deg around Y
    V3D rX(0,0,-1);
    V3D rY(0,1,0);
    V3D rZ(1,0,0);
    q(rX,rY,rZ);
    p(90, V3D(0,1,0));
    TS_ASSERT(p==q);
  }

  void testSetFromDirectionCosineMatrix2b()
  {
    //Rotate -45 deg around Y
    double a = sqrt(2.0)/2;
    V3D rX(a,0,a);
    V3D rY(0,1,0);
    V3D rZ(-a,0,a);
    q(rX,rY,rZ);
    p(-45.0, V3D(0,1,0));
    TS_ASSERT(p==q);

    V3D oX(1,0,0);
    V3D oY(0,1,0);
    V3D oZ(0,0,1);
    q.rotate(oX);
    q.rotate(oY);
    q.rotate(oZ);
    TS_ASSERT(oX==rX);
    TS_ASSERT(oY==rY);
    TS_ASSERT(oZ==rZ);
  }

  void testSetFromDirectionCosineMatrix3()
  {
    //Rotate 90 deg around Z
    V3D rX(0,1,0);
    V3D rY(-1,0,0);
    V3D rZ(0,0,1);
    q(rX,rY,rZ);
    p(90, V3D(0,0,1));
    TS_ASSERT(p==q);
  }

  void testSetFromDirectionCosineMatrix4()
  {
    //Rotate 90 deg around X
    V3D rX(1,0,0);
    V3D rY(0,0,1);
    V3D rZ(0,-1,0);
    q(rX,rY,rZ);
    p(90, V3D(1,0,0));
    TS_ASSERT(p==q);
  }

  void compareArbitrary(const Quat& rotQ)
  {
    V3D oX(1,0,0);
    V3D oY(0,1,0);
    V3D oZ(0,0,1);
    V3D rX = oX;
    V3D rY = oY;
    V3D rZ = oZ;

    //Rotate the reference frame
    rotQ.rotate(rX);
    rotQ.rotate(rY);
    rotQ.rotate(rZ);

    //Now find it.
    q(rX,rY,rZ);

    q.rotate(oX);
    q.rotate(oY);
    q.rotate(oZ);
    TS_ASSERT(oX==rX);
    TS_ASSERT(oY==rY);
    TS_ASSERT(oZ==rZ);
    TS_ASSERT(rotQ==q);

    //    std::cout << "\nRotated coordinates are " << rX << rY << rZ << "\n";
    //    std::cout << "Expected (p) is" << p << "; got " << q << "\n";
    //    std::cout << "Re-Rotated coordinates are " << oX << oY << oZ << "\n";
  }

  void testSetFromDirectionCosineMatrix_arbitrary()
  {
    Quat rotQ;
    //Try a couple of random rotations
    rotQ = Quat(124.0, V3D(0.1, 0.2, sqrt(0.95)));
    this->compareArbitrary(rotQ);
    rotQ = Quat(-546.0, V3D(-0.5, 0.5, sqrt(0.5)));
    this->compareArbitrary(rotQ);
    rotQ = Quat(34.0, V3D(-0.5, 0.5, sqrt(0.5))) * Quat(-25.0, V3D(0.1, 0.2, sqrt(0.95)));
    this->compareArbitrary(rotQ);
  }

  void testConstructorFromDirectionCosine()
  {
    double a = sqrt(2.0)/2;
    V3D rX(a,0,a);
    V3D rY(0,1,0);
    V3D rZ(-a,0,a);
    Quat rotQ = Quat(rX,rY,rZ);
    p(-45.0, V3D(0,1,0));
    TS_ASSERT(rotQ==p);
  }

};

#endif
