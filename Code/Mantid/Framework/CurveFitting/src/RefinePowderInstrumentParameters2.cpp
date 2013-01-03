#include "MantidCurveFitting/FitPowderPeakParameters.h"

#include "MantidAPI/Axis.h"
#include "MantidAPI/TextAxis.h"
#include "MantidAPI/TableRow.h"
#include "MantidKernel/ListValidator.h"

namespace Mantid
{
namespace CurveFitting
{

  DECLARE_ALGORITHM(FitPowderPeakParameters)

  //----------------------------------------------------------------------------------------------
  /** Constructor
   */
  FitPowderPeakParameters::FitPowderPeakParameters()
  {
  }
    
  //----------------------------------------------------------------------------------------------
  /** Destructor
   */
  FitPowderPeakParameters::~FitPowderPeakParameters()
  {
  }

  //----------------------------------------------------------------------------------------------
  /** Set up documention
    */
  void FitPowderPeakParameters::initDocs()
  {
    setWikiSummary("Refine the instrument geometry related parameters for powder diffractomer. ");
    setOptionalMessage("Parameters include Dtt1, Dtt1t, Dtt2t, Zero, Zerot. ");
  }

  //----------------------------------------------------------------------------------------------
  /** Declare properties
    */
  void FitPowderPeakParameters::init()
  {
    // Peak position workspace
    declareProperty(new WorkspaceProperty<Workspace2D>("InputPeakPositionWorkspace", "Anonymous",
                                                       Direction::Input),
                    "Data workspace containing workspace positions in TOF agains dSpacing.");

    // Workspace Index
    declareProperty("WorkspaceIndex", 0,
                    "Workspace Index of the peak positions in PeakPositionWorkspace.");

    // Output workspace
    declareProperty(new WorkspaceProperty<Workspace2D>("OutputPeakPositionWorkspace", "Anonymous2",
                                                       Direction::Output),
                    "Output data workspace containing refined workspace positions in TOF agains dSpacing.");

    // Input Table workspace containing instrument profile parameters
    declareProperty(new WorkspaceProperty<TableWorkspace>("InputInstrumentParameterWorkspace", "Anonymous3",
                                                          Direction::Input),
                    "INput tableWorkspace containg instrument's parameters.");

    // Output table workspace containing the refined parameters
    declareProperty(new WorkspaceProperty<TableWorkspace>("OutputInstrumentParameterWorkspace", "Anonymous4",
                                                          Direction::Output),
                    "Output tableworkspace containing instrument's fitted parameters. ");

    // Refinement algorithm
    vector<string> algoptions;
    algoptions.push_back("OneStepFit");
    algoptions.push_back("MonteCarlo");
    auto validator = boost::make_shared<Kernel::StringListValidator>(algoptions);
    declareProperty("RefinementAlgorithm", "MonteCarlo", validator,
                    "Algorithm to refine the instrument parameters.");

    // Random walk steps
    declareProperty("RandomWalkSteps", 10000, "Number of Monte Carlo random walk steps. ");

    // Random seed
    declareProperty("MonteCarloRandomSeed", 0, "Random seed for Monte Carlo simulation. ");

    // Method to calcualte the standard error of peaks
    vector<string> stdoptions;
    stdoptions.push_back("ConstantValue");
    stdoptions.push_back("UseInputValue");
    auto listvalidator = boost::make_shared<Kernel::StringListValidator>(stdoptions);
    declareProperty("StandardError", "ConstantValue", listvalidator,
                    "Algorithm to calculate the standard error of peak positions.");

    return;
  }

