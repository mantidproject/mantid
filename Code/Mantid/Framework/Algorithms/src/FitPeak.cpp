/*WIKI*
 This algorithm ...

 The output [[TableWorkspace]] contains the following columns...

 ==== Subalgorithms used ====
 ...

 ==== Treating weak peaks vs. high background ====
 FindPeaks uses a more complicated approach to fit peaks if '''HighBackground''' is flagged. In this case, FindPeak will fit the background first, and then do a Gaussian fit the peak with the fitted background removed.  This procedure will be repeated for a couple of times with different guessed peak widths.  And the parameters of the best result is selected.  The last step is to fit the peak with a combo function including background and Gaussian by using the previously recorded best background and peak parameters as the starting values.

 ==== Criteria To Validate Peaks Found ====
 FindPeaks finds peaks by fitting a Guassian with background to a certain range in the input histogram.  [[Fit]] may not give a correct result even if chi^2 is used as criteria alone.  Thus some other criteria are provided as options to validate the result
 1. Peak position.  If peak positions are given, and trustful, then the fitted peak position must be within a short distance to the give one.
 2. Peak height.  In the certain number of trial, peak height can be used to select the best fit among various starting sigma values.

 ==== Fit Window ====
 If FitWindows is defined, then a peak's range to fit (i.e., x-min and x-max) is confined by this window.

 If FitWindows is defined, starting peak centres are NOT user's input, but found by highest value within peak window. (Is this correct???)


 *WIKI*/
//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAlgorithms/FitPeak.h"
#include "MantidAPI/CompositeFunction.h"
#include "MantidAPI/CostFunctionFactory.h"
#include "MantidAPI/FunctionProperty.h"
#include "MantidAPI/FunctionFactory.h"
#include "MantidAPI/FuncMinimizerFactory.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidAPI/WorkspaceProperty.h"
#include "MantidAPI/TableRow.h"
#include "MantidKernel/ListValidator.h"
#include "MantidKernel/IValidator.h"
#include "MantidKernel/StartsWithValidator.h"
#include "MantidDataObjects/TableWorkspace.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/BoundedValidator.h"

#include "MantidAPI/MultiDomainFunction.h"

#include "boost/algorithm/string.hpp"
#include "boost/algorithm/string/trim.hpp"

using namespace Mantid;
using namespace Mantid::API;
using namespace Mantid::DataObjects;
using namespace Mantid::Kernel;

using namespace std;

const double MAGICNUMBER = 2.0;

namespace Mantid
{
namespace Algorithms
{

  DECLARE_ALGORITHM(FitPeak)

  //----------------------------------------------------------------------------------------------
  /** Constructor
   */
  FitPeak::FitPeak()
  {
    m_minimizer = "Levenberg-MarquardtMD";
    m_bestRwp = DBL_MAX;
  }
    
  //----------------------------------------------------------------------------------------------
  /** Destructor
   */
  FitPeak::~FitPeak()
  {
  }

  //----------------------------------------------------------------------------------------------
  /** Document
   */
  void FitPeak::initDocs()
  {
    setWikiSummary("");
    setOptionalMessage("");

    return;
  }

  //----------------------------------------------------------------------------------------------
  /** Declare properties
   */
  void FitPeak::init()
  {
    declareProperty(new WorkspaceProperty<MatrixWorkspace>("InputWorkspace", "", Direction::Input),
                    "Name of the input workspace for peak fitting.");

    declareProperty(new WorkspaceProperty<MatrixWorkspace>("OutputWorkspace", "", Direction::Output),
                    "Name of the output workspace containing fitted peak.");

    declareProperty(new WorkspaceProperty<TableWorkspace>("ParameterTableWorkspace", "", Direction::Output),
                    "Name of the table workspace containing the fitted parameters. ");

    boost::shared_ptr<BoundedValidator<int> > mustBeNonNegative = boost::make_shared<BoundedValidator<int> >();
    mustBeNonNegative->setLower(0);
    declareProperty("WorkspaceIndex", 0, mustBeNonNegative, "Workspace index ");

    std::vector<std::string> peakNames = FunctionFactory::Instance().getFunctionNames<IPeakFunction>();
    vector<string> peakFullNames = addFunctionParameterNames(peakNames);
    declareProperty("PeakFunctionType", "", boost::make_shared<StringListValidator>(peakFullNames),
                    "Peak function type. ");

    declareProperty(new ArrayProperty<string>("PeakParameterNames"),
                    "List of peak parameter names. ");

    declareProperty(new ArrayProperty<double>("PeakParameterValues"),
                    "List of peak parameter values.  They must have a 1-to-1 mapping to PeakParameterNames list. ");

    declareProperty(new ArrayProperty<double>("FittedPeakParameterValues", Direction::Output),
                    "Fitted peak parameter values. ");

    vector<string> bkgdtypes;
    bkgdtypes.push_back("Flat");
    bkgdtypes.push_back("Linear");
    bkgdtypes.push_back("Quadratic");
    declareProperty("BackgroundType", "Linear", boost::make_shared<StringListValidator>(bkgdtypes),
                    "Type of Background.");

    declareProperty(new ArrayProperty<string>("BackgroundParameterNames"),
                    "List of background parameter names. ");

    declareProperty(new ArrayProperty<double>("BackgroundParameterValues"),
                    "List of background parameter values.  "
                    "They must have a 1-to-1 mapping to BackgroundParameterNames list. ");

    declareProperty(new ArrayProperty<double>("FittedBackgroundParameterValues", Direction::Output),
                    "Fitted background parameter values. ");

    declareProperty(new ArrayProperty<double>("FitWindow"),
                    "Enter a comma-separated list of the expected X-position of windows to fit. "
                    "The number of values must be 2.");

    declareProperty(new ArrayProperty<double>("PeakRange"),
                    "Enter a comma-separated list of expected x-position as peak range. "
                    "The number of values must be 2.");

    declareProperty("FitBackgroundFirst", true, "If true, then the algorithm will fit background first. "
                    "And then the peak. ");

    declareProperty("RawParams", true, "If true, then the output table workspace contains the raw profile parameter. "
                    "Otherwise, the effective parameters will be written. ");

    auto mustBePositive = boost::make_shared<BoundedValidator<int> >();
    mustBePositive->setLower(1);
    declareProperty("MinGuessedPeakWidth", 2, mustBePositive,
                    "Minimum guessed peak width for fit. It is in unit of number of pixels.");

    declareProperty("MaxGuessedPeakWidth", 10, mustBePositive,
                    "Maximum guessed peak width for fit. It is in unit of number of pixels.");

    declareProperty("GuessedPeakWidthStep", EMPTY_INT(), mustBePositive,
                    "Step of guessed peak width. It is in unit of number of pixels.");

    auto mustBePostiveDbl = boost::make_shared<BoundedValidator<double> >();
    mustBePostiveDbl->setLower(DBL_MIN);
    declareProperty("PeakPositionTolerance", EMPTY_DBL(), mustBePostiveDbl,
                    "Peak position tolerance.  If fitted peak's position differs from proposed value more than "
                    "the given value, fit is treated as failure. ");

    vector<string> costFuncOptions;
    costFuncOptions.push_back("Chi-Square");
    costFuncOptions.push_back("Rwp");
    declareProperty("CostFunction","Chi-Square",
                    Kernel::IValidator_sptr(new Kernel::ListValidator<std::string>(costFuncOptions)),
                    "Cost functions");

    std::vector<std::string> minimizerOptions = API::FuncMinimizerFactory::Instance().getKeys();

    declareProperty("Minimizer", "Levenberg-Marquardt",
      Kernel::IValidator_sptr(new Kernel::StartsWithValidator(minimizerOptions)),
      "Minimizer to use for fitting. Minimizers available are \"Levenberg-Marquardt\", \"Simplex\","
                    "\"Conjugate gradient (Fletcher-Reeves imp.)\", \"Conjugate gradient (Polak-Ribiere imp.)\", \"BFGS\", and \"Levenberg-MarquardtMD\"");

    declareProperty("CostFunctionValue", DBL_MAX, "Value of cost function of the fitted peak. ", Kernel::Direction::Output);

    return;
  }

