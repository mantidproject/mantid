//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidKernel/Exception.h"
#include "MantidAPI/IFunction.h"
#include "MantidAPI/IConstraint.h"
#include "MantidAPI/ParameterTie.h"
#include "MantidDataObjects/Workspace2D.h"

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
  for(std::vector<std::pair<int,ParameterTie*> >::iterator it = m_ties.begin();it != m_ties.end(); it++)
      delete it->second;
  m_ties.clear();
}

/** Base class implementation of derivative function throws error. This is to check if such a function is provided
    by derivative class. In the derived classes this method must return the derivatives of the resuduals function
    (defined in void Fit1D::function(const double*, double*, const double*, const double*, const double*, const int&))
    with respect to the fit parameters. If this method is not reimplemented the derivative free simplex minimization
    algorithm is used.
* @param out Derivatives
* @param xValues X values for data points
* @param nData Number of data points
 */
void IFunction::functionDeriv(Jacobian* out, const double* xValues, const int& nData)
{
  throw Kernel::Exception::NotImplementedError("No derivative function provided");
}

/// Initialize the function providing it the workspace
void IFunction::setWorkspace(boost::shared_ptr<const DataObjects::Workspace2D> workspace,int wi,int xMin,int xMax)
{
  m_workspace = workspace;
  m_workspaceIndex = wi;
  m_xMinIndex = xMin;
  m_xMaxIndex = xMax;
}

/** Add a constraint
 *  @param ic Pointer to a constraint.
 */
void IFunction::addConstraint(IConstraint* ic)
{
  m_constraints.push_back(ic);
}


/** This method calls function() and add any penalty to its output if constraints are violated.
*
* @param out function values of for the data points
* @param xValues X values for data points
* @param nData Number of data points
 */
void IFunction::functionWithConstraint(double* out, const double* xValues, const int& nData)
{
  function(out, xValues, nData);


  // Add penalty factor to function if any constraint is violated

  double penalty = 0.0;
  for (unsigned i = 0; i < m_constraints.size(); i++)
  {
    penalty += m_constraints[i]->check(this);
  }

  for (int i = 0; i < nData; i++)
  {
    out[i] += penalty;
  }
}


/** This method calls functionDeriv() and add any penalty to its output if constraints are violated.
*
* @param out Derivatives
* @param xValues X values for data points
* @param nData Number of data points
 */
void IFunction::functionDerivWithConstraint(Jacobian* out, const double* xValues, const int& nData)
{
  functionDeriv(out, xValues, nData);

  for (unsigned i = 0; i < m_constraints.size(); i++)
  {  
    boost::shared_ptr<std::vector<double> > penalty = m_constraints[i]->checkDeriv(this);

    // for each active paramter check if there is a penalty and if yes add to derivatives
    for (unsigned int ii = 0; ii<(*penalty).size(); ii++)
      if ((*penalty)[ii] != 0.0)
        out->addNumberToColumn((*penalty)[ii], ii);
  }
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
 * @param i The index of active parameter to set
 * @param value The new value for the parameter
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
  m_ties.push_back(std::pair<int,ParameterTie*>(i,tie));
  this->removeActive(i);
}

/**
 * Apply the ties.
 */
void IFunction::applyTies()
{
  for(std::vector<std::pair<int,ParameterTie*> >::iterator tie=m_ties.begin();tie!=m_ties.end();++tie)
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

/**
 * Writes a string that can be used in Fit.Function to create a copy of this function
 */
std::string IFunction::asString()const
{
  std::ostringstream ostr;
  ostr << "name="<<this->name();
  for(int i=0;i<nParams();i++)
    ostr<<','<<parameterName(i)<<'='<<parameter(i);
  return ostr.str();
}

/**
 * Operator <<
 * @param ostr The output stream
 * @param f The function
 */
std::ostream& operator<<(std::ostream& ostr,const IFunction& f)
{
  ostr << f.asString();
  return ostr;
}

} // namespace API
} // namespace Mantid
