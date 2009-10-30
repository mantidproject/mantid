#include <muParser.h>
#include "MantidAPI/ParameterTie.h"
#include "MantidAPI/CompositeFunction.h"

namespace Mantid
{
namespace API
{

  /** Constructor
   * @param funct A pointer to the function which parameter will be tied
   * @param parName The name of the parameter to be tied
   */
  ParameterTie::ParameterTie(API::IFunction* funct,const std::string& parName)
    :m_parser(new mu::Parser()),m_function(funct)
  {
    m_parser->DefineNameChars("0123456789_."
                       "abcdefghijklmnopqrstuvwxyz"
                       "ABCDEFGHIJKLMNOPQRSTUVWXYZ");
    m_parser->SetVarFactory(AddVariable, this);

    m_iPar = funct->parameterIndex(parName);

    //std::string name;
    //int index;
    //parseName(std::string(parName),index,name);
    //if (index < 0)
    //  m_iPar = funct->parameterIndex(parName);
    //else
    //{
    //  CompositeFunction* fun = dynamic_cast<CompositeFunction*>(m_function);
    //  if (!fun)
    //    throw std::invalid_argument("CompositeFunction expected");
    //  m_iPar = fun->m_paramOffsets[index] + fun->getFunction(index)->parameterIndex(name);
    //}
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

    //std::string name;
    //int index;
    //parseName(std::string(varName),index,name);
    //if (index < 0)
    //  return &(tie.m_function->getParameter(name));
    //else
    //{
    //  CompositeFunction* fun = dynamic_cast<CompositeFunction*>(tie.m_function);
    //  if (!fun)
    //    throw std::invalid_argument("CompositeFunction expected");
    //  return &(fun->getFunction(index)->getParameter(name));
    //}
    //return 0;
  }

  /**
   * Set tie expression
   * @param expr A math expression
   */
  void ParameterTie::set(const std::string& expr)
  {
    try
    {
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

    return res;
  }

} // namespace CurveFitting
} // namespace Mantid