  //----------------------------------------------------------------------------------------------
  /** Declare properties
   */
  void FitPeak::exec()
  {
    // Get input properties
    processProperties();

    // Create functions
    createFunctions();

    // Check input function, guessed value, and etc.
    prescreenInputData();

    // Fit peak
    if (m_fitBkgdFirst)
    {
      fitPeakMultipleStep();
    }
    else
    {
      fitPeakOneStep();
    }

    // Output
    setupOutput();

    return;
  }

  //----------------------------------------------------------------------------------------------
  /** Add function's parameter names after peak function name
    */
  std::vector<std::string> FitPeak::addFunctionParameterNames(std::vector<std::string> funcnames)
  {
    vector<string> vec_funcparnames(funcnames.size());

    for (size_t i = 0; i < funcnames.size(); ++i)
    {
      // Add original name in
      vec_funcparnames.push_back(funcnames[i]);

      // Add a full function name and parameter names in
      IFunction_sptr tempfunc = FunctionFactory::Instance().createFunction(funcnames[i]);

      stringstream parnamess;
      parnamess << funcnames[i] << " (";
      vector<string> funcpars = tempfunc->getParameterNames();
      for (size_t j = 0; j < funcpars.size(); ++j)
      {
        parnamess << funcpars[j];
        if (j != funcpars.size()-1)
          parnamess << ", ";
      }
      parnamess << ")";

      vec_funcparnames.push_back(parnamess.str());
    }

    return vec_funcparnames;
  }


  //----------------------------------------------------------------------------------------------
  /** Process input properties
    */
  void FitPeak::processProperties()
  {
    // Data workspace (input)
    m_dataWS = getProperty("InputWorkspace");
    int tempint = getProperty("WorkspaceIndex");
    m_wsIndex = static_cast<size_t>(tempint);

    // Fit window
    const MantidVec& vecX = m_dataWS->readX(m_wsIndex);

    vector<double> fitwindow = getProperty("FitWindow");
    if (fitwindow.size() != 2)
    {
      throw runtime_error("Must enter 2 and only 2 items in fit window. ");
    }
    m_minFitX = fitwindow[0];
    if (m_minFitX < vecX.front())
      m_minFitX = vecX.front();
    m_maxFitX = fitwindow[1];
    if (m_maxFitX > vecX.back())
      m_maxFitX = vecX.back();

    if (m_maxFitX <= m_minFitX)
    {
      stringstream errss;
      errss << "Minimum X (" << m_minFitX << ") is larger and equal to maximum X ("
            << m_maxFitX << ") to fit.  It is not allowed. ";
      g_log.error(errss.str());
      throw runtime_error(errss.str());
    }

    // Peak range
    vector<double> peakrange = getProperty("PeakRange");
    if (peakrange.size() != 2)
    {
      throw runtime_error("Must enter 2 and only 2 items in fit window. ");
    }
    m_minPeakX = peakrange[0];
    m_maxPeakX = peakrange[1];
    if (m_maxFitX <= m_minFitX)
    {
      stringstream errss;
      errss << "Minimum peak range (" << m_minPeakX << ") is larger and equal to maximum X ("
            << m_maxPeakX << ") of the range of peak.  It is not allowed. ";
      g_log.error(errss.str());
      throw runtime_error(errss.str());
    }

    if (m_minPeakX < m_minFitX)
    {
      g_log.warning() << "Minimum peak range is out side of the lower boundary of fit window.  ";
    }
    if (m_maxPeakX > m_maxFitX)
    {
      g_log.warning() << "Maximum peak range is out side of the upper boundary of fit window. ";
    }

    i_minFitX = getVectorIndex(vecX, m_minFitX);
    i_maxFitX = getVectorIndex(vecX, m_maxFitX);
    i_minPeakX = getVectorIndex(vecX, m_minPeakX);
    i_maxPeakX = getVectorIndex(vecX, m_maxPeakX);

    //
    m_fitBkgdFirst = getProperty("FitBackgroundFirst");

    //
    m_outputRawParams = getProperty("RawParams");

    // Trying FWHM in a certain range
    m_minGuessedPeakWidth = getProperty("MinGuessedPeakWidth");
    m_maxGuessedPeakWidth = getProperty("MaxGuessedPeakWidth");
    m_fwhmFitStep = getProperty("GuessedPeakWidthStep");
    if (isEmpty(m_fwhmFitStep))
      m_fitWithStepPeakWidth = false;
    else
    {
      m_fitWithStepPeakWidth = true;
      if (m_minGuessedPeakWidth > m_maxGuessedPeakWidth)
      {
        std::stringstream errss;
        errss << "User specified wrong guessed peak width parameters (must be postive and make sense). "
              << "User inputs are min = " << m_minGuessedPeakWidth << ", max = " << m_maxGuessedPeakWidth
              << ", step = " << m_fwhmFitStep;
        g_log.error(errss.str());
        throw std::runtime_error(errss.str());
      }
    }

    m_peakPositionTolerance = getProperty("PeakPositionTolerance");
    if (isEmpty(m_peakPositionTolerance))
      m_usePeakPositionTolerance = false;
    else
      m_usePeakPositionTolerance = true;

    // Cost function
    string costfunname = getProperty("CostFunction");
    if (costfunname == "Chi-Square")
    {
      m_costFunction = "Least squares";
    }
    else if (costfunname == "Rwp")
    {
      m_costFunction = "Rwp";
    }
    else
    {
      g_log.error() << "Cost function " << costfunname << " is not supported. " << "\n";
      throw runtime_error("Cost function is not supported. ");
    }

    // Minimizer
    m_minimizer = getPropertyValue("Minimizer");

    return;
  }


