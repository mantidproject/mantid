#include "MantidCurveFitting/Polynomial.h"
#include "MantidAPI/FunctionFactory.h"
#include <boost/lexical_cast.hpp>

using namespace Mantid::Kernel;
using namespace Mantid::API;

using namespace std;

namespace Mantid {
namespace CurveFitting {

DECLARE_FUNCTION(Polynomial)

//----------------------------------------------------------------------------------------------
/** Constructor
 */
Polynomial::Polynomial() : m_n(0) {}

//----------------------------------------------------------------------------------------------
/** Destructor
 */
Polynomial::~Polynomial() {}

//----------------------------------------------------------------------------------------------
/** Function to calcualte polynomial
 */
void Polynomial::function1D(double *out, const double *xValues,
                            const size_t nData) const {
  // 1. Use a vector for all coefficient
  vector<double> coeff(m_n + 1, 0.0);
  for (int i = 0; i < m_n + 1; ++i)
    coeff[i] = getParameter(i);

  // 2. Calculate
  for (size_t i = 0; i < nData; ++i) {
    double x = xValues[i];
    double temp = coeff[0];
    double nx = x;
    for (int j = 1; j <= m_n; ++j) {
      temp += coeff[j] * nx;
      nx *= x;
    }
    out[i] = temp;
  }

  return;
}

//----------------------------------------------------------------------------------------------
/** Function to calcualte polynomial based on vector

void Polynomial::functionLocal(std::vector<double> &out, std::vector<double>
xValues) const
{
  size_t nData = xValues.size();
  if (out.size() > xValues.size())
  {
    std::stringstream errss;
    errss << "Polynomial::functionLocal: input vector out has a larger size ("
          << out.size() << ") than xValues's (" << nData << ").";
    throw std::runtime_error(errss.str());
  }

  for (size_t i = 0; i < nData; ++i)
  {
    double x = xValues[i];
    double temp = getParameter(0);
    double nx = x;
    for (int j = 1; j <= m_n; ++j)
    {
      temp += getParameter(j)*nx;
      nx *= x;
    }
    out[i] = temp;
  }

  return;
}
*/

//----------------------------------------------------------------------------------------------
/** Function to calculate derivative analytically
 */
void Polynomial::functionDeriv1D(API::Jacobian *out, const double *xValues,
                                 const size_t nData) {
  for (size_t i = 0; i < nData; i++) {
    double x = xValues[i];
    double nx = 1;
    for (int j = 0; j <= m_n; ++j) {
      out->set(i, j, nx);
      nx *= x;
    }
  }

  return;
}

//----------------------------------------------------------------------------------------------
/** Get Attribute names
 * @return A list of attribute names (identical to Polynomial)
*/
std::vector<std::string> Polynomial::getAttributeNames() const {
  std::vector<std::string> res;
  res.push_back("n");
  return res;
}

//----------------------------------------------------------------------------------------------
/** Get Attribute
 * @param attName :: Attribute name. If it is not "n" exception is thrown.
 * @return a value of attribute attName
 * (identical to Polynomial)
 */
API::IFunction::Attribute
Polynomial::getAttribute(const std::string &attName) const {
  if (attName == "n") {
    return Attribute(m_n);
  }

  throw std::invalid_argument("Polynomial: Unknown attribute " + attName);
}

//----------------------------------------------------------------------------------------------
/** Set Attribute
 * @param attName :: The attribute name. If it is not "n" exception is thrown.
 * @param att :: An int attribute containing the new value. The value cannot be
 * negative.
 * (identical to Polynomial)
 */
void Polynomial::setAttribute(const std::string &attName,
                              const API::IFunction::Attribute &att) {
  if (attName == "n") {
    // set the polynomial order
    if (m_n >= 0) {
      clearAllParameters();
    }
    m_n = att.asInt();
    if (m_n < 0) {
      throw std::invalid_argument(
          "Polynomial: polynomial order cannot be negative.");
    }
    for (int i = 0; i <= m_n; ++i) {
      std::string parName = "A" + boost::lexical_cast<std::string>(i);
      declareParameter(parName);
    }
  }
}

//----------------------------------------------------------------------------------------------
/** Check if attribute attName exists
  */
bool Polynomial::hasAttribute(const std::string &attName) const {
  return attName == "n";
}

} // namespace CurveFitting
} // namespace Mantid
