#include "MantidCurveFitting/FullprofPolynomial.h"
#include "MantidAPI/FunctionFactory.h"
#include <boost/lexical_cast.hpp>

namespace Mantid {
namespace CurveFitting {

DECLARE_FUNCTION(FullprofPolynomial)

//----------------------------------------------------------------------------------------------
/** Constructor
 */
FullprofPolynomial::FullprofPolynomial() : m_n(6), m_bkpos(1.) {
  // Declare first 6th order polynomial as default
  for (int i = 0; i < m_n; ++i) {
    std::string parName = "A" + boost::lexical_cast<std::string>(i);
    declareParameter(parName);
  }
}

//----------------------------------------------------------------------------------------------
/** Destructor
 */
FullprofPolynomial::~FullprofPolynomial() {}

//----------------------------------------------------------------------------------------------
/** Function to calcualteFullprofPolynomial
 */
void FullprofPolynomial::function1D(double *out, const double *xValues,
                                    const size_t nData) const {
  // Generate a vector for all coefficient
  std::vector<double> B(m_n, 0.0);
  for (int i = 0; i < m_n; ++i)
    B[i] = getParameter(i);

  // Calculate
  for (size_t i = 0; i < nData; ++i) {
    double tof = xValues[i];
    double x = tof / m_bkpos - 1.;
    double pow_x = 1.;
#if 1
    // It is the first try as benchmark
    double y_b = 0.;
    for (int j = 0; j < m_n; ++j) {
      y_b += B[j] * pow_x;
      pow_x *= x;
    }
#else
    // This is the efficient coding
    double y_b = B[0];
    for (size_t j = 1; j < m_n; ++j) {
      pow_x *= x;
      y_b += B[j] * pow_x;
    }
#endif

    out[i] = y_b;
  }

  return;
}

//----------------------------------------------------------------------------------------------
/** Function to calculate derivative analytically
 */
void FullprofPolynomial::functionDeriv1D(API::Jacobian *out,
                                         const double *xValues,
                                         const size_t nData) {
  for (size_t i = 0; i < nData; i++) {
    double tof = xValues[i];
    double x = tof / m_bkpos - 1.;

    // Order 0
    double pow_x = 1.0;
    out->set(i, 0, pow_x);
    // Rest
    for (int j = 1; j < m_n; ++j) {
      // It does dirivative to B_j
      pow_x *= x;
      out->set(i, j, pow_x);
    }
  }

  return;
}

//----------------------------------------------------------------------------------------------
/** Get Attribute names
 * @return A list of attribute names (identical toFullprofPolynomial)
*/
std::vector<std::string> FullprofPolynomial::getAttributeNames() const {
  std::vector<std::string> res;
  res.push_back("n");
  res.push_back("Bkpos");

  return res;
}

//----------------------------------------------------------------------------------------------
/** Get Attribute
 * @param attName :: Attribute name. If it is not "n" exception is thrown.
 * @return a value of attribute attName
 * (identical toFullprofPolynomial)
 */
API::IFunction::Attribute
FullprofPolynomial::getAttribute(const std::string &attName) const {
  if (attName == "n") {
    return Attribute(m_n);
  } else if (attName == "Bkpos")
    return Attribute(m_bkpos);

  throw std::invalid_argument("Polynomial: Unknown attribute " + attName);
}

//----------------------------------------------------------------------------------------------
/** Set Attribute
 * @param attName :: The attribute name. If it is not "n" exception is thrown.
 * @param att :: An int attribute containing the new value. The value cannot be
 * negative.
 * (identical toFullprofPolynomial)
 */
void FullprofPolynomial::setAttribute(const std::string &attName,
                                      const API::IFunction::Attribute &att) {
  if (attName == "n") {
    // set theFullprofPolynomial order
    int attint = att.asInt();
    if (attint < 0) {
      throw std::invalid_argument(
          "Polynomial:FullprofPolynomial order cannot be negative.");
    } else if (attint != 6 && attint != 12) {
      throw std::runtime_error(
          "FullprofPolynomial's order must be either 6 or 12. ");
    } else if (attint != m_n) {
      // Only order is (either 6 or 12) and different from current order
      clearAllParameters();

      m_n = attint;
      for (int i = 0; i < m_n; ++i) {
        std::string parName = "A" + boost::lexical_cast<std::string>(i);
        declareParameter(parName);
      }
    }
  } else if (attName == "Bkpos") {
    // Background original position
    m_bkpos = att.asDouble();
  }

  return;
}

//----------------------------------------------------------------------------------------------
/** Check if attribute attName exists
  */
bool FullprofPolynomial::hasAttribute(const std::string &attName) const {
  bool has = false;
  if (attName == "n")
    has = true;
  else if (attName == "Bkpos")
    has = true;

  return has;
}

} // namespace CurveFitting
} // namespace Mantid
