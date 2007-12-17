#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <complex>
#include <cmath>
#include <list>
#include <vector>
#include <map>
#include <stack>
#include <string>
#include <algorithm>
#include <boost/regex.hpp>
#include <gsl/gsl_poly.h>

#include "AuxException.h"
#include "MantidKernel/Logger.h"
#include "XMLattribute.h"
#include "XMLobject.h"
#include "XMLgroup.h"
#include "XMLread.h" 
#include "XMLcollect.h" 
#include "IndexIterator.h"
#include "MantidKernel/Support.h"
#include "regexSupport.h"
#include "Matrix.h"
#include "Vec3D.h"
#include "BaseVisit.h"
#include "Surface.h"

namespace Mantid
{

namespace Geometry
{

Kernel::Logger& Surface::PLog = Kernel::Logger::get("Surface"); 

/// @cond
const double STolerance(1e-6);
/// @endcond

Surface::Surface() : 
  Name(-1),BaseEqn(10)
  /*!
    Constructor
  */
{}

Surface::Surface(const Surface& A) : 
  Name(A.Name),
  BaseEqn(A.BaseEqn)
  /*!
    Copy constructor
    \param A :: Surface to copy
  */
{ }


Surface&
Surface::operator=(const Surface& A)
  /*!
    Assignment operator
    \param A :: Surface to copy
    \return *this
  */
{
  if (this!=&A)
    {
      Name=A.Name;
      BaseEqn.resize(10);
      copy(A.BaseEqn.begin(),A.BaseEqn.end(),BaseEqn.begin());
    }
  return *this;
}

Surface::~Surface()
  /*!
    Destructor
  */
{}

void 
Surface::print() const
  /*!
    Simple print out function for surface header 
  */
{
  std::cout<<"Surf (name,trans) == "<<Name<<std::endl;
  return;
}

double 
Surface::eqnValue(const Geometry::Vec3D& Pt) const
  /*!
    Helper function to calcuate the value
    of the equation at a fixed point 
    \param Pt :: Point to determine the equation surface 
    value at
    \return value Eqn(Pt) : -ve inside +ve outside
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
Surface::side(const Geometry::Vec3D& Pt) const
  /*!
    Determine if the the Point is true to the surface or
    on the other side
    \param Pt :: Point to check
    \retval 1 : if the point is "true" to the surface  
    \retval -1 : if the point is "false" to the surface  
    \retval 0 :: The point is on the surface
  */
{
  double res=eqnValue(Pt);
  if (fabs(res)<STolerance)
    return 0;
  return (res>0) ? 1 : -1;
}


Geometry::Vec3D
Surface::surfaceNormal(const Geometry::Vec3D& Pt) const
  /*!
    Given a point on the surface 
    Calculate the normal at the point 
    Some rather disturbing behaviour happens if 
    the point is a significant distance from the surface
    \param Pt :: Point to calcution
    \return normal unit vector
  */
{
   Geometry::Vec3D N(2*BaseEqn[0]*Pt[0]+BaseEqn[3]*Pt[1]+BaseEqn[4]*Pt[2]+BaseEqn[6],
	   2*BaseEqn[1]*Pt[1]+BaseEqn[3]*Pt[0]+BaseEqn[5]*Pt[2]+BaseEqn[7],
	   2*BaseEqn[2]*Pt[2]+BaseEqn[4]*Pt[0]+BaseEqn[5]*Pt[1]+BaseEqn[8]);
   N.makeUnit();
   return N;
}

void
Surface::matrixForm(Geometry::Matrix<double>& A,Geometry::Vec3D& B,double& C) const
  /*!
    Converts the baseEqn into the matrix form such that
    \f[ x^T A x + B^T x + C =0 \f]
    \param A :: Matrix to place equation into
    \param B :: Vector point 
    \param C :: Constant value
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
Surface::distanceTrue(const Geometry::Vec3D& Pt) const
  /*!
    Proper calcuation of a point to a general surface 
    \param Pt :: Point to calculate distance from surace
    \return distance from point to surface (signed)
  */
{
  // Job 1 :: Create matrix and vector representation
  Geometry::Matrix<double> A(3,3);
  Geometry::Vec3D B;
  double C;
  matrixForm(A,B,C);
  
  //Job 2 :: calculate the diagonal matrix
  Geometry::Matrix<double> D(3,3);
  Geometry::Matrix<double> R(3,3);
  if (!A.Diagonalise(R,D))
    {
      std::cerr<<"Problem with matrix :: distance now guessed at"<<std::endl;
      return distance(Pt);
    }
  Geometry::Matrix<double> Rt(R);
  Rt.Transpose();
  Geometry::Vec3D alpha=Rt*Pt;
  Geometry::Vec3D beta=Rt*B;
    
  // Calculate fundermental equation:
  double T[7];
  const double aa(alpha[0]);   const double aa2(aa*aa);
  const double ab(alpha[1]);   const double ab2(aa*aa);
  const double ac(alpha[2]);   const double ac2(aa*aa);

  const double ba(beta[0]);   const double ba2(ba*ba);
  const double bb(beta[1]);   const double bb2(bb*bb);
  const double bc(beta[2]);   const double bc2(bc*bc);

  const double da(D[0][0]);   const double da2(da*da);
  const double db(D[1][3]);   const double db2(db*db);
  const double dc(D[2][2]);   const double dc2(dc*dc);


  T[0]=C+ aa*ba + ab*bb + ac*bc + aa2*da + ab2*db +  ac2*dc;

  T[1]= -ba2-bb2-bc2 + 4*ab*bb*da + 4*ac*bc*da + 4*aa*ba*db + 4*ac*bc*db + 
            4*aa2*da*db + 4*ab2*da*db + 4*aa*ba*dc +  4*ab*bb*dc + 4*aa*da*dc + 
            4*ac*da*dc + 4*ab*db*dc + 4*ac*db*dc;
  T[2]=  -ba2*da - 4*bb2*da - 4*bc2*da + 4*ab*bb*da2 + 
            4*ac*bc*da2 - 4*ba2*db - bb2*db - 4*bc*db + 
            16*ac*bc*da*db + 4*ab2*da2*db + 4*aa*ba*db2 + 
            4*ac*bc*db2 + 4*aa2*da*db2 - 4*ba2*dc - 
            4*bb2*dc - bc2*dc + 16*ab*bb*da*dc + 
            4*ac2*da2*dc + 16*aa*ba*db*dc + 
            16*aa2*da*db*dc + 16*ab2*da*db*dc + 
            16*ac2*da*db*dc + 4*ac2*db2*dc + 4*aa*ba*dc2 + 
            4*ab*bb*dc2 + 4*aa2*da*dc2 + 
            4*ab2*db*dc2;

  T[3]=   -4*bb2*da2 - 4*bc2*da2 - 4*ba2*da*db - 4*bb2*da*db - 
            16*bc2*da*db + 16*ac*bc*da2*db - 4*ba2*db2 - 
            4*bc2*db2 + 16*ac*bc*da*db2 - 4*ba2*da*dc - 
            16*bb2*da*dc - 4*bc2*da*dc + 16*ab*bb*da2*dc - 
            16*ba2*db*dc - 4*bb2*db*dc - 4*bc2*db*dc + 
            16*ab2*da2*db*dc + 16*ac2*da2*db*dc + 
            16*aa*ba*db2*dc + 16*aa2*da*db2*dc + 
            16*ac2*da*db2*dc - 4*ba2*dc2 - 4*bb2*dc2 + 
            16*ab*bb*da*dc2 + 16*aa*ba*db*dc2 + 
            16*aa2*da*db*dc2 + 
            16*ab2*da*db*dc2;
  
  T[4]=   -4*bb2*da2*db - 16*bc2*da2*db - 4*ba2*da*db2 - 
            16*bc2*da*db2 + 16*ac*bc*da2*db2 - 
            16*bb2*da2*dc - 4*bc2*da2*dc - 
            16*ba2*da*db*dc - 16*bb2*da*db*dc - 
            16*bc2*da*db*dc - 16*ba2*db2*dc - 
            4*bc2*db2*dc + 16*ac2*da2*db2*dc - 
            4*ba2*da*dc2 - 16*bb2*da*dc2 + 
            16*ab*bb*da2*dc2 - 16*ba2*db*dc2 - 
            4*bb2*db*dc2 + 16*ab2*da2*db*dc2 + 
            16*aa*ba*db2*dc2 +  16*aa2*da*db2*dc2;
  
  T[5]=-16*(bc2*da2*db2 + bb2*da2*db*dc + bc2*da2*db*dc + 
            ba2*da*db2*dc + bc2*da*db2*dc +
            bb2*da2*dc2 + ba2*da*db*dc2 + 
            bb2*da*db*dc2 + ba2*db2*dc2);
  
  T[6]= -16*(bc2*da2*db2*dc + bb2*da2*db*dc2 +ba2*da*db2*dc2);

  // Solve main equation:
  gsl_poly_complex_workspace* WS= gsl_poly_complex_workspace_alloc(7);
  double Z[12];
  gsl_poly_complex_solve (T, 7, WS, Z);
  gsl_poly_complex_workspace_free (WS);

  double Out(1e38);
  int index(-1);
  for(int i=0;i<12;i+=2)
    {
      if (fabs(Z[i])<Out && fabs(Z[i+1])<STolerance)  // Non-complex
        {
	  index=i;
	  Out=fabs(Z[i]);
	}
    }
  if (index<0)
    return -1.0;
  
  Matrix<double> DI(3,3);
  DI[0][0]=1.0+2*Out*da;
  DI[1][1]=1.0+2*Out*db;
  DI[2][2]=1.0+2*Out*dc;
  Vec3D xvec = (R*DI) * (alpha-beta*Out);
  return xvec.Distance(Pt);
}

double
Surface::distance(const Geometry::Vec3D& Pt) const
  /*!
    Calculate distance from a point to a general surface 
    Approximate form (but quick)
    \param Pt :: Pont to calculate from
    \return Distance
  */
{
  double res=eqnValue(Pt);
  // This is not the true normal since evaluated at P not at intersect 
  Geometry::Vec3D normSur(2*BaseEqn[0]*Pt[0]+BaseEqn[3]*Pt[1]+BaseEqn[4]*Pt[2]+BaseEqn[6],
		2*BaseEqn[1]*Pt[1]+BaseEqn[3]*Pt[0]+BaseEqn[5]*Pt[2]+BaseEqn[7],
		2*BaseEqn[2]*Pt[2]+BaseEqn[4]*Pt[0]+BaseEqn[5]*Pt[1]+BaseEqn[8]);
  return res/(normSur.abs());
}

int
Surface::onSurface(const Geometry::Vec3D& Pt) const
  /*!
    Test to see if a point is on the surface 
    \param Pt :: Point to test
    \returns 0 : if not on surface; 1 if on surace
  */
{
  double res=eqnValue(Pt);
  return (fabs(res)>STolerance) ? 0 : 1;
}


void
Surface::displace(const Geometry::Vec3D& Pt)
  /*!
    Apply a general displacement to the surface
    \param Pt :: Point to add to surface coordinate
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
Surface::rotate(const Geometry::Matrix<double>& MX) 
  /*!
    Rotate the surface by matrix MX
    \param MX :: Matrix for rotation (not inverted like MCNPX)
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
Surface::printGeneral() const
  /*! 
    Print out the genreal equation 
    for debugging.
  */
{
  std::cout<<Name<<" : ";
  for(int i=0;i<10;i++)
    {
      std::cout<<BaseEqn[i]<<" ";
    }
  std::cout<<std::endl;
  return;
}

void
Surface::writeHeader(std::ostream& OX) const
  /*!
    Writes out the start of an MCNPX surface description .
    Does not check the length etc
    \param OX :: Output stream
  */
{
  OX<<Name<<" ";
  return;
}
  

void
Surface::write(std::ostream& OX) const
  /*!
    Writes out  an MCNPX surface description 
    \param OX :: Output stream (required for multiple std::endl)
  */
{
  std::ostringstream cx;
  writeHeader(cx);
  cx.precision(Surface::Nprecision);
  cx<<"gq ";
  for(int i=0;i<10;i++)
    cx<<" "<<BaseEqn[i]<<" ";
  StrFunc::writeMCNPX(cx.str(),OX);
  return;
}

int
Surface::importXML(IndexIterator<XML::XMLobject,XML::XMLgroup>& SK,
		   const int singleFlag)
  /*!
    Given a distribution that has been put into an XML base set.
    The XMLcollection need to have the XMLgroup pointing to
    the section for this Surface. 

    \param SK :: IndexIterator scheme
    \param singleFlag :: Single pass identifer [expected 1 ]
    \retval 0 :: success
   */
{
  int errCnt(0);
  int levelExit(SK.getLevel());
  do
    {
      if (*SK)
        {
	  int errNum(1);
	  const std::string& KVal= SK->getKey();
//	  const std::string KBase= SK->getKeyBase();
//	  const int Knum =  SK->getKeyNum();
	  const XML::XMLread* RPtr=dynamic_cast<const XML::XMLread*>(*SK);
	  
	  if (RPtr)
	    {
	      if (KVal=="Name")
		errNum=1-StrFunc::convert(RPtr->getFront(),Name);
	      else if (KVal=="BaseEqn")
		errNum=RPtr->convertToContainer(BaseEqn);
	    }
	  // Failure test:
	  if (errNum)
	    {
	      PLog.warning("importXML :: Key failed "+KVal);
	      errCnt++;
	    }
	}
      if (!singleFlag) SK++;
    } while (!singleFlag && SK.getLevel()>=levelExit);
  
  return errCnt;
}

void
Surface::procXML(XML::XMLcollect& XOut) const
  /*!
    This writes the XML schema
    \param XOut :: Output parameter
   */
{
  XOut.addComp("Name",Name);
  XOut.addComp("BaseEqn",BaseEqn);
  return;
}

void
Surface::writeXML(const std::string& Fname) const
  /*!
    The generic XML writing system.
    This should call the virtual function procXML
    to correctly built the XMLcollection.
    \param Fname :: Filename 
  */
{
  XML::XMLcollect XOut;
  XOut.addGrp("Surface");
  this->procXML(XOut);          
  XOut.closeGrp();
  std::ofstream OX;
  OX.open(Fname.c_str());
  XOut.writeXML(OX);
  return;
}
  
}  // NAMESPACE Geometry

} // NAMESPACE Mantid
