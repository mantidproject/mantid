//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidKernel/Exception.h"
#include "MantidAPI/Function.h"
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
Function::Function(const Function& f)
{
  m_indexMap.assign(f.m_indexMap.begin(),f.m_indexMap.end());
  m_parameterNames.assign(f.m_parameterNames.begin(),f.m_parameterNames.end());
  m_parameters.assign(f.m_parameters.begin(),f.m_parameters.end());
}

///Assignment operator
Function& Function::operator=(const Function& f)
{
  m_indexMap.assign(f.m_indexMap.begin(),f.m_indexMap.end());
  m_parameterNames.assign(f.m_parameterNames.begin(),f.m_parameterNames.end());
  m_parameters.assign(f.m_parameters.begin(),f.m_parameters.end());
  return *this;
}

/// Destructor
Function::~Function()
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
 *  @param i The parameter index
 *  @param value The new value
 *  @param explicitlySet A boolean falgging the parameter as explicitly set (by user)
 */
void Function::setParameter(int i, const double& value, bool explicitlySet)
{
  if (i >= nParams() || i < 0)
  {
    throw std::out_of_range("Function parameter index out of range.");
  }
  m_parameters[i] = value;
  if (explicitlySet)
  {
    m_explicitlySet[i] = true;
  }
}

/** Get the i-th parameter.
 *  @param i The parameter index
 *  @return the value of the requested parameter
 */
double Function::getParameter(int i)const
{
  if (i >= nParams() || i < 0)
  {
    throw std::out_of_range("Function parameter index out of range.");
  }
  return m_parameters[i];
}

/**
 * Sets a new value to a parameter by name.
 * @param name The name of the parameter.
 * @param value The new value
 * @param explicitlySet A boolean flagging the parameter as explicitly set (by user)
 */
void Function::setParameter(const std::string& name, const double& value, bool explicitlySet)
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
  setParameter(it - m_parameterNames.begin(),value,explicitlySet);
}

/**
 * Parameters by name.
 * @param name The name of the parameter.
 * @return the value of the named parameter
 */
double Function::getParameter(const std::string& name)const
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
 * @return the index of the named parameter
 */
int Function::parameterIndex(const std::string& name)const
{
  std::string ucName(name);
  //std::transform(name.begin(), name.end(), ucName.begin(), toupper);
  std::vector<std::string>::const_iterator it = 
    std::find(m_parameterNames.begin(),m_parameterNames.end(),ucName);
  if (it == m_parameterNames.end())
  {
    std::ostringstream msg;
    msg << "Function "<<this->name()<<" does not have parameter ("<<ucName<<").";
    throw std::invalid_argument(msg.str());
  }
  return int(it - m_parameterNames.begin());
}

/** Returns the name of parameter i
 * @param i The index of a parameter
 * @return the name of the parameter at the requested index
 */
std::string Function::parameterName(int i)const
{
  if (i >= nParams() || i < 0)
  {
    throw std::out_of_range("Function parameter index out of range.");
  }
  return m_parameterNames[i];
}
/**
 * Declare a new parameter. To used in the implementation'c constructor.
 * @param name The parameter name.
 * @param initValue The initial value for the parameter
 */
void Function::declareParameter(const std::string& name,double initValue )
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
  m_explicitlySet.push_back(false);
}

/**
 * Returns the "global" index of an active parameter.
 * @param i The index of an active parameter
 * @return the global index of the requested parameter
 */
int Function::indexOfActive(int i)const
{
  if (i >= nActive())
    throw std::out_of_range("Function parameter index out of range.");

  return m_indexMap[i];
}

/**
 * Returns the name of an active parameter.
 * @param i The index of an active parameter
 * @return the name of the active parameter
 */
std::string Function::nameOfActive(int i)const
{
  return m_parameterNames[indexOfActive(i)];
}

/**
 * query if the parameter is active
 * @param i The index of a declared parameter
 * @return true if parameter i is active
 */
bool Function::isActive(int i)const
{
  return std::find(m_indexMap.begin(),m_indexMap.end(),i) != m_indexMap.end();
}

/** This method doesn't create a tie
 * @param i A declared parameter index to be removed from active
 */
