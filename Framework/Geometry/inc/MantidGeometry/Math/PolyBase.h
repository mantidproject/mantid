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

  int solveQuadratic(std::complex<double> &, std::complex<double> &) const;
  int solveCubic(std::complex<double> &, std::complex<double> &,
                 std::complex<double> &) const;

public:
  explicit PolyBase(int const);
  PolyBase(int const, double const);
  PolyBase(const PolyBase &);
  ~PolyBase();

  // member access
  void setDegree(int const);
  int getDegree() const;
  operator const std::vector<double> &() const;
  operator std::vector<double> &();
  double operator[](int const) const;
  double &operator[](int const);

  // assignment
  PolyBase &operator=(const PolyBase &);

  // evaluation
  double operator()(double const) const;

  // arithmetic updates
  PolyBase &operator+=(const PolyBase &);
  PolyBase &operator-=(const PolyBase &);
  PolyBase &operator*=(const PolyBase &);

  // arithmetic operations
  PolyBase operator+(const PolyBase &) const;
  PolyBase operator-(const PolyBase &) const;
  PolyBase operator*(const PolyBase &) const;
  //  PolyBase operator/(const PolyBase&) const;

  // input is degree 0 poly
  PolyBase operator+(double const) const; // input is degree 0 poly
  PolyBase operator-(double const) const;
  PolyBase operator*(double const) const;
  PolyBase operator/(double const) const;

  PolyBase operator-() const;

  // input is degree 0 poly
  PolyBase &operator+=(double const);
  PolyBase &operator-=(double const);
  PolyBase &operator*=(double const);
  PolyBase &operator/=(double const);

  // derivation
  PolyBase getDerivative() const;
  PolyBase &derivative();

  // inversion ( invpoly[i] = poly[degree-i] for 0 <= i <= degree )
  PolyBase GetInversion() const;

  void compress(double const);

  void divide(const PolyBase &, PolyBase &, PolyBase &,
              double const = -1.0) const;

  std::vector<double> realRoots(double const = -1.0);
  std::vector<std::complex<double>> calcRoots(double const = -1.0);

  void write(std::ostream &) const;
};

PolyBase operator*(double const, const PolyBase &);
std::ostream &operator<<(std::ostream &, const PolyBase &);

} // NAMESPACE mathlevel

} // NAMESPACE Mantid
#endif