  //----------------------------------------------------------------------------------------------
  /** Create functions from input properties
    */
  void FitPeak::createFunctions()
  {
    //=========================================================================
    // Generate background function
    //=========================================================================
    string bkgdtype = getPropertyValue("BackgroundType");
    // FIXME - Fix the inconsistency in nameing the background
    if (bkgdtype == "Flat" || bkgdtype == "Linear")
      bkgdtype += "Background";

    m_bkgdFunc = boost::dynamic_pointer_cast<IBackgroundFunction>(
          FunctionFactory::Instance().createFunction(bkgdtype));
    g_log.debug() << "Created background function of type " << bkgdtype << "\n";

    // Set background function parameter values
    vector<string> vec_bkgdparnames = getProperty("BackgroundParameterNames");
    vector<double> vec_bkgdparvalues = getProperty("BackgroundParameterValues");
    if (vec_bkgdparnames.size() != vec_bkgdparvalues.size() || vec_bkgdparnames.size() == 0)
    {
      stringstream errss;
      errss << "Input background properties' arrays are incorrect: # of parameter names = " << vec_bkgdparnames.size()
            << ", # of parameter values = " << vec_bkgdparvalues.size() << "\n";
      g_log.error(errss.str());
      throw runtime_error(errss.str());
    }

    // Set parameter values
    for (size_t i = 0; i < vec_bkgdparnames.size(); ++i)
    {
      m_bkgdFunc->setParameter(vec_bkgdparnames[i], vec_bkgdparvalues[i]);
    }

    //=========================================================================
    // Generate peak function
    //=========================================================================
    string peaktypeprev = getPropertyValue("PeakFunctionType");
    bool defaultparorder = true;
    string peaktype = parsePeakTypeFull(peaktypeprev, defaultparorder);
    m_peakFunc = boost::dynamic_pointer_cast<IPeakFunction>(
          FunctionFactory::Instance().createFunction(peaktype));
    g_log.debug() << "Create peak function of type " << peaktype << "\n";

    // Peak parameters' names
    m_peakParameterNames = getProperty("PeakParameterNames");
    if (m_peakParameterNames.size() == 0)
    {
      if (defaultparorder)
      {
        // Use default peak parameter names' order
        m_peakParameterNames = m_peakFunc->getParameterNames();
      }
      else
      {
        throw runtime_error("Peak parameter names' input is not in default mode. "
                            "It cannot be left empty. ");
      }
    }

    // Peak parameters' value
    vector<double> vec_peakparvalues = getProperty("PeakParameterValues");
    if (m_peakParameterNames.size() != vec_peakparvalues.size())
    {
      stringstream errss;
      errss << "Input peak parameters' names (" << m_peakParameterNames.size()
            << ") and values (" << vec_peakparvalues.size() << ") have different numbers. ";
      throw runtime_error(errss.str());
    }

    // Set peak parameter values
    for (size_t i = 0; i < m_peakParameterNames.size(); ++i)
    {
      m_peakFunc->setParameter(m_peakParameterNames[i], vec_peakparvalues[i]);
    }

    return;
  }

  //----------------------------------------------------------------------------------------------
  /** Parse peak type from full peak type/parameter names string
    */
  std::string FitPeak::parsePeakTypeFull(const std::string& fullstring, bool& defaultparorder)
  {
    string peaktype;

    size_t n = std::count(fullstring.begin(), fullstring.end(), '(');
    if (n > 0)
    {
      peaktype = fullstring.substr(0, fullstring.find("("));
      boost::algorithm::trim(peaktype);
      defaultparorder = true;
    }
    else
    {
      peaktype = fullstring;
      defaultparorder = false;
    }

    return peaktype;

  }

