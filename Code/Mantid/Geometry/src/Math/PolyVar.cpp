#include <iostream>
#include <cmath>
#include <complex>
#include <vector>
#include <algorithm>
#include <iterator>
#include <functional>
#include <gsl/gsl_poly.h>


#include "MantidKernel/Exception.h"
#include "MantidKernel/Support.h"
#include "MantidGeometry/Math/MatrixBase.h"
#include "MantidGeometry/Math/PolyVar.h"


namespace Mantid
{

  namespace mathLevel   
  {

    template<int VCount>
    std::ostream& 
      operator<<(std::ostream& OX,const PolyVar<VCount>& A)
      /*!
      External Friend :: outputs point to a stream 
      \param OX :: output stream
      \param A :: PolyFunction to write
      \returns The output stream (OX)
      */
    {
      if (!A.write(OX))
        OX<<0;
      return OX;
    }

    template<int VCount>
    PolyVar<VCount>::PolyVar(const int iD) : 
    PolyFunction(),
      iDegree((iD>0) ? iD : 0),
      PCoeff(iDegree+1)
      /*!
      Constructor 
      \param iD :: degree
      */
    {
      fill(PCoeff.begin(),PCoeff.end(),PolyVar<VCount-1>(0));
    }

    template<int VCount>
    PolyVar<VCount>::PolyVar(const int iD,const double E) : 
    PolyFunction(E),
      iDegree((iD>0) ? iD : 0),
      PCoeff(iDegree+1)
      /*!
      Constructor 
      \param iD :: degree
      \param E :: Accuracy
      */
    {
      fill(PCoeff.begin(),PCoeff.end(),PolyVar<VCount-1>(0,E));
    }

    template<int VCount>
    PolyVar<VCount>::PolyVar(const PolyVar<VCount>& A)  :
    PolyFunction(A),iDegree(A.iDegree),
      PCoeff(A.PCoeff)
      /*! 
      Copy Constructor
      \param A :: PolyVar to copy
      */
    { }

    template<int VCount>
    template<int ICount>
    PolyVar<VCount>::PolyVar(const PolyVar<ICount>& A) :
    iDegree(0),PCoeff(1)
      /*! 
      Copy constructor to a down reference
      \param A :: PolyVar to copy assign as PCoeff[0]
      */
    {
      if (ICount>VCount)
      {
        std::cerr<<"ERROR WITH ICOUNT"<<std::endl;
        exit(1);
      }

      PCoeff[0]=A;
    }

    template<int VCount>
    PolyVar<VCount>& 
      PolyVar<VCount>::operator=(const PolyVar<VCount>& A)
      /*! 
      Assignment operator
      \param A :: PolyVar to copy
      \return *this
      */
    {
      if (this!=&A)
      {
        PolyFunction::operator=(A);
        iDegree = A.iDegree;
        PCoeff=A.PCoeff;
      }
      return *this;
    }

    template<int VCount>
    template<int ICount>
    PolyVar<VCount>& 
      PolyVar<VCount>::operator=(const PolyVar<ICount>& A)
      /*! 
      Assignment operator
      \param A :: PolyVar to copy
      \return *this
      */
    {
      iDegree = 0;
      PCoeff.resize(1);
      PCoeff[0]=A;
      return *this;
    }

    template<int VCount>
    PolyVar<VCount>& 
      PolyVar<VCount>::operator=(const double& V)
      /*! 
      Assignment operator
      \param V :: Double value to set
      \return *this
      */
    {
      iDegree = 0;
      PCoeff.resize(1);
      PCoeff[0]=V;
      return *this;
    }

    template<int VCount> 
    PolyVar<VCount>::~PolyVar()
      /// Destructor
    {}

    template<int VCount>
    void 
      PolyVar<VCount>::setDegree(const int iD)
      /*!
      Set the degree value (assuming that we have
      a valid iDegree setup)
      \param iD :: degree
      */
    {
      const int xD = (iD>0) ? iD : 0;
      if (xD>iDegree)
      {
        for(int i=iDegree;i<xD;i++)
          PCoeff.push_back(PolyVar<VCount-1>(0,this->Eaccuracy));
      }
      else   // reduction in power 
        PCoeff.resize(xD+1);

      iDegree=xD;
      return;
    }

