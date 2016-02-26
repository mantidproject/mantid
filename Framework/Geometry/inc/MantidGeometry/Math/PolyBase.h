#ifndef mathLevel_PolyBase_h
#define mathLevel_PolyBase_h

#include "MantidGeometry/DllConfig.h"

namespace Mantid {

namespace mathLevel {

/**
  \class PolyBase
  \version 1.0
  \author S. Ansell and D. Eberly
  \date December 2007
  \brief Holds a polynominal as a primary type

  This holds a single variable poynomial of primary positive type.
  Assumes Euclidean division, hence a remainders.
  Internal solution of the polynomial is possible. Conversion to
  other forms is not internally handled.

  \todo Add conversion to a continued fraction.
  \todo ADD solveQuartic
*/

class MANTID_GEOMETRY_DLL PolyBase {
private:
  int iDegree;                 ///< Degree of polynomial [0 == constant]
  std::vector<double> afCoeff; ///< Coefficients
  double Eaccuracy;            ///< Polynomic accuracy

  int solveQuadratic(std::complex<double> & /*AnsA*/,
                     std::complex<double> & /*AnsB*/) const;
  int solveCubic(std::complex<double> & /*AnsA*/,
                 std::complex<double> & /*AnsB*/,
                 std::complex<double> & /*AnsC*/) const;

public:
  explicit PolyBase(int const /*iD*/);
  PolyBase(int const /*iD*/, double const /*E*/);
  PolyBase(const PolyBase & /*A*/);
  ~PolyBase();

  // member access
  void setDegree(int const /*iD*/);
  int getDegree() const;
  operator const std::vector<double> &() const;
  operator std::vector<double> &();
  double operator[](int const /*i*/) const;
  double &operator[](int const /*i*/);

  // assignment
  PolyBase &operator=(const PolyBase & /*A*/);

  // evaluation
  double operator()(double const /*X*/) const;

  // arithmetic updates
  PolyBase &operator+=(const PolyBase & /*A*/);
  PolyBase &operator-=(const PolyBase & /*A*/);
  PolyBase &operator*=(const PolyBase & /*A*/);

  // arithmetic operations
  PolyBase operator+(const PolyBase & /*A*/) const;
  PolyBase operator-(const PolyBase & /*A*/) const;
  PolyBase operator*(const PolyBase & /*A*/) const;
  //  PolyBase operator/(const PolyBase&) const;

  // input is degree 0 poly
  PolyBase operator+(double const /*V*/) const; // input is degree 0 poly
  PolyBase operator-(double const /*V*/) const;
  PolyBase operator*(double const /*V*/) const;
  PolyBase operator/(double const /*V*/) const;

  PolyBase operator-() const;

  // input is degree 0 poly
  PolyBase &operator+=(double const /*V*/);
  PolyBase &operator-=(double const /*V*/);
  PolyBase &operator*=(double const /*V*/);
  PolyBase &operator/=(double const /*V*/);

  // derivation
  PolyBase getDerivative() const;
  PolyBase &derivative();

  // inversion ( invpoly[i] = poly[degree-i] for 0 <= i <= degree )
  PolyBase GetInversion() const;

  void compress(double const /*epsilon*/);

  void divide(const PolyBase & /*pD*/, PolyBase & /*pQ*/, PolyBase & /*pR*/,
              double const /*epsilon*/ = -1.0) const;

  std::vector<double> realRoots(double const /*epsilon*/ = -1.0);
  std::vector<std::complex<double>> calcRoots(double const /*epsilon*/ = -1.0);

  void write(std::ostream & /*OX*/) const;
};

PolyBase operator*(double const, const PolyBase &);
std::ostream &operator<<(std::ostream & /*OX*/, const PolyBase & /*A*/);

} // NAMESPACE mathlevel

} // NAMESPACE Mantid
#endif
