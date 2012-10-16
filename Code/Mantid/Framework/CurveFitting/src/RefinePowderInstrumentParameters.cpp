#include "MantidCurveFitting/RefinePowderInstrumentParameters.h"

#include "MantidKernel/ListValidator.h"
#include "MantidKernel/ArrayProperty.h"

#include "MantidAPI/TableRow.h"
#include "MantidAPI/FunctionDomain1D.h"
#include "MantidAPI/FunctionValues.h"
#include "MantidAPI/IFunction.h"
#include "MantidAPI/IPeakFunction.h"
#include "MantidAPI/ParameterTie.h"
#include "MantidAPI/ConstraintFactory.h"
#include "MantidAPI/FunctionFactory.h"
#include "MantidAPI/TextAxis.h"

#include "MantidCurveFitting/Fit.h"
#include "MantidCurveFitting/BackgroundFunction.h"
#include "MantidCurveFitting/Polynomial.h"
#include "MantidCurveFitting/BoundaryConstraint.h"
#include "MantidCurveFitting/Gaussian.h"

#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/split.hpp>

#include <fstream>
#include <iomanip>

using namespace Mantid;
using namespace Mantid::API;
using namespace Mantid::Kernel;
using namespace Mantid::DataObjects;

using namespace std;

namespace Mantid
{
namespace CurveFitting
{

  DECLARE_ALGORITHM(RefinePowderInstrumentParameters)

  //----------------------------------------------------------------------------------------------
  /** Constructor
   */
  RefinePowderInstrumentParameters::RefinePowderInstrumentParameters()
  {
  }

  //----------------------------------------------------------------------------------------------
  /** Destructor
   */
  RefinePowderInstrumentParameters::~RefinePowderInstrumentParameters()
  {
  }

  //----------------------------------------------------------------------------------------------
  /** Set up documention
    */
  void RefinePowderInstrumentParameters::initDocs()
  {
    setWikiSummary("Refine the instrument geometry related parameters for powder diffractomer. ");
    setOptionalMessage("Parameters include Dtt1, Dtt1t, Dtt2t, Zero, Zerot. ");
  }

  //----------------------------------------------------------------------------------------------
  /** Parameter declaration
   */
  void RefinePowderInstrumentParameters::init()
  {
    // Input/output peaks table workspace
    declareProperty(new API::WorkspaceProperty<DataObjects::TableWorkspace>("InputPeakParameterWorkspace", "Anonymous", Direction::Input),
                    "TableWorkspace containg all peaks' parameters.");

    // Input and output instrument parameters table workspace
    declareProperty(
          new API::WorkspaceProperty<DataObjects::TableWorkspace>
          ("InputInstrumentParameterWorkspace", "AnonymousInstrument", Direction::InOut),
          "TableWorkspace containg instrument's parameters.");

    // Output workspace
    declareProperty(new API::WorkspaceProperty<DataObjects::Workspace2D>("OutputWorkspace", "AnonymousOut", Direction::Output),
                    "Output Workspace2D for the d-TOF curves. ");

    // Workspace to output fitted peak parameters
    declareProperty(new WorkspaceProperty<TableWorkspace>("OutputInstrumentParameterWorkspace", "AnonymousOut2", Direction::Output),
                    "Output TableWorkspace for the fitted peak parameters for each peak.");

    // Workspace to output N best MC parameters
    declareProperty(new WorkspaceProperty<TableWorkspace>("OutputBestResultsWorkspace", "", Direction::Output, PropertyMode::Optional),
                    "Output TableWorkspace for the N best MC fitting results. ");

    // Lower limit on number of peaks for fitting
    declareProperty("MinNumberFittedPeaks", 5,
                    "Minimum number of fitted peaks for refining instrument parameters.");

    // Refinement algorithm
    vector<string> algoptions;
    algoptions.push_back("DirectFit");
    algoptions.push_back("MonteCarlo");
    auto validator = boost::make_shared<Kernel::StringListValidator>(algoptions);
    declareProperty("RefinementAlgorithm", "MonteCarlo", validator,
                    "Algorithm to refine the instrument parameters.");

    declareProperty("RandomWalkSteps", 10000, "Number of Monte Carlo random walk steps. ");

    // Parameters to fit
    declareProperty(new Kernel::ArrayProperty<std::string>("ParametersToFit"),
                    "Names of the parameters to fit. ");

    // Mininum allowed peak's sigma (avoid wrong fitting peak with very narrow width)
    declareProperty("MinSigma", 1.0, "Minimum allowed value for Sigma of a peak.");

    // Method to calcualte the standard error of peaks
    vector<string> stdoptions;
    stdoptions.push_back("ConstantValue");
    stdoptions.push_back("PeakFitting");
    auto listvalidator = boost::make_shared<Kernel::StringListValidator>(stdoptions);
    declareProperty("StandardError", "ConstantValue", listvalidator,
                    "Algorithm to calculate the standard error of peak positions.");

    declareProperty("NumberBestFitRecorded", 1, "Number of best fits (Monte Carlo) recorded and output. ");

    declareProperty("MonteCarloRandomSeed", 0, "Random seed for Monte Carlo simulation. ");

    return;
  }

