#include <iostream>
#include <cmath>
#include <complex>
#include <vector>
#include <iterator>
#include <algorithm>

#include "MantidGeometry/Math/mathSupport.h"

namespace Mantid {

template <typename InputIter>
int
solveQuadratic(const InputIter Coef,
               std::pair<std::complex<double>, std::complex<double>> &OutAns)
/**
  Solves Complex Quadratic
  @param Coef :: iterator over all the coefients in the order
  \f[ Ax^2+Bx+C \f].
  @param OutAns :: complex roots of the equation
  @return number of unique solutions
*/
{
  double a, b, c, cf;
  a = (*Coef);
  b = *(Coef + 1);
  c = *(Coef + 2);

  if (a == 0.0) {
    if (b == 0.0) {
      OutAns.first = std::complex<double>(0.0, 0.0);
      OutAns.second = std::complex<double>(0.0, 0.0);
      return 0;
    } else {
      OutAns.first = std::complex<double>(-c / b, 0.0);
      OutAns.second = OutAns.first;
      return 1;
    }
  }
  cf = b * b - 4 * a * c;
  if (cf >= 0) /* Real Roots */
  {
    const double q = (b >= 0) ? -0.5 * (b + sqrt(cf)) : -0.5 * (b - sqrt(cf));
    OutAns.first = std::complex<double>(q / a, 0.0);
    OutAns.second = std::complex<double>(c / q, 0.0);
    return (cf == 0) ? 1 : 2;
  }

  std::complex<double> CQ(-0.5 * b,
                          (b >= 0 ? -0.5 * sqrt(-cf) : 0.5 * sqrt(-cf)));
  OutAns.first = CQ / a;
  OutAns.second = c / CQ;
  return 2;
}

template <typename CInputIter>
int solveCubic(const CInputIter Coef, std::complex<double> &AnsA,
               std::complex<double> &AnsB, std::complex<double> &AnsC)
/**
  Solves Cubic equation
  @param Coef :: iterator over all the coefients in the order
  \f[ Ax^3+Bx^2+Cx+D \f].
  @param AnsA,AnsB,AnsC :: complex roots of the equation
  @return number of unique solutions
*/

{
  typedef std::complex<double> Cpair;
  double q, r; /* solution parameters */
  double termR, discrim;
  double r13;
  std::pair<std::complex<double>, std::complex<double>> SQ;

  if ((*Coef) == 0) {
    const int xi = solveQuadratic(Coef + 1, SQ);
    AnsA = SQ.first;
    AnsB = SQ.second;
    AnsC = SQ.second;
    return xi;
  }
  if (*(Coef + 3) == 0) {
    const int xi = solveQuadratic(Coef, SQ);
    std::cerr << "Xi == " << xi << std::endl;
    AnsA = SQ.first;
    AnsB = (xi == 1) ? SQ.first : SQ.second;
    AnsC = std::complex<double>(0.0, 0.0);
    return (AnsC != AnsA) ? xi + 1 : xi;
  }
  const double a = (*Coef);
  const double b = *(Coef + 1) / a;
  const double c = *(Coef + 2) / a;
  const double d = *(Coef + 3) / a;

  q = (3.0 * c - (b * b)) / 9.0;               // -q
  r = -27.0 * d + b * (9.0 * c - 2.0 * b * b); // -r
  r /= 54.0;

  discrim = q * q * q + r * r; // r^2-qq^3
  /* The first root is always real. */
  termR = (b / 3.0);

  if (discrim > 1e-13) /* one root real, two are complex */
  {
    double s = r + sqrt(discrim);
    s = ((s < 0) ? -pow(-s, (1.0 / 3.0)) : pow(s, (1.0 / 3.0)));
    double t = r - sqrt(discrim);
    t = ((t < 0) ? -pow(-t, (1.0 / 3.0)) : pow(t, (1.0 / 3.0)));
    AnsA = Cpair(-termR + s + t, 0.0);
    // second real point.
    termR += (s + t) / 2.0;
    double termI = sqrt(3.0) * (-t + s) / 2;
    AnsB = Cpair(-termR, termI);
    AnsC = Cpair(-termR, -termI);
    return 3;
  }

  /* The remaining options are all real */

  if (discrim < 1e-13) // All roots real
  {
    q = -q;
    double q3 = q * q * q;
    q3 = acos(-r / sqrt(q3));
    r13 = -2.0 * sqrt(q);
    AnsA = Cpair(-termR + r13 * cos(q3 / 3.0), 0.0);
    AnsB = Cpair(-termR + r13 * cos((q3 + 2.0 * M_PI) / 3.0), 0.0);
    AnsC = Cpair(-termR + r13 * cos((q3 - 2.0 * M_PI) / 3.0), 0.0);
    return 3;
  }

  // Only option left is that all roots are real and unequal
  // (to get here, q*q*q=r*r)

  r13 = ((r < 0) ? -pow(-r, (1.0 / 3.0)) : pow(r, (1.0 / 3.0)));
  AnsA = Cpair(-termR + 2.0 * r13, 0.0);
  AnsB = Cpair(-(r13 + termR), 0.0);
  AnsC = Cpair(-(r13 + termR), 0.0);
  return 2;
}

/// \cond TEMPLATE

template MANTID_GEOMETRY_DLL int
solveQuadratic(const double *,
               std::pair<std::complex<double>, std::complex<double>> &);
template MANTID_GEOMETRY_DLL int
solveQuadratic(double *,
               std::pair<std::complex<double>, std::complex<double>> &);
template MANTID_GEOMETRY_DLL int
solveQuadratic(const std::vector<double>::const_iterator,
               std::pair<std::complex<double>, std::complex<double>> &);
template MANTID_GEOMETRY_DLL int solveCubic(const double *,
                                            std::complex<double> &,
                                            std::complex<double> &,
                                            std::complex<double> &);
template MANTID_GEOMETRY_DLL int solveCubic(double *, std::complex<double> &,
                                            std::complex<double> &,
                                            std::complex<double> &);

template MANTID_GEOMETRY_DLL int solveCubic(const std::vector<double>::iterator,
                                            std::complex<double> &,
                                            std::complex<double> &,
                                            std::complex<double> &);
template MANTID_GEOMETRY_DLL int
solveCubic(const std::vector<double>::const_iterator, std::complex<double> &,
           std::complex<double> &, std::complex<double> &);

/// \endcond TEMPLATE
}