  //----------------------------------------------------------------------------------------------
  /// Fit peak in one step
  void FitPeak::fitPeakOneStep()
  {
    // Set up a composite function
    CompositeFunction_sptr compfunc = boost::make_shared<CompositeFunction>();
    compfunc->addFunction(m_peakFunc);
    compfunc->addFunction(m_bkgdFunc);

    g_log.information() << "One-Step-Fit Function: " << compfunc->asString() << "\n";

    // Set up a list of guessed FWHM
    // Calculate guessed FWHM
    std::vector<double> vec_FWHM;
    setupGuessedFWHM(vec_FWHM);

    // Store starting setup
    map<string, double> temperrormap;
    push(m_peakFunc, m_bkupPeakFunc, temperrormap);
    push(m_bkgdFunc, m_bkupBkgdFunc, temperrormap);

    // Fit with different starting values of peak width
    size_t numfits = vec_FWHM.size();
    for (size_t i = 0; i < numfits; ++i)
    {
      // set FWHM
      g_log.debug() << "[SingleStepFit] FWHM = " << vec_FWHM[i] << "\n";
      m_peakFunc->setFwhm(vec_FWHM[i]);

      // fit and process result
      double goodndess = fitFunctionSD(compfunc, m_dataWS, m_wsIndex, m_minFitX, m_maxFitX, false);
      processNStoreFitResult(goodndess, true);

      // restore the function parameters
      if (i != numfits-1)
      {
        pop(m_bkupPeakFunc, m_peakFunc);
        pop(m_bkupBkgdFunc, m_bkgdFunc);
      }
    }

    // Retrieve the best result stored
    pop(m_bestPeakFunc, m_peakFunc);
    pop(m_bestBkgdFunc, m_bkgdFunc);
    m_finalGoodnessValue = m_bestRwp;

    g_log.information() << "One-Step-Fit Best Fitted Function: " << compfunc->asString() << "\n";

    return;
  }

  //----------------------------------------------------------------------------------------------
  /** Fit peak in a robust manner.  Multiple fit will be
    */
  void FitPeak::fitPeakMultipleStep()
  {
    // Fit background
    m_bkgdFunc = fitBackground(m_bkgdFunc);

    // Backup original data due to pure peak data to be made
    backupOriginalData();

    // Make pure peak
    makePurePeakWS();

    // Estimate the peak height
    double est_peakheight = estimatePeakHeight(m_peakFunc, m_dataWS, m_wsIndex, m_minFitX, m_maxFitX);
    m_peakFunc->setHeight(est_peakheight);

    // Calculate guessed FWHM
    std::vector<double> vec_FWHM;
    setupGuessedFWHM(vec_FWHM);

    // Store starting setup
    map<string, double> bkupPeakErrorMap;
    push(m_peakFunc, m_bkupPeakFunc, bkupPeakErrorMap);

    // Fit with different starting values of peak width
    for (size_t i = 0; i < vec_FWHM.size(); ++i)
    {
      // Restore
      if (i > 0)
        pop(m_bkupPeakFunc, m_peakFunc);

      // Set FWHM
      m_peakFunc->setFwhm(vec_FWHM[i]);
      g_log.debug() << "Round " << i << " of " << vec_FWHM.size() << ". Using proposed FWHM = "
                    << vec_FWHM[i] << "\n";

      // Fit
      double rwp = fitPeakFunction(m_peakFunc, m_dataWS, m_wsIndex, m_minFitX, m_maxFitX);

      // Store result
      processNStoreFitResult(rwp,false);
    }

    // Make a combo fit

    // Get best fitting peak function
    pop(m_bestPeakFunc, m_peakFunc);
    g_log.information() << "MultStep-Fit: Best Fitted Peak: " << m_peakFunc->asString() << "\n";

    // Recover the original Y value from pure peak data range
    recoverOriginalData();

    m_finalGoodnessValue = fitCompositeFunction(m_peakFunc, m_bkgdFunc, m_dataWS, m_wsIndex, m_minFitX, m_maxFitX);
    g_log.information() << "Final " << m_costFunction << " = " << m_finalGoodnessValue << "\n";

    return;
  }


  //----------------------------------------------------------------------------------------------
  /** Check input data and get some information parameters
    */
  void FitPeak::prescreenInputData()
  {
    // Check functions
    if (!m_peakFunc || !m_bkgdFunc)
      throw runtime_error("Either peak function or background function has not been set up.");

    // Check validity on peak centre
    double centre_guess = m_peakFunc->centre();
    g_log.debug() << "Fit Peak with given window:  Guessed center = " << centre_guess
                        << "  x-min = " << m_minFitX
                        << ", x-max = " << m_maxFitX << "\n";
    if (m_minFitX >= centre_guess || m_maxFitX <= centre_guess)
    {
      g_log.error("Peak centre is out side of fit window.");
      throw runtime_error("Peak centre is out side of fit window. ");
    }

    // Peak width and centre: from user input
    m_userGuessedFWHM = m_peakFunc->fwhm();
    m_userPeakCentre = m_peakFunc->centre();

    return;
  }

  //----------------------------------------------------------------------------------------------
  /** Set up the output workspaces
    * including (1) data workspace (2) function parameter workspace
    */
  void FitPeak::setupOutput()
  {
    // Data workspace
    size_t nspec = 3;
    // Get a vector for fit window
    const MantidVec& vecX = m_dataWS->readX(m_wsIndex);
    size_t vecsize = i_maxFitX - i_minFitX + 1;
    vector<double> vecoutx(vecsize);
    for (size_t i = i_minFitX; i <= i_maxFitX; ++i)
      vecoutx[i-i_minFitX] = vecX[i];

    // Create workspace
    size_t sizex = vecoutx.size();
    size_t sizey = vecoutx.size();
    MatrixWorkspace_sptr outws = boost::dynamic_pointer_cast<MatrixWorkspace>(
          WorkspaceFactory::Instance().create("Workspace2D", nspec, sizex, sizey));

    // Calculate again
    FunctionDomain1DVector domain(vecoutx);
    FunctionValues values(domain);

    CompositeFunction_sptr compfunc = boost::make_shared<CompositeFunction>();
    compfunc->addFunction(m_peakFunc);
    compfunc->addFunction(m_bkgdFunc);
    compfunc->function(domain, values);
    for (size_t i = 0; i < sizex; ++i)
    {
      for (size_t j = 0; j < 3; ++j)
      {
        outws->dataX(j)[i] = domain[i];
      }
    }
    const MantidVec& vecY = m_dataWS->readY(m_wsIndex);
    for (size_t i = 0; i < sizey; ++i)
    {
      outws->dataY(0)[i] = vecY[i+i_minFitX];
      outws->dataY(1)[i] = values[i];
      outws->dataY(2)[i] = outws->dataY(0)[i]  - outws->dataY(1)[i];
    }

    // Set property
    setProperty("OutputWorkspace", outws);

    // Function parameter table workspaces
    TableWorkspace_sptr peaktablews = genOutputTableWS(m_peakFunc, m_fitErrorPeakFunc, m_bkgdFunc, m_fitErrorBkgdFunc);
    setProperty("ParameterTableWorkspace", peaktablews);

    // Parameter vector    
    vector<double> vec_fitpeak;
    for (size_t i = 0; i < m_peakParameterNames.size(); ++i)
    {
      double value = m_peakFunc->getParameter(m_peakParameterNames[i]);
      vec_fitpeak.push_back(value);
    }

    setProperty("FittedPeakParameterValues", vec_fitpeak);

    // Background
    vector<string> vec_bkgdnames = getProperty("BackgroundParameterNames");
    vector<double> vec_fitbkgd;

    for (size_t i = 0; i < vec_bkgdnames.size(); ++i)
    {
      double value = m_bkgdFunc->getParameter(vec_bkgdnames[i]);
      vec_fitbkgd.push_back(value);
    }

    setProperty("FittedBackgroundParameterValues", vec_fitbkgd);

    // Output chi^2 or Rwp
    setProperty("CostFunctionValue", m_finalGoodnessValue);

    return;
  }