  //----------------------------------------------------------------------------------------------
  /** Main execution
   */
  void RefinePowderInstrumentParameters::exec()
  {
    enum {DirectFit, MonteCarlo} refinealgorithm;

    // 1. Get input
    DataObjects::TableWorkspace_sptr peakWS = this->getProperty("InputPeakParameterWorkspace");
    DataObjects::TableWorkspace_sptr parameterWS = this->getProperty("InputInstrumentParameterWorkspace");

    mMinSigma = getProperty("MinSigma");

    int tempint = getProperty("MinNumberFittedPeaks");
    if (tempint <= 1)
    {
      g_log.error() << "Input MinNumberFittedPeaks = " << tempint
                    << " is too small. " << endl;
      throw std::invalid_argument("Input MinNumberFittedPeaks is too small.");
    }
    mMinNumFittedPeaks = static_cast<size_t>(tempint);

    tempint = getProperty("NumberBestFitRecorded");
    if (tempint <= 0)
      throw runtime_error("Input NumberBestFitRecorded cannot be less and equal to 0. ");
    mMaxNumberStoredParameters = static_cast<size_t>(tempint);

    string algoption = getProperty("RefinementAlgorithm");
    if (algoption.compare("DirectFit") == 0)
      refinealgorithm = DirectFit;
    else if (algoption.compare("MonteCarlo") == 0)
      refinealgorithm = MonteCarlo;
    else
      throw runtime_error("RefinementAlgorithm other than DirectFit and MonteCarlo are not supported.");

    // 2. Parse input table workspace
    genPeaksFromTable(peakWS);
    importParametersFromTable(parameterWS, mFuncParameters);
    mOrigParameters = mFuncParameters;

    // 3. Generate a cener workspace as function of d-spacing.
    bool usemc = false;
    if (refinealgorithm == MonteCarlo)
      usemc = true;
    genPeakCentersWorkspace(usemc, mMaxNumberStoredParameters);

    // 4. Fit instrument geometry function
    stringstream errss;
    TableWorkspace_sptr mcresultws;

    switch (refinealgorithm)
    {
      case DirectFit:
        // a) Simple (directly) fit all parameters
        fitInstrumentParameters();
        break;

      case MonteCarlo:
        // b) Use Monte Carlo/Annealing method to search global minimum
        refineInstrumentParametersMC(parameterWS);
        mcresultws = genMCResultTable();
        setProperty("OutputBestResultsWorkspace", mcresultws);
        break;

      default:
        // c) Unsupported
        errss << "Refinement algorithm " << algoption << " is not supported.  Quit!";
        g_log.error(errss.str());
        throw invalid_argument(errss.str());
        break;
    }

    // 5. Set output workspace
    this->setProperty("OutputWorkspace", dataWS);

    // 6. Output new instrument parameters
    DataObjects::TableWorkspace_sptr fitparamws = genOutputInstrumentParameterTable();
    this->setProperty("OutputInstrumentParameterWorkspace", fitparamws);

    return;
  }


  //------- Related to Fitting Instrument Geometry Function  -------------------

