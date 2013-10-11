#include "MantidCurveFitting/FullprofPolynomial.h"
#include "MantidAPI/FunctionFactory.h"
#include <boost/lexical_cast.hpp>

namespace Mantid
{
namespace CurveFitting
{

  DECLARE_FUNCTION(FullprofPolynomial)

  //----------------------------------------------------------------------------------------------
  /** Constructor
   */
  FullprofPolynomial::FullprofPolynomial():m_n(6), m_bkpos(1.)
  {
    // Declare first 6th order polynomial as default
    for(int i=0; i<=m_n; ++i)
    {
      std::string parName = "A" + boost::lexical_cast<std::string>(i);
      declareParameter(parName);
    }
  }

  //----------------------------------------------------------------------------------------------
  /** Destructor
   */
 FullprofPolynomial::~FullprofPolynomial()
  {
  }

  //----------------------------------------------------------------------------------------------
  /** Function to calcualteFullprofPolynomial
   */
  void FullprofPolynomial::function1D(double* out, const double* xValues, const size_t nData)const
  {
    // FIXME - Not correct
    // TODO - Make it correct
    /*
    x = tof*1.0/bkpos-1.
    pow_x = 1.
    for i in xrange(order):
        y_b += B[i] * pow_x

        pow_x = pow_x*x
      */
    throw std::runtime_error("Implement ASAP");

    // 1. Use a vector for all coefficient
    std::vector<double> coeff(m_n+1, 0.0);
    for (int i = 0; i < m_n+1; ++i)
      coeff[i] = getParameter(i);

    // 2. Calculate
    for (size_t i = 0; i < nData; ++i)
    {
      double x = xValues[i];
      double temp = coeff[0];
      double nx = x;
      for (int j = 1; j <= m_n; ++j)
      {
        temp += coeff[j]*nx;
        nx *= x;
      }
      out[i] = temp;
    }

    return;
  }

  //----------------------------------------------------------------------------------------------
  /** Function to calcualteFullprofPolynomial based on vector

  void FullprofPolynomial::functionLocal(std::vector<double> &out, std::vector<double> xValues) const
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
  void FullprofPolynomial::functionDeriv1D(API::Jacobian* out, const double* xValues, const size_t nData)
  {
    // FIXME - Not correct!
    // TODO - Re-do ASAP
    throw std::runtime_error("It is not correct!");
    for (size_t i = 0; i < nData; i++)
    {
      double x = xValues[i];
      double nx = 1;
      for (int j = 0; j <= m_n; ++j)
      {
        out->set(i, j, nx);
        nx *= x;
      }
    }

    return;
  }

  //----------------------------------------------------------------------------------------------
  /** Get Attribute names
   * @return A list of attribute names (identical toFullprofPolynomial)
  */
  std::vector<std::string> FullprofPolynomial::getAttributeNames()const
  {
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
  API::IFunction::Attribute FullprofPolynomial::getAttribute(const std::string& attName)const
  {
    if (attName == "n")
    {
      return Attribute(m_n);
    }
    else if (attName == "Bkpos")
      return Attribute(m_bkpos);

    throw std::invalid_argument("Polynomial: Unknown attribute " + attName);
  }

  //----------------------------------------------------------------------------------------------
  /** Set Attribute
   * @param attName :: The attribute name. If it is not "n" exception is thrown.
   * @param att :: An int attribute containing the new value. The value cannot be negative.
   * (identical toFullprofPolynomial)
   */
  void FullprofPolynomial::setAttribute(const std::string& attName,const API::IFunction::Attribute& att)
  {
    if (attName == "n")
    {
      // set theFullprofPolynomial order
      int attint = att.asInt();
      if (attint < 0)
      {
        throw std::invalid_argument("Polynomial:FullprofPolynomial order cannot be negative.");
      }
      else if (attint != 6 && attint != 12)
      {
        throw std::runtime_error("FullprofPolynomial's order must be either 6 or 12. ");
      }
      else if (attint != m_n)
      {
        // Only order is (either 6 or 12) and different from current order
        clearAllParameters();

        m_n = attint;
        for(int i=0; i<=m_n; ++i)
        {
          std::string parName = "A" + boost::lexical_cast<std::string>(i);
          declareParameter(parName);
        }
      }
    }
    else if (attName == "Bkpos")
    {
      // Background original position
      m_bkpos = att.asDouble();
    }

    return;
  }

  //----------------------------------------------------------------------------------------------
  /** Check if attribute attName exists
    */
  bool FullprofPolynomial::hasAttribute(const std::string& attName)const
  {
    bool has = false;
    if (attName == "n")
      has = true;
    else if (attName == "Bkpos")
      has = true;

    return has;
  }


} // namespace CurveFitting
} // namespace Mantid