    template<int VCount>
    void 
      PolyVar<VCount>::zeroPoly()
      /*!
      Zeros each polynominal coefficient
      */
    {
      for(int i=0;i<=iDegree;i++)
      {
        PCoeff[i]=PolyVar<VCount-1>(0,this->Eaccuracy);
      }
      return;
    }

    template<int VCount>
    int 
      PolyVar<VCount>::getDegree() const 
      /*!
      Accessor to degree size
      \return size 
      */
    {
      return iDegree;
    }

    template<int VCount>
    void
      PolyVar<VCount>::setComp(const int Index,const double& V)
      /*!
      Set a component
      \param Index :: The Index
      \param V :: Value
      */
    {
      if (Index>iDegree || Index<0)
        throw Kernel::Exception::IndexError(Index,iDegree+1,"PolyVar::setComp(double)");
      PCoeff[Index] = V;
      return;  
    }

    template<int VCount>
    template<int ICount>
    void
      PolyVar<VCount>::setComp(const int Index,const PolyVar<ICount>& FX)
      /*!
      Set a component
      \param Index :: The index
      \param FX :: Base compoenente
      */
    {
      if (Index>iDegree || Index<0)
        throw Kernel::Exception::IndexError(Index,iDegree+1,"PolyVar::setComp(PolyBase)");
      PCoeff[Index] = FX;
      return;
    }

          /*!
      Calculate the value of the polynomial at a point
      \param DArray :: Values [x,y,z]
      \return the value
      */
    template<int VCount>
    double PolyVar<VCount>::operator()(const double* DArray) const
    {
      double X(1.0);
      double sum(0.0);
      for(int i=0;i<=iDegree;i++)
      {
        sum+=PCoeff[i](DArray)*X;
        X*=DArray[VCount-1];
      }
      return sum;
    }

    template<int VCount>
    double 
      PolyVar<VCount>::operator()(const std::vector<double>& DArray) const
      /*!
      Calculate the value of the polynomial at a point
      \param DArray :: Values [x,y,z]
      \return results
      */
    {
      if (DArray.size()<VCount)
        throw Kernel::Exception::IndexError(DArray.size(),VCount,"PolVar::operator()");
      double X(1.0);
      double sum(0.0);
      for(int i=0;i<=iDegree;i++)
      {
        sum+=PCoeff[i](DArray)*X;
        X*=DArray[VCount-1];
      }
      return sum;
    }


    template<int VCount>
    PolyVar<VCount>&
      PolyVar<VCount>::operator+=(const PolyVar<VCount>& A)
      /*!
      PolyVar addition
      \param A :: PolyVar 
      \return (*this+A);
      */
    {
      const int iMax((iDegree>A.iDegree)  ? iDegree : A.iDegree);
      PCoeff.resize(iMax+1);

      for(int i=0;i<=A.iDegree;i++)
        PCoeff[i]+=A.PCoeff[i];
      iDegree=iMax;
      return *this;
    }
    template<int VCount>
    PolyVar<VCount>&
      PolyVar<VCount>::operator-=(const PolyVar<VCount>& A)
      /*!
      PolyVar subtraction
      \param A :: PolyVar 
      \return (*this-A);
      */
    {
      const int iMax((iDegree>A.iDegree)  ? iDegree : A.iDegree);
      PCoeff.resize(iMax+1);

      for(int i=0;i<=A.iDegree;i++)
        PCoeff[i]-=A.PCoeff[i];
      iDegree=iMax;
      return *this;
    }