  /** Fit instrument parameters.  It is a straight forward fitting to
   */
  void RefinePowderInstrumentParameters::fitInstrumentParameters()
  {
    cout << "=========== Method [FitInstrumentParameters] is Under Heavy Construction! ===============" << endl;
    cout << "            Read All FIXME Notes Inside This Method  " << endl;

    // 1. Initialize the fitting function
    CurveFitting::ThermalNeutronDtoTOFFunction rawfunc;
    CurveFitting::ThermalNeutronDtoTOFFunction_sptr mfunc =
        boost::make_shared<CurveFitting::ThermalNeutronDtoTOFFunction>(rawfunc);
    mfunc->initialize();

    // 2. Set up parameters values
    std::vector<std::string> funparamnames = mfunc->getParameterNames();

    std::vector<std::string> paramtofit = getProperty("ParametersToFit");
    std::sort(paramtofit.begin(), paramtofit.end());

    stringstream msgss;
    msgss << "Set Instrument Function Parameter : " << endl;

    std::map<std::string, double>::iterator paramiter;
    for (size_t i = 0; i < funparamnames.size(); ++i)
    {
      string parname = funparamnames[i];
      paramiter = mFuncParameters.find(parname);
      if (paramiter == mFuncParameters.end())
      {
        // Not found and thus skip
        continue;
      }

      double parvalue = paramiter->second;
      mfunc->setParameter(parname, parvalue);
      msgss << setw(10) << parname << " = " << parvalue << endl;
    }

    cout << msgss.str();

    // 3. Fix parameters that are not listed in parameter-to-fit.  Unfix the rest
    size_t numparams = funparamnames.size();
    for (size_t i = 0; i < numparams; ++i)
    {
      string parname = funparamnames[i];
      vector<string>::iterator vsiter;
      vsiter = std::find(paramtofit.begin(), paramtofit.end(), parname);

      if (vsiter == paramtofit.end())
        mfunc->fix(i);
      else
        mfunc->unfix(i);
    }

    // 4. Create and setup fit algorithm
    g_log.information() << "Fit instrument geometry: " << mfunc->asString() << std::endl;

    stringstream outss;
    for (size_t i = 0; i < dataWS->readX(0).size(); ++i)
      outss << dataWS->readX(0)[i] << "\t\t" << dataWS->readY(0)[i] << "\t\t" << dataWS->readE(0)[i] << endl;
    cout << "Input Peak Position Workspace To Fit: " << endl << outss.str() << endl;

    API::IAlgorithm_sptr fitalg = createSubAlgorithm("Fit", 0.0, 0.2, true);
    fitalg->initialize();

    fitalg->setProperty("Function", boost::dynamic_pointer_cast<API::IFunction>(mfunc));
    fitalg->setProperty("InputWorkspace", dataWS);
    fitalg->setProperty("WorkspaceIndex", 0);
    fitalg->setProperty("Minimizer", "Levenberg-MarquardtMD");
    fitalg->setProperty("CostFunction", "Least squares");
    fitalg->setProperty("MaxIterations", 1000);

    bool successfulfit = fitalg->execute();
    if (!fitalg->isExecuted() || ! successfulfit)
    {
      // Early return due to bad fit
      g_log.error() << "Fitting to instrument geometry function failed. " << std::endl;
      throw std::runtime_error("Fitting failed.");
    }

    double chi2 = fitalg->getProperty("OutputChi2overDoF");
    std::string fitstatus = fitalg->getProperty("OutputStatus");

    cout << "Fit geometry function result:  Chi^2 = " << chi2
         << "; Fit Status = " << fitstatus << std::endl;

    API::IFunction_sptr fitfunc = fitalg->getProperty("Function");

    // 4. Set the output data (model and diff)
    API::FunctionDomain1DVector domain(dataWS->readX(1));
    API::FunctionValues values(domain);
    mfunc->function(domain, values);

    for (size_t i = 0; i < domain.size(); ++i)
    {
      dataWS->dataY(1)[i] = values[i];
      dataWS->dataY(2)[i] = dataWS->readY(0)[i] - values[i];
    }

    // 5. Update fitted parameters
    for (size_t i = 0; i < funparamnames.size(); ++i)
    {
      std::string parname = funparamnames[i];
      double parvalue = fitfunc->getParameter(parname);
      mFuncParameters[parname] = parvalue;
    }

    // 6. Pretty screen output
    stringstream dbss;
    dbss << "************ Fit Parameter Result *************" << std::endl;
    for (paramiter = mFuncParameters.begin(); paramiter != mFuncParameters.end(); ++paramiter)
    {
      std::string parname = paramiter->first;
      double inpparvalue = mOrigParameters[parname];
      double parvalue = paramiter->second;
      dbss << setw(20) << parname << " = " << setw(15) << setprecision(6) << parvalue
           << "\t\tFrom " << setw(15) << setprecision(6) << inpparvalue
           << "\t\tDiff = " << inpparvalue-parvalue << endl;
    }
    dbss << "*********************************************" << std::endl;
    cout << dbss.str();

    return;
  }