  //----------------------------------------------------------------------------------------------
  /** Fit background with multiple domain
    */
  IBackgroundFunction_sptr FitPeak::fitBackground(IBackgroundFunction_sptr bkgdfunc)
  {
    // backkup
    map<string, double> errormap;
    push(bkgdfunc, m_bkupBkgdFunc, errormap);

    std::vector<double> vec_bkgd;
    vector<double> vec_xmin(2);
    vector<double> vec_xmax(2);
    vec_xmin[0] = m_minFitX;
    vec_xmin[1] = m_maxPeakX;
    vec_xmax[0] = m_minPeakX;
    vec_xmax[1] = m_maxFitX;
    double chi2 = fitFunctionMD(boost::dynamic_pointer_cast<IFunction>(bkgdfunc),
                                m_dataWS, m_wsIndex, vec_xmin, vec_xmax);

    if (chi2 > DBL_MAX-1)
    {
      pop(m_bkupBkgdFunc, bkgdfunc);
    }

    return bkgdfunc;
  }

  //----------------------------------------------------------------------------------------------
  /** Make a pure peak WS in the fit window region from m_background_function
    */
  void FitPeak::makePurePeakWS()
  {
    // Calculate background
    const MantidVec& vecX = m_dataWS->readX(m_wsIndex);
    FunctionDomain1DVector domain(vecX.begin()+i_minFitX, vecX.begin()+i_maxFitX);
    FunctionValues bkgdvalues(domain);
    m_bkgdFunc->function(domain, bkgdvalues);

    // Calculate pure background and put weight on peak if using Rwp
    MantidVec& vecY = m_dataWS->dataY(m_wsIndex);
    MantidVec& vecE = m_dataWS->dataE(m_wsIndex);
    for (size_t i = i_minFitX; i < i_maxFitX; ++i)
    {
      double y = vecY[i];
      y -= bkgdvalues[i-i_minFitX];
      if (y < 0.)
        y = 0.;
      vecY[i] = y;
      vecE[i] = 1.0;
    }

    return;
  }

  //----------------------------------------------------------------------------------------------
  /** Set up a set of starting values for FWHM (which is the most tricky part)
    */
  void FitPeak::setupGuessedFWHM(std::vector<double>& vec_FWHM)
  {
    // From user specified guess value
    vec_FWHM.push_back(m_userGuessedFWHM);

    // From user specified minimum value to maximim value
    if (!m_fitWithStepPeakWidth)
      return;

    const MantidVec& vecX = m_dataWS->readX(m_wsIndex);

    int i_centre = static_cast<int>(getVectorIndex(m_dataWS->readX(m_wsIndex), m_peakFunc->centre()));
    int i_maxindex = static_cast<int>(vecX.size())-1;

    for (int iwidth = m_minGuessedPeakWidth; iwidth <= m_maxGuessedPeakWidth; iwidth +=
         m_fwhmFitStep)
    {
      // There are 3 possible situation: peak at left edge, peak in proper range, peak at righ edge
      int ileftside = i_centre - iwidth/2;
      if (ileftside < 0)
        ileftside = 0;

      int irightside = i_centre + iwidth/2;
      if (irightside > i_maxindex)
        irightside = i_maxindex;

      double in_fwhm = vecX[irightside] - vecX[ileftside];

      if (in_fwhm < 1.0E-20)
      {
        g_log.warning() << "It is impossible to have zero peak width as iCentre = "
                        << i_centre << ", iWidth = " << iwidth << "\n"
                        << "More information: Spectrum = " << m_wsIndex << "; Range of X is "
                        << vecX.front() << ", " << vecX.back()
                        << "; Peak centre = " << vecX[i_centre];
      }
      else
      {
        g_log.debug() << "Fx330 i_width = " << iwidth << ", i_left = " << ileftside << ", i_right = "
                      << irightside << ", FWHM = " << in_fwhm << ".\n";
      }

      vec_FWHM.push_back(in_fwhm);
    }

    return;
  }

  //----------------------------------------------------------------------------------------------
  /** Process and store fit result
    * @param rwp :: goodness of this fit
    * @param storebkgd :: option to store (i.e., push) background function as well
    */
  void FitPeak::processNStoreFitResult(double rwp, bool storebkgd)
  {
    bool fitsuccess = true;
    string failreason("");

    if (rwp < DBL_MAX)
    {
      // A valid returned value RWP

      // Check non-negative height
      double f_height = m_peakFunc->height();
      if (f_height <= 0.)
      {
        rwp = DBL_MAX;
        failreason += "Negative peak height. ";
        fitsuccess = false;
      }

      // Check peak position
      double f_centre = m_peakFunc->centre();
      if (m_usePeakPositionTolerance)
      {
        // Peak position criteria is on position tolerance
        if (fabs(f_centre - m_userPeakCentre) > m_peakPositionTolerance)
        {
          rwp = DBL_MAX;
          failreason = "Peak centre out of tolerance. ";
          fitsuccess = false;
        }
      }
      else if (f_centre < m_minPeakX || f_centre > m_maxPeakX)
      {
        rwp = DBL_MAX;
        failreason += "Peak centre out of input peak range. ";
        fitsuccess = false;
      }

    } // RWP fine
    else
    {
      failreason = "(Single-step) Fit returns a DBL_MAX.";
      fitsuccess = false;
    }

    // Store result if
    if (rwp < m_bestRwp && fitsuccess)
    {
      push(m_peakFunc, m_bestPeakFunc, m_fitErrorPeakFunc);
      if (storebkgd)
        push(m_bkgdFunc, m_bestBkgdFunc, m_fitErrorBkgdFunc);
      m_bestRwp = rwp;
    }

    return;
  }

