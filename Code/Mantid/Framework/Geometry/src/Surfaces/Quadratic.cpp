 #include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <complex>
#include <cmath>
#include <list>
#include <vector>
#include <set>
#include <map>
#include <stack>
#include <string>
#include <algorithm>
#include <boost/multi_array.hpp>
#include <gsl/gsl_poly.h>

#include "MantidGeometry/Tolerance.h"
#include "MantidGeometry/Math/mathSupport.h"
#include "MantidGeometry/Math/Matrix.h"
#include "MantidGeometry/V3D.h"
#include "MantidGeometry/Math/PolyBase.h"
#include "MantidGeometry/Surfaces/BaseVisit.h"
#include "MantidGeometry/Surfaces/Surface.h"
#include "MantidGeometry/Surfaces/Quadratic.h"

#include "MantidKernel/Logger.h"
#include "MantidKernel/Strings.h"

namespace Mantid
{

namespace Geometry
{

Kernel::Logger& Quadratic::PLog(Kernel::Logger::get("Quadratic"));

/// Numerical tolerance
//const double QTolerance(1e-6);

Quadratic::Quadratic() : Surface(),
  BaseEqn(10)
  /**
    Constructor
  */
{}

Quadratic::Quadratic(const Quadratic& A) : Surface(A),
  BaseEqn(A.BaseEqn)
  /**
    Copy constructor
    @param A :: Quadratic to copy
  */
{ }


Quadratic&
Quadratic::operator=(const Quadratic& A)
  /**
    Assignment operator
    @param A :: Quadratic to copy
    @return *this
  */
{
  if (this!=&A)
    {
      Surface::operator=(A);
      BaseEqn=A.BaseEqn;
    }
  return *this;
}

Quadratic::~Quadratic()
  /**
    Destructor
  */
{}


double 
Quadratic::eqnValue(const Geometry::V3D& Pt) const
  /**
    Helper function to calcuate the value
    of the equation at a fixed point 
    @param Pt :: Point to determine the equation surface 
    value at
    @return value Eqn(Pt) : -ve inside +ve outside
  */
{
  double res(0.0);
  res+=BaseEqn[0]*Pt[0]*Pt[0];
  res+=BaseEqn[1]*Pt[1]*Pt[1];
  res+=BaseEqn[2]*Pt[2]*Pt[2];
  res+=BaseEqn[3]*Pt[0]*Pt[1];    
  res+=BaseEqn[4]*Pt[0]*Pt[2];    
  res+=BaseEqn[5]*Pt[1]*Pt[2];    
  res+=BaseEqn[6]*Pt[0];    
  res+=BaseEqn[7]*Pt[1];    
  res+=BaseEqn[8]*Pt[2];    
  res+=BaseEqn[9];
  return res;
}

int
Quadratic::side(const Geometry::V3D& Pt) const
  /**
    Determine if the the Point is true to the surface or
    on the other side
    @param Pt :: Point to check
    @retval 1 : if the point is "true" to the surface  
    @retval -1 : if the point is "false" to the surface  
    @retval 0 :: The point is on the surface
  */
{
  double res=eqnValue(Pt);
  if (fabs(res)<Tolerance)
    return 0;
  return (res>0) ? 1 : -1;
}


Geometry::V3D
Quadratic::surfaceNormal(const Geometry::V3D& Pt) const
  /**
    Given a point on the surface 
    Calculate the normal at the point 
    Some rather disturbing behaviour happens if 
    the point is a significant distance from the surface
    @param Pt :: Point to calcution
    @return normal unit vector
  */
{
   Geometry::V3D N(2*BaseEqn[0]*Pt[0]+BaseEqn[3]*Pt[1]+BaseEqn[4]*Pt[2]+BaseEqn[6],
	   2*BaseEqn[1]*Pt[1]+BaseEqn[3]*Pt[0]+BaseEqn[5]*Pt[2]+BaseEqn[7],
	   2*BaseEqn[2]*Pt[2]+BaseEqn[4]*Pt[0]+BaseEqn[5]*Pt[1]+BaseEqn[8]);
   N.normalize();
   return N;
}

void
Quadratic::matrixForm(Geometry::Matrix<double>& A,
		      Geometry::V3D& B,double& C) const
  /**
    Converts the baseEqn into the matrix form such that
    \f[ x^T A x + B^T x + C =0 \f]
    @param A :: Matrix to place equation into
    @param B :: Vector point 
    @param C :: Constant value
  */
{
  A.setMem(3,3);    // set incase memory out
  for(int i=0;i<3;i++)
    A[i][i]=BaseEqn[i];

  A[0][1]=A[1][0]=BaseEqn[3]/2.0;
  A[0][2]=A[2][0]=BaseEqn[4]/2.0;
  A[1][2]=A[2][1]=BaseEqn[5]/2.0;
  
  for(int i=0;i<3;i++)
    B[i]=BaseEqn[6+i];
  C=BaseEqn[9];
  return;
}

double
Quadratic::distance(const Geometry::V3D& Pt) const
  /**
    Proper calcuation of a point to a general surface 
    @param Pt :: Point to calculate distance from surace
    @return distance from point to surface (signed)
  */
{
  // Job 1 :: Create matrix and vector representation
  Geometry::Matrix<double> A(3,3);
  Geometry::V3D B;
  double cc;
  matrixForm(A,B,cc);
  
  //Job 2 :: calculate the diagonal matrix
  Geometry::Matrix<double> D(3,3);
  Geometry::Matrix<double> R(3,3);
  if (!A.Diagonalise(R,D))
    {
      std::cerr<<"Problem with matrix :: distance now guessed at"<<std::endl;
      return distance(Pt);
    }

  Geometry::V3D alpha=R.Tprime()*Pt;
  Geometry::V3D beta=R.Tprime()*B;
    
  // Calculate fundermental equation:
  const double aa(alpha[0]);  const double aa2(aa*aa);
  const double ab(alpha[1]);  const double ab2(ab*ab);
  const double ac(alpha[2]);  const double ac2(ac*ac);

  const double ba(beta[0]);  const double ba2(ba*ba);
  const double bb(beta[1]);  const double bb2(bb*bb);
  const double bc(beta[2]);  const double bc2(bc*bc);

  const double da(D[0][0]);  const double da2(da*da);
  const double db(D[1][1]);  const double db2(db*db);
  const double dc(D[2][2]);  const double dc2(dc*dc);
  
  mathLevel::PolyBase T(6);

  T[0]=aa*ba+ab*bb+ac*bc+cc+aa2*da+ab2*db+ac2*dc;

  T[1]= -ba2-bb2-bc2 + 4*ab*bb*da + 4*ac*bc*da + 4*cc*da + 4*aa*ba*db + 
	 4*ac*bc*db + 4*cc*db + 4*aa2*da*db + 4*ab2*da*db + 
	 4*aa*ba*dc + 4*ab*bb*dc + 4*cc*dc + 4*aa2*da*dc + 
	 4*ac2*da*dc + 4*ab2*db*dc +  4*ac2*db*dc;
	 
  T[2]= -ba2*da -4*bb2*da -4*bc2*da + 4*ab*bb*da2 + 4*ac*bc*da2 + 4*cc*da2  
    - 4*ba2*db - bb2*db - 4*bc2*db + 16*ac*bc*da*db + 
    16*cc*da*db + 4*ab2*da2*db + 4*aa*ba*db2 + 
    4*ac*bc*db2 + 4*cc*db2 + 4*aa2*da*db2 - 
    4*ba2*dc - 4*bb2*dc - bc2*dc + 16*ab*bb*da*dc + 
    16*cc*da*dc + 4*ac2*da2*dc + 16*aa*ba*db*dc + 
    16*cc*db*dc + 16*aa2*da*db*dc + 16*ab2*da*db*dc + 
    16*ac2*da*db*dc + 4*ac2*db2*dc + 4*aa*ba*dc2 + 
    4*ab*bb*dc2 + 4*cc*dc2 + 4*aa2*da*dc2 + 
    4*ab2*db*dc2;

  T[3]= -4*bb2*da2 - 4*bc2*da2 - 4*ba2*da*db - 4*bb2*da*db - 
         16*bc2*da*db + 16*ac*bc*da2*db + 16*cc*da2*db - 
          4*ba2*db2 - 4*bc2*db2 + 16*ac*bc*da*db2 + 
          16*cc*da*db2 - 4*ba2*da*dc - 16*bb2*da*dc - 
          4*bc2*da*dc + 16*ab*bb*da2*dc + 16*cc*da2*dc - 
          16*ba2*db*dc - 4*bb2*db*dc - 4*bc2*db*dc + 
          64*cc*da*db*dc + 16*ab2*da2*db*dc + 
          16*ac2*da2*db*dc + 16*aa*ba*db2*dc + 
          16*cc*db2*dc + 16*aa2*da*db2*dc + 
          16*ac2*da*db2*dc - 4*ba2*dc2 - 4*bb2*dc2 + 
          16*ab*bb*da*dc2 + 16*cc*da*dc2 + 16*aa*ba*db*dc2 + 
          16*cc*db*dc2 + 16*aa2*da*db*dc2 + 
          16*ab2*da*db*dc2;

  T[4]=-4*bb2*da2*db - 16*bc2*da2*db - 4*ba2*da*db2 - 16*bc2*da*db2 + 
         16*ac*bc*da2*db2 + 16*cc*da2*db2 - 16*bb2*da2*dc - 4*bc2*da2*dc - 
          16*ba2*da*db*dc - 16*bb2*da*db*dc - 16*bc2*da*db*dc + 64*cc*da2*db*dc - 
          16*ba2*db2*dc - 4*bc2*db2*dc + 64*cc*da*db2*dc + 16*ac2*da2*db2*dc - 
          4*ba2*da*dc2 - 16*bb2*da*dc2 + 16*ab*bb*da2*dc2 + 16*cc*da2*dc2 - 
          16*ba2*db*dc2 - 4*bb2*db*dc2 + 64*cc*da*db*dc2 + 16*ab2*da2*db*dc2 + 
          16*aa*ba*db2*dc2 + 16*cc*db2*dc2 + 16*aa2*da*db2*dc2;


  T[5]= -16*bc2*da2*db2 - 16*bb2*da2*db*dc - 16*bc2*da2*db*dc - 
          16*ba2*da*db2*dc - 16*bc2*da*db2*dc + 
          64*cc*da2*db2*dc - 16*bb2*da2*dc2 - 
          16*ba2*da*db*dc2 - 16*bb2*da*db*dc2 + 
          64*cc*da2*db*dc2 - 16*ba2*db2*dc2 + 
          64*cc*da*db2*dc2;

  T[6]= 16*da*db*dc*(-bc2*da*db -bb2*da*dc -ba2*db*dc + 
		     4*cc*da*db*dc);

  std::vector<double> TRange=T.realRoots(1e-10);
  if (TRange.empty())
    return -1.0;

  double Out= -1;
  Geometry::V3D xvec;
  std::vector<double>::const_iterator vc;
  for(vc=TRange.begin();vc!=TRange.end();vc++)
  {
    const double daI=1.0+2* (*vc) *da;
    const double dbI=1.0+2* (*vc) *db;
    const double dcI=1.0+2* (*vc) *dc;
    if ((daI*daI)>Tolerance || ((dbI*dbI)>Tolerance  && (dcI*dcI)<Tolerance) )
    {
      Geometry::Matrix<double> DI(3,3);
      DI[0][0]=1.0/daI;
      DI[1][1]=1.0/dbI;
      DI[2][2]=1.0/dcI;
      xvec = R*(DI*(alpha-beta* *vc));  // care here: to avoid 9*9 +9*3 in favour of 9*3+9*3 ops.
      const double Dist=xvec.distance(Pt);
      if (Out<0 || Out>Dist)
        Out=Dist;
    }
  }
  return Out;
}

int
Quadratic::onSurface(const Geometry::V3D& Pt) const
  /**
    Test to see if a point is on the surface 
    @param Pt :: Point to test
    @return 0 : if not on surface; 1 if on surace
  */
{
  const double res=eqnValue(Pt);
  return (fabs(res)>Tolerance) ? 0 : 1;
}


void
Quadratic::displace(const Geometry::V3D& Pt)
  /**
    Apply a general displacement to the surface
    @param Pt :: Point to add to surface coordinate
  */
{
  BaseEqn[9]+= Pt[0]*(Pt[0]*BaseEqn[0]-BaseEqn[6])+
               Pt[1]*(Pt[1]*BaseEqn[1]-BaseEqn[7])+
               Pt[2]*(Pt[2]*BaseEqn[2]-BaseEqn[8])+
               BaseEqn[4]*Pt[0]*Pt[1]+
               BaseEqn[5]*Pt[0]*Pt[2]+
               BaseEqn[6]*Pt[1]*Pt[2];  
  BaseEqn[6]+= -2*BaseEqn[0]*Pt[0]-BaseEqn[3]*Pt[1]-BaseEqn[4]*Pt[2];
  BaseEqn[7]+= -2*BaseEqn[1]*Pt[1]-BaseEqn[3]*Pt[0]-BaseEqn[5]*Pt[2];
  BaseEqn[8]+= -2*BaseEqn[2]*Pt[2]-BaseEqn[4]*Pt[0]-BaseEqn[5]*Pt[1];
  return;
}

void 
Quadratic::rotate(const Geometry::Matrix<double>& MX) 
  /**
    Rotate the surface by matrix MX
    @param MX :: Matrix for rotation (not inverted like MCNPX)
   */
{
  Geometry::Matrix<double> MA=MX;
  MA.Invert();
  const double a(MA[0][0]),b(MA[0][1]),c(MA[0][2]);
  const double d(MA[1][0]),e(MA[1][1]),f(MA[1][2]);
  const double g(MA[2][0]),h(MA[2][1]),j(MA[2][2]);
  double B[9];
  B[0]=BaseEqn[0]*a*a+BaseEqn[1]*d*d+BaseEqn[2]*g*g+
    BaseEqn[3]*a*d+BaseEqn[4]*a*g+BaseEqn[5]*d*g;

  B[1]=BaseEqn[0]*b*b+BaseEqn[1]*e*e+BaseEqn[2]*h*h+
    BaseEqn[3]*b*e+BaseEqn[4]*b*h+BaseEqn[5]*e*h;

  B[2]=BaseEqn[0]*c*c+BaseEqn[1]*f*f+BaseEqn[2]*j*j+
    BaseEqn[3]*c*f+BaseEqn[4]*c*j+BaseEqn[5]*f*j;

  B[3]=2.0*(BaseEqn[0]*a*b+BaseEqn[1]*d*e+BaseEqn[2]*g*h)+
    BaseEqn[3]*(a*e+b*d)+BaseEqn[4]*(a*h+b*g)+BaseEqn[5]*(d*j+e*g);

  B[4]=2.0*(BaseEqn[0]*a*c+BaseEqn[1]*d*f+BaseEqn[2]*g*j)+
    BaseEqn[3]*(a*f+c*d)+BaseEqn[4]*(a*j+c*h)+BaseEqn[5]*(d*j+f*h);

  B[5]=2.0*(BaseEqn[0]*b*c+BaseEqn[1]*e*f+BaseEqn[2]*h*j)+
    BaseEqn[3]*(b*f+c*e)+BaseEqn[4]*(b*j+c*h)+BaseEqn[5]*(e*j+f*h);

  B[6]=BaseEqn[6]*a+BaseEqn[7]*d+BaseEqn[8]*g;

  B[7]=BaseEqn[6]*b+BaseEqn[7]*e+BaseEqn[8]*h;

  B[8]=BaseEqn[6]*c+BaseEqn[7]*f+BaseEqn[8]*j;

  for(int i=0;i<9;i++)       // Item 9 left alone
    BaseEqn[i]=B[i];
  return;
}

void
Quadratic::print() const
  /** 
    Print out the genreal equation 
    for debugging.
  */
{
  Surface::print();
  for(int i=0;i<10;i++)
    std::cout<<BaseEqn[i]<<" ";
  std::cout<<std::endl;
  return;
}

void
Quadratic::write(std::ostream& OX) const
  /**
    Writes out  an MCNPX surface description 
    Note : Swap since base equation is swapped in gq output 
    (mcnp 4c manual pg. 3-14)
    @param OX :: Output stream (required for multiple std::endl)
  */
{
  std::ostringstream cx;
  writeHeader(cx);
  cx.precision(Surface::Nprecision);
  cx<<"gq ";
  for(int i=0;i<4;i++)
    cx<<" "<<BaseEqn[i]<<" ";
  cx<<" "<<BaseEqn[5]<<" ";
  cx<<" "<<BaseEqn[4]<<" ";
  for(int i=6;i<10;i++)
    cx<<" "<<BaseEqn[i]<<" ";
  Mantid::Kernel::Strings::writeMCNPX(cx.str(),OX);
  return;
}


}   // NAMESPACE Geometry

}   // NAMESPACE Mantid
