//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidKernel/Exception.h"
#include "MantidAPI/Function.h"
#include "MantidAPI/IConstraint.h"
#include "MantidAPI/ParameterTie.h"

#include <sstream>
#include <iostream>

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
}

/** Add a constraint
 *  @param ic Pointer to a constraint.
 */
void Function::addConstraint(IConstraint* ic)
{
  m_constraints.push_back(ic);
}

/** Reference to the i-th parameter.
 *  @param i The parameter index
 */
double& Function::parameter(int i)
{
  if (i >= nParams())
    throw std::out_of_range("Function parameter index out of range.");
  return m_parameters[i];
}

/** Reference to the i-th parameter.
 *  @param i The parameter index
 */
double Function::parameter(int i)const
{
  if (i >= nParams())
    throw std::out_of_range("Function parameter index out of range.");
  return m_parameters[i];
}

/**
 * Parameters by name.
 * @param name The name of the parameter.
 */
double& Function::getParameter(const std::string& name)
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
    msg << "Function parameter ("<<ucName<<") does not exist.";
    throw std::invalid_argument(msg.str());
  }
  return int(it - m_parameterNames.begin());
}

/**
 * Checks that a pointer points to a parameter of this function and returns its index.
 * @param p A pointer to a double variable.
 * @return The index of the parameter or -1 if p is not a pointer to any of the function's parameters.
 */
int Function::parameterIndex(const double* p)const
{
  const double* p0 = &m_parameters.front();
  const double* p1 = &m_parameters.back();
  if (p >= p0 && p <= p1)
  {
    return p - p0;
  }
  return -1;
}

/** Returns the name of parameter i
 * @param i The index of a parameter
 */
std::string Function::parameterName(int i)const
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
}

/** This method calls function() and add any penalty to its output if constraints are violated.
*
* @param out function values of for the data points
* @param xValues X values for data points
* @param nData Number of data points
 */
void Function::functionWithConstraint(double* out, const double* xValues, const int& nData)
{
  function(out, xValues, nData);


  // Add penalty factor to function if any constraint is violated

 /* double penalty = 0.0;
  for (unsigned i = 0; i < m_constraints.size(); i++)
  {
    penalty += m_constraints[i]->check(this);
  }

  for (int i = 0; i < nData; i++)
  {
    out[i] += penalty;
  }*/
}


/** This method calls functionDeriv() and add any penalty to its output if constraints are violated.
*
* @param out Derivatives
* @param xValues X values for data points
* @param nData Number of data points
 */
void Function::functionDerivWithConstraint(Jacobian* out, const double* xValues, const int& nData)
{
  functionDeriv(out, xValues, nData);

 /* for (unsigned i = 0; i < m_constraints.size(); i++)
  {  
    boost::shared_ptr<std::vector<double> > penalty = m_constraints[i]->checkDeriv(this);

    // for each active paramter check if there is a penalty and if yes add to derivatives
    for (unsigned int ii = 0; ii<(*penalty).size(); ii++)
      if ((*penalty)[ii] != 0.0)
        out->addNumberToColumn((*penalty)[ii], ii);
  }*/
}


/**
 * Returns the "global" index of an active parameter.
 * @param i The index of an active parameter
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
 */
std::string Function::nameOfActive(int i)const
{
  return m_parameterNames[indexOfActive(i)];
}

/**
 * Returns true if parameter i is active
 * @param i The index of a declared parameter
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
  if (i >= nParams())
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
  if (i >= nParams())
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
  m_ties.push_back(tie);
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
class TieEqual
{
  const double* m_par;///< pointer
public:
  /** Constructor
   * @param par A pointer to the parameter you want to search for
   */
  TieEqual(const double* par):m_par(par){}
  /**
   * @return True if found
   */
  bool operator()(ParameterTie* p)
  {
    return p->parameter() == m_par;
  }
};

/** Removes i-th parameter's tie if it is tied or does nothing.
 * @param i The index of the tied parameter.
 * @return True if successfull
 */
bool Function::removeTie(int i)
{
  if (i >= nParams())
  {
    throw std::out_of_range("Function parameter index out of range.");
  }
  double* par = &parameter(i);
  std::vector<ParameterTie*>::iterator it = std::find_if(m_ties.begin(),m_ties.end(),TieEqual(par));
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
  if (i >= nParams())
  {
    throw std::out_of_range("Function parameter index out of range.");
  }
  const double* par = &m_parameters[i];
  std::vector<ParameterTie*>::const_iterator it = std::find_if(m_ties.begin(),m_ties.end(),TieEqual(par));
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
    int i = parameterIndex((**it).parameter());
    restoreActive(i);
    delete *it;
  }
  m_ties.clear();
}

} // namespace API
} // namespace Mantid