    template<int VCount>
    PolyVar<VCount>&
      PolyVar<VCount>::operator*=(const PolyVar<VCount>& A)
      /*!
      Multiply two Polynomials
      \param A :: PolyVar 
      \return (*this*A);
      */
    {
      std::vector<PolyVar<VCount-1> > POut(iDegree+A.iDegree+2); 
      std::vector<int> Zero(A.iDegree+1);
      transform(A.PCoeff.begin(),A.PCoeff.end(),Zero.begin(),
        std::bind2nd(std::mem_fun_ref(&PolyVar<VCount-1>::isZero),this->Eaccuracy));

      for(int i=0;i<=iDegree;i++)
      {
        // Cheaper than the calls to operator*
        if (!PCoeff[i].isZero(this->Eaccuracy)) 
        {
          for(int j=0;j<=A.iDegree;j++)
            if (!Zero[j])
            {
              POut[i+j]+= PCoeff[i]*A.PCoeff[j];
            }
        }
      }

      PCoeff=POut;
      iDegree=iDegree+A.iDegree+1;
      compress();  
      return *this;
    }

    template<int VCount>
    PolyVar<VCount> 
      PolyVar<VCount>::operator+(const PolyVar<VCount>& A) const
      /*!
      PolyVar addition
      \param A :: PolyVar addition
      \return (*this+A);
      */
    {
      PolyVar<VCount> kSum(*this);
      return kSum+=A;
    }

    template<int VCount>
    PolyVar<VCount> 
      PolyVar<VCount>::operator-(const PolyVar<VCount>& A) const
      /*!
      PolyVar subtraction
      \param A :: PolyVar 
      \return (*this-A);
      */
    {
      PolyVar<VCount> kSum(*this);
      return kSum-=A;
    }

    template<int VCount>
    PolyVar<VCount> 
      PolyVar<VCount>::operator*(const PolyVar<VCount>& A) const
      /*!
      PolyVar multiplication
      \param A :: PolyVar 
      \return (*this*A);
      */
    {
      PolyVar<VCount> kSum(*this);
      return kSum*=A;
    }


    // SCALAR BASICS

    template<int VCount>
    PolyVar<VCount> 
      PolyVar<VCount>::operator+(const double V) const
      /*!
      PolyVar addition
      \param V :: PolyVar 
      \return (*this+A);
      */
    {
      PolyVar<VCount> kSum(*this);
      return kSum+=V;
    }

    template<int VCount>
    PolyVar<VCount> 
      PolyVar<VCount>::operator-(const double V) const
      /*!
      PolyVar substraction
      \param V :: Value to subtract
      \return (*this-A);
      */
    {
      PolyVar<VCount> kSum(*this);
      return kSum-=V;
    }

    template<int VCount>
    PolyVar<VCount>
      PolyVar<VCount>::operator*(const double V) const
      /*!
      PolyVar multiplication
      \param V :: Value to Multiply
      \return (*this*V);
      */
    {
      PolyVar<VCount> kSum(*this);
      return kSum*=V;
    }

    template<int VCount>
    PolyVar<VCount>
      PolyVar<VCount>::operator/(const double V) const
      /*!
      PolyVar division
      \param V :: Value to divide by
      \return (*this/A);
      */
    {
      PolyVar<VCount> kSum(*this);
      return kSum/=V;
    }

    //
    // ---------------- SCALARS --------------------------
    //

    template<int VCount>
    PolyVar<VCount>&
      PolyVar<VCount>::operator+=(const double V) 
      /*!
      PolyVar addition
      \param V :: Value to add
      \return (*this+V);
      */
    {
      PCoeff[0]+=V;
      return *this;
    }

    template<int VCount>
    PolyVar<VCount>&
      PolyVar<VCount>::operator-=(const double V) 
      /*!
      PolyVar subtraction
      \param V :: Value to subtract
      \return (*this+V);
      */
    {
      PCoeff[0]-=V;  // There is always zero component
      return *this;
    }

    template<int VCount>
    PolyVar<VCount>&
      PolyVar<VCount>::operator*=(const double V)  
      /*!
      PolyVar multiplication
      \param V :: Value to multipication
      \return (*this*V);
      */
    {
      typename std::vector< PolyVar<VCount-1> >::iterator vc;
      for(vc=PCoeff.begin();vc!=PCoeff.end();vc++)
        (*vc)*=V;
      return *this;
    }

    template<int VCount>
    PolyVar<VCount>&
      PolyVar<VCount>::operator/=(const double V) 
      /*!
      PolyVar division scalar
      \param V :: Value to divide
      \return (*this/V);
      */
    {
      typename std::vector< PolyVar<VCount-1> >::iterator vc;
      for(vc=PCoeff.begin();vc!=PCoeff.end();vc++)
        (*vc)/=V;
      return *this;
    }

