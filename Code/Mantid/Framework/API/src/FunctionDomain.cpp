//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/FunctionDomain.h"
#include <iostream> 

namespace Mantid
{
namespace API
{

  /**
   * Constructs a domain base of size n.
   * @param n :: Size of the domain, i.e. number of values a function should calculate.
   */
  FunctionDomain::FunctionDomain(size_t n)
  {
    if (n == 0)
    {
      throw std::invalid_argument("FunctionDomain cannot have zero size.");
    }
    m_calculated.resize(n);
  }

  /**
   * Set a fitting data value.
   * @param i :: Index
   * @param value :: Value
   */
  void FunctionDomain::setFitData(size_t i,double value)
  {
    if (m_data.size() != m_calculated.size())
    {
      setDataSize();
    }
    m_data[i] = value;
  }

  /**
   * Set fitting data values.
   * @param values :: Values for fitting
   */
  void FunctionDomain::setFitData(const std::vector<double>& values)
  {
    if (values.size() != this->size())
    {
      throw std::invalid_argument("Setting data of a wrong size");
    }
    m_data.assign(values.begin(),values.end());
  }

  /**
   * Get a fitting data value
   * @param i :: Index
   */
  double FunctionDomain::getFitData(size_t i) const
  {
    if (m_data.size() != m_calculated.size())
    {
      throw std::runtime_error("Fitting data was not set");
    }
    return m_data[i];
  }

  /**
   * Set a fitting weight
   * @param i :: Index
   * @param value :: Value
   */
  void FunctionDomain::setFitWeight(size_t i,double value)
  {
    if (m_data.size() != m_calculated.size())
    {
      setDataSize();
    }
    m_weights[i] = value;
  }

  /**
   * Set fitting data values.
   * @param values :: Values for fitting
   */
  void FunctionDomain::setFitWeights(const std::vector<double>& values)
  {
    if (values.size() != this->size())
    {
      throw std::invalid_argument("Setting data of a wrong size");
    }
    m_weights.assign(values.begin(),values.end());
  }

  /**
   * Get a fitting weight.
   * @param i :: Index
   */
  double FunctionDomain::getFitWeight(size_t i) const
  {
    if (m_data.size() != m_calculated.size())
    {
      throw std::runtime_error("Fitting data was not set");
    }
    return m_weights[i];
  }

  /**
   * Resize the fitting data and weight buffers to match the size of the calculated buffer.
   */
  void FunctionDomain::setDataSize()
  {
    m_data.resize(m_calculated.size());
    m_weights.resize(m_calculated.size());
  }

} // namespace API
} // namespace Mantid