  //----------------------------------------------------------------------------------------------
  /** Main execution body
    */
  void FitPowderPeakParameters::exec()
  {
    // 1. Process input
    processInputProperties();

    // 2. Parse input table workspace
    parseTableWorkspaces();

    // 3. Set up main function for peak positions
    ThermalNeutronDtoTOFFunction rawfunc;
    m_positionFunc = boost::make_shared<ThermalNeutronDtoTOFFunction>(rawfunc);
    m_positionFunc->initialize();

    // 3. Fit
    // a) Set up parameter value
    setFunctionParameterValues(m_positionFunc, m_profileParameters);

    // b) Generate some global useful value and Calculate starting chi^2
    API::FunctionDomain1DVector domain(m_dataWS->readX(m_wsIndex));
    API::FunctionValues rawvalues(domain);
    m_positionFunc->function(domain, rawvalues);

    // d) Calcualte statistic
    double startchi2 = calculateFunctionError(m_positionFunc, m_dataWS, m_wsIndex);

    // b) Fit by type
    double finalchi2 = DBL_MAX;
    if (m_fitMode == FIT)
    {
      // Fit by non-Monte Carlo method
      g_log.notice("Fit by non Monte Carlo algorithm. ");
      finalchi2 = execFitParametersNonMC();
    }
    else
    {
      // Fit by Monte Carlo method
      g_log.notice("Fit by Monte Carlo algorithm. ");
      throw runtime_error("Haven't been implemented yet!");
    }

    // updateFunctionParameterValues(m_positionFunc, m_profileParameters);

    // 4. Process the output
    TableWorkspace_sptr fitparamtable = genOutputProfileTable(m_profileParameters,
                                                              startchi2, finalchi2);
    setProperty("OutputInstrumentParameterWorkspace", fitparamtable);

    Workspace2D_sptr outdataws = genOutputWorkspace(domain, rawvalues);
    setProperty("OutputPeakPositionWorkspace", outdataws);

    return;
  }

  //----------------------------------------------------------------------------------------------
  /** Process input properties
    */
  void FitPowderPeakParameters::processInputProperties()
  {
    // Data Workspace
    m_dataWS = getProperty("InputPeakPositionWorkspace");

    m_wsIndex = getProperty("WorkspaceIndex");
    if (m_wsIndex < 0 || m_wsIndex >= static_cast<int>(m_dataWS->getNumberHistograms()))
    {
      throw runtime_error("Input workspace index is out of range.");
    }

    // Parameter TableWorkspace
    m_paramTable = getProperty("InputInstrumentParameterWorkspace");

    // Fit mode
    string fitmode = getProperty("RefinementAlgorithm");
    if (fitmode.compare("OneStepFit") == 0)
      m_fitMode = FIT;
    else if (fitmode.compare("MonteCarlo") == 0)
      m_fitMode = MONTECARLO;
    else
    {
      m_fitMode = FIT;
      throw runtime_error("Input RefinementAlgorithm is not supported.");
    }

    // Stanard error mode
    string stdmode = getProperty("StandardError");
    if (stdmode.compare("ConstantValue") == 0)
      m_stdMode = CONSTANT;
    else if (stdmode.compare("UseInputValue") == 0)
      m_stdMode = USEINPUT;
    else
    {
      m_stdMode = USEINPUT;
      throw runtime_error("Input StandardError (mode) is not supported.");
    }

    // Monte Carlo
    m_numWalkSteps = getProperty("RandomWalkSteps");
    if (m_numWalkSteps <= 0)
      throw runtime_error("Monte Carlo walk steps cannot be less or equal to 0. ");

    m_randomSeed = getProperty("MonteCarloRandomSeed");

    return;
  }

  //----------------------------------------------------------------------------------------------
  /** Parse TableWorkspaces
    */
  void FitPowderPeakParameters::parseTableWorkspaces()
  {
    m_profileParameters.clear();

    parseTableWorkspace(m_paramTable, m_profileParameters);

  }

  //----------------------------------------------------------------------------------------------
  /** Parse table workspace to a map of Parameters
    */
  void FitPowderPeakParameters::parseTableWorkspace(TableWorkspace_sptr tablews,
                                                    map<string, Parameter>& parammap)
  {
    // 1. Process Table column names
    std::vector<std::string> colnames = tablews->getColumnNames();
    map<string, size_t> colnamedict;
    convertToDict(colnames, colnamedict);

    int iname = getStringIndex(colnamedict, "Name");
    int ivalue = getStringIndex(colnamedict, "Value");
    int ifit = getStringIndex(colnamedict, "FitOrTie");
    int imin = getStringIndex(colnamedict, "Min");
    int imax = getStringIndex(colnamedict, "Max");
    int istep = getStringIndex(colnamedict, "StepSize");

    if (iname < 0 || ivalue < 0 || ifit < 0)
      throw runtime_error("TableWorkspace does not have column Name, Value and/or Fit.");

    // 3. Parse
    size_t numrows = tablews->rowCount();
    for (size_t irow = 0; irow < numrows; ++irow)
    {
      string parname = tablews->cell<string>(irow, iname);
      double parvalue = tablews->cell<double>(irow, ivalue);
      string fitq = tablews->cell<string>(irow, ifit);

      double minvalue;
      if (imin >= 0)
        minvalue = tablews->cell<double>(irow, imin);
      else
        minvalue = -DBL_MAX;

      double maxvalue;
      if (imax >= 0)
        maxvalue = tablews->cell<double>(irow, imax);
      else
        maxvalue = DBL_MAX;

      double stepsize;
      if (istep >= 0)
        stepsize = tablews->cell<double>(irow, istep);
      else
        stepsize = 1.0;

      Parameter newpar;
      newpar.name = parname;
      newpar.value = parvalue;
      newpar.minvalue = minvalue;
      newpar.maxvalue = maxvalue;
      newpar.stepsize = stepsize;

      // If empty string, fit is default to be false
      bool fit = false;
      if (fitq.size() > 0)
      {
        if (fitq[0] == 'F' || fitq[0] == 'f')
          fit = true;
      }
      newpar.fit = fit;

      parammap.insert(make_pair(parname, newpar));
    }

    return;
  }