    template<int VCount>
    PolyVar<VCount> 
      PolyVar<VCount>::operator-() const
      /*!
      PolyVar negation operator
      \return -(*this);
      */
    {
      PolyVar<VCount> KOut(*this);
      KOut *= -1.0;
      return KOut;
    }

    template<int VCount>
    int
      PolyVar<VCount>::operator==(const PolyVar<VCount>& A) const
      /*!
      Determine if two polynomials are equal
      \param A :: Other polynomial to use
      \return 1 :: true 0 on false
      */
    {
      int i;
      for(i=0;i<=A.iDegree && i<=iDegree;i++)
        if (PCoeff[i]!=A.PCoeff[i])
          return 0;
      if (A.iDegree>iDegree)
      {
        for(;i<=A.iDegree;i++)
          if (!A.PCoeff[i].isZero(this->Eaccuracy))
            return 0;
      }
      else if (A.iDegree<iDegree)
      {
        for(;i<=iDegree;i++)
          if (!PCoeff[i].isZero(this->Eaccuracy))
            return 0;
      }
      return 1;
    }

    template<int VCount>
    int
      PolyVar<VCount>::operator!=(const PolyVar<VCount>& A) const
      /*!
      Determine if two polynomials are different
      \param A :: Other polynomial to use
      \return 1 is polynomial differ:  0 on false
      */
    {
      return (A==*this) ? 0 : 1;
    }

    template<int VCount>
    PolyVar<VCount> 
      PolyVar<VCount>::getDerivative() const
      /*!
      Take derivative
      \return dP / dx
      */
    {
      PolyVar<VCount> KOut(*this);
      return KOut.derivative();
    }

    template<int VCount>
    PolyVar<VCount>&
      PolyVar<VCount>::derivative()
      /*!
      Take derivative of this polynomial
      \return dP / dx
      */
    {
      if (iDegree<1)
      {
        PCoeff[0] *= 0.0;
        return *this;
      }

      for(int i=0;i<iDegree;i++)
        PCoeff[i]=PCoeff[i+1] * (i+1.0);
      iDegree--;

      return *this;
    }

    template<int VCount>
    PolyVar<VCount>
      PolyVar<VCount>::GetInversion() const
      /*!
      Inversion of the coefficients
      Note that this is useful for power balancing.
      \return (Poly)^-1
      */
    {
      PolyVar<VCount> InvPoly(iDegree);
      for (int i=0; i<=iDegree; i++)
        InvPoly.PCoeff[i] = PCoeff[iDegree-i];  
      return InvPoly;
    }

    template<int VCount>
    void 
      PolyVar<VCount>::compress(const double epsilon)
      /*!
      Two part process remove coefficients of zero or near zero: 
      Reduce degree by eliminating all (nearly) zero leading coefficients
      and by making the leading coefficient one.  
      \param epsilon :: coeficient to use to decide if a values is zero
      */
    {
      const double eps((epsilon>0.0) ? epsilon : this->Eaccuracy);
      for (;iDegree>=0 && PCoeff[iDegree].isZero(eps);iDegree--);

      PCoeff.resize(iDegree+1);

      return;
    }

    template<int VCount>
    int 
      PolyVar<VCount>::getCount(const double eps) const
      /*!
      Determine number of not zero
      \param eps :: Value to used
      \return Number of non-zero components
      */
    {
      int cnt(0);
      for(int i=0;i<=iDegree;i++)
        if(!PCoeff[i].isZero(eps))
          cnt++;
      return cnt;
    }

    template<int VCount>
    int 
      PolyVar<VCount>::isZero(const double eps) const
      /*!
      Determine if is zero
      \param eps :: Value to used
      \retval 1 :: all components  -eps<Value<eps
      \retval 0 :: one or more non zero po
      */
    {
      int i;
      for(i=0;i<=iDegree && PCoeff[i].isZero(eps);i++);
      return (i<=iDegree) ? 0 : 1;
    }