  /** Refine instrument parameters by Monte Carlo method
    */
  void RefinePowderInstrumentParameters::refineInstrumentParametersMC(TableWorkspace_sptr parameterWS)
  {
    // 1. Get function's parameter names
    getD2TOFFuncParamNames(mPeakFunctionParameterNames);

    // 2. Parse parameter (table) workspace
    vector<double> stepsizes, lowerbounds, upperbounds;
    importMonteCarloParametersFromTable(parameterWS, mPeakFunctionParameterNames, stepsizes, lowerbounds, upperbounds);

    stringstream dbss;
    for (size_t i = 0; i < mPeakFunctionParameterNames.size(); ++i)
    {
      dbss << setw(20) << mPeakFunctionParameterNames[i] << ": Min = " << setw(15) << setprecision(6) << lowerbounds[i]
           << ", Max = " << setw(15) << setprecision(6) << upperbounds[i]
           << ", Step Size = " << setw(15) << setprecision(6) << stepsizes[i] << endl;
    }
    g_log.notice() << "Monte Carlo Parameters: " << endl << dbss.str();

    // 3. Maximum step size
    size_t maxsteps;
    int tempint = getProperty("RandomWalkSteps");
    if (tempint > 0)
      maxsteps = static_cast<size_t>(tempint);
    else
      throw runtime_error("RandomwWalkSteps cannot be less than or equal to 0. ");

    // 4. Random seed and step size rescale factor
    //    FIXME Need to have random seed as an input for better statistic
    int randomseed = getProperty("MonteCarloRandomSeed");
    srand(randomseed);

    double stepsizescalefactor = 1.1;

    // 5. Monte Carlo simulation
    doParameterSpaceRandomWalk(mPeakFunctionParameterNames, lowerbounds, upperbounds, stepsizes, maxsteps,
                               stepsizescalefactor);

    // 6. Record the result
    const MantidVec& X = dataWS->readX(0);
    const MantidVec& Y = dataWS->readY(0);
    const MantidVec& E = dataWS->readE(0);
    FunctionDomain1DVector domain(X);
    FunctionValues values(domain);
    for (size_t i = 0; i < mBestParameters.size(); ++i)
    {
      // a. Set the function with the
      for (size_t j = 0; j < mPeakFunctionParameterNames.size(); ++j)
      {
        mFunction->setParameter(mPeakFunctionParameterNames[j], mBestParameters[i].second[j]);
      }

      // b. Calculate
      calculateD2TOFFunction(domain, values, Y, E);

      // c. Put the data to output workspace
      MantidVec& newY = dataWS->dataY(2*i+1);
      MantidVec& newD = dataWS->dataY(2*i+2);
      for (size_t j = 0; j < newY.size(); ++j)
      {
        newY[j] = values[j];
        newD[j] = Y[j] - values[j];
      }
    }

    return;
  }

