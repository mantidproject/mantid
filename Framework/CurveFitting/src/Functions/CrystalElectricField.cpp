// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidCurveFitting/Functions/CrystalElectricField.h"
#include "MantidKernel/WarningSuppressions.h"

#include <algorithm>
#include <array>
#include <cmath>

namespace Mantid {
namespace CurveFitting {
namespace Functions {

namespace {

// The missing braces warning is a false positive -
// https://llvm.org/bugs/show_bug.cgi?id=21629
GNU_DIAG_OFF("missing-braces")

// Get a complex conjugate of the value returned by
// ComplexMatrix::operator(i,j)
ComplexType conjg(const ComplexMatrixValueConverter &conv) {
  return std::conj(static_cast<ComplexType>(conv));
}

// number of rare earth ions (trivalent rare earths with unfilled f-shell)
const int maxNre = 13;

// define some rare earth constants (ggj is the Lande g-factor, ddimj=2J+1)
const std::array<double, maxNre> ggj = {
    6.0 / 7., 4.0 / 5., 8.0 / 11., 3.0 / 5., 2.0 / 7., 0.0,     2.0,
    3.0 / 2., 4.0 / 3., 5.0 / 4.,  6.0 / 5., 7.0 / 6., 8.0 / 7.};

const std::array<double, maxNre> ddimj = {
    6.0, 9.0, 10.0, 9.0, 6.0, 1.0, 8.0, 13.0, 16.0, 17.0, 16.0, 13.0, 8.0};
//---------------------------
// set some natural constants
//---------------------------
const double pi = 4.0 * atan(1.0);
const double kb = 1.38062;     // x 10**(-23) J/K,   Boltzmann constant k_B
const double hh = 6.626075540; // x 10**(-34) J*sec, Planks constant h
const double hq = hh / 2 / pi;
const double ee = 1.6021773349; // x 10**(-19) Coulomb, electric charge
const double me = 9.109389754;  // x 10**(-31) kg, electron mass
//       within the above choose, the factor which connects
//       1meV and 1 Kelvin is fixed and given by:
//       10*ee/kb =: fmevkelvin = 11.6047...
//       this means 1 meV is nearly 11.6 K
const double c_fmevkelvin = 10 * ee / kb;
//  magneton of Bohr in meV per tesla
const double c_myb = hq / me / 2;

//--------------------------------
// define the delta function
//--------------------------------
double delta(double mj, double nj, double j) {
  double res = 0.0;
  if (mj == nj && fabs(mj) <= j && fabs(nj) <= j) {
    res = 1.0;
  }
  return res;
}

//--------------------
//  jp(n) = <n+1|j+|n>
//--------------------
double jp(double nj, double j) {
  if (fabs(nj) <= j) {
    return sqrt(j * (j + 1) - nj * (nj + 1));
  } else {
    return 0.0;
  }
}

//--------------------
//  jm(n) = <n-1|j-|n>
//--------------------
double jm(double nj, double j) {
  if (fabs(nj) <= j) {
    return sqrt(j * (j + 1) - nj * (nj - 1));
  } else {
    return 0.0;
  }
}

//-----------------------
//  jp2(n) = <n+2|j+^2|n>
//-----------------------
double jp2(double nj, double j) { return jp(nj + 1, j) * jp(nj, j); }

//-----------------------
//  jm2(n) = <n-2|j-^2|n>
//-----------------------
double jm2(double nj, double j) { return jm(nj - 1, j) * jm(nj, j); }

//-----------------------
//  jp3(n) = <n+3|j+^3|n>
//-----------------------
double jp3(double nj, double j) {
  return jp(nj + 2, j) * jp(nj + 1, j) * jp(nj, j);
}

//-----------------------
//  jm3(n) = <n-3|j-^3|n>
//-----------------------
double jm3(double nj, double j) {
  return jm(nj - 2, j) * jm(nj - 1, j) * jm(nj, j);
}

//-----------------------
//  jp4(n) = <n+4|j+^4|n>
//-----------------------
double jp4(double nj, double j) {
  return jp(nj + 3, j) * jp(nj + 2, j) * jp(nj + 1, j) * jp(nj, j);
}

//-----------------------
//  jm4(n) = <n-4|j-^4|n>
//-----------------------
double jm4(double nj, double j) {
  return jm(nj - 3, j) * jm(nj - 2, j) * jm(nj - 1, j) * jm(nj, j);
}

//-----------------------
//  jp5(n) = <n+5|j+^5|n>
//-----------------------
double jp5(double nj, double j) {
  return jp(nj + 4, j) * jp(nj + 3, j) * jp(nj + 2, j) * jp(nj + 1, j) *
         jp(nj, j);
}

//-----------------------
//  jm5(n) = <n-5|j-^5|n>
//-----------------------
double jm5(double nj, double j) {
  return jm(nj - 4, j) * jm(nj - 3, j) * jm(nj - 2, j) * jm(nj - 1, j) *
         jm(nj, j);
}

//-----------------------
//  jp6(n) = <n+6|j+^6|n>
//-----------------------
double jp6(double nj, double j) {
  return jp(nj + 5, j) * jp(nj + 4, j) * jp(nj + 3, j) * jp(nj + 2, j) *
         jp(nj + 1, j) * jp(nj, j);
}

//-----------------------
//  jm6(n) = <n-6|j-^6|n>
//-----------------------
double jm6(double nj, double j) {
  return jm(nj - 5, j) * jm(nj - 4, j) * jm(nj - 3, j) * jm(nj - 2, j) *
         jm(nj - 1, j) * jm(nj, j);
}

//-----------------------------
double f20(double nj, double j) { return 3 * pow(nj, 2) - j * (j + 1); }

//------------------------------
double f21(double nj, double) { return nj; }

//------------------------------
double f22(double, double) { return 1.0; }

//------------------------------
double f40(double nj, double j) {
  return 35 * pow(nj, 4) - 30 * j * (j + 1) * pow(nj, 2) + 25 * pow(nj, 2) -
         6 * j * (j + 1) + 3 * pow(j, 2) * pow((j + 1), 2);
}

//------------------------------
double f41(double nj, double j) {
  return 7 * pow(nj, 3) - 3 * j * (j + 1) * nj - nj;
}

//------------------------------
double f42(double nj, double j) { return 7 * pow(nj, 2) - j * (j + 1) - 5; }

//------------------------------
double f43(double nj, double) { return nj; }

//------------------------------
double f44(double, double) { return 1.0; }

//------------------------------
double f60(double nj, double j) {
  return 231 * pow(nj, 6) - 315 * j * (j + 1) * pow(nj, 4) + 735 * pow(nj, 4) +
         105 * pow(j, 2) * pow((j + 1), 2) * pow(nj, 2) -
         525 * j * (j + 1) * pow(nj, 2) + 294 * pow(nj, 2) -
         5 * pow(j, 3) * pow((j + 1), 3) + 40 * pow(j, 2) * pow((j + 1), 2) -
         60 * j * (j + 1);
}

//------------------------------
double f61(double nj, double j) {
  return 33 * pow(nj, 5) - (30 * j * (j + 1) - 15) * pow(nj, 3) -
         10 * j * (j + 1) * nj + 5 * pow(j, 2) * pow((j + 1), 2) * nj + 12 * nj;
}

//------------------------------
double f62(double nj, double j) {
  return 33 * pow(nj, 4) - 18 * j * (j + 1) * pow(nj, 2) - 123 * pow(nj, 2) +
         pow(j, 2) * pow(j + 1, 2) + 10 * j * (j + 1) + 102;
}

//------------------------------
double f63(double nj, double j) {
  return 11 * pow(nj, 3) - 3 * j * (j + 1) * nj - 59 * nj;
}

//------------------------------
double f64(double nj, double j) { return 11 * pow(nj, 2) - j * (j + 1) - 38; }

//------------------------------
double f65(double nj, double) { return nj; }

//------------------------------
double f66(double, double) { return 1.0; }

//-------------------------------
// ff(k,q,nj,j) := fkq(nj,j)
//-------------------------------
double ff(int k, int q, double nj, double j) {
  int qq = abs(q);
  if (k == 2 && qq == 0) {
    return f20(nj, j);
  } else if (k == 2 && qq == 1) {
    return f21(nj, j);
  } else if (k == 2 && qq == 2) {
    return f22(nj, j);
  } else if (k == 4 && qq == 0) {
    return f40(nj, j);
  } else if (k == 4 && qq == 1) {
    return f41(nj, j);
  } else if (k == 4 && qq == 2) {
    return f42(nj, j);
  } else if (k == 4 && qq == 3) {
    return f43(nj, j);
  } else if (k == 4 && qq == 4) {
    return f44(nj, j);
  } else if (k == 6 && qq == 0) {
    return f60(nj, j);
  } else if (k == 6 && qq == 1) {
    return f61(nj, j);
  } else if (k == 6 && qq == 2) {
    return f62(nj, j);
  } else if (k == 6 && qq == 3) {
    return f63(nj, j);
  } else if (k == 6 && qq == 4) {
    return f64(nj, j);
  } else if (k == 6 && qq == 5) {
    return f65(nj, j);
  } else if (k == 6 && qq == 6) {
    return f66(nj, j);
  }
  throw std::runtime_error("Unknown case in ff function.");
}

// c----------------------------------
// c testing if the real number r is 0
// c----------------------------------
void ifnull(double &r) {
  const double macheps = 1.0e-14;
  if (fabs(r) <= macheps)
    r = 0.0;
}

//---------------------------------------------
// calculates the neutron scattering radius r0
//---------------------------------------------
double c_r0() {
  return -1.91 * pow(ee, 2) / me; // * 10**(-12) cm
}

// c---------------------------------------
double epsilon(int k, int q) {
  const std::array<double, 49> eps = {
      1.00000000000000,   0.707106781186547,  1.00000000000000,
      -0.707106781186547, 0.612372435695795,  1.22474487139159,
      0.500000000000000,  -1.22474487139159,  0.612372435695795,
      0.559016994374947,  1.36930639376292,   0.433012701892219,
      0.500000000000000,  -0.433012701892219, 1.36930639376292,
      -0.559016994374947, 0.522912516583797,  1.47901994577490,
      0.395284707521047,  0.559016994374947,  0.125000000000000,
      -0.559016994374947, 0.395284707521047,  -1.47901994577490,
      0.522912516583797,  0.496078370824611,  1.56873754975139,
      0.369754986443726,  0.603807364424560,  0.114108866146910,
      0.125000000000000,  -0.114108866146910, 0.603807364424560,
      -0.369754986443726, 1.56873754975139,   -0.496078370824611,
      0.474958879799083,  1.64530582263602,   0.350780380010057,
      0.640434422872475,  0.106739070478746,  0.135015431216830,
      0.062500000000000,  -0.135015431216830, 0.106739070478746,
      -0.640434422872475, 0.350780380010057,  -1.64530582263602,
      0.474958879799083};
  return eps[k * (k + 1) + q];
}

// c---------------------------------------
double omega(int k, int q) {
  const std::array<double, 49> oma = {
      1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0,
      1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0,
      1.0, 1.0, 3.0, 3.0, 1.0, 3.0, 3.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0,
      1.0, 3.0, 3.0, 1.0, 3.0, 3.0, 1.0, 1.0, 1.0, 1.0};

  return oma[k * (k + 1) + q];
}

//--------------------------------------------------------
// Function to calculate factorial
//--------------------------------------------------------
double fac(double n) {
  if (n < 0.0)
    return 0.0;
  if (n == 0.0)
    return 1.0;
  double f = 1.0;
  int m = static_cast<int>(std::floor(n));
  for (int i = 1; i <= m; ++i) {
    f *= i;
  }
  return f;
}

//--------------------------------------------------------
// binom (n over k)
//--------------------------------------------------------
double binom(int n, int k) {
  return fac(double(n)) / fac(double(k)) / fac(double(n - k));
}

//--------------------------------------------------------
//              (k)
// calculates  D    (a,b,c)   [ The Wigner D-matrix ]
//              ms m
//
// see Lindner A, 'Drehimpulse in der Quantenmechanik',
// ISBN 3-519-03061-6                  (j)
// Stuttgart: Teubner, 1984  page 86 (d    (beta)=...)
// for equation (1)                    m ms
//
// see Buckmaster phys. stat. sol. (a) 13 (1972) 9
// for equation (2)
//
//--------------------------------------------------------
ComplexType ddrot(int j, int m, int ms, double a, double b, double c) {
  // c       equation (1)
  double d = delta(double(ms), double(m), double(j));
  ifnull(b);
  if (b != 0.0) {
    d = 0.0;
    for (int n = std::max(0, -(m + ms)); n <= std::min(j - m, j - ms);
         ++n) { // do n=max(0,-(m+ms)),min(j-m,j-ms)
      d = d + pow(-1.0, (j - ms - n)) * binom(j - ms, n) *
                  binom(j + ms, j - m - n) *
                  pow(cos(0.5 * b), (2 * n + m + ms)) *
                  pow(sin(0.5 * b), (2 * j - 2 * n - m - ms));
    }
    d = d * sqrt(fac(double(j + m)) / fac(double(j + ms)) * fac(double(j - m)) /
                 fac(double(j - ms)));
  }
  // c       equation (2)
  return ComplexType(cos(m * a), -sin(m * a)) * d *
         ComplexType(cos(ms * c), -sin(ms * c));
}
//--------------------------------------------------
// <jm| jp^|q| or jm^|q|  |nj>
//--------------------------------------------------
double jop(int q, double mj, double nj, double j) {
  switch (q) {
  case 0:
    return delta(mj, nj, j);
  case 1:
    return delta(mj, nj + 1, j) * jp(nj, j);
  case 2:
    return delta(mj, nj + 2, j) * jp2(nj, j);
  case 3:
    return delta(mj, nj + 3, j) * jp3(nj, j);
  case 4:
    return delta(mj, nj + 4, j) * jp4(nj, j);
  case 5:
    return delta(mj, nj + 5, j) * jp5(nj, j);
  case 6:
    return delta(mj, nj + 6, j) * jp6(nj, j);
  case -1:
    return delta(mj, nj - 1, j) * jm(nj, j);
  case -2:
    return delta(mj, nj - 2, j) * jm2(nj, j);
  case -3:
    return delta(mj, nj - 3, j) * jm3(nj, j);
  case -4:
    return delta(mj, nj - 4, j) * jm4(nj, j);
  case -5:
    return delta(mj, nj - 5, j) * jm5(nj, j);
  case -6:
    return delta(mj, nj - 6, j) * jm6(nj, j);
  default:
    throw std::runtime_error("Cannot calculate jop with this q value.");
  }
}

//--------------------------------------------------
// calculate the full stevens operators
//--------------------------------------------------
double full_okq(int k, int q, double mj, double nj, double j) {
  return 0.5 * jop(q, mj, nj, j) * (ff(k, q, mj, j) + ff(k, q, nj, j));
}

//-------------------------------
//  calculates <i|j+|k>
//-------------------------------
ComplexType matjp(const ComplexFortranMatrix &ev, int i, int k, int dim) {
  ComplexFortranVector v(1, dim);
  auto j = 0.5 * (double(dim) - 1.0);
  for (int s = dim; s >= 2; --s) { // do 10 s=dim,2,-1
    auto sj = double(s) - j - 1.0;
    v(s) = ev(s - 1, k) * jp(sj - 1, j);
  }
  v(1) = 0.0;
  ComplexType res = 0.0;
  for (int s = 1; s <= dim; ++s) { // do 20 s=1,dim
    res += conjg(ev(s, i)) * v(s);
  }
  return res;
}
//--------------------
// calculates <i|j-|k>
//--------------------
ComplexType matjm(const ComplexFortranMatrix &ev, int i, int k, int dim) {
  ComplexFortranVector v(1, dim);
  auto j = 0.5 * (double(dim) - 1.0);
  for (int s = 1; s <= dim - 1; ++s) { // do 10 s=1,dim-1
    auto sj = double(s) - j - 1.0;
    v(s) = ev(s + 1, k) * jm(sj + 1, j);
  }
  v(dim) = 0.0;
  ComplexType res = 0.0;
  for (int s = 1; s <= dim; ++s) { // do 20 s=1,dim
    res += conjg(ev(s, i)) * v(s);
  }
  return res;
}
//--------------------
// calculates <i|jx|k>
//--------------------
ComplexType matjx(const ComplexFortranMatrix &ev, int i, int k, int dim) {
  return 0.5 * (matjm(ev, i, k, dim) + matjp(ev, i, k, dim));
}
//-------------------------
// calculates |<i|jx|k>|**2
//-------------------------
double matjx2(const ComplexFortranMatrix &ev, int i, int k, int dim) {
  return std::norm(matjx(ev, i, k, dim));
}
//--------------------
// calculates <i|jy|k>
//--------------------
ComplexType matjy(const ComplexFortranMatrix &ev, int i, int k, int dim) {
  auto ci = ComplexType(0.0, 1.0);
  return 0.5 * ci * (matjm(ev, i, k, dim) - matjp(ev, i, k, dim));
}
//-------------------------
// calculates |<i|jy|k>|**2
//-------------------------
double matjy2(const ComplexFortranMatrix &ev, int i, int k, int dim) {
  return std::norm(matjy(ev, i, k, dim));
}
//--------------------
// calculates <i|jz|k>
//--------------------
ComplexType matjz(const ComplexFortranMatrix &ev, int i, int k, int dim) {
  ComplexFortranVector v(1, dim);
  auto j = 0.5 * (double(dim) - 1.0);
  for (int s = 1; s <= dim; ++s) { // do 10 s=1,dim
    auto sj = double(s) - j - 1.0;
    v(s) = ev(s, k) * sj;
  }
  ComplexType res = 0.0;
  for (int s = 1; s <= dim; ++s) { //	do 20 s=1,dim
    res += conjg(ev(s, i)) * v(s);
  }
  return res;
}
//-------------------------
// calculates |<i|jz|k>|**2
//-------------------------
double matjz2(const ComplexFortranMatrix &ev, int i, int k, int dim) {
  return std::norm(matjz(ev, i, k, dim));
}

//---------------------------------------------------------------
// calculates all transition matrix elements for a single crystal
// and a polycrystalline sample (powder)
//---------------------------------------------------------------
void matcalc(const ComplexFortranMatrix &ev, int dim, DoubleFortranMatrix &jx2,
             DoubleFortranMatrix &jy2, DoubleFortranMatrix &jz2,
             DoubleFortranMatrix &jt2) {
  for (int i = 1; i <= dim; ++i) {   // do 10 i=1,dim
    for (int k = 1; k <= dim; ++k) { // do 20 k=1,dim
      jx2(i, k) = matjx2(ev, i, k, dim);
      jy2(i, k) = matjy2(ev, i, k, dim);
      jz2(i, k) = matjz2(ev, i, k, dim);
      jt2(i, k) = 2.0 / 3 * (jx2(i, k) + jy2(i, k) + jz2(i, k));
    }
  }
}

//------------------------------------------------------------
// make sure that no underflow and overflow appears within exp
//------------------------------------------------------------
double exp_(double z) {
  const double zmax = 71.0;
  if (z < -zmax) {
    return 0.0;
  }
  if (z < zmax) {
    return exp(z);
  }
  return exp(zmax);
}

//------------------------------------------
// calculates all transition intensities for
// a polycrystalline sample (powder)
//------------------------------------------
void intcalc(double r0, double gj, double z, const DoubleFortranMatrix &jt2,
             const DoubleFortranVector &e, DoubleFortranMatrix &inten, int dim,
             double temp) {
  // Original code from FOCUS calculated integrated intensity in barn
  // auto constant = 4.0 * pi * pow(0.5 * r0 * gj, 2);
  // ISIS normalised data is in milibarn/steradian - need to multiply
  // by 1000 / 4 / PI
  auto constant = pow(0.5 * r0 * gj, 2) * 1000.;
  if (temp == 0.0) {
    temp = 1.0;
  }
  // convert temperature to meV
  temp /= c_fmevkelvin;

  for (int i = 1; i <= dim; ++i) { // do 10 i=1,dim
    auto coeff = exp_(-e(i) / temp) / z * constant;
    for (int k = 1; k <= dim; ++k) { // do 20 k=1,dim
      inten(i, k) = coeff * jt2(i, k);
    }
  }
}
//-------------------------------------
// calculation of the occupation factor
//-------------------------------------
double c_occupation_factor(const DoubleFortranVector &energy, double dimj,
                           double temp) {
  int dim = static_cast<int>(dimj);
  double occupation_factor = 0.0;
  if (temp == 0.0) {
    temp = 1.0;
  }
  // convert temperature to meV
  temp /= c_fmevkelvin;
  for (int s = 1; s <= dim; ++s) { // do 10 s=1,dim
    occupation_factor += exp_(-energy(s) / temp);
  }
  return occupation_factor;
}

//--------------------------------------
// calculation of the zeeman hamiltonian
//--------------------------------------
void zeeman(ComplexFortranMatrix &hamiltonian, const int nre,
            const DoubleFortranVector &bext, const DoubleFortranVector &bmol) {
  auto i = ComplexType(0.0, 1.0);
  auto bmolp = bmol(1) + i * bmol(2);
  auto bmolm = bmol(1) - i * bmol(2);
  auto bmolz = bmol(3);
  auto bextp = bext(1) + i * bext(2);
  auto bextm = bext(1) - i * bext(2);
  ComplexType bextz = bext(3);
  auto gj = (nre > 0) ? ggj[nre - 1] : 2.;
  auto facmol = 2 * (gj - 1) * c_myb;
  auto facext = gj * c_myb;
  // Negative nre means arbitrary J, with abs(nre) = 2J. dimj=2J+1
  auto dimj = (nre > 0) ? ddimj[nre - 1] : (abs(nre) + 1);
  auto j = 0.5 * (dimj - 1.0);
  int dim = static_cast<int>(dimj);
  hamiltonian.allocate(1, dim, 1, dim);
  hamiltonian.zero();
  //-------------------------------------------------------------------
  //       define only the lower triangle of h(m,n)
  //-------------------------------------------------------------------
  for (int m = 1; m <= dim; ++m) { //	do 10 m=1,dim
    auto mj = double(m) - j - 1.0;
    for (int n = 1; n <= m; ++n) { // do 20 n=1,m
      auto nj = double(n) - j - 1.0;
      // add the molecular field
      //  f*J*B = f*( 1/2*(J+ * B-  +  J- * B+) + Jz*Bz )
      hamiltonian(m, n) =
          hamiltonian(m, n) +
          0.5 * facmol * bmolm * delta(mj, nj + 1, j) * jp(nj, j) +
          0.5 * facmol * bmolp * delta(mj, nj - 1, j) * jm(nj, j) +
          facmol * bmolz * delta(mj, nj, j) * nj +
          // c add an external magnetic field
          0.5 * facext * bextm * delta(mj, nj + 1, j) * jp(nj, j) +
          0.5 * facext * bextp * delta(mj, nj - 1, j) * jm(nj, j) +
          facext * bextz * delta(mj, nj, j) * nj;
      hamiltonian(n, m) = conjg(hamiltonian(m, n));
    }
  }
}

//---------------------------------------
// Calculation of the eigenvalues/vectors
//---------------------------------------
void diagonalise(const ComplexFortranMatrix &hamiltonian,
                 DoubleFortranVector &eigenvalues,
                 ComplexFortranMatrix &eigenvectors) {
  // Diagonalisation of the hamiltonian
  auto dim = hamiltonian.len1();
  eigenvalues.allocate(1, dim);
  eigenvectors.allocate(1, dim, 1, dim);
  ComplexFortranMatrix h = hamiltonian;
  h.eigenSystemHermitian(eigenvalues, eigenvectors);

  // Sort the eigenvalues in ascending order
  auto sortedIndices = eigenvalues.sortIndices();
  eigenvalues.sort(sortedIndices);
  // Eigenvectors are in columns. Sort the columns
  // to match the sorted eigenvalues.
  eigenvectors.sortColumns(sortedIndices);

  // Shift the lowest energy level to 0
  auto indexMin = static_cast<int>(eigenvalues.indexOfMinElement() + 1);
  auto eshift = eigenvalues(indexMin);
  eigenvalues += -eshift;
}

GNU_DIAG_OFF("missing-braces")

} // anonymous namespace

/// Calculates the eigenvalues/vectors of a crystal field Hamiltonian in a
/// specified external magnetic field.
/// @param eigenvalues :: Output. The eigenvalues in ascending order. The
/// smallest value is subtracted from all eigenvalues so they always
/// start with 0.
/// @param eigenvectors :: Output. The matrix of eigenvectors. The eigenvectors
///    are in columns with indices corresponding to the indices of eigenvalues.
/// @param hamiltonian  :: The crystal field hamiltonian in meV.
/// @param nre :: A number denoting the type of ion.
///  |1=Ce|2=Pr|3=Nd|4=Pm|5=Sm|6=Eu|7=Gd|8=Tb|9=Dy|10=Ho|11=Er|12=Tm|13=Yb|
/// @param bext :: The external field in Cartesians (Hx, Hy, Hz) in Tesla
///    The z-axis is parallel to the crystal field quantisation axis.
void calculateZeemanEigensystem(DoubleFortranVector &eigenvalues,
                                ComplexFortranMatrix &eigenvectors,
                                const ComplexFortranMatrix &hamiltonian,
                                int nre, const DoubleFortranVector &bext) {
  ComplexFortranMatrix h = hamiltonian;
  DoubleFortranVector bmol(1, 3);
  bmol.zero();
  // Adds the external and molecular fields
  ComplexFortranMatrix hz;
  zeeman(hz, nre, bext, bmol);
  h -= hz;
  // Now run the actual diagonalisation
  diagonalise(h, eigenvalues, eigenvectors);
}

/// Calculate eigenvalues and eigenvectors of the crystal field hamiltonian.
/// @param eigenvalues :: Output. The eigenvalues in ascending order. The
/// smallest value is subtracted from all eigenvalues so they always
/// start with 0.
/// @param eigenvectors :: Output. The matrix of eigenvectors. The eigenvectors
///    are in columns with indices corresponding to the indices of eigenvalues.
/// @param hamiltonian  :: Output. The crystal field hamiltonian.
/// @param hzeeman  :: Output. The zeeman hamiltonian.
/// @param nre :: A number denoting the type of ion.
///  |1=Ce|2=Pr|3=Nd|4=Pm|5=Sm|6=Eu|7=Gd|8=Tb|9=Dy|10=Ho|11=Er|12=Tm|13=Yb|
/// @param bmol :: The molecular field in Cartesian (Bx, By, Bz) in Tesla
/// @param bext :: The external field in Cartesian (Hx, Hy, Hz) in Tesla
///    The z-axis is parallel to the crystal field quantisation axis.
/// @param bkq :: The crystal field parameters in meV.
/// @param alpha_euler :: The alpha Euler angle in radians
/// @param beta_euler :: The beta Euler angle in radians
/// @param gamma_euler :: The gamma Euler angle in radians
void calculateEigensystem(DoubleFortranVector &eigenvalues,
                          ComplexFortranMatrix &eigenvectors,
                          ComplexFortranMatrix &hamiltonian,
                          ComplexFortranMatrix &hzeeman, int nre,
                          const DoubleFortranVector &bmol,
                          const DoubleFortranVector &bext,
                          const ComplexFortranMatrix &bkq, double alpha_euler,
                          double beta_euler, double gamma_euler) {
  if (nre > maxNre) {
    throw std::out_of_range("nre is out of range");
  }

  // initialize some rare earth constants
  auto dimj = (nre > 0) ? ddimj[nre - 1] : (abs(nre) + 1);

  //------------------------------------------------------------
  //       transform the Bkq with
  //       H = sum_k=0 Bk0 Ok0 + sum_k>0_q>0  ReBkq ReOkq + ImBkq ImOkq
  //       to a representation with
  //                     *
  //       H = sum_kq dkq  Okq
  //
  //       one finds:   dk0 = Bk0  and for q<>0: dkq = Bkq/2
  //-------------------------------------------------------
  ComplexFortranMatrix dkq_star(1, 6, -6, 6);
  dkq_star.zero();
  auto i = ComplexType(0.0, 1.0);
  for (int k = 2; k <= 6; k += 2) { // do k=2,6,2
    for (int q = 0; q <= k; ++q) {  //  do q=0,k
      ComplexType b_kq = bkq(k, q);
      dkq_star(k, q) = b_kq;
      if (q != 0) {
        dkq_star(k, q) = dkq_star(k, q) / 2.0;
      }
      dkq_star(k, -q) = conjg(dkq_star(k, q));
    }
  }
  //-------------------------------------------------------------------
  //  the parameters are   conjg(D_kq)
  //-------------------------------------------------------------------
  for (int k = 2; k <= 6; k += 2) { // do k=2,6,2
    for (int q = -k; q <= k; ++q) { // do q=-k,k
      dkq_star(k, q) = conjg(dkq_star(k, q));
    }
  }
  //-------------------------------------------------------------------
  // Rotate the crystal field quantisation axis by the specified
  // Euler angles. In some cases the number of CF parameters can be
  // reduced by chosing the quantisation axis along a high symmetry
  // rotation axis, rather than along a crystallographic axis.
  // The eigenvalues should remain the same, but the eigenvectors will
  // change.
  // The rotation is done using a Wigner D-matrix. As noted by
  // Buckmaster, the Stevens operators cannot be rotated as is, because
  // the have an inconsistent normalisation between differen k, q terms
  // Thus they have to be converted to and from the "Wybourne"
  // normalisation using the epsilon / omega values.
  // There was a bug in the original FOCUS code. Multiplying by
  // omega*epsilon converts Wybourne parameters to Stevens parameters.
  // Thus to convert the original dkq_star(k,qs) from Steven to Wybourn
  // we should divide by epsilon(k,qs)*omega(k,qs) and then multiply by
  // the new dkq_star(k,q) by epsilon(k,q)*omega(k,q).
  //-------------------------------------------------------------------
  ComplexFortranMatrix rdkq_star(1, 6, -6, 6);
  for (int k = 2; k <= 6; k += 2) { // do k=2,6,2
    for (int q = -k; q <= k; ++q) { // do q=-k,k
      rdkq_star(k, q) = ComplexType(0.0, 0.0);
      for (int qs = -k; qs <= k; ++qs) { // do qs=-k,k
        rdkq_star(k, q) =
            rdkq_star(k, q) +
            dkq_star(k, qs) * epsilon(k, q) / epsilon(k, qs) * omega(k, q) /
                omega(k, qs) *
                ddrot(k, q, qs, alpha_euler, beta_euler, gamma_euler);
      }
    }
  }
  // -------------------------------------------------------------------
  // rotate the external magnetic field
  // -------------------------------------------------------------------
  //       aj  = sqrt(3/j/(j+1)/(2*j+1))
  //
  //       the following hamiltonian will be rotated
  //
  //        cry        -----    cry    cry
  //       h     = f * >     Bex     V       with  <j|| V1 ||j> = sqrt(3)
  //        mag        -----    1q    1q
  //                     q
  // -------------------------------------------------------------------

  ComplexFortranMatrix bex(1, 1, -1, 1);
  bex(1, 1) = -(bext(1) - i * bext(2)) * M_SQRT1_2;
  bex(1, -1) = (bext(1) + i * bext(2)) * M_SQRT1_2;
  bex(1, 0) = bext(3);

  //  calculates Bex(1,q) for a canted moment
  //
  //       moment    cry       cry   cry     +
  //      h       = R   (abc) h     R   (abc)
  //       mag                 mag
  //
  ComplexFortranMatrix rbex(1, 1, -1, 1);
  for (int q = -1; q <= 1; ++q) { // do q=-1,1
    rbex(1, q) = ComplexType(0.0, 0.0);
    for (int qs = -1; qs <= 1; ++qs) { // do qs=-1,1
      rbex(1, q) = rbex(1, q) + bex(1, qs) * ddrot(1, q, qs, alpha_euler,
                                                   beta_euler, gamma_euler);
    }
  }
  DoubleFortranVector rbext(1, 3);
  rbext(1) = real(static_cast<ComplexType>(rbex(1, -1))) * M_SQRT2;
  rbext(2) = imag(static_cast<ComplexType>(rbex(1, -1))) * M_SQRT2;
  rbext(3) = real(static_cast<ComplexType>(rbex(1, 0)));

  int dim = static_cast<int>(dimj);
  hamiltonian.allocate(1, dim, 1, dim);
  hamiltonian.zero();
  //-------------------------------------------------------------------
  //      calculate the crystal field hamiltonian
  //      define only the lower triangle of h(m,n)
  //-------------------------------------------------------------------
  // total momentum J of R3+
  auto j = 0.5 * (dimj - 1.0);
  for (int m = 1; m <= dim; ++m) { // do m=1,dim
    auto mj = double(m) - j - 1.0;
    for (int n = 1; n <= m; ++n) { // do n=1,m
      auto nj = double(n) - j - 1.0;
      for (int k = 2; k <= 6; k += 2) { // do k=2,6,2
        for (int q = -k; q <= k; ++q) { // do q=-k,k
          hamiltonian(m, n) =
              hamiltonian(m, n) + rdkq_star(k, q) * full_okq(k, q, mj, nj, j);
        }
      }
      hamiltonian(n, m) = conjg(hamiltonian(m, n));
    }
  }

  // Adds the external and molecular fields
  zeeman(hzeeman, nre, rbext, bmol);
  hamiltonian -= hzeeman;

  // Now run the actual diagonalisation
  diagonalise(hamiltonian, eigenvalues, eigenvectors);
}

//-------------------------
// transforms the indices
//-------------------------
int no(int i, const IntFortranVector &d, int n) {
  // implicit none
  // integer i,d(17*17),n,up,lo
  int up = 0;
  for (int nno = 1; nno <= n; ++nno) { // do no=1,n
    int lo = up + 1;
    up = lo + d(nno) - 1;
    if (i >= lo && i <= up) {
      return nno;
    }
  }
  return n;
}

/// Find out how many degenerated energy levels exists.
/// Store the intensities of the degenarated levels.
/// @param energy :: The energies.
/// @param mat :: The transition matrix elements. (Intensities without
///    considering degeneracy).
/// @param degeneration :: Degeneration number for each transition.
/// @param e_energies :: Energy values of the degenerated energy levels.
/// @param i_energies :: Intensities of the degenerated energy levels.
/// @param de :: Energy levels which are closer than de are assumed to be
///              degenerated.
void deg_on(const DoubleFortranVector &energy, const DoubleFortranMatrix &mat,
            IntFortranVector &degeneration, DoubleFortranVector &e_energies,
            DoubleFortranMatrix &i_energies, double de) {
  //  real*8  energy(17)           ! already defined in CF_FABI.INC
  //  real*8  mat(17,17)
  //	integer degeneration(17*17)  ! stores the degeneration of a level
  // 	integer n_energies           ! no. of degenerated energy levels
  //	real*8  e_energies(17*17)    ! energy values of the degenerated energy
  //                                 levels
  //	real*8  i_energies(17,17)    ! intensities of the degenerated energy
  //                                 levels
  //  real*8 de,di

  //	energy levels which are closer than de are assumed to be degenerated
  //	only those excitations are taken into account whose intensities are
  //	greater equal than di

  int dim = static_cast<int>(energy.size());
  IntFortranVector tempDegeneration(dim);
  DoubleFortranVector tempEnergies(dim);

  // find out how many degenerated energy levels exists
  tempEnergies(1) = 0.0;
  tempDegeneration(1) = 1;
  int k = 1;
  for (int i = 2; i <= dim; ++i) { // do i=2,dim
    if (std::fabs(tempEnergies(k) - energy(i)) >= de) {
      k = k + 1;
      tempEnergies(k) = energy(i);
      tempDegeneration(k) = 1;
    } else {
      tempDegeneration(k) = tempDegeneration(k) + 1;
    }
  }
  int n_energies = k;

  // Resize the output arrays
  degeneration.allocate(n_energies);
  e_energies.allocate(n_energies);
  i_energies.allocate(n_energies, n_energies);

  // store the intensities of the degenarated levels
  i_energies.zero();
  for (int i = 1; i <= dim; ++i) { // do i=1,dim
    int ii = no(i, tempDegeneration, n_energies);
    degeneration(ii) = tempDegeneration(ii);
    e_energies(ii) = tempEnergies(ii);
    for (int k = 1; k <= dim; ++k) { // do k=1,dim
      int kk = no(k, tempDegeneration, n_energies);
      i_energies(ii, kk) = i_energies(ii, kk) + mat(i, k);
    }
  }
}

/// Calculate the intensities of transitions.
/// @param nre :: Ion number.
/// @param energies :: The energies.
/// @param wavefunctions :: The wavefunctions.
/// @param temperature :: The temperature.
/// @param de :: Energy levels which are closer than de are assumed to be
///              degenerated.
/// @param degeneration :: Degeneration number for each transition.
/// @param e_energies :: Energy values of the degenerated energy levels.
/// @param i_energies :: Intensities of the degenerated energy levels.
void calculateIntensities(int nre, const DoubleFortranVector &energies,
                          const ComplexFortranMatrix &wavefunctions,
                          double temperature, double de,
                          IntFortranVector &degeneration,
                          DoubleFortranVector &e_energies,
                          DoubleFortranMatrix &i_energies) {
  int dim = static_cast<int>(energies.size());
  auto dimj = (nre > 0) ? ddimj[nre - 1] : (abs(nre) + 1);
  if (static_cast<double>(dim) != dimj) {
    throw std::runtime_error("calculateIntensities was called for a wrong ion");
  }

  // calculates the transition matrixelements for a single crystal and
  // a powdered sample
  DoubleFortranMatrix jx2mat(1, dim, 1, dim);
  DoubleFortranMatrix jy2mat(1, dim, 1, dim);
  DoubleFortranMatrix jz2mat(1, dim, 1, dim);
  DoubleFortranMatrix jt2mat(1, dim, 1, dim);
  matcalc(wavefunctions, dim, jx2mat, jy2mat, jz2mat, jt2mat);

  // calculates the sum over all occupation_factor
  auto occupation_factor = c_occupation_factor(energies, dimj, temperature);

  // calculates the transition intensities for a powdered sample
  auto r0 = c_r0();
  auto gj = (nre > 0) ? ggj[nre - 1] : 2.;
  DoubleFortranMatrix mat(1, dim, 1, dim);
  intcalc(r0, gj, occupation_factor, jt2mat, energies, mat, dim, temperature);

  deg_on(energies, mat, degeneration, e_energies, i_energies, de);
}

/// Calculate the excitations (transition energies) and their intensities.
/// Take account of any degeneracy.
/// @param e_energies :: Energy values of the degenerated energy levels.
/// @param i_energies :: Intensities of the degenerated energy levels.
/// @param de :: Excitations which are closer than de are assumed to be
///              degenerated.
/// @param di :: Only those excitations are taken into account whose intensities
///              are greater or equal than di.
/// @param e_excitations :: The output excitation energies.
/// @param i_excitations :: The output excitation intensities.
void calculateExcitations(const DoubleFortranVector &e_energies,
                          const DoubleFortranMatrix &i_energies, double de,
                          double di, DoubleFortranVector &e_excitations,
                          DoubleFortranVector &i_excitations) {
  int n_energies = static_cast<int>(e_energies.size());
  auto dimj = n_energies;
  // Calculate transition energies (excitations) and corresponding
  // intensities.
  DoubleFortranVector eex(n_energies * n_energies);
  DoubleFortranVector iex(n_energies * n_energies);
  int nex = 0;
  for (int i = 1; i <= n_energies; ++i) {   // do i=1,n_energies
    for (int k = 1; k <= n_energies; ++k) { // do k=1,n_energies
      nex = nex + 1;
      eex(nex) = e_energies(k) - e_energies(i);
      iex(nex) = i_energies(i, k); // !I(i->k)
      ifnull(eex(nex));
    }
  }
  // nex at this point is the largest possible number
  // of transitions

  // Sort excitations in ascending order.
  std::vector<size_t> ind = eex.sortIndices();
  eex.sort(ind);

  // Define lambda that transforms C++ indices to fortran indices
  auto index = [&ind](int i) { return static_cast<int>(ind[i - 1] + 1); };

  DoubleFortranVector tempEex(nex);
  IntFortranVector degeneration(nex);
  tempEex(1) = eex(1);
  degeneration(1) = 1;

  int k = 1;
  // Check if there are any overlapping (degenerate) excitations
  for (int i = 2; i <= nex; ++i) { // do i=2,nex
    if (std::fabs(tempEex(k) - eex(i)) >= de) {
      k = k + 1;
      tempEex(k) = eex(i);
      degeneration(k) = 1;
    } else {
      degeneration(k) = degeneration(k) + 1;
    }
  }
  // Number of non-overlapping transitions
  int n_excitation = k;

  DoubleFortranVector tempIex(n_excitation);
  for (int i = 1; i <= nex; ++i) { // do i=1,nex
    auto ii = no(i, degeneration, n_excitation);
    tempIex(ii) = tempIex(ii) + iex(index(i));
  }

  ind = tempIex.sortIndices(false);
  tempIex.sort(ind);

  // Keep only transitions that are strong enough
  // i >= di
  e_excitations.allocate(n_excitation);
  i_excitations.allocate(n_excitation);
  k = 0;
  for (int i = 1; i <= n_excitation; ++i) { // do i=1,n_excitation
    if (tempIex(i) >= di || dimj == 1) {
      k = k + 1;
      i_excitations(k) = tempIex(i);
      e_excitations(k) = tempEex(index(i));
    }
  }
  // nex now is the actual number of excitations that will
  // be outputted.
  nex = k;
  if (nex != n_excitation) {
    e_excitations.allocate(nex);
    i_excitations.allocate(nex);
  }
}

/// Calculate the diagonal matrix elements of the magnetic moment operator
/// in a particular eigenvector basis.
/// @param ev :: Input. The eigenvector basis.
/// @param Hdir :: Input. Cartesian direction of the magnetic moment operator
/// @param nre :: Input. The ion number to calculate for.
/// @param moment :: Output. The diagonal elements of the magnetic moment matrix
void calculateMagneticMoment(const ComplexFortranMatrix &ev,
                             const DoubleFortranVector &Hdir, const int nre,
                             DoubleFortranVector &moment) {
  int dim = (nre > 0) ? (int)ddimj[nre - 1] : (abs(nre) + 1);
  auto gj = (nre > 0) ? ggj[nre - 1] : 2.;
  moment.allocate(dim);
  for (auto i = 1; i <= dim; ++i) {
    moment(i) = real(matjx(ev, i, i, dim)) * Hdir(1) + // <ev|jx|ev>
                real(matjy(ev, i, i, dim)) * Hdir(2) + // <ev|jy|ev>
                real(matjz(ev, i, i, dim)) * Hdir(3);  // <ev|jz|ev>
  }
  moment *= gj;
}

/// Calculate the full magnetic moment matrix in a particular eigenvector basis.
/// @param ev :: Input. The eigenvector basis.
/// @param Hdir :: Input. Cartesian direction of the magnetic moment operator
/// @param nre :: Input. The ion number to calculate for.
/// @param mumat :: Output. The matrix elements of the magnetic moment matrix
void calculateMagneticMomentMatrix(const ComplexFortranMatrix &ev,
                                   const std::vector<double> &Hdir,
                                   const int nre, ComplexFortranMatrix &mumat) {
  int dim = (nre > 0) ? (int)ddimj[nre - 1] : (abs(nre) + 1);
  auto gj = (nre > 0) ? ggj[nre - 1] : 2.;
  mumat.allocate(1, dim, 1, dim);
  for (auto i = 1; i <= dim; ++i) {
    for (auto j = 1; j <= dim; ++j) {
      mumat(i, j) = matjx(ev, i, j, dim) * Hdir[0] + // <ev|jx|ev'>
                    matjy(ev, i, j, dim) * Hdir[1] + // <ev|jy|ev'>
                    matjz(ev, i, j, dim) * Hdir[2];  // <ev|jz|ev'>
    }
  }
  mumat *= gj;
}

} // namespace Functions
} // namespace CurveFitting
} // namespace Mantid
