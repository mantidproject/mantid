#include "MantidAPI/FunctionFactory.h"
#include "MantidAPI/IFitFunction.h"
#include "MantidAPI/CompositeFunction.h"
#include "MantidAPI/Expression.h"
#include "MantidAPI/ConstraintFactory.h"
#include "MantidAPI/IConstraint.h"
#include "MantidAPI/Workspace.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidKernel/Logger.h"
#include "MantidKernel/LibraryManager.h"
#include <Poco/StringTokenizer.h>
#include <sstream>

namespace Mantid
{
  namespace API
  {

    FunctionFactoryImpl::FunctionFactoryImpl() : Kernel::DynamicFactory<IFitFunction>(), g_log(Kernel::Logger::get("FunctionFactory"))
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

    IFitFunction* FunctionFactoryImpl::createFunction(const std::string& type) const
    {
      IFitFunction* fun = dynamic_cast<IFitFunction*>(createUnwrapped(type));
      if (!fun)
      {
        throw std::runtime_error("Function "+type+" cannot be cast to IFitFunction");
      }
      fun->initialize();
      return fun;
    }



    /**Creates an instance of a function
     * @param input :: An input string which defines the function and initial values for the parameters.
     * Parameters of different functions are separated by ';'. Parameters of the same function
     * are separated by ','. parameterName=value pairs are used to set a parameter value. For each function
     * "name" parameter must be set to a function name. E.g.
     * input = "name=LinearBackground,A0=0,A1=1; name = Gaussian, PeakCentre=10.,Sigma=1"
     * @return A pointer to the created function
     */
    IFitFunction* FunctionFactoryImpl::createInitialized(const std::string& input) const
    {
      //std::vector<std::string> ops;
      //ops.push_back(";");
      //ops.push_back(",");
      //ops.push_back("=");
      //ops.push_back("== < > <= >=");

      Expression expr;
      try
      {
        expr.parse(input);
      }
      catch(...)
      {
        inputError(input);
      }

      const Expression& e = expr.bracketsRemoved();

      if (e.name() == ";")
      {
        IFitFunction* fun = createComposite(e);
        if (!fun) inputError();
        return fun;
      }

      return createSimple(e);

    }

    /** 
     * Create a function from an expression.
     * @param expr :: The input expression
     * @return A pointer to the created function
     */
    IFitFunction* FunctionFactoryImpl::createSimple(const Expression& expr)const
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

      IFitFunction* fun = createFunction(fnName);
      std::string wsName,wsParam;
      for(++term;term!=terms.end();++term)
      {// loop over function's parameters/attributes
        if (term->name() != "=") inputError(expr.str());
        std::string parName = term->terms()[0].name();
        std::string parValue = term->terms()[1].str();
        if (fun->hasAttribute(parName))
        {// set attribute
          if (parValue.size() > 1 && parValue[0] == '"')
          {// remove the double quotes
            parValue = parValue.substr(1,parValue.size()-2);
          }
          IFitFunction::Attribute att = fun->getAttribute(parName);
          att.fromString(parValue);
          fun->setAttribute(parName,att);
        }
        else if (parName.size() >= 10 && parName.substr(0,10) == "constraint")
        {// or it can be a list of constraints
          addConstraints(fun,(*term)[1]);
        }
        else if (parName == "ties")
        {
          addTies(fun,(*term)[1]);
        }
        else if (parName == "Workspace")
        {
          wsName = parValue;
        }
        else if (parName == "WSParam")
        {
          wsParam = parValue;
        }
        else
        {// set initial parameter value
          fun->setParameter(parName,atof(parValue.c_str()));
        }
      }// for term

      if (!wsName.empty())
      {
        Workspace_sptr ws = AnalysisDataService::Instance().retrieve(wsName);
        fun->setWorkspace(ws,wsParam);
      }

      return fun;
    }