  /** Core Monte Carlo random walk on parameter-space
    */
  void RefinePowderInstrumentParameters::doParameterSpaceRandomWalk(vector<string> parnames, vector<double> lowerbounds,
                                                                    vector<double> upperbounds, vector<double> stepsizes,
                                                                    size_t maxsteps, double stepsizescalefactor)
  {
    /* Input Check
    stringstream inpinfo;
    inpinfo << "Input Instrument Parameter Information. Maximum Step = " << maxsteps << endl;
    inpinfo << setw(20) << "Name" << setw(20) << "Value" << setw(20) << "Lower Boundary"
            << setw(20) << "Upper Boundary" << setw(20) << "Step Size" << endl;
    for (size_t i = 0; i < parnames.size(); ++i)
      inpinfo << setw(20) << parnames[i] << setw(20) << mFuncParameters[parnames[i]]
              << setw(20) << lowerbounds[i]
              << setw(20) << upperbounds[i] << setw(20) <<  stepsizes[i] << endl;
    cout << inpinfo.str();
    ------------*/

    // 1. Set up starting values, esp. to mFunction
    size_t numparameters = parnames.size();
    vector<double> paramvalues;
    for (size_t i = 0; i < numparameters; ++i)
    {
      string parname = parnames[i];
      double parvalue = mFuncParameters[parname];
      paramvalues.push_back(parvalue);
      mFunction->setParameter(parname, parvalue);
    }

    const MantidVec& X = dataWS->readX(0);
    const MantidVec& rawY = dataWS->readY(0);
    const MantidVec& rawE = dataWS->readE(0);
    FunctionDomain1DVector domain(X);
    FunctionValues values(domain);

    // 2. Determine the parameters to fit
    vector<string> paramstofit = getProperty("ParametersToFit");
    set<string> paramstofitset;
    bool refineallparams;
    if (paramstofit.size() == 0)
    {
      // Default case to refine all parameters
      refineallparams = true;
    }
    else
    {
      // Refine part of the parameters
      refineallparams = false;
      vector<string>::iterator vsiter;
      for (vsiter = paramstofit.begin(); vsiter != paramstofit.end(); ++vsiter)
      {
        paramstofitset.insert(*vsiter);
      }
    }

    stringstream dbss;
    set<string>::iterator setiter;
    for (setiter = paramstofitset.begin(); setiter != paramstofitset.end(); ++setiter)
    {
      string name = *setiter;
      dbss << setw(20) << name;
    }
    g_log.notice() << "Parameters to refine: " << dbss.str() << endl;

    // 3. Do MC loops
    double curchi2 = calculateD2TOFFunction(domain, values, rawY, rawE);

    g_log.notice() << "Monte Carlo Random Walk Starting Chi^2 = " << curchi2 << endl;

    size_t paramindex = 0;
    size_t numacceptance = 0;
    for (size_t istep = 0; istep < maxsteps; ++istep)
    {
      // a. Determine whether to refine this parameter
      if (!refineallparams)
      {
        if (paramstofitset.count(parnames[paramindex]) == 0)
        {
          // This parameter is not to be refined...
          // cout << "MC Step " << istep << ":  Refine " << parnames[paramindex] << " Denied!" << endl;
          ++ paramindex;
          if (paramindex >= parnames.size())
            paramindex = 0;
          continue;
        }
      }
      // cout << "MC Step " << istep << ":  Refine " << parnames[paramindex] << endl;

      // b. Propose for a new value
      double randomnumber = static_cast<double>(rand())/static_cast<double>(RAND_MAX);
      double newvalue = paramvalues[paramindex] + (randomnumber-1.0)*stepsizes[paramindex];
      if (newvalue > upperbounds[paramindex])
      {
        newvalue = lowerbounds[paramindex] + (newvalue-upperbounds[paramindex]);
      }
      else if (newvalue < lowerbounds[paramindex])
      {
        newvalue = upperbounds[paramindex] - (lowerbounds[paramindex]-newvalue);
      }

      try
      {
        mFunction->setParameter(parnames[paramindex], newvalue);
      }
      catch (runtime_error err)
      {
        stringstream errss;
        errss << "New Value = " << newvalue << ", Random Number = " << randomnumber << "Step Size = " << stepsizes[paramindex]
              << ", Step size rescale factor = " << stepsizescalefactor;
        g_log.error(errss.str());
        throw err;
      }

      // b. Calcualte the new
      double newchi2 = calculateD2TOFFunction(domain, values, rawY, rawE);

      // c. Accept?
      bool accept;
      double prob = exp(-(newchi2-curchi2)/curchi2);
      double randnumber = static_cast<double>(rand())/static_cast<double>(RAND_MAX);

      // cout << "MC Step " << istep << ": New Chi^2 = " << newchi2 << "; Old Chi^2 = " << curchi2
      //      << ".  Acceptance Prob. = " << prob << "; Random = " << randnumber << endl;

      if (randnumber < prob)
      {
        accept = true;
      }
      else
      {
        accept = false;
      }

      // d. Adjust step size
      if (false)
      {
        if (newchi2 < curchi2)
        {
          // Decrease step size if it approaches a minima
          stepsizes[paramindex] = stepsizes[paramindex]/stepsizescalefactor;
        }
        else
        {
          // Increase step size if it diverges
          double newstepsize = stepsizes[paramindex]*stepsizescalefactor;
          double maxstepsize = upperbounds[paramindex]-lowerbounds[paramindex];
          if (newstepsize >= maxstepsize)
          {
            newstepsize = maxstepsize;
          }
          stepsizes[paramindex] = newstepsize;
        }
      }

      // e. Record the solution
      if (accept)
      {
        // i.   Accept the new value
        paramvalues[paramindex] = newvalue;

        // ii.  Add the new values to vector
        vector<double> parametervalues = paramvalues;
        mBestParameters.push_back(make_pair(newchi2, parametervalues));

        // iii. Sort and delete the last if necessary
        sort(mBestParameters.begin(), mBestParameters.end());
        if (mBestParameters.size() > mMaxNumberStoredParameters)
          mBestParameters.pop_back();

        // iv.  Update chi2 and ...
        curchi2 = newchi2;
        ++ numacceptance;
      }

      // z. Update the parameter index for next movement
      ++ paramindex;
      if (paramindex >= numparameters)
        paramindex = 0;

    } // FOREACH MC Step

    // 3. Debug output
    stringstream mcresult;
    mcresult << "Monte Carlo Result for " << mBestParameters.size() << " Best Results" << endl;
    mcresult << "Number of acceptance = " << numacceptance << ", out of " << maxsteps << " MC steps."
             << "Accept ratio = " << static_cast<double>(numacceptance)/static_cast<double>(maxsteps) << endl;
    for (size_t i = 0; i < mBestParameters.size(); ++i)
    {
      mcresult << setw(3) << i << ":  Chi^2 = " << mBestParameters[i].first << endl;
    }
    g_log.notice() << mcresult.str();

    return;
  }

  /** Get the names of the parameters of D-TOF conversion function
    */
  void RefinePowderInstrumentParameters::getD2TOFFuncParamNames(vector<string>& parnames)
  {
    // 1. Clear output
    parnames.clear();

    // 2. Get the parameter names from function
    CurveFitting::ThermalNeutronDtoTOFFunction d2toffunc;
    d2toffunc.initialize();
    std::vector<std::string> funparamnames = d2toffunc.getParameterNames();

    mFunction = boost::make_shared<ThermalNeutronDtoTOFFunction>(d2toffunc);

    // 3. Copy
    parnames = funparamnames;

    return;
  }

  /** Calcualte the function
    */
  double RefinePowderInstrumentParameters::calculateD2TOFFunction(FunctionDomain1DVector domain, FunctionValues &values,
                                                                  const MantidVec& rawY, const MantidVec& rawE)
  {
    // 1. Check validity
    if (!mFunction)
    {
      throw std::runtime_error("mFunction has not been initialized!");
    }
    else
    {
      /*
      vector<string> parnames = mFunction->getParameterNames();
      for (size_t i = 0; i < parnames.size(); ++i)
      {
        cout << "DBx1125  " << parnames[i] << " = " << mFunction->getParameter(parnames[i]) << endl;
      }
      */
      ;
    }

    if (domain.size() != values.size() || domain.size() != rawY.size() || rawY.size() != rawE.size())
    {
      throw std::runtime_error("Input domain, values and raw data have different sizes.");
    }

    // 2. Calculate vlaues
    mFunction->function(domain, values);

    // 3. Calculate the difference
    double chi2 = 0;
    for (size_t i = 0; i < domain.size(); ++i)
    {
      double temp =  (values[i]-rawY[i])/rawE[i];
      chi2 += temp*temp;
      // cout << "Peak " << i << ": Model = " << values[i] << ", Data = " << rawY[i] << ".  Standard Error = " << rawE[i] << endl;
    }

    return chi2;
  }

