#include "MantidAPI/FunctionFactory.h"
#include "MantidAPI/IFunction.h"
#include "MantidAPI/CompositeFunction.h"
#include "MantidAPI/Expression.h"
#include "MantidAPI/ConstraintFactory.h"
#include "MantidAPI/IConstraint.h"
#include "MantidKernel/Logger.h"
#include "MantidKernel/LibraryManager.h"
#include <Poco/StringTokenizer.h>

namespace Mantid
{
  namespace API
  {

    FunctionFactoryImpl::FunctionFactoryImpl() : Kernel::DynamicFactory<IFunction>(), g_log(Kernel::Logger::get("FunctionFactory"))
    {
      // we need to make sure the library manager has been loaded before we 
      // are constructed so that it is destroyed after us and thus does
      // not close any loaded DLLs with loaded algorithms in them
      Mantid::Kernel::LibraryManager::Instance();
      g_log.debug() << "FunctionFactory created." << std::endl;
    }

    FunctionFactoryImpl::~FunctionFactoryImpl()
    {
    }

    IFunction* FunctionFactoryImpl::createFunction(const std::string& type) const
    {
      IFunction* fun = createUnwrapped(type);
      fun->initialize();
      return fun;
    }



    /**Creates an instance of a function
     * @param input An input string which defines the function and initial values for the parameters.
     * Parameters of different functions are separated by ';'. Parameters of the same function
     * are separated by ','. parameterName=value pairs are used to set a parameter value. For each function
     * "name" parameter must be set to a function name. E.g.
     * input = "name=LinearBackground,A0=0,A1=1; name = Gaussian, PeakCentre=10.,Sigma=1"
     * @return A pointer to the created function
     */
    IFunction* FunctionFactoryImpl::createInitialized(const std::string& input) const
    {
      std::vector<std::string> ops;
      ops.push_back(";");
      ops.push_back(",");
      ops.push_back("=");
      ops.push_back("== < > <= >=");

      Expression expr(ops);
      try
      {
        expr.parse(input);
      }
      catch(...)
      {
        inputError(input);
      }

      if (expr.name() == ";")
      {
        return createComposite(expr);
      }

      return createSimple(expr);

    }

    /** 
     * Create a function from an expression.
     * @param expr The input expression
     * @return A pointer to the created function
     */
    IFunction* FunctionFactoryImpl::createSimple(const Expression& expr)const
    {
      if (expr.name() == "=" && expr.size() > 1)
      {
        return createFunction(expr.terms()[1].name());
      }

      if (expr.name() != "," || expr.size() == 0)
      {
        inputError(expr.str());
      }

      const std::vector<Expression>& terms = expr.terms();
      std::vector<Expression>::const_iterator term = terms.begin();

      if (term->name() != "=") inputError(expr.str());
      if (term->terms()[0].name() != "name" && term->terms()[0].name() != "composite")
      {
        throw std::invalid_argument("Function name must be defined before its parameters");
      }
      std::string fnName = term->terms()[1].name();

      IFunction* fun = createFunction(fnName);

      for(++term;term!=terms.end();++term)
      {// loop over function's parameters/attributes
        if (term->name() != "=") inputError(expr.str());
        std::string parName = term->terms()[0].name();
        std::string parValue = term->terms()[1].name();
        if (fun->hasAttribute(parName))
        {// set attribute
          if (parValue.size() > 1 && parValue[0] == '"')
          {// remove the double quotes
            parValue = parValue.substr(1,parValue.size()-2);
          }
          fun->setAttribute(parName,parValue);
        }
        else
        {// set initial parameter value
          fun->setParameter(parName,atof(parValue.c_str()));
          if ((*term)[1].isFunct())
          {// than its argument is a constraint
            if ((*term)[1][0].name() == "==")
            {
              IConstraint* c = ConstraintFactory::Instance().createUnwrapped("BoundaryConstraint");
              c->initialize((*term)[1]);
              fun->addConstraint(c);
            }
            else
            {
              IConstraint* c = ConstraintFactory::Instance().createUnwrapped((*term)[1][0].name());
              c->initialize((*term)[1][0]);
              fun->addConstraint(c);
            }
          }
        }
      }// for term

      return fun;
    }

    /** 
     * Create a composite function from an expression.
     * @param expr The input expression
     * @return A pointer to the created function
     */
    CompositeFunction* FunctionFactoryImpl::createComposite(const Expression& expr)const
    {
      if (expr.name() != ";") inputError(expr.str());

      const std::vector<Expression>& terms = expr.terms();
      std::vector<Expression>::const_iterator term = terms.begin();

      CompositeFunction* cfun = 0;

      if (term->name() == "=")
      {
        if (term->terms()[0].name() == "composite")
        {
          cfun = dynamic_cast<CompositeFunction*>(createFunction(term->terms()[1].name()));
          if (!cfun) inputError(expr.str());
          term++;
        }
        else if (term->terms()[0].name() == "name")
        {
          cfun = dynamic_cast<CompositeFunction*>(createFunction("CompositeFunction"));
          if (!cfun) inputError(expr.str());
        }
        else
        {
          inputError(expr.str());
        }
      }
      else if (term->name() == ",")
      {
        std::vector<Expression>::const_iterator firstTerm = term->terms().begin();
        if (firstTerm->name() == "=")
        {
          if (firstTerm->terms()[0].name() == "composite")
          {
            cfun = dynamic_cast<CompositeFunction*>(createSimple(*term));
            if (!cfun) inputError(expr.str());
            term++;
          }
          else if (firstTerm->terms()[0].name() == "name")
          {
            cfun = dynamic_cast<CompositeFunction*>(createFunction("CompositeFunction"));
            if (!cfun) inputError(expr.str());
          }
          else
          {
            inputError(expr.str());
          }
        }
      }
      else if (term->name() == ";")
      {
        cfun = dynamic_cast<CompositeFunction*>(createFunction("CompositeFunction"));
        if (!cfun) inputError(expr.str());
      }
      else
      {
        inputError(expr.str());
      }

      for(;term!=terms.end();term++)
      {
        IFunction* fun;
        if (term->name() == ";")
        {
          fun = createComposite(*term);
        }
        else
        {
          fun = createSimple(*term);
        }
        cfun->addFunction(fun);
      }

      return cfun;
    }

    /// Throw an exception
    void FunctionFactoryImpl::inputError(const std::string& str)const
    {
      std::string msg("Error in input string to FunctionFactory");
      if (!str.empty())
      {
        msg += "\n" + str;
      }
      throw std::invalid_argument(msg);
    }

  } // namespace API
} // namespace Mantid