    template<int VCount>
    int 
      PolyVar<VCount>::isUnit(const double eps) const
      /*!
      Determine if is a unit base value.
      Note: this needs to be suttle, since
      x is unit, as is y, as is xy
      \param eps :: Value to used
      \retval 1 :: +ve unit
      \retval 0 :: Not unit
      \retval -1 :: -ve unit 
      */
    {
      int i;
      for(i=iDegree;i>0 && PCoeff[i].isZero(eps);i--);
      if (i)
        return 0;

      return PCoeff[0].isUnit(eps);
    }

    template<int VCount>
    int 
      PolyVar<VCount>::isUnitary(const double eps) const
      /*!
      Determine if is a unit base value.
      Note: this needs to be suttle, since
      x is unit, as is y, as is xy
      \param eps :: Value to used
      \retval 1 :: +ve unit
      \retval 0 :: Not unit
      \retval -1 :: -ve unit 
      */
    {
      int item(0);
      int flag(0);
      for(int i=iDegree;i>=0 && flag<2;i--)
      {
        if (!PCoeff[i].isZero(eps))
        {
          item=i;
          flag++;
        }
      }
      if (flag==2 || flag==0) // all zeros are also NOT unit
        return 0;
      return ((item==0) ? 1 : 2) * PCoeff[item].isUnit(eps);
    }

    template<int VCount>
    PolyVar<VCount-1>
      PolyVar<VCount>::reduce(const PolyVar<VCount>& A) const
      /*!
      The objective is to use A and this to reduce the
      variable count by 1.
      \param A :: PolyVar to use
      \return new polynomial
      */
    {
      const int ANum=PCoeff.size();
      const int BNum=A.PCoeff.size();

      const std::vector<PolyVar<VCount-1> >& 
        Major((ANum>BNum) ? PCoeff : A.PCoeff);

      const std::vector<PolyVar<VCount-1> >& 
        Minor((ANum>BNum) ? A.PCoeff : PCoeff);

      const int majorIndex(Major.size());
      const int minorIndex(Minor.size());
      const int MSize(ANum+BNum-2);

      // Assume VCount > 2:
      // Need to make a square matrix:
      Geometry::MatrixBase<PolyVar<VCount-1> > MX(MSize,MSize);
      int lneCnt(0);
      for(int i=majorIndex-1;i<MSize;i++)
      {
        for(int j=0;j<majorIndex;j++)
          MX[lneCnt][(j+lneCnt) % MSize]=Major[j];
        for(int j=majorIndex;j<MSize;j++)
          MX[lneCnt][(j+lneCnt) % MSize]=PolyVar<VCount-1>(0);
        lneCnt++;
      }

      for(int i=0;i<=MSize-minorIndex;i++)
      {
        for(int j=0;j<minorIndex;j++)
          MX[lneCnt][(j+i) % MSize]=Minor[j];
        for(int j=minorIndex;j<MSize;j++)
          MX[lneCnt][(j+i) % MSize]=PolyVar<VCount-1>(0);
        lneCnt++;
      }
      PolyVar<VCount-1> Out = MX.laplaceDeterminate();
      return Out;
    }

