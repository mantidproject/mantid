//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidCurveFitting/Functions/CrystalElectricField.h"

#include <array>
#include <cmath>
#include <iostream>

namespace Mantid {
namespace CurveFitting {

namespace Functions {

namespace {

// Get a complex conjugate of the value returned by
// ComplexMatrix::operator(i,j)
ComplexType conjg( const ComplexMatrixValueConverter& conv) {
  return std::conj(static_cast<ComplexType>(conv));
}

//TODO: these should be input parameters
const double alpha_euler = 0.0;
const double beta_euler = 0.0;
const double gamma_euler = 0.0;

// number of rare earth ions (?)
const size_t maxNre = 13;
// [max?] size of the Hamiltonian
const size_t sizeOfHam = 17;
// some size of something in array definitions of the form '0:6'
const size_t N0_6 = 7;

// define some rare earth constants (?)
const std::array<double, maxNre> ggj = {
    6.0 / 7., 4.0 / 5., 8.0 / 11., 3.0 / 5., 2.0 / 7., 0.0,     2.0,
    3.0 / 2., 4.0 / 3., 5.0 / 4.,  6.0 / 5., 7.0 / 6., 8.0 / 7.};

const std::array<double, maxNre> ddimj = {
    6.0, 9.0, 10.0, 9.0, 6.0, 1.0, 8.0, 13.0, 16.0, 17.0, 16.0, 13.0, 8.0};
//       -----------------------------------------------
//       for the values of alphaj,betaj and gammaj
//       see: Abragam and Bleaney, Electronic Paramagnetic
//            Resonance of Transition Ions, 1970
//            appendix B, table 20, page 874-875
//
const std::array<double, maxNre> aalphaj = {
    -1.0 * 2 / 5 / 7,
    -1.0 * 2 * 2 * 13 / 3 / 3 / 5 / 5 / 11,
    -1.0 * 7 / 3 / 3 / 11 / 11,
     1.0 * 2 * 7 / 3 / 5 / 11 / 11,
     1.0 * 13 / 3 / 3 / 5 / 7,
     1.0 * 0,
     1.0 * 0,
    -1.0 * 1 / 3 / 3 / 11,
    -1.0 * 2 / 3 / 3 / 5 / 7,
    -1.0 * 1 / 2 / 3 / 3 / 5 / 5,
     1.0 * 2 * 2 / 3 / 3 / 5 / 5 / 7,
     1.0 * 1 / 3 / 3 / 11,
     1.0 * 2 / 3 / 3 / 7};

const std::array<double, maxNre> bbetaj = {
     1.0 * 2 / 3 / 3 / 5 / 7,
    -1.0 * 2 * 2 / 3 / 3 / 5 / 11 / 11,
    -1.0 * 2 * 2 * 2 * 17 / 3 / 3 / 3 / 11 / 11 / 11 / 13,
     1.0 * 2 * 2 * 2 * 7 * 17 / 3 / 3 / 3 / 5 / 11 / 11 / 11 / 13,
     1.0 * 2 * 13 / 3 / 3 / 3 / 5 / 7 / 11,
     1.0 * 0,
     1.0 * 0,
     1.0 * 2 / 3 / 3 / 3 / 5 / 11 / 11,
    -1.0 * 2 * 2 * 2 / 3 / 3 / 3 / 5 / 7 / 11 / 13,
    -1.0 * 1 / 2 / 3 / 5 / 7 / 11 / 13,
     1.0 * 2 / 3 / 3 / 5 / 7 / 11 / 13,
     1.0 * 2 * 2 * 2 / 3 / 3 / 3 / 3 / 5 / 11 / 11,
    -1.0 * 2 / 3 / 5 / 7 / 11};

const std::array<double, maxNre> ggammaj = {
     1.0 * 0,
     1.0 * 2 * 2 * 2 * 2 * 17 / 3 / 3 / 3 / 3 / 5 / 7 / 11 / 11 / 13,
    -1.0 * 5 * 17 * 19 / 3 / 3 / 3 / 7 / 11 / 11 / 11 / 13 / 13,
     1.0 * 2 * 2 * 2 * 17 * 19 / 3 / 3 / 3 / 7 / 11 / 11 / 11 / 13 / 13,
     1.0 * 0,
     1.0 * 0,
     1.0 * 0,
    -1.0 * 1 / 3 / 3 / 3 / 3 / 7 / 11 / 11 / 13,
     1.0 * 2 * 2 / 3 / 3 / 3 / 7 / 11 / 11 / 13 / 13,
    -1.0 * 5 / 3 / 3 / 3 / 7 / 11 / 11 / 13 / 13,
     1.0 * 2 * 2 * 2 / 3 / 3 / 3 / 7 / 11 / 11 / 13 / 13,
    -1.0 * 5 / 3 / 3 / 3 / 3 / 7 / 11 / 11 / 13,
     1.0 * 2 * 2 / 3 / 3 / 3 / 7 / 11 / 13};
//       ---------------
//       for the values of <r2>,<r4>,<r6> look to:
//       Freeman and Desclaux, Journal of Magnetism
//       and Magnetic Materials 12 (1979) 11 ff
//       [<r2>] = a0**2       
//

const std::array<double, maxNre> rr2 = {1.309,  1.1963, 1.114,  1.0353, 0.9743,
                                        0.9175, 0.8671, 0.8220, 0.7814, 0.7446,
                                        0.7111, 0.6804, 0.6522};

//       ---------------
//       [<r4>] = a0**4
const std::array<double, maxNre> rr4 = {3.964, 3.3335, 2.910, 2.5390, 2.260,
                                        2.020, 1.820,  1.651, 1.505,  1.379,
                                        1.270, 1.174,  1.089};

//       ---------------
//       [<r6>] = a0**6
const std::array<double, maxNre> rr6 = {23.31, 18.353, 15.03, 12.546, 10.55,
                                        9.039, 7.831,  6.582, 6.048,  5.379,
                                        4.816, 4.340,  3.932};

//---------------------------
// set some natural constants
//---------------------------
const double pi = 4.0*atan(1.0);
const double kb = 1.38062;       // x 10**(-23) J/K,   Boltzmann constant k_B
const double hh = 6.626075540;   // x 10**(-34) J*sec, Planks constant h
const double hq = hh/2/pi;
const double ee = 1.6021773349;  // x 10**(-19) Coulomb, electric charge
const double cc = 2.99792458;    // x 10**(8) m/sec, speed of light
const double me = 9.109389754;   // x 10**(-31) kg, electron mass
const double na = 6.022045;      // x 10**(23) particles/mol, Avogardo constant  
//       within the above choose, the factor which connects
//       1meV and 1 Kelvin is fixed and given by:
//       10*ee/kb =: fmevkelvin = 11.6047...
//       this means 1 meV is nearly 11.6 K
const double c_fmevkelvin = 10*ee/kb;
//  magneton of Bohr in kelvin per tesla
const double c_myb = hq/me/2*c_fmevkelvin;

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
double jp2(double nj, double j) { 
  return jp(nj + 1, j) * jp(nj, j); 
}

//-----------------------
//  jm2(n) = <n-2|j-^2|n> 
//-----------------------
double jm2(double nj, double j) {
	return jm(nj-1,j)*jm(nj,j);
}

//-----------------------
//  jp3(n) = <n+3|j+^3|n> 
//-----------------------
double jp3(double nj, double j) {
	return jp(nj+2,j)*jp(nj+1,j)*jp(nj,j);
}

//-----------------------
//  jm3(n) = <n-3|j-^3|n> 
//-----------------------
double jm3(double nj, double j) {
	return jm(nj-2,j)*jm(nj-1,j)*jm(nj,j);
}

//-----------------------
//  jp4(n) = <n+4|j+^4|n> 
//-----------------------
double jp4(double nj, double j) {
	return jp(nj+3,j)*jp(nj+2,j)*jp(nj+1,j)*jp(nj,j);
}

//-----------------------
//  jm4(n) = <n-4|j-^4|n> 
//-----------------------
double jm4(double nj, double j) {
	return jm(nj-3,j)*jm(nj-2,j)*jm(nj-1,j)*jm(nj,j);
}

//-----------------------
//  jp5(n) = <n+5|j+^5|n> 
//-----------------------
double jp5(double nj, double j) {
	return jp(nj+4,j)*jp(nj+3,j)*jp(nj+2,j)*jp(nj+1,j)*jp(nj,j);
}

//-----------------------
//  jm5(n) = <n-5|j-^5|n> 
//-----------------------
double jm5(double nj, double j) {
	return jm(nj-4,j)*jm(nj-3,j)*jm(nj-2,j)*jm(nj-1,j)*jm(nj,j);
}

//-----------------------
//  jp6(n) = <n+6|j+^6|n> 
//-----------------------
double jp6(double nj, double j) {
	return jp(nj+5,j)*jp(nj+4,j)*jp(nj+3,j)*jp(nj+2,j)*jp(nj+1,j)*jp(nj,j);
}

//-----------------------
//  jm6(n) = <n-6|j-^6|n> 
//-----------------------
double jm6(double nj, double j) {
	return jm(nj-5,j)*jm(nj-4,j)*jm(nj-3,j)*jm(nj-2,j)*jm(nj-1,j)*jm(nj,j);
}

//------------------------------
//  f(1) = 1, f(-1) = 1/i = (-i)
//------------------------------
ComplexType f(double s) {
	return ComplexType((s+1)/2, (s-1)/2);
}

//c-----------------------------
double f20(double nj, double j) {
	return 3*pow(nj,2) - j*(j+1);
}

//c------------------------------
double f21(double nj, double j) {
	return nj;
}

//c------------------------------
double f22(double nj, double j) {
	return 1.0;
}

//c------------------------------
double f40(double nj, double j) {
	return 35*pow(nj,4) - 30*j*(j+1)*pow(nj,2) + 25*pow(nj,2) - 6*j*(j+1) 
           + 3*pow(j,2)*pow((j+1),2);
}

//c------------------------------
double f41(double nj, double j) {
	return 7*pow(nj,3) - 3*j*(j+1)*nj - nj;
}

//c------------------------------
double f42(double nj, double j) {
	return 7*pow(nj,2) - j*(j+1) - 5;
}

//c------------------------------
double f43(double nj, double j) {
	return nj;
}

//c------------------------------
double f44(double nj, double j) {
	return 1.0;
}

//c------------------------------
double f60(double nj, double j) {
	return 231*pow(nj,6) - 315*j*(j+1)*pow(nj,4) + 735*pow(nj,4)
           + 105*pow(j,2)*pow((j+1),2)*pow(nj,2) - 525*j*(j+1)*pow(nj,2)
           + 294*pow(nj,2) - 5*pow(j,3)*pow((j+1),3) + 40*pow(j,2)*pow((j+1),2)
           - 60*j*(j+1);
}

//c------------------------------
double f61(double nj, double j) {
	return 33*pow(nj,5) - ( 30*j*(j+1) - 15 )*pow(nj,3) 
           - 10*j*(j+1)*nj + 5*pow(j,2)*pow((j+1),2)*nj
           +  12*nj;
}

//c------------------------------
double f62(double nj, double j) {
	return 33*pow(nj,4) - 18*j*(j+1)*pow(nj,2) - 123*pow(nj,2)
           + pow(j,2)*pow(j+1,2) + 10*j*(j+1) + 102;
}

//c------------------------------     
double f63(double nj, double j) {
	return 11*pow(nj,3) - 3*j*(j+1)*nj - 59*nj;
}

//c------------------------------  
double f64(double nj, double j) {
	return 11*pow(nj,2) - j*(j+1) - 38;
}

//c------------------------------
double f65(double nj, double) {
	return nj;
}

//c------------------------------  
double f66(double, double) {
	return 1.0;
}


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

//c----------------------------------
//c testing if the real number r is 0
//c----------------------------------
void ifnull(double &r) {
	const double macheps = 1.0e-14;
	if (fabs(r) <= macheps) r=0.0;
}

//c-------------------------------------
//c testing if the complex number c is 0
//c-------------------------------------
ComplexType cifnull(const ComplexType& c) {
	auto i = ComplexType(0.0, 1.0);
	double re= (0.5*( c+conj(c) )).real();
	double im= (0.5*(-c+conj(c) )*i).real();
	ifnull(re);
	ifnull(im);
	return ComplexType(re, im);
}

//---------------------------------------------
// calculates the neutron scattering radius r0
//---------------------------------------------
double c_r0() {
	return -1.91*pow(ee,2)/me; // * 10**(-12) cm    
}


//--------------------------------------------------
// set the full stevens' operators Okq to their R3+ value
//--------------------------------------------------
void s_okq(double dimj, int k, int q, const DoubleFortranMatrix &sbkq,
           ComplexMatrix &okq) {
  okq.resize(sizeOfHam, sizeOfHam);
//  ComplexType i = {0.0, 1.0};
  // TODO: what is the range of dimj?
  int dim = static_cast<int>(dimj);
  double j = 0.5 * (dimj - 1.0);
  for (int m = 0; m < dim; ++m) {
    double mj = double(m + 1) - j - 1.0;
    for (int n = 0; n < dim; ++n) {
      double nj = double(n + 1) - j - 1.0;
      if (k == 0) {
        if (q == 0) { // q=0 chose j+ operator
          okq.set(m, n, delta(mj, nj + 1, j) * jp(nj, j));
        } else if (q == 1) { // q=1 chose jz operator
          okq.set(m, n, delta(mj, nj, j) * nj);
        } else if (q == 2) { // q=2 chose j- operator
          okq.set(m, n, delta(mj, nj - 1, j) * jm(nj, j));
        } else if (q == 4) { // q=4 chose o4 = o40 + 5*o44
          auto d0 = delta(mj, nj, j);
          ComplexType o40 = d0 * ff(4, 0, nj, j);
          auto d4 = delta(mj, nj + 4, j) * jp4(nj, j) +
                    delta(mj, nj - 4, j) * jm4(nj, j);
          ComplexType o44 = 0.25 * (ff(4, 4, mj, j) + ff(4, 4, nj, j)) * d4;
          okq.set(m, n, o40 + 5.0 * o44);
        } else if (q == 6) { // q=6 chose o6 = o60 -21*o64
          auto d0 = delta(mj, nj, j);
          auto o60 = d0 * ff(6, 0, nj, j);
          auto d4 = delta(mj, nj + 4, j) * jp4(nj, j) +
                    delta(mj, nj - 4, j) * jm4(nj, j);
          auto o64 = 0.25 * (ff(6, 4, mj, j) + ff(6, 4, nj, j)) * d4;
          okq.set(m, n, o60 - 21 * o64);
        }
      } else {
        if (q == 0) {
          auto d0 = delta(mj, nj, j);
          okq.set(m, n, d0 * ff(k, q, nj, j) * (1 + sbkq.get(k, q)) / 2);
        } else if (q == 1) {
          auto d1 = delta(mj, nj + 1, j) * jp(nj, j) +
                    sbkq.get(k, q) * delta(mj, nj - 1, j) * jm(nj, j);
          okq.set(m, n, 0.25 * (ff(k, q, mj, j) + ff(k, q, nj, j)) * d1 *
                            f(sbkq.get(k, q)));
        } else if (q == 2) {
          auto d2 = delta(mj, nj + 2, j) * jp2(nj, j) +
                    sbkq.get(k, q) * delta(mj, nj - 2, j) * jm2(nj, j);
          okq.set(m, n, 0.25 * (ff(k, q, mj, j) + ff(k, q, nj, j)) * d2 *
                            f(sbkq.get(k, q)));
        } else if (q == 3) {
          auto d3 = delta(mj, nj + 3, j) * jp3(nj, j) +
                    sbkq.get(k, q) * delta(mj, nj - 3, j) * jm3(nj, j);
          okq.set(m, n, 0.25 * (ff(k, q, mj, j) + ff(k, q, nj, j)) * d3 *
                            f(sbkq.get(k, q)));
        } else if (q == 4) {
          auto d4 = delta(mj, nj + 4, j) * jp4(nj, j) +
                    sbkq.get(k, q) * delta(mj, nj - 4, j) * jm4(nj, j);
          okq.set(m, n, 0.25 * (ff(k, q, mj, j) + ff(k, q, nj, j)) * d4 *
                            f(sbkq.get(k, q)));
        } else if (q == 5) {
          auto d5 = delta(mj, nj + 5, j) * jp5(nj, j) +
                    sbkq.get(k, q) * delta(mj, nj - 5, j) * jm5(nj, j);
          okq.set(m, n, 0.25 * (ff(k, q, mj, j) + ff(k, q, nj, j)) * d5 *
                            f(sbkq.get(k, q)));
        } else if (q == 6) {
          auto d6 = delta(mj, nj + 6, j) * jp6(nj, j) +
                    sbkq.get(k, q) * delta(mj, nj - 6, j) * jm6(nj, j);
          okq.set(m, n, 0.25 * (ff(k, q, mj, j) + ff(k, q, nj, j)) * d6 *
                            f(sbkq.get(k, q)));
        }
      }
    } // for n
  }   // for m
}

//-------------------------------------------------
// calculates the norm ||A|| of a given operator A
//-------------------------------------------------
// k=0 q=0 : j+
// k=0 q=1 : jz
// k=0 q=2 : j-
// k=0 q=4 : o4 = o40+ 5*o44
// k=0 q=6 : o6 = o60-21*o64
double c_operator_norm(double dimj, int k, int q, const DoubleFortranMatrix& sbkq, ComplexMatrix& okq) {
	s_okq(dimj, k, q, sbkq, okq);

	double res = 0.0;
	for(size_t m=0; m < static_cast<size_t>(dimj); ++m) {
    for(size_t n=0; n < static_cast<size_t>(dimj); ++n) {
	      //res += okq.get(m,n) * conj(okq.get(m,n));
	      res += std::norm(okq.get(m,n));
    }
  }
	res /= dimj;
	res = sqrt(res);
  return res;
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

//c---------------------------------------
  double omega(int k, int q) {
    const std::array<double,49> oma = {
                                   1.0,
                              1.0, 1.0, 1.0,
                          1.0,1.0, 1.0, 1.0,1.0,
                      1.0,1.0,1.0, 1.0, 1.0,1.0,1.0,
                  1.0,1.0,1.0,1.0, 1.0, 1.0,1.0,1.0,1.0,
              1.0,1.0,1.0,3.0,3.0, 1.0, 3.0,3.0,1.0,1.0,1.0,
          1.0,1.0,1.0,1.0,3.0,3.0, 1.0, 3.0,3.0,1.0,1.0,1.0,1.0 };

    return oma[k*(k+1) + q];
  }

  // c***********************************************************************
  // c                                                                      *
  // c                   Function  fac                                      *
  // c                                                                      *
  // c***********************************************************************
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

  // c--------------------------------------------------------
  // c binom (n over k)
  // c--------------------------------------------------------
  double binom(int n, int k) {
    return fac(double(n)) / fac(double(k)) / fac(double(n - k));
  }

  // c--------------------------------------------------------
  // c              (k)
  // c calculates  D    (a,b,c)
  // c              ms m
  // c
  // c see Lindner A, 'Drehimpulse in der Quantenmechanik',
  // c ISBN 3-519-03061-6                  (j)
  // c Stuttgart: Teubner, 1984  page 86 (d    (beta)=...)
  // c for equation (1)                    m ms
  // c
  // c see Buckmaster phys. stat. sol. (a) 13 (1972) 9
  // c for equation (2)
  // c
  // c--------------------------------------------------------
  ComplexType ddrot(int j, int m, int ms, double a, double b, double c) {
    // c       equation (1)
    double d = delta(double(ms), double(m), double(j));
    ifnull(b);
    if (b != 0.0) {
      d = 0.0;
      for (int n = std::max(0, -(m + ms)); n <= std::min(j - m, j - ms);
           ++n) { // do n=max(0,-(m+ms)),min(j-m,j-ms)
        d = d +
            pow(-1.0, (j - ms - n)) * binom(j - ms, n) *
                binom(j + ms, j - m - n) * pow(cos(0.5 * b), (2 * n + m + ms)) *
                pow(sin(0.5 * b), (2 * j - 2 * n - m - ms));
      }
      d = d * sqrt(fac(double(j + m)) / fac(double(j + ms)) *
                   fac(double(j - m)) / fac(double(j - ms)));
    }
    // c       equation (2)
    return ComplexType(cos(m * a), -sin(m * a)) * d *
           ComplexType(cos(ms * c), -sin(ms * c));
  }
//c--------------------------------------------------
//c <jm| jp^|q| or jm^|q|  |nj>
//c--------------------------------------------------
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
    throw std::runtime_error("Cannot calculate jop with this q value.");
  }

