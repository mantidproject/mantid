//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidCurveFitting/CostFuncRwp.h"
#include "MantidCurveFitting/Jacobian.h"
#include "MantidCurveFitting/SeqDomain.h"
#include "MantidAPI/IConstraint.h"
#include "MantidAPI/CompositeDomain.h"
#include "MantidAPI/FunctionValues.h"

#include <iomanip>

namespace
{
  const bool debug = false;
}

namespace Mantid
{
namespace CurveFitting
{

DECLARE_COSTFUNCTION(CostFuncRwp,Rwp)

/**
 * Constructor
 */
CostFuncRwp::CostFuncRwp() : CostFuncFitting(),m_value(0),m_pushed(false),
  m_log(Kernel::Logger::get("CostFuncRwp")) {}

//----------------------------------------------------------------------
/** Calculate value of cost function
 *  @return :: The value of the function
 */
double CostFuncRwp::val() const
{
  if ( !m_dirtyVal ) return m_value;

  // Check function (m_function) is set properly
  checkValidity();

  m_value = 0.0;

  auto seqDomain = boost::dynamic_pointer_cast<SeqDomain>(m_domain);

  if (seqDomain)
  {
    // m_domain is a 'SeqDomain'
    seqDomain->rwpVal(*this);
  }
  else
  {
    // m_domain is NOT a 'SeqDomain':calculate Rwp inside
    auto simpleValues = boost::dynamic_pointer_cast<API::FunctionValues>(m_values);
    if (!simpleValues)
    {
      throw std::runtime_error("LeastSquares: unsupported IFunctionValues.");
    }
    addVal(m_domain,simpleValues);
  }

#if 0
  // add penalty
  for(size_t i=0;i<m_function->nParams();++i)
  {
    if ( !m_function->isActive(i) ) continue;
    API::IConstraint* c = m_function->getConstraint(i);
    if (c)
    {
      m_value += c->check();
    }
  }
#else
  // There is no PENALTY in Rwp
#endif

  m_dirtyVal = false;
  return m_value;
}

//----------------------------------------------------------------------
/**
 * Add a contribution to the cost function value from the fitting function evaluated on a particular domain.
 * @param domain :: A domain 
 * @param values :: Values
 */
void CostFuncRwp::addVal(API::FunctionDomain_sptr domain, API::FunctionValues_sptr values)const
{
  m_function->function(*domain,*values);
  size_t ny = values->size();

  double retVal = 0.0;
  double denVal = 0.0;

  // FIXME : This might give a wrong answer in case of multiple-domain
  for (size_t i = i; i < ny; ++i)
  {
    double obsval = values->getFitData(i);
    double calval = values->getCalculated(i);
    double weight = values->getFitWeight(i);
    double val = calval - obsval;
    retVal += val * val * weight;
    denVal += obsval * obsval * weight;
  }
  
  PARALLEL_ATOMIC
  m_value += retVal/denVal;

  return;
}

//----------------------------------------------------------------------
/** Calculate the derivatives of the cost function
 * @param der :: Container to output the derivatives
 */
void CostFuncRwp::deriv(std::vector<double>& der) const
{
  valDerivHessian(false,true,false);

  if (der.size() != nParams())
  {
    der.resize(nParams());
  }
  for(size_t i = 0; i < nParams(); ++i)
  {
    der[i] = m_der.get(i);
  }
}

//----------------------------------------------------------------------
/** Calculate the value and the derivatives of the cost function
 * @param der :: Container to output the derivatives
 * @return :: The value of the function
 */
double CostFuncRwp::valAndDeriv(std::vector<double>& der) const
{
  valDerivHessian(true,true,false);

  if (der.size() != nParams())
  {
    der.resize(nParams());
  }
  for(size_t i = 0; i < nParams(); ++i)
  {
    der[i] = m_der.get(i);
  }
  return m_value;
}

//----------------------------------------------------------------------
/** Calculate the value and the first and second derivatives of the cost function
 *  @param evalFunction :: If false cost function isn't evaluated and returned value (0.0) should be ignored.
 *    It is for efficiency reasons.
 *  @param evalDeriv :: flag for evaluation of the first derivatives
 *  @param evalHessian :: flag for evaluation of the second derivatives
 */
double CostFuncRwp::valDerivHessian(bool evalFunction, bool evalDeriv, bool evalHessian) const
{
  // No need to evaluate derivative: Calculate value and return
  if (m_pushed || !evalDeriv)
  {
    return val();
  }

  // Val, Deriv and Hessian are all calculated: Return
  if (!m_dirtyVal && !m_dirtyDeriv && !m_dirtyHessian)
    return m_value;

  // Set the flag whether to evaluate function
  if (m_dirtyVal) evalFunction = true;

  // Initialize some variables
  checkValidity();

  if (evalFunction)
  {
    m_value = 0.0;
  }
  if (evalDeriv)
  {
    m_der.resize(nParams());
    m_der.zero();
  }
  if (evalHessian)
  {
    m_hessian.resize(nParams(),nParams());
    m_hessian.zero();
  }

  auto seqDomain = boost::dynamic_pointer_cast<SeqDomain>(m_domain);

  if (seqDomain)
  {
    // SeqDomain:
    seqDomain->rwpValDerivHessian(*this,evalFunction,evalDeriv,evalHessian);
  }
  else
  {
    // Non-SeqDomain: calculate Hessian in this class
    auto simpleValues = boost::dynamic_pointer_cast<API::FunctionValues>(m_values);
    if (!simpleValues)
    {
      throw std::runtime_error("LeastSquares: unsupported IFunctionValues.");
    }
    addValDerivHessian(m_function,m_domain,simpleValues,evalFunction,evalDeriv,evalHessian);
  }

  // Add values/derivatives/Hessians from constraint: NOT IN RWP!
  size_t np = m_function->nParams();
#if 0
  // Add constraints penalty
  size_t np = m_function->nParams();
  if (evalFunction)
  {
    for(size_t i = 0; i < np; ++i)
    {
      API::IConstraint* c = m_function->getConstraint(i);
      if (c)
      {
        m_value += c->check();
      }
    }
    m_dirtyVal = false;
  }
#else
  // No constraint penalty
  if (evalFunction)
  {
    m_dirtyVal = false;
  }
#endif

  if (evalDeriv)
  {
    // Derivative (vector)
    size_t i = 0;
    for(size_t ip = 0; ip < np; ++ip)
    {
      if ( !m_function->isActive(ip) )
        continue;
#if 0
      API::IConstraint* c = m_function->getConstraint(ip);
      if (c)
      {
        double d =  m_der.get(i) + c->checkDeriv();
        m_der.set(i,d);
      }
      ++i;
#else
      // TODO - Verify that there is no need to do this step.
#endif
    }
    m_dirtyDeriv = false;
  }

  if (evalDeriv)
  {
    // Herssian (matrix)
    size_t i = 0;
    for(size_t ip = 0; ip < np; ++ip)
    {
      if ( !m_function->isActive(ip) )
        continue;
#if 0
      API::IConstraint* c = m_function->getConstraint(ip);
      if (c)
      {
        double d =  m_hessian.get(i,i) + c->checkDeriv2();
        m_hessian.set(i,i,d);
      }
#else
      // TODO - Verify that there is no need to add constraint's derivative to Hessian
#endif
      ++i;
    }
    // clear the dirty flag if hessian was actually calculated
    m_dirtyHessian = m_hessian.isEmpty();
  }

  return m_value;
}

//----------------------------------------------------------------------
/**
 * Update the cost function, derivatives and hessian by adding values calculated
 * on a domain.
 * @param function :: Function to use to calculate the value and the derivatives
 * @param domain :: The domain.
 * @param values :: The fit function values
 * @param evalFunction :: Flag to evaluate the function
 * @param evalDeriv :: Flag to evaluate the derivatives
 * @param evalHessian :: Flag to evaluate the Hessian
 */
void CostFuncRwp::addValDerivHessian(
    API::IFunction_sptr function,
    API::FunctionDomain_sptr domain,
    API::FunctionValues_sptr values,
    bool evalFunction , bool evalDeriv, bool evalHessian) const
{
  UNUSED_ARG(evalDeriv);

  // Initialize some variables
  size_t nparm = function->nParams();  // number of parameters
  size_t ndata = domain->size(); // number of data points
  Jacobian jacobian(ndata,nparm);

  // Evalulate function & its derivative
  if (evalFunction)
  {
    function->function(*domain,*values);
  }
  function->functionDeriv(*domain,jacobian);

#if 0
  // Out b/c no debug
  if (debug)
  {
    std::cerr << "Jacobian:\n";
    for(size_t i = 0; i < ny; ++i)
    {
      for(size_t ip = 0; ip < np; ++ip)
      {
        if ( !m_function->isActive(ip) ) continue;
        std::cerr << jacobian.get(i,ip) << ' ';
      }
      std::cerr << std::endl;
    }
  }
#endif

  size_t iActiveP = 0;
  double fVal = 0.0;
  double denValue = 0.0;

  for(size_t ip = 0; ip < nparm; ++ip)
  {
    if ( !function->isActive(ip) )
      continue;

    double d = 0.0;
    for(size_t id = 0; id < ndata; ++id)
    {
      double calc = values->getCalculated(id);
      double obs = values->getFitData(id);
      double w = values->getFitWeight(id);
#if 1
      double y = ( calc - obs ) * w;
      d += y * jacobian.get(id,ip) * w;
      if (iActiveP == 0 && evalFunction)
      {
        fVal += y * y;
      }
#else
      // FIXME - Need to check with Roman whether it is correct about "d"
      double diff = calc - obs;
      double y = (calc-obs) * w ;
      d += y * jacobian.get(id, ip) * w;
      if (iActiveP == 0 && evalFunction)
      {
        fVal += diff * diff * w;
        denValue += obs * obs * w;
      }
#endif
    }
    PARALLEL_CRITICAL(der_set)
    {
      double der = m_der.get(iActiveP);
      m_der.set(iActiveP, der + d);
    }
    //std::cerr << "der " << ip << ' ' << der[iActiveP] << std::endl;
    ++iActiveP;
  } // ENDFOR (iparameter)

  if (evalFunction)
  {
    PARALLEL_ATOMIC
    m_value += fVal;
    throw std::runtime_error("Does it look right? ");
  }

  if (evalHessian)
  {
    // Hessian is an (nparam x nparam) matrix
    size_t i1 = 0; // active parameter index
    for(size_t i = 0; i < nparm; ++i) // over parameters
    {
      if ( !function->isActive(i) )
        continue;

      size_t i2 = 0; // active parameter index
      for(size_t j = 0; j <= i; ++j) // over ~ half of parameters
      {
        if ( !function->isActive(j) )
          continue;

        // Both parameter (i) and (j) are active.
        double d = 0.0;
        for(size_t k = 0; k < ndata; ++k) // over fitting data
        {
          double w = values->getFitWeight(k);
#if 1
          d += jacobian.get(k,i) * jacobian.get(k,j) * w * w;
#else
          // FIXME/TODO - Dig out the formular for Rwp
          throw std::runtime_error("Implement by formular of Rwp ASAP!");
          d += DBL_MAX;
#endif
        }
        PARALLEL_CRITICAL(hessian_set)
        {
          double h = m_hessian.get(i1,i2);
          m_hessian.set(i1,i2, h + d);
          if (i1 != i2)
          {
            m_hessian.set(i2,i1,h + d);
          }
        }
        ++i2;
      }
      ++i1;
    }

  } // Evalue Hessian

  return;
}

//----------------------------------------------------------------------
/**
 * Return cached or calculate the drivatives.
 */
const GSLVector& CostFuncRwp::getDeriv() const
{
  if (m_pushed)
  {
    return m_der;
  }
  if (m_dirtyVal || m_dirtyDeriv || m_dirtyHessian)
  {
    valDerivHessian();
  }
  return m_der;
}

//----------------------------------------------------------------------
/**
 * Return cached or calculate the Hessian.
 */
const GSLMatrix& CostFuncRwp::getHessian() const
{
  if (m_pushed)
  {
    return m_hessian;
  }
  if (m_dirtyVal || m_dirtyDeriv || m_dirtyHessian)
  {
    valDerivHessian();
  }
  return m_hessian;
}

//----------------------------------------------------------------------
/**
 * Save current parameters, derivatives and hessian.
 */
void CostFuncRwp::push()
{
  if (m_pushed)
  {
    throw std::runtime_error("Least squares: double push.");
  }
  // make sure we are not dirty
  m_pushedValue = valDerivHessian();
  getParameters(m_pushedParams);
  m_pushed = true;
}

//----------------------------------------------------------------------
/**
 * Restore saved parameters, derivatives and hessian.
 */
void CostFuncRwp::pop()
{
  if ( !m_pushed )
  {
    throw std::runtime_error("Least squares: empty stack.");
  }
  setParameters(m_pushedParams);
  m_value = m_pushedValue;
  m_pushed = false;
  m_dirtyVal = false;
  m_dirtyDeriv = false;
  m_dirtyHessian = false;
}

//----------------------------------------------------------------------
/**
 * Discard saved parameters, derivatives and hessian.
 */
void CostFuncRwp::drop()
{
  if ( !m_pushed )
  {
    throw std::runtime_error("Least squares: empty stack.");
  }
  m_pushed = false;
  setDirty();
}

//----------------------------------------------------------------------
/**
 * Copy the parameter values from a GSLVector.
 * @param params :: A vector to copy the parameters from
 */
void CostFuncRwp::setParameters(const GSLVector& params)
{
  if (nParams() != params.size())
  {
    throw std::runtime_error("Parameter vector has wrong size in CostFuncRwp.");
  }
  for(size_t i = 0; i < nParams(); ++i)
  {
    setParameter(i,params.get(i));
  }
  m_function->applyTies();
}

//----------------------------------------------------------------------
/**
 * Copy the parameter values to a GSLVector.
 * @param params :: A vector to copy the parameters to
 */
void CostFuncRwp::getParameters(GSLVector& params) const
{
  if (params.size() != nParams())
  {
    params.resize(nParams());
  }
  for(size_t i = 0; i < nParams(); ++i)
  {
    params.set(i,getParameter(i));
  }
}

//----------------------------------------------------------------------
/**
  * Calculates covariance matrix for fitting function's active parameters. 
  * @param covar :: Output cavariance matrix.
  * @param epsrel :: Tolerance.
  */
void CostFuncRwp::calActiveCovarianceMatrix(GSLMatrix& covar, double epsrel)
{
  UNUSED_ARG(epsrel);

  if (m_hessian.isEmpty())
  {
    valDerivHessian();
  }

  // Output Hessian related information to LOG
  if(m_log.is(Kernel::Logger::Priority::PRIO_INFORMATION))
  {
    m_log.information() << "== Hessian (H) ==\n";
    std::ios::fmtflags prevState = m_log.information().flags();
    m_log.information() << std::left << std::fixed;
    for(size_t i = 0; i < m_hessian.size1(); ++i)
    {
      for(size_t j = 0; j < m_hessian.size2(); ++j)
      {
        m_log.information() << std::setw(10);
        m_log.information() << m_hessian.get(i,j) << "  ";
      }
      m_log.information() << "\n";
    }
    m_log.information().flags(prevState);
  }

  // Calculate covaraince matrix: covariance matrix is the inverse of Hessian
  covar = m_hessian;
  covar.invert();
  // Output covariance matrix
  if(m_log.is(Kernel::Logger::Priority::PRIO_INFORMATION))
  {
    m_log.information() << "== Covariance matrix (H^-1) ==\n";
    std::ios::fmtflags prevState = m_log.information().flags();
    m_log.information() << std::left << std::fixed;
    for(size_t i = 0; i < covar.size1(); ++i)
    {
      for(size_t j = 0; j < covar.size2(); ++j)
      {
        m_log.information() << std::setw(10);
        m_log.information() << covar.get(i,j) << "  ";
      }
      m_log.information() << "\n";
    }
    m_log.information().flags(prevState);
  }

  return;
}

} // namespace CurveFitting
} // namespace Mantid