  //----------------------------------------------------------------------------------------------
  /** Fit instrument parameters by non Monte Carlo algorithm
    * Requirement:  m_positionFunc should have the best fit result;
   */
  double FitPowderPeakParameters::execFitParametersNonMC()
  {
    // 1. Set up constraints
    setFunctionParameterFitSetups(m_positionFunc, m_profileParameters);

    // 2. Fit function
    double chi2;
    fitFunction(m_positionFunc, m_dataWS, m_wsIndex, chi2);

    return chi2;
  }

  //----------------------------------------------------------------------------------------------
  /** Calculate Chi^2 of the a function with all parameters are fixed
    */
  double FitPowderPeakParameters::calculateFunctionError(IFunction_sptr function,
                                                         Workspace2D_sptr dataws, int wsindex)
  {
    // 1. Record the fitting information
    vector<string> parnames = function->getParameterNames();
    vector<bool> vecFix(parnames.size(), false);

    for (size_t i = 0; i < parnames.size(); ++i)
    {
      bool fixed = function->isFixed(i);
      vecFix[i] = fixed;
      if (!fixed)
        function->fix(i);
    }

    // 2. Fit with zero iteration
    double chi2;
    string fitstatus;
    doFitFunction(function, dataws, wsindex, "Levenberg-MarquardtMD", 0, chi2, fitstatus);

    // 3. Restore the fit/fix setup
    for (size_t i = 0; i < parnames.size(); ++i)
    {
      if (!vecFix[i])
        function->unfix(i);
    }

    return chi2;
  }

  //----------------------------------------------------------------------------------------------
  /** Fit a function by trying various minimizer or minimizer combination
    *
    * @param chi2:  chi2 of the final (best) solution
    */
  double FitPowderPeakParameters::fitFunction(IFunction_sptr function, Workspace2D_sptr dataws,
                                              int wsindex, double& chi2)
  {
    // 1. Store original
    map<string, pair<double, double> > start_paramvaluemap, paramvaluemap1, paramvaluemap2,
        paramvaluemap3;
    storeFunctionParameterValue(function, start_paramvaluemap);

    // 1. Simplex
    string minimizer = "Simplex";
    double chi2simplex;
    string fitstatussimplex;
    int numiters = 10000;
    bool fitgood1 = doFitFunction(function, dataws, wsindex, minimizer, numiters,
                                  chi2simplex, fitstatussimplex);

    if (fitgood1)
      storeFunctionParameterValue(function, paramvaluemap1);
    else
      chi2simplex = DBL_MAX;

    // 2. Continue Levenberg-Marquardt following Simplex
    minimizer = "Levenberg-MarquardtMD";
    double chi2lv2;
    string fitstatuslv2;
    numiters = 1000;
    bool fitgood2 = doFitFunction(function, dataws, wsindex, minimizer, numiters,
                                  chi2lv2, fitstatuslv2);
    if (fitgood2)
      storeFunctionParameterValue(function, paramvaluemap2);
    else
      chi2lv2 = DBL_MAX;

    // 3. Fit by L.V. solely
    map<string, Parameter> tempparmap;
    restoreFunctionParameterValue(start_paramvaluemap, function, tempparmap);
    double chi2lv1;
    string fitstatuslv1;
    bool fitgood3 = doFitFunction(function, dataws, wsindex, minimizer, numiters,
                                  chi2lv1, fitstatuslv1);
    if (fitgood3)
      storeFunctionParameterValue(function, paramvaluemap3);
    else
      chi2lv1 = DBL_MAX;

    // 4. Compare best
    bool retvalue;
    g_log.error() << "Find Bug:  Chi2s = " << chi2simplex << ", " << chi2lv2 << ", " << chi2lv1 << endl;
    if (fitgood1 || fitgood2 || fitgood3)
    {
      // At least one good fit
      if (fitgood1 && chi2simplex <= chi2lv2 && chi2simplex <= chi2lv1)
      {
        chi2 = chi2simplex;
        restoreFunctionParameterValue(paramvaluemap1, function, m_profileParameters);
      }
      else if (fitgood2 && chi2lv2 <= chi2lv1)
      {
        restoreFunctionParameterValue(paramvaluemap2, function, m_profileParameters);
        chi2 = chi2lv2;
      }
      else if (fitgood3)
      {
        chi2 = chi2lv1;
        restoreFunctionParameterValue(paramvaluemap3, function, m_profileParameters);
      }
      else
      {
        throw runtime_error("This situation is impossible to happen!");
      }

      retvalue = true;
    }
    else
    {
      // No fit is good
      retvalue = false;
      chi2 = DBL_MAX;
    }

    return retvalue;
  }

