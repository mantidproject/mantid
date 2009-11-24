#include <muParser.h>
#include "MantidAPI/CompositeFunction.h"
#include "MantidAPI/ParameterTie.h"

namespace Mantid
{
namespace API
{

  /** Constructor
   * @param funct A pointer to the function which parameter will be tied
   * @param parName The name of the parameter to be tied
   */
  ParameterTie::ParameterTie(IFunction* funct,const std::string& parName)
    :m_parser(new mu::Parser()),m_function(funct)
  {
    m_parser->DefineNameChars("0123456789_."
                       "abcdefghijklmnopqrstuvwxyz"
                       "ABCDEFGHIJKLMNOPQRSTUVWXYZ");
    m_parser->SetVarFactory(AddVariable, this);

    m_par = &funct->getParameter(parName);
  }

  /// Destructor
  ParameterTie::~ParameterTie()
  {
    delete m_parser;
  }

  /** Static callback function used by MuParser to initialize variables implicitly
   * @param varName The name of a new variable
   * @param palg Pointer to this ParameterTie
  */
  double* ParameterTie::AddVariable(const char *varName, void *palg)
  {
    ParameterTie& tie = *(ParameterTie*)palg;
    return &(tie.m_function->getParameter(std::string(varName)));
  }

  /**
   * Set tie expression
   * @param expr A math expression
   */
  void ParameterTie::set(const std::string& expr)
  {
    try
    {// Set the expression and initialize the variables
      m_parser->SetExpr(expr);
      m_parser->Eval();
    }
    catch(Kernel::Exception::NotImplementedError&)
    {
      throw std::invalid_argument("Function index was not specified in a parameter name");
    }
    catch(std::exception&)
    {
      throw;
    }
    catch(...)
    {
      throw std::runtime_error("Error in expresseion "+expr);
    }
  }

  double ParameterTie::eval()
  {
    double res = 0;
    try
    {
      res = m_parser->Eval();
    }
    catch(...)
    {
      throw std::runtime_error("Error in expresseion");
    }

    *m_par = res;

    return res;
  }

  /** This method takes a list of double pointers and checks if any of them match
   * to the variables defined in the internal mu::Parser
   * @param pars A list of pointers to check.
   * @return True if any of the pars is used as a variable in the mu::Parser
   */
  bool ParameterTie::findParameters(const std::vector<const double*>& pars)const
  {
    mu::varmap_type vars = m_parser->GetVar();
    for(mu::varmap_type::const_iterator it=vars.begin();it!=vars.end();it++)
    {
      if (std::find(pars.begin(),pars.end(),it->second) != pars.end())
      {
        return true;
      }
    }

    return false;
  }


} // namespace CurveFitting
} // namespace Mantid

