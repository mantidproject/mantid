//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/IFitFunction.h"
#include "MantidAPI/Jacobian.h"
#include "MantidAPI/IConstraint.h"
#include "MantidAPI/ParameterTie.h"

#include "MantidKernel/Exception.h"
#include "MantidKernel/Logger.h"
#include "MantidKernel/IPropertyManager.h"

#include <boost/lexical_cast.hpp>

#include <sstream>
#include <iostream> 

namespace Mantid
{
namespace API
{
  
/** Base class implementation of derivative IFitFunction throws error. This is to check if such a function is provided
    by derivative class. In the derived classes this method must return the derivatives of the function
    with respect to the fit parameters. If this method is not reimplemented the derivative free simplex minimization
    algorithm is used or the derivatives are computed numerically.
 * @param out :: Pointer to a Jacobian matrix. If it is NULL the method is called in order to check whether it's implemented or not.
 *      If the derivatives are implemented the method must simply return, otherwise it must throw Kernel::Exception::NotImplementedError.
 */
void IFitFunction::functionDeriv(Jacobian* out)
{
  UNUSED_ARG(out);
  throw ("No derivative IFitFunction provided");
}


/**
  * If any of the parameters do not satisfy a constraint penalty values will be added to the function output.
  * This method is called by Fit algorithm after calling function(double*out)
  * @param out :: The output form function(double* out) to which the penalty will be added.
  */
void IFitFunction::addPenalty(double *out)const
{
    double penalty = 0.;
    for(size_t i=0;i<nParams();++i)
    {
      API::IConstraint* c = getConstraint(i);
      if (c)
      {
        penalty += c->check();
      }
    }

    size_t n = dataSize() - 1;
    // add penalty to first and last point and every 10th point in between
    if ( penalty != 0.0 )
    {
      out[0] += penalty;
      out[n] += penalty;

      for (size_t i = 9; i < n; i+=10)
      {
        out[i] += penalty;
      }
    }
}

/**
  * If a penalty was added to the function output the derivatives are modified accordingly.
  * This method is called by Fit algorithm after calling functionDeriv(Jacobian *out)
  * @param out :: The Jacobian to be corrected
  */
void IFitFunction::addPenaltyDeriv(Jacobian *out)const
{
  size_t n = dataSize() - 1;
  for(size_t i=0;i<nParams();++i)
  {  
    API::IConstraint* c = getConstraint(i);
    if (c)
    {
      double penalty = c->checkDeriv();
      if ( penalty != 0.0 )
      {
        size_t ia = activeIndex(i);
        double deriv = out->get(0,ia);
        out->set(0,ia,deriv + penalty);
        deriv = out->get(n,ia);
        out->set(n,ia,deriv+penalty);

        for (size_t j = 9; j < n; j+=10)
        {
          deriv = out->get(j,ia);
          out->set(j,ia,deriv+penalty);
        }
      }
    } // if (c)
  }

}

/**
 * Operator \<\<
 * @param ostr :: The output stream
 * @param f :: The IFitFunction
 */
std::ostream& operator<<(std::ostream& ostr,const IFitFunction& f)
{
  ostr << f.asString();
  return ostr;
}

} // namespace API
} // namespace Mantid

///\cond TEMPLATE
namespace Mantid
{
namespace Kernel
{

template<> MANTID_API_DLL
boost::shared_ptr<Mantid::API::IFitFunction> IPropertyManager::getValue<boost::shared_ptr<Mantid::API::IFitFunction> >(const std::string &name) const
{
  PropertyWithValue<boost::shared_ptr<Mantid::API::IFitFunction> >* prop =
                    dynamic_cast<PropertyWithValue<boost::shared_ptr<Mantid::API::IFitFunction> >*>(getPointerToProperty(name));
  if (prop)
  {
    return *prop;
  }
  else
  {
    std::string message = "Attempt to assign property "+ name +" to incorrect type. Expected IFitFunction.";
    throw std::runtime_error(message);
  }
}

} // namespace Kernel
} // namespace Mantid
///\endcond TEMPLATE
