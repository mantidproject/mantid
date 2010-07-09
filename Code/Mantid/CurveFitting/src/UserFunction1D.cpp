//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidCurveFitting/UserFunction1D.h"
#include "MantidKernel/UnitFactory.h"
#include <boost/tokenizer.hpp>

namespace Mantid
{
namespace CurveFitting
{

// Register the class into the algorithm factory
DECLARE_ALGORITHM(UserFunction1D)

using namespace Kernel;
using namespace API;

/** Static callback function used by MuParser to initialize variables implicitly
 *  @param varName The name of a new variable
 *  @param palg Pointer to the algorithm
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
    declareProperty("Function","",new MandatoryValidator<std::string>,"The fit function");
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
 *  @param in A pointer ot the input function parameters
 *  @param out A pointer to the output fitting function buffer. The buffer must be large enough to receive nData double values.
 *        The fitting procedure will try to minimise Sum(out[i]^2)
 *  @param xValues The array of nData x-values.
 *  @param nData The size of the fitted data.
 */
void UserFunction1D::function(const double* in, double* out, const double* xValues, const int& nData)
{
    for(int i=0;i<m_nPars;i++)
        m_parameters[i] = in[i];

    for (int i = 0; i < nData; i++) {
        m_x = xValues[i];
        out[i] = m_parser.Eval();
    }
}

/** 
* @param in Input fitting parameter values
* @param out Derivatives
* @param xValues X values for data points
* @param nData Number of data points
 */
void UserFunction1D::functionDeriv(const double* in, Jacobian* out, const double* xValues, const int& nData)
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
    for (int i = 0; i < nData; i++) 
    {
      out->set(i,j, (m_tmp1[i] - m_tmp[i])/dp[j]);
    }
    in1[j] -= dp[j];
  }
}

} // namespace CurveFitting
} // namespace Mantid
