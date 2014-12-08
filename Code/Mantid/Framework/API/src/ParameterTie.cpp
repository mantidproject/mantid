#include "MantidAPI/CompositeFunction.h"
#include "MantidAPI/ParameterTie.h"
#include "MantidGeometry/muParser_Silent.h"

#include <boost/lexical_cast.hpp>
#include <boost/regex.hpp>

namespace Mantid
{
namespace API
{

  /** Constructor
   * @param funct :: A pointer to the function which parameter will be tied
   * @param parName :: The name of the parameter to be tied
   * @param expr :: A mathematical expression for the tie
   * @param isDefault :: Flag to mark as default the value of an object associated with this reference: a tie or a constraint.
   */
  ParameterTie::ParameterTie(IFunction* funct,const std::string& parName,const std::string& expr, bool isDefault)
    :ParameterReference(funct,funct->parameterIndex(parName),isDefault),m_parser(new mu::Parser()),m_function1(funct)
  {
    m_parser->DefineNameChars("0123456789_."
                       "abcdefghijklmnopqrstuvwxyz"
                       "ABCDEFGHIJKLMNOPQRSTUVWXYZ");
    m_parser->SetVarFactory(AddVariable, this);
    if (!expr.empty())
    {
      set(expr);
    }
  }

  /// Destructor
  ParameterTie::~ParameterTie()
  {
    for(std::map<double*,ParameterReference>::const_iterator it=m_varMap.begin();it!=m_varMap.end();++it)
    {
      delete it->first;
    }
    delete m_parser;
  }

  /** Static callback function used by MuParser to initialize variables implicitly
   * @param varName :: The name of a new variable
   * @param palg :: Pointer to this ParameterTie
   * @return pointer to added variable
   */
  double* ParameterTie::AddVariable(const char *varName, void *palg)
  {
    ParameterTie& tie = *(ParameterTie*)palg;
    ParameterReference ref(tie.m_function1,tie.m_function1->parameterIndex(std::string(varName)));

    double* var = new double;
    *var = 0;
    tie.m_varMap[var] = ref;

    return var;
  }

  /**
   * Set tie expression
   * @param expr :: A math expression
   */
  void ParameterTie::set(const std::string& expr)
  {
    for(std::map<double*,ParameterReference>::const_iterator it=m_varMap.begin();it!=m_varMap.end();++it)
    {
      delete it->first;
    }
    if (m_varMap.size())
    {
      m_varMap.clear();
    }
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

    // Create the template m_expression
    boost::regex rx("\\b(([[:alpha:]]|_)([[:alnum:]]|_|\\.)*)\\b(?!(\\s*\\())");
    std::string input = expr;
    boost::smatch res;
    std::string::const_iterator start = input.begin();
    std::string::const_iterator end = input.end();

    std::map<std::string,int> varNames;
    int i = 0;
    for(std::map<double*,ParameterReference>::const_iterator it=m_varMap.begin();it!=m_varMap.end();++it)
    {
      varNames[m_function1->parameterName(m_function1->getParameterIndex(it->second))] = i;
      i++;
    }

    m_expression = "";
    while(boost::regex_search(start,end,res,rx))
    {
      m_expression.append(start,res[0].first);
      m_expression += "#" + boost::lexical_cast<std::string>(varNames[res[1]]);
      start = res[0].second;
    }
    m_expression.append(start,end);
  }

  double ParameterTie::eval()
  {
    double res = 0;
    try
    {
      for(std::map<double*,ParameterReference>::const_iterator it=m_varMap.begin();it!=m_varMap.end();++it)
      {
        *(it->first) = it->second.getParameter();
      }
      res = m_parser->Eval();
    }
    catch(...)
    {
      throw std::runtime_error("Error in expresseion");
    }

    setParameter(res);

    return res;
  }

  /**
   * All parameters in the tie must be parameters of fun.
   * @param fun :: Function that can re-create the tie from the output string.
   * @return string representation of function
   */
  std::string ParameterTie::asString(const IFunction* fun)const
  {
    if (!fun)
    {
      fun = m_function1;
    }
    std::string res_expression;
    try
    {
      res_expression = fun->parameterName(fun->getParameterIndex(*this)) + "=";

      if (m_varMap.size() == 0)
      {// constants
        return res_expression + m_expression;;
      }

      boost::regex rx(std::string("#(\\d+)"));
      boost::smatch res;
      std::string::const_iterator start = m_expression.begin();
      std::string::const_iterator end = m_expression.end();

      while(boost::regex_search(start,end,res,rx))
      {
        res_expression.append(start,res[0].first);

        int iTemp = boost::lexical_cast<int>(res[1]);
        int i = 0;
        for(std::map<double*,ParameterReference>::const_iterator it=m_varMap.begin();it!=m_varMap.end();++it)
        {
          if (i == iTemp)
          {
            res_expression += fun->parameterName(fun->getParameterIndex(it->second));
            break;
          }
          i++;
        }

        start = res[0].second;
      }
      res_expression.append(start,end);
    }
    catch(...)
    {// parameters are not from function fun
      res_expression = "";
    }
    return res_expression;
  }

  /** This method takes a list of double pointers and checks if any of them match
   * to the variables defined in the internal mu::Parser
   * @param fun :: A function
   * @return True if any of the parameters is used as a variable in the mu::Parser
   */
  bool ParameterTie::findParametersOf(const IFunction* fun)const
  {
    for(std::map<double*,ParameterReference>::const_iterator it=m_varMap.begin();it!=m_varMap.end();++it)
    {
      if (it->second.getFunction() == fun)
      {
        return true;
      }
    }

    return false;
  }

  /**
   * Check if the tie is a constant.
   */
  bool ParameterTie::isConstant() const
  {
    return m_varMap.empty();
  }

} // namespace CurveFitting
} // namespace Mantid