  //----------------------------------------------------------------------------------------------
  /** Push/store a fit result
    */
  void FitPeak::push(IFunction_const_sptr func, std::map<std::string, double>& funcparammap,
                     std::map<std::string, double>& paramerrormap)
  {
    // Clear map
    funcparammap.clear();
    paramerrormap.clear();

    // Set up
    vector<string> funcparnames = func->getParameterNames();
    size_t nParam = funcparnames.size();
    for (size_t i = 0; i < nParam; ++i)
    {
      double parvalue = func->getParameter(i);
      funcparammap.insert(make_pair(funcparnames[i], parvalue));

      double parerror = func->getError(i);
      paramerrormap.insert(make_pair(funcparnames[i], parerror));
    }

    return;
  }

  //----------------------------------------------------------------------------------------------
  /** Restore the parameters value to a function from a string/double map
    */
  void FitPeak::pop(const std::map<std::string, double>& funcparammap, API::IFunction_sptr func)
  {
    // TODO - One possible optimization is to record function parameters in index/value
    std::map<std::string, double>::const_iterator miter;
    for (miter = funcparammap.begin(); miter != funcparammap.end(); ++miter)
    {
      string parname = miter->first;
      double parvalue = miter->second;
      func->setParameter(parname, parvalue);
    }

    return;
  }

  //----------------------------------------------------------------------------------------------
  /** Fit peak function (only. so must be pure peak).
    * In this function, the fit result will be examined if fit is 'successful' in order to rule out
    * some fit with unphysical result.
    * @return :: chi-square/Rwp
    */
  double FitPeak::fitPeakFunction(API::IPeakFunction_sptr peakfunc, MatrixWorkspace_sptr dataws,
                                 size_t wsindex, double startx, double endx)
  {
    // Check validity and debug output
    if (!peakfunc)
      throw std::runtime_error("fitPeakFunction's input peakfunc has not been initialized.");
    else
      g_log.debug() << "Function (to fit): " << peakfunc->asString() << "  From "
                    << startx << "  to " << endx << ".\n";

    double goodness = fitFunctionSD(peakfunc, dataws, wsindex, startx, endx, false);
    g_log.debug() << "Peak parameter goodness-Fit = " << goodness << "\n";

    return goodness;
  }


  //----------------------------------------------------------------------------------------------
  /** Check the fitted peak value to see whether it is valud
    * @return :: Rwp/chi2
    */
  double FitPeak::checkFittedPeak(IPeakFunction_sptr peakfunc, double costfuncvalue, std::string& errorreason)
  {
    if (costfuncvalue < DBL_MAX)
    {
      // Fit is successful.  Check whether the fit result is physical
      stringstream errorss;
      double peakcentre = peakfunc->centre();
      if (peakcentre < m_minPeakX || peakcentre > m_maxPeakX)
      {
        errorss << "Peak centre (at " << peakcentre << " ) is out of specified range )"
                << m_minPeakX << ", " << m_maxPeakX << "). ";
        costfuncvalue = DBL_MAX;
      }

      double peakheight = peakfunc->height();
      if (peakheight < 0)
      {
        errorss << "Peak height (" << peakheight << ") is negative. ";
        costfuncvalue = DBL_MAX;
      }
      double peakfwhm = peakfunc->fwhm();
      if (peakfwhm > (m_maxFitX - m_minFitX) * MAGICNUMBER)
      {
        errorss << "Peak width is unreasonably wide. ";
        costfuncvalue = DBL_MAX;
      }
      errorreason = errorss.str();
    }
    else
    {
      // Fit is not successful
      errorreason = "Fit() on peak function is NOT successful.";
    }

    return costfuncvalue;
  }

  //----------------------------------------------------------------------------------------------
  /** Estimate the peak height from a set of data containing pure peaks
    */
  double FitPeak::estimatePeakHeight(API::IPeakFunction_sptr peakfunc, MatrixWorkspace_sptr dataws,
                                     size_t wsindex, double startx, double endx)
  {
    // Get current peak height
    double peakcentre = peakfunc->centre();
    vector<double> svvec(1, peakcentre);
    FunctionDomain1DVector svdomain(svvec);
    FunctionValues svvalues(svdomain);
    peakfunc->function(svdomain, svvalues);
    double curpeakheight = svvalues[0];

    g_log.debug() << "Estimate-Peak-Height: Current peak height = " << curpeakheight << "\n";

    // Get maximum peak value among
    const MantidVec& vecX = dataws->readX(wsindex);
    size_t ixmin = getVectorIndex(vecX, startx);
    size_t ixmax = getVectorIndex(vecX, endx);

    const MantidVec& vecY = dataws->readY(wsindex);
    double ymax = vecY[ixmin+1];
    size_t iymax = ixmin+1;
    for (size_t i = ixmin+2; i < ixmax; ++i)
    {
      double tempy = vecY[i];
      if (tempy > ymax)
      {
        ymax = tempy;
        iymax = i;
      }
    }
    g_log.debug() << "Estimate-Peak-Height: Maximum Y value between " << startx << " and "
                  << endx << " is "
                  << ymax << " at X = " << vecX[iymax] << ".\n";

    // Compute new peak
    double estheight = ymax/curpeakheight*peakfunc->height();

    return estheight;
  }


