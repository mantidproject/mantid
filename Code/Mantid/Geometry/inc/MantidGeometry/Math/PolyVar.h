#ifndef mathLevel_PolyVar_h
#define mathLevel_PolyVar_h

#include "MantidKernel/System.h"
#include "MantidGeometry/Math/PolyFunction.h"

namespace Mantid
{
namespace mathLevel
{

  /*!
    \class PolyVar
    \version 1.0
    \author S. Ansell 
    \date December 2007
    \brief Holds a polynominal as a secondary type

    This holds a multi-variable polynomial of primary positive type.
    Assumes Euclidean division, hence a remainders.
    Internal solution of the polynomial is possible. Conversion to 
    other forms is not internally handled.

    Note that Index refers to the depth. It is only valid from 1 to N.
    PolyVar<1> has been specialised
  */

template<int VCount>
class DLLExport PolyVar  : public PolyFunction
{
 private:

  int iDegree;                                ///< Degree of polynomial  
  std::vector<PolyVar<VCount-1> > PCoeff;     ///< Polynominals [0]=const 

 public:

  explicit PolyVar(int const =0);
  PolyVar(int const,double const);
  PolyVar(const PolyVar<VCount>&);                       
  template<int ICount> PolyVar(const PolyVar<ICount>&);
  PolyVar<VCount>& operator=(const PolyVar<VCount>&);
  template<int ICount> PolyVar<VCount>& operator=(const PolyVar<ICount>&);
  PolyVar<VCount>& operator=(const double&);
  ~PolyVar();

    // member access
  void setDegree(int const);
  int getDegree() const;
  void zeroPoly();

  template<int ICount>
  void setComp(const int Index,const PolyVar<ICount>& FX);
  void setComp(const int Index,double const& V);
  // get value
  double operator()(const std::vector<double>&) const;
  double operator()(const double*) const;

  // arithmetic updates
  PolyVar<VCount>& operator+=(const PolyVar<VCount>&);
  PolyVar<VCount>& operator-=(const PolyVar<VCount>&);
  PolyVar<VCount>& operator*=(const PolyVar<VCount>&);
 

  // arithmetic operation
  PolyVar<VCount> operator+(const PolyVar<VCount>&) const;
  PolyVar<VCount> operator-(const PolyVar<VCount>&) const;
  PolyVar<VCount> operator*(const PolyVar<VCount>&) const;

  PolyVar<VCount> operator+(double const) const;  // input is degree 0 poly
  PolyVar<VCount> operator-(double const) const;  // input is degree 0 poly
  PolyVar<VCount> operator*(double const) const;
  PolyVar<VCount> operator/(double const) const;
  PolyVar<VCount> operator-() const;

 // input is degree 0 poly
  PolyVar<VCount>& operator+=(double const); 
  PolyVar<VCount>& operator-=(double const); 
  PolyVar<VCount>& operator*=(double const);
  PolyVar<VCount>& operator/=(double const);

  int operator==(const PolyVar<VCount>&) const;
  int operator!=(const PolyVar<VCount>&) const;

    // derivation
  PolyVar getDerivative() const;
  PolyVar& derivative();

  // inversion ( invpoly[i] = poly[degree-i] for 0 <= i <= degree )
  PolyVar GetInversion() const;
  void compress(double const =1.0);

  int isZero(double const) const;
  int isUnit(double const) const;
  int isUnitary(double const) const;
  int getCount(double const) const;
  /// Returns 1 if this is a Double, otherwise 0
  int isDouble() const { return 0; }
  /// Get a double valued coefficient:
  double getDouble() const { return PCoeff[0].getDouble(); }  
  PolyVar<VCount-1> reduce(const PolyVar<VCount>&) const;

  int read(const std::string&);
  int write(std::ostream& OX,const int prePlus =0) const;
};

template<int VCount> 
std::ostream& operator<<(std::ostream&,const PolyVar<VCount>&);

/// Specialisation for single degree polynomials
template<> 
class DLLExport PolyVar<1> : public PolyFunction
{
 private:

  int iDegree;                    ///< Degree of polynomial [0 == constant]
  std::vector<double> PCoeff;     ///< Coefficients

  int solveQuadratic(std::complex<double>&,std::complex<double>&) const;
  int solveCubic(std::complex<double>&,std::complex<double>&,
		 std::complex<double>&) const;

 public:

  explicit PolyVar<1>(int const =0);
  PolyVar<1>(int const,double const);
  PolyVar<1>(const PolyVar<1>&);
  PolyVar<1>& operator=(const PolyVar<1>&);
  PolyVar<1>& operator=(const double&);
  ~PolyVar<1>();

    // member access
  void setDegree(int const);
  int getDegree() const;
  void zeroPoly();

  operator const std::vector<double>& () const;
  operator std::vector<double>& ();
  double operator[](int const) const;
  double& operator[](int const);


  // evaluation
  double operator()(double const) const;
  double operator()(const std::vector<double>&) const;
  double operator()(const double*) const;

  // arithmetic updates
  PolyVar<1>& operator+=(const PolyVar<1>&);
  PolyVar<1>& operator-=(const PolyVar<1>&);
  PolyVar<1>& operator*=(const PolyVar<1>&);

  // arithmetic operations
  PolyVar<1> operator+(const PolyVar<1>&) const;
  PolyVar<1> operator-(const PolyVar<1>&) const;
  PolyVar<1> operator*(const PolyVar<1>&) const;

  PolyVar<1> operator+(double const) const;  // input is degree 0 poly
  PolyVar<1> operator-(double const) const;  // input is degree 0 poly
  PolyVar<1> operator*(double const) const;
  PolyVar<1> operator/(double const) const;
  PolyVar<1> operator-() const;

 // input is degree 0 poly
  PolyVar<1>& operator+=(double const); 
  PolyVar<1>& operator-=(double const); 
  PolyVar<1>& operator*=(double const);
  PolyVar<1>& operator/=(double const);

  int operator==(const PolyVar<1>&) const;
  int operator!=(const PolyVar<1>&) const;
  int operator==(const double&) const;
  int operator!=(const double&) const;

    // derivation
  PolyVar<1> getDerivative() const;
  PolyVar<1>& derivative();

  // inversion ( invpoly[i] = poly[degree-i] for 0 <= i <= degree )
  PolyVar<1> GetInversion() const;

  void compress(double const); 

  void divide(const PolyVar<1>&,PolyVar<1>&,PolyVar<1>&,double const =-1.0) const;

  std::vector<double> realRoots(double const= -1.0);
  std::vector<std::complex<double> > calcRoots(double const= -1.0);

  int isZero(double const) const;
  int isUnit(double const) const;
  int isUnitary(double const) const;
  int getCount(double const) const;
  /// Returns 1 if this is a Double, otherwise 0
  int isDouble() const { return 1; }
  /// Get a double valued coefficient:
  double getDouble() const { return PCoeff[0]; }

  int read(const std::string&);
  int write(std::ostream& OX,const int prePlus =0) const;
};

}  // NAMESPACE mathlevel

}  // NAMESPACE Mantid

#endif
