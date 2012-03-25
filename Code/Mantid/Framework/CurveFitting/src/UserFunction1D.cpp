/*WIKI* 

This algorithm fits a spectrum to a user defined function. The function is supplied to the algorithm as a text string. The function here is a mathematical expression using numbers, variable names and internal function names. Symbols '+', '-', '*', '/', and '^' can be used for arithmetic operations. Names can contain only letters, digits, and the underscore symbol '_'. The internal functions are: 
{|
!Name 
!Argc. 
!Explanation 
|-
|sin || 1 || sine function 
|-
|cos || 1 || cosine function 
|-
|tan || 1 || tangens function 
|-
|asin || 1 || arcus sine function 
|-
|acos || 1 || arcus cosine function 
|-
|atan || 1 || arcus tangens function 
|-
|sinh || 1 || hyperbolic sine function 
|-
|cosh || 1 || hyperbolic cosine 
|-
|tanh || 1 || hyperbolic tangens function 
|-
|asinh || 1 || hyperbolic arcus sine function 
|-
|acosh || 1 || hyperbolic arcus tangens function 
|-
|atanh || 1 || hyperbolic arcur tangens function 
|-
|log2 || 1 || logarithm to the base 2 
|-
|log10 || 1 || logarithm to the base 10 
|-
|log || 1 || logarithm to the base 10 
|-
|ln || 1 || logarithm to base e (2.71828...) 
|-
|exp || 1 || e raised to the power of x 
|-
|sqrt || 1 || square root of a value 
|-
|sign || 1 || sign function -1 if x<0; 1 if x>0 
|-
|rint || 1 || round to nearest integer 
|-
|abs || 1 || absolute value 
|-
|if || 3 || if ... then ... else ... 
|-
|min || var. || min of all arguments 
|-
|max || var. || max of all arguments 
|-
|sum || var. || sum of all arguments 
|-
|avg || var. || mean value of all arguments 
|}

An example of ''Function'' property is "a + b*x + c*x^2". Valiable ''x'' is used to represent the values of the X-vector of the input spectrum. All other variable names are treated as fitting parameters. A parameter can be given an initial value in the ''InitialParameters'' property. For example, "b=1, c=0.2". The order in which the variables are listed is not important. If a variable is not given a value, it is initialized with 0.0. If some of the parameters should be fixed in the fit list them in the ''Fix'' property in any order, e.g. "a,c".

The resulting parameters are returned in a [[TableWorkspace]] set in ''OutputParameters'' property. Also for displaying purposes ''OutputWorkspace'' is returned. It contains the initial spectrum, the fitted spectrum and their difference.

== Example ==
[[Image:UserFunction1D.gif]]

In this example the fitting function is a*exp(-(x-c)^2*s). The parameter ''s'' is fixed.

*WIKI*/
//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidCurveFitting/UserFunction1D.h"
#include "MantidKernel/UnitFactory.h"
#include "MantidKernel/MandatoryValidator.h"
#include <boost/tokenizer.hpp>