  //----------------------------------------------------------------------------------------------
  /** Fit peak function and background function as composite function
    * @return :: Rwp/chi2
    */
  double FitPeak::fitCompositeFunction(API::IPeakFunction_sptr peakfunc, API::IBackgroundFunction_sptr bkgdfunc,
                                       API::MatrixWorkspace_sptr dataws, size_t wsindex,
                                       double startx, double endx)
  {
    boost::shared_ptr<CompositeFunction> compfunc = boost::make_shared<CompositeFunction>();
    compfunc->addFunction(peakfunc);
    compfunc->addFunction(bkgdfunc);

    // Do calculation for starting chi^2/Rwp
    bool modecal = true;
    double goodness_init = fitFunctionSD(compfunc, dataws, wsindex, startx, endx, modecal);
    g_log.debug() << "Peak+Backgruond: Pre-fit Goodness = " << goodness_init << "\n";

    map<string, double> bkuppeakmap, bkupbkgdmap, bkuppeakerrormap, bkupbkgderrormap;
    push(peakfunc, bkuppeakmap, bkuppeakerrormap);
    push(bkgdfunc, bkupbkgdmap, bkupbkgderrormap);

    // Fit
    modecal = false;
    double goodness = fitFunctionSD(compfunc, dataws, wsindex, startx, endx, modecal);
    string errorreason;
    goodness = checkFittedPeak(peakfunc, goodness, errorreason);

    double goodness_final = DBL_MAX;
    if (goodness < goodness_init)
    {
      // Fit for composite function renders a better result
      goodness_final = goodness;
      errorreason = "";
    }
    else if (goodness_init <= goodness && goodness_init < DBL_MAX)
    {
      goodness_final = goodness_init;
      errorreason = "";
      g_log.information("Fit peak/background composite function FAILS to render a better solution.");
      pop(bkuppeakmap, peakfunc);
      pop(bkupbkgdmap, bkgdfunc);
    }
    else
    {
      g_log.information("Fit peak-background function fails in all approaches! ");
    }

    return goodness_final;
  }

  //----------------------------------------------------------------------------------------------
  /** Get an index of a value in a sorted vector.  The index should be the item with value nearest to X
    */
  size_t FitPeak::getVectorIndex(const MantidVec& vecx, double x)
  {
    size_t index;
    if (x <= vecx.front())
    {
      index = 0;
    }
    else if (x >= vecx.back())
    {
      index = vecx.size()-1;
    }
    else
    {
      vector<double>::const_iterator fiter;
      fiter = lower_bound(vecx.begin(), vecx.end(), x);
      index = static_cast<size_t>(fiter-vecx.begin());
      if (index == 0)
        throw runtime_error("It seems impossible to have this value. ");
      if (x-vecx[index-1] < vecx[index]-x)
        --index;
    }

    return index;
  }

  //----------------------------------------------------------------------------------------------
  /** Backup original data from i_minFitX to i_maxFitX
    */
  void FitPeak::backupOriginalData()
  {
    const MantidVec& vecY = m_dataWS->readY(m_wsIndex);
    const MantidVec& vecE = m_dataWS->readE(m_wsIndex);

    m_vecybkup.assign(vecY.begin() + i_minFitX, vecY.begin() + i_maxFitX+1);
    m_vecebkup.assign(vecE.begin() + i_minFitX, vecE.begin() + i_maxFitX+1);

    return;
  }

  //----------------------------------------------------------------------------------------------
  /** Backup original data from i_minFitX to i_maxFitX
    */
  void FitPeak::recoverOriginalData()
  {
    MantidVec& dataY = m_dataWS->dataY(m_wsIndex);
    MantidVec& dataE = m_dataWS->dataE(m_wsIndex);

    copy(m_vecybkup.begin(), m_vecybkup.end(), dataY.begin() + i_minFitX);
    copy(m_vecebkup.begin(), m_vecebkup.end(), dataE.begin() + i_minFitX);

    return;
  }

  //----------------------------------------------------------------------------------------------
  /** Fit function in single domain
    * @exception :: (1) Fit cannot be called. (2) Fit.isExecuted is false (cannot be executed)
    * @return :: chi^2 or Rwp depending on input.  If fit is not SUCCESSFUL, return DBL_MAX
    */
  double FitPeak::fitFunctionSD(IFunction_sptr fitfunc, MatrixWorkspace_sptr dataws,
                                size_t wsindex, double xmin, double xmax, bool calmode)
  {
    // Set up calculation mode: for pure chi-square/Rwp
    int maxiteration = 50;
    vector<string> parnames;
    if (calmode)
    {
      // Fix all parameters
      parnames = fitfunc->getParameterNames();
      for (size_t i = 0; i < parnames.size(); ++i)
        fitfunc->fix(i);

      maxiteration = 1;
    }
    else
    {
      // Unfix all parameters
      // FIXME - Remove this after 0-error is solved.
      parnames = fitfunc->getParameterNames();
      for (size_t i = 0; i < parnames.size(); ++i)
        fitfunc->unfix(i);
    }

    // Set up sub algorithm fit
    IAlgorithm_sptr fit;
    try
    {
      fit = createChildAlgorithm("Fit", -1, -1, true);
    }
    catch (Exception::NotFoundError &)
    {
      std::stringstream errss;
      errss << "The FitPeak algorithm requires the CurveFitting library";
      g_log.error(errss.str());
      throw std::runtime_error(errss.str());
    }

    // Set the properties
    fit->setProperty("Function", fitfunc);
    fit->setProperty("InputWorkspace", dataws);
    fit->setProperty("WorkspaceIndex", static_cast<int>(wsindex));
    fit->setProperty("MaxIterations", maxiteration);
    fit->setProperty("StartX", xmin);
    fit->setProperty("EndX", xmax);
    fit->setProperty("Minimizer", m_minimizer);
    fit->setProperty("CostFunction", m_costFunction);
    fit->setProperty("CalcErrors", true);

    // Execute fit and get result of fitting background
    g_log.debug() << "FitSingleDomain: Fit " << fit->asString() << ".\n";

    fit->executeAsChildAlg();
    if (!fit->isExecuted())
    {
      g_log.error("Fit for background is not executed. ");
      throw std::runtime_error("Fit for background is not executed. ");
    }

    // Retrieve result
    std::string fitStatus = fit->getProperty("OutputStatus");
    double chi2 = EMPTY_DBL();
    if (fitStatus == "success" || calmode)
    {
      chi2 = fit->getProperty("OutputChi2overDoF");
      fitfunc = fit->getProperty("Function");
    }

    // Release the ties
    if (calmode)
    {
      for (size_t i = 0; i < parnames.size(); ++i)
        fitfunc->unfix(i);
    }

    g_log.information() << "FitSingleDomain Fitted-Function " << fitfunc->asString()
                        << ": Fit-status = " << fitStatus
                        << ", chi^2 = " << chi2 << ".\n";

    return chi2;
  }

