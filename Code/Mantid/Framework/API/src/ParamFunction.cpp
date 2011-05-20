//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidKernel/Exception.h"
#include "MantidAPI/ParamFunction.h"
#include "MantidAPI/IConstraint.h"
#include "MantidAPI/ParameterTie.h"

#include <sstream>
#include <iostream>
#include <limits>

namespace Mantid
{
namespace API
{

/// Copy contructor
ParamFunction::ParamFunction(const ParamFunction& f)
{
  m_indexMap.assign(f.m_indexMap.begin(),f.m_indexMap.end());
  m_parameterNames.assign(f.m_parameterNames.begin(),f.m_parameterNames.end());
  m_parameterDescriptions.assign(f.m_parameterDescriptions.begin(),f.m_parameterDescriptions.end());
  m_parameters.assign(f.m_parameters.begin(),f.m_parameters.end());
}

///Assignment operator
ParamFunction& ParamFunction::operator=(const ParamFunction& f)
{
  m_indexMap.assign(f.m_indexMap.begin(),f.m_indexMap.end());
  m_parameterNames.assign(f.m_parameterNames.begin(),f.m_parameterNames.end());
  m_parameterDescriptions.assign(f.m_parameterDescriptions.begin(),f.m_parameterDescriptions.end());
  m_parameters.assign(f.m_parameters.begin(),f.m_parameters.end());
  return *this;
}

/// Destructor
ParamFunction::~ParamFunction()
{
  for(std::vector<ParameterTie*>::iterator it = m_ties.begin();it != m_ties.end(); it++)
  {
    delete *it;
  }
  m_ties.clear();
  for(std::vector<IConstraint*>::iterator it = m_constraints.begin();it!= m_constraints.end();it++)
  {
    delete *it;
  }
  m_constraints.clear();
}


/** Sets a new value to the i-th parameter.
 *  @param i :: The parameter index
 *  @param value :: The new value
 *  @param explicitlySet :: A boolean falgging the parameter as explicitly set (by user)
 */
void ParamFunction::setParameter(int i, const double& value, bool explicitlySet)
{
  if (i >= nParams() || i < 0)
  {
    throw std::out_of_range("ParamFunction parameter index out of range.");
  }
  m_parameters[i] = value;
  if (explicitlySet)
  {
    m_explicitlySet[i] = true;
  }
}

/** Sets a new parameter description to the i-th parameter.
 *  @param i :: The parameter index
 *  @param description :: New parameter description
 */
void ParamFunction::setParameterDescription(size_t i, const std::string& description)
{
  // Casting this to an int could result in it being turned into a -1
  // so need to check that also. 
  // @todo: nParams should return size_t and get rid of -1 error returns
  int iindex = static_cast<int>(i);
  if( iindex >= nParams() || iindex < 0 )
  {
    throw std::out_of_range("ParamFunction parameter index out of range.");
  }
  m_parameterDescriptions[i] = description;
}

/** Get the i-th parameter.
 *  @param i :: The parameter index
 *  @return the value of the requested parameter
 */
double ParamFunction::getParameter(int i)const
{
  if (i >= nParams() || i < 0)
  {
    throw std::out_of_range("ParamFunction parameter index out of range.");
  }
  return m_parameters[i];
}

/**
 * Sets a new value to a parameter by name.
 * @param name :: The name of the parameter.
 * @param value :: The new value
 * @param explicitlySet :: A boolean flagging the parameter as explicitly set (by user)
 */
void ParamFunction::setParameter(const std::string& name, const double& value, bool explicitlySet)
{
  std::string ucName(name);
  //std::transform(name.begin(), name.end(), ucName.begin(), toupper);
  std::vector<std::string>::const_iterator it = 
    std::find(m_parameterNames.begin(),m_parameterNames.end(),ucName);
  if (it == m_parameterNames.end())
  {
    std::ostringstream msg;
    msg << "ParamFunction parameter ("<<ucName<<") does not exist.";
    throw std::invalid_argument(msg.str());
  }
  setParameter(static_cast<int>(it - m_parameterNames.begin()),value,explicitlySet);
}

/**
 * Sets a new description to a parameter by name.
 * @param name :: The name of the parameter.
 * @param description :: New parameter description
 */
void ParamFunction::setParameterDescription(const std::string& name, const std::string& description)
{
  std::string ucName(name);
  //std::transform(name.begin(), name.end(), ucName.begin(), toupper);
  std::vector<std::string>::const_iterator it = 
    std::find(m_parameterNames.begin(),m_parameterNames.end(),ucName);
  if (it == m_parameterNames.end())
  {
    std::ostringstream msg;
    msg << "ParamFunction parameter ("<<ucName<<") does not exist.";
    throw std::invalid_argument(msg.str());
  }
  setParameterDescription(static_cast<int>(it - m_parameterNames.begin()),description);
}


/**
 * Parameters by name.
 * @param name :: The name of the parameter.
 * @return the value of the named parameter
 */
double ParamFunction::getParameter(const std::string& name)const
{
  std::string ucName(name);
  //std::transform(name.begin(), name.end(), ucName.begin(), toupper);
  std::vector<std::string>::const_iterator it = 
    std::find(m_parameterNames.begin(),m_parameterNames.end(),ucName);
  if (it == m_parameterNames.end())
  {
    std::ostringstream msg;
    msg << "ParamFunction parameter ("<<ucName<<") does not exist.";
    throw std::invalid_argument(msg.str());
  }
  return m_parameters[it - m_parameterNames.begin()];
}

/**
 * Returns the index of the parameter named name.
 * @param name :: The name of the parameter.
 * @return the index of the named parameter
 */
int ParamFunction::parameterIndex(const std::string& name)const
{
  std::string ucName(name);
  //std::transform(name.begin(), name.end(), ucName.begin(), toupper);
  std::vector<std::string>::const_iterator it = 
    std::find(m_parameterNames.begin(),m_parameterNames.end(),ucName);
  if (it == m_parameterNames.end())
  {
    std::ostringstream msg;
    msg << "ParamFunction "<<this->name()<<" does not have parameter ("<<ucName<<").";
    throw std::invalid_argument(msg.str());
  }
  return int(it - m_parameterNames.begin());
}

/** Returns the name of parameter i
 * @param i :: The index of a parameter
 * @return the name of the parameter at the requested index
 */
std::string ParamFunction::parameterName(int i)const
{
  if (i >= nParams() || i < 0)
  {
    throw std::out_of_range("ParamFunction parameter index out of range.");
  }
  return m_parameterNames[i];
}

/** Returns the description of parameter i
 * @param i :: The index of a parameter
 * @return the description of the parameter at the requested index
 */
std::string ParamFunction::parameterDescription(size_t i)const
{
  if (static_cast<int>(i) >= nParams())
  {
    throw std::out_of_range("ParamFunction parameter index out of range.");
  }
  return m_parameterDescriptions[i];
}

/**
 * Declare a new parameter. To used in the implementation'c constructor.
 * @param name :: The parameter name.
 * @param initValue :: The initial value for the parameter
 * @param description :: The description for the parameter
 */
void ParamFunction::declareParameter(const std::string& name,double initValue, const std::string& description)
{
  std::string ucName(name);
  //std::transform(name.begin(), name.end(), ucName.begin(), toupper);
  std::vector<std::string>::const_iterator it = 
    std::find(m_parameterNames.begin(),m_parameterNames.end(),ucName);
  if (it != m_parameterNames.end())
  {
    std::ostringstream msg;
    msg << "ParamFunction parameter ("<<ucName<<") already exists.";
    throw std::invalid_argument(msg.str());
  }

  m_indexMap.push_back(nParams());
  m_parameterNames.push_back(ucName);
  m_parameterDescriptions.push_back(description);
  m_parameters.push_back(initValue);
  m_explicitlySet.push_back(false);
}

/**
 * Returns the "global" index of an active parameter.
 * @param i :: The index of an active parameter
 * @return the global index of the requested parameter
 */
int ParamFunction::indexOfActive(int i)const
{
  if (i >= nActive())
    throw std::out_of_range("ParamFunction parameter index out of range.");

  return m_indexMap[i];
}

/**
 * Returns the name of an active parameter.
 * @param i :: The index of an active parameter
 * @return the name of the active parameter
 */
std::string ParamFunction::nameOfActive(int i)const
{
  return m_parameterNames[indexOfActive(i)];
}

/**
 * Returns the description of an active parameter.
 * @param i :: The index of an active parameter
 * @return the description of the active parameter
 */
std::string ParamFunction::descriptionOfActive(size_t i)const
{
  return m_parameterDescriptions[indexOfActive(static_cast<int>(i))];
}

/**
 * query if the parameter is active
 * @param i :: The index of a declared parameter
 * @return true if parameter i is active
 */
bool ParamFunction::isActive(int i)const
{
  return std::find(m_indexMap.begin(),m_indexMap.end(),i) != m_indexMap.end();
}

/** This method doesn't create a tie
 * @param i :: A declared parameter index to be removed from active
 */
void ParamFunction::removeActive(int i)
{
  if (!isActive(i)) return;
  if (i >= nParams() || i < 0)
    throw std::out_of_range("ParamFunction parameter index out of range.");

  if (m_indexMap.size() == 0)// This should never happen
  {
    for(int j=0;j<nParams();j++)
      if (j != i)
        m_indexMap.push_back(j);
  }
  else
  {
    std::vector<int>::iterator it = std::find(m_indexMap.begin(),m_indexMap.end(),i);
    if (it != m_indexMap.end())
    {
      m_indexMap.erase(it);
    }
  }
}

/** Makes a parameter active again. It doesn't change the parameter's tie.
 * @param i :: A declared parameter index to be restored to active
 */
void ParamFunction::restoreActive(int i)
{
  if (i >= nParams()  || i < 0)
    throw std::out_of_range("ParamFunction parameter index out of range.");

  if (nParams() == nActive()) return;

  std::vector<int>::iterator it = std::find_if(m_indexMap.begin(),m_indexMap.end(),std::bind2nd(std::greater<int>(),i));
  if (it != m_indexMap.end())
  {
    m_indexMap.insert(it,i);
  }
  else
  {
    m_indexMap.push_back(i);
  }
}

/**
 * @param i :: The index of a declared parameter
 * @return The index of declared parameter i in the list of active parameters or -1
 *         if the parameter is tied.
 */
int ParamFunction::activeIndex(int i)const
{
  std::vector<int>::const_iterator it = std::find(m_indexMap.begin(),m_indexMap.end(),i);
  if (it == m_indexMap.end()) return -1;
  return int(it - m_indexMap.begin());
}

/**
 * Attaches a tie to this ParamFunction. The attached tie is owned by the ParamFunction.
 * @param tie :: A pointer to a new tie
 */
void ParamFunction::addTie(ParameterTie* tie)
{
  size_t iPar = tie->getIndex();
  bool found = false;
  for(std::vector<ParameterTie*>::size_type i=0;i<m_ties.size();i++)
  {
    if (m_ties[i]->getIndex() == iPar) 
    {
      found = true;
      delete m_ties[i];
      m_ties[i] = tie;
      break;
    }
  }
  if (!found)
  {
    m_ties.push_back(tie);
  }
}

/**
 * Apply the ties.
 */
void ParamFunction::applyTies()
{
  for(std::vector<ParameterTie*>::iterator tie=m_ties.begin();tie!=m_ties.end();++tie)
  {
    (**tie).eval();
  }
}

/**
 * Used to find ParameterTie for a parameter i
 */
class ReferenceEqual
{
  const size_t m_i;///< index to find
public:
  /** Constructor
   */
  ReferenceEqual(int i):m_i(i){}
  /**Bracket operator
   * @param p :: the parameter you are looking for
   * @return True if found
   */
  bool operator()(ParameterReference* p)
  {
    return p->getIndex() == m_i;
  }
};

/** Removes i-th parameter's tie if it is tied or does nothing.
 * @param i :: The index of the tied parameter.
 * @return True if successfull
 */
bool ParamFunction::removeTie(int i)
{
  if (i >= nParams() || i < 0)
  {
    throw std::out_of_range("ParamFunction parameter index out of range.");
  }
  std::vector<ParameterTie*>::iterator it = std::find_if(m_ties.begin(),m_ties.end(),ReferenceEqual(i));
  if (it != m_ties.end())
  {
    delete *it;
    m_ties.erase(it);
    restoreActive(i);
    return true;
  }
  return false;
}

/** Get tie of parameter number i
 * @param i :: The index of a declared parameter.
 * @return A pointer to the tie
 */
ParameterTie* ParamFunction::getTie(int i)const
{
  if (i >= nParams() || i < 0)
  {
    throw std::out_of_range("ParamFunction parameter index out of range.");
  }
  std::vector<ParameterTie*>::const_iterator it = std::find_if(m_ties.begin(),m_ties.end(),ReferenceEqual(i));
  if (it != m_ties.end())
  {
    return *it;
  }
  return NULL;
}

/** Remove all ties
 */
void ParamFunction::clearTies()
{
  for(std::vector<ParameterTie*>::iterator it = m_ties.begin();it != m_ties.end(); it++)
  {
    int i = getParameterIndex(**it);
    restoreActive(i);
    delete *it;
  }
  m_ties.clear();
}

/** Add a constraint
 *  @param ic :: Pointer to a constraint.
 */
void ParamFunction::addConstraint(IConstraint* ic)
{
  size_t iPar = ic->getIndex();
  bool found = false;
  for(std::vector<IConstraint*>::size_type i=0;i<m_constraints.size();i++)
  {
    if (m_constraints[i]->getIndex() == iPar) 
    {
      found = true;
      delete m_constraints[i];
      m_constraints[i] = ic;
      break;
    }
  }
  if (!found)
  {
    m_constraints.push_back(ic);
  }
}

/** Get constraint of parameter number i
 * @param i :: The index of a declared parameter.
 * @return A pointer to the constraint or NULL
 */
IConstraint* ParamFunction::getConstraint(int i)const
{
  if (i >= nParams() || i < 0)
  {
    throw std::out_of_range("ParamFunction parameter index out of range.");
  }
  std::vector<IConstraint*>::const_iterator it = std::find_if(m_constraints.begin(),m_constraints.end(),ReferenceEqual(i));
  if (it != m_constraints.end())
  {
    return *it;
  }
  return NULL;
}

/** Remove a constraint
 * @param parName :: The name of a parameter which constarint to remove.
 */
void ParamFunction::removeConstraint(const std::string& parName)
{
  size_t iPar = parameterIndex(parName);
  for(std::vector<IConstraint*>::iterator it=m_constraints.begin();it!=m_constraints.end();it++)
  {
    if (iPar == (**it).getIndex())
    {
      delete *it;
      m_constraints.erase(it);
      break;
    }
  }
}

void ParamFunction::setParametersToSatisfyConstraints()
{
  for (unsigned int i = 0; i < m_constraints.size(); i++)
  {
    m_constraints[i]->setParamToSatisfyConstraint();
  }
}



/// Nonvirtual member which removes all declared parameters
void ParamFunction::clearAllParameters()
{
  for(std::vector<ParameterTie*>::iterator it = m_ties.begin();it != m_ties.end(); it++)
  {
    delete *it;
  }
  m_ties.clear();
  for(std::vector<IConstraint*>::iterator it = m_constraints.begin();it!= m_constraints.end();it++)
  {
    delete *it;
  }
  m_constraints.clear();

  m_parameters.clear();
  m_parameterNames.clear();
  m_parameterDescriptions.clear();
  m_indexMap.clear();
}

/// Get the address of the parameter
/// @param i :: the index of the parameter required
/// @returns the address of the parameter
double* ParamFunction::getParameterAddress(int i)
{
  if (i >= nParams() || i < 0)
  {
    throw std::out_of_range("ParamFunction parameter index out of range.");
  }
  return &m_parameters[i];
}

/// Checks if a parameter has been set explicitly
bool ParamFunction::isExplicitlySet(int i)const
{
  if (i >= nParams() || i < 0)
  {
    throw std::out_of_range("ParamFunction parameter index out of range.");
  }
  return m_explicitlySet[i];
}

/**
 * Returns the index of parameter if the ref points to this ParamFunction or -1
 * @param ref :: A reference to a parameter
 * @return Parameter index or -1
 */
int ParamFunction::getParameterIndex(const ParameterReference& ref)const
{
  if (ref.getFunction() == this && static_cast<int>(ref.getIndex()) < nParams())
  {
    return static_cast<int>(ref.getIndex());
  }
  return -1;
}

/**
 * @param ref :: The reference
 * @return A ParamFunction containing parameter pointed to by ref
 */
IFitFunction* ParamFunction::getContainingFunction(const ParameterReference& ref)const
{
  if (ref.getFunction() == this && static_cast<int>(ref.getIndex()) < nParams())
  {
    return ref.getFunction();
  }
  return NULL;
}

/**
 * @param fun :: The ParamFunction
 * @return A ParamFunction containing fun
 */
IFitFunction* ParamFunction::getContainingFunction(const IFitFunction* fun)
{
  if (fun == this)
  {
    return this;
  }
  return NULL;
}

} // namespace API
} // namespace Mantid