  //----------------------------------------------------------------------------------------------
  /** Fit function
    * Minimizer: "Levenberg-MarquardtMD"/"Simplex"
   */
  bool FitPowderPeakParameters::doFitFunction(IFunction_sptr function, Workspace2D_sptr dataws, int wsindex,
                                              string minimizer, int numiters, double& chi2, string& fitstatus)
  {
    // 0. Debug output
    stringstream outss;
    outss << "Fit function: " << m_positionFunc->asString() << endl << "Data To Fit: \n";
    for (size_t i = 0; i < dataws->readX(0).size(); ++i)
      outss << dataws->readX(wsindex)[i] << "\t\t" << dataws->readY(wsindex)[i] << "\t\t"
            << dataws->readE(wsindex)[i] << endl;
    g_log.information() << outss.str();

    // 1. Create and setup fit algorithm
    API::IAlgorithm_sptr fitalg = createChildAlgorithm("Fit", 0.0, 0.2, true);
    fitalg->initialize();

    fitalg->setProperty("Function", function);
    fitalg->setProperty("InputWorkspace", dataws);
    fitalg->setProperty("WorkspaceIndex", wsindex);
    fitalg->setProperty("Minimizer", minimizer);
    fitalg->setProperty("CostFunction", "Least squares");
    fitalg->setProperty("MaxIterations", numiters);
    fitalg->setProperty("CalcErrors", true);

    // 2. Fit
    bool successfulfit = fitalg->execute();
    if (!fitalg->isExecuted() || ! successfulfit)
    {
      // Early return due to bad fit
      g_log.warning("Fitting to instrument geometry function failed. ");
      chi2 = DBL_MAX;
      fitstatus = "Minimizer throws exception.";
      return false;
    }

    // 3. Understand solution
    chi2 = fitalg->getProperty("OutputChi2overDoF");
    string tempfitstatus = fitalg->getProperty("OutputStatus");
    fitstatus = tempfitstatus;

    bool goodfit = fitstatus.compare("success") == 0;

    stringstream dbss;
    dbss << "Fit Result (GSL):  Chi^2 = " << chi2
         << "; Fit Status = " << fitstatus << ", Return Bool = " << goodfit << std::endl;
    vector<string> funcparnames = function->getParameterNames();
    for (size_t i = 0; i < funcparnames.size(); ++i)
      dbss << funcparnames[i] << " = " << setw(20) << function->getParameter(funcparnames[i])
           << " +/- " << function->getError(i) << endl;
    g_log.debug() << dbss.str();

    return goodfit;
  }

  //----------------------------------------------------------------------------------------------
  /** Construct an output TableWorkspace for fitting result (profile parameters)
    */
  TableWorkspace_sptr FitPowderPeakParameters::genOutputProfileTable(map<string, Parameter> parameters,
                                                                     double startchi2, double finalchi2)
  {
    // 1. Create TableWorkspace
    TableWorkspace_sptr tablews(new TableWorkspace);

    tablews->addColumn("str", "Name");
    tablews->addColumn("double", "Value");
    tablews->addColumn("str", "FitOrTie");
    tablews->addColumn("double", "Min");
    tablews->addColumn("double", "Max");
    tablews->addColumn("double", "StepSize");
    tablews->addColumn("double", "Error");

    // 2. Set values
    map<string, Parameter>::iterator pariter;
    for (pariter = parameters.begin(); pariter != parameters.end(); ++pariter)
    {
      Parameter& param = pariter->second;
      TableRow newrow = tablews->appendRow();

      string fitortie;
      if (param.fit)
        fitortie = "fit";
      else
        fitortie = "tie";

      newrow << param.name << param.value << fitortie << param.minvalue << param.maxvalue
             << param.stepsize << param.error;
    }

    // 3. Row for Chi^2
    TableRow newrow = tablews->appendRow();
    newrow << "Chi2_Init" << startchi2 << "t" << 0.0 << 0.0 << 0.0 << 0.0;

    newrow = tablews->appendRow();
    newrow << "Chi2_Result" << finalchi2 << "t" << 0.0 << 0.0 << 0.0 << 0.0;

    return tablews;
  }