  // c--------------------------------------------------
  // c calculate the full stevens operators
  // c--------------------------------------------------
  double full_okq(int k, int q, double mj, double nj, double j) {
    return 0.5*jop(q,mj,nj,j)*( ff(k,q,mj,j)+ff(k,q,nj,j) );
  }

  // c-------------------------------
  // c  calculates <i|j+|k>
  // c-------------------------------
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
  // c--------------------
  // c calculates <i|j-|k>
  // c--------------------
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
  // c--------------------
  // c calculates <i|jx|k>
  // c--------------------
  ComplexType matjx(const ComplexFortranMatrix &ev, int i, int k, int dim) {
    return 0.5 * (matjm(ev, i, k, dim) + matjp(ev, i, k, dim));
  }
  // c-------------------------
  // c calculates |<i|jx|k>|**2
  // c-------------------------
  double matjx2(const ComplexFortranMatrix &ev, int i, int k, int dim) {
    return std::norm(matjx(ev, i, k, dim));
  }
  // c--------------------
  // c calculates <i|jy|k>
  // c--------------------
  ComplexType matjy(const ComplexFortranMatrix &ev, int i, int k, int dim) {
    auto ci = ComplexType(0.0, 1.0);
    return 0.5 * ci * (matjm(ev, i, k, dim) - matjp(ev, i, k, dim));
  }
  // c-------------------------
  // c calculates |<i|jy|k>|**2
  // c-------------------------
  double matjy2(const ComplexFortranMatrix &ev, int i, int k, int dim) {
    return std::norm(matjy(ev, i, k, dim));
  }
  // c--------------------
  // c calculates <i|jz|k>
  // c--------------------
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
  // c-------------------------
  // c calculates |<i|jz|k>|**2
  // c-------------------------
  double matjz2(const ComplexFortranMatrix &ev, int i, int k, int dim) {
    return std::norm(matjz(ev, i, k, dim));
  }

