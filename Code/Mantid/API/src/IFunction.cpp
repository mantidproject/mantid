//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/IFunction.h"
#include "MantidKernel/Exception.h"

#include <sstream>
#include <iostream>

namespace Mantid
{
namespace API
{

/// Copy contructor
IFunction::IFunction(const IFunction& f)
{
  m_indexMap.assign(f.m_indexMap.begin(),f.m_indexMap.end());
  m_parameterNames.assign(f.m_parameterNames.begin(),f.m_parameterNames.end());
  m_parameters.assign(f.m_parameters.begin(),f.m_parameters.end());
}

///Assignment operator
IFunction& IFunction::operator=(const IFunction& f)
{
  m_indexMap.assign(f.m_indexMap.begin(),f.m_indexMap.end());
  m_parameterNames.assign(f.m_parameterNames.begin(),f.m_parameterNames.end());
  m_parameters.assign(f.m_parameters.begin(),f.m_parameters.end());
  return *this;
}

/** Update parameters
 *  @param in Pointer to an array with active parameters values.
 */
void IFunction::updateActive(const double* in)
{
  if (in)
    for(int i=0;i<nActive();i++)
    {
      setActiveParameter(i,in[i]);
    }
  // apply ties
}

void IFunction::setActiveParameter(int i,double value)
{
  int j = indexOfActive(i);
  parameter(j) = value;
}

double IFunction::activeParameter(int i)const
{
  int j = indexOfActive(i);
  return parameter(j);
}

/** Reference to the i-th parameter.
 *  @param i The parameter index
 */
double& IFunction::parameter(int i)
{
  if (i >= nParams())
    throw std::out_of_range("Function parameter index out of range.");
  return m_parameters[i];
}

/** Reference to the i-th parameter.
 *  @param i The parameter index
 */
double IFunction::parameter(int i)const
{
  if (i >= nParams())
    throw std::out_of_range("Function parameter index out of range.");
  return m_parameters[i];
}

/**
 * Parameters by name.
 * @param name The name of the parameter.
 */
double& IFunction::getParameter(const std::string& name)
{
  std::vector<std::string>::const_iterator it = 
    std::find(m_parameterNames.begin(),m_parameterNames.end(),name);
  if (it == m_parameterNames.end())
  {
    std::ostringstream msg;
    msg << "Function parameter ("<<name<<") does not exist.";
    throw std::invalid_argument(msg.str());
  }
  return m_parameters[it - m_parameterNames.begin()];
}

/**
 * Parameters by name.
 * @param name The name of the parameter.
 */
double IFunction::getParameter(const std::string& name)const
{
  std::vector<std::string>::const_iterator it = 
    std::find(m_parameterNames.begin(),m_parameterNames.end(),name);
  if (it == m_parameterNames.end())
  {
    std::ostringstream msg;
    msg << "Function parameter ("<<name<<") does not exist.";
    throw std::invalid_argument(msg.str());
  }
  return m_parameters[it - m_parameterNames.begin()];
}

/** Returns the name of parameter i
 * @param i The index of a parameter
 */
std::string IFunction::parameterName(int i)const
{
  if (i >= nParams())
    throw std::out_of_range("Function parameter index out of range.");
  return m_parameterNames[i];
}
/**
 * Declare a new parameter. To used in the implementation'c constructor.
 * @param name The parameter name.
 * @param initValue The initial value for the parameter
 */
void IFunction::declareParameter(const std::string& name,double initValue )
{
  std::vector<std::string>::const_iterator it = 
    std::find(m_parameterNames.begin(),m_parameterNames.end(),name);
  if (it != m_parameterNames.end())
  {
    std::ostringstream msg;
    msg << "Function parameter ("<<name<<") already exists.";
    throw std::invalid_argument(msg.str());
  }

  m_parameterNames.push_back(name);
  m_parameters.push_back(initValue);
}

/**
 * Returns the "global" index of an active parameter.
 * @param i The index of an active parameter
 */
int IFunction::indexOfActive(int i)const
{
  if (m_indexMap.empty()) 
  {
    if (i < nParams()) return i;
    else
      throw std::out_of_range("Function parameter index out of range.");
  }

  if (i >= nActive())
    throw std::out_of_range("Function parameter index out of range.");

  return m_indexMap[i];
}

/**
 * Returns the name of an active parameter.
 * @param i The index of an active parameter
 */
std::string IFunction::nameOfActive(int i)const
{
  return m_parameterNames[indexOfActive(i)];
}

/**
 * Calculate the Jacobian with respect to parameters actually declared in the function
 * @param out The output Jacobian
 * @param xValues The x-values
 * @param nData The number of data points (and x-values).
 */
void IFunction::calJacobianForCovariance(Jacobian* out, const double* xValues, const int& nData)
{
  this->functionDeriv(out,xValues,nData);
}

} // namespace API
} // namespace Mantid