  //------------------------------- Processing Inputs ----------------------------------------
  /** Genearte peaks from input workspace
    * Peaks are stored in a map.  (HKL) is the key
    */
  void RefinePowderInstrumentParameters::genPeaksFromTable(DataObjects::TableWorkspace_sptr peakparamws)
  {
    // 1. Check and clear input and output
    if (!peakparamws)
    {
      g_log.error() << "Input tableworkspace for peak parameters is invalid!" << std::endl;
      throw std::invalid_argument("Invalid input table workspace for peak parameters");
    }

    mPeaks.clear();

    // 2. Parse table workspace rows to generate peaks
    vector<string> colnames = peakparamws->getColumnNames();
    size_t numrows = peakparamws->rowCount();

    for (size_t ir = 0; ir < numrows; ++ir)
    {
      // a) Generate peak
      CurveFitting::BackToBackExponential newpeak;
      newpeak.initialize();

      // b) Parse parameters
      int h, k, l;
      double alpha, beta, tof_h, sigma, sigma2, chi2, height, dbtemp;
      sigma2 = -1;

      API::TableRow row = peakparamws->getRow(ir);
      for (size_t ic = 0; ic < colnames.size(); ++ic)
      {
        if (colnames[ic].compare("H") == 0)
          row >> h;
        else if (colnames[ic].compare("K") == 0)
          row >> k;
        else if (colnames[ic].compare("L") == 0)
          row >> l;
        else if (colnames[ic].compare("Alpha") == 0)
          row >> alpha;
        else if (colnames[ic].compare("Beta") == 0)
          row >> beta;
        else if (colnames[ic].compare("Sigma2") == 0)
          row >> sigma2;
        else if (colnames[ic].compare("Sigma") == 0)
          row >> sigma;
        else if (colnames[ic].compare("Chi2") == 0)
          row >> chi2;
        else if (colnames[ic].compare("Height") == 0)
          row >> height;
        else if (colnames[ic].compare("TOF_h") == 0)
          row >> tof_h;
        else
        {
          // FIXME Try to set it to a wrong type and see whether it can go on or not!
          row >> dbtemp;
        }
      }

      if (sigma2 > 0)
        sigma = sqrt(sigma2);

      // c) Set peak parameters and etc.
      newpeak.setParameter("A", alpha);
      newpeak.setParameter("B", beta);
      newpeak.setParameter("S", sigma);
      newpeak.setParameter("X0", tof_h);
      newpeak.setParameter("I", height);

      // d) Make to share pointer and set to instance data structure (map)
      CurveFitting::BackToBackExponential_sptr newpeakptr =
          boost::make_shared<CurveFitting::BackToBackExponential>(newpeak);

      std::vector<int> hkl;
      hkl.push_back(h); hkl.push_back(k); hkl.push_back(l);

      mPeaks.insert(std::make_pair(hkl, newpeakptr));

      mPeakErrors.insert(make_pair(hkl, chi2));

      g_log.information() << "[GeneratePeaks] Peak " << ir << " HKL = [" << hkl[0] << ", " << hkl[1] << ", "
                          << hkl[2] << "], Input Center = " << setw(10) << setprecision(6) << newpeak.centre()
                          << endl;

    } // ENDFOR Each potential peak

    return;
  }

  /** Import TableWorkspace containing the instrument parameters for fitting
   * the diffrotometer geometry parameters
   */
  void RefinePowderInstrumentParameters::importParametersFromTable(
          DataObjects::TableWorkspace_sptr parameterWS, std::map<std::string, double>& parameters)
  {
    // 1. Check column orders
    std::vector<std::string> colnames = parameterWS->getColumnNames();
    if (colnames.size() < 2)
    {
      g_log.error() << "Input parameter table workspace does not have enough number of columns. "
                    << " Number of columns = " << colnames.size() << " < 3 as required. " << std::endl;
      throw std::runtime_error("Input parameter workspace is wrong. ");
    }

    if (colnames[0].compare("Name") != 0 ||
        colnames[1].compare("Value") != 0)
    {
      g_log.error() << "Input parameter table workspace does not have the columns in order.  "
                    << " It must be Name, Value, FitOrTie." << std::endl;
      throw std::runtime_error("Input parameter workspace is wrong. ");
    }

    // 2. Import data to maps
    std::string parname;
    double value;

    size_t numrows = parameterWS->rowCount();

    for (size_t ir = 0; ir < numrows; ++ir)
    {
      try
      {
        API::TableRow trow = parameterWS->getRow(ir);
        trow >> parname >> value;
        parameters.insert(std::make_pair(parname, value));
      }
      catch (runtime_error err)
      {
        g_log.error() << "Import table workspace " << parameterWS->name() << " error in line " << ir << ".  "
                      << " Requires [string, double] in the first 2 columns." << endl;
        throw err;
      }
    }

    return;
  }

