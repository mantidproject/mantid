//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidCurveFitting/BoundaryConstraint.h"

#include "MantidKernel/Logger.h"
//#include <boost/tokenizer.hpp>
//#include <algorithm>

namespace Mantid
{
namespace CurveFitting
{

//using namespace Kernel;
using namespace API;

// Get a reference to the logger
Kernel::Logger& BoundaryConstraint::g_log = Kernel::Logger::get("BoundaryConstraint");


/** Check to see if constraint is valid with respect to a given fitting function, this
 *  means for now that the parameter name which have been specified for the constraint
 *  is also one of the active parameters of the fitting function
 *
 *  @param fn fitting function
 *  @return true if name of constraint is also an active fitting param
 */
bool BoundaryConstraint::isValid(API::IFunction* fn)
{
  for (int i = 0; i < fn->nActive(); i++)
  {
    if ( m_parameterName.compare(fn->nameOfActive(i)) == 0 )  
      return true;
  }
  return false;
}


/** Set penalty factor
 *
 *  @param c penalty factor
 */
void BoundaryConstraint::setPenaltyFactor(const double& c) 
{ 
  if (c <= 0.0)
  {
    g_log.warning() << "Penalty factor <= 0 selected for boundary constraint." 
      << " Only positive penalty factor allowed. Penalty factor set to 1";
    m_penaltyFactor = 1;
  }
  {
    m_penaltyFactor = c;
  }
}

/** instantiate m_activeParameterIndex if not already instantiated
 *
 *  @param fn fitting function
 */
void BoundaryConstraint::instantiateParameterIndex(IFunction* fn)
{
  if (m_activeParameterIndex < 0)
  {
    for (int i = 0; i < fn->nActive(); i++)
    {
      if ( m_parameterName.compare(fn->nameOfActive(i)) == 0 )
      {
        m_activeParameterIndex = i;
      }
    }
  }
}

double BoundaryConstraint::check(IFunction* fn)
{
  instantiateParameterIndex(fn);

  double paramValue = fn->parameter(fn->indexOfActive(m_activeParameterIndex));

  double penalty = 0.0;

  if (m_hasLowerBound)
    if ( paramValue < m_lowerBound )
      penalty = (m_lowerBound-paramValue)* m_penaltyFactor;
  if (m_hasUpperBound)
    if ( paramValue > m_upperBound )
      penalty = (paramValue-m_upperBound)* m_penaltyFactor;

  return penalty;
}

boost::shared_ptr<std::vector<double> > BoundaryConstraint::checkDeriv(IFunction* fn)
{
  boost::shared_ptr<std::vector<double> > penalty(new std::vector<double>(fn->nActive(),0.0)); 

  double paramValue = fn->parameter(fn->indexOfActive(m_activeParameterIndex));

  if (m_hasLowerBound)
    if ( paramValue < m_lowerBound )
      (*penalty)[m_activeParameterIndex] = - m_penaltyFactor;
  if (m_hasUpperBound)
    if ( paramValue > m_upperBound )
      (*penalty)[m_activeParameterIndex] = m_penaltyFactor;

  return penalty;
}


} // namespace CurveFitting
} // namespace Mantid