  //----------------------------------------------------------------------------------------------
  /** Construct output
   */
  Workspace2D_sptr FitPowderPeakParameters::genOutputWorkspace(FunctionDomain1DVector domain,
                                                               FunctionValues rawvalues)
  {
    // 1. Create and set up output workspace
    size_t lenx = m_dataWS->readX(m_wsIndex).size();
    size_t leny = m_dataWS->readY(m_wsIndex).size();

    Workspace2D_sptr outws = boost::dynamic_pointer_cast<Workspace2D>
        (WorkspaceFactory::Instance().create("Workspace2D", 6, lenx, leny));

    outws->getAxis(0)->setUnit("dSpacing");

    TextAxis* taxis = new TextAxis(outws->getNumberHistograms());
    taxis->setLabel(0, "Data");
    taxis->setLabel(1, "Model");
    taxis->setLabel(2, "DiffDM");
    taxis->setLabel(3, "Start");
    taxis->setLabel(4, "DiffDS");
    taxis->setLabel(5, "Zdiff");
    outws->replaceAxis(1, taxis);

    // 3. Re-calculate values
    FunctionValues funcvalues(domain);
    m_positionFunc->function(domain, funcvalues);

    // 4. Add values
    // a) X axis
    for (size_t iws = 0; iws < outws->getNumberHistograms(); ++iws)
    {
      MantidVec& vecX = outws->dataX(iws);
      for (size_t n = 0; n < lenx; ++n)
        vecX[n] = domain[n];
    }

    // b) Y axis
    const MantidVec& dataY = m_dataWS->readY(m_wsIndex);

    for (size_t i = 0; i < domain.size(); ++i)
    {
      outws->dataY(0)[i] = dataY[i];
      outws->dataY(1)[i] = funcvalues[i];
      outws->dataY(2)[i] = dataY[i] - funcvalues[i];
      outws->dataY(3)[i] = rawvalues[i];
      outws->dataY(4)[i] = dataY[i] - rawvalues[i];
    }

    // 5. Zscore
    vector<double> zscore = Kernel::getZscore(outws->readY(2));
    for (size_t i = 0; i < domain.size(); ++i)
      outws->dataY(5)[i] = zscore[i];

    return outws;
  }

  //----------------------------------------------------------------------------------------------
  /** Set parameter values to function from Parameter map
   */
  void FitPowderPeakParameters::setFunctionParameterValues(IFunction_sptr function,
                                                           map<string, Parameter> params)
  {
    // 1. Prepare
    vector<string> funparamnames = function->getParameterNames();

    // 2. Set up
    stringstream msgss;
    msgss << "Set Instrument Function Parameter : " << endl;

    std::map<std::string, Parameter>::iterator paramiter;
    for (size_t i = 0; i < funparamnames.size(); ++i)
    {
      string parname = funparamnames[i];
      paramiter = params.find(parname);

      if (paramiter != params.end())
      {
        // Found, set up the parameter
        Parameter& param = paramiter->second;
        function->setParameter(parname, param.value);

        msgss << setw(10) << parname << " = " << param.value << endl;
      }
      else
      {
        // Not found and thus quit
        stringstream errss;
        errss << "Peak profile parameter " << parname << " is not found in input parameters. ";
        g_log.error(errss.str());
        throw runtime_error(errss.str());
      }
    } // ENDFOR parameter name

    g_log.information(msgss.str());

    return;
  }

  //----------------------------------------------------------------------------------------------
  /** Update parameter values to Parameter map from fuction map
  void FitPowderPeakParameters::updateFunctionParameterValues(IFunction_sptr function,
                                                              map<string, Parameter>& params)
  {
    // 1. Prepare
    vector<string> funparamnames = function->getParameterNames();

    // 2. Set up
    stringstream msgss;
    msgss << "Update Instrument Function Parameter To Storage Map : " << endl;

    std::map<std::string, Parameter>::iterator paramiter;
    for (size_t i = 0; i < funparamnames.size(); ++i)
    {
      string parname = funparamnames[i];
      paramiter = params.find(parname);

      if (paramiter != params.end())
      {
        // Found, set up the parameter
        Parameter& param = paramiter->second;
        param.prevalue = param.value;
        param.value = function->getParameter(parname);

        msgss << setw(10) << parname << " = " << param.value << endl;
      }
    } // ENDFOR parameter name

    g_log.information(msgss.str());

    return;
  }
  */

