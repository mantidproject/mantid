#include "MantidCurveFitting/RalNlls/DTRS.h"
#include "MantidCurveFitting/FortranDefs.h"

#include <limits>
#include <string>

namespace Mantid {
namespace CurveFitting {
namespace RalNlls {


//! Contains  ral_nlls_roots
//!             ral_dtrs
//! THIS VERSION: RAL_NLLS 1.1 - 07/03/2016 AT 09:45 GMT.
//
//!-*-*-*-*-*-*-*-*-*-  R A L _ N L L S _ R O O T S   M O D U L E  -*-*-*-*-*-*-*-
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

typedef double real;
typedef double REAL;
typedef int integer;
typedef int INTEGER;
typedef bool logical;
typedef bool LOGICAL;

namespace {
const double HUGE = std::numeric_limits<double>::max();

const REAL zero = 0.0;
const REAL one = 1.0;
const REAL two = 2.0;
const REAL three = 3.0;
const REAL four = 4.0;
const REAL six = 6.0;
const REAL quarter = 0.25;
const REAL threequarters = 0.75;
const REAL onesixth = one / six;
const REAL onethird = one / three;
const REAL half = 0.5;
const REAL twothirds = two / three;
//!     REAL pi = four * ATAN( 1.0 );
const REAL pi = 3.1415926535897931;
//!     REAL magic = twothirds * pi
const REAL magic = 2.0943951023931953;  //!! 2 pi/3
const REAL epsmch = std::numeric_limits<double>::epsilon();
const REAL infinity = HUGE;

}

//!  interface to LAPACK: eigenvalues of a Hessenberg matrix
//
//      INTERFACE HSEQR
//        SUBROUTINE SHSEQR( job, compz, n, ilo, ihi, H, ldh,  WR, WI, Z, ldz,   &
//                           WORK, lwork, info )
//        INTEGER, INTENT( IN )  ihi, ilo, ldh, ldz, lwork, n
//        INTEGER, INTENT( OUT )  info
//        CHARACTER ( LEN = 1 ), INTENT( IN )  compz, job
//        REAL, INTENT( INOUT )  H( ldh, * ), Z( ldz, * )
//        REAL, INTENT( OUT )  WI( * ), WR( * ), WORK( * )
//        END SUBROUTINE SHSEQR
//
//        SUBROUTINE DHSEQR( job, compz, n, ilo, ihi, H, ldh,  WR, WI, Z, ldz,   &
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

///-*-*-*-*-*-   R O O T S _ q u a d r a t i c  S U B R O U T I N E   -*-*-*-*-*-
///
/// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
///
///  Find the number and values of real roots of the quadratic equation
///
///                   a2 * x**2 + a1 * x + a0 = 0
///
///  where a0, a1 and a2 are real
///
/// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
void roots_quadratic(double a0, double a1, double a2, double tol, int nroots, 
                     double& root1, double &root2, bool debug ) {

//!  Dummy arguments
//
//      INTEGER, INTENT( OUT )  nroots
//      REAL , INTENT( IN )  a2, a1, a0, tol
//      REAL , INTENT( OUT )  root1, root2
//      LOGICAL, INTENT( IN )  debug

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
//!  Non-executable statements
//
// 2000 FORMAT( ' root ', I1, ': value = ', ES12.4, ' quadratic = ', ES12.4, &
//              ' delta = ', ES12.4 )
// 2010 FORMAT( ' root ', I1, ': value = ', ES12.4, ' quadratic = ', ES12.4 )
//
//
} //!  End of subroutine ROOTS_quadratic

//!-*-*-*-*-*-*-*-   R O O T S _ c u b i c  S U B R O U T I N E   -*-*-*-*-*-*-*-
//
//      SUBROUTINE ROOTS_cubic( a0, a1, a2, a3, tol, nroots, root1, root2,       &
//                              root3, debug )
//
//! =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
//!
//!  Find the number and values of real roots of the cubicc equation
//!
//!                a3 * x**3 + a2 * x**2 + a1 * x + a0 = 0
//!
//!  where a0, a1, a2 and a3 are real
//!
//! =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
//
//!  Dummy arguments
//
//      INTEGER, INTENT( OUT )  nroots
//      REAL , INTENT( IN )  a3, a2, a1, a0, tol
//      REAL , INTENT( OUT )  root1, root2, root3
//      LOGICAL, INTENT( IN )  debug
//
//!  Local variables
//
//      INTEGER  info, nroots_q
//      REAL   a, b, c, d, e, f, p, q, s, t, w, x, y, z
//      REAL   c0, c1, c2, b0, b1, pprime, u1, u2
//      REAL   H( 3, 3 ), ER( 3 ), EI( 3 ), ZZ( 1, 3 ), WORK( 33 )
//
//!  define method used:
//!    1 = Nonweiler, 2 = Littlewood, 3 = Viete, other = companion matrix
//
//      INTEGER, PARAMETER  method = 1
//
//!  Check to see if the quartic is actually a cubic
//
//      IF ( a3 == zero ) THEN
//        CALL ROOTS_quadratic( a0, a1, a2, tol, nroots, root1, root2, debug )
//        root3 = infinity
//        RETURN
//      END IF
//
//!  Deflate the polnomial if the trailing coefficient is zero
//
//      IF ( a0 == zero ) THEN
//        root1 = zero
//        CALL ROOTS_quadratic( a1, a2, a3, tol, nroots, root2, root3, debug )
//        nroots = nroots + 1
//        RETURN
//      END IF
//
//!  1. Use Nonweiler's method (CACM 11:4, 1968, pp269)
//
//      IF ( method == 1 ) THEN
//        c0 = a0 / a3
//        c1 = a1 / a3
//        c2 = a2 / a3
//
//        s = c2 / three
//        t = s * c2
//        b = 0.5_wp * ( s * ( twothirds * t - c1 ) + c0 )
//        t = ( t - c1 ) / three
//        c = t * t * t ; d = b * b - c
//
//! 1 real + 2 equal real or 2 complex roots
//
//        IF ( d >= zero ) THEN
//          d = ( SQRT( d ) + ABS( b ) ) ** onethird
//          IF ( d != zero ) then
//            IF ( b > zero ) then
//              b = - d
//            ELSE
//              b = d
//            END IF
//            c = t / b
//          END IF
//          d = SQRT( threequarters ) * ( b - c )
//          b = b + c ; c = - 0.5 * b - s
//          root1 = b - s
//          IF ( d == zero ) THEN
//            nroots = 3 ; root2 = c ; root3 = c
//          ELSE
//            nroots = 1
//          END IF
//
//! 3 real roots
//
//        ELSE
//          IF ( b == zero ) THEN
//            d = twothirds * ATAN( one )
//          ELSE
//            d = ATAN( SQRT( - d ) / ABS( b ) ) / three
//          END IF
//          IF ( b < zero ) THEN
//            b = two * SQRT( t )
//          ELSE
//            b = - two * SQRT( t )
//          END IF
//          c = COS( d ) * b
//          t = - SQRT( threequarters ) * SIN( d ) * b - half * c
//          d = - t - c - s ; c = c - s ; t = t - s
//          IF ( ABS( c ) > ABS( t ) ) then
//            root3 = c
//          ELSE
//            root3 = t
//            t = c
//          END IF
//          IF ( ABS( d ) > ABS( t ) ) THEN
//            root2 = d
//          ELSE
//            root2 = t
//            t = d
//          END IF
//          root1 = t ; nroots = 3
//        END IF
//
//!  2. Use Littlewood's method
//
//      ELSE IF ( method == 2 ) THEN
//        c2 = a2 / ( three * a3 ) ; c1 = a1 / ( three * a3 ) ; c0 = a0 / a3
//        x = c1 - c2 * c2
//        y = c0 - c2* ( x + x + c1 )
//        z = y ** 2 + four * x ** 3
//
//!  there are three real roots
//
//        IF ( z < zero ) THEN
//          a = - two * SQRT( - x )
//          b = y / ( a * x )
//          y = ATAN2( SQRT( one - b ), SQRT( one + b ) ) * twothirds
//          IF ( c2 < zero ) y = y + magic
//
//!  calculate root which does not involve cancellation
//
//          nroots = 1 ; root1 = a * COS( y ) - c2
//
//!  there may be only one real root
//
//        ELSE
//          a = SQRT( z ) ; b = half * ( ABS( y ) + a ) ; c = b ** onethird
//          IF ( c <= zero ) THEN
//            nroots = 3 ; root1 = - c2 ; root2 = - c2 ; root3 = - c2
//            GO TO 900
//          ELSE
//            nroots = 1
//            c = c - ( c ** 3 - b ) / ( three * c * c )
//            e = c * c + ABS( x )
//            f = one / ( ( x / c ) ** 2 + e )
//            IF ( x >= zero ) THEN
//              x = e / c ; z = y * f
//            ELSE
//              x = a * f ; z = SIGN( one, y ) * e / c
//            END IF
//            IF ( z * c2 >= zero ) THEN
//              root1 = - z - c2
//            ELSE
//              root2 = half * z - c2
//              root3 = half * SQRT( three ) * ABS( x )
//              root1 = - c0 / ( root2 * root2 + root3 * root3 )
//              GO TO 900
//            END IF
//          END IF
//        END IF
//
//!  deflate cubic
//
//        b0 = - c0 / root1
//        IF ( ABS( root1 ** 3 ) <= ABS( c0 ) ) THEN
//          b1 = root1 + three * c2
//        ELSE
//          b1 = ( b0 - three * c1 ) / root1
//        END IF
//        CALL ROOTS_quadratic( b0, b1, one, epsmch, nroots_q,                   &
//                              root2, root3, debug )
//        nroots = nroots + nroots_q
//
//
//!  3. Use Viete's method
//
//      ELSE IF ( method == 3 ) THEN
//        w = a2 / ( three * a3 )
//        p = ( a1 / ( three * a3 ) - w ** 2 ) ** 3
//        q = - half * ( two * w ** 3 - ( a1 * w - a0 ) / a3 )
//        d = p + q ** 2
//
//!  three real roots
//
//        IF ( d < zero ) THEN
//          s = ACOS( MIN( one, MAX( - one, q / SQRT( - p ) ) ) )
//          p = two * ( - p ) ** onesixth
//          nroots = 3
//          root1 = p * COS( onethird * ( s + two * pi ) ) - w
//          root2 = p * COS( onethird * ( s + four * pi ) ) - w
//          root3 = p * COS( onethird * ( s + six * pi ) ) - w
//
//!  one real root
//
//        ELSE
//          d = SQRT( d ) ; u1 = q + d ; u2 = q - d
//          nroots = 1
//          root1 = SIGN( ABS( u1 ) ** onethird, u1 ) +                          &
//                  SIGN( ABS( u2 ) ** onethird, u2 ) - w
//        END IF
//
//!  4. Compute the roots as the eigenvalues of the relevant compainion matrix
//
//      ELSE
//        H( 1, 1 ) = zero ; H( 2, 1 ) = one ; H( 3, 1 ) = zero
//        H( 1, 2 ) = zero ; H( 2, 2 ) = zero ; H( 3, 2 ) = one
//        H( 1, 3 ) = - a0 / a3 ; H( 2, 3 ) = - a1 / a3 ; H( 3, 3 ) = - a2 / a3
//        CALL HSEQR( 'E', 'N', 3, 1, 3, H, 3, ER, EI, ZZ, 1, WORK, 33, info )
//        IF ( info != 0 ) THEN
//          IF ( debug ) WRITE( out,                                             &
//         &   "( ' ** error return ', I0, ' from HSEQR in ROOTS_cubic' )" ) info
//          nroots = 0
//          RETURN
//        END IF
//
//!  count and record the roots
//
//        nroots = COUNT( ABS( EI ) <= epsmch )
//        IF ( nroots == 1 ) THEN
//          IF (  ABS( EI( 1 ) ) <= epsmch ) THEN
//            root1 = ER( 1 )
//          ELSE IF (  ABS( EI( 2 ) ) <= epsmch ) THEN
//            root1 = ER( 2 )
//          ELSE
//            root1 = ER( 3 )
//          END IF
//        ELSE
//          root1 = ER( 1 ) ;  root2 = ER( 2 ) ;  root3 = ER( 3 )
//        END IF
//      END IF
//
//!  reorder the roots
//
//  900 CONTINUE
//      IF ( nroots == 3 ) THEN
//        IF ( root1 > root2 ) THEN
//          a = root2 ; root2 = root1 ; root1 = a
//        END IF
//        IF ( root2 > root3 ) THEN
//          a = root3
//          IF ( root1 > root3 ) THEN
//            a = root1 ; root1 = root3
//          END IF
//          root3 = root2 ; root2 = a
//        END IF
//        IF ( debug ) WRITE( out, "( ' 3 real roots ' )" )
//      ELSE IF ( nroots == 2 ) THEN
//        IF ( debug ) WRITE( out, "( ' 2 real roots ' )" )
//      ELSE
//        IF ( debug ) WRITE( out, "( ' 1 real root ' )" )
//      END IF
//
//!  perfom a Newton iteration to ensure that the roots are accurate
//
//      p = ( ( a3 * root1 + a2 ) * root1 + a1 ) * root1 + a0
//      pprime = ( three * a3 * root1 + two * a2 ) * root1 + a1
//      IF ( pprime != zero ) THEN
//        IF ( debug ) WRITE( out, 2000 ) 1, root1, p, - p / pprime
//        root1 = root1 - p / pprime
//        p = ( ( a3 * root1 + a2 ) * root1 + a1 ) * root1 + a0
//      END IF
//      IF ( debug ) WRITE( out, 2010 ) 1, root1, p
//
//      IF ( nroots == 3 ) THEN
//        p = ( ( a3 * root2 + a2 ) * root2 + a1 ) * root2 + a0
//        pprime = ( three * a3 * root2 + two * a2 ) * root2 + a1
//        IF ( pprime != zero ) THEN
//          IF ( debug ) WRITE( out, 2000 ) 2, root2, p, - p / pprime
//          root2 = root2 - p / pprime
//          p = ( ( a3 * root2 + a2 ) * root2 + a1 ) * root2 + a0
//        END IF
//        IF ( debug ) WRITE( out, 2010 ) 2, root2, p
//
//        p = ( ( a3 * root3 + a2 ) * root3 + a1 ) * root3 + a0
//        pprime = ( three * a3 * root3 + two * a2 ) * root3 + a1
//        IF ( pprime != zero ) THEN
//          IF ( debug ) WRITE( out, 2000 ) 3, root3, p, - p / pprime
//          root3 = root3 - p / pprime
//          p = ( ( a3 * root3 + a2 ) * root3 + a1 ) * root3 + a0
//        END IF
//        IF ( debug ) WRITE( out, 2010 ) 3, root3, p
//      END IF
//
//      RETURN
//
//!  Non-executable statements
//
// 2000 FORMAT( ' root ', I1, ': value = ', ES12.4, ' cubic = ', ES12.4,         &
//              ' delta = ', ES12.4 )
// 2010 FORMAT( ' root ', I1, ': value = ', ES12.4, ' cubic = ', ES12.4 )
//
//
//!  End of subroutine ROOTS_cubic
//
//      END SUBROUTINE ROOTS_cubic
//
//!-*-*-*-*-*-*-   R O O T S _ q u a r t i c   S U B R O U T I N E   -*-*-*-*-*-*-
//
//      SUBROUTINE ROOTS_quartic( a0, a1, a2, a3, a4, tol, nroots, root1, root2, &
//                                root3, root4, debug )
//
//! =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
//!
//!  Find the number and values of real roots of the quartic equation
//!
//!        a4 * x**4 + a3 * x**3 + a2 * x**2 + a1 * x + a0 = 0
//!
//!  where a0, a1, a2, a3 and a4 are real
//!
//! =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
//
//!  Dummy arguments
//
//      INTEGER, INTENT( OUT )  nroots
//      REAL , INTENT( IN )  a4, a3, a2, a1, a0, tol
//      REAL , INTENT( OUT )  root1, root2, root3, root4
//      LOGICAL, INTENT( IN )  debug
//
//!  Local variables
//
//      INTEGER  type_roots, nrootsc
//      REAL   a, alpha, b, beta, c, d, delta, gamma, r
//      REAL   x1, xm, xmd, xn, xnd
//      REAL   d3, d2, d1, d0, b4, b3, b2, b1
//      REAL   rootc1, rootc2, rootc3, p, pprime
//
//!  Check to see if the quartic is actually a cubic
//
//      IF ( a4 == zero ) THEN
//        CALL ROOTS_cubic( a0, a1, a2, a3, tol, nroots, root1, root2, root3,    &
//                          debug )
//        root4 = infinity
//        RETURN
//      END IF
//
//!  Use Ferrari's algorithm
//
//!  Initialize
//
//      nroots = 0
//      b1 = a3 / a4
//      b2 = a2 / a4
//      b3 = a1 / a4
//      b4 = a0 / a4
//      d3 = one
//      d2 =  - b2
//      d1 = b1 * b3 - four * b4
//      d0 = b4 * ( four * b2 - b1 * b1 ) - b3 * b3
//
//!  Compute the roots of the auxiliary cubic
//
//      CALL ROOTS_cubic( d0, d1, d2, d3, tol, nrootsc, rootc1, rootc2, rootc3, &
//                        debug )
//      IF ( nrootsc > 1 ) rootc1 = rootc3
//      x1 = b1 * b1 * quarter - b2 + rootc1
//      IF ( x1 < zero ) THEN
//        xmd = SQRT( - x1 )
//        xnd = quarter * ( two * b3 - b1 * rootc1 ) / xmd
//        alpha = half * b1 * b1 - rootc1 - b2
//        beta = four * xnd - b1 * xmd
//        r = SQRT( alpha * alpha + beta * beta )
//        gamma = SQRT( half * ( alpha + r ) )
//        IF ( gamma == zero ) THEN
//          delta = SQRT( - alpha )
//        ELSE
//          delta = beta * half / gamma
//        END IF
//        root1 = half * ( - half * b1 + gamma )
//        root2 = half * ( xmd + delta )
//        root3 = half * ( - half * b1 - gamma )
//        root4 = half * ( xmd - delta )
//        GO TO 900
//      END IF
//      IF ( x1 != zero ) THEN
//        xm = SQRT( x1 )
//        xn = quarter * ( b1 * rootc1 - two * b3 ) / xm
//      ELSE
//        xm = zero
//        xn = SQRT( quarter * rootc1 * rootc1 - b4 )
//      END IF
//      alpha = half * b1 * b1 - rootc1 - b2
//      beta = four * xn - b1 * xm
//      gamma = alpha + beta
//      delta = alpha - beta
//      a = - half * b1
//
//!  Compute how many real roots there are
//
//      type_roots = 1
//      IF ( gamma >= zero ) THEN
//        nroots = nroots + 2
//        type_roots = 0
//        gamma = SQRT( gamma )
//      ELSE
//        gamma = SQRT( - gamma )
//      END IF
//      IF ( delta >= zero ) THEN
//        nroots = nroots + 2
//        delta = SQRT( delta )
//      ELSE
//        delta = SQRT( - delta )
//      END IF
//      type_roots = nroots + type_roots
//
//!  Two real roots
//
//      IF ( type_roots == 3 ) THEN
//        root1 = half * ( a - xm - delta )
//        root2 = half * ( a - xm + delta )
//        root3 = half * ( a + xm )
//        root4 = half * gamma
//        GO TO 900
//      ELSE IF ( type_roots != 4 ) THEN
//        IF ( type_roots == 2 ) THEN
//          root1 = half * ( a + xm - gamma )
//          root2 = half * ( a + xm + gamma )
//        ELSE
//
//!  No real roots
//
//          root1 = half * ( a + xm )
//          root2 = half * gamma
//        END IF
//        root3 = half * ( a - xm ) * half
//        root4 = half * delta
//        GO TO 900
//      END IF
//
//!  Four real roots
//
//      b = half * ( a + xm + gamma )
//      d = half * ( a - xm + delta )
//      c = half * ( a - xm - delta )
//      a = half * ( a + xm - gamma )
//
//!  Sort the roots
//
//      root1 = MIN( a, b, c, d )
//      root4 = MAX( a, b, c, d )
//
//      IF ( a == root1 ) THEN
//        root2 = MIN( b, c, d )
//      ELSE IF ( b == root1 ) THEN
//        root2 = MIN( a, c, d )
//      ELSE IF ( c == root1 ) THEN
//        root2 = MIN( a, b, d )
//      ELSE
//        root2 = MIN( a, b, c )
//      END IF
//
//      IF ( a == root4 ) THEN
//        root3 = MAX( b, c, d )
//      ELSE IF ( b == root4 ) THEN
//        root3 = MAX( a, c, d )
//      ELSE IF ( c == root4 ) THEN
//        root3 = MAX( a, b, d )
//      ELSE
//        root3 = MAX( a, b, c )
//      END IF
//
//  900 CONTINUE
//
//!  Perfom a Newton iteration to ensure that the roots are accurate
//
//      IF ( debug ) THEN
//        IF ( nroots == 0 ) THEN
//          WRITE( out, "( ' no real roots ' )" )
//        ELSE IF ( nroots == 2 ) THEN
//          WRITE( out, "( ' 2 real roots ' )" )
//        ELSE IF ( nroots == 4 ) THEN
//          WRITE( out, "( ' 4 real roots ' )" )
//        END IF
//      END IF
//      IF ( nroots == 0 ) RETURN
//
//      p = ( ( ( a4 * root1 + a3 ) * root1 + a2 ) * root1 + a1 ) * root1 + a0
//      pprime = ( ( four * a4 * root1 + three * a3 ) * root1 + two * a2 )       &
//                 * root1 + a1
//      IF ( pprime != zero ) THEN
//        IF ( debug ) WRITE( out, 2000 ) 1, root1, p, - p / pprime
//        root1 = root1 - p / pprime
//        p = ( ( ( a4 * root1 + a3 ) * root1 + a2 ) * root1 + a1 ) * root1 + a0
//      END IF
//      IF ( debug ) WRITE( out, 2010 ) 1, root1, p
//
//      p = ( ( ( a4 * root2 + a3 ) * root2 + a2 ) * root2 + a1 ) * root2 + a0
//      pprime = ( ( four * a4 * root2 + three * a3 ) * root2 + two * a2 )       &
//                 * root2 + a1
//      IF ( pprime != zero ) THEN
//        IF ( debug ) WRITE( out, 2000 ) 2, root2, p, - p / pprime
//        root2 = root2 - p / pprime
//        p = ( ( ( a4 * root2 + a3 ) * root2 + a2 ) * root2 + a1 ) * root2 + a0
//      END IF
//      IF ( debug ) WRITE( out, 2010 ) 2, root2, p
//
//      IF ( nroots == 4 ) THEN
//        p = ( ( ( a4 * root3 + a3 ) * root3 + a2 ) * root3 + a1 ) * root3 + a0
//        pprime = ( ( four * a4 * root3 + three * a3 ) * root3 + two * a2 )     &
//                   * root3 + a1
//        IF ( pprime != zero ) THEN
//          IF ( debug ) WRITE( out, 2000 ) 3, root3, p, - p / pprime
//          root3 = root3 - p / pprime
//          p = ( ( ( a4 * root3 + a3 ) * root3 + a2 ) * root3 + a1 ) * root3 + a0
//        END IF
//        IF ( debug ) WRITE( out, 2010 ) 3, root3, p
//
//        p = ( ( ( a4 * root4 + a3 ) * root4 + a2 ) * root4 + a1 ) * root4 + a0
//        pprime = ( ( four * a4 * root4 + three * a3 ) * root4 + two * a2 )     &
//                   * root4 + a1
//        IF ( pprime != zero ) THEN
//          IF ( debug ) WRITE( out, 2000 ) 4, root4, p, - p / pprime
//          root4 = root4 - p / pprime
//          p = ( ( ( a4 * root4 + a3 ) * root4 + a2 ) * root4 + a1 ) * root4 + a0
//        END IF
//        IF ( debug ) WRITE( out, 2010 ) 4, root4, p
//      END IF
//
//      RETURN
//
//!  Non-executable statements
//
// 2000 FORMAT( ' root ', I1, ': value = ', ES12.4, ' quartic = ', ES12.4,       &
//              ' delta = ', ES12.4 )
// 2010 FORMAT( ' root ', I1, ': value = ', ES12.4, ' quartic = ', ES12.4 )
//
//!  End of subroutine ROOTS_quartic
//
//      END SUBROUTINE ROOTS_quartic
//
//!  End of module ROOTS
//
//   END MODULE RAL_NLLS_ROOTS_double
//
//! THIS VERSION: RAL_NLLS 1.0 - 22/12/2015 AT 14:15 GMT.
//
//!-*-*-*-*-*-*-*-  R A L _ N L L S _ D T R S  double  M O D U L E  *-*-*-*-*-*-*-
//
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
//
//      USE RAL_NLLS_SYMBOLS
//      USE RAL_NLLS_ROOTS_double, ONLY: ROOTS_cubic
//
//      IMPLICIT NONE
//
//      PRIVATE
//      PUBLIC  DTRS_initialize, DTRS_solve, DTRS_solve_main
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
//      INTEGER, PARAMETER  history_max = 100
//      INTEGER, PARAMETER  max_degree = 3
//      REAL , PARAMETER  zero = 0.0_wp
//      REAL , PARAMETER  half = 0.5_wp
//      REAL , PARAMETER  point4 = 0.4_wp
//      REAL , PARAMETER  one = 1.0_wp
//      REAL , PARAMETER  two = 2.0_wp
//      REAL , PARAMETER  three = 3.0_wp
//      REAL , PARAMETER  six = 6.0_wp
//      REAL , PARAMETER  sixth = one / six
//      REAL , PARAMETER  ten = 10.0_wp
//      REAL , PARAMETER  twentyfour = 24.0_wp
//      REAL , PARAMETER  largest = HUGE( one )
//      REAL , PARAMETER  lower_default = - half * largest
//      REAL , PARAMETER  upper_default = largest
//      REAL , PARAMETER  epsmch = EPSILON( one )
//      REAL , PARAMETER  teneps = ten * epsmch
//      REAL , PARAMETER  roots_tol = teneps
//      LOGICAL  roots_debug = .FALSE.
//
//!--------------------------
//!  Derived type definitions
//!--------------------------
//
//!  - - - - - - - - - - - - - - - - - - - - - - -
//!   control derived type with component defaults
//!  - - - - - - - - - - - - - - - - - - - - - - -
//
//      TYPE, PUBLIC  DTRS_control_type
//
//!  unit for error messages
//
//        INTEGER  error = 6
//
//!  unit for monitor output
//
//        INTEGER  out = 6
//
//!  unit to write problem data into file problem_file
//
//        INTEGER  problem = 0
//
//!  controls level of diagnostic output
//
//        INTEGER  print_level = 0
//
//!  maximum degree of Taylor approximant allowed
//
//        INTEGER  taylor_max_degree = 3
//
//!  any entry of H that is smaller than h_min * MAXVAL( H ) we be treated as zero
//
//        REAL   h_min = epsmch
//
//!  any entry of C that is smaller than c_min * MAXVAL( C ) we be treated as zero
//
//        REAL   c_min = epsmch
//
//!  lower and upper bounds on the multiplier, if known
//
//        REAL   lower = lower_default
//        REAL   upper = upper_default
//
//!  stop when | ||x|| - radius | <=
//!     max( stop_normal * radius, stop_absolute_normal )
//
//        REAL   stop_normal = epsmch
//        REAL   stop_absolute_normal = epsmch
//
//!  is the solution is REQUIRED to lie on the boundary (i.e., is the constraint
//!  an equality)?
//
//        LOGICAL  equality_problem= .FALSE.
//
//!  name of file into which to write problem data
//
//        CHARACTER ( LEN = 30 )  problem_file =                               &
//         'trs_problem.data' // REPEAT( ' ', 14 )
//
//!  all output lines will be prefixed by
//!    prefix(2:LEN(TRIM(%prefix))-1)
//!  where prefix contains the required string enclosed in quotes,
//!  e.g. "string" or 'string'
//
//        CHARACTER ( LEN = 30 )  prefix  = '""                            '
//
//      END TYPE
//
//!  - - - - - - - - - - - - - - - - - - - - - - - -
//!   history derived type with component defaults
//!  - - - - - - - - - - - - - - - - - - - - - - - -
//
//      TYPE, PUBLIC  DTRS_history_type
//
//!  value of lambda
//
//        REAL   lambda = zero
//
//!  corresponding value of ||x(lambda)||_M
//
//        REAL   x_norm = zero
//      END TYPE
//
//!  - - - - - - - - - - - - - - - - - - - - - - -
//!   inform derived type with component defaults
//!  - - - - - - - - - - - - - - - - - - - - - - -
//
//      TYPE, PUBLIC  DTRS_inform_type
//
//!   reported return status:
//!      0 the solution has been found
//!     -3 n and/or Delta is not positive
//!    -16 ill-conditioning has prevented furthr progress
//
//        INTEGER  status = 0
//
//!  the number of (||x||_M,lambda) pairs in the history
//
//        INTEGER  len_history = 0
//
//!  the value of the quadratic function
//
//        REAL   obj = HUGE( one )
//
//!  the M-norm of x, ||x||_M
//
//        REAL   x_norm = zero
//
//!  the Lagrange multiplier corresponding to the trust-region constraint
//
//        REAL   multiplier = zero
//
//!  a lower bound max(0,-lambda_1), where lambda_1 is the left-most
//!  eigenvalue of (H,M)
//
//        REAL   pole = zero
//
//!  has the hard case occurred?
//
//        LOGICAL  hard_case = .FALSE.
//
//!  history information
//
//        TYPE ( DTRS_history_type ), DIMENSION( history_max )  history
//      END TYPE
//
//!  interface to BLAS: two norm
//
//     INTERFACE NRM2
//
//       FUNCTION SNRM2( n, X, incx )
//       REAL  SNRM2
//       INTEGER, INTENT( IN )  n, incx
//       REAL, INTENT( IN ), DIMENSION( incx * ( n - 1 ) + 1 )  X
//       END FUNCTION SNRM2
//
//       FUNCTION DNRM2( n, X, incx )
//       DOUBLE PRECISION  DNRM2
//       INTEGER, INTENT( IN )  n, incx
//       DOUBLE PRECISION, INTENT( IN ), DIMENSION( incx * ( n - 1 ) + 1 )  X
//       END FUNCTION DNRM2
//
//     END INTERFACE
//
//    CONTAINS
//
//!-*-*-*-*-*-*-  D T R S _ I N I T I A L I Z E   S U B R O U T I N E   -*-*-*-*-
//
//      SUBROUTINE DTRS_initialize( control, inform )
//
//! =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
//!
//!  .  Set initial values for the TRS control parameters  .
//!
//!  Arguments:
//!  =========
//!
//!   control  a structure containing control information. See DTRS_control_type
//!   inform   a structure containing information. See DRQS_inform_type
//!
//! =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
//
//!-----------------------------------------------
//!   D u m m y   A r g u m e n t
//!-----------------------------------------------
//
//      TYPE ( DTRS_CONTROL_TYPE ), INTENT( OUT )  control
//      TYPE ( DTRS_inform_type ), INTENT( OUT )  inform
//
//      inform%status = RAL_NLLS_ok
//
//!  Set initial control parameter values
//
//      control%stop_normal = epsmch ** 0.75
//      control%stop_absolute_normal = epsmch ** 0.75
//
//      RETURN
//
//!  End of subroutine DTRS_initialize
//
//      END SUBROUTINE DTRS_initialize
//
//!-*-*-*-*-*-*-*-*-  D T R S _ S O L V E   S U B R O U T I N E  -*-*-*-*-*-*-*-
//
//      SUBROUTINE DTRS_solve( n, radius, f, C, H, X, control, inform )
//
//! =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
//!
//!  Solve the trust-region subproblem
//!
//!      minimize     q(x) = 1/2 <x, H x> + <c, x> + f
//!      subject to   ||x||_2 <= radius  or ||x||_2 = radius
//!
//!  where H is diagonal, using a secular iteration
//!
//! =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
//!
//!  Arguments:
//!  =========
//!
//!   n - the number of unknowns
//!
//!   radius - the trust-region radius
//!
//!   f - the value of constant term for the quadratic function
//!
//!   C - a vector of values for the linear term c
//!
//!   H -  a vector of values for the diagonal matrix H
//!
//!   X - the required solution vector x
//!
//!   control - a structure containing control information. See DTRS_control_type
//!
//!   inform - a structure containing information. See DTRS_inform_type
//!
//! =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
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
//
//!      minimize    q_s(x_s) = 1/2 <x_s, H_s x_s> + <c_s, x_s> + f_s
//!      subject to    ||x_s||_2 <= radius_s  or ||x_s||_2 = radius_s
//
//!  where H_s = H / s_h and c_s = c / s_c for scale factors s_h and s_c
//
//!  This corresponds to
//!    radius_s = ( s_h / s_c ) radius,
//!    f_s = ( s_h / s_c^2 ) f
//!  and the solution may be recovered as
//!    x = ( s_c / s_h ) x_s
//!    lambda = s_h lambda_s
//!    q(x) = ( s_c^2 / s_ h ) q_s(x_s)
//
//!write(6,"( A2, 5ES13.4E3 )" ) 'H', H
//!write(6,"( A2, 5ES13.4E3 )" ) 'C', C
//
//!  scale H by the largest H and remove relatively tiny H
//
//      scale_h = MAXVAL( ABS( H ) )
//      IF ( scale_h > zero ) THEN
//        DO i = 1, n
//          IF ( ABS( H( i ) ) >= control%h_min * scale_h ) THEN
//            H_scale( i ) = H( i ) / scale_h
//          ELSE
//            H_scale( i ) = zero
//          END IF
//        END DO
//      ELSE
//        scale_h = one
//        H_scale = zero
//      END IF
//
//!  scale c by the largest c and remove relatively tiny c
//
//      scale_c = MAXVAL( ABS( C ) )
//      IF ( scale_c > zero ) THEN
//        DO i = 1, n
//          IF ( ABS( C( i ) ) >= control%h_min * scale_c ) THEN
//            C_scale( i ) = C( i ) / scale_c
//          ELSE
//            C_scale( i ) = zero
//          END IF
//        END DO
//      ELSE
//        scale_c = one
//        C_scale = zero
//      END IF
//
//      radius_scale = ( scale_h / scale_c ) * radius
//      f_scale = ( scale_h / scale_c ** 2 ) * f
//
//      control_scale = control
//      IF ( control_scale%lower != lower_default )                              &
//        control_scale%lower = control_scale%lower / scale_h
//      IF ( control_scale%upper != upper_default )                              &
//        control_scale%upper = control_scale%upper / scale_h
//
//!  solve the scaled problem
//
//      CALL DTRS_solve_main( n, radius_scale, f_scale, C_scale, H_scale, X,     &
//                            control_scale, inform )
//
//!  unscale the solution, function value, multiplier and related values
//
//      X = ( scale_c / scale_h ) * X
//      inform%obj = ( scale_c ** 2 / scale_h ) * inform%obj
//      inform%multiplier = scale_h * inform%multiplier
//      inform%pole = scale_h * inform%pole
//      DO i = 1, inform%len_history
//        inform%history( i )%lambda = scale_h * inform%history( i )%lambda
//        inform%history( i )%x_norm                                             &
//          = ( scale_c / scale_h ) * inform%history( i )%x_norm
//      END DO
//
//!  End of subroutine DTRS_solve
//
//      END SUBROUTINE DTRS_solve
//
//!-*-*-*-*-*-*-*-*-  D T R S _ S O L V E   S U B R O U T I N E  -*-*-*-*-*-*-*-
//
//      SUBROUTINE DTRS_solve_main( n, radius, f, C, H, X, control, inform )
//
//! =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
//!
//!  Solve the trust-region subproblem
//!
//!      minimize     1/2 <x, H x> + <c, x> + f
//!      subject to    ||x||_2 <= radius  or ||x||_2 = radius
//!
//!  where H is diagonal, using a secular iteration
//!
//! =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
//!
//!  Arguments:
//!  =========
//!
//!   n - the number of unknowns
//!
//!   radius - the trust-region radius
//!
//!   f - the value of constant term for the quadratic function
//!
//!   C - a vector of values for the linear term c
//!
//!   H -  a vector of values for the diagonal matrix H
//!
//!   X - the required solution vector x
//!
//!   control - a structure containing control information. See DTRS_control_type
//!
//!   inform - a structure containing information. See DTRS_inform_type
//!
//! =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
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
//
//!  prefix for all output
//
//      CHARACTER ( LEN = LEN( TRIM( control%prefix ) ) - 2 )  prefix
//      IF ( LEN( TRIM( control%prefix ) ) > 2 )                                 &
//        prefix = control%prefix( 2 : LEN( TRIM( control%prefix ) ) - 1 )
//
//!write(6,"( A2, 5ES13.4E3 )" ) 'H', H
//!write(6,"( A2, 5ES13.4E3 )" ) 'C', C
//!write(6,"( A, ES13.4E3 )" ) 'radius', radius
//
//!  output problem data
//
//      IF ( control%problem > 0 ) THEN
//        INQUIRE( FILE = control%problem_file, EXIST = problem_file_exists )
//        IF ( problem_file_exists ) THEN
//          OPEN( control%problem, FILE = control%problem_file,                  &
//                FORM = 'FORMATTED', STATUS = 'OLD' )
//          REWIND control%problem
//        ELSE
//          OPEN( control%problem, FILE = control%problem_file,                  &
//                FORM = 'FORMATTED', STATUS = 'NEW' )
//        END IF
//        WRITE( control%problem, * ) n, COUNT( C( : n ) != zero ),              &
//          COUNT( H( : n ) != zero )
//        WRITE( control%problem, * ) radius, f
//        DO i = 1, n
//          IF ( C( i ) != zero ) WRITE( control%problem, * ) i, C( i )
//        END DO
//        DO i = 1, n
//          IF ( H( i ) != zero ) WRITE( control%problem, * ) i, i, H( i )
//        END DO
//        CLOSE( control%problem )
//      END IF
//
//!  set initial values
//
//      X = zero ; inform%x_norm = zero ; inform%obj = f
//      inform%hard_case = .FALSE.
//      delta_lambda = zero
//
//!  record desired output level
//
//      out = control%out
//      print_level = control%print_level
//      printi = out > 0 .AND. print_level > 0
//      printt = out > 0 .AND. print_level > 1
//      printd = out > 0 .AND. print_level > 2
//
//!  check for n < 0 or Delta < 0
//
//      IF ( n < 0 .or. radius < 0 ) THEN
//         inform%status = RAL_NLLS_error_restrictions
//         RETURN
//      END IF
//
//!  compute the two-norm of c and the extreme eigenvalues of H
//
//      c_norm = TWO_NORM( C )
//      lambda_min = MINVAL( H( : n ) )
//      lambda_max = MAXVAL( H( : n ) )
//
//      IF ( printt ) WRITE( out, "( A, ' ||c|| = ', ES10.4, ', ||H|| = ',       &
//     &                             ES10.4, ', lambda_min = ', ES11.4 )" )      &
//          prefix, c_norm, MAXVAL( ABS( H( : n ) ) ), lambda_min
//
//      region = 'L'
//      IF ( printt )                                                            &
//        WRITE( out, "( A, 4X, 28( '-' ), ' phase two ', 28( '-' ) )" ) prefix
//      IF ( printi ) WRITE( out, 2030 ) prefix
//
//!  check for the trivial case
//
//      IF ( c_norm == zero .AND. lambda_min >= zero ) THEN
//        IF (  control%equality_problem ) THEN
//          DO i = 1, n
//            IF ( H( i ) == lambda_min ) THEN
//              i_hard = i
//              EXIT
//            END IF
//          END DO
//          X( i_hard ) = one / radius
//          inform%x_norm = radius
//          inform%obj = f + lambda_min * radius ** 2
//          lambda = - lambda_min
//        ELSE
//          lambda = zero
//        END IF
//        IF ( printi ) THEN
//          WRITE( out, "( A, A2, I4, 3ES22.15 )" )  prefix, region,             &
//          0, ABS( inform%x_norm - radius ), lambda, ABS( delta_lambda )
//          WRITE( out, "( A,                                                    &
//       &    ' Normal stopping criteria satisfied' )" ) prefix
//        END IF
//        inform%status = RAL_NLLS_ok
//        GO TO 900
//      END IF
//
//!  construct values lambda_l and lambda_u for which lambda_l <= lambda_optimal
//!   <= lambda_u, and ensure that all iterates satisfy lambda_l <= lambda
//!   <= lambda_u
//
//      c_norm_over_radius = c_norm / radius
//      IF ( control%equality_problem ) THEN
//        lambda_l = MAX( control%lower,  - lambda_min,                          &
//                        c_norm_over_radius - lambda_max )
//        lambda_u = MIN( control%upper,                                         &
//                        c_norm_over_radius - lambda_min )
//      ELSE
//        lambda_l = MAX( control%lower, zero, - lambda_min,                     &
//                        c_norm_over_radius - lambda_max )
//        lambda_u = MIN( control%upper,                                         &
//                        MAX( zero, c_norm_over_radius - lambda_min ) )
//      END IF
//      lambda = lambda_l
//
//!  check for the "hard case"
//
//      IF ( lambda == - lambda_min ) THEN
//        c2 = zero
//        inform%hard_case = .TRUE.
//        DO i = 1, n
//          IF ( H( i ) == lambda_min ) THEN
//!           IF ( ABS( C( i ) ) > epsmch ) THEN
//            IF ( ABS( C( i ) ) > epsmch * c_norm ) THEN
//              inform%hard_case = .FALSE.
//              c2 = c2 + C( i ) ** 2
//            ELSE
//              i_hard = i
//            END IF
//          END IF
//        END DO
//
//!  the hard case may occur
//
//        IF ( inform%hard_case ) THEN
//          DO i = 1, n
//            IF ( H( i ) != lambda_min ) THEN
//              X( i )  = - C( i ) / ( H( i ) + lambda )
//            ELSE
//              X( i ) = zero
//            END IF
//          END DO
//          inform%x_norm = TWO_NORM( X )
//
//!  the hard case does occur
//
//          IF ( inform%x_norm <= radius ) THEN
//            IF ( inform%x_norm < radius ) THEN
//
//!  compute the step alpha so that X + alpha E_i_hard lies on the trust-region
//!  boundary and gives the smaller value of q
//
//              utx = X( i_hard ) / radius
//              distx = ( radius - inform%x_norm ) *                             &
//                ( ( radius + inform%x_norm ) / radius )
//              alpha = sign( distx / ( abs( utx ) +                             &
//                            sqrt( utx ** 2 + distx / radius ) ), utx )
//
//!  record the optimal values
//
//              X( i_hard ) = X( i_hard ) + alpha
//            END IF
//            inform%x_norm = TWO_NORM( X )
//            inform%obj =                                                       &
//                f + half * ( DOT_PRODUCT( C, X ) - lambda * radius ** 2 )
//            IF ( printi ) THEN
//              WRITE( out, "( A, A2, I4, 3ES22.15 )" )  prefix, region,         &
//              0, ABS( inform%x_norm - radius ), lambda, ABS( delta_lambda )
//              WRITE( out, "( A,                                                &
//           &    ' Hard-case stopping criteria satisfied' )" ) prefix
//            END IF
//            inform%status = RAL_NLLS_ok
//            GO TO 900
//
//!  the hard case didn't occur after all
//
//          ELSE
//            inform%hard_case = .FALSE.
//
//!  compute the first derivative of ||x|(lambda)||^2 - radius^2
//
//            w_norm2 = zero
//            DO i = 1, n
//              IF ( H( i ) != lambda_min )                                      &
//                w_norm2 = w_norm2 + C( i ) ** 2 / ( H( i ) + lambda ) ** 3
//            END DO
//            x_norm2( 1 ) = - two * w_norm2
//
//!  compute the Newton correction
//
//            lambda =                                                           &
//              lambda + ( inform%x_norm ** 2 - radius ** 2 ) / x_norm2( 1 )
//            lambda_l = MAX( lambda_l, lambda )
//          END IF
//
//!  there is a singularity at lambda. Compute the point for which the
//!  sum of squares of the singular terms is equal to radius^2
//
//        ELSE
//!         lambda = lambda + SQRT( c2 ) / radius
//          lambda = lambda + MAX( SQRT( c2 ) / radius, lambda * epsmch )
//          lambda_l = MAX( lambda_l, lambda )
//        END IF
//      END IF
//
//!  the iterates will all be in the L region. Prepare for the main loop
//
//      it = 0
//      max_order = MAX( 1, MIN( max_degree, control%taylor_max_degree ) )
//
//!  start the main loop
//
//      DO
//        it = it + 1
//
//!  if H(lambda) is positive definite, solve  H(lambda) x = - c
//
//        DO i = 1, n
//          X( i )  = - C( i ) / ( H( i ) + lambda )
//        END DO
//
//!  compute the two-norm of x
//
//        inform%x_norm = TWO_NORM( X )
//        x_norm2( 0 ) = inform%x_norm ** 2
//
//!  if the Newton step lies within the trust region, exit
//
//        IF ( lambda == zero .AND. inform%x_norm <= radius ) THEN
//          inform%obj = f + half * DOT_PRODUCT( C, X )
//          inform%status = RAL_NLLS_ok
//          region = 'L'
//          IF ( printi ) THEN
//            WRITE( out, "( A, A2, I4, 2ES22.15 )" ) prefix,                    &
//              region, it, inform%x_norm - radius, lambda
//            WRITE( out, "( A, ' Interior stopping criteria satisfied')" ) prefix
//          END IF
//          GO TO 900
//        END IF
//
//!  the current estimate gives a good approximation to the required root
//
//        IF ( ABS( inform%x_norm - radius ) <=                                  &
//               MAX( control%stop_normal * radius,                              &
//                    control%stop_absolute_normal ) ) THEN
//          IF ( inform%x_norm > radius ) THEN
//            lambda_l = MAX( lambda_l, lambda )
//          ELSE
//            region = 'G'
//            lambda_u = MIN( lambda_u, lambda )
//          END IF
//          IF ( printt .AND. it > 1 ) WRITE( out, 2030 ) prefix
//          IF ( printi ) THEN
//            WRITE( out, "( A, A2, I4, 3ES22.15 )" )  prefix, region,           &
//            it, ABS( inform%x_norm - radius ), lambda, ABS( delta_lambda )
//            WRITE( out, "( A,                                                  &
//         &    ' Normal stopping criteria satisfied' )" ) prefix
//          END IF
//          inform%status = RAL_NLLS_ok
//          EXIT
//        END IF
//
//        lambda_l = MAX( lambda_l, lambda )
//
//!  debug printing
//
//        IF ( printd ) THEN
//          WRITE( out, "( A, 8X, 'lambda', 13X, 'x_norm', 15X, 'radius' )" )    &
//            prefix
//          WRITE( out, "( A, 3ES20.12 )") prefix, lambda, inform%x_norm, radius
//        ELSE IF ( printi ) THEN
//          IF ( printt .AND. it > 1 ) WRITE( out, 2030 ) prefix
//          WRITE( out, "( A, A2, I4, 3ES22.15 )" ) prefix, region, it,          &
//            ABS( inform%x_norm - radius ), lambda, ABS( delta_lambda )
//        END IF
//
//!  record, for the future, values of lambda which give small ||x||
//
//        IF ( inform%len_history < history_max ) THEN
//          inform%len_history = inform%len_history + 1
//          inform%history( inform%len_history )%lambda = lambda
//          inform%history( inform%len_history )%x_norm = inform%x_norm
//        END IF
//
//!  a lambda in L has been found. It is now simply a matter of applying
//!  a variety of Taylor-series-based methods starting from this lambda
//
//!  precaution against rounding producing lambda outside L
//
//        IF ( lambda > lambda_u ) THEN
//          inform%status = RAL_NLLS_error_ill_conditioned
//          EXIT
//        END IF
//
//!  compute first derivatives of x^T M x
//
//!  form ||w||^2 = x^T H^-1(lambda) x
//
//        w_norm2 = zero
//        DO i = 1, n
//          w_norm2 = w_norm2 + C( i ) ** 2 / ( H( i ) + lambda ) ** 3
//        END DO
//
//!  compute the first derivative of x_norm2 = x^T M x
//
//        x_norm2( 1 ) = - two * w_norm2
//
//!  compute pi_beta = ||x||^beta and its first derivative when beta = - 1
//
//        beta = - one
//        CALL DTRS_pi_derivs( 1, beta, x_norm2( : 1 ), pi_beta( : 1 ) )
//
//!  compute the Newton correction (for beta = - 1)
//
//        delta_lambda = - ( pi_beta( 0 ) - ( radius ) ** beta ) / pi_beta( 1 )
//
//        n_lambda = 1
//        lambda_new( n_lambda ) = lambda + delta_lambda
//
//        IF ( max_order >= 3 ) THEN
//
//!  compute the second derivative of x^T x
//
//          z_norm2 = zero
//          DO i = 1, n
//            z_norm2 = z_norm2 + C( i ) ** 2 / ( H( i ) + lambda ) ** 4
//          END DO
//          x_norm2( 2 ) = six * z_norm2
//
//!  compute the third derivatives of x^T x
//
//          v_norm2 = zero
//          DO i = 1, n
//            v_norm2 = v_norm2 + C( i ) ** 2 / ( H( i ) + lambda ) ** 5
//          END DO
//          x_norm2( 3 ) = - twentyfour * v_norm2
//
//!  compute pi_beta = ||x||^beta and its derivatives when beta = 2
//
//          beta = two
//          CALL DTRS_pi_derivs( max_order, beta, x_norm2( : max_order ),        &
//                               pi_beta( : max_order ) )
//
//!  compute the "cubic Taylor approximaton" step (beta = 2)
//
//          a_0 = pi_beta( 0 ) - ( radius ) ** beta
//          a_1 = pi_beta( 1 )
//          a_2 = half * pi_beta( 2 )
//          a_3 = sixth * pi_beta( 3 )
//          a_max = MAX( ABS( a_0 ), ABS( a_1 ), ABS( a_2 ), ABS( a_3 ) )
//          IF ( a_max > zero ) THEN
//            a_0 = a_0 / a_max ; a_1 = a_1 / a_max
//            a_2 = a_2 / a_max ; a_3 = a_3 / a_max
//          END IF
//          CALL ROOTS_cubic( a_0, a_1, a_2, a_3, roots_tol, nroots,             &
//                            root1, root2, root3, roots_debug )
//          n_lambda = n_lambda + 1
//          IF ( nroots == 3 ) THEN
//            lambda_new( n_lambda ) = lambda + root3
//          ELSE
//            lambda_new( n_lambda ) = lambda + root1
//          END IF
//
//!  compute pi_beta = ||x||^beta and its derivatives when beta = - 0.4
//
//          beta = - point4
//          CALL DTRS_pi_derivs( max_order, beta, x_norm2( : max_order ),        &
//                               pi_beta( : max_order ) )
//
//!  compute the "cubic Taylor approximaton" step (beta = - 0.4)
//
//          a_0 = pi_beta( 0 ) - ( radius ) ** beta
//          a_1 = pi_beta( 1 )
//          a_2 = half * pi_beta( 2 )
//          a_3 = sixth * pi_beta( 3 )
//          a_max = MAX( ABS( a_0 ), ABS( a_1 ), ABS( a_2 ), ABS( a_3 ) )
//          IF ( a_max > zero ) THEN
//            a_0 = a_0 / a_max ; a_1 = a_1 / a_max
//            a_2 = a_2 / a_max ; a_3 = a_3 / a_max
//          END IF
//          CALL ROOTS_cubic( a_0, a_1, a_2, a_3, roots_tol, nroots,             &
//                            root1, root2, root3, roots_debug )
//          n_lambda = n_lambda + 1
//          IF ( nroots == 3 ) THEN
//            lambda_new( n_lambda ) = lambda + root3
//          ELSE
//            lambda_new( n_lambda ) = lambda + root1
//          END IF
//        END IF
//
//!  record all of the estimates of the optimal lambda
//
//        IF ( printd ) THEN
//          WRITE( out, "( A, ' lambda_t (', I1, ')', 3ES20.13 )" )              &
//            prefix, MAXLOC( lambda_new( : n_lambda ) ),                        &
//            lambda_new( : MIN( 3, n_lambda ) )
//          IF ( n_lambda > 3 ) WRITE( out, "( A, 13X, 3ES20.13 )" )             &
//            prefix, lambda_new( 4 : MIN( 6, n_lambda ) )
//        END IF
//
//!  compute the best Taylor improvement
//
//        lambda_plus = MAXVAL( lambda_new( : n_lambda ) )
//        delta_lambda = lambda_plus - lambda
//        lambda = lambda_plus
//
//!  improve the lower bound if possible
//
//        lambda_l = MAX( lambda_l, lambda_plus )
//
//!  check that the best Taylor improvement is significant
//
//        IF ( ABS( delta_lambda ) < epsmch * MAX( one, ABS( lambda ) ) ) THEN
//          IF ( printi ) WRITE( out, "( A, ' normal exit with no ',             &
//         &                     'significant Taylor improvement' )" ) prefix
//          inform%status = RAL_NLLS_ok
//          EXIT
//        END IF
//
//!  End of main iteration loop
//
//      END DO
//
//!  Record the optimal obective value
//
//      inform%obj = f + half * ( DOT_PRODUCT( C, X ) - lambda * x_norm2( 0 ) )
//
//!  ----
//!  Exit
//!  ----
//
// 900  CONTINUE
//      inform%multiplier = lambda
//      inform%pole = MAX( zero, - lambda_min )
//      RETURN
//
//! Non-executable statements
//
// 2030 FORMAT( A, '    it     ||x||-radius             lambda ',                &
//                 '              d_lambda' )
//
//!  End of subroutine DTRS_solve_main
//
//      END SUBROUTINE DTRS_solve_main
//
//!-*-*-*-*-*-*-  D T R S _ P I _ D E R I V S   S U B R O U T I N E   -*-*-*-*-*-
//
//      SUBROUTINE DTRS_pi_derivs( max_order, beta, x_norm2, pi_beta )
//
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
//
//!-----------------------------------------------
//!   D u m m y   A r g u m e n t s
//!-----------------------------------------------
//
//      INTEGER, INTENT( IN )  max_order
//      REAL , INTENT( IN )  beta, x_norm2( 0 : max_order )
//      REAL , INTENT( OUT )  pi_beta( 0 : max_order )
//
//!-----------------------------------------------
//!   L o c a l   V a r i a b l e
//!-----------------------------------------------
//
//      REAL   hbeta
//
//      hbeta = half * beta
//      pi_beta( 0 ) = x_norm2( 0 ) ** hbeta
//      pi_beta( 1 ) = hbeta * ( x_norm2( 0 ) ** ( hbeta - one ) ) * x_norm2( 1 )
//      IF ( max_order == 1 ) RETURN
//      pi_beta( 2 ) = hbeta * ( x_norm2( 0 ) ** ( hbeta - two ) ) *             &
//        ( ( hbeta - one ) * x_norm2( 1 ) ** 2 + x_norm2( 0 ) * x_norm2( 2 ) )
//      IF ( max_order == 2 ) RETURN
//      pi_beta( 3 ) = hbeta * ( x_norm2( 0 ) ** ( hbeta - three ) ) *           &
//        ( x_norm2( 3 ) * x_norm2( 0 ) ** 2 + ( hbeta - one ) *                 &
//          ( three * x_norm2( 0 ) * x_norm2( 1 ) * x_norm2( 2 ) +               &
//            ( hbeta - two ) * x_norm2( 1 ) ** 3 ) )
//
//      RETURN
//
//!  End of subroutine DTRS_pi_derivs
//
//      END SUBROUTINE DTRS_pi_derivs
//
//!-*-*-*-*-*  D T R S _ T H E T A _ D E R I V S   S U B R O U T I N E   *-*-*-*-
//
//      SUBROUTINE DTRS_theta_derivs( max_order, beta, lambda, sigma,            &
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
//      theta_beta( 2 ) = beta * ( los ** ( beta - two ) ) *                    &
//                        ( beta - one ) * oos ** 2
//      IF ( max_order == 2 ) RETURN
//      theta_beta( 3 ) = beta * ( los ** ( beta - three ) ) *                  &
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
//       IF ( n > 0 ) THEN
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
//!-*-*-*-*-*-  End of R A L _ N L L S _ D T R S  double  M O D U L E  *-*-*-*-*-
//
//   END MODULE RAL_NLLS_DTRS_double
//
//



} // RalNlls
} // CurveFitting
} // Mantid