          /*!
      Given a line of type 
      y^2+xy+3.0x 
      convert into a function:
      Variables in list are x,y,z,a,b,c,....
      \param Line the inpout values in the format x,y,z,a,b,c,....
      \return 0 on success
      */
    template<int VCount>
    int PolyVar<VCount>::read(const std::string& Line)
    {
      const char Variable("xyzabc"[VCount-1]);
      std::string CLine=StrFunc::removeSpace(Line);
      setDegree(PolyFunction::getMaxSize(CLine,Variable));
      zeroPoly();

      std::string::size_type pos=CLine.find(Variable);
      while (pos!=std::string::npos)
      {
        int compStart(pos);
        int sign(0);     // default is positive but not found
        // get preFunction 
        int bracket(0);  // Depth of brackets
        int bCut(-1);    // Position of last ( 
        while(compStart>0)
        {
          compStart--;
          // Process brackets
          if (CLine[compStart]=='(' || CLine[compStart]==')')
          {
            bracket+= CLine[compStart]=='(' ? 1 : -1;
            if (!bracket)  // hit end of bracket:
              bCut=compStart;
          }
          else if (bracket==0)
          {
            if (CLine[compStart]=='+' || CLine[compStart]=='-')
            {
              sign=(CLine[compStart]=='+') ? 1 : -1;
              break;
            }
          } 
        }
        if (bracket)
          throw std::invalid_argument("Invalid line in PolVar::read : " + Line);

        std::string Comp;      
        if (bCut>=0)
          Comp=CLine.substr(bCut+1,pos-bCut-2);  
        else if (sign)
        {
          Comp=CLine.substr(compStart+1,pos-1);
        }
        else
        {
          Comp=CLine.substr(compStart,pos-compStart);
          sign=1;
        }
        // Need -ve on bracket: ??
        // Find power
        int pV(1);
        CLine.erase(0,pos+1);
        if (!CLine.empty() && CLine[0]=='^')
        {
          CLine.erase(0,1);
          if (!StrFunc::sectPartNum(CLine,pV) || pV<0)
            return -1;
        }
        if (Comp.empty())
          PCoeff[pV]=sign;
        else 
        {
          if (PCoeff[pV].read(Comp))
            return -2;
          if (sign<0) PCoeff[pV]*=-1.0;
        }
        // Find next value
        pos=CLine.find(Variable);
      }

      if (!CLine.empty())
        PCoeff[0].read(CLine);
      // Process PCoeff[0]
      return 0;
    }


    template<int VCount>
    int
      PolyVar<VCount>::write(std::ostream& OX,const int prePlus) const
      /*!
      Basic write command
      \param OX :: output stream
      \param prePlus :: prePlus
      \retval 0 :: nothing written 
      \retval 1 :: normal equation
      \retval -1 :: unitary equation ("1") 
      */
    {
      const char Variable("xyzabc"[VCount-1]);
      int nowrite(0);
      for(int i=iDegree;i>=0;i--)
      {
        const int cnt=PCoeff[i].getCount(this->Eaccuracy);
        if (cnt>1)
        {
          if (i)
          {
            if (prePlus || nowrite ) 
              OX<<((prePlus>=0) ? "+" : "-");
            OX<<"("<<PCoeff[i]<<")";
          }
          else
            PCoeff[0].write(OX,(prePlus || nowrite));

          nowrite=1;
        }
        else if (cnt==1)
        {
          const int oneFlag=PCoeff[i].isUnit(this->Eaccuracy);
          if (oneFlag>0 && (nowrite || prePlus)) 
            OX<<"+";
          else if (oneFlag<0) 
            OX<<"-";

          if (!oneFlag)
            PCoeff[i].write(OX,nowrite|prePlus);
          else if (!i)
            OX<<"1";

          nowrite=1;
        }

        if (i && cnt)
        {
          OX<<Variable;
          if (i!=1)
            OX<<"^"<<i;
        }
      }
      return nowrite;
    }  

    /// \cond TEMPLATE

    template class DLLExport PolyVar<2>;   // f(x,y,z)
    template class DLLExport PolyVar<3>;   // f(x,y,z,a)

    template DLLExport std::ostream& operator<<(std::ostream&,const PolyVar<1>&);
    template DLLExport std::ostream& operator<<(std::ostream&,const PolyVar<2>&);
    template DLLExport std::ostream& operator<<(std::ostream&,const PolyVar<3>&);

    template DLLExport void PolyVar<2>::setComp(int,const PolyVar<1>&);

    template DLLExport PolyVar<2>& PolyVar<2>::operator=(const PolyVar<1>&);
    template DLLExport PolyVar<3>& PolyVar<3>::operator=(const PolyVar<1>&);
    template DLLExport PolyVar<3>& PolyVar<3>::operator=(const PolyVar<2>&);

    template DLLExport PolyVar<2>::PolyVar(const PolyVar<1>&);
    template DLLExport PolyVar<3>::PolyVar(const PolyVar<1>&);
    template DLLExport PolyVar<3>::PolyVar(const PolyVar<2>&);


    /// \endcond TEMPLATE

  }  // NAMESPACE  mathLevel

} // NAMESPACE Mantid
