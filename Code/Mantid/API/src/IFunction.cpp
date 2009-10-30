//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidKernel/Exception.h"
#include "MantidAPI/IFunction.h"
#include "MantidAPI/ParameterTie.h"

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

/// Destructor
IFunction::~IFunction()
{
  for(std::map<int,ParameterTie*>::iterator it = m_ties.begin();it != m_ties.end(); it++)
      delete it->second;
  m_ties.clear();
}

/** Update active parameters. Ties are applied.
 *  @param in Pointer to an array with active parameters values.
 */
void IFunction::updateActive(const double* in)
{
  if (in)
    for(int i=0;i<nActive();i++)
    {
      setActiveParameter(i,in[i]);
    }
  applyTies();
}

/**
 * Sets active parameter i to value. Ties are not applied.
 */
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
  std::string ucName(name);
  //std::transform(name.begin(), name.end(), ucName.begin(), toupper);
  std::vector<std::string>::const_iterator it = 
    std::find(m_parameterNames.begin(),m_parameterNames.end(),ucName);
  if (it == m_parameterNames.end())
  {
    std::ostringstream msg;
    msg << "Function parameter ("<<ucName<<") does not exist.";
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
  std::string ucName(name);
  //std::transform(name.begin(), name.end(), ucName.begin(), toupper);
  std::vector<std::string>::const_iterator it = 
    std::find(m_parameterNames.begin(),m_parameterNames.end(),ucName);
  if (it == m_parameterNames.end())
  {
    std::ostringstream msg;
    msg << "Function parameter ("<<ucName<<") does not exist.";
    throw std::invalid_argument(msg.str());
  }
  return m_parameters[it - m_parameterNames.begin()];
}

/**
 * Returns the index of the parameter named name.
 * @param name The name of the parameter.
 */
int IFunction::parameterIndex(const std::string& name)const
{
  std::string ucName(name);
  //std::transform(name.begin(), name.end(), ucName.begin(), toupper);
  std::vector<std::string>::const_iterator it = 
    std::find(m_parameterNames.begin(),m_parameterNames.end(),ucName);
  if (it == m_parameterNames.end())
  {
    std::ostringstream msg;
    msg << "Function parameter ("<<ucName<<") does not exist.";
    throw std::invalid_argument(msg.str());
  }
  return int(it - m_parameterNames.begin());
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
  std::string ucName(name);
  //std::transform(name.begin(), name.end(), ucName.begin(), toupper);
  std::vector<std::string>::const_iterator it = 
    std::find(m_parameterNames.begin(),m_parameterNames.end(),ucName);
  if (it != m_parameterNames.end())
  {
    std::ostringstream msg;
    msg << "Function parameter ("<<ucName<<") already exists.";
    throw std::invalid_argument(msg.str());
  }

  m_indexMap.push_back(nParams());
  m_parameterNames.push_back(ucName);
  m_parameters.push_back(initValue);
}

/**
 * Returns the "global" index of an active parameter.
 * @param i The index of an active parameter
 */
int IFunction::indexOfActive(int i)const
{
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
 * Returns true if parameter i is active
 * @param i The index of a declared parameter
 */
bool IFunction::isActive(int i)const
{
  return std::find(m_indexMap.begin(),m_indexMap.end(),i) != m_indexMap.end();
}

/**
 * @param i A declared parameter index to be removed from active
 */
void IFunction::removeActive(int i)
{
  if (i >= nParams())
    throw std::out_of_range("Function parameter index out of range.");

  if (m_indexMap.size() == 0)
  {
    for(int j=0;j<nParams();j++)
      if (j != i)
        m_indexMap.push_back(j);
  }
  else
  {
    std::vector<int>::iterator it = std::find(m_indexMap.begin(),m_indexMap.end(),i);
    if (it != m_indexMap.end())
      m_indexMap.erase(it);
  }
}

/**
 * @param i The index of a declared parameter
 * @return The index of declared parameter i in the list of active parameters or -1
 *         if the parameter is tied.
 */
int IFunction::activeIndex(int i)const
{
  std::vector<int>::const_iterator it = std::find(m_indexMap.begin(),m_indexMap.end(),i);
  if (it == m_indexMap.end()) return -1;
  return int(it - m_indexMap.begin());
}

/**
 * Ties a parameter to other parameters
 * @param parName The name of the parameter to tie.
 * @param expr    A math expression 
 */
void IFunction::tie(const std::string& parName,const std::string& expr)
{
  ParameterTie* tie = new ParameterTie(this,parName);
  int i = tie->index();
  if (!this->isActive(i))
  {
    delete tie;
    throw std::logic_error("Parameter "+parName+" is already tied.");
  }
  tie->set(expr);
  m_ties[i] = tie;
  this->removeActive(i);
}

/**
 * Apply the ties.
 */
void IFunction::applyTies()
{
  for(std::map<int,ParameterTie*>::iterator tie=m_ties.begin();tie!=m_ties.end();++tie)
  {
    this->parameter(tie->first) = tie->second->eval();
  }
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
