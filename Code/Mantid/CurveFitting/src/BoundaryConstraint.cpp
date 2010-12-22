//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidCurveFitting/BoundaryConstraint.h"
#include "MantidAPI/Expression.h"
#include "MantidAPI/ConstraintFactory.h"
#include "MantidKernel/Logger.h"
#include <boost/lexical_cast.hpp>
#include <sstream>

namespace Mantid
{
namespace CurveFitting
{

DECLARE_CONSTRAINT(BoundaryConstraint)

//using namespace Kernel;
using namespace API;

// Get a reference to the logger
Kernel::Logger& BoundaryConstraint::g_log = Kernel::Logger::get("BoundaryConstraint");

/** Constructor with boundary arguments
 * @param fun The function
 * @param paramName The parameter name
 * @param lowerBound The lower bound
 * @param upperBound The upper bound
 */
BoundaryConstraint::BoundaryConstraint(API::IFitFunction* fun, const std::string paramName, const double lowerBound, const double upperBound) : 
m_activeParameterIndex(-1),
m_penaltyFactor(1000.0),
m_parameterName(paramName),
m_hasLowerBound( true), 
m_hasUpperBound( true),    
m_lowerBound(lowerBound), 
m_upperBound(upperBound)
{
  reset(fun,fun->parameterIndex(paramName)),
  m_activeParameterIndex = fun->activeIndex(getIndex());
}

/** Initialize the constraint from an expression.
 * @param fun The function
 * @param expr The initializing expression which must look like this:
 * " 10 < Sigma < 20 " or
 * " Sigma > 20 "
 */
void BoundaryConstraint::initialize(API::IFitFunction* fun, const API::Expression& expr)
{
  if ( expr.size() < 2 || expr.name() != "==")
  {
    g_log.error("Wrong initialization expression");
    throw std::invalid_argument("Wrong initialization expression");
  }
  clearBounds();
  const Expression& terms(expr);

  std::vector<double> values(3);
  int ilow = -1;
  int ihi = -1;
  std::string parName;
  for(int i=0;i<terms.size();i++)
  {
    std::string name = terms[i].str();
    try
    {
      double d = boost::lexical_cast<double>(name);
      values[i] = d;
      std::string op = terms[i].operator_name();
      if (op.empty())
      {
        op = terms[i+1].operator_name();
        if (op[0] == '<')
        {
          ilow = i;
        }
        else if (op[0] == '>')
        {
          ihi = i;
        }
        else
        {
          g_log.error("Unknown operator in initialization expression");
          throw std::invalid_argument("Unknown operator in initialization expression");
        }
      }// if empty
      else
      {
        if (op[0] == '<')
        {
          ihi = i;
        }
        else if (op[0] == '>')
        {
          ilow = i;
        }
        else
        {
          g_log.error("Unknown operator in initialization expression");
          throw std::invalid_argument("Unknown operator in initialization expression");
        }
      }// if not empty
    }
    catch(boost::bad_lexical_cast)
    {
      if ( !parName.empty() )
      {
        g_log.error("Non-numeric value for a bound");
        throw std::invalid_argument("Non-numeric value for a bound");
      }
      parName = name;
    }
  }// for i 

  try
  {
    int i = fun->parameterIndex(parName);
    reset(fun,i);
    m_parameterName = parName;
  }
  catch(...)
  {
    g_log.error()<<"Parameter "<<parName<<" not found in function "<<fun->name()<<'\n';
    throw;
  }

  m_activeParameterIndex = getFunction()->activeIndex(getIndex());

  if (m_activeParameterIndex < 0)
  {
    std::ostringstream msg;
     msg << "Constaint name " << m_parameterName << " is not one of the active parameter"
      << " names of function " << fun->name() << ". Therefore"
      << " this constraint applied to this funtion serves no purpose";
    g_log.error(msg.str());
    throw std::runtime_error(msg.str());
  }

  if (ilow >= 0)
  {
    setLower(values[ilow]);
  }
  if (ihi >= 0)
  {
    setUpper(values[ihi]);
  }

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

/* determine which is the active parameter. If constraint name is not amoung
 *  the active parameter of function then return -1
 *
 *  @param fn fitting function
 *  @return active parameter index or -1 if no active parameter index found
 */
//int BoundaryConstraint::determineParameterIndex(IFitFunction* fn)
//{
//  //if (m_activeParameterIndex < 0)
//  //{
//  int retVal = -1;
//  for (int i = 0; i < fn->nActive(); i++)
//  {
//    if ( m_parameterName.compare(fn->nameOfActive(i)) == 0 )
//    {
//      retVal = i;
//    }
//  }
//  return retVal;
//  //}
//}

void BoundaryConstraint::setParamToSatisfyConstraint()
{
  m_activeParameterIndex = getFunction()->activeIndex(getIndex());

  if (m_activeParameterIndex < 0)
  {
    g_log.warning() << "Constaint name " << m_parameterName << " is not one of the active parameter"
      << " names of function " << getFunction()->name() << ". Therefore"
      << " this constraint applied to this funtion serves no purpose";
    return;
  }

  if ( !(m_hasLowerBound || m_hasUpperBound) )
  {
    g_log.warning() << "No bounds have been set on BoundaryConstraint for parameter " << m_parameterName << ". Therefore"
      << " this constraint serves no purpose!"; 
    return;
  }

  double paramValue = getFunction()->getParameter(getIndex());

  if (m_hasLowerBound)
    if ( paramValue < m_lowerBound )
      getFunction()->setParameter(getIndex(),m_lowerBound,false);
  if (m_hasUpperBound)
    if ( paramValue > m_upperBound )
      getFunction()->setParameter(getIndex(),m_upperBound,false);
}


double BoundaryConstraint::check()
{
  m_activeParameterIndex = getFunction()->activeIndex(getIndex());

  if (m_activeParameterIndex < 0)
  {
    g_log.warning() << "Constaint name " << m_parameterName << " is not one of the active parameter"
      << " names of function " << getFunction()->name() << ". Therefore"
      << " this constraint applied to this funtion serves no purpose";
    return 0.0;
  }

  if ( !(m_hasLowerBound || m_hasUpperBound) )
  {
    g_log.warning() << "No bounds have been set on BoundaryConstraint for parameter " << m_parameterName << ". Therefore"
      << " this constraint serves no purpose!"; 
    return 0.0;
  }


  double paramValue = getFunction()->getParameter(getIndex());

  double penalty = 0.0;

  if (m_hasLowerBound)
    if ( paramValue < m_lowerBound )
      penalty = (m_lowerBound-paramValue)* m_penaltyFactor;
  if (m_hasUpperBound)
    if ( paramValue > m_upperBound )
      penalty = (paramValue-m_upperBound)* m_penaltyFactor;

  return penalty;
}

double BoundaryConstraint::checkDeriv()
{
  double penalty = 0.0;

  if (m_activeParameterIndex < 0 || !(m_hasLowerBound || m_hasUpperBound))
  {
    // no point in logging any warning here since checkDeriv() will always be called after
    // check() is called 
    return penalty;
  } 

  double paramValue = getFunction()->getParameter(getIndex());

  if (m_hasLowerBound)
    if ( paramValue < m_lowerBound )
      penalty = -m_penaltyFactor;
  if (m_hasUpperBound)
    if ( paramValue > m_upperBound )
      penalty = m_penaltyFactor;

  return penalty;
}

std::string BoundaryConstraint::asString()const
{
  std::ostringstream ostr;
  if (m_hasLowerBound)
  {
    ostr << m_lowerBound << '<';
  }
  ostr << getFunction()->parameterName(getIndex());
  if (m_hasUpperBound)
  {
    ostr<< '<' << m_upperBound;
  }
  return ostr.str();
}

} // namespace CurveFitting
} // namespace Mantid