    /** 
     * Create a composite function from an expression.
     * @param expr :: The input expression
     * @return A pointer to the created function
     */
    CompositeFunction* FunctionFactoryImpl::createComposite(const Expression& expr)const
    {
      if (expr.name() != ";") inputError(expr.str());

      if (expr.size() == 0)
      {
        return 0;
      }

      const std::vector<Expression>& terms = expr.terms();
      std::vector<Expression>::const_iterator it = terms.begin();
      const Expression& term = it->bracketsRemoved();

      CompositeFunction* cfun = 0;
      std::string wsName,wsParam;
      if (term.name() == "=")
      {
        if (term.terms()[0].name() == "composite")
        {
          cfun = dynamic_cast<CompositeFunction*>(createFunction(term.terms()[1].name()));
          if (!cfun) inputError(expr.str());
          it++;
        }
        else if (term.terms()[0].name() == "name")
        {
          cfun = dynamic_cast<CompositeFunction*>(createFunction("CompositeFunctionMW"));
          if (!cfun) inputError(expr.str());
        }
        else
        {
          inputError(expr.str());
        }
      }
      else if (term.name() == ",")
      {
        std::vector<Expression>::const_iterator firstTerm = term.terms().begin();
        if (firstTerm->name() == "=")
        {
          if (firstTerm->terms()[0].name() == "composite")
          {
            cfun = dynamic_cast<CompositeFunction*>(createSimple(term));
            if (!cfun) inputError(expr.str());
            it++;
          }
          else if (firstTerm->terms()[0].name() == "name")
          {
            cfun = dynamic_cast<CompositeFunction*>(createFunction("CompositeFunctionMW"));
            if (!cfun) inputError(expr.str());
          }
          else
          {
            inputError(expr.str());
          }
        }
      }
      else if (term.name() == ";")
      {
        cfun = dynamic_cast<CompositeFunction*>(createFunction("CompositeFunctionMW"));
        if (!cfun) inputError(expr.str());
      }
      else
      {
        inputError(expr.str());
      }

      for(;it!=terms.end();it++)
      {
        const Expression& term = it->bracketsRemoved();
        IFitFunction* fun;
        if (term.name() == ";")
        {
          fun = createComposite(term);
          if (!fun) continue;
        }
        else
        {
          std::string parName = term[0].name();
          std::string parValue = term[1].str();
          if (term[0].name().size() >= 10 && term[0].name().substr(0,10) == "constraint")
          {
            addConstraints(cfun,term[1]);
            continue;
          }
          else if (term[0].name() == "ties")
          {
            addTies(cfun,term[1]);
            continue;
          }
          else if (parName == "Workspace")
          {
            wsName = parValue;
          }
          else if (parName == "WSParam")
          {
            wsParam = parValue;
          }
          else
          {
            fun = createSimple(term);
          }
        }
        cfun->addFunction(fun);
      }

      if (!wsName.empty())
      {
        Workspace_sptr ws = AnalysisDataService::Instance().retrieve(wsName);
        cfun->setWorkspace(ws,wsParam);
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

    /** 
     * Add constraints to the created function
     * @param fun :: The function
     * @param expr :: The constraint expression. The expression name must be either a single constraint
     *    expression such as "0 < Sigma < 1" or a list of constraint expressions separated by commas ','
     *    and enclosed in brackets "(...)" .
     */
    void FunctionFactoryImpl::addConstraints(IFitFunction* fun,const Expression& expr)const
    {
      if (expr.name() == ",")
      {
        for(size_t i=0;i<expr.size();i++)
        {
          addConstraint(fun,expr[i]);
        }
      }
      else
      {
        addConstraint(fun,expr);
      }
    }

    /** 
     * Add a constraints to the function
     * @param fun :: The function
     * @param expr :: The constraint expression.
     */
    void FunctionFactoryImpl::addConstraint(IFitFunction* fun,const Expression& expr)const
    {
      IConstraint* c = ConstraintFactory::Instance().createInitialized(fun,expr);
      fun->addConstraint(c);
    }

    /**
     * @param fun :: The function
     * @param expr :: The tie expression: either parName = TieString or a list
     *   of name = string pairs
     */
    void FunctionFactoryImpl::addTies(IFitFunction* fun,const Expression& expr)const
    {
      if (expr.name() == "=")
      {
        addTie(fun,expr);
      }
      else if (expr.name() == ",")
      {
        for(size_t i=0;i<expr.size();i++)
        {
          addTie(fun,expr[i]);
        }
      }
    }

    /**
     * @param fun :: The function
     * @param expr :: The tie expression: parName = TieString
     */
    void FunctionFactoryImpl::addTie(IFitFunction* fun,const Expression& expr)const
    {
      if (expr.size() > 1)
      {// if size > 2 it is interpreted as setting a tie (last expr.term) to multiple parameters, e.g 
        // f1.alpha = f2.alpha = f3.alpha = f0.beta^2/2
        const std::string value = expr[expr.size()-1].str();
	for( size_t i = expr.size() - 1; i != 0; )
	{
	  --i;
          fun->tie(expr[i].name(),value);	  
	}
      }
    }

    /**
      * Create a fitting function from a string.
      * @param input :: The input string, has a form of a function call: funName(attr1=val,param1=val,...,ties=(param3=2*param1,...),constraints=(p2>0,...))
      */
    IFitFunction* FunctionFactoryImpl::createFitFunction(const std::string& input) const
    {
      Expression expr;
      try
      {
        expr.parse(input);
      }
      catch(...)
      {
        inputError(input);
      }
      return createFitFunction(expr);
    }

    /**
      * Create a fitting function from an expression.
      * @param expr :: The input expression made by parsing the input string to createFitFunction(const std::string& input)
      */
    IFitFunction* FunctionFactoryImpl::createFitFunction(const Expression& expr) const
    {
      const Expression& e = expr.bracketsRemoved();

      std::string fnName = e.name();

      IFitFunction* fun = createUnwrapped(fnName);
      if (!fun)
      {
        throw std::runtime_error("Cannot create function "+fnName);
      }
      fun->initialize();

      const std::vector<Expression>& terms = e.terms();
      std::vector<Expression>::const_iterator term = terms.begin();

      for(;term!=terms.end();++term)
      {// loop over function's parameters/attributes
        if (term->name() == "=")
        {
          std::string parName = term->terms()[0].name();
          std::string parValue = term->terms()[1].str();
          if (fun->hasAttribute(parName))
          {// set attribute
            if (parValue.size() > 1 && parValue[0] == '"')
            {// remove the double quotes
              parValue = parValue.substr(1,parValue.size()-2);
            }
            IFitFunction::Attribute att = fun->getAttribute(parName);
            att.fromString(parValue);
            fun->setAttribute(parName,att);
          }
          else if (parName.size() >= 10 && parName.substr(0,10) == "constraint")
          {// or it can be a list of constraints
            addConstraints(fun,(*term)[1]);
          }
          else if (parName == "ties")
          {
            addTies(fun,(*term)[1]);
          }
          else
          {// set initial parameter value
            fun->setParameter(parName,atof(parValue.c_str()));
          }
        }
        else // if the term isn't a name=value pair it could be a member function of a composite function
        {
          throw Kernel::Exception::NotImplementedError("Composite functions are not implemented yet for IFitFunction");
          //CompositeFunction* cfun = dynamic_cast<CompositeFunction*>(fun);
          //if (!cfun)
          //{
          //  throw std::runtime_error("Cannot add a function to a non-composite function "+fnName);
          //}
          //IFitFunction* mem = createFitFunction(*term);
          //cfun->addFunction(mem);
        }
      }// for term

      return fun;
    }

  } // namespace API
} // namespace Mantid