  //----------------------------------------------------------------------------------------------
  /** Fit function in multi-domain
    */
  double FitPeak::fitFunctionMD(IFunction_sptr fitfunc, MatrixWorkspace_sptr dataws,
                                size_t wsindex, vector<double> vec_xmin, vector<double> vec_xmax)
  {
    // Validate
    if (vec_xmin.size() != vec_xmax.size())
      throw runtime_error("Sizes of xmin and xmax (vectors) are not equal. ");

    // Set up sub algorithm fit
    IAlgorithm_sptr fit;
    try
    {
      fit = createChildAlgorithm("Fit", -1, -1, true);
    }
    catch (Exception::NotFoundError &)
    {
      std::stringstream errss;
      errss << "The FitPeak algorithm requires the CurveFitting library";
      g_log.error(errss.str());
      throw std::runtime_error(errss.str());
    }

    // This use multi-domain; but does not know how to set up
    boost::shared_ptr<MultiDomainFunction> funcmd = boost::make_shared<MultiDomainFunction>();

    // Set function first
    funcmd->addFunction(fitfunc);

    // set domain for function with index 0 covering both sides
    funcmd->clearDomainIndices();
    std::vector<size_t> ii(2);
    ii[0] = 0;
    ii[1] = 1;
    funcmd->setDomainIndices(0, ii);

    // Set the properties
    fit->setProperty("Function", boost::dynamic_pointer_cast<IFunction>(funcmd));
    fit->setProperty("InputWorkspace", dataws);
    fit->setProperty("WorkspaceIndex", static_cast<int>(wsindex));
    fit->setProperty("StartX", vec_xmin[0]);
    fit->setProperty("EndX", vec_xmax[0]);
    fit->setProperty("InputWorkspace_1", dataws);
    fit->setProperty("WorkspaceIndex_1", static_cast<int>(wsindex));
    fit->setProperty("StartX_1", vec_xmin[1]);
    fit->setProperty("EndX_1", vec_xmax[1]);
    fit->setProperty("MaxIterations", 50);
    fit->setProperty("Minimizer", m_minimizer);
    fit->setProperty("CostFunction", "Least squares");

    g_log.information() << "FitMultiDomain: Funcion " << funcmd->asString() << "\n";

    // Execute
    fit->execute();
    if (!fit->isExecuted())
    {
      throw runtime_error("Fit is not executed on multi-domain function/data. ");
    }

    // Retrieve result
    std::string fitStatus = fit->getProperty("OutputStatus");
    g_log.debug() << "[DB] Multi-domain fit status: " << fitStatus << ".\n";

    double chi2 = EMPTY_DBL();
    if (fitStatus == "success")
    {
      chi2 = fit->getProperty("OutputChi2overDoF");
      g_log.information() << "FitMultidomain: Successfully-Fitted Function " <<fitfunc->asString()
                          << ", Chi^2 = "<< chi2 << "\n";
    }

    return chi2;
  }

  //----------------------------------------------------------------------------------------------
  /** Generate table workspace
    */
  TableWorkspace_sptr FitPeak::genOutputTableWS(IPeakFunction_sptr peakfunc, map<string, double> peakerrormap,
                                                IBackgroundFunction_sptr bkgdfunc, map<string, double> bkgderrormap)
  {
    // Empty table
    TableWorkspace_sptr outtablews = boost::make_shared<TableWorkspace>();
    outtablews->addColumn("str", "Name");
    outtablews->addColumn("double", "Value");
    outtablews->addColumn("double", "Error");

    // Set chi^2
    TableRow newrow = outtablews->appendRow();
    newrow << "ChiSquare" << m_finalGoodnessValue;

    // Set peak paraemters
    newrow = outtablews->appendRow();
    newrow << peakfunc->name();

    if (m_outputRawParams)
    {
      vector<string> peakparnames = peakfunc->getParameterNames();
      for (size_t i = 0; i < peakparnames.size(); ++i)
      {
        string& parname = peakparnames[i];
        double parvalue = peakfunc->getParameter(parname);
        double error = peakerrormap[parname];
        newrow = outtablews->appendRow();
        newrow << parname << parvalue << error;
      }
    }
    else
    {
      newrow = outtablews->appendRow();
      newrow << "centre" << peakfunc->centre();

      newrow = outtablews->appendRow();
      newrow << "width" << peakfunc->fwhm();

      newrow = outtablews->appendRow();
      newrow << "height" << peakfunc->height();
    }

    // Set background paraemters
    newrow = outtablews->appendRow();
    newrow << bkgdfunc->name();

    if (m_outputRawParams)
    {
      vector<string> bkgdparnames = bkgdfunc->getParameterNames();
      for (size_t i = 0; i < bkgdparnames.size(); ++i)
      {
        string& parname = bkgdparnames[i];
        double parvalue = bkgdfunc->getParameter(parname);
        double error = bkgderrormap[parname];
        newrow = outtablews->appendRow();
        newrow << parname << parvalue << error;
      }
    }
    else
    {
      string bkgdtype = getProperty("BackgroundType");

      newrow = outtablews->appendRow();
      newrow << "backgroundintercept" << bkgdfunc->getParameter("A0");
      if (bkgdtype != "Flat")
      {
        newrow = outtablews->appendRow();
        newrow << "backgroundintercept" << bkgdfunc->getParameter("A1");
      }
      if (bkgdtype == "Quadratic")
      {
        newrow = outtablews->appendRow();
        newrow << "A2" << bkgdfunc->getParameter("A2");
      }
    }

    return outtablews;
  }

} // namespace Algorithms
} // namespace Mantid