  // c-------------------------
  // c calculates |<i|jT|k>|**2
  // c-------------------------
  double matjt2(const ComplexFortranMatrix &ev, int i, int k, int dim) {
    return 2.0 / 3 * (matjx2(ev, i, k, dim) + matjy2(ev, i, k, dim) +
                      matjz2(ev, i, k, dim));
  }

  // c---------------------------------------------------------------
  // c calculates all transition matrix elements for a single crystal
  // c and a polycrystalline sample (powder)
  // c---------------------------------------------------------------
  void matcalc(const ComplexFortranMatrix &ev, int dim,
               DoubleFortranMatrix &jx2, DoubleFortranMatrix &jy2,
               DoubleFortranMatrix &jz2, DoubleFortranMatrix &jt2) {
    for (int i = 1; i <= dim; ++i) {   // do 10 i=1,dim
      for (int k = 1; k <= dim; ++k) { //   do 20 k=1,dim
        jx2(i, k) = matjx2(ev, i, k, dim);
        jy2(i, k) = matjy2(ev, i, k, dim);
        jz2(i, k) = matjz2(ev, i, k, dim);
        jt2(i, k) = 2.0 / 3 * (jx2(i, k) + jy2(i, k) + jz2(i, k));
      }
    }
  }

  // c------------------------------------------------------------
  // c make sure that no underflow and overflow appears within exp
  // c------------------------------------------------------------
  double exp_(double z) {
    //	implicit none
    //	real*8 z,exp,zmax
    const double zmax = 71.0;
    //	exp_ = exp(zmax)
    //	if(z.lt.0.0d0) exp_ = 0.0d0
    //	if(abs(z).lt.zmax) exp_ = exp(z)
    if (z < -zmax) {
      return 0.0;
    }
    if (z < zmax) {
      return exp(z);
    }
    return exp(zmax);
  }