  /** Import the Monte Carlo related parameters from table
    * Arguments
    */
  void RefinePowderInstrumentParameters::importMonteCarloParametersFromTable(
      TableWorkspace_sptr tablews, vector<string> parameternames, vector<double>& stepsizes,
      vector<double>& lowerbounds, vector<double>& upperbounds)
  {
    // 1. Get column information
    vector<string> colnames = tablews->getColumnNames();
    size_t imax, imin, istep;
    imax = colnames.size()+1;
    imin = imax;
    istep = imax;

    for (size_t i = 0; i < colnames.size(); ++i)
    {
      if (colnames[i].compare("Max") == 0)
        imax = i;
      else if (colnames[i].compare("Min") == 0)
        imin = i;
      else if (colnames[i].compare("StepSize") == 0)
        istep = i;
    }

    if (imax > colnames.size() || imin > colnames.size() || istep > colnames.size())
    {
      stringstream errss;
      errss << "Input parameter workspace misses information for Monte Carlo minimizer. "
            << "One or more of the following columns are missing (Max, Min, StepSize).";
      g_log.error(errss.str());
      throw runtime_error(errss.str());
    }

    // 2. Parse input to a map
    stepsizes.clear();
    lowerbounds.clear();
    upperbounds.clear();

    map<string, vector<double> > mcparameters;
    size_t numrows = tablews->rowCount();
    for (size_t ir = 0; ir < numrows; ++ir)
    {
      TableRow row = tablews->getRow(ir);
      string parname;
      double tmax, tmin, tstepsize;
      row >> parname;
      for (size_t ic = 1; ic < colnames.size(); ++ic)
      {
        double tmpdbl;
        string tmpstr;
        try
        {
          row >> tmpdbl;
        }
        catch (runtime_error err)
        {
          g_log.error() << "Import MC parameter " << colnames[ic] << " error in row " << ir
                        << " of workspace " << tablews->name() << endl;
          row >> tmpstr;
          g_log.error() << "Should be " << tmpstr << endl;
        }

        if (ic == imax)
          tmax = tmpdbl;
        else if (ic == imin)
          tmin = tmpdbl;
        else if (ic == istep)
          tstepsize = tmpdbl;
      }
      vector<double> tmpvec;
      tmpvec.push_back(tmin);
      tmpvec.push_back(tmax);
      tmpvec.push_back(tstepsize);
      mcparameters.insert(make_pair(parname, tmpvec));
    }

    // 3. Retrieve the information for geometry parameters
    map<string, vector<double> >::iterator mit;
    for (size_t i = 0; i < parameternames.size(); ++i)
    {
      // a) Get on hold of the MC parameter vector
      string parname = parameternames[i];
      mit = mcparameters.find(parname);
      if (mit == mcparameters.end())
      {
        // Not found the parameter.  raise error!
        stringstream errss;
        errss << "Input instrument parameter workspace does not have parameter " << parname
              << ".  Information is incomplete for Monte Carlo simulation." << endl;
        g_log.error(errss.str());
        throw runtime_error(errss.str());
      }
      vector<double> mcparvalues = mit->second;

      // b) Build for the output
      lowerbounds.push_back(mcparvalues[0]);
      upperbounds.push_back(mcparvalues[1]);
      stepsizes.push_back(mcparvalues[2]);
    }

    return;
  }


  /** Calculate thermal neutron's d-spacing
    */
  double RefinePowderInstrumentParameters::calculateDspaceValue(std::vector<int> hkl, double lattice)
  {    
    // FIXME  It only works for the assumption that the lattice is cubical
    double h = static_cast<double>(hkl[0]);
    double k = static_cast<double>(hkl[1]);
    double l = static_cast<double>(hkl[2]);

    double d = lattice/sqrt(h*h+k*k+l*l);

    return d;
  }

  //------- Related to Algorith's Output  -------------------------------------------------------------------

