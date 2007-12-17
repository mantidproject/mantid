#ifndef mathLevel_PolyBase_h
#define mathLevel_PolyBase_h


namespace Mantid
{

namespace mathLevel
{

  /*!
    \class PolyBase
    \version 1.0
    \author S. Ansell and D. Eberly
    \date December 2007
    \brief Holds a polynominal as a primary type
  */

class PolyBase
{
protected:

  int iDegree;                    ///< Degree of polynomial [0 == constant]
  std::vector<double> afCoeff;    ///< Coefficients

public:

  explicit PolyBase(int const);
  PolyBase(const PolyBase&);
  ~PolyBase();

    // member access
  void SetDegree(int const);
  int GetDegree() const;
  operator const std::vector<double>& () const;
  operator std::vector<double>& ();
  double operator[](int const) const;
  double& operator[](int const);

  // assignment
  PolyBase& operator=(const PolyBase&);

  // evaluation
  double operator()(double const) const;

  // arithmetic updates
  PolyBase& operator+=(const PolyBase&);
  PolyBase& operator-=(const PolyBase&);
  PolyBase& operator*=(const PolyBase&);

  // arithmetic operations
  PolyBase operator+(const PolyBase&) const;
  PolyBase operator-(const PolyBase&) const;
  PolyBase operator*(const PolyBase&) const;
  PolyBase operator/(const PolyBase&) const;

  PolyBase operator+(double const) const;  // input is degree 0 poly
  PolyBase operator-(double const) const;  // input is degree 0 poly
  PolyBase operator*(double const) const;
  PolyBase operator/(double const) const;
  PolyBase operator-() const;

  PolyBase& operator+=(double const);  // input is degree 0 poly
  PolyBase& operator-=(double const);  // input is degree 0 poly
  PolyBase& operator*=(double const);
  PolyBase& operator/=(double const);

    // derivation
  PolyBase GetDerivative() const;
  PolyBase& derivative();

  // inversion ( invpoly[i] = poly[degree-i] for 0 <= i <= degree )
  PolyBase GetInversion() const;

  void Compress(double const);

  void Divide(const PolyBase&,PolyBase&,PolyBase&,double const) const;

};

PolyBase operator*(double const,const PolyBase&);

}  // NAMESPACE mathlevel

}  // NAMESPACE Mantid
#endif
