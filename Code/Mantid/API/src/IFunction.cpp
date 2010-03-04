//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidGeometry/Instrument/ParameterMap.h"
#include "MantidGeometry/Instrument/Component.h"
#include "MantidGeometry/Instrument/DetectorGroup.h"
#include "MantidGeometry/Instrument/FitParameter.h"
#include "MantidKernel/Exception.h"
#include "MantidAPI/IFunction.h"
#include "MantidAPI/IConstraint.h"
#include "MantidAPI/ParameterTie.h"
#include "MantidAPI/SpectraDetectorMap.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/CompositeFunction.h"

#include <sstream>
#include <iostream>

namespace Mantid
{
namespace API
{

/** Base class implementation of derivative IFunction throws error. This is to check if such a function is provided
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
  throw Kernel::Exception::NotImplementedError("No derivative IFunction provided");
}

/** Initialize the function providing it the workspace
 * @param workspace The workspace to set
 * @param wi The workspace index
 * @param xMin The lower bin index
 * @param xMax The upper bin index
 */
void IFunction::setWorkspace(boost::shared_ptr<const API::MatrixWorkspace> workspace,int wi,int xMin,int xMax)
{
  m_workspace = workspace;
  m_workspaceIndex = wi;
  m_xMinIndex = xMin;
  m_xMaxIndex = xMax;

  // check if parameter are specified in instrument definition file

  Geometry::ParameterMap& paramMap = m_workspace->instrumentParameters();

  // in some tests where workspace a created on the fly a spectra to detector map
  // is for convenience not created. 
  if ( !(m_workspace->spectraMap().nElements()) )  
    return;

  Geometry::IDetector_sptr det = m_workspace->getDetector(wi);

  // if det is a detector groupworkspace then take as the detector
  // the detector returned by det->getID()
  if ( boost::dynamic_pointer_cast<Geometry::DetectorGroup>(det) )
  {
    API::IInstrument_sptr inst = m_workspace->getInstrument();
    det = inst->getDetector(det->getID());
  }

  for (int i = 0; i < nParams(); i++)
  {
    if ( !isExplicitlySet(i) )
    {
      Geometry::Parameter_sptr param = paramMap.getRecursive(&(*det), parameterName(i), "fitting");
      if (param != Geometry::Parameter_sptr())
      {
        // get FitParameter
        const Geometry::FitParameter& fitParam = param->value<Geometry::FitParameter>();

        // check first if this parameter is actually specified for this function
        if ( name().compare(fitParam.getFunction()) == 0 )
        {
          // update value
          setParameter(i, fitParam.getValue());
          // set any tie which may be specified
          if ( fitParam.getTie().compare("") )
          {  
            std::ostringstream str;
            str << fitParam.getValue();
            tie(parameterName(i), str.str());
          }
        }
      }
    }
  }
}

/** Update active parameters. Ties are applied.
 *  @param in Pointer to an array with active parameters values. Must be at least nActive() doubles long.
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
  setParameter(j,value,false);
}

double IFunction::activeParameter(int i)const
{
  int j = indexOfActive(i);
  return getParameter(j);
}

/** Create a new tie. IFunctions can have their own types of ties.
 * @param parName The parameter name for this tie
 */
ParameterTie* IFunction::createTie(const std::string& parName)
{
  return new ParameterTie(this,parName);
}

/**
 * Ties a parameter to other parameters
 * @param parName The name of the parameter to tie.
 * @param expr    A math expression 
 */
void IFunction::tie(const std::string& parName,const std::string& expr)
{
  ParameterTie* tie = this->createTie(parName);
  int i = getParameterIndex(*tie);
  if (i < 0)
  {
    delete tie;
    throw std::logic_error("Parameter "+parName+" was not found.");
  }

  if (!this->isActive(i))
  {
    delete tie;
    throw std::logic_error("Parameter "+parName+" is already tied.");
  }
  tie->set(expr);
  addTie(tie);
  this->removeActive(i);
}

/** Removes the tie off a parameter. The parameter becomes active
 * This method can be used when constructing and editing the IFunction in a GUI
 * @param parName The name of the paramter which ties will be removed.
 */
void IFunction::removeTie(const std::string& parName)
{
  int i = parameterIndex(parName);
  this->removeTie(i);
}

/**
 * Calculate the Jacobian with respect to parameters actually declared in the IFunction
 * @param out The output Jacobian
 * @param xValues The x-values
 * @param nData The number of data points (and x-values).
 */
void IFunction::calJacobianForCovariance(Jacobian* out, const double* xValues, const int& nData)
{
  this->functionDeriv(out,xValues,nData);
}

/**
 * Writes a string that can be used in Fit.IFunction to create a copy of this IFunction
 */
std::string IFunction::asString()const
{
  std::ostringstream ostr;
  ostr << "name="<<this->name();
  std::vector<std::string> attr = this->getAttributeNames();
  for(size_t i=0;i<attr.size();i++)
  {
    ostr<<','<<attr[i]<<'='<<this->getAttribute(attr[i]);
  }
  for(int i=0;i<nParams();i++)
  {
    ostr<<','<<parameterName(i)<<'='<<getParameter(i);
  }
  IConstraint* c = this->firstConstraint();
  if (c)
  {
    ostr << ",constraints=(";
  }
  while(c)
  {
    ostr << c->asString();
    c = this->nextConstraint();
    if (c) 
    {
      ostr << ',';
    }
    else
    {
      ostr << ")";
    }
  }
  std::string ties;
  for(int i=0;i<nParams();i++)
  {
    const ParameterTie* tie = getTie(i);
    if (tie)
    {
      std::string tmp = tie->asString(this);
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
  if (!ties.empty())
  {
    ostr << ",ties=(" << ties << ")";
  }
  return ostr.str();
}

/**
 * Operator <<
 * @param ostr The output stream
 * @param f The IFunction
 */
std::ostream& operator<<(std::ostream& ostr,const IFunction& f)
{
  ostr << f.asString();
  return ostr;
}

} // namespace API
} // namespace Mantid
