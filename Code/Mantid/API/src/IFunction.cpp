//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/IFunction.h"
#include "MantidKernel/Exception.h"

#include <sstream>

namespace Mantid
{
namespace API
{

/** Update parameters
 *  @param in Pointer to an array with active parameters values.
 */
void IFunction::updateActive(const double* in)
{
  if (in)
    for(int i=0;i<nActive();i++)
      activeParameter(i) = in[i];
  // apply ties
}

double& IFunction::activeParameter(int i)
{
  return (m_indexMap.size() > 0) ? parameter( m_indexMap[i] ) : parameter(i);
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
  return m_parameters[0];
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
  if (it == m_parameterNames.end())
  {
    std::ostringstream msg;
    msg << "Function parameter ("<<name<<") already exists.";
    throw std::invalid_argument(msg.str());
  }

  m_parameterNames.push_back(name);
  m_parameters.push_back(initValue);
}


} // namespace API
} // namespace Mantid
