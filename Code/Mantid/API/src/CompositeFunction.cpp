//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/CompositeFunction.h"
#include "MantidKernel/Exception.h"

#include <boost/lexical_cast.hpp>
#include <boost/shared_array.hpp>
#include <sstream>
#include <iostream>

namespace Mantid
{
namespace API
{

DECLARE_FUNCTION(CompositeFunction)

/// Copy contructor
CompositeFunction::CompositeFunction(const CompositeFunction& f)
:m_nActive(f.m_nParams),m_nParams(f.m_nParams)
{
  m_functions.assign(f.m_functions.begin(),f.m_functions.end());
  m_activeOffsets.assign(f.m_activeOffsets.begin(),f.m_activeOffsets.end());
  m_paramOffsets.assign(f.m_paramOffsets.begin(),f.m_paramOffsets.end());
}

///Assignment operator
CompositeFunction& CompositeFunction::operator=(const CompositeFunction& f)
{
  m_nActive = f.m_nActive;
  m_nParams = f.m_nParams;
  m_functions.assign(f.m_functions.begin(),f.m_functions.end());
  m_activeOffsets.assign(f.m_activeOffsets.begin(),f.m_activeOffsets.end());
  m_paramOffsets.assign(f.m_paramOffsets.begin(),f.m_paramOffsets.end());
  return *this;
}

///Destructor
CompositeFunction::~CompositeFunction()
{
  for(int i=0;i<nFunctions();i++)
    if (m_functions[i]) delete m_functions[i];
}


/// Function initialization. Declare function parameters in this method.
void CompositeFunction::init()
{
}

/// Writes itself into a string
std::string CompositeFunction::asString()const
{
  std::ostringstream ostr;
  for(int i=0;i<nFunctions();i++)
  {
    ostr << getFunction(i)->asString() << ';';
  }
  return ostr.str();
}

/// Function you want to fit to.
void CompositeFunction::function(double* out, const double* xValues, const int& nData)
{
  if (nData <= 0) return;
  boost::shared_array<double> tmpOut(new double[nData]);
  for(int i=0;i<nFunctions();i++)
  {
    if (i == 0)
      m_functions[i]->function(out,xValues,nData);
    else
    {
      m_functions[i]->function(tmpOut.get(),xValues,nData);
      std::transform(out,out+nData,tmpOut.get(),out,std::plus<double>());
    }
  }
}

/** A Jacobian for individual functions
 */
class PartialJacobian: public Jacobian
{
  Jacobian* m_J;  ///< pointer to the overall Jacobian
  int m_iP0;      ///< offset in the overall Jacobian for a particular function
public:
  /** Constructor
   * @param J A pointer to the overall Jacobian
   * @param iP0 The parameter index (declared) offset for a particular function
   */
  PartialJacobian(Jacobian* J,int iP0):m_J(J),m_iP0(iP0)
  {}
  /**
   * Overridden Jacobian::set(...).
   * @param iY The index of the data point
   * @param iP The parameter index of an individual function.
   * @param value The derivative value
   */
  void set(int iY, int iP, double value)
  {
      m_J->set(iY,m_iP0 + iP,value);
  }
};

/// Derivatives of function with respect to active parameters
void CompositeFunction::functionDeriv(Jacobian* out, const double* xValues, const int& nData)
{
  if (nData <= 0) return;
  for(int i=0;i<nFunctions();i++)
  {
    PartialJacobian J(out,m_paramOffsets[i]);
    m_functions[i]->functionDeriv(&J,xValues,nData);
  }
}

/// Derivatives to be used in covariance matrix calculation. 
void CompositeFunction::calJacobianForCovariance(Jacobian* out, const double* xValues, const int& nData)
{
  if (nData <= 0) return;
  for(int i=0;i<nFunctions();i++)
  {
    PartialJacobian J(out,m_paramOffsets[i]);
    m_functions[i]->calJacobianForCovariance(&J,xValues,nData);
  }
}


/// Address of i-th parameter
double& CompositeFunction::parameter(int i)
{
  int iFun = functionIndex(i);
  return m_functions[ iFun ]->parameter(i - m_paramOffsets[iFun]);
}

/// Address of i-th parameter
double CompositeFunction::parameter(int i)const
{
  int iFun = functionIndex(i);
  return m_functions[ iFun ]->parameter(i - m_paramOffsets[iFun]);
}

/// Get parameter by name.
double& CompositeFunction::getParameter(const std::string& name)
{
  std::string pname;
  int index;
  parseName(name,index,pname);
  if (index < 0)
    throw std::invalid_argument("CompositeFunction::getParameter: parameter name must contain function index");
  else
  {   
    return getFunction(index)->getParameter(pname);
  }
}

/// Get parameter by name.
double CompositeFunction::getParameter(const std::string& name)const
{
  std::string pname;
  int index;
  parseName(name,index,pname);
  if (index < 0)
    throw std::invalid_argument("CompositeFunction::getParameter: parameter name must contain function index");
  else
  {   
    return getFunction(index)->getParameter(pname);
  }
}

/// Total number of parameters
int CompositeFunction::nParams()const
{
  return m_nParams;
}

/**
 * 
 * @param name The name of a parameter
 */
int CompositeFunction::parameterIndex(const std::string& name)const
{
  std::string pname;
  int index;
  parseName(name,index,pname);
  if (index < 0)
    throw std::invalid_argument("CompositeFunction::getParameter: parameter name must contain function index");

  return m_paramOffsets[index] + getFunction(index)->parameterIndex(pname);
}

/// Returns the name of parameter i
std::string CompositeFunction::parameterName(int i)const
{
  int iFun = functionIndex(i);
  std::ostringstream ostr;
  ostr << 'f' << iFun << '.' << m_functions[ iFun ]->parameterName(i - m_paramOffsets[iFun]);
  return ostr.str();
}

/// Number of active (in terms of fitting) parameters
int CompositeFunction::nActive()const
{
  return m_nActive;
}

/// Value of i-th active parameter. Override this method to make fitted parameters different from the declared
double CompositeFunction::activeParameter(int i)const
{
  int iFun = functionIndexActive(i);
  return m_functions[ iFun ]->activeParameter(i - m_activeOffsets[iFun]);
}

/// Set new value of i-th active parameter. Override this method to make fitted parameters different from the declared
void CompositeFunction::setActiveParameter(int i, double value)
{
  int iFun = functionIndexActive(i);
  return m_functions[ iFun ]->setActiveParameter(i - m_activeOffsets[iFun],value);
}

/// Update parameters after a fitting iteration
void CompositeFunction::updateActive(const double* in)
{
  for(int iFun = 0; iFun < int(m_functions.size()); iFun++)
  {
    m_functions[ iFun ]->updateActive(in + m_activeOffsets[ iFun ]);
  }
  applyTies();
}

/// Returns "global" index of active parameter i
int CompositeFunction::indexOfActive(int i)const
{
  int iFun = functionIndexActive(i);
  return m_paramOffsets[ iFun ] + m_functions[ iFun ]->indexOfActive(i - m_activeOffsets[iFun]);
}

/// Returns the name of active parameter i
std::string CompositeFunction::nameOfActive(int i)const
{
  int iFun = functionIndexActive(i);
  std::ostringstream ostr;
  ostr << 'f' << iFun << '.' << m_functions[ iFun ]->nameOfActive(i - m_activeOffsets[iFun]);
  return ostr.str();
}

/**
 * Returns true if parameter i is active
 * @param i The index of a declared parameter
 */
bool CompositeFunction::isActive(int i)const
{
  int iFun = functionIndex(i);
  return m_functions[ iFun ]->isActive(i - m_paramOffsets[iFun]);
}

/**
 * @param i A declared parameter index to be removed from active
 */
void CompositeFunction::removeActive(int i)
{
  int iFun = functionIndex(i);
  int ia = m_activeOffsets[iFun] + m_functions[iFun]->activeIndex(i - m_paramOffsets[iFun]);
  m_iFunctionActive.erase(m_iFunctionActive.begin()+ia);
  m_functions[ iFun ]->removeActive(i - m_paramOffsets[iFun]);

  --m_nActive;
  for(int j=iFun+1;j<nFunctions();j++)
    m_activeOffsets[j] -= 1;
}

/**
 * @param i The index of a declared parameter
 * @return The index of declared parameter i in the list of active parameters or -1
 *         if the parameter is tied.
 */
int CompositeFunction::activeIndex(int i)const
{
  int iFun = functionIndex(i);
  return m_activeOffsets[iFun] + m_functions[iFun]->activeIndex(i - m_paramOffsets[iFun]);
}

/** Add a function
 * @param f A pointer to the added function
 */
void CompositeFunction::addFunction(IFunction* f)
{
  m_iFunction.insert(m_iFunction.end(),f->nParams(),m_functions.size());
  m_iFunctionActive.insert(m_iFunctionActive.end(),f->nActive(),m_functions.size());
  m_functions.push_back(f);
  //?f->init();
  if (m_paramOffsets.size() == 0)
  {
    m_paramOffsets.push_back(0);
    m_activeOffsets.push_back(0);
    m_nParams = f->nParams();
    m_nActive = f->nActive();
  }
  else
  {
    m_paramOffsets.push_back(m_nParams);
    m_activeOffsets.push_back(m_nActive);
    m_nParams += f->nParams();
    m_nActive += f->nActive();
  }
}

/**
 * @param i The index of the function
 */
IFunction* CompositeFunction::getFunction(int i)const
{
  if ( i >= nFunctions() )
    throw std::out_of_range("Function index out of range.");

  return m_functions[i];
}

/**
 * Get the index of the function to which parameter i belongs
 * @param i The parameter index
 */
int CompositeFunction::functionIndex(int i)const
{
  if (i >= nParams())
    throw std::out_of_range("Function parameter index out of range.");
  return m_iFunction[i];
}

/**
 * Get the index of the function to which parameter i belongs
 * @param i The active parameter index
 */
int CompositeFunction::functionIndexActive(int i)const
{
  if (i >= nParams())
    throw std::out_of_range("Function parameter index out of range.");
  return m_iFunctionActive[i];
}

/**
* @param varName The variable name which may contain function index ( [f<index.>]name )
* @param index Receives function index or -1 
* @param name Receives the parameter name
*/
void CompositeFunction::parseName(const std::string& varName,int& index, std::string& name)
{
  size_t i = varName.find('.');
  if (i == std::string::npos)
  {
    name = varName;
    index = -1;
    return;
  }
  else
  {
    if (varName[0] != 'f')
      throw std::invalid_argument("External function parameter name must start with 'f'");

    std::string sindex = varName.substr(1,i-1);
    index = boost::lexical_cast<int>(sindex);

    if (i == varName.size() - 1)
      throw std::invalid_argument("Name cannot be empty");

    name = varName.substr(i+1);
  }
}

/** Initialize the function providing it the workspace
 * @param workspace The shared pointer to a workspace to which the function will be fitted
 * @param spec The number of a spectrum for fitting
 * @param xMin The minimum bin index of spectrum spec that will be used in fitting
 * @param xMax The maximum bin index of spectrum spec that will be used in fitting
 */
void CompositeFunction::setWorkspace(boost::shared_ptr<const DataObjects::Workspace2D> workspace,int spec,int xMin,int xMax)
{
  IFunction::setWorkspace(workspace,spec,xMin,xMax);
  for(int i=0;i<nFunctions();i++)
    getFunction(i)->setWorkspace(workspace,spec,xMin,xMax);
}


} // namespace API
} // namespace Mantid
