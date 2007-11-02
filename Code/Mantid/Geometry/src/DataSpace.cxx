#include <iostream>
#include <iomanip>
#include <fstream>
#include <cmath>
#include <complex>
#include <string>
#include <list>
#include <vector>
#include <algorithm>
#include <functional>
#include <boost/regex.hpp>

#include "Logger.h"
#include "support.h"
#include "mathSupport.h"
#include "DataSpace.h"

namespace Mantid
{

namespace Geometry
{

Logger& DataSpace::PLog = Logger::get("DataSpace");
DataSpace::DataSpace() :
  Title("Workspace"),Npts(0)
  /*!
    Constructor (with default title)
  */
{}

DataSpace::DataSpace(const DataSpace& A)  :
  Title(A.Title),Npts(A.Npts),
  X(A.X),Y(A.Y),Err(A.Err)
  /*!
    Copy Constructor
    \param A :: Workspace to copy
  */
{ }

DataSpace&
DataSpace::operator=(const DataSpace& A)
  /*!
    Assignment operator
    \param A :: Workspace to copy
    \return *this
  */
{
  if (&A!=this)
    {
      Title=A.Title;
      Npts=A.Npts;
      X=A.X;
      Y=A.Y;
      Err=A.Err;
    }
  return *this;
}

DataSpace::~DataSpace()
  /*!
    Destructor
  */
{}

DataSpace
DataSpace::operator+(const DataSpace& A) const
  /*!
    Addition operator (make use of the += operator)
    \param A :: Workspace to operate with 
    \return this + A
  */
{
  DataSpace Temp(*this);
  Temp+=A;
  return Temp;
}

DataSpace
DataSpace::operator-(const DataSpace& A) const
  /*!
    Subtraction operator (make use of the -= operator )
    \param A :: Workspace to operate with 
    \return this - A
  */
{
  DataSpace Temp(*this);
  Temp-=A;
  return Temp;
}


DataSpace
DataSpace::operator*(const DataSpace& A) const
  /*!
    Multiplication operator (make use of the *= operator )
    For use with Workspaces
    \param A :: Workspace to operate with 
    \return this * A
  */

{
  DataSpace Temp(*this);
  Temp*=A;
  return Temp;
}

DataSpace& 
DataSpace::operator+=(const DataSpace& A)
  /*!
    Addition operator (assumes equal points)
    \param A :: Workspace to operate with 
    \return this =+ A
  */
{
  if (A.Npts != Npts)
    {
      std::cerr<<"Points not equal in  operator+="<<std::endl;
      exit(1);
    }
  
  for(int i=0;i<A.Npts;i++)
    {  
      Y[i] += A.Y[i];
      Err[i] = sqrt(Err[i]*Err[i]+A.Err[i]*A.Err[i]);
    }
  return *this;
}

DataSpace& 
DataSpace::operator-=(const DataSpace& A)
  /*!
    Subtraction operator (assumes equal points)
    For use with Workspaces
  */
{
  if (A.Npts != Npts)
    {
      std::cerr<<"Points not equal in operator-="<<std::endl;
      exit(1);
    }
  for(int i=0;i<A.Npts;i++)
    {
      Y[i] -= A.Y[i];
      Err[i] = sqrt(Err[i]*Err[i]+A.Err[i]*A.Err[i]);
    }
  return *this;
}


DataSpace& 
DataSpace::operator+=(const double A)
  /*!
    Addition operator (single value)
    \param A :: Value to add
    \return this += A
  */
{
  std::vector<double>::iterator vc;
  for(vc=Y.begin();vc!=Y.end();vc++)
    (*vc)+=A;
  return *this;
}

DataSpace& 
DataSpace::operator-=(const double A)
  /*!
    Subtraction operator (single value)
    \param A :: Value to subtract
    \return this -= A
  */
{
  std::vector<double>::iterator vc;
  for(vc=Y.begin();vc!=Y.end();vc++)
    (*vc)-=A;
  return *this;
}

DataSpace& 
DataSpace::operator*=(const DataSpace& A)
  /*!
    Multiplication operator 
    \param A :: Value to multiply 
    \return this *= A

  */
{
  if (A.Npts != Npts)
    {
      std::cerr<<"Points not equal in multiply operator"<<std::endl;
      exit(1);
    }
  for(int i=0;i<A.Npts;i++)
    {
      Err[i] = sqrt(A.Y[i]*A.Y[i]*Err[i]*Err[i]+
		    Y[i]*Y[i]*A.Err[i]*A.Err[i]);
      Y[i] *= A.Y[i];
    }
  return *this;
}

DataSpace& 
DataSpace::operator*=(const double A)
  /*!
    Multiplication operator (single value)
    \param A :: Value to multiply 
    \return this *= A
  */
{
  for(int i=0;i<Npts;i++)
    {
      Y[i] *= A;
      Err[i] *= A;
    }
  return *this;
}

DataSpace& 
DataSpace::operator/=(const double A)
  /*!
    Division operator (single value) (with check for zero)
    \param A :: Value to divide by
    \return this *= A

  */
{
  if (A!=0.0)
    {
      for(int i=0;i<Npts;i++)
        {
	  Y[i] /= A;
	  Err[i] /= A;
	}
    }
  else
    {
      for(int i=0;i<Npts;i++)
        {
	  Y[i] =0.0;
	  Err[i] = -1.0;
	}
    }
  return *this;
}

DataSpace& 
DataSpace::operator/=(const DataSpace& A)
{
  if (A.Npts != Npts)
    {
      std::cerr<<"Points not equal in division operator"<<std::endl;
      exit(1);
    }
  for(int i=0;i<A.Npts;i++)
    {
      if (A.Y[i])
      {
	Y[i] /= A.Y[i];
	// Note: using yval[i] after division.
	Err[i] = sqrt(Err[i]*Err[i] + Y[i]*Y[i] *
		      A.Err[i]*A.Err[i])/A.Y[i];
      }
      else
        {
	  Y[i]=0.0;           // mark point as worthless
	  Err[i]=-1;         
	}
    }
  return *this;
}

void
DataSpace::convAngleToQ(const double lambda)
  /*! 
     This function converts Angle to Q.
     It currently assumes that the data is angular.
     \param Lambda :: wavelength 
   */
{
  const double four_pi(4*M_PI);
  const double degToRad(90.0/M_PI);    // note: convert to theta/2
  std::vector<double>::iterator vc;
  for(vc=X.begin();vc!=X.end();vc++)
    (*vc)=four_pi*sin(*vc * degToRad) / lambda;
  return;
}

int
DataSpace::write(const std::string& Fname,const int append) const
  /*!
    Function to write out the object.
    This overload is used as an initialier for write(Fstream)
    \param Fname : File name
    \param append : append to a previous file
    \returns -ve on error 0 on success
  */
{
  std::ofstream of;
  if (append)
    of.open(Fname.c_str(),std::ios::out|std::ios::app);
  else
    of.open(Fname.c_str(),std::ios::out);
  if (!of)
    {
      std::cerr<<"Error writing file "<<Fname<<std::endl;
      return -1;
    }
  const int rtval=write(of);
  of.close();
  return rtval;
}

int
DataSpace::write(std::ostream& OX) const
  /*!
    Function to write out the object.
    This overload is used as an initialier
    \returns -ve on error 0 on success. 
    (currently state of the the file stream)
  */
{
  OX<<" 3 "<<Npts<<std::endl;
  OX<<"# "<<Title<<std::endl;
  for(int i=0;i<Npts;i++)
    OX<<X[i]<<" "<<Y[i]<<" "<<Err[i]<<std::endl;
  return (OX.good()) ? 0 : -2;
}

int 
DataSpace::readFour(const std::string& Fn)
  /*! 
     Function to read in the experimental data file.
     The type is assumed to be Fourbris
     \retval 0 :: on success:
     \retval 1 :: The file can't be found
     \retval -1 :: There are no valid points in the file 
     File format is   
     - title line 
     - X , Y  {Error} 
  */
{
  std::ifstream ipf;
  ipf.open(Fn.c_str());
  if (!ipf) 
    {
      std::cerr<<"Unable to open file "<<Fn<<std::endl;
      return 1;
    }

  int a,b;
  double spc(1.0);
  std::string SLine=StrFunc::getLine(ipf,512);
  int flag=StrFunc::section(SLine,a);     // type :: number of points :: space 
  flag+=StrFunc::section(SLine,b);
  flag+=StrFunc::section(SLine,spc);

  // Three types of file :
  // basic (npts ; file)
  // type 1: (y value)
  // type 2: (y, e)
  // type 3: (x, y, e)

  int nCnt=0;
  int type=0;

  if (flag==1 || a>10)         // number of points only
     {
       nCnt=a;
       type=3;
     }
  else if (flag>1)
    {
      nCnt=b;
      type=a;
    }
  
  if (nCnt<=0 || (type==1 && flag<=2))
    {
      std::cerr<<"No point to read"<<std::endl;
      return -1;
    }

  int cnt=0;
  // initialise to zero so can use later 
  // if not set.
  double xv[3]={ 0,0,0 };

  X.clear();
  Y.clear();
  Err.clear();
  X.resize(nCnt);
  Y.resize(nCnt);
  Err.resize(nCnt);

  while(cnt<nCnt && ipf)
    {
      SLine=StrFunc::getLine(ipf,255);
      StrFunc::stripComment(SLine);
      if (!StrFunc::isEmpty(SLine))
	{
	  int iRead(type);          //number of components read
	  int firstLine=1;
	  while(iRead==type && cnt<nCnt)
	    {
	      for(iRead=0;iRead<type && 
		    StrFunc::section(SLine,xv[iRead]);iRead++);
	      if (iRead!=type && cnt && firstLine)
	        {
		  std::cerr<<"Error with file :: Read "<<cnt<<" pts "<<std::endl;
		  return -2;
		}
	      firstLine=0;
	      if (iRead==type)   //did we read enought points
	        {
		  if (type==3)
		    {
		      X[cnt]=xv[0];
		      Y[cnt]=xv[1];
		      Err[cnt]=(fabs(xv[2])>1e-30) ? xv[2] : 1.0;
		    }
		  else
		    {
		      X[cnt]=cnt*spc;
		      Y[cnt]= xv[0]; 
		      Err[cnt]=(type==2 && fabs(xv[1])>1e-30) ? xv[1] : 1.0;
		    }
		  cnt++;
		}
	    }
	}
    }
  Npts=cnt;
  ipf.close(); 
  return 0;
}

int
DataSpace::removeZeroX() 
  /*!
    Removes those points below Zero 
    \returns number of points removed 
  */
{
  unsigned int i;
  for(i=0;i<X.size() && X[i]<=0.0;i++);
  if (!i)
    return 0;
  X.erase(X.begin(),X.begin()+i);
  Y.erase(Y.begin(),Y.begin()+i);
  Err.erase(Err.begin(),Err.begin()+i);
  Npts-=i;
  return i;
}

void
DataSpace::setLevel(const int pts,const double step,
		    const double val,const double rmin)
  /*!
    Give a number of points and a step
    sets the the whole array to the value
    and the error to zero.
    \param pts :: number of point in the new array
    \param step :: X coordinate step
    \param val :: value to set the Y axis
   */
{
  X.clear();
  Y.clear();
  Err.clear();
  X.resize(pts);
  Y.resize(pts);
  Err.resize(pts);
  double xv;
  for(int i=0;i<pts;i++)
    {
      xv=(i+0.5)*step;
      X[i]=xv;
      if (xv>rmin || rmin<0.0)
	Y[i]=0.0;
      else
	Y[i]= -val;
      Err[i]=0.0;
    }
  Npts=(pts>0) ? pts : 0;
  return;
}


DataSpace&
DataSpace::rebin(const double Xmin,const double Xmax,
	      const double step)
  /*!
    Given input range rebin with statistical sampling 
  */
{
  const int MaxN=static_cast<int>((Xmax-Xmin)/step)+1;

  std::vector<double> X3(MaxN);
  std::vector<double> Y3(MaxN);
  std::vector<double> E3(MaxN);

  int i(0); // points in main chain

  double qlast=0.0;
  
  for(int k=0;k<MaxN;k++)
    {
      X3.push_back(Xmin+k*step);
      double xmin=(k) ? X3[k-1] : Xmin-step;
      double xmax=X3[k];
      double value=0.0;
      double evalue=0.0;
      
      while (i!=Npts && X[i]<xmax)
	{
	  if (X[i]>xmin)
	    {
	      double qstep=(qlast>xmin) ? X[i]-qlast : X[i]-xmin;
	      qstep/=(X[i]-qlast);
	      value+=Y[i]*qstep;
	      evalue+=Err[i]*Err[i]*qstep*qstep;
	    }
	  qlast=X[i];
	  i++;
	}
      // Need something here to catch part left over on loop
      Y3.push_back(value);
      E3.push_back(sqrt(evalue));
    }
  //Now delete the old and replace with A3...
  Npts=MaxN;
  X=X3;
  Y=Y3;
  Err=E3;
  return *this;
}

double
DataSpace::calcFitBasic(const DataSpace& A) const
  /*! 
     Calculates a simple fit (assume both are equal in X)
     Uses this-> Error and not A error
     /returns Chi^2
  */
{
  const int pts=(A.Npts<Npts) ? A.Npts : Npts;
  double chi(0.0);
  double ty;
  for(int i=0;i<pts;i++)
    {
      ty=(A.Y[i]-Y[i]);
      chi+=(ty*ty)/(Err[i]*Err[i]);
    }
  return chi;
}

double
DataSpace::calcFitBasic(const DataSpace& A,std::vector<double>& FitPts) const
  /*! 
     Calculates a simple fit (assume both are equal in X)
     Uses this-> Error and not A error
     /returns Chi^2
  */
{
  const int pts=(A.Npts<Npts) ? A.Npts : Npts;
  FitPts.resize(pts);
  double chi(0.0);
  double ty;
  for(int i=0;i<pts;i++)
    {
      ty=(A.Y[i]-Y[i]);
      chi+=(ty*ty)/(Err[i]*Err[i]);
      FitPts[i]=(ty*ty)/(Err[i]*Err[i]);
    }
  return chi;
}

double
DataSpace::calcEntropy() const 
  /*!
    Calulates \f[ \sum_i \lbrace \Delta^_j/R_j -1/2 R_j < \Delta_j < 1/2 R_j 
    \f]
  */
{
  double Psum(0.0);
  for(int i=1;i<Npts-1;i++)
    {
      const double Rj=0.5*fabs(Y[i+1]-Y[i-1]);
      const double Pj=0.25*(Y[i+1]+2.0*Y[i]+Y[i-1]);
      const double Dj=fabs(Y[i]-Pj);
      
      if (Dj<0.5*Rj)
	Psum+=(Dj*Dj)/Rj;
      else
	Psum+=Dj;
    }
  return Psum;
}


double
DataSpace::calcEntropy(std::vector<double>& EntPts) const 
  /*!
    Calulates \f[ \sum_i \lbrace \Delta^_j/R_j -1/2 R_j < \Delta_j < 1/2 R_j 
    \f]
    \param EntPts :: output for each point
  */
{
  double Psum(0.0);
  EntPts.resize(Npts);
  EntPts[0]=0.0;
  EntPts[Npts-1]=0.0;
  for(int i=1;i<Npts-1;i++)
    {
      const double Rj=0.5*fabs(Y[i+1]-Y[i-1]);
      const double Pj=0.25*(Y[i+1]+2.0*Y[i]+Y[i-1]);
      const double Dj=fabs(Y[i]-Pj);
      
      if (Dj<0.5*Rj)
	{
	  Psum+=(Dj*Dj)/Rj;
	  EntPts[i]=(Dj*Dj)/Rj;
	}
      else
	{
	  Psum+=Dj;
	  EntPts[i]=Dj;
	}
    }
  return Psum;
}

void
DataSpace::calcTrans(const DataSpace& B,const double factor)
  /*!
    Calculates a full F.T. for the g(r) or 
    S(Q). (with the factor taking care of
    the g(r)->s(q) conversion.
    This workspace is used to get the mesh to calculate
    the F.T. over
    \param B :: Workspace containing S(q) or G(r) 
    \param factor :: multiplicative factor
    Factor should be 
    - \f[ \frac{1}{4 \pi \rho}  \f]  (r -> Q)
    - \f[ 2 \pi^2 \rho  \f]  (Q -> r) 
  */
{

  for(int i=0;i<Npts;i++)
    Y[i]=0.0;

  double Astep;   // step size
  
  for(int i=0;i<B.Npts;i++)      //loop over S(Q) 
    {
      if (B.Y[i]!=0.0)
	{
	  // get step size
	  if (i==0)
	    Astep=0.5*(B.X[1]-B.X[0]);
	  else if (i==Npts-1)
	    Astep=0.5*(B.X[i]-B.X[i-1]);
	  else
	    Astep=(B.X[i+1]-B.X[i-1])/2.0;
	  const double Q(B.X[i]);
	  for(int j=0;j<Npts;j++)
	    {
	      double Qr = Q*X[j];
	      Qr = sin(Qr);
	      Y[j]+=Q * B.Y[i] * Qr * Astep;      // write out G(r)
	    }
	}
    }
  for(int i=0;i<Npts;i++)
    Y[i]/=X[i]*factor;
  return;
}

} // NAMESPACE MonteCarlo

}  // NAMESPACE Mantid
