//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidCurveFitting/BoundaryConstraint.h"

#include "MantidKernel/Logger.h"

namespace Mantid
{
namespace CurveFitting
{

//using namespace Kernel;
using namespace API;

// Get a reference to the logger
Kernel::Logger& BoundaryConstraint::g_log = Kernel::Logger::get("BoundaryConstraint");

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

/** determine which is the active parameter. If constraint name is not amoung
 *  the active parameter of function then return -1
 *
 *  @param fn fitting function
 *  @return active parameter index or -1 if no active parameter index found
 */
int BoundaryConstraint::determineParameterIndex(IFunction* fn)
{
  //if (m_activeParameterIndex < 0)
  //{
  int retVal = -1;
  for (int i = 0; i < fn->nActive(); i++)
  {
    if ( m_parameterName.compare(fn->nameOfActive(i)) == 0 )
    {
      retVal = i;
    }
  }
  return retVal;
  //}
}

void BoundaryConstraint::setParamToSatisfyConstraint(API::IFunction* fn)
{
  m_activeParameterIndex = determineParameterIndex(fn);

  if (m_activeParameterIndex < 0)
  {
    g_log.warning() << "Constaint name " << m_parameterName << " is not one of the active parameter"
      << " names of function " << fn->name() << ". Therefore"
      << " this constraint applied to this funtion serves no purpose";
    return;
  }

  if ( !(m_hasLowerBound || m_hasUpperBound) )
  {
    g_log.warning() << "No bounds have been set on BoundaryConstraint for parameter " << m_parameterName << ". Therefore"
      << " this constraint serves no purpose!"; 
    return;
  }

  double paramValue = fn->getParameter(fn->indexOfActive(m_activeParameterIndex));

  if (m_hasLowerBound)
    if ( paramValue < m_lowerBound )
      fn->setParameter(fn->nameOfActive(m_activeParameterIndex),m_lowerBound,false);
  if (m_hasUpperBound)
    if ( paramValue > m_upperBound )
      fn->setParameter(fn->nameOfActive(m_activeParameterIndex),m_upperBound,false);
}


double BoundaryConstraint::check(IFunction* fn)
{
  m_activeParameterIndex = determineParameterIndex(fn);

  if (m_activeParameterIndex < 0)
  {
    g_log.warning() << "Constaint name " << m_parameterName << " is not one of the active parameter"
      << " names of function " << fn->name() << ". Therefore"
      << " this constraint applied to this funtion serves no purpose";
    return 0.0;
  }

  if ( !(m_hasLowerBound || m_hasUpperBound) )
  {
    g_log.warning() << "No bounds have been set on BoundaryConstraint for parameter " << m_parameterName << ". Therefore"
      << " this constraint serves no purpose!"; 
    return 0.0;
  }


  double paramValue = fn->getParameter(fn->indexOfActive(m_activeParameterIndex));

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

  if (m_activeParameterIndex < 0 || !(m_hasLowerBound || m_hasUpperBound))
  {
    // no point in logging any warning here since checkDeriv() will always be called after
    // check() is called 
    return penalty;
  } 

  double paramValue = fn->getParameter(fn->indexOfActive(m_activeParameterIndex));

  if (m_hasLowerBound)
    if ( paramValue < m_lowerBound )
      (*penalty)[m_activeParameterIndex] = -m_penaltyFactor;
  if (m_hasUpperBound)
    if ( paramValue > m_upperBound )
      (*penalty)[m_activeParameterIndex] = m_penaltyFactor;

  return penalty;
}


} // namespace CurveFitting
} // namespace Mantid
