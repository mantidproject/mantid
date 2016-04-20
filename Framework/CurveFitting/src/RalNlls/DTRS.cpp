#include "MantidCurveFitting/RalNlls/DTRS.h"

#include <algorithm>
#include <gsl/gsl_blas.h>
#include <string>
#include <tuple>

#define for_do(i, a, b) for (int i = a; i <= b; ++i)

namespace Mantid {
namespace CurveFitting {
namespace RalNlls {

namespace {

typedef double real;
typedef double REAL;
typedef int integer;
typedef int INTEGER;
typedef bool logical;
typedef bool LOGICAL;

double smallest(double a, double b, double c, double d) {
  return std::min(std::min(a, b), std::min(c, d));
}

double smallest(double a, double b, double c) {
  return std::min(std::min(a, b), c);
}

double biggest(double a, double b, double c, double d) {
  return std::max(std::max(a, b), std::max(c, d));
}

double biggest(double a, double b, double c) {
  return std::max(std::max(a, b), c);
}

double maxAbsVal(const DoubleFortranVector &v) {
  auto p = v.indicesOfMinMaxElements();
  return std::max(fabs(v.get(p.first)), fabs(v.get(p.second)));
}

double minAbsVal(const DoubleFortranVector &v) {
  auto p = v.indicesOfMinMaxElements();
  return std::min(fabs(v.get(p.first)), fabs(v.get(p.second)));
}

std::pair<double, double> minMaxValues(const DoubleFortranVector &v) {
  auto p = v.indicesOfMinMaxElements();
  return std::make_pair(v.get(p.first), v.get(p.second));
}

double two_norm(const DoubleFortranVector &v) {
  if (v.size() == 0)
    return 0.0;
  return gsl_blas_dnrm2(v.gsl());
}

double dot_product(const DoubleFortranVector &v1,
                   const DoubleFortranVector &v2) {
  return v1.dot(v2);
}

double maxVal(const DoubleFortranVector &v, int n) {
  double res = -std::numeric_limits<double>::max();
  for (int i = 1; i <= n; ++i) {
    auto val = v(i);
    if (val > res) {
      res = val;
    }
  }
  return res;
}

const REAL zero = 0.0;
const REAL one = 1.0;
const REAL two = 2.0;
const REAL three = 3.0;
const REAL four = 4.0;
const REAL six = 6.0;
const REAL ten = 10.0;
const REAL quarter = 0.25;
const REAL point4 = 0.4;
const REAL threequarters = 0.75;
const REAL onesixth = one / six;
const REAL sixth = onesixth;
const REAL onethird = one / three;
const REAL half = 0.5;
const REAL twothirds = two / three;
const REAL twentyfour = 24.0;
//!     REAL pi = four * ATAN( 1.0 );
const REAL pi = 3.1415926535897931;
//!     REAL magic = twothirds * pi
const REAL magic = 2.0943951023931953; //!! 2 pi/3
const REAL infinity = HUGE;
const int max_degree = 3;
const int history_max = 100;
const REAL teneps = ten * epsmch;
const REAL roots_tol = teneps;
const bool roots_debug = false;

} // anonymous namespace

void dtrs_solve_main(int n, double radius, double f,
                     const DoubleFortranVector &c, const DoubleFortranVector &h,
                     DoubleFortranVector &x, const dtrs_control_type &control,
                     dtrs_inform_type &inform);

//! Contains  ral_nlls_roots
//!             ral_dtrs
//! THIS VERSION: RAL_NLLS 1.1 - 07/03/2016 AT 09:45 GMT.
//
//!-*-*-*-*-*-*-*-*-*-  R A L _ N L L S _ R O O T S   M O D U L E
//!-*-*-*-*-*-*-*-
//
//!  Copyright reserved, Gould/Orban/Toint, for RAL_NLLS productions
//!  Principal author: Nick Gould
//
//!  History -
//!   extracted from GALAHAD package ROOTS, March 7th, 2016
//
//!  For full documentation, see
//!   http://galahad.rl.ac.uk/galahad-www/specs.html
//
//   MODULE RAL_NLLS_ROOTS_double
//
//!     --------------------------------------------------------------------
//!     |                                                                  |
//!     |  Find (all the) real roots of polynomials with real coefficients |
//!     |                                                                  |
//!     --------------------------------------------------------------------
//
//      USE RAL_NLLS_SYMBOLS
//
//      IMPLICIT NONE
//
//      PRIVATE
//      PUBLIC  ROOTS_quadratic, ROOTS_cubic, ROOTS_quartic
//
//!--------------------
//!   P r e c i s i o n
//!--------------------
//
//      INTEGER, PARAMETER  wp = KIND( 1.0D+0 )
//
//!----------------------
//!   P a r a m e t e r s
//!----------------------
//
//      INTEGER, PARAMETER  out = 6

//!  interface to LAPACK: eigenvalues of a Hessenberg matrix
//
//      INTERFACE HSEQR
//        SUBROUTINE SHSEQR( job, compz, n, ilo, ihi, H, ldh,  WR, WI, Z, ldz, &
//                           WORK, lwork, info )
//        INTEGER, INTENT( IN )  ihi, ilo, ldh, ldz, lwork, n
//        INTEGER, INTENT( OUT )  info
//        CHARACTER ( LEN = 1 ), INTENT( IN )  compz, job
//        REAL, INTENT( INOUT )  H( ldh, * ), Z( ldz, * )
//        REAL, INTENT( OUT )  WI( * ), WR( * ), WORK( * )
//        END SUBROUTINE SHSEQR
//
//        SUBROUTINE DHSEQR( job, compz, n, ilo, ihi, H, ldh,  WR, WI, Z, ldz, &
//                           WORK, lwork, info )
//        INTEGER, INTENT( IN )  ihi, ilo, ldh, ldz, lwork, n
//        INTEGER, INTENT( OUT )  info
//        CHARACTER ( LEN = 1 ), INTENT( IN )  compz, job
//        DOUBLE PRECISION, INTENT( INOUT )  H( ldh, * ), Z( ldz, * )
//        DOUBLE PRECISION, INTENT( OUT )  WI( * ), WR( * ), WORK( * )
//        END SUBROUTINE DHSEQR
//      END INTERFACE HSEQR
//
//   CONTAINS

///-*-*-*-*-*-   R O O T S _ q u a d r a t i c  S U B R O U T I N E -*-*-*-*-*-
/// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
///
///  Find the number and values of real roots of the quadratic equation
///
///                   a2 * x**2 + a1 * x + a0 = 0
///
///  where a0, a1 and a2 are real
///
/// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
void roots_quadratic(double a0, double a1, double a2, double tol, int &nroots,
                     double &root1, double &root2, bool debug) {

  //!  Dummy arguments
  //
  //      INTEGER, INTENT( OUT )  nroots
  //      REAL , INTENT( IN )  a2, a1, a0, tol
  //      REAL , INTENT( OUT )  root1, root2
  //      LOGICAL, INTENT( IN )  debug

  UNUSED_ARG(debug);
  auto rhs = tol * a1 * a1;
  if (std::fabs(a0 * a2) > rhs) { // !  really is quadratic
    root2 = a1 * a1 - four * a2 * a0;
    if (abs(root2) <= pow(epsmch * a1, 2)) { // ! numerical double root
      nroots = 2;
      root1 = -half * a1 / a2;
      root2 = root1;
    } else if (root2 < zero) { // ! complex not real roots
      nroots = 0;
      root1 = zero;
      root2 = zero;
    } else { // ! distint real roots
      auto d = -half * (a1 + sign(sqrt(root2), a1));
      nroots = 2;
      root1 = d / a2;
      root2 = a0 / d;
      if (root1 > root2) {
        d = root1;
        root1 = root2;
        root2 = d;
      }
    }
  } else if (a2 == zero) {
    if (a1 == zero) {
      if (a0 == zero) { //! the function is zero
        nroots = 1;
        root1 = zero;
        root2 = zero;
      } else { //! the function is constant
        nroots = 0;
        root1 = zero;
        root2 = zero;
      }
    } else { //! the function is linear
      nroots = 1;
      root1 = -a0 / a1;
      root2 = zero;
    }
  } else { // ! very ill-conditioned quadratic
    nroots = 2;
    if (-a1 / a2 > zero) {
      root1 = zero;
      root2 = -a1 / a2;
    } else {
      root1 = -a1 / a2;
      root2 = zero;
    }
  }

  //!  perfom a Newton iteration to ensure that the roots are accurate

  if (nroots >= 1) {
    auto p = (a2 * root1 + a1) * root1 + a0;
    auto pprime = two * a2 * root1 + a1;
    if (pprime != zero) {
      // if ( debug ) write( out, 2000 ) 1, root1, p, - p / pprime
      root1 = root1 - p / pprime;
      p = (a2 * root1 + a1) * root1 + a0;
    }
    // if ( debug ) write( out, 2010 ) 1, root1, p
    if (nroots == 2) {
      p = (a2 * root2 + a1) * root2 + a0;
      pprime = two * a2 * root2 + a1;
      if (pprime != zero) {
        // if ( debug ) write( out, 2000 ) 2, root2, p, - p / pprime
        root2 = root2 - p / pprime;
        p = (a2 * root2 + a1) * root2 + a0;
      }
      // if ( debug ) write( out, 2010 ) 2, root2, p
    }
  }
} //!  End of subroutine ROOTS_quadratic

///-*-*-*-*-*-*-*-   R O O T S _ c u b i c  S U B R O U T I N E -*-*-*-*-*-*-*-
/// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
///
///  Find the number and values of real roots of the cubic equation
///
///                a3 * x**3 + a2 * x**2 + a1 * x + a0 = 0
///
///  where a0, a1, a2 and a3 are real
///
/// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
void roots_cubic(double a0, double a1, double a2, double a3, double tol,
                 int &nroots, double &root1, double &root2, double &root3,
                 bool debug) {

  //!  Dummy arguments
  //      INTEGER, INTENT( OUT )  nroots
  //      REAL , INTENT( IN )  a3, a2, a1, a0, tol
  //      REAL , INTENT( OUT )  root1, root2, root3
  //      LOGICAL, INTENT( IN )  debug
  UNUSED_ARG(debug);

  //!  Check to see if the cubic is actually a quadratic
  if (a3 == zero) {
    roots_quadratic(a0, a1, a2, tol, nroots, root1, root2, debug);
    root3 = infinity;
    return;
  }

  //!  Deflate the polnomial if the trailing coefficient is zero
  if (a0 == zero) {
    root1 = zero;
    roots_quadratic(a1, a2, a3, tol, nroots, root2, root3, debug);
    nroots = nroots + 1;
    return;
  }

  //!  1. Use Nonweiler's method (CACM 11:4, 1968, pp269)

  double c0 = a0 / a3;
  double c1 = a1 / a3;
  double c2 = a2 / a3;

  double s = c2 / three;
  double t = s * c2;
  double b = 0.5 * (s * (twothirds * t - c1) + c0);
  t = (t - c1) / three;
  double c = t * t * t;
  double d = b * b - c;

  //! 1 real + 2 equal real or 2 complex roots
  if (d >= zero) {
    d = pow(sqrt(d) + fabs(b), onethird);
    if (d != zero) {
      if (b > zero) {
        b = -d;
      } else {
        b = d;
      }
      c = t / b;
    }
    d = sqrt(threequarters) * (b - c);
    b = b + c;
    c = -0.5 * b - s;
    root1 = b - s;
    if (d == zero) {
      nroots = 3;
      root2 = c;
      root3 = c;
    } else {
      nroots = 1;
    }
  } else { //! 3 real roots
    if (b == zero) {
      d = twothirds * atan(one);
    } else {
      d = atan(sqrt(-d) / fabs(b)) / three;
    }
    if (b < zero) {
      b = two * sqrt(t);
    } else {
      b = -two * sqrt(t);
    }
    c = cos(d) * b;
    t = -sqrt(threequarters) * sin(d) * b - half * c;
    d = -t - c - s;
    c = c - s;
    t = t - s;
    if (abs(c) > abs(t)) {
      root3 = c;
    } else {
      root3 = t;
      t = c;
    }
    if (abs(d) > abs(t)) {
      root2 = d;
    } else {
      root2 = t;
      t = d;
    }
    root1 = t;
    nroots = 3;
  }

  //!  reorder the roots

  //  900 CONTINUE
  if (nroots == 3) {
    if (root1 > root2) {
      double a = root2;
      root2 = root1;
      root1 = a;
    }
    if (root2 > root3) {
      double a = root3;
      if (root1 > root3) {
        a = root1;
        root1 = root3;
      }
      root3 = root2;
      root2 = a;
    }
  }

  //!  perfom a Newton iteration to ensure that the roots are accurate
  double p = ((a3 * root1 + a2) * root1 + a1) * root1 + a0;
  double pprime = (three * a3 * root1 + two * a2) * root1 + a1;
  if (pprime != zero) {
    root1 = root1 - p / pprime;
    p = ((a3 * root1 + a2) * root1 + a1) * root1 + a0;
  }

  if (nroots == 3) {
    p = ((a3 * root2 + a2) * root2 + a1) * root2 + a0;
    pprime = (three * a3 * root2 + two * a2) * root2 + a1;
    if (pprime != zero) {
      root2 = root2 - p / pprime;
      p = ((a3 * root2 + a2) * root2 + a1) * root2 + a0;
    }

    p = ((a3 * root3 + a2) * root3 + a1) * root3 + a0;
    pprime = (three * a3 * root3 + two * a2) * root3 + a1;
    if (pprime != zero) {
      root3 = root3 - p / pprime;
      p = ((a3 * root3 + a2) * root3 + a1) * root3 + a0;
    }
  }

} //!  End of subroutine ROOTS_cubic

///-*-*-*-*-*-*-   R O O T S _ q u a r t i c   S U B R O U T I N E -*-*-*-*-*-*-
///
/// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
///
///  Find the number and values of real roots of the quartic equation
///
///        a4 * x**4 + a3 * x**3 + a2 * x**2 + a1 * x + a0 = 0
///
///  where a0, a1, a2, a3 and a4 are real
///
/// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
void roots_quartic(double a0, double a1, double a2, double a3, double a4,
                   double tol, int &nroots, double &root1, double &root2,
                   double &root3, double &root4, bool debug) {
  //
  //!  Dummy arguments
  //
  //      INTEGER, INTENT( OUT )  nroots
  //      REAL , INTENT( IN )  a4, a3, a2, a1, a0, tol
  //      REAL , INTENT( OUT )  root1, root2, root3, root4
  //      LOGICAL, INTENT( IN )  debug

  //!  Check to see if the quartic is actually a cubic

  if (a4 == zero) {
    roots_cubic(a0, a1, a2, a3, tol, nroots, root1, root2, root3, debug);
    root4 = infinity;
    return;
  }

  //!  Use Ferrari's algorithm

  //!  Initialize

  nroots = 0;
  double b1 = a3 / a4;
  double b2 = a2 / a4;
  double b3 = a1 / a4;
  double b4 = a0 / a4;
  double d3 = one;
  double d2 = -b2;
  double d1 = b1 * b3 - four * b4;
  double d0 = b4 * (four * b2 - b1 * b1) - b3 * b3;

  //!  Compute the roots of the auxiliary cubic
  int nrootsc;
  double rootc1, rootc2, rootc3;
  roots_cubic(d0, d1, d2, d3, tol, nrootsc, rootc1, rootc2, rootc3, debug);
  if (nrootsc > 1) {
    rootc1 = rootc3;
  }
  double x1 = b1 * b1 * quarter - b2 + rootc1;
  if (x1 < zero) {
    auto xmd = sqrt(-x1);
    auto xnd = quarter * (two * b3 - b1 * rootc1) / xmd;
    auto alpha = half * b1 * b1 - rootc1 - b2;
    auto beta = four * xnd - b1 * xmd;
    auto r = sqrt(alpha * alpha + beta * beta);
    auto gamma = sqrt(half * (alpha + r));
    double delta = 0.0;
    if (gamma == zero) {
      delta = sqrt(-alpha);
    } else {
      delta = beta * half / gamma;
    }
    root1 = half * (-half * b1 + gamma);
    root2 = half * (xmd + delta);
    root3 = half * (-half * b1 - gamma);
    root4 = half * (xmd - delta);
    goto Label900;
  }
  double xm = 0.0;
  double xn = 0.0;
  if (x1 != zero) {
    xm = sqrt(x1);
    xn = quarter * (b1 * rootc1 - two * b3) / xm;
  } else {
    xm = zero;
    xn = sqrt(quarter * rootc1 * rootc1 - b4);
  }
  auto alpha = half * b1 * b1 - rootc1 - b2;
  auto beta = four * xn - b1 * xm;
  auto gamma = alpha + beta;
  auto delta = alpha - beta;
  auto a = -half * b1;

  //!  compute how many real roots there are

  int type_roots = 1;
  if (gamma >= zero) {
    nroots = nroots + 2;
    type_roots = 0;
    gamma = sqrt(gamma);
  } else {
    gamma = sqrt(-gamma);
  }
  if (delta >= zero) {
    nroots = nroots + 2;
    delta = sqrt(delta);
  } else {
    delta = sqrt(-delta);
  }
  type_roots = nroots + type_roots;

  //!  two real roots

  if (type_roots == 3) {
    root1 = half * (a - xm - delta);
    root2 = half * (a - xm + delta);
    root3 = half * (a + xm);
    root4 = half * gamma;
    goto Label900;
  } else if (type_roots != 4) {
    if (type_roots == 2) {
      root1 = half * (a + xm - gamma);
      root2 = half * (a + xm + gamma);
    } else {
      //!  no real roots
      root1 = half * (a + xm);
      root2 = half * gamma;
    }
    root3 = half * (a - xm) * half;
    root4 = half * delta;
    goto Label900;
  }

  //!  four real roots

  auto b = half * (a + xm + gamma);
  auto d = half * (a - xm + delta);
  auto c = half * (a - xm - delta);
  a = half * (a + xm - gamma);

  //!  sort the roots

  root1 = smallest(a, b, c, d);
  root4 = biggest(a, b, c, d);

  if (a == root1) {
    root2 = smallest(b, c, d);
  } else if (b == root1) {
    root2 = smallest(a, c, d);
  } else if (c == root1) {
    root2 = smallest(a, b, d);
  } else {
    root2 = smallest(a, b, c);
  }

  if (a == root4) {
    root3 = biggest(b, c, d);
  } else if (b == root4) {
    root3 = biggest(a, c, d);
  } else if (c == root4) {
    root3 = biggest(a, b, d);
  } else {
    root3 = biggest(a, b, c);
  }

Label900: // Oops, a label :(

  //!  Perfom a Newton iteration to ensure that the roots are accurate
  if (nroots == 0)
    return;

  auto p = (((a4 * root1 + a3) * root1 + a2) * root1 + a1) * root1 + a0;
  auto pprime =
      ((four * a4 * root1 + three * a3) * root1 + two * a2) * root1 + a1;
  if (pprime != zero) {
    root1 = root1 - p / pprime;
    p = (((a4 * root1 + a3) * root1 + a2) * root1 + a1) * root1 + a0;
  }

  p = (((a4 * root2 + a3) * root2 + a2) * root2 + a1) * root2 + a0;
  pprime = ((four * a4 * root2 + three * a3) * root2 + two * a2) * root2 + a1;
  if (pprime != zero) {
    root2 = root2 - p / pprime;
    p = (((a4 * root2 + a3) * root2 + a2) * root2 + a1) * root2 + a0;
  }

  if (nroots == 4) {
    p = (((a4 * root3 + a3) * root3 + a2) * root3 + a1) * root3 + a0;
    pprime = ((four * a4 * root3 + three * a3) * root3 + two * a2) * root3 + a1;
    if (pprime != zero) {
      root3 = root3 - p / pprime;
      p = (((a4 * root3 + a3) * root3 + a2) * root3 + a1) * root3 + a0;
    }

    p = (((a4 * root4 + a3) * root4 + a2) * root4 + a1) * root4 + a0;
    pprime = ((four * a4 * root4 + three * a3) * root4 + two * a2) * root4 + a1;
    if (pprime != zero) {
      root4 = root4 - p / pprime;
      p = (((a4 * root4 + a3) * root4 + a2) * root4 + a1) * root4 + a0;
    }
  }

} //!  End of subroutine ROOTS_quartic

//! THIS VERSION: RAL_NLLS 1.0 - 22/12/2015 AT 14:15 GMT.
//
//!   R A L _ N L L S _ D T R S  double  M O D U L E
//!
//!  Copyright reserved, Gould/Orban/Toint, for GALAHAD productions
//!  Principal author: Nick Gould
//
//!  History -
//!   extracted from GALAHAD package TRS, December 22nd, 2015
//
//   MODULE RAL_NLLS_DTRS_double
//
//!       -----------------------------------------------
//!      |                                               |
//!      | Solve the trust-region subproblem             |
//!      |                                               |
//!      |    minimize     1/2 <x, H x> + <c, x> + f     |
//!      |    subject to      ||x||_2 <= radius          |
//!      |    or              ||x||_2  = radius          |
//!      |                                               |
//!      | where H is diagonal                           |
//!      |                                               |
//!       -----------------------------------------------

///-*-*-*-*-*-*-  D T R S _ I N I T I A L I Z E   S U B R O U T I N E -*-*-*-*-
/// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
///
///  .  Set initial values for the TRS control parameters  .
///
///  Arguments:
///  =========
///
///   control  a structure containing control information. See DTRS_control_type
///   inform   a structure containing information. See DRQS_inform_type
///
/// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
void dtrs_initialize(dtrs_control_type &control, dtrs_inform_type &inform) {
  //
  //!-----------------------------------------------
  //!   D u m m y   A r g u m e n t
  //!-----------------------------------------------
  //
  //      TYPE ( DTRS_CONTROL_TYPE ), INTENT( OUT )  control
  //      TYPE ( DTRS_inform_type ), INTENT( OUT )  inform

  inform.status = ErrorCode::ral_nlls_ok;

  //!  Set initial control parameter values
  control.stop_normal = pow(epsmch, 0.75);
  control.stop_absolute_normal = pow(epsmch, 0.75);
}

///-*-*-*-*-*-*-*-*-  D T R S _ S O L V E   S U B R O U T I N E  -*-*-*-*-*-*-*-
/// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
///
///  Solve the trust-region subproblem
///
///      minimize     q(x) = 1/2 <x, H x> + <c, x> + f
///      subject to   ||x||_2 <= radius  or ||x||_2 = radius
///
///  where H is diagonal, using a secular iteration
///
/// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
///
///  Arguments:
///  =========
///
///   n - the number of unknowns
///
///   radius - the trust-region radius
///
///   f - the value of constant term for the quadratic function
///
///   C - a vector of values for the linear term c
///
///   H -  a vector of values for the diagonal matrix H
///
///   X - the required solution vector x
///
///   control - a structure containing control information. See
///   DTRS_control_type
///
///   inform - a structure containing information. See DTRS_inform_type
///
/// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
void dtrs_solve(int n, double radius, double f, const DoubleFortranVector &c,
                const DoubleFortranVector &h, DoubleFortranVector &x,
                const dtrs_control_type &control, dtrs_inform_type &inform) {
  //
  //!-----------------------------------------------
  //!   D u m m y   A r g u m e n t s
  //!-----------------------------------------------
  //
  //      INTEGER, INTENT( IN )  n
  //      REAL , INTENT( IN )  radius
  //      REAL , INTENT( IN )  f
  //      REAL , INTENT( IN ), DIMENSION( n )  C, H
  //      REAL , INTENT( OUT ), DIMENSION( n )  X
  //      TYPE ( DTRS_control_type ), INTENT( IN )  control
  //      TYPE ( DTRS_inform_type ), INTENT( INOUT )  inform
  //
  //!  local variables
  //
  //      INTEGER  i
  //      REAL   scale_h, scale_c, f_scale, radius_scale
  //      REAL , DIMENSION( n )  C_scale, H_scale
  //      TYPE ( DTRS_control_type )  control_scale
  //
  //!  scale the problem to solve instead
  //!      minimize    q_s(x_s) = 1/2 <x_s, H_s x_s> + <c_s, x_s> + f_s
  //!      subject to    ||x_s||_2 <= radius_s  or ||x_s||_2 = radius_s

  //!  where H_s = H / s_h and c_s = c / s_c for scale factors s_h and s_c

  //!  This corresponds to
  //!    radius_s = ( s_h / s_c ) radius,
  //!    f_s = ( s_h / s_c^2 ) f
  //!  and the solution may be recovered as
  //!    x = ( s_c / s_h ) x_s
  //!    lambda = s_h lambda_s
  //!    q(x) = ( s_c^2 / s_ h ) q_s(x_s)

  //!  scale H by the largest H and remove relatively tiny H

  DoubleFortranVector h_scale(n);
  auto scale_h = maxAbsVal(h); // MAXVAL( ABS( H ) )
  if (scale_h > zero) {
    for (int i = 1; i <= n; ++i) { // do i = 1, n
      if (fabs(h(i)) >= control.h_min * scale_h) {
        h_scale(i) = h(i) / scale_h;
      } else {
        h_scale(i) = zero;
      }
    }
  } else {
    scale_h = one;
    h_scale.zero();
  }

  //!  scale c by the largest c and remove relatively tiny c

  DoubleFortranVector c_scale(n);
  auto scale_c = maxAbsVal(c); // maxval( abs( c ) )
  if (scale_c > zero) {
    for (int i = 1; i <= n; ++i) { // do i = 1, n
      if (abs(c(i)) >= control.h_min * scale_c) {
        c_scale(i) = c(i) / scale_c;
      } else {
        c_scale(i) = zero;
      }
    }
  } else {
    scale_c = one;
    c_scale.zero();
  }

  double radius_scale = (scale_h / scale_c) * radius;
  double f_scale = (scale_h / pow(scale_c, 2)) * f;

  auto control_scale = control;
  if (control_scale.lower != lower_default) {
    control_scale.lower = control_scale.lower / scale_h;
  }
  if (control_scale.upper != upper_default) {
    control_scale.upper = control_scale.upper / scale_h;
  }

  //!  solve the scaled problem

  dtrs_solve_main(n, radius_scale, f_scale, c_scale, h_scale, x, control_scale,
                  inform);

  //!  unscale the solution, function value, multiplier and related values

  //  x = ( scale_c / scale_h ) * x
  x *= scale_c / scale_h;
  inform.obj *= pow(scale_c, 2) / scale_h;
  inform.multiplier *= scale_h;
  inform.pole *= scale_h;
  for (size_t i = 0; i < inform.history.size();
       ++i) { //      do i = 1, inform.len_history
    inform.history[i].lambda *= scale_h;
    inform.history[i].x_norm *= scale_c / scale_h;
  }

} //!  End of subroutine DTRS_solve

///-*-*-*-*-*-*-*-*-  D T R S _ S O L V E   S U B R O U T I N E  -*-*-*-*-*-*-*-
/// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
///
///  Solve the trust-region subproblem
///
///      minimize     1/2 <x, H x> + <c, x> + f
///      subject to    ||x||_2 <= radius  or ||x||_2 = radius
///
///  where H is diagonal, using a secular iteration
///
/// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
///
///  Arguments:
///  =========
///
///   n - the number of unknowns
///
///   radius - the trust-region radius
///
///   f - the value of constant term for the quadratic function
///
///   C - a vector of values for the linear term c
///
///   H -  a vector of values for the diagonal matrix H
///
///   X - the required solution vector x
///
///   control - a structure containing control information. See
///   DTRS_control_type
///
///   inform - a structure containing information. See DTRS_inform_type
///
/// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
void dtrs_solve_main(int n, double radius, double f,
                     const DoubleFortranVector &c, const DoubleFortranVector &h,
                     DoubleFortranVector &x, const dtrs_control_type &control,
                     dtrs_inform_type &inform) {
  //
  //!-----------------------------------------------
  //!   D u m m y   A r g u m e n t s
  //!-----------------------------------------------
  //
  //      INTEGER, INTENT( IN )  n
  //      REAL , INTENT( IN )  radius
  //      REAL , INTENT( IN )  f
  //      REAL , INTENT( IN ), DIMENSION( n )  C, H
  //      REAL , INTENT( OUT ), DIMENSION( n )  X
  //      TYPE ( DTRS_control_type ), INTENT( IN )  control
  //      TYPE ( DTRS_inform_type ), INTENT( INOUT )  inform
  //
  //!-----------------------------------------------
  //!   L o c a l   V a r i a b l e s
  //!-----------------------------------------------
  //
  //      INTEGER  i, it, out, nroots, print_level
  //      INTEGER  max_order, n_lambda, i_hard
  //      REAL   lambda, lambda_l, lambda_u, delta_lambda
  //      REAL   alpha, utx, distx
  //      REAL   c_norm, c_norm_over_radius, v_norm2, w_norm2
  //      REAL   beta, z_norm2, root1, root2, root3
  //      REAL   lambda_min, lambda_max, lambda_plus, c2
  //      REAL   a_0, a_1, a_2, a_3, a_max
  //      REAL , DIMENSION( 3 )  lambda_new
  //      REAL , DIMENSION( 0 : max_degree )  x_norm2, pi_beta
  //      LOGICAL  printi, printt, printd, problem_file_exists
  //      CHARACTER ( LEN = 1 )  region

  //!  set initial values

  x.zero();
  inform.x_norm = zero;
  inform.obj = f;
  inform.hard_case = false;
  double delta_lambda = zero;
  char region = 'L';

  //!  check for n < 0 or delta < 0
  if (n < 0 || radius < 0) {
    inform.status = ErrorCode::ral_nlls_error_restrictions;
    return;
  }

  DoubleFortranVector x_norm2(0, max_degree), pi_beta(0, max_degree);

  //!  compute the two-norm of c and the extreme eigenvalues of H

  double c_norm = two_norm(c);
  double lambda_min = 0.0;
  double lambda_max = 0.0;
  std::tie(lambda_min, lambda_max) =
      minMaxValues(h); //  minval( h( : n ) ) // maxval( h( : n ) )

  region = 'L';
  double lambda = 0.0;
  //!  check for the trivial case
  if (c_norm == zero && lambda_min >= zero) {
    if (control.equality_problem) {
      int i_hard = 1;                // TODO: is init value of 1 correct?
      for (int i = 1; i <= n; ++i) { // do i = 1, n
        if (h(i) == lambda_min) {
          i_hard = i;
          break;
        };
      }
      x(i_hard) = one / radius;
      inform.x_norm = radius;
      inform.obj = f + lambda_min * radius * radius;
      lambda = -lambda_min;
    } else {
      lambda = zero;
    }
    inform.status = ErrorCode::ral_nlls_ok;
    goto Label900;
  }

  //!  construct values lambda_l and lambda_u for which lambda_l <=
  //!  lambda_optimal
  //!   <= lambda_u, and ensure that all iterates satisfy lambda_l <= lambda
  //!   <= lambda_u

  double c_norm_over_radius = c_norm / radius;
  double lambda_l = 0.0, lambda_u = 0.0;
  if (control.equality_problem) {
    lambda_l =
        biggest(control.lower, -lambda_min, c_norm_over_radius - lambda_max);
    lambda_u = std::min(control.upper, c_norm_over_radius - lambda_min);
  } else {
    lambda_l = biggest(control.lower, zero, -lambda_min,
                        c_norm_over_radius - lambda_max);
    lambda_u = std::min(control.upper,
                        std::max(zero, c_norm_over_radius - lambda_min));
  }
  lambda = lambda_l;

  //!  check for the "hard case"
  if (lambda == -lambda_min) {
    int i_hard = 1; // TODO: is init value of 1 correct?
    double c2 = zero;
    inform.hard_case = true;
    for_do(i, 1, n) {
      if (h(i) == lambda_min) {
        //!           if ( abs( c( i ) ) > epsmch ) {
        if (abs(c(i)) > epsmch * c_norm) {
          inform.hard_case = false;
          c2 = c2 + pow(c(i), 2);
        } else {
          i_hard = i;
        }
      }
    }

    //!  the hard case may occur
    if (inform.hard_case) {
      for_do(i, 1, n) {
        if (h(i) != lambda_min) {
          x(i) = -c(i) / (h(i) + lambda);
        } else {
          x(i) = zero;
        }
      }
      inform.x_norm = two_norm(x);

      //!  the hard case does occur

      if (inform.x_norm <= radius) {
        if (inform.x_norm < radius) {

          //!  compute the step alpha so that x + alpha e_i_hard lies on the
          //!  trust-region
          //!  boundary and gives the smaller value of q

          auto utx = x(i_hard) / radius;
          auto distx =
              (radius - inform.x_norm) * ((radius + inform.x_norm) / radius);
          auto alpha = sign(
              distx / (abs(utx) + sqrt(pow(utx, 2) + distx / radius)), utx);

          //!  record the optimal values

          x(i_hard) = x(i_hard) + alpha;
        }
        inform.x_norm = two_norm(x);
        inform.obj = f + half * (dot_product(c, x) - lambda * pow(radius, 2));
        inform.status = ErrorCode::ral_nlls_ok;
        goto Label900;

        //!  the hard case didn't occur after all
      } else {
        inform.hard_case = false;

        //!  compute the first derivative of ||x|(lambda)||^2 - radius^2
        auto w_norm2 = zero;
        for_do(i, 1, n) {
          if (h(i) != lambda_min)
            w_norm2 = w_norm2 + pow(c(i), 2) / pow((h(i) + lambda), 3);
        }
        x_norm2(1) = -two * w_norm2;

        //!  compute the newton correction

        lambda = lambda + (pow(inform.x_norm, 2) - pow(radius, 2)) / x_norm2(1);
        lambda_l = std::max(lambda_l, lambda);
      }

      //!  there is a singularity at lambda. compute the point for which the
      //!  sum of squares of the singular terms is equal to radius^2
    } else {
      //!         lambda = lambda + sqrt( c2 ) / radius
      lambda = lambda + std::max(sqrt(c2) / radius, lambda * epsmch);
      lambda_l = std::max(lambda_l, lambda);
    }
  }

  //!  the iterates will all be in the L region. Prepare for the main loop
  int it = 0;
  auto max_order = std::max(1, std::min(max_degree, control.taylor_max_degree));

  //!  start the main loop
  for (;;) {
    it = it + 1;

    //!  if h(lambda) is positive definite, solve  h(lambda) x = - c

    for_do(i, 1, n) { x(i) = -c(i) / (h(i) + lambda); }

    //!  compute the two-norm of x

    inform.x_norm = two_norm(x);
    x_norm2(0) = pow(inform.x_norm, 2);

    //!  if the newton step lies within the trust region, exit

    if (lambda == zero && inform.x_norm <= radius) {
      inform.obj = f + half * dot_product(c, x);
      inform.status = ErrorCode::ral_nlls_ok;
      region = 'L';
      goto Label900;
    }

    //!  the current estimate gives a good approximation to the required
    //!  root

    if (abs(inform.x_norm - radius) <=
        std::max(control.stop_normal * radius, control.stop_absolute_normal)) {
      if (inform.x_norm > radius) {
        lambda_l = std::max(lambda_l, lambda);
      } else {
        region = 'G';
        lambda_u = std::min(lambda_u, lambda);
      }
      inform.status = ErrorCode::ral_nlls_ok;
      break;
    }

    lambda_l = std::max(lambda_l, lambda);

    //!  record, for the future, values of lambda which give small ||x||

    if (inform.len_history < history_max) {
      dtrs_history_type history_item;
      history_item.lambda = lambda;
      history_item.x_norm = inform.x_norm;
      inform.history.push_back(history_item);
      inform.len_history = inform.len_history + 1;
    }

    //!  a lambda in L has been found. It is now simply a matter of applying
    //!  a variety of Taylor-series-based methods starting from this lambda

    //!  precaution against rounding producing lambda outside L

    if (lambda > lambda_u) {
      inform.status = ErrorCode::ral_nlls_error_ill_conditioned;
      break;
    }

    //!  compute first derivatives of x^T M x

    //!  form ||w||^2 = x^T H^-1(lambda) x

    double w_norm2 = zero;
    for_do(i, 1, n) {
      w_norm2 = w_norm2 + pow(c(i), 2) / pow(h(i) + lambda, 3);
    }

    //!  compute the first derivative of x_norm2 = x^T M x

    x_norm2(1) = -two * w_norm2;

    //!  compute pi_beta = ||x||^beta and its first derivative when beta = - 1

    double beta = -one;
    dtrs_pi_derivs(1, beta, x_norm2, pi_beta);

    //!  compute the Newton correction (for beta = - 1)

    delta_lambda = -(pi_beta(0) - pow((radius), beta)) / pi_beta(1);

    DoubleFortranVector lambda_new(3);
    int n_lambda = 1;
    lambda_new(n_lambda) = lambda + delta_lambda;

    if (max_order >= 3) {

      //!  compute the second derivative of x^T x

      double z_norm2 = zero;
      for_do(i, 1, n) {
        z_norm2 = z_norm2 + pow(c(i), 2) / pow((h(i) + lambda), 4);
      }
      x_norm2(2) = six * z_norm2;

      //!  compute the third derivatives of x^T x

      double v_norm2 = zero;
      for_do(i, 1, n) {
        v_norm2 = v_norm2 + pow(c(i), 2) / pow((h(i) + lambda), 5);
      }
      x_norm2(3) = -twentyfour * v_norm2;

      //!  compute pi_beta = ||x||^beta and its derivatives when beta = 2

      beta = two;
      dtrs_pi_derivs(max_order, beta, x_norm2, pi_beta);

      //!  compute the "cubic Taylor approximaton" step (beta = 2)

      auto a_0 = pi_beta(0) - pow((radius), beta);
      auto a_1 = pi_beta(1);
      auto a_2 = half * pi_beta(2);
      auto a_3 = sixth * pi_beta(3);
      auto a_max = biggest(fabs(a_0), fabs(a_1), fabs(a_2), fabs(a_3));
      if (a_max > zero) {
        a_0 = a_0 / a_max;
        a_1 = a_1 / a_max;
        a_2 = a_2 / a_max;
        a_3 = a_3 / a_max;
      }
      int nroots = 0;
      double root1 = 0, root2 = 0, root3 = 0;

      roots_cubic(a_0, a_1, a_2, a_3, roots_tol, nroots, root1, root2, root3,
                  roots_debug);
      n_lambda = n_lambda + 1;
      if (nroots == 3) {
        lambda_new(n_lambda) = lambda + root3;
      } else {
        lambda_new(n_lambda) = lambda + root1;
      }

      //!  compute pi_beta = ||x||^beta and its derivatives when beta = - 0.4

      beta = -point4;
      dtrs_pi_derivs(max_order, beta, x_norm2, pi_beta);

      //!  compute the "cubic Taylor approximaton" step (beta = - 0.4)

      a_0 = pi_beta(0) - pow((radius), beta);
      a_1 = pi_beta(1);
      a_2 = half * pi_beta(2);
      a_3 = sixth * pi_beta(3);
      a_max = biggest(fabs(a_0), fabs(a_1), fabs(a_2), fabs(a_3));
      if (a_max > zero) {
        a_0 = a_0 / a_max;
        a_1 = a_1 / a_max;
        a_2 = a_2 / a_max;
        a_3 = a_3 / a_max;
      }
      roots_cubic(a_0, a_1, a_2, a_3, roots_tol, nroots, root1, root2, root3,
                  roots_debug);
      n_lambda = n_lambda + 1;
      if (nroots == 3) {
        lambda_new(n_lambda) = lambda + root3;
      } else {
        lambda_new(n_lambda) = lambda + root1;
      }
    }

    //!  compute the best Taylor improvement

    auto lambda_plus = maxVal(lambda_new, n_lambda);
    delta_lambda = lambda_plus - lambda;
    lambda = lambda_plus;

    //!  improve the lower bound if possible

    lambda_l = std::max(lambda_l, lambda_plus);

    //!  check that the best Taylor improvement is significant

    if (fabs(delta_lambda) < epsmch * std::max(one, fabs(lambda))) {
      inform.status = ErrorCode::ral_nlls_ok;
      break;
    }

    //!  End of main iteration loop
  } // for(;;)

  //!  Record the optimal obective value

  inform.obj = f + half * (dot_product(c, x) - lambda * x_norm2(0));
Label900:
  inform.multiplier = lambda;
  inform.pole = std::max(zero, -lambda_min);

} //      END SUBROUTINE DTRS_solve_main

//!-*-*-*-*-*-*-  D T R S _ P I _ D E R I V S   S U B R O U T I N E -*-*-*-*-*-
//! =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
//!
//!  Compute pi_beta = ||x||^beta and its derivatives
//!
//!  Arguments:
//!  =========
//!
//!  Input -
//!   max_order - maximum order of derivative
//!   beta - power
//!   x_norm2 - (0) value of ||x||^2,
//!             (i) ith derivative of ||x||^2, i = 1, max_order
//!  Output -
//!   pi_beta - (0) value of ||x||^beta,
//!             (i) ith derivative of ||x||^beta, i = 1, max_order
//!
//!  Extracted wholesale from module RAL_NLLS_RQS
//!
//! =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
void dtrs_pi_derivs(int max_order, double beta,
                    const DoubleFortranVector &x_norm2,
                    DoubleFortranVector &pi_beta) {
  //!-----------------------------------------------
  //!   D u m m y   A r g u m e n t s
  //!-----------------------------------------------
  //
  //      INTEGER, INTENT( IN )  max_order
  //      REAL , INTENT( IN )  beta, x_norm2( 0 : max_order )
  //      REAL , INTENT( OUT )  pi_beta( 0 : max_order )

  real hbeta = half * beta;
  pi_beta(0) = pow(x_norm2(0), hbeta);
  pi_beta(1) = hbeta * (pow(x_norm2(0), (hbeta - one))) * x_norm2(1);
  if (max_order == 1)
    return;
  pi_beta(2) = hbeta * (pow(x_norm2(0), (hbeta - two))) *
               ((hbeta - one) * pow(x_norm2(1), 2) + x_norm2(0) * x_norm2(2));
  if (max_order == 2)
    return;
  pi_beta(3) = hbeta * (pow(x_norm2(0), (hbeta - three))) *
               (x_norm2(3) * pow(x_norm2(0), 2) +
                (hbeta - one) * (three * x_norm2(0) * x_norm2(1) * x_norm2(2) +
                                 (hbeta - two) * pow(x_norm2(1), 3)));

  //!  End of subroutine DTRS_pi_derivs
  //
}

//!-*-*-*-*-*  D T R S _ T H E T A _ D E R I V S   S U B R O U T I N E *-*-*-*-
//
//      SUBROUTINE DTRS_theta_derivs( max_order, beta, lambda, sigma, &
//                                     theta_beta )
//
//! =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
//!
//!  Compute theta_beta = (lambda/sigma)^beta and its derivatives
//!
//!  Arguments:
//!  =========
//!
//!  Input -
//!   max_order - maximum order of derivative
//!   beta - power
//!   lambda, sigma - lambda and sigma
//!  Output -
//!   theta_beta - (0) value of (lambda/sigma)^beta,
//!             (i) ith derivative of (lambda/sigma)^beta, i = 1, max_order
//!
//!  Extracted wholesale from module RAL_NLLS_RQS
//!
//! =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
//
//!-----------------------------------------------
//!   D u m m y   A r g u m e n t s
//!-----------------------------------------------
//
//      INTEGER, INTENT( IN )  max_order
//      REAL , INTENT( IN )  beta, lambda, sigma
//      REAL , INTENT( OUT )  theta_beta( 0 : max_order )
//
//!-----------------------------------------------
//!   L o c a l   V a r i a b l e
//!-----------------------------------------------
//
//      REAL   los, oos
//
//      los = lambda / sigma
//      oos = one / sigma
//
//      theta_beta( 0 ) = los ** beta
//      theta_beta( 1 ) = beta * ( los ** ( beta - one ) ) * oos
//      IF ( max_order == 1 ) RETURN
//      theta_beta( 2 ) = beta * ( los ** ( beta - two ) ) * &
//                        ( beta - one ) * oos ** 2
//      IF ( max_order == 2 ) RETURN
//      theta_beta( 3 ) = beta * ( los ** ( beta - three ) ) * &
//                        ( beta - one ) * ( beta - two ) * oos ** 3
//
//      RETURN
//
//!  End of subroutine DTRS_theta_derivs
//
//      END SUBROUTINE DTRS_theta_derivs
//
//!-*-*-*-*-  G A L A H A D   T W O  _ N O R M   F U N C T I O N   -*-*-*-*-
//
//       FUNCTION TWO_NORM( X )
//
//!  Compute the l_2 norm of the vector X
//
//!  Dummy arguments
//
//       REAL   TWO_NORM
//       REAL , INTENT( IN ), DIMENSION( : )  X
//
//!  Local variable
//
//       INTEGER  n
//       n = SIZE( X )
//
//       IF ( n > 0 ) {
//         TWO_NORM = NRM2( n, X, 1 )
//       ELSE
//         TWO_NORM = zero
//       END IF
//       RETURN
//
//!  End of function TWO_NORM
//
//       END FUNCTION TWO_NORM
//
//!-*-*-*-*-*-  End of R A L _ N L L S _ D T R S  double  M O D U L E *-*-*-*-*-
//
//   END MODULE RAL_NLLS_DTRS_double
//
//

} // RalNlls
} // CurveFitting
} // Mantid