  /** Get peak positions from peak functions
    * Arguments:
    * Output: outWS  1 spectrum .  dspacing - peak center
   */
  void RefinePowderInstrumentParameters::genPeakCentersWorkspace(bool montecarlo, size_t numbestfit)
  {
    // 1. Collect values in a vector for sorting
    double lattice = mFuncParameters["LatticeConstant"];
    if (lattice < 1.0E-5)
    {
      std::stringstream errmsg;
      errmsg << "Input Lattice constant = " << lattice << " is wrong or not set up right. ";
      throw std::invalid_argument(errmsg.str());
    }

    string stdoption = getProperty("StandardError");
    bool useconstanterror = false;
    if (stdoption.compare("ConstantValue") == 0)
    {
      useconstanterror = true;
    }

    std::map<std::vector<int>, CurveFitting::BackToBackExponential_sptr>::iterator peakiter;
    std::vector<std::pair<double, std::pair<double, double> > > peakcenters; // d_h [TOF_h, CHI2]

    for (peakiter = mPeaks.begin(); peakiter != mPeaks.end(); ++peakiter)
    {
      vector<int> hkl = peakiter->first;
      BackToBackExponential_sptr peak = peakiter->second;

      double sigma = peak->getParameter("S");
      if (sigma < mMinSigma)
      {
        g_log.information() << "Peak (" << hkl[0] << ", " << hkl[1] << ", " << hkl[2]
                            << ") has unphysically small Sigma = " << sigma << ".  "
                            << "It is thus excluded. " << endl;
        continue;
      }

      double dh = calculateDspaceValue(hkl, lattice);
      double center = peak->centre();
      double height = peak->height();
      double chi2;
      if (useconstanterror)
      {
        chi2 = 1.0;
      }
      else
      {
        chi2 = sqrt(mPeakErrors[hkl])/height;
      }

      peakcenters.push_back(make_pair(dh, make_pair(center, chi2)));
    }

    // 2. Sort by d-spacing value
    std::sort(peakcenters.begin(), peakcenters.end());

    // 3. Create output workspace
    size_t size = peakcenters.size();
    size_t nspec;
    if (montecarlo)
    {
      // Monte Carlo, 1 + 2*N spectra:  raw, N x (refined, diff)
      nspec = 1+2*numbestfit;
    }
    else
    {
      // Regular fit, 3 spectra:  raw, refined, diff
      nspec = 3;
    }

    dataWS = boost::dynamic_pointer_cast<DataObjects::Workspace2D>
        (API::WorkspaceFactory::Instance().create("Workspace2D", nspec, size, size));
    dataWS->getAxis(0)->setUnit("dSpacing");

    // 4. Put data to output workspace
    for (size_t i = 0; i < peakcenters.size(); ++i)
    {
      for (size_t j = 0; j < nspec; ++j)
      {
        dataWS->dataX(j)[i] = peakcenters[i].first;
      }
      dataWS->dataY(0)[i] = peakcenters[i].second.first;
      dataWS->dataE(0)[i] = peakcenters[i].second.second;
    }

    return;
  }

  /** Generate a Monte Carlo result table containing the N best results
    * Column: chi2, parameter1, parameter2, ... ...
    */
  DataObjects::TableWorkspace_sptr RefinePowderInstrumentParameters::genMCResultTable()
  {
    // 1. Create table workspace
    DataObjects::TableWorkspace_sptr tablews =
        boost::shared_ptr<TableWorkspace>(new TableWorkspace());

    tablews->addColumn("double", "Chi2");
    for (size_t i = 0; i < mPeakFunctionParameterNames.size(); ++i)
    {
      tablews->addColumn("double", mPeakFunctionParameterNames[i]);
    }

    // 2. Put values in
    for (size_t ib = 0; ib < mBestParameters.size(); ++ib)
    {
      TableRow newrow = tablews->appendRow();
      double chi2 = mBestParameters[ib].first;
      newrow << chi2;
      for (size_t ip = 0; ip < mPeakFunctionParameterNames.size(); ++ip)
      {
        double tempdbl = mBestParameters[ib].second[ip];
        newrow << tempdbl;
      }
    } // ENDFOR 1 Best Answer

    return tablews;
  }

  /** Generate an output table workspace containing the fitted instrument parameters.
    * Requirement (1): The output table workspace should be usable by Le Bail Fitting
    */
  DataObjects::TableWorkspace_sptr RefinePowderInstrumentParameters::genOutputInstrumentParameterTable()
  {
    //  TableWorkspace is not copyable (default CC is incorrect and no point in writing a non-default one)
    DataObjects::TableWorkspace_sptr newtablews =
        boost::shared_ptr<DataObjects::TableWorkspace>(new DataObjects::TableWorkspace());
    newtablews->addColumn("str", "Name");
    newtablews->addColumn("double", "Value");
    newtablews->addColumn("str", "FitOrTie");

    std::map<std::string, double>::iterator pariter;

    for (pariter = mFuncParameters.begin(); pariter != mFuncParameters.end(); ++pariter)
    {
      API::TableRow newrow = newtablews->appendRow();
      std::string parname = pariter->first;
      double parvalue = pariter->second;
      newrow << parname << parvalue;
    }

    return newtablews;
  }

} // namespace CurveFitting
} // namespace Mantid