namespace Mantid
{
namespace CurveFitting
{

// Register the class into the algorithm factory
DECLARE_ALGORITHM(UserFunction1D)

/// Sets documentation strings for this algorithm
void UserFunction1D::initDocs()
{
  this->setWikiSummary("Fits a histogram from a workspace to a user defined function. ");
  this->setOptionalMessage("Fits a histogram from a workspace to a user defined function.");
}


using namespace Kernel;
using namespace API;

/** Static callback function used by MuParser to initialize variables implicitly
 *  @param varName :: The name of a new variable
 *  @param palg :: Pointer to the algorithm
 *  @return A pointer to the initialized variable
 */
double* UserFunction1D::AddVariable(const char *varName, void *palg)
{
    UserFunction1D& alg = *(UserFunction1D*)palg;

    if (std::string(varName) != "x")
    {
        alg.declareProperty(varName,0.0);
        alg.m_parameterNames.push_back(varName);
    }
    else
    {
        alg.m_x_set = true;
        alg.m_x = 0.;
        return &alg.m_x;
    }

    return &alg.m_parameters[alg.m_nPars++];
}

/** Declare properties that are not fit parameters
 */
void UserFunction1D::declareAdditionalProperties()
{
    declareProperty("Function","",boost::make_shared<MandatoryValidator<std::string> >(),"The fit function");
    declareProperty("InitialParameters","","The comma separated list of initial values of the fit parameters in the form varName=value");
}

/**  Declare fit parameters using muParser's implicit variable initialization.
 */
void UserFunction1D::prepare()
{
    m_parser.SetVarFactory(AddVariable, this);
    std::string funct = getProperty("Function");
    m_parser.SetExpr(funct);

    //Call Eval() to implicitly initialize the variables
    m_parser.Eval();

    if (!m_x_set)
        throw std::runtime_error("Formula does not contain the x variable");

    // Set the initial values to the fit parameters
    std::string initParams = getProperty("InitialParameters");
    if (!initParams.empty())
    {
        typedef boost::tokenizer<boost::char_separator<char> > tokenizer;

        boost::char_separator<char> sep(",");
        tokenizer values(initParams, sep);
        for (tokenizer::iterator it = values.begin(); it != values.end(); ++it)
        {
            size_t ieq = it->find('=');
            if (ieq == std::string::npos) throw std::invalid_argument("Property InitialParameters is malformed");
            std::string varName = it->substr(0,ieq);
            std::string varValue = it->substr(ieq+1);
            size_t i0 = varName.find_first_not_of(" \t");
            size_t i1 = varName.find_last_not_of(" \t");
            if (i0 == std::string::npos) throw std::invalid_argument("Property InitialParameters is malformed");
            varName = varName.substr(i0,i1-i0+1);
            if (varName.empty() || varValue.empty()) throw std::invalid_argument("Property InitialParameters is malformed");
            double value = atof(varValue.c_str());
            if (!existsProperty(varName)) throw std::invalid_argument("Fit parameter "+varName+" does not exist");
            setProperty(varName,value);
        }
    }

}

/*double UserFunction1D::function(const double* in, const double& x)
{
  for(int i=0;i<m_nPars;i++)
    m_parameters[i] = in[i];

  m_x = x;
  return m_parser.Eval();
}*/

/** Calculate the fitting function.
 *  @param in :: A pointer ot the input function parameters
 *  @param out :: A pointer to the output fitting function buffer. The buffer must be large enough to receive nData double values.
 *        The fitting procedure will try to minimise Sum(out[i]^2)
 *  @param xValues :: The array of nData x-values.
 *  @param nData :: The size of the fitted data.
 */
void UserFunction1D::function(const double* in, double* out, const double* xValues, const size_t nData)
{
    for(size_t i=0;i<static_cast<size_t>(m_nPars);i++)
        m_parameters[i] = in[i];

    for (size_t i = 0; i < nData; i++) {
        m_x = xValues[i];
        out[i] = m_parser.Eval();
    }
}

/** 
* @param in :: Input fitting parameter values
* @param out :: Derivatives
* @param xValues :: X values for data points
* @param nData :: Number of data points
 */
void UserFunction1D::functionDeriv(const double* in, Jacobian* out, const double* xValues, const size_t nData)
{
  //throw Exception::NotImplementedError("No derivative function provided");
  if (nData == 0) return;
  std::vector<double> dp(m_nPars);
  std::vector<double> in1(m_nPars);
  for(int i=0;i<m_nPars;i++)
  {
    in1[i] = in[i];
    m_parameters[i] = in[i];
    if (m_parameters[i] != 0.0)
      dp[i] = m_parameters[i]*0.01;
    else
      dp[i] = 0.01;
  }

  if (!m_tmp)
  {
    m_tmp.reset(new double[nData]);
    m_tmp1.reset(new double[nData]);
  }

  function(in, m_tmp.get(),xValues, nData);

  for (int j = 0; j < m_nPars; j++) 
  {
    in1[j] += dp[j];
    function(&in1[0], m_tmp1.get(),xValues, nData);
    for (size_t i = 0; i < nData; i++)
    {
      out->set(i,j, (m_tmp1[i] - m_tmp[i])/dp[j]);
    }
    in1[j] -= dp[j];
  }
}

} // namespace CurveFitting
} // namespace Mantid
