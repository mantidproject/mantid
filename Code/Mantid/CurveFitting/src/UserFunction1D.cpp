//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidCurveFitting/UserFunction1D.h"
#include "MantidAPI/TableRow.h"
#include "MantidDataObjects/Workspace2D.h"
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
    @param varName The name of a new variable
    @param palg Pointer to the algorithm
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

// Get a reference to the logger
Logger& UserFunction1D::g_log = Logger::get("UserFunction1D");

/** Declare properties that are not fit parameters
 */
void UserFunction1D::declareAdditionalProperties()
{
    declareProperty("Function","",new MandatoryValidator<std::string>,"The fit function");
    declareProperty("InitialParameters","","The comma separated list of initial values of the fit parameters in the form varName=value");
    declareProperty(
        new WorkspaceProperty<API::ITableWorkspace>("Parameters","",Direction::Output),
        "The name of the TableWorkspace in which to store the final fit parameters" );
    declareProperty(new WorkspaceProperty<MatrixWorkspace>("OutputWorkspace","",Direction::Output), 
        "Name of the output Workspace holding resulting simlated spectrum");
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

/** Calculate the fitting function.
 *  @param in A pointer ot the input function parameters
 *  @param out A pointer to the output fitting function buffer. The buffer must be large enough to receive nData double values.
 *        The fitting procedure will try to minimise Sum(out[i]^2)
 *  @param xValues The array of nData x-values.
 *  @param yValues The array of nData y-values.
 *  @param yErrors The array of nData error values.
 *  @param nData The size of the fitted data.
 */
void UserFunction1D::function(const double* in, double* out, const double* xValues, const double* yValues, const double* yErrors, const int& nData)
{
    for(int i=0;i<m_nPars;i++)
        m_parameters[i] = in[i];

    //mu::varmap_type variables = m_parser.GetVar();
    //mu::varmap_type::const_iterator item = variables.begin();
    //for (; item!=variables.end(); ++item)
    //    std::cout << "Name: " << item->first << " Address: [0x" << item->second << "] = "<<*item->second<<"\n";

    for (int i = 0; i < nData; i++) {
        m_x = xValues[i];
        double Yi = m_parser.Eval();
        double Yv = yValues[i];
        double Err = yErrors[i];
        if (Err <= 0.) Err = 1.;
        out[i] = (Yi - Yv)/Err;
    }
}

/**  Use this method to finalize the fit. Create output workspaces with the fit parameters
 *   and the fitted function
 */
void UserFunction1D::finalize()
{
    // Save the final fit parameters in the output table workspace
    ITableWorkspace_sptr m_result = WorkspaceFactory::Instance().createTable("TableWorkspace");
    m_result->addColumn("str","Name");
    m_result->addColumn("double","Value");
    for(int i=0;i<m_nPars;i++)
    {
        TableRow row = m_result->appendRow();
        row << m_parameterNames[i] << m_parameters[i];
    }
    setProperty("Parameters",m_result);

    // Save the fitted and simulated spectra in the output workspace
    MatrixWorkspace_const_sptr inputWorkspace = getProperty("InputWorkspace");
    int iSpec = getProperty("SpectrumIndex");
    const std::vector<double>& inputX = inputWorkspace->readX(iSpec);
    const std::vector<double>& inputY = inputWorkspace->readY(iSpec);

    Mantid::DataObjects::Workspace2D_sptr ws = boost::dynamic_pointer_cast<Mantid::DataObjects::Workspace2D>
        (WorkspaceFactory::Instance().create("Workspace2D",3,inputX.size(),inputY.size()));
    ws->setTitle("");
    ws->getAxis(0)->unit() = UnitFactory::Instance().create("TOF");

    for(int i=0;i<3;i++)
        ws->dataX(i) = inputWorkspace->readX(iSpec);

    ws->dataY(0) = inputWorkspace->readY(iSpec);

    std::vector<double>& Y = ws->dataY(1);
    std::vector<double>& E = ws->dataY(2);

    for(unsigned int i=0;i<Y.size();i++)
    {
        m_x = inputX[i];
        Y[i] = m_parser.Eval();
        E[i] = inputY[i] - Y[i];
    }

    setProperty("OutputWorkspace",boost::dynamic_pointer_cast<MatrixWorkspace>(ws));

}

} // namespace CurveFitting
} // namespace Mantid