  // c------------------------------------------
  // c calculates all transition intensities for
  // c a polycrystalline sample (powder)
  // c------------------------------------------
  void intcalc(double pi, double r0, double gj, double z,
               const DoubleFortranMatrix &jt2, const DoubleFortranVector &e,
               DoubleFortranMatrix &inten, int dim, double temp) {
    auto constant = 4.0 * pi * pow(0.5 * r0 * gj, 2);
    if (temp == 0.0) {
      temp = 1.0;
    }
    for (int i = 1; i <= dim; ++i) { // do 10 i=1,dim
      auto coeff = exp_(-e(i) / temp) / z * constant;
      for (int k = 1; k <= dim; ++k) { //	   do 20 k=1,dim
        inten(i, k) = coeff * jt2(i, k);
        std::cerr << "inten " << i << ' ' << k << ' ' << inten(i, k) << std::endl;
      }
    }
  }
  // c-------------------------------------
  // c calculation of the occupation factor
  // c-------------------------------------
  double c_occupation_factor(const DoubleFortranVector &energy, double dimj,
                             double temp) {
    int dim = static_cast<int>(dimj);
    double occupation_factor = 0.0;
    if (temp == 0.0) {
      temp = 1.0;
    }
    for (int s = 1; s <= dim; ++s) { // do 10 s=1,dim
      occupation_factor += exp_(-energy(s) / temp);
    }
    return occupation_factor;
  }

} // anonymous

std::tuple<GSLVector, ComplexMatrix>
sc_crystal_field(int nre, const std::string &type, int symmetry,
                 const DoubleFortranMatrix &sbkq, DoubleFortranVector &bmol,
                 DoubleFortranVector &bext, ComplexFortranMatrix &bkq,
                 double temp) {
  if (nre <= 0 || nre > maxNre) {
    throw std::out_of_range("nre is out of range");
  }
  //// initialize some rare earth constants
  auto gj = ggj[nre-1];
  auto dimj = ddimj[nre-1];
  //auto alphaj = aalphaj[nre-1];
  //auto betaj = bbetaj[nre-1];
  //auto gammaj = ggammaj[nre-1];
  //auto r2 = rr2[nre-1];
  //auto r4 = rr4[nre-1];
  //auto r6 = rr6[nre-1];

  // magneton of Bohr in kelvin per tesla
  auto myb = c_myb;
  std::cerr << "myb=" << myb << std::endl;
  ////for (int k = 2; k <= 6; k += 2) { //	   do k=2,6,2
  ////  for (int m = 0; m <= k; ++m) {  //	      do m=0,k
  ////    std::cerr << "bkq= " << k << ' ' << m << ' ' << std::setprecision(18) << ComplexType(bkq(k, m)) << std::endl;
  ////  }
  ////}

  //if (type == "Vkq" || type == "vkq" || type == "VKQ") {
  //  // c          the Vkq are in kelvin and are the parameters which uses the
  //  // normalized
  //  // c          operators Okq/||Okq|| and j+/||j+|| and j-/||j-|| and
  //  // jz/||jz||
  //  // c          calculates the Bkq,B+,B- and Bz from the given Vkq,V+,V- and
  //  // Vz
  //  auto f_bmol = 2 * (gj - 1) * myb;
  //  auto f_bext = gj * myb;
  //  ComplexMatrix okq(sizeOfHam, sizeOfHam);
  //  okq.zero();
  //  auto norm =
  //      c_operator_norm(dimj, 0, 0, sbkq, okq);    // 00 = j+ note:||j+||=||j-||
  //  bmol(1) = 2 * bmol(1) / norm / f_bmol;         //
  //  bext(1) = 2 * bext(1) / norm / f_bext;         //  V- / ||j+|| = 1/2*f*B-
  //  bmol(2) = 2 * bmol(2) / norm / f_bmol;         //
  //  bext(2) = 2 * bext(2) / norm / f_bext;         //  V+ / ||j-|| = 1/2*f*B+
  //  norm = c_operator_norm(dimj, 0, 1, sbkq, okq); // 01 = jz
  //  bmol(3) = bmol(3) / norm / f_bmol;             //
  //  bext(3) = bext(3) / norm / f_bext;             // Vz / ||jz|| = f*Bz
  //  DoubleFortranMatrix ssbkq(0, 6, 0, 6);
  //  ssbkq.zero();

  //  for (int k = 2; k <= 6; k += 2) { //	   do k=2,6,2
  //    for (int m = 0; m <= k; ++m) {  //	      do m=0,k
  //      bkq(k, m) = cifnull(bkq.get(k, m));
  //      if (bkq(k, m) != 0.0) {
  //        ssbkq(k, m) = 1.0; // note ||re(Okq)|| = ||im(Okq)|| for q<>0
  //        if (symmetry == 8) {
  //          norm = 1.0;
  //          if (k == 4)
  //            norm = c_operator_norm(dimj, 4, 0, ssbkq, okq);
  //          if (k == 6)
  //            norm = c_operator_norm(dimj, 6, 0, ssbkq, okq);
  //        } else {
  //          norm = c_operator_norm(dimj, k, m, ssbkq, okq);
  //        }
  //        bkq(k, m) = bkq(k, m) / norm;
  //      }
  //    }
  //  }
  //}

//       ------------------
//       gives nearly 11.604... K
//	pi = 4.0d0*atan(1.0d0)
//	fmevkelvin = c_fmevkelvin()                        
//       ------------------
//       magnetic neutron scattering radius
  auto r0 = c_r0();
  std::cerr << "r0=" << r0 << std::endl;
//       ------------------------------------------------------------
//       transform the Bkq with
//       H = sum_k=0 Bk0 Ok0 + sum_k>0_q>0  ReBkq ReOkq + ImBkq ImOkq
//       to a representation with
//                     *
//       H = sum_kq dkq  Okq
//
//       one finds:   dk0 = Bk0  and for q<>0: dkq = Bkq/2
//       ------------------------------------------------------------
  ComplexFortranMatrix dkq_star(1, 6, -6, 6); // complex*16   dkq_star(6,-6:6)
  dkq_star.zero();
  auto i = ComplexType(0.0, 1.0);
  for (int k = 2; k <= 6; k += 2) { // do k=2,6,2
    for (int q = 0; q <= k; ++q) {  //  do q=0,k
      ComplexType b_kq = bkq(k, q);
      if (sbkq.get(k, q) == -1) {
        b_kq = i * b_kq;
      }
      dkq_star(k,q) = b_kq;
      if (q != 0) {
        dkq_star(k, q) = dkq_star(k, q) / 2.0;
      }
      dkq_star(k, -q) = conjg(dkq_star(k, q));
      //std::cerr << "dkq*= " << k << ' ' << q << ' ' << std::setprecision(18) << ComplexType(dkq_star(k, q)) << std::endl;
    }
  }
   //c       -------------------------------------------------------------------
   //c       the parameters are   conjg(D_kq)
   //c       -------------------------------------------------------------------
  for (int k = 2; k <= 6; k += 2) { // do k=2,6,2
    for (int q = -k; q <= k; ++q) { // do q=-k,k
      dkq_star(k, q) = conjg(dkq_star(k, q));
    }
  }
  // c       -------------------------------------------------------------------
  auto a = alpha_euler;
  auto b = beta_euler;
  auto c = gamma_euler;
  ComplexFortranMatrix  rdkq_star(1, 6, -6, 6);
  for (int k = 2; k <= 6; k += 2) { // do k=2,6,2
    for (int q = -k; q <= k; ++q) { // do q=-k,k
      rdkq_star(k, q) = ComplexType(0.0, 0.0);
      for (int qs = -k; qs <= k; ++qs) { // do qs=-k,k
        rdkq_star(k, q) = rdkq_star(k, q) +
                          dkq_star(k, qs) * epsilon(k, qs) / epsilon(k, q) *
                              omega(k, qs) / omega(k, q) *
                              ddrot(k, q, qs, a, b, c);
      }
    }
  }
  // c       -------------------------------------------------------------------
  // c       rotate the external magnetic field
  // c       -------------------------------------------------------------------
  // c       aj  = sqrt(3/j/(j+1)/(2*j+1))
  // c
  // c       the following hamiltonian will be rotated
  // c
  // c        cry        -----    cry    cry
  // c       h     = f * >     Bex     V       with  <j|| V1 ||j> = sqrt(3)
  // c        mag        -----    1q    1q
  // c                     q
  // c
  
  ComplexFortranMatrix  bex(1,1, -1,1);
  bex(1, 1) = - (bext(1)-i*bext(2))/sqrt(2.0); // ! /aj
  bex(1,-1) =   (bext(1)+i*bext(2))/sqrt(2.0); // ! /aj
  bex(1, 0) =    bext(3);                      // ! /aj
  
  // c       calculates Bex(1,q) for a canted moment
  // c
  // c       moment    cry       cry   cry     +
  // c      h       = R   (abc) h     R   (abc)
  // c       mag                 mag
  // c
  ComplexFortranMatrix  rbex(1,1, -1,1);
  for(int q = -1; q <= 1; ++q) { //        do q=-1,1
    rbex(1,q) = ComplexType(0.0, 0.0);
    for(int qs = -1; qs <= 1; ++qs) { //         do qs=-1,1
      rbex(1,q) = rbex(1,q) + bex(1,qs)*ddrot(1,q,qs,a,b,c);
    }
  }
  
  ComplexType rbextp, rbextm, rbextz;
  rbextp =  rbex(1,-1)*sqrt(2.0);    //  !*aj
  rbextm =  rbex(1, 1)*(-sqrt(2.0)); //  !*aj
  rbextz =  rbex(1, 0);              //  !*aj
  // c       -------------------------------------------------------------------
  // c       magneton of Bohr in kelvin per tesla
  //	myb    = c_myb()
  auto facmol = 2*(gj-1)*myb;
  auto facext = gj*myb;
  //	i = cmplx(0.0d0,1.0d0)
  auto bmolp = bmol(1)+i*bmol(2);
  auto bmolm = bmol(1)-i*bmol(2);
  auto bmolz = bmol(3);
  auto bextp = rbextp;
  auto bextm = rbextm;
  auto bextz = rbextz;
  // c       -----------------------------------------
  // c       set the hamiltonian matrix h to complex 0
  // c       -----------------------------------------
  //	call mx_clear(h)
  int dim = static_cast<int>(dimj);
  std::cerr << "dim=" << dim <<std::endl;
  ComplexFortranMatrix h(1, dim, 1, dim);
  std::cerr << h.size1() << " x " << h.size2() << std::endl;
  h.zero();
  // c       -------------------------------------------------------------------
  // c       calculate the crystal field hamiltonian
  // c       define only the lower triangle of h(m,n)
  // c       -------------------------------------------------------------------
  auto j   = 0.5*(dimj-1.0); // ! total momentum J of R3+
  for(int m=1; m <= dim; ++m) { //  do m=1,dim
    auto mj = double(m) - j - 1.0;
  	for(int n=1; n <=m; ++n) { //   do n=1,m
  	  auto nj = double(n) - j - 1.0;
      // h is already zero
      // h(m,n) = cmplx(0.0d0,0.0d0)
      for(int k=2; k <=6; k +=2) { //            do k=2,6,2
        for(int q=-k; q <= k; ++q) { //             do q=-k,k
          h(m,n) = h(m,n) + rdkq_star(k,q)*full_okq(k,q,mj,nj,j);
        }
      }
    }
  }
  // c       -------------------------------------------------------------------
  // c       define only the lower triangle of h(m,n)
  // c       -------------------------------------------------------------------
  //	dim = dimj
  //	j   = 0.5d0*(dimj-1) ! total momentum J of R3+
  for(int m=1; m <= dim; ++m) { //	do 10 m=1,dim
    auto mj = double(m) - j - 1.0;
    for(int n=1; n <= m; ++n) { //	   do 20 n=1,m
     auto nj = double(n) - j - 1.0;
  // c
  // c add the molecular field
  // c f*J*B = f*( 1/2*(J+ * B-  +  J- * B+) + Jz*Bz )
  // c
     h(m,n) = h(m,n) + 0.5*facmol*bmolm*delta(mj,nj+1,j)*jp(nj,j)
                     + 0.5*facmol*bmolp*delta(mj,nj-1,j)*jm(nj,j)
                     +       facmol*bmolz*delta(mj,nj,j)*nj
  // c
  // c add an external magnetic field
  // c
                     + 0.5*facext*bextm*delta(mj,nj+1,j)*jp(nj,j)
                     + 0.5*facext*bextp*delta(mj,nj-1,j)*jm(nj,j)
                     +       facext*bextz*delta(mj,nj,j)*nj;
     h(n, m) = conjg(h(m, n));
     //std::cerr << m << ' ' << n << ' ' << std::setprecision(18) << ComplexType(h(m,n)) << std::endl;
    }
  }

  auto h1 = h;
  // c
  // c diagonalisation of the hamiltonian h
  // c
  DoubleFortranVector energy(1, dim);
  ComplexFortranMatrix wavefunction(1, dim, 1, dim);
  h.eigenSystemHermitian(energy, wavefunction);
  auto sortedIndices = energy.sortIndices();
  energy.sort(sortedIndices);
  wavefunction.sortColumns(sortedIndices);
  // c
  // c shift the lowest energy level to 0
  // c
  auto indexMin = static_cast<int>(energy.indexOfMinElement() + 1);
  auto eshift = energy(indexMin);
  for(int n = 1; n <= dim; ++n) {//	do 50 n=1,dim
    std::cerr << "En " << n << ' ' << std::setprecision(18) << energy[n] << std::endl;
    energy(n) = energy(n) - eshift;
  }

  std::cerr << wavefunction.copyColumn(0) << std::endl;
  ComplexMatrix II = wavefunction.ctr() * h1 * wavefunction;
  for(size_t i = 0; i < II.size1(); ++i) {
    std::cerr << "II " << i << ' ' << ComplexType(II(i,i)) << std::endl;
  }

  // c
  // c write the energies out (in meV)
  // c
  // c	write(6,*) (energy(m)/11.6,m=1,dim)
  // c
  // c test if the wavefunction matrix wavefunction(dim,dim) is unitary
  // c
  // c       call t_orthonormalisation(6,wavefunction,dimj)
  // c
  // c calculates the transition matrixelements for a single crystal and
  // c a powdered sample
  // c
  DoubleFortranMatrix jx2mat(1, dim, 1, dim);
  DoubleFortranMatrix jy2mat(1, dim, 1, dim);
  DoubleFortranMatrix jz2mat(1, dim, 1, dim);
  DoubleFortranMatrix jt2mat(1, dim, 1, dim);
  matcalc(wavefunction,dim,jx2mat,jy2mat,jz2mat,jt2mat);
  // c
  // c calculates the sum over all occupation_factor
  // c
  auto occupation_factor = c_occupation_factor(energy, dimj, temp);
  // c
  // c calculates the transition intensities for a powdered sample
  // c
  //	call
  DoubleFortranMatrix intensity(1, dim, 1, dim);
  intcalc(pi, r0, gj, occupation_factor, jt2mat, energy, intensity, dim, temp);
  // c
  // c end of subroutine cfcalc
  // c
  return std::make_tuple(energy.moveToBaseVector(), wavefunction.moveToBaseMatrix());
}


} // namespace Functions
} // namespace CurveFitting
} // namespace Mantid
