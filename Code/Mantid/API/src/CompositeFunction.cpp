//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/CompositeFunction.h"
#include "MantidKernel/Exception.h"

#include <boost/shared_array.hpp>
#include <sstream>
#include <iostream>

namespace Mantid
{
namespace API
{

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
  Jacobian* m_J;///< pointer to the overall Jacobian
  int m_iP0;      ///< offset in the overall Jacobian for a particular function
public:
  /** Constructor
   * @param J A pointer to the overall Jacobian
   */
  PartialJacobian(Jacobian* J,int iP0):m_J(J),m_iP0(iP0){}
  /**
   * Overridden Jacobian::set(...).
   * @param iY The index of the data point
   * @param iP The parameter index of an individual function.
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
    PartialJacobian J(out,m_activeOffsets[i]);
    m_functions[i]->functionDeriv(&J,xValues,nData);
  }
}

/// Derivatives to be used in covariance matrix calculation. 
void CompositeFunction::calJacobianForCovariance(Jacobian* out, const double* xValues, const int& nData)
{
  if (nData <= 0) return;
  for(int i=0;i<nFunctions();i++)
  {
    PartialJacobian J(out,m_activeOffsets[i]);
    m_functions[i]->calJacobianForCovariance(&J,xValues,nData);
  }
}


/// Address of i-th parameter
double& CompositeFunction::parameter(int i)
{
  if (i >= nParams())
    throw std::out_of_range("Function parameter index out of range.");
  int iFun =  m_iFunction[i];
  return m_functions[ iFun ]->parameter(i - m_activeOffsets[iFun]);
}

/// Address of i-th parameter
double CompositeFunction::parameter(int i)const
{
  if (i >= nParams())
    throw std::out_of_range("Function parameter index out of range.");
  int iFun =  m_iFunction[i];
  return m_functions[ iFun ]->parameter(i - m_activeOffsets[iFun]);
}

/// Get parameter by name.
double& CompositeFunction::getParameter(const std::string& name)
{
  throw Kernel::Exception::NotImplementedError("CompositeFunction::getParameter is not implemented");
  return m_tst;
}

/// Get parameter by name.
double CompositeFunction::getParameter(const std::string& name)const
{
  throw Kernel::Exception::NotImplementedError("CompositeFunction::getParameter is not implemented");
  return m_tst;
}

/// Total number of parameters
int CompositeFunction::nParams()const
{
  return m_nParams;
}

/// Returns the name of parameter i
std::string CompositeFunction::parameterName(int i)const
{
  if (i >= nParams())
    throw std::out_of_range("Function parameter index out of range.");
  int iFun =  m_iFunction[i];
  return m_functions[ iFun ]->parameterName(i - m_activeOffsets[iFun]);
}

/// Number of active (in terms of fitting) parameters
int CompositeFunction::nActive()const
{
  return m_nActive;
}

/// Value of i-th active parameter. Override this method to make fitted parameters different from the declared
double CompositeFunction::activeParameter(int i)const
{
  if (i >= nParams())
    throw std::out_of_range("Function parameter index out of range.");
  int iFun =  m_iFunction[i];
  return m_functions[ iFun ]->activeParameter(i - m_activeOffsets[iFun]);
}

/// Set new value of i-th active parameter. Override this method to make fitted parameters different from the declared
void CompositeFunction::setActiveParameter(int i, double value)
{
  if (i >= nParams())
    throw std::out_of_range("Function parameter index out of range.");
  int iFun =  m_iFunction[i];
  return m_functions[ iFun ]->setActiveParameter(i - m_activeOffsets[iFun],value);
}

/// Update parameters after a fitting iteration
void CompositeFunction::updateActive(const double* in)
{
  for(int iFun = 0; iFun < int(m_functions.size()); iFun++)
  {
    m_functions[ iFun ]->updateActive(in + m_activeOffsets[ iFun ]);
  }
}

/// Returns "global" index of active parameter i
int CompositeFunction::indexOfActive(int i)const
{
  if (i >= nParams())
    throw std::out_of_range("Function parameter index out of range.");
  int iFun =  m_iFunction[i];
  return m_paramOffsets[ iFun ] + m_functions[ iFun ]->indexOfActive(i - m_activeOffsets[iFun]);
}

/// Returns the name of active parameter i
std::string CompositeFunction::nameOfActive(int i)const
{
  if (i >= nParams())
    throw std::out_of_range("Function parameter index out of range.");
  int iFun =  m_iFunction[i];
  return m_functions[ iFun ]->nameOfActive(i - m_activeOffsets[iFun]);
}

/** Add a function
 * @param f A pointer to the added function
 */
void CompositeFunction::addFunction(IFunction* f)
{
  m_iFunction.insert(m_iFunction.end(),f->nParams(),m_functions.size());
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

IFunction* CompositeFunction::getFunction(int i)
{
  if ( i >= nFunctions() )
    throw std::out_of_range("Function index out of range.");

  return m_functions[i];
}

} // namespace API
} // namespace Mantid
