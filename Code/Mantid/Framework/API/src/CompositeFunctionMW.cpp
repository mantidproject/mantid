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
  if (name() != "CompositeFunctionMW")
  {
    ostr << "composite=" <<name() << ";";
  }
  for(size_t i=0;i<nFunctions();i++)
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
  for(size_t i=0;i<nParams();i++)
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
  for(size_t i=0;i<nFunctions();i++)
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

/// Derivatives of function with respect to active parameters
void CompositeFunctionMW::functionDeriv(Jacobian* out, const double* xValues, const int& nData)
{
  for(size_t i=0;i<nFunctions();i++)
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
  for(size_t i=0;i<nFunctions();i++)
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
  for(size_t i=0;i<nFunctions();i++)
  {
    IFunctionMW* fun = dynamic_cast<IFunctionMW*>(getFunction(i));
    fun->setMatrixWorkspace(workspace,spec,xMin,xMax);
  }
}

void CompositeFunctionMW::setWorkspace(boost::shared_ptr<const Workspace> ws,const std::string& slicing,bool copyData)
{
  IFunctionMW::setWorkspace(ws,slicing);
  for(size_t iFun=0;iFun<nFunctions();iFun++)
  {
    IFunctionMW* fun = dynamic_cast<IFunctionMW*>(getFunction(iFun));
    //fun->setWorkspace(ws, slicing, copyData); // TODO: This was added by JZ May 13, 2011, to fix tests. Does this make sense to someone who knows?
    fun->setUpNewStuff(m_xValues,m_weights);
  }
}

void CompositeFunctionMW::setUpNewStuff(boost::shared_array<double> xs,boost::shared_array<double> weights)
{
  IFunctionMW::setUpNewStuff(xs,weights);
  for(size_t iFun=0;iFun<nFunctions();iFun++)
  {
    IFunctionMW* fun = dynamic_cast<IFunctionMW*>(getFunction(iFun));
    fun->setUpNewStuff(xs,weights);
  }
}

} // namespace API
} // namespace Mantid