  //----------------------------------------------------------------------------------------------
  /** Set parameter fitting setup (boundary, fix or unfix) to function from Parameter map
   */
  void FitPowderPeakParameters::setFunctionParameterFitSetups(IFunction_sptr function,
                                                              map<string, Parameter> params)
  {
    // 1. Prepare
    vector<string> funparamnames = m_positionFunc->getParameterNames();

    // 2. Set up
    std::map<std::string, Parameter>::iterator paramiter;
    for (size_t i = 0; i < funparamnames.size(); ++i)
    {
      string parname = funparamnames[i];
      paramiter = params.find(parname);

      if (paramiter != params.end())
      {
        // Found, set up the parameter
        Parameter& param = paramiter->second;
        if (param.fit)
        {
          // If fit.  Unfix it and set up constraint
          function->unfix(i);

          double lowerbound = param.minvalue;
          double upperbound = param.maxvalue;
          if (lowerbound >= -DBL_MAX*0.1 || upperbound <= DBL_MAX*0.1)
          {
            // If there is a boundary
            BoundaryConstraint *bc = new BoundaryConstraint(function.get(), parname, lowerbound,
                                                            upperbound, false);
            function->addConstraint(bc);
          }
        }
        else
        {
          // If fix.
          function->fix(i);
        }
      }
      else
      {
        // Not found and thus quit
        stringstream errss;
        errss << "Peak profile parameter " << parname << " is not found in input parameters. ";
        g_log.error(errss.str());
        throw runtime_error(errss.str());
      }
    } // ENDFOR parameter name

    return;
  }


  //================================= External Functions =========================================

  //----------------------------------------------------------------------------------------------
  /** Convert a vector to a lookup map (dictionary)
   */
  void convertToDict(vector<string> strvec, map<string, size_t>& lookupdict)
  {
    lookupdict.clear();

    for (size_t i = 0; i < strvec.size(); ++i)
      lookupdict.insert(make_pair(strvec[i], i));

    return;
  }

  //----------------------------------------------------------------------------------------------
  /** Get the index from lookup dictionary (map)
   */
  int getStringIndex(map<string, size_t> lookupdict, string key)
  {
    map<string, size_t>::iterator fiter;
    fiter = lookupdict.find(key);

    int returnvalue;

    if (fiter == lookupdict.end())
    {
      // does not exist
      returnvalue = -1;
    }
    else
    {
      // exist
      returnvalue = static_cast<int>(fiter->second);
    }

    return returnvalue;
  }

  //----------------------------------------------------------------------------------------------
  /** Store function parameter values to a map
    */
  void storeFunctionParameterValue(IFunction_sptr function, map<string, pair<double, double> >& parvaluemap)
  {
    parvaluemap.clear();

    vector<string> parnames = function->getParameterNames();
    for (size_t i = 0; i < parnames.size(); ++i)
    {
      string& parname = parnames[i];
      double parvalue = function->getParameter(i);
      double parerror = function->getError(i);
      parvaluemap.insert(make_pair(parname, make_pair(parvalue, parerror)));
    }

    return;
  }

  //----------------------------------------------------------------------------------------------
  /** Restore function parameter values to a map
    */
  void restoreFunctionParameterValue(map<string, pair<double, double> > parvaluemap, IFunction_sptr function,
                                     map<string, Parameter>& parammap)
  {
    vector<string> parnames = function->getParameterNames();

    for (size_t i = 0; i < parnames.size(); ++i)
    {
      string& parname = parnames[i];
      map<string, pair<double, double> >::iterator miter;
      miter = parvaluemap.find(parname);

      if (miter != parvaluemap.end())
      {
        double parvalue = miter->second.first;
        double parerror = miter->second.second;

        // 1. Function
        function->setParameter(parname, parvalue);

        // 2. Parameter map
        map<string, Parameter>::iterator pariter = parammap.find(parname);
        if (pariter != parammap.end())
        {
          // Find the entry
          pariter->second.value = parvalue;
          pariter->second.error = parerror;
        }
      }
    }

    return;
  }

} // namespace CurveFitting
} // namespace Mantid


































