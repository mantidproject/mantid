//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidKernel/Exception.h"
#include "MantidAPI/CompositeFunctionMW.h"
#include "MantidAPI/ParameterTie.h"
#include "MantidAPI/IConstraint.h"

#include <boost/lexical_cast.hpp>
#include <boost/shared_array.hpp>
#include <sstream>
#include <iostream>
#include <algorithm>

namespace Mantid
{
namespace API
{

DECLARE_FUNCTION(CompositeFunctionMW)

/// Copy contructor
CompositeFunctionMW::CompositeFunctionMW(const CompositeFunctionMW& f)
:CompositeFunction(f),IFunctionMW(f)
{
}

///Destructor
CompositeFunctionMW::~CompositeFunctionMW()
{
}

/** 
 * Writes itself into a string. Functions derived from CompositeFunctionMW must
 * override this method with something like this:
 *   std::string NewFunction::asString()const
 *   {
 *      ostr << "composite=" << this->name() << ';';
 *      // write NewFunction's own attributes and parameters
 *      ostr << CompositeFunctionMW::asString();
 *      // write NewFunction's own ties and constraints
 *      // ostr << ";constraints=(" << ... <<")";
 *   }
 * @return the string representation of the composite function
 */
std::string CompositeFunctionMW::asString()const
{
  std::ostringstream ostr;
  for(int i=0;i<nFunctions();i++)
  {
    IFitFunction* fun = getFunction(i);
    bool isComp = dynamic_cast<CompositeFunctionMW*>(fun) != 0;
    if (isComp) ostr << '(';
    ostr << fun->asString();
    if (isComp) ostr << ')';
    if (i < nFunctions() - 1)
    {
      ostr << ';';
    }
  }
  std::string ties;
  for(int i=0;i<nParams();i++)
  {
    const ParameterTie* tie = getTie(i);
    if (tie)
    {
      IFitFunction* fun = getFunction(functionIndex(i));
      std::string tmp = tie->asString(fun);
      if (tmp.empty())
      {
        tmp = tie->asString(this);
        if (!tmp.empty())
        {
          if (!ties.empty())
          {
            ties += ",";
          }
          ties += tmp;
        }
      }
    }
  }
  if (!ties.empty())
  {
    ostr << ";ties=(" << ties << ")";
  }
  return ostr.str();
}

/// Function you want to fit to.
void CompositeFunctionMW::function(double* out, const double* xValues, const int& nData)const
{
  if (nData <= 0) return;
  boost::shared_array<double> tmpOut(new double[nData]);
  for(int i=0;i<nFunctions();i++)
  {
    IFunctionMW* fun = dynamic_cast<IFunctionMW*>(getFunction(i));
    if (i == 0)
      fun->function(out,xValues,nData);
    else
    {
      fun->function(tmpOut.get(),xValues,nData);
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
  int m_iaP0;      ///< offset in the active Jacobian for a particular function
public:
  /** Constructor
   * @param J :: A pointer to the overall Jacobian
   * @param iP0 :: The parameter index (declared) offset for a particular function
   * @param iap0 :: The active parameter index (declared) offset for a particular function
   */
  PartialJacobian(Jacobian* J,int iP0, int iap0):m_J(J),m_iP0(iP0),m_iaP0(iap0)
  {}
  /**
   * Overridden Jacobian::set(...).
   * @param iY :: The index of the data point
   * @param iP :: The parameter index of an individual function.
   * @param value :: The derivative value
   */
  void set(int iY, int iP, double value)
  {
      m_J->set(iY,m_iP0 + iP,value);
  }
  /**
   * Overridden Jacobian::get(...).
   * @param iY :: The index of the data point
   * @param iP :: The parameter index of an individual function.
   */
  double get(int iY, int iP)
  {
      return m_J->get(iY,m_iP0 + iP);
  }
 /**  Add number to all iY (data) Jacobian elements for a given iP (parameter)
  *   @param value :: Value to add
  *   @param iActiveP :: The index of an active parameter.
  */
  virtual void addNumberToColumn(const double& value, const int& iActiveP) 
  {
    m_J->addNumberToColumn(value,m_iaP0+iActiveP);
  }
};

/// Derivatives of function with respect to active parameters
void CompositeFunctionMW::functionDeriv(Jacobian* out, const double* xValues, const int& nData)
{
  for(int i=0;i<nFunctions();i++)
  {
    PartialJacobian J(out,paramOffset(i),activeOffset(i));
    IFunctionMW* fun = dynamic_cast<IFunctionMW*>(getFunction(i));
    fun->functionDeriv(&J,xValues,nData);
  }
}

/// Derivatives to be used in covariance matrix calculation. 
void CompositeFunctionMW::calJacobianForCovariance(Jacobian* out, const double* xValues, const int& nData)
{
  if (nData <= 0) return;
  for(int i=0;i<nFunctions();i++)
  {
    PartialJacobian J(out,paramOffset(i),activeOffset(i));
    IFunctionMW* fun = dynamic_cast<IFunctionMW*>(getFunction(i));
    fun->calJacobianForCovariance(&J,xValues,nData);
  }
}

/** Initialize the function providing it the workspace
 * @param workspace :: The shared pointer to a workspace to which the function will be fitted
 * @param spec :: The number of a spectrum for fitting
 * @param xMin :: The minimum bin index of spectrum spec that will be used in fitting
 * @param xMax :: The maximum bin index of spectrum spec that will be used in fitting
 */
void CompositeFunctionMW::setMatrixWorkspace(boost::shared_ptr<const API::MatrixWorkspace> workspace,int spec,int xMin,int xMax)
{
  IFunctionMW::setMatrixWorkspace(workspace,spec,xMin,xMax);
  for(int i=0;i<nFunctions();i++)
  {
    IFunctionMW* fun = dynamic_cast<IFunctionMW*>(getFunction(i));
    fun->setMatrixWorkspace(workspace,spec,xMin,xMax);
  }
}

void CompositeFunctionMW::setWorkspace(boost::shared_ptr<Workspace> ws,const std::string& slicing)
{
  IFunctionMW::setWorkspace(ws,slicing);
  for(int iFun=0;iFun<nFunctions();iFun++)
  {
    IFunctionMW* fun = dynamic_cast<IFunctionMW*>(getFunction(iFun));
    fun->setUpNewStuff(m_xValues,m_weights);
  }
}

void CompositeFunctionMW::setUpNewStuff(boost::shared_array<double> xs,boost::shared_array<double> weights)
{
  IFunctionMW::setUpNewStuff(xs,weights);
  for(int iFun=0;iFun<nFunctions();iFun++)
  {
    IFunctionMW* fun = dynamic_cast<IFunctionMW*>(getFunction(iFun));
    fun->setUpNewStuff(xs,weights);
  }
}

} // namespace API
} // namespace Mantid
