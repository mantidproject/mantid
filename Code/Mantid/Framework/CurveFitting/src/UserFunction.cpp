//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidCurveFitting/UserFunction.h"
#include <boost/tokenizer.hpp>
#include <muParser.h>

namespace Mantid
{
  namespace CurveFitting
  {

    // Register the class into the function factory
    DECLARE_FUNCTION(UserFunction)

    using namespace Kernel;
    using namespace API;

    /// Constructor
    UserFunction::UserFunction() : m_parser(new mu::Parser()), m_x_set(false) {}

    /// Destructor
    UserFunction::~UserFunction()
    {
      delete m_parser;
    }

    /** Static callback function used by MuParser to initialize variables implicitly
    @param varName The name of a new variable
    @param pufun Pointer to the function
    */
    double* UserFunction::AddVariable(const char *varName, void *pufun)
    {
      UserFunction& fun = *(UserFunction*)pufun;

      if (std::string(varName) != "x")
      {
        try
        {
          fun.declareParameter(varName,0.0);
        }
        catch(...)
        {}
      }
      else
      {
        fun.m_x_set = true;
        fun.m_x = 0.;
      }

      return &fun.m_x;
    }

    /**  Declare fit parameters using muParser's implicit variable initialization.
     * @param attName Attribute name, must be "Formula"
     * @param value The attribute value. For "Formula" it must be a mu::Parser expression string
     */
    void UserFunction::setAttribute(const std::string& attName,const IFunction::Attribute& value)
    {
      if (attName != "Formula")
      {
        IFunction::setAttribute(attName,value);
        return;
      }

      m_x_set = false;
      clearAllParameters();

      try
      {
        mu::Parser tmp_parser;
        tmp_parser.SetVarFactory(AddVariable, this);

        m_formula = value.asString();
        tmp_parser.SetExpr(m_formula);

        //Call Eval() to implicitly initialize the variables
        tmp_parser.Eval();

      }
      catch(...)
      {
        throw std::invalid_argument("Cannot parse expression " + m_formula);
      }

      if (!m_x_set)
      {
        throw std::invalid_argument("Formula does not contain the x variable");
      }


      m_parser->ClearVar();
      m_parser->DefineVar("x",&m_x);
      for(int i=0;i<nParams();i++)
      {
        m_parser->DefineVar(parameterName(i),getParameterAddress(i));
      }

      m_parser->SetExpr(m_formula);
    }

    /** Calculate the fitting function.
    *  @param out A pointer to the output fitting function buffer. The buffer must be large enough to receive nData double values.
    *        The fitting procedure will try to minimise Sum(out[i]^2)
    *  @param xValues The array of nData x-values.
    *  @param nData The size of the fitted data.
    */
    void UserFunction::function(double* out, const double* xValues, const int& nData)const
    {
      for (int i = 0; i < nData; i++) 
      {
          m_x = xValues[i];
          out[i] = m_parser->Eval();
      }
    }

    /** 
    * @param out Derivatives
    * @param xValues X values for data points
    * @param nData Number of data points
    */
    void UserFunction::functionDeriv(Jacobian* out, const double* xValues, const int& nData)
    {
      if (nData == 0) return;
      std::vector<double> dp(nParams());
      std::vector<double> param(nParams());
      for(int i=0;i<nParams();i++)
      {
        double param = getParameter(i);
        if (param != 0.0)
        {
          dp[i] = param*0.01;
        }
        else
        {
          dp[i] = 0.01;
        }
      }

      if (!m_tmp)
      {
        m_tmp.reset(new double[nData]);
        m_tmp1.reset(new double[nData]);
      }

      function(m_tmp.get(),xValues, nData);

      for (int j = 0; j < nParams(); j++) 
      {
        double p0 = getParameter(j);
        setParameter(j,p0 + dp[j],false);
        function(m_tmp1.get(),xValues, nData);
        for (int i = 0; i < nData; i++) 
        {
          out->set(i,j, (m_tmp1[i] - m_tmp[i])/dp[j]);
        }
        setParameter(j,p0,false);
      }
    }

  } // namespace CurveFitting
} // namespace Mantid
