#include "MantidAPI/FunctionFactory.h"
#include "MantidAPI/IFunction.h"
#include "MantidAPI/CompositeFunction.h"
#include "MantidKernel/Logger.h"
#include <Poco/StringTokenizer.h>

namespace Mantid
{
  namespace API
  {

    FunctionFactoryImpl::FunctionFactoryImpl() : Kernel::DynamicFactory<IFunction>(), g_log(Kernel::Logger::get("FunctionFactory"))
    {
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
      typedef Poco::StringTokenizer tokenizer;
      tokenizer functions(input, ";", tokenizer::TOK_IGNORE_EMPTY | tokenizer::TOK_TRIM);

      bool isComposite = functions.count() > 1;
      API::IFunction* function = 0;

      if (isComposite)
      {
        function = createFunction("CompositeFunction");
      }

      // Loop over functions. 
      for (tokenizer::Iterator ifun = functions.begin(); ifun != functions.end(); ++ifun)
      {
        tokenizer params(*ifun, ",", tokenizer::TOK_IGNORE_EMPTY | tokenizer::TOK_TRIM);
        std::map<std::string,std::string> param;
        // Loop over function parameters to fill in param map: param[<name>]=<init_value>
        for (tokenizer::Iterator par = params.begin(); par != params.end(); ++par)
        {
          tokenizer name_value(*par, "=", tokenizer::TOK_IGNORE_EMPTY | tokenizer::TOK_TRIM);
          if (name_value.count() > 1)
          {
            std::string name = name_value[0];
            //std::transform(name.begin(), name.end(), name.begin(), toupper);
            param[name] = name_value[1];
          }
        }

        // param["name"] gives the name(type) of the function
        std::string functionName = param["name"];
        if (functionName.empty())
          throw std::runtime_error("Function is not defined");

        API::IFunction* fun = API::FunctionFactory::Instance().createFunction(functionName);

        if (isComposite)
        {
          static_cast<API::CompositeFunction*>(function)->addFunction(fun);
        }
        else
        {
          function = fun;
        }

        // Loop over param to set the initial values and constraints
        std::map<std::string,std::string>::const_iterator par = param.begin();
        for(;par!=param.end();++par)
        {
          const std::string& parName(par->first);
          if (parName != "name")
          {
            // The init value part may contain a constraint setting string in brackets 
            std::string parValue = par->second;
            size_t i = parValue.find_first_of('(');
            if (i != std::string::npos)
            {
              size_t j = parValue.find_last_of(')');
              if (j != std::string::npos && j > i+1)
              {
                tokenizer constraint(parValue.substr(i+1,j-i-1), ":", tokenizer::TOK_IGNORE_EMPTY | tokenizer::TOK_TRIM);
                if (constraint.count()>0)
                {
                  /////// to be implemented when the constraint factory is ready
                  //BoundaryConstraint* con = new BoundaryConstraint(parName);
                  //if (constraint.count() >= 1)
                  //{
                  //  con->setLower(atof(constraint[0].c_str()));
                  //}
                  //if (constraint.count() > 1)
                  //{
                  //  con->setUpper(atof(constraint[1].c_str()));
                  //}
                  //fun->addConstraint(con);
                }
              }
              parValue.erase(i);
            }
            fun->getParameter(parName) = atof(parValue.c_str());
          }
        }
      }
      return function;
    }




  } // namespace API
} // namespace Mantid
