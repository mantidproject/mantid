//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/FunctionValues.h"
#include <iostream> 
#include <algorithm>

namespace Mantid
{
namespace API
{

  /**
   * Constructs a domain base of size n.
   * @param n :: Size of the domain, i.e. number of values a function should calculate.
   */
  FunctionValues::FunctionValues(const FunctionDomain& domain)
  {
    reset(domain);
  }

  /** Copy constructor.
   *  @param values :: Values to copy from.
   */
  FunctionValues::FunctionValues(const FunctionValues& values):
  m_calculated(values.m_calculated),
  m_data(values.m_data),
  m_weights(values.m_weights)
  {
  }

  /// Reset the values to match a new domain.
  void FunctionValues::reset(const FunctionDomain& domain)
  {
    if (domain.size() == 0)
    {
      throw std::invalid_argument("FunctionValues cannot have zero size.");
    }
    m_calculated.resize(domain.size());
  }

  /**
   * Expand to a new size. Preserve old values. Do not contract.
   * @param n :: A new size, must be greater than the current size.
   */
  void FunctionValues::expand(size_t n)
  {
    if (n < size())
    {
      throw std::invalid_argument("Cannot make FunctionValues smaller");
    }
    m_calculated.resize(n);
    if (!m_data.empty())
    {
      m_data.resize(n);
    }
    if (!m_weights.empty())
    {
      m_weights.resize(n);
    }
  }

  /// set all calculated values to same number
  void FunctionValues::setCalculated(double value)
  {
    std::fill(m_calculated.begin(),m_calculated.end(),value);
  }

  /**
   * Get a pointer to calculated data at index i
   * @param i :: Index.
   */
  double* FunctionValues::getPointerToCalculated(size_t i)
  {
    if (i < size())
    {
      return &m_calculated[i];
    }
    throw std::out_of_range("FunctionValue index out of range.");
  }


  /// Set all calculated values to zero
  void FunctionValues::zeroCalculated()
  {
    setCalculated(0.0);
  }

  /**
   * Copy calculated values to a buffer
   * @param to :: Pointer to the buffer
   */
  void FunctionValues::copyTo(double* to) const
  {
    std::copy(m_calculated.begin(),m_calculated.end(),to);
  }

  /// Add calculated values to values in a buffer and save result to the buffer
  /// @param to :: Pointer to the buffer, it must be large enough
  void FunctionValues::add(double* to) const
  {
    std::transform(m_calculated.begin(),m_calculated.end(),to,to,std::plus<double>());
  }

  /// Multiply calculated values by values in a buffer and save result to the buffer
  /// @param to :: Pointer to the buffer, it must be large enough
  void FunctionValues::multiply(double* to) const
  {
    std::transform(m_calculated.begin(),m_calculated.end(),to,to,std::multiplies<double>());
  }

  /**
   * Set a fitting data value.
   * @param i :: Index
   * @param value :: Value
   */
  void FunctionValues::setFitData(size_t i,double value)
  {
    if (m_data.size() != m_calculated.size())
    {
      m_data.resize(m_calculated.size());
    }
    m_data[i] = value;
  }

  /**
   * Set fitting data values.
   * @param values :: Values for fitting
   */
  void FunctionValues::setFitData(const std::vector<double>& values)
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
  double FunctionValues::getFitData(size_t i) const
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
  void FunctionValues::setFitWeight(size_t i,double value)
  {
    if (m_weights.size() != m_calculated.size())
    {
      m_weights.resize(m_calculated.size());
    }
    m_weights[i] = value;
  }

  /**
   * Set fitting data values.
   * @param values :: Values for fitting
   */
  void FunctionValues::setFitWeights(const std::vector<double>& values)
  {
    if (values.size() != this->size())
    {
      throw std::invalid_argument("Setting data of a wrong size");
    }
    m_weights.assign(values.begin(),values.end());
  }

  /**
   * Set all weights to the same value.
   * @param value :: The value to set.
   */
  void FunctionValues::setFitWeights(const double& value)
  {
    m_weights.resize(m_calculated.size(),value);
  }


  /**
   * Get a fitting weight.
   * @param i :: Index
   */
  double FunctionValues::getFitWeight(size_t i) const
  {
    if (m_weights.size() != m_calculated.size())
    {
      throw std::runtime_error("Fitting weights was not set");
    }
    return m_weights[i];
  }

  /**
   * Set fitting data copied from other FunctionValues' calculated values.
   */
  void FunctionValues::setFitDataFromCalculated(const FunctionValues& values)
  {
    m_data.assign(values.m_calculated.begin(),values.m_calculated.end());
  }

} // namespace API
} // namespace Mantid
