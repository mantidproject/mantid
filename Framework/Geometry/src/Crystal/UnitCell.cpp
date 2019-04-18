// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidGeometry/Crystal/UnitCell.h"
#include "MantidKernel/Matrix.h"
#include "MantidKernel/StringTokenizer.h"
#include "MantidKernel/System.h"
#include "MantidKernel/V3D.h"
#include <cfloat>
#include <iomanip>
#include <ios>
#include <stdexcept>

#include <boost/lexical_cast.hpp>

namespace Mantid {
namespace Geometry {
using Mantid::Kernel::DblMatrix;
using Mantid::Kernel::V3D;

/** Default constructor.
 \f$ a = b = c =  1 \mbox{\AA, } \alpha = \beta = \gamma = 90^\circ \f$ */
UnitCell::UnitCell()
    : da(6), ra(6), errorda(6), G(3, 3), Gstar(3, 3), B(3, 3), ModHKL(3, 3),
      errorModHKL(3, 3) {
  da[0] = da[1] = da[2] = 1.;
  da[3] = da[4] = da[5] = deg2rad * 90.0;
  errorda[0] = errorda[1] = errorda[2] = errorda[3] = errorda[4] = errorda[5] =
      0.0;
  MaxOrder = 0;
  CrossTerm = false;
  recalculate();
}

/** Constructor
 @param _a, _b, _c :: lattice parameters \f$ a, b, c \f$ \n
 with \f$\alpha = \beta = \gamma = 90^\circ \f$*/
UnitCell::UnitCell(double _a, double _b, double _c)
    : da(6), ra(6), errorda(6), G(3, 3), Gstar(3, 3), B(3, 3), ModHKL(3, 3),
      errorModHKL(3, 3) {
  da[0] = _a;
  da[1] = _b;
  da[2] = _c;
  // Angles are 90 degrees in radians ->Pi/2
  da[3] = da[4] = da[5] = 0.5 * M_PI;
  errorda[0] = errorda[1] = errorda[2] = errorda[3] = errorda[4] = errorda[5] =
      0.0;
  MaxOrder = 0;
  CrossTerm = false;
  recalculate();
}

/** Constructor
 @param _a, _b, _c, _alpha, _beta, _gamma :: lattice parameters\n
 @param angleunit :: units for angle, of type #AngleUnits. Default is degrees.
 */
UnitCell::UnitCell(double _a, double _b, double _c, double _alpha, double _beta,
                   double _gamma, const int angleunit)
    : da(6), ra(6), errorda(6), G(3, 3), Gstar(3, 3), B(3, 3), ModHKL(3, 3),
      errorModHKL(3, 3) {
  da[0] = _a;
  da[1] = _b;
  da[2] = _c;
  // Angle transformed in radians
  if (angleunit == angDegrees) {
    da[3] = deg2rad * _alpha;
    da[4] = deg2rad * _beta;
    da[5] = deg2rad * _gamma;
  } else {
    da[3] = _alpha;
    da[4] = _beta;
    da[5] = _gamma;
  }
  errorda[0] = errorda[1] = errorda[2] = errorda[3] = errorda[4] = errorda[5] =
      0.0;
  MaxOrder = 0;
  CrossTerm = false;
  recalculate();
}

/** Get lattice parameter
 @return a1 :: lattice parameter \f$ a \f$ (in \f$ \mbox{\AA} \f$ )
 @see a()*/
double UnitCell::a1() const { return da[0]; }

/** Get lattice parameter
 @return a2 :: lattice parameter \f$ b \f$ (in \f$ \mbox{\AA} \f$ )
 @see b()*/
double UnitCell::a2() const { return da[1]; }

/** Get lattice parameter
 @return a3 :: lattice parameter \f$ c \f$ (in \f$ \mbox{\AA} \f$ )
 @see c()*/

double UnitCell::a3() const { return da[2]; }
/** Get lattice parameter a1-a3 as function of index (0-2)
 @return a_n :: lattice parameter \f$ a,b or c \f$ (in \f$ \mbox{\AA} \f$ )
 */
double UnitCell::a(int nd) const {
  if (nd < 0 || nd > 2)
    throw(std::invalid_argument(
        "lattice parameter index can change from 0 to 2 "));
  return da[nd];
}

/** Get lattice parameter
 @return alpha1 :: lattice parameter \f$ \alpha \f$ (in radians)
 @see alpha()*/
double UnitCell::alpha1() const { return da[3]; }

/** Get lattice parameter
 @return alpha2 :: lattice parameter \f$ \beta \f$ (in radians)
 @see beta()*/
double UnitCell::alpha2() const { return da[4]; }

/** Get lattice parameter
 @return alpha3 :: lattice parameter \f$ \gamma \f$ (in radians)
 @see gamma()*/
double UnitCell::alpha3() const { return da[5]; }

/** Get lattice parameter
 @return a :: lattice parameter \f$ a \f$ (in \f$ \mbox{\AA} \f$ )
 @see a1()*/
double UnitCell::a() const { return da[0]; }

/** Get lattice parameter
 @return b :: lattice parameter \f$ b \f$ (in \f$ \mbox{\AA} \f$ )
 @see a2()*/
double UnitCell::b() const { return da[1]; }

/** Get lattice parameter
 @return c :: lattice parameter \f$ c \f$ (in \f$ \mbox{\AA} \f$ )
 @see a3()*/
double UnitCell::c() const { return da[2]; }

/** Get lattice parameter
 @return alpha :: lattice parameter \f$ \alpha \f$ (in degrees)
 @see alpha1()*/
double UnitCell::alpha() const { return da[3] * rad2deg; }

/** Get lattice parameter
 @return beta :: lattice parameter \f$ \beta \f$ (in degrees)
 @see alpha2()*/
double UnitCell::beta() const { return da[4] * rad2deg; }

/** Get lattice parameter
 @return gamma :: lattice parameter \f$ \gamma \f$ (in degrees)
 @see alpha3()*/
double UnitCell::gamma() const { return da[5] * rad2deg; }

/** Get reciprocal lattice parameter
 @return b1 :: lattice parameter \f$ a^{*} \f$ (in \f$ \mbox{\AA}^{-1} \f$ )
 @see astar()*/
double UnitCell::b1() const { return ra[0]; }

/** Get reciprocal lattice parameter
 @return b2 :: lattice parameter \f$ b^{*} \f$ (in \f$ \mbox{\AA}^{-1} \f$ )
 @see bstar()*/
double UnitCell::b2() const { return ra[1]; }

/** Get reciprocal lattice parameter
 @return b3 :: lattice parameter \f$ c^{*} \f$ (in \f$ \mbox{\AA}^{-1} \f$ )
 @see cstar()*/
double UnitCell::b3() const { return ra[2]; }

/** Get reciprocal lattice parameter
 @return beta1 :: lattice parameter \f$ \alpha^{*} \f$ (in radians)
 @see alphastar()*/
double UnitCell::beta1() const { return ra[3]; }

/** Get reciprocal lattice parameter
 @return beta2 :: lattice parameter \f$ \beta^{*} \f$ (in radians)
 @see betastar()*/
double UnitCell::beta2() const { return ra[4]; }

/** Get reciprocal lattice parameter
 @return beta3 :: lattice parameter \f$ \gamma^{*} \f$ (in radians)
 @see gammastar()*/
double UnitCell::beta3() const { return ra[5]; }

/** Get reciprocal lattice parameter
 @return astar :: lattice parameter \f$ a^{*} \f$ (in \f$ \mbox{\AA}^{-1} \f$ )
 @see b1()*/
double UnitCell::astar() const { return ra[0]; }

/** Get reciprocal lattice parameter
 @return bstar :: lattice parameter \f$ b^{*} \f$ (in \f$ \mbox{\AA}^{-1} \f$ )
 @see b2()*/
double UnitCell::bstar() const { return ra[1]; }

/** Get reciprocal lattice parameter
 @return cstar :: lattice parameter \f$ c^{*} \f$ (in \f$ \mbox{\AA}^{-1} \f$ )
 @see b3()*/
double UnitCell::cstar() const { return ra[2]; }

/** Get reciprocal lattice parameter
 @return alphastar :: lattice parameter \f$ \alpha^{*} \f$ (in degrees)
 @see beta1()*/
double UnitCell::alphastar() const { return ra[3] * rad2deg; }

/** Get reciprocal lattice parameter
 @return  betastar:: lattice parameter \f$ \beta^{*} \f$ (in degrees)
 @see beta2()*/
double UnitCell::betastar() const { return ra[4] * rad2deg; }

/** Get reciprocal lattice parameter
 @return  gammastar:: lattice parameter \f$ \gamma^{*} \f$ (in degrees)
 @see beta3()*/
double UnitCell::gammastar() const { return ra[5] * rad2deg; }

/** Get lattice parameter error
 @return errora :: errorlattice parameter \f$ a \f$ (in \f$ \mbox{\AA} \f$ )
 */
double UnitCell::errora() const { return errorda[0]; }

/** Get lattice parameter error
 @return errorb :: errorlattice parameter \f$ b \f$ (in \f$ \mbox{\AA} \f$ )
 */
double UnitCell::errorb() const { return errorda[1]; }

/** Get lattice parameter error
 @return errorc :: errorlattice parameter \f$ c \f$ (in \f$ \mbox{\AA} \f$ )
 */
double UnitCell::errorc() const { return errorda[2]; }

/** Get lattice parameter error
 @param angleunit :: units for angle, of type #AngleUnits . Default is degrees.
 @return erroralpha :: errorlattice parameter \f$ alpha \f$ (in degrees or
 radians )
 */
double UnitCell::erroralpha(const int angleunit) const {
  if (angleunit == angDegrees) {
    return errorda[3] * rad2deg;
  } else {
    return errorda[3];
  }
}

/** Get lattice parameter error
 @param angleunit :: units for angle, of type #AngleUnits . Default is degrees.
 @return erroralpha :: errorlattice parameter \f$ beta \f$ (in degrees or
 radians
 )
 */
double UnitCell::errorbeta(const int angleunit) const {
  if (angleunit == angDegrees) {
    return errorda[4] * rad2deg;
  } else {
    return errorda[4];
  }
}

/** Get lattice parameter error
 @param angleunit :: units for angle, of type #AngleUnits . Default is degrees.
 @return erroralpha :: errorlattice parameter \f$ gamma \f$ (in degrees or
 radians )
 */
double UnitCell::errorgamma(const int angleunit) const {
  if (angleunit == angDegrees) {
    return errorda[5] * rad2deg;
  } else {
    return errorda[5];
  }
}

/** Get lattice parameter error
 @return errorc :: errorlattice parameter \f$ volume \f$ (in \f$ \mbox{\AA} \f$
 )
 */
double UnitCell::errorvolume() const {
  // From latcon.py by Art Schultz
  double V = volume();
  double delta_V_alphaV = 0.0;
  if (erroralpha() > 0.0) {
    double alpha1 = alpha() - 0.5 * erroralpha();
    double Va1 = UnitCell(a(), b(), c(), alpha1, beta(), gamma()).volume();
    double alpha2 = alpha() + 0.5 * erroralpha();
    double Va2 = UnitCell(a(), b(), c(), alpha2, beta(), gamma()).volume();
    delta_V_alphaV = (Va2 - Va1) / V;
  }

  double delta_V_betaV = 0.0;
  if (errorbeta() > 0.0) {
    double beta1 = beta() - 0.5 * errorbeta();
    double Va1 = UnitCell(a(), b(), c(), alpha(), beta1, gamma()).volume();
    double beta2 = beta() + 0.5 * errorbeta();
    double Va2 = UnitCell(a(), b(), c(), alpha(), beta2, gamma()).volume();
    delta_V_betaV = (Va2 - Va1) / V;
  }

  double delta_V_gammaV = 0.0;
  if (errorgamma() > 0.0) {
    double gamma1 = gamma() - 0.5 * errorgamma();
    double Va1 = UnitCell(a(), b(), c(), alpha(), beta(), gamma1).volume();
    double gamma2 = gamma() + 0.5 * errorgamma();
    double Va2 = UnitCell(a(), b(), c(), alpha(), beta(), gamma2).volume();
    delta_V_gammaV = (Va2 - Va1) / V;
  }

  return V * sqrt(std::pow(errora() / a(), 2) + std::pow(errorb() / b(), 2) +
                  std::pow(errorc() / c(), 2) + std::pow(delta_V_alphaV, 2) +
                  std::pow(delta_V_betaV, 2) + std::pow(delta_V_gammaV, 2));
}

/** Set lattice parameters
 @param _a, _b, _c, _alpha, _beta, _gamma :: lattice parameters\n
 @param angleunit :: units for angle, of type #AngleUnits . Default is degrees.
 */

void UnitCell::set(double _a, double _b, double _c, double _alpha, double _beta,
                   double _gamma, const int angleunit) {
  da[0] = _a;
  da[1] = _b;
  da[2] = _c;
  if (angleunit == angDegrees) {
    da[3] = deg2rad * _alpha;
    da[4] = deg2rad * _beta;
    da[5] = deg2rad * _gamma;
  } else {
    da[3] = _alpha;
    da[4] = _beta;
    da[5] = _gamma;
  }
  recalculate();
}

/** Set lattice parameter errors
 @param _aerr, _berr, _cerr, _alphaerr, _betaerr, _gammaerr :: lattice
 parameter errors\n
 @param angleunit :: units for angle, of type #AngleUnits . Default is degrees.
 */

void UnitCell::setError(double _aerr, double _berr, double _cerr,
                        double _alphaerr, double _betaerr, double _gammaerr,
                        const int angleunit) {
  errorda[0] = _aerr;
  errorda[1] = _berr;
  errorda[2] = _cerr;
  if (angleunit == angDegrees) {
    errorda[3] = deg2rad * _alphaerr;
    errorda[4] = deg2rad * _betaerr;
    errorda[5] = deg2rad * _gammaerr;
  } else {
    errorda[3] = _alphaerr;
    errorda[4] = _betaerr;
    errorda[5] = _gammaerr;
  }
}

/** Set modulation vectors for satellites
 @param _dh1	offset for H for first vector
 @param _dk1	offset for K for first vector
 @param _dl1	offset for l for first vector
 @param _dh2	offset for H for second vector
 @param _dk2	offset for K for second vector
 @param _dl2	offset for l for second vector
 @param _dh3	offset for H for third vector
 @param _dk3	offset for K for third vector
 @param _dl3	offset for l for third vector
 */

void UnitCell::setModHKL(double _dh1, double _dk1, double _dl1, double _dh2,
                         double _dk2, double _dl2, double _dh3, double _dk3,
                         double _dl3) {
  ModHKL[0][0] = _dh1;
  ModHKL[1][0] = _dk1;
  ModHKL[2][0] = _dl1;
  ModHKL[0][1] = _dh2;
  ModHKL[1][1] = _dk2;
  ModHKL[2][1] = _dl2;
  ModHKL[0][2] = _dh3;
  ModHKL[1][2] = _dk3;
  ModHKL[2][2] = _dl3;
}

/** Set modulation vectors for satellites
 @param newModHKL modulation vectors for HKL for three vectors
 */

void UnitCell::setModHKL(const DblMatrix &newModHKL) { ModHKL = newModHKL; }

/** Set modulation vectors for satellites
 @param newErrorModHKL errors for modulation vectors for HKL for three vectors
 */

void UnitCell::setErrorModHKL(const DblMatrix &newErrorModHKL) {
  errorModHKL = newErrorModHKL;
}

/** Set modulation vectors for satellites
 @param _dh1err	error for offset for H for first vector
 @param _dk1err	error for offset for K for first vector
 @param _dl1err	error for offset for l for first vector
 @param _dh2err	error for offset for H for second vector
 @param _dk2err	error for offset for K for second vector
 @param _dl2err	error for offset for l for second vector
 @param _dh3err	error for offset for H for third vector
 @param _dk3err	error for offset for K for third vector
 @param _dl3err	error for offset for l for third vector
 */

void UnitCell::setErrorModHKL(double _dh1err, double _dk1err, double _dl1err,
                              double _dh2err, double _dk2err, double _dl2err,
                              double _dh3err, double _dk3err, double _dl3err) {
  errorModHKL[0][0] = _dh1err;
  errorModHKL[1][0] = _dk1err;
  errorModHKL[2][0] = _dl1err;
  errorModHKL[0][1] = _dh2err;
  errorModHKL[1][1] = _dk2err;
  errorModHKL[2][1] = _dl2err;
  errorModHKL[0][2] = _dh3err;
  errorModHKL[1][2] = _dk3err;
  errorModHKL[2][2] = _dl3err;
}

/** Set modulation vectors for satellites
 @param _dh1	offset for H for first vector
 @param _dk1	offset for K for first vector
 @param _dl1	offset for l for first vector
 */

void UnitCell::setModVec1(double _dh1, double _dk1, double _dl1) {
  ModHKL[0][0] = _dh1;
  ModHKL[1][0] = _dk1;
  ModHKL[2][0] = _dl1;
}

/** Set modulation vectors for satellites
 @param _dh2	offset for H for second vector
 @param _dk2	offset for K for second vector
 @param _dl2	offset for l for second vector
 */

void UnitCell::setModVec2(double _dh2, double _dk2, double _dl2) {
  ModHKL[0][1] = _dh2;
  ModHKL[1][1] = _dk2;
  ModHKL[2][1] = _dl2;
}

/** Set modulation vectors for satellites
 @param _dh3	offset for H for third vector
 @param _dk3	offset for K for third vector
 @param _dl3	offset for l for third vector
 */

void UnitCell::setModVec3(double _dh3, double _dk3, double _dl3) {
  ModHKL[0][2] = _dh3;
  ModHKL[1][2] = _dk3;
  ModHKL[2][2] = _dl3;
}

/** Set modulation vectors for satellites
 @param newModVec modulation vectors for HKL for first vector
 */

void UnitCell::setModVec1(const V3D &newModVec) {
  ModHKL[0][0] = newModVec[0];
  ModHKL[1][0] = newModVec[1];
  ModHKL[2][0] = newModVec[2];
}

/** Set modulation vectors for satellites
 @param newModVec modulation vectors for HKL for second vector
 */

void UnitCell::setModVec2(const V3D &newModVec) {
  ModHKL[0][1] = newModVec[0];
  ModHKL[1][1] = newModVec[1];
  ModHKL[2][1] = newModVec[2];
}

/** Set modulation vectors for satellites
 @param newModVec modulation vectors for HKL for third vector
 */

void UnitCell::setModVec3(const V3D &newModVec) {
  ModHKL[0][2] = newModVec[0];
  ModHKL[1][2] = newModVec[1];
  ModHKL[2][2] = newModVec[2];
}

/** Set modulation vectors for satellites
 @param i       index of vector to set
 @param _dherr	error for offset for H for ith vector
 @param _dkerr	error for offset for K for ith vector
 @param _dlerr	error for offset for l for ith vector
 */

void UnitCell::setModerr(int i, double _dherr, double _dkerr, double _dlerr) {
  errorModHKL[0][i] = _dherr;
  errorModHKL[1][i] = _dkerr;
  errorModHKL[2][i] = _dlerr;
}

/** Set modulation vectors for satellites
 @param _dh1err	error for offset for H for first vector
 @param _dk1err	error for offset for K for first vector
 @param _dl1err	error for offset for l for first vector
 */

void UnitCell::setModerr1(double _dh1err, double _dk1err, double _dl1err) {
  errorModHKL[0][0] = _dh1err;
  errorModHKL[1][0] = _dk1err;
  errorModHKL[2][0] = _dl1err;
}

/** Set modulation vectors for satellites
 @param _dh2err	error for offset for H for second vector
 @param _dk2err	error for offset for K for second vector
 @param _dl2err	error for offset for l for second vector
 */

void UnitCell::setModerr2(double _dh2err, double _dk2err, double _dl2err) {
  errorModHKL[0][1] = _dh2err;
  errorModHKL[1][1] = _dk2err;
  errorModHKL[2][1] = _dl2err;
}

/** Set modulation vectors for satellites
 @param _dh3err	error for offset for H for third vector
 @param _dk3err	error for offset for K for third vector
 @param _dl3err	error for offset for l for third vector
 */

void UnitCell::setModerr3(double _dh3err, double _dk3err, double _dl3err) {
  errorModHKL[0][2] = _dh3err;
  errorModHKL[1][2] = _dk3err;
  errorModHKL[2][2] = _dl3err;
}

/** Set modulation vectors for satellites
 @param MaxO    maximum order of modulation vectors
 */

void UnitCell::setMaxOrder(int MaxO) { MaxOrder = MaxO; }

/** Set modulation vectors for satellites
 @param CT      if true, use cross terms
 */

void UnitCell::setCrossTerm(bool CT) { CrossTerm = CT; }

/** Get modulation vectors for satellites
 @param j       index of vector to get
 @return ModVec :: modulation vector
 */

const Kernel::V3D UnitCell::getModVec(int j) const {
  return V3D(getdh(j), getdk(j), getdl(j));
}

/** Get errors for modulation vectors for satellites
 @param j       index of vector to get
 @return VecErr :: error of modulation vector
 */

const Kernel::V3D UnitCell::getVecErr(int j) const {
  return V3D(getdherr(j), getdkerr(j), getdlerr(j));
}

/** Get modulation vectors for satellites
 @return ModHKL :: modulation vectors
 */

const Kernel::DblMatrix &UnitCell::getModHKL() const { return ModHKL; }

/** Get modulation vectors for satellites
 @param j       index of vector to get
 @return dh :: dh of modulation vector
 */

double UnitCell::getdh(int j) const { return ModHKL[0][j]; }

/** Get modulation vectors for satellites
 @param j       index of vector to get
 @return ModVec :: modulation vector
 */

double UnitCell::getdk(int j) const { return ModHKL[1][j]; }

/** Get modulation vectors for satellites
 @param j       index of vector to get
 @return ModVec :: modulation vector
 */

double UnitCell::getdl(int j) const { return ModHKL[2][j]; }

/** Get error of modulation vectors for satellites
 @param j       index of vector to get
 @return ModVecErr :: error of modulation vector
 */

double UnitCell::getdherr(int j) const { return errorModHKL[0][j]; }

/** Get error of modulation vectors for satellites
 @param j       index of vector to get
 @return ModVecErr :: error of modulation vector
 */

double UnitCell::getdkerr(int j) const { return errorModHKL[1][j]; }

/** Get error  of modulation vectors for satellites
 @param j       index of vector to get
 @return ModVecErr :: error of modulation vector
 */

double UnitCell::getdlerr(int j) const { return errorModHKL[2][j]; }

/** Get max order
 @return MaxOrder :: maximum order
 */

int UnitCell::getMaxOrder() const { return MaxOrder; }

/** Get cross term boolean
 @return CrossTerm :: if true, use cross terms
 */

bool UnitCell::getCrossTerm() const { return CrossTerm; }

/** Set lattice parameter
 @param _a :: lattice parameter \f$ a \f$ (in \f$ \mbox{\AA} \f$ )*/
void UnitCell::seta(double _a) {
  da[0] = _a;
  recalculate();
}
/** Set lattice parameter error
 @param _aerr :: lattice parameter \f$ a \f$ error (in \f$ \mbox{\AA} \f$ )*/
void UnitCell::setErrora(double _aerr) { errorda[0] = _aerr; }

/** Set lattice parameter
 @param _b :: lattice parameter \f$ b \f$ (in \f$ \mbox{\AA} \f$ )*/
void UnitCell::setb(double _b) {
  da[1] = _b;
  recalculate();
}
/** Set lattice parameter error
 @param _berr :: lattice parameter \f$ b \f$ error (in \f$ \mbox{\AA} \f$ )*/
void UnitCell::setErrorb(double _berr) { errorda[1] = _berr; }
/** Set lattice parameter
 @param _c :: lattice parameter \f$ c \f$ (in \f$ \mbox{\AA} \f$ )*/
void UnitCell::setc(double _c) {
  da[2] = _c;
  recalculate();
}
/** Set lattice parameter error
 @param _cerr :: lattice parameter \f$ c \f$ error (in \f$ \mbox{\AA} \f$ )*/
void UnitCell::setErrorc(double _cerr) { errorda[2] = _cerr; }
/** Set lattice parameter
 @param _alpha :: lattice parameter \f$ \alpha \f$
 @param angleunit :: units for angle, of type #AngleUnits. Default is degrees.
 */
void UnitCell::setalpha(double _alpha, const int angleunit) {
  if (angleunit == angDegrees)
    da[3] = deg2rad * _alpha;
  else
    da[3] = _alpha;
  recalculate();
}
/** Set lattice parameter error
 @param _alphaerr :: lattice parameter \f$ \alpha \f$ error
 @param angleunit :: units for angle, of type #AngleUnits. Default is degrees.
 */
void UnitCell::setErroralpha(double _alphaerr, const int angleunit) {
  if (angleunit == angDegrees)
    errorda[3] = deg2rad * _alphaerr;
  else
    errorda[3] = _alphaerr;
}
/** Set lattice parameter
 @param _beta :: lattice parameter \f$ \beta \f$
 @param angleunit :: units for angle, of type #AngleUnits. Default is degrees.
 */
void UnitCell::setbeta(double _beta, const int angleunit) {
  if (angleunit == angDegrees)
    da[4] = deg2rad * _beta;
  else
    da[4] = _beta;
  recalculate();
}

/** Set lattice parameter error
 @param _betaerr :: lattice parameter \f$ \beta \f$ error
 @param angleunit :: units for angle, of type #AngleUnits. Default is degrees.
 */
void UnitCell::setErrorbeta(double _betaerr, const int angleunit) {
  if (angleunit == angDegrees)
    errorda[4] = deg2rad * _betaerr;
  else
    errorda[4] = _betaerr;
}

/** Set lattice parameter
 @param _gamma :: lattice parameter \f$ \gamma \f$
 @param angleunit :: units for angle, of type #AngleUnits. Default is degrees.
 */
void UnitCell::setgamma(double _gamma, const int angleunit) {
  if (angleunit == angDegrees)
    da[5] = deg2rad * _gamma;
  else
    da[5] = _gamma;
  recalculate();
}

/** Set lattice parameter error
 @param _gammaerr :: lattice parameter \f$ \gamma \f$ error
 @param angleunit :: units for angle, of type #AngleUnits. Default is degrees.
 */
void UnitCell::setErrorgamma(double _gammaerr, const int angleunit) {
  if (angleunit == angDegrees)
    errorda[5] = deg2rad * _gammaerr;
  else
    errorda[5] = _gammaerr;
}

/// Return d-spacing (\f$ \mbox{ \AA } \f$) for a given h,k,l coordinate
double UnitCell::d(double h, double k, double l) const {
  return 1.0 / dstar(V3D(h, k, l));
}

/// Return d-spacing (\f$ \mbox{ \AA } \f$) for a given h,k,l coordinate
double UnitCell::d(const V3D &hkl) const { return 1.0 / dstar(hkl); }

/// Return d*=1/d (\f$ \mbox{ \AA }^{-1} \f$) for a given h,k,l coordinate
double UnitCell::dstar(double h, double k, double l) const {
  return dstar(V3D(h, k, l)); // create a V3D vector h,k,l
}

/// Return d*=1/d (\f$ \mbox{ \AA }^{-1} \f$) for a given h,k,l coordinate
double UnitCell::dstar(const V3D &hkl) const {
  V3D Q = B * hkl; // transform into $AA^-1$
  return Q.norm();
}

/// Calculate the angle in degrees or radians between two reciprocal vectors
/// (h1,k1,l1) and (h2,k2,l2)
double UnitCell::recAngle(double h1, double k1, double l1, double h2, double k2,
                          double l2, const int angleunit) const {
  V3D Q1(h1, k1, l1), Q2(h2, k2, l2);
  double E, ang;
  Q1 = Gstar * Q1;
  E = Q1.scalar_prod(Q2);
  double temp = E / dstar(h1, k1, l1) / dstar(h2, k2, l2);
  if (temp > 1)
    ang = 0.;
  else if (temp < -1)
    ang = M_PI;
  else
    ang = acos(temp);
  if (angleunit == angDegrees)
    return rad2deg * ang;
  else
    return ang;
}

/// Volume of the direct unit-cell
double UnitCell::volume() const {
  double volume = G.determinant();
  return sqrt(volume);
}

/// Volume of the reciprocal lattice
double UnitCell::recVolume() const {
  double recvolume = Gstar.determinant();
  return sqrt(recvolume);
}

/// Get the metric tensor
/// @return G :: metric tensor
const Kernel::DblMatrix &UnitCell::getG() const { return G; }

/// Get the reciprocal metric tensor
/// @return Gstar :: metric tensor of the reciprocal lattice
const Kernel::DblMatrix &UnitCell::getGstar() const { return Gstar; }

/// Get the B-matrix
/// @return B :: B matrix in Busing-Levy convention
const Kernel::DblMatrix &UnitCell::getB() const { return B; }

/// Get the inverse of the B-matrix
/// @return Binv :: inverse of the B matrix in Busing-Levy convention
const Kernel::DblMatrix &UnitCell::getBinv() const { return Binv; }

/// Private function, called at initialization or whenever lattice parameters
/// are changed
void UnitCell::recalculate() {
  if ((da[3] > da[4] + da[5]) || (da[4] > da[3] + da[5]) ||
      (da[5] > da[4] + da[3])) {
    throw std::invalid_argument("Invalid angles");
  }
  calculateG();
  calculateGstar();
  calculateReciprocalLattice();
  calculateB();
}

void UnitCell::calculateG() {
  G[0][0] = da[0] * da[0];
  G[1][1] = da[1] * da[1];
  G[2][2] = da[2] * da[2];
  G[0][1] = da[0] * da[1] * cos(da[5]);
  G[0][2] = da[0] * da[2] * cos(da[4]);
  G[1][2] = da[1] * da[2] * cos(da[3]);
  G[1][0] = G[0][1];
  G[2][0] = G[0][2];
  G[2][1] = G[1][2];
}

/// Private function to calculate #Gstar matrix
void UnitCell::calculateGstar() {
  // Reciprocal metrix tensor is simply the inverse of the direct one
  double det = G.determinant();
  if (det == 0) {
    throw std::range_error("UnitCell not properly initialized");
  }
  Gstar = G;
  if (Gstar.Invert() == 0) {
    throw std::range_error("UnitCell not properly initialized");
  }
}

/// Private function to calculate reciprocal lattice parameters
void UnitCell::calculateReciprocalLattice() {
  ra[0] = sqrt(Gstar[0][0]);                 // a*
  ra[1] = sqrt(Gstar[1][1]);                 // b*
  ra[2] = sqrt(Gstar[2][2]);                 // c*
  ra[3] = acos(Gstar[1][2] / ra[1] / ra[2]); // alpha*
  ra[4] = acos(Gstar[0][2] / ra[0] / ra[2]); // beta*
  ra[5] = acos(Gstar[0][1] / ra[0] / ra[1]); // gamma*
}

/// Private function to calculate #B matrix
void UnitCell::calculateB() {
  // B matrix using a right handed coordinate system with b1 along x and y in
  // the (b1,b2) plane.
  // This is the convention in Busing and Levy.
  // | b1 b2cos(beta3)      b3cos(beta2)        |
  // | 0  b2sin(beta3) -b3sin(beta2)cos(alpha1) |
  // | 0       0                  1/a3          |
  B[0][0] = ra[0];
  B[0][1] = ra[1] * cos(ra[5]);
  B[0][2] = ra[2] * cos(ra[4]);
  B[1][0] = 0.;
  B[1][1] = ra[1] * sin(ra[5]);
  B[1][2] = -ra[2] * sin(ra[4]) * cos(da[3]);
  B[2][0] = 0.;
  B[2][1] = 0.;
  B[2][2] = 1. / da[2];

  /// Now let's cache the inverse B
  Binv = B;
  Binv.Invert();
}

/// Recalculate lattice from reciprocal metric tensor (Gstar=transpose(UB)*UB)
void UnitCell::recalculateFromGstar(const DblMatrix &NewGstar) {
  if (NewGstar.numRows() != 3 || NewGstar.numCols() != 3) {
    std::ostringstream msg;
    msg << "UnitCell::recalculateFromGstar - Expected a 3x3 matrix but was "
           "given a "
        << NewGstar.numRows() << "x" << NewGstar.numCols();
    throw std::invalid_argument(msg.str());
  }

  if (NewGstar[0][0] * NewGstar[1][1] * NewGstar[2][2] <= 0.)
    throw std::invalid_argument("NewGstar");
  Gstar = NewGstar;
  calculateReciprocalLattice();
  G = Gstar;
  G.Invert();
  da[0] = sqrt(G[0][0]);                 // a
  da[1] = sqrt(G[1][1]);                 // b
  da[2] = sqrt(G[2][2]);                 // c
  da[3] = acos(G[1][2] / da[1] / da[2]); // alpha
  da[4] = acos(G[0][2] / da[0] / da[2]); // beta
  da[5] = acos(G[0][1] / da[0] / da[1]); // gamma
  calculateB();
}

std::ostream &operator<<(std::ostream &out, const UnitCell &unitCell) {
  // always show the lattice constants
  out << "Lattice Parameters:" << std::fixed << std::setprecision(6)
      << std::setw(12) << unitCell.a() << std::fixed << std::setprecision(6)
      << std::setw(12) << unitCell.b() << std::fixed << std::setprecision(6)
      << std::setw(12) << unitCell.c() << std::fixed << std::setprecision(6)
      << std::setw(12) << unitCell.alpha() << std::fixed << std::setprecision(6)
      << std::setw(12) << unitCell.beta() << std::fixed << std::setprecision(6)
      << std::setw(12) << unitCell.gamma() << std::fixed << std::setprecision(6)
      << " " << std::setw(12) << unitCell.volume();

  // write out the uncertainty if there is a positive one somewhere
  if ((unitCell.errora() > 0) || (unitCell.errorb() > 0) ||
      (unitCell.errorc() > 0) || (unitCell.erroralpha() > 0) ||
      (unitCell.errorbeta() > 0) || (unitCell.errorgamma() > 0))
    out << "\nParameter Errors  :" << std::fixed << std::setprecision(6)
        << std::setw(12) << unitCell.errora() << std::fixed
        << std::setprecision(6) << std::setw(12) << unitCell.errorb()
        << std::fixed << std::setprecision(6) << std::setw(12)
        << unitCell.errorc() << std::fixed << std::setprecision(6)
        << std::setw(12) << unitCell.erroralpha() << std::fixed
        << std::setprecision(6) << std::setw(12) << unitCell.errorbeta()
        << std::fixed << std::setprecision(6) << std::setw(12)
        << unitCell.errorgamma() << std::fixed << std::setprecision(6)
        << std::setw(12) << unitCell.errorvolume();

  return out;
}

std::string unitCellToStr(const UnitCell &unitCell) {
  std::ostringstream stream;
  stream << std::setprecision(9);

  stream << unitCell.a() << " " << unitCell.b() << " " << unitCell.c() << " "
         << unitCell.alpha() << " " << unitCell.beta() << " "
         << unitCell.gamma();

  return stream.str();
}

UnitCell strToUnitCell(const std::string &unitCellString) {

  Mantid::Kernel::StringTokenizer cellTokens(
      unitCellString, " ", Mantid::Kernel::StringTokenizer::TOK_IGNORE_EMPTY);

  std::vector<double> components;
  components.reserve(cellTokens.size());
  for (const auto &token : cellTokens) {
    components.emplace_back(boost::lexical_cast<double>(token));
  }

  switch (components.size()) {
  case 3:
    return UnitCell(components[0], components[1], components[2]);
  case 6:
    return UnitCell(components[0], components[1], components[2], components[3],
                    components[4], components[5]);
  default:
    throw std::runtime_error("Failed to parse unit cell input string: " +
                             unitCellString);
  }
}

} // namespace Geometry
} // namespace Mantid