void Function::removeActive(int i)
{
  if (!isActive(i)) return;
  if (i >= nParams() || i < 0)
    throw std::out_of_range("Function parameter index out of range.");

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
 * @param i A declared parameter index to be restored to active
 */
void Function::restoreActive(int i)
{
  if (i >= nParams()  || i < 0)
    throw std::out_of_range("Function parameter index out of range.");

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
 * @param i The index of a declared parameter
 * @return The index of declared parameter i in the list of active parameters or -1
 *         if the parameter is tied.
 */
int Function::activeIndex(int i)const
{
  std::vector<int>::const_iterator it = std::find(m_indexMap.begin(),m_indexMap.end(),i);
  if (it == m_indexMap.end()) return -1;
  return int(it - m_indexMap.begin());
}

/**
 * Attaches a tie to this function. The attached tie is owned by the function.
 * @param tie A pointer to a new tie
 */
void Function::addTie(ParameterTie* tie)
{
  int iPar = tie->getIndex();
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
void Function::applyTies()
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
  const int m_i;///< index to find
public:
  /** Constructor
   */
  ReferenceEqual(int i):m_i(i){}
  /**Bracket operator
   * @param p the parameter you are looking for
   * @return True if found
   */
  bool operator()(ParameterReference* p)
  {
    return p->getIndex() == m_i;
  }
};

/** Removes i-th parameter's tie if it is tied or does nothing.
 * @param i The index of the tied parameter.
 * @return True if successfull
 */
bool Function::removeTie(int i)
{
  if (i >= nParams() || i < 0)
  {
    throw std::out_of_range("Function parameter index out of range.");
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
 * @param i The index of a declared parameter.
 * @return A pointer to the tie
 */
ParameterTie* Function::getTie(int i)const
{
  if (i >= nParams() || i < 0)
  {
    throw std::out_of_range("Function parameter index out of range.");
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
void Function::clearTies()
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
 *  @param ic Pointer to a constraint.
 */
void Function::addConstraint(IConstraint* ic)
{
  int iPar = ic->getIndex();
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
 * @param i The index of a declared parameter.
 * @return A pointer to the constraint or NULL
 */
IConstraint* Function::getConstraint(int i)const
{
  if (i >= nParams() || i < 0)
  {
    throw std::out_of_range("Function parameter index out of range.");
  }
  std::vector<IConstraint*>::const_iterator it = std::find_if(m_constraints.begin(),m_constraints.end(),ReferenceEqual(i));
  if (it != m_constraints.end())
  {
    return *it;
  }
  return NULL;
}

/** Remove a constraint
 * @param parName The name of a parameter which constarint to remove.
 */
void Function::removeConstraint(const std::string& parName)
{
  int iPar = parameterIndex(parName);
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

void Function::setParametersToSatisfyConstraints()
{
  for (unsigned int i = 0; i < m_constraints.size(); i++)
  {
    m_constraints[i]->setParamToSatisfyConstraint();
  }
}


/** Calculate numerical derivatives.
 * @param out Derivatives
 * @param xValues X values for data points
 * @param nData Number of data points
 */
void Function::calNumericalDeriv(Jacobian* out, const double* xValues, const int& nData)
{
    double stepPercentage = 0.001; // step percentage
    double step; // real step
    double minDouble = std::numeric_limits<double>::min();
    double cutoff = 100.0*minDouble/stepPercentage;
    int nParam = nParams();

    // allocate memory if not already done
    if ( m_tmpFunctionOutputMinusStep.size() < nData)
    {
      m_tmpFunctionOutputMinusStep.resize(nData);
    }
    if ( m_tmpFunctionOutputPlusStep.size() < nData)
    {
      m_tmpFunctionOutputPlusStep.resize(nData);
    }

    function(&m_tmpFunctionOutputMinusStep[0], xValues, nData);

    for (int iP = 0; iP < nParam; iP++)
    {
      if ( isActive(iP) )
      {
        const double& val = getParameter(iP);
        if (val < cutoff)
        {
          step = cutoff;
        }
        else
        {
          step = val*stepPercentage;
        }

        double paramPstep = val + step;
        setParameter(iP, paramPstep);
        function(&m_tmpFunctionOutputPlusStep[0], xValues, nData);

        step = paramPstep - val;
        setParameter(iP, val);

        for (int i = 0; i < nData; i++) {
          out->set(i,iP, 
            (m_tmpFunctionOutputPlusStep[i]-m_tmpFunctionOutputMinusStep[i])/step);
        }
      }
    }
}


/// Nonvirtual member which removes all declared parameters
void Function::clearAllParameters()
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
  m_indexMap.clear();
}

/// Get the address of the parameter
/// @param i the index of the parameter required
/// @returns the address of the parameter
double* Function::getParameterAddress(int i)
{
  if (i >= nParams() || i < 0)
  {
    throw std::out_of_range("Function parameter index out of range.");
  }
  return &m_parameters[i];
}

/// Checks if a parameter has been set explicitly
bool Function::isExplicitlySet(int i)const
{
  if (i >= nParams() || i < 0)
  {
    throw std::out_of_range("Function parameter index out of range.");
  }
  return m_explicitlySet[i];
}

/**
 * Returns the index of parameter if the ref points to this function or -1
 * @param ref A reference to a parameter
 * @return Parameter index or -1
 */
int Function::getParameterIndex(const ParameterReference& ref)const
{
  if (ref.getFunction() == this && ref.getIndex() < nParams())
  {
    return ref.getIndex();
  }
  return -1;
}

/**
 * @param ref The reference
 * @return A function containing parameter pointed to by ref
 */
IFitFunction* Function::getContainingFunction(const ParameterReference& ref)const
{
  if (ref.getFunction() == this && ref.getIndex() < nParams())
  {
    return ref.getFunction();
  }
  return NULL;
}

/**
 * @param fun The function
 * @return A function containing fun
 */
IFitFunction* Function::getContainingFunction(const IFitFunction* fun)
{
  if (fun == this)
  {
    return this;
  }
  return NULL;
}

} // namespace API
} // namespace Mantid
