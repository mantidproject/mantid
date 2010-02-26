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
    :ParameterReference(funct,funct->parameterIndex(parName)),m_parser(new mu::Parser())
  {
    m_parser->DefineNameChars("0123456789_."
                       "abcdefghijklmnopqrstuvwxyz"
                       "ABCDEFGHIJKLMNOPQRSTUVWXYZ");
    m_parser->SetVarFactory(AddVariable, this);
    m_function1 = funct;
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
    double* var = new double;
    *var = 0;
    ParameterReference ref(tie.m_function1,tie.m_function1->parameterIndex(std::string(varName)));
    tie.m_varMap[var] = ref;
    return var;
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
      for(std::map<double*,ParameterReference>::const_iterator it=m_varMap.begin();it!=m_varMap.end();it++)
      {
        *(it->first) = it->second.getParameter();
      }
      res = m_parser->Eval();
    }
    catch(...)
    {
      throw std::runtime_error("Error in expresseion");
    }

//    *m_par = res;
    setParameter(res);

    return res;
  }

  /** This method takes a list of double pointers and checks if any of them match
   * to the variables defined in the internal mu::Parser
   * @param fun A function
   * @return True if any of the parameters is used as a variable in the mu::Parser
   */
  bool ParameterTie::findParametersOf(const IFunction* fun)const
  {
    for(std::map<double*,ParameterReference>::const_iterator it=m_varMap.begin();it!=m_varMap.end();it++)
    {
      if (it->second.getFunction() == fun)
      {
        return true;
      }
    }

    return false;
  }


} // namespace CurveFitting
} // namespace Mantid

