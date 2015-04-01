//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include <cmath>
#include <limits>

#include "MantidCurveFitting/DiffSphere.h"
#include "MantidCurveFitting/BoundaryConstraint.h"
#include "MantidAPI/FunctionFactory.h"
#include <boost/math/special_functions/bessel.hpp>
#include "MantidAPI/ParameterTie.h"

namespace Mantid {
namespace CurveFitting {

using namespace Kernel;
using namespace API;

DECLARE_FUNCTION(ElasticDiffSphere)
DECLARE_FUNCTION(InelasticDiffSphere)
DECLARE_FUNCTION(DiffSphere)

ElasticDiffSphere::ElasticDiffSphere() {
  // declareParameter("Height", 1.0); //parameter "Height" already declared in
  // constructor of base class DeltaFunction
  declareParameter("Radius", 2.0, "Sphere radius");
  declareAttribute("Q", API::IFunction::Attribute(1.0));
}

void ElasticDiffSphere::init() {
  // Ensure positive values for Height and Radius
  BoundaryConstraint *HeightConstraint = new BoundaryConstraint(
      this, "Height", std::numeric_limits<double>::epsilon(), true);
  addConstraint(HeightConstraint);

  BoundaryConstraint *RadiusConstraint = new BoundaryConstraint(
      this, "Radius", std::numeric_limits<double>::epsilon(), true);
  addConstraint(RadiusConstraint);
}

double ElasticDiffSphere::HeightPrefactor() const {
  const double R = getParameter("Radius");
  const double Q = getAttribute("Q").asDouble();

  // Penalize negative parameters
  if (R < std::numeric_limits<double>::epsilon()) {
    return std::numeric_limits<double>::infinity();
  }

  return pow(3 * boost::math::sph_bessel(1, Q * R) / (Q * R), 2);
}

// initialize class attribute m_xnl with a list of coefficients in string format
void InelasticDiffSphere::initXnlCoeff() {
  /* List of 98 coefficients sorted by increasing value (F.Volino,Mol. Phys.
   * 41,271-279,1980)
   * For each coefficient, the triad (coeff, l, n) is defined
   */
  size_t ncoeff = 98;

  double xvalues[] = {
      2.081576,  3.342094,  4.493409,  4.514100,  5.646704,  5.940370,
      6.756456,  7.289932,  7.725252,  7.851078,  8.583755,  8.934839,
      9.205840,  9.840446,  10.010371, 10.613855, 10.904122, 11.070207,
      11.079418, 11.972730, 12.143204, 12.279334, 12.404445, 13.202620,
      13.295564, 13.472030, 13.846112, 14.066194, 14.258341, 14.590552,
      14.651263, 15.244514, 15.310887, 15.579236, 15.819216, 15.863222,
      16.360674, 16.609346, 16.977550, 17.042902, 17.117506, 17.220755,
      17.408034, 17.947180, 18.127564, 18.356318, 18.453241, 18.468148,
      18.742646, 19.262710, 19.270294, 19.496524, 19.581889, 19.862424,
      20.221857, 20.371303, 20.406581, 20.538074, 20.559428, 20.795967,
      21.231068, 21.537120, 21.578053, 21.666607, 21.840012, 21.899697,
      21.999955, 22.578058, 22.616601, 22.662493, 23.082796, 23.106568,
      23.194996, 23.390490, 23.519453, 23.653839, 23.783192, 23.906450,
      24.360789, 24.382038, 24.474825, 24.689873, 24.850085, 24.899636,
      25.052825, 25.218652, 25.561873, 25.604057, 25.724794, 25.846084,
      26.012188, 26.283265, 26.516603, 26.552589, 26.666054, 26.735177,
      26.758685, 26.837518};

  size_t lvalues[] = {1,  2,  0,  3,  4,  1,  5,  2, 0,  6,  3,  7,  1,  4,
                      8,  2,  0,  5,  9,  3,  10, 6, 1,  11, 4,  7,  2,  0,
                      12, 5,  8,  3,  13, 1,  9,  6, 14, 4,  10, 2,  7,  0,
                      15, 5,  11, 8,  16, 3,  1,  6, 12, 17, 9,  4,  2,  0,
                      13, 18, 7,  10, 5,  14, 19, 3, 8,  1,  11, 6,  20, 15,
                      4,  9,  12, 2,  0,  21, 16, 7, 10, 13, 5,  22, 3,  17,
                      1,  8,  14, 11, 23, 6,  18, 4, 9,  2,  0,  15, 24, 12};

  size_t nvalues[] = {
      0, 0, 1, 0, 0, 1, 0, 1, 2, 0, 1, 0, 2, 1, 0, 2, 3, 1, 0, 2, 0, 1, 3, 0, 2,
      1, 3, 4, 0, 2, 1, 3, 0, 4, 1, 2, 0, 3, 1, 4, 2, 5, 0, 3, 1, 2, 0, 4, 5, 3,
      1, 0, 2, 4, 5, 6, 1, 0, 3, 2, 4, 1, 0, 5, 3, 6, 2, 4, 0, 1, 5, 3, 2, 6, 7,
      0, 1, 4, 3, 2, 5, 0, 6, 1, 7, 4, 2, 3, 0, 5, 1, 6, 4, 7, 8, 2, 0, 3};

  for (size_t i = 0; i < ncoeff; i++) {
    xnlc coeff;
    coeff.x = xvalues[i];
    coeff.l = lvalues[i];
    coeff.n = nvalues[i];
    m_xnl.push_back(coeff);
  }
}

/// Initialize a set of coefficients that will remain constant during fitting
void InelasticDiffSphere::initAlphaCoeff() {
  for (std::vector<xnlc>::const_iterator it = m_xnl.begin(); it != m_xnl.end();
       ++it) {
    double x = it->x; // eigenvalue for a (n, l) pair
    double l = (double)(it->l);
    m_alpha.push_back((2.0 * l + 1) * 6.0 * x * x / (x * x - l * (l + 1)));
  }
}

/* Factor "J" is defined as [Q*a*j(l+1,Q*a) - l*j(l,Q*a)] / [(Q*a)^2 - x^2]
 * Both numerator and denominator goes to zero when Q*a approaches x, giving
 * rise to
 * numerical indeterminacies. To avoid them, we will interpolate linearly.
 */
void InelasticDiffSphere::initLinJlist() {
  for (size_t i = 0; i < m_xnl.size(); i++) {
    linearJ abJ;
    double x = m_xnl[i].x; // eigenvalue for a (n, l) pair
    unsigned int l = (unsigned int)(m_xnl[i].l);
    double Qa = x - m_divZone; // left of the numerical divergence point
    double J0 = (Qa * boost::math::sph_bessel(l + 1, Qa) -
                 l * boost::math::sph_bessel(l, Qa)) /
                (Qa * Qa - x * x);
    Qa = x + m_divZone; // right of the numerical divergence point
    double J1 = (Qa * boost::math::sph_bessel(l + 1, Qa) -
                 l * boost::math::sph_bessel(l, Qa)) /
                (Qa * Qa - x * x);
    abJ.slope = (J1 - J0) / (2 * m_divZone); // slope of the linear
                                             // interpolation
    abJ.intercept =
        J0 -
        abJ.slope * (x - m_divZone); // intercept of the linear interpolation
    m_linearJlist.push_back(
        abJ); // store the parameters of the linear interpolation for this it->x
  }
}

InelasticDiffSphere::InelasticDiffSphere()
    : m_lmax(24), m_divZone(0.1), m_hbar(0.658211626) {
  declareParameter("Intensity", 1.0, "scaling factor");
  declareParameter("Radius", 2.0, "Sphere radius, in Angstroms");
  declareParameter("Diffusion", 0.05, "Diffusion coefficient, in units of "
                                      "A^2*THz, if energy in meV, or A^2*PHz "
                                      "if energy in ueV");
  declareParameter("Shift", 0.0, "Shift in domain");

  declareAttribute("Q", API::IFunction::Attribute(1.0));
}

/// Initialize a set of coefficients and terms that are invariant during fitting
void InelasticDiffSphere::init() {
  initXnlCoeff();   // initialize m_xnl with the list of coefficients xnlist
  initAlphaCoeff(); // initialize m_alpha, certain factors constant over the fit
  initLinJlist();   // initialize m_linearJlist, linear interpolation around
                    // numerical divergence
}

/// Calculate the (2l+1)*A_{n,l} coefficients for each Lorentzian
std::vector<double>
InelasticDiffSphere::LorentzianCoefficients(double a) const {
  // precompute the 2+m_lmax spherical bessel functions (26 in total)
  std::vector<double> jl(2 + m_lmax);
  for (size_t l = 0; l < 2 + m_lmax; l++) {
    jl[l] = boost::math::sph_bessel((unsigned int)(l), a);
  }

  // store the coefficient of each Lorentzian in vector YJ(a,w)
  size_t ncoeff = m_xnl.size();
  std::vector<double> YJ(ncoeff);

  for (size_t i = 0; i < ncoeff; i++) {
    double x = m_xnl[i].x;
    unsigned int l = (unsigned int)(m_xnl[i].l);

    double J;
    if (fabs(a - x) > m_divZone) {
      J = (a * jl[l + 1] - l * jl[l]) / (a * a - x * x);
    } else {
      J = m_linearJlist[i].slope * a +
          m_linearJlist[i].intercept; // linear interpolation instead
    }

    YJ[i] = m_alpha[i] * (J * J);
  }

  return YJ;
} // end of LorentzianCoefficients

void InelasticDiffSphere::function1D(double *out, const double *xValues,
                                     const size_t nData) const {
  const double I = getParameter("Intensity");
  const double R = getParameter("Radius");
  const double D = getParameter("Diffusion");
  const double Q = getAttribute("Q").asDouble();
  const double S = getParameter("Shift");

  // // Penalize negative parameters
  if (I < std::numeric_limits<double>::epsilon() ||
      R < std::numeric_limits<double>::epsilon() ||
      D < std::numeric_limits<double>::epsilon()) {
    for (size_t i = 0; i < nData; i++) {
      out[i] = std::numeric_limits<double>::infinity();
    }
    return;
  }

  // List of Lorentzian HWHM
  std::vector<double> HWHM;
  size_t ncoeff = m_xnl.size();
  for (size_t n = 0; n < ncoeff; n++) {
    double x = m_xnl[n].x; // eigenvalue
    HWHM.push_back(m_hbar * x * x * D / (R * R));
  }

  std::vector<double> YJ;
  YJ = LorentzianCoefficients(Q * R); // The (2l+1)*A_{n,l}
  for (size_t i = 0; i < nData; i++) {
    double energy = xValues[i] - S; // from meV to THz (or from micro-eV to PHz)
    out[i] = 0.0;
    for (size_t n = 0; n < ncoeff; n++) {
      double L = (1.0 / M_PI) * HWHM[n] /
                 (HWHM[n] * HWHM[n] + energy * energy); // Lorentzian
      out[i] += I * YJ[n] * L;
    }
  }
}

/* Propagate the attribute to its member functions.
 * NOTE: we pass this->getAttribute(name) by reference, thus the same
 * object is shared by the composite function and its members.
 */
void DiffSphere::trickleDownAttribute(const std::string &name) {
  for (size_t iFun = 0; iFun < nFunctions(); iFun++) {
    IFunction_sptr fun = getFunction(iFun);
    if (fun->hasAttribute(name)) {
      fun->setAttribute(name, this->getAttribute(name));
    }
  }
}

/* Same as parent except we overwrite attributes of member functions
 * having the same name
 */
void
DiffSphere::declareAttribute(const std::string &name,
                             const API::IFunction::Attribute &defaultValue) {
  API::ImmutableCompositeFunction::declareAttribute(name, defaultValue);
  trickleDownAttribute(name);
}

/* Same as parent except we overwrite attributes of member functions
 * having the same name
 */
void DiffSphere::setAttribute(const std::string &name, const Attribute &att) {
  API::ImmutableCompositeFunction::setAttribute(name, att);
  trickleDownAttribute(name);
}

void DiffSphere::init() {
  m_elastic = boost::dynamic_pointer_cast<ElasticDiffSphere>(
      API::FunctionFactory::Instance().createFunction("ElasticDiffSphere"));
  addFunction(m_elastic);
  m_inelastic = boost::dynamic_pointer_cast<InelasticDiffSphere>(
      API::FunctionFactory::Instance().createFunction("InelasticDiffSphere"));
  addFunction(m_inelastic);

  setAttributeValue("NumDeriv", true);
  declareAttribute("Q", API::IFunction::Attribute(1.0));

  // Set the aliases
  setAlias("f1.Intensity", "Intensity");
  setAlias("f1.Radius", "Radius");
  setAlias("f1.Diffusion", "Diffusion");
  setAlias("f1.Shift", "Shift");

  // Set the ties between Elastic and Inelastic parameters
  addDefaultTies("f0.Height=f1.Intensity,f0.Radius=f1.Radius");
  applyTies();
}

} // namespace CurveFitting
} // namespace Mantid
