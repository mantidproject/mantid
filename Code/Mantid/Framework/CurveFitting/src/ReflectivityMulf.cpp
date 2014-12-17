#include "MantidCurveFitting/ReflectivityMulf.h"
#include "MantidAPI/FunctionFactory.h"
#include <boost/lexical_cast.hpp>

#define PI 3.14159265358979323846264338327950288419716939937510582

using namespace Mantid::Kernel;
using namespace Mantid::API;

using namespace std;

namespace Mantid {
namespace CurveFitting {

DECLARE_FUNCTION(ReflectivityMulf)

//----------------------------------------------------------------------------------------------
/** Constructor
 */
ReflectivityMulf::ReflectivityMulf() : m_nlayer(0), m_nlayer_old(0) {}

// Initialize Basic Parameters
void ReflectivityMulf::init() {
  m_nlayer_old = 0;
  declareAttribute("nlayer", Attribute(0));
  declareParameter("Theta", 2.3);
  declareParameter("ScaleFactor", 1.0);
  declareParameter("AirSLD", 0.0);
  declareParameter("BulkSLD", 6.35e-6);
  declareParameter("Roughness", 2.5);
  declareParameter("BackGround", 1.0e-6);
  declareParameter("Resolution", 5.0);
  return;
}

//----------------------------------------------------------------------------------------------
/** Function to calcualte reflectivity
 */
void ReflectivityMulf::function1D(double *out, const double *xValues,
                                  const size_t nData) const {
  // 1. Use a vector for all coefficient
  vector<double> coeff(m_nlayer * 3 + 7, 0.0);

  for (int i = 0; i < 7; ++i) {
    coeff[i] = getParameter(i);
  }
  for (int i = 0; i < m_nlayer; ++i) {
    // Layer SLD
    coeff[7 + i * 3] = getParameter(7 + i * 3);
    // Layer d
    coeff[8 + i * 3] = getParameter(8 + i * 3);
    // Layer Roughness
    coeff[9 + i * 3] = getParameter(9 + i * 3);
  }

  // 2. Calculate
  std::vector<double> dn(m_nlayer + 2);
  std::vector<double> rnbn(m_nlayer + 2);
  std::vector<double> zbb(m_nlayer + 2);
  int nit = 21;
  std::vector<double> cy(nData);

  std::complex<double> c0(0.0, 0.0);
  std::complex<double> ci(0.0, 1.0);
  std::complex<double> cr(1.0, 0.0);
  std::vector<std::complex<double>> rnfn(m_nlayer + 2);
  std::vector<std::complex<double>> pfn(m_nlayer + 2);
  std::vector<std::complex<double>> betan(m_nlayer + 2);
  std::complex<double> rnf(0.0, 0.0);
  std::complex<double> rnf1(0.0, 0.0);
  std::complex<double> ac1(0.0, 0.0);
  std::complex<double> ac2(0.0, 0.0);
  std::complex<double> ac3(0.0, 0.0);
  std::complex<double> ac4(0.0, 0.0);
  std::complex<double> a12t(0.0, 0.0);
  std::complex<double> a22t(0.0, 0.0);
  std::complex<double> btm(0.0, 0.0);
  std::complex<double> btm1(0.0, 0.0);
  std::complex<double> cbtm(0.0, 0.0);
  std::complex<double> cbtm1(0.0, 0.0);

  std::complex<double> a111(0.0, 0.0);
  std::complex<double> a112(0.0, 0.0);
  std::complex<double> a121(0.0, 0.0);
  std::complex<double> a122(0.0, 0.0);
  std::complex<double> a211(0.0, 0.0);
  std::complex<double> a212(0.0, 0.0);
  std::complex<double> a221(0.0, 0.0);
  std::complex<double> a222(0.0, 0.0);
  std::complex<double> a311(0.0, 0.0);
  std::complex<double> a312(0.0, 0.0);
  std::complex<double> a321(0.0, 0.0);
  std::complex<double> a322(0.0, 0.0);

  double theta0 = coeff[0] * PI / 180.0;
  double scalefac = coeff[1];
  rnbn[0] = coeff[2];
  rnbn[m_nlayer + 1] = coeff[3];
  zbb[0] = coeff[4] * coeff[4];
  zbb[m_nlayer + 1] = 0.0;
  double bgd = coeff[5];
  double pthet = coeff[6];

  dn[0] = 0.0;
  dn[m_nlayer + 1] = 0.0;

  if (m_nlayer > 0) {
    for (int i = 0; i < m_nlayer; ++i) {
      rnbn[i + 1] = coeff[7 + i * 3];
      dn[i + 1] = coeff[8 + i * 3];
      zbb[i + 1] = coeff[9 + i * 3];
    }
  }

  double tmax, tmin, x, st0, ct0, ans, gauss, f;
  int nit1 = nit + 1;
  std::vector<double> xnit(nit1);
  int ii, k;

  double dthet = theta0 * pthet / 100.0;
  dthet = dthet / 2.35;
  double dthetr = dthet * 2.51;
  tmax = theta0 + dthet * 3;
  tmin = theta0 - dthet * 3;
  double dt = (tmax - tmin) / nit;
  for (int i = 0; i < nit1; i++) {
    xnit[i] = tmin + dt * i;
  }

  // Could be parallelized at this point

  for (size_t j = 0; j < nData; ++j) {
    double lambda = 4 * PI * sin(theta0) / xValues[j];
    cy[j] = 0.0;
    double tl = lambda * lambda;
    double tlc = 8.0 * PI * PI / tl;
    double con = tl / (2.0 * PI);

    for (k = 0; k < m_nlayer + 2; k++) {
      rnfn[k] = (1.0 - con * rnbn[k]);
    }

    for (ii = 0; ii < nit1; ii++) {
      x = xnit[ii];
      ct0 = cos(x);
      st0 = sin(x);
      pfn[0] = rnfn[0] * st0;

      for (int i = 1; i < m_nlayer + 1; i++) {
        rnf = (rnfn[i] * rnfn[i]) * cr;
        rnf1 = (rnfn[0] * rnfn[0]) * cr;
        pfn[i] = sqrt(rnf - (rnf1 * ct0 * ct0));
      }

      rnf = (rnfn[m_nlayer + 1] * rnfn[m_nlayer + 1]) * cr;
      rnf1 = (rnfn[0] * rnfn[0]) * cr;
      pfn[m_nlayer + 1] = sqrt(rnf - (rnf1 * ct0 * ct0));

      for (int i = 1; i < m_nlayer + 1; i++) {
        betan[i] = 2.0 * PI * dn[i] * pfn[i] / lambda;
      }

      a111 = cr;
      if ((pfn[0] + pfn[1]) != c0) {
        a12t = (pfn[0] - pfn[1]) / (pfn[0] + pfn[1]);
      } else {
        a12t = c0;
      }
      a112 = a12t * exp(-tlc * zbb[0] * pfn[0] * pfn[1]);
      a121 = a112;
      a122 = cr;

      for (int i = 1; i < m_nlayer + 1; i++) {
        btm = betan[i] * ci;
        btm1 = -1.0 * betan[i] * ci;
        if (imag(btm1) == 0.0)
          btm = betan[i] * 2.0 * ci;
        cbtm = exp(btm);
        if (real(btm1) == 0.0)
          cbtm1 = exp(btm1);
        if (imag(btm1) == 0.0)
          cbtm1 = 1.0 * cr;
        a211 = cbtm;
        if ((pfn[i] + pfn[i + 1]) != c0)
          a22t = (pfn[i] - pfn[i + 1]) / (pfn[i] + pfn[i + 1]);
        if ((pfn[i] + pfn[i + 1]) == c0)
          a22t = c0;
        a22t = a22t * exp(-tlc * zbb[i] * pfn[i] * pfn[i + 1]);
        a212 = a22t * cbtm;
        a221 = a22t * cbtm1;
        a222 = cbtm1;

        a311 = a111 * a211 + a112 * a221;
        a312 = a111 * a212 + a112 * a222;
        a321 = a121 * a211 + a122 * a221;
        a322 = a121 * a212 + a122 * a222;

        a111 = a311;
        a112 = a312;
        a121 = a321;
        a122 = a322;
      }

      ac1 = a121;
      ac2 = conj(ac1);
      ac3 = a111;
      ac4 = conj(ac3);
      if (ac3 == c0) {
        ans = 1.0;
      } else {
        ans = real((ac1 * ac2) / (ac3 * ac4));
      }

      gauss = (1.0 / dthetr) *
              exp(-0.5 * ((theta0 - x) / dthet) * (theta0 - x) / dthet);
      f = ans * gauss;
      cy[j] = cy[j] + f * dt;
    }
    out[j] = (cy[j] + bgd) * scalefac;
    // g_log.information() << "cy[j]" << cy[j] << "\n";
  }
  // End of parallelized section

  return;
}

//----------------------------------------------------------------------------------------------
/** Set Attribute
 * @param attName :: The attribute name. If it is not "nlayer" exception is
 * thrown.
 * @param att :: An int attribute containing the new value. The value cannot be
 * negative.
 * (identical to ReflectivityMulf)
 */
void ReflectivityMulf::setAttribute(const std::string &attName,
                                    const API::IFunction::Attribute &att) {
  storeAttributeValue(attName, att);
  if (attName == "nlayer") {
    m_nlayer = att.asInt();
    if (m_nlayer < 0) {
      throw std::invalid_argument("ReflectivityMulf: reflectivity number of "
                                  "layers cannot be negative.");
    }

    // get the current parameter values using the old value of nlayer
    vector<double> coeff(m_nlayer * 3 + 7, 0.0);
    for (int i = 0; i < 7; ++i)
      coeff[i] = getParameter(i);

    if (m_nlayer > m_nlayer_old) {
      for (int i = 0; i < m_nlayer_old; ++i) {
        coeff[7 + i * 3] = getParameter(7 + i * 3);
        coeff[8 + i * 3] = getParameter(8 + i * 3);
        coeff[9 + i * 3] = getParameter(9 + i * 3);
      }
      // fill in the missing values if m_nlayer>m_nlayer_old
      for (int i = m_nlayer_old; i < m_nlayer; ++i) {
        coeff[7 + i * 3] = 0.0;
        coeff[8 + i * 3] = 0.0;
        coeff[9 + i * 3] = 0.0;
      }
    } else {
      for (int i = 0; i < m_nlayer; ++i) {
        coeff[7 + i * 3] = getParameter(7 + i * 3);
        coeff[8 + i * 3] = getParameter(8 + i * 3);
        coeff[9 + i * 3] = getParameter(9 + i * 3);
      }
    }
    // now update the value of nlayer_old
    m_nlayer_old = m_nlayer;

    // set the reflectivity layers
    if (m_nlayer >= 0) {
      clearAllParameters();
    }

    declareParameter("Theta", coeff[0]);
    declareParameter("ScaleFactor", coeff[1]);
    declareParameter("AirSLD", coeff[2]);
    declareParameter("BulkSLD", coeff[3]);
    declareParameter("Roughness", coeff[4]);
    declareParameter("BackGround", coeff[5]);
    declareParameter("Resolution", coeff[6]);

    for (int i = 0; i < m_nlayer; ++i) {
      std::string parName = "SLD_Layer" + boost::lexical_cast<std::string>(i);
      declareParameter(parName, coeff[7 + i * 3]);
      parName = "d_Layer" + boost::lexical_cast<std::string>(i);
      declareParameter(parName, coeff[8 + i * 3]);
      parName = "Rough_Layer" + boost::lexical_cast<std::string>(i);
      declareParameter(parName, coeff[9 + i * 3]);
    }
  }
}

} // namespace CurveFitting
} // namespace Mantid
