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

using namespace Mantid;
using namespace Mantid::API;
using namespace Mantid::DataObjects;
using namespace Mantid::Kernel;

using namespace std;

const bool DEBUG219 = true;
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
    declareProperty("PeakFunctionType", "Gaussian", boost::make_shared<StringListValidator>(peakNames),
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

    // TODO - Add FittedBackgroundParameterValues

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

    // TODO - Add choice on cost function and minmizer
#if 0
    std::vector<std::string> costFuncOptions = API::CostFunctionFactory::Instance().getKeys();
    costFuncOptions.push_back("Chi-Square");
    costFuncOptions.push_back("Rwp");
    declareProperty("CostFunction","Least squares",
                    Kernel::IValidator_sptr(new Kernel::ListValidator<std::string>(costFuncOptions)),
                    "Cost functions");

    std::vector<std::string> minimizerOptions = API::FuncMinimizerFactory::Instance().getKeys();

    declareProperty("Minimizer", "Levenberg-Marquardt",
      Kernel::IValidator_sptr(new Kernel::StartsWithValidator(minimizerOptions)),
      "Minimizer to use for fitting. Minimizers available are \"Levenberg-Marquardt\", \"Simplex\", \"Conjugate gradient (Fletcher-Reeves imp.)\", \"Conjugate gradient (Polak-Ribiere imp.)\", \"BFGS\", and \"Levenberg-MarquardtMD\"");
#endif
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
  /** Create functions from input properties
    */
  void FitPeak::createFunctions()
  {
    g_log.information("Running 'createFunction'. ");

    // Generate background function
    string bkgdtype = getPropertyValue("BackgroundType");
    // Fix the inconsistency in nameing the background
    if (bkgdtype == "Flat" || bkgdtype == "Linear")
      bkgdtype += "Background";

    m_bkgdFunc = boost::dynamic_pointer_cast<IBackgroundFunction>(
          FunctionFactory::Instance().createFunction(bkgdtype));
    g_log.information() << "[DB] Created background function of type " << bkgdtype << "\n";

    // Set background function parameter values
    vector<string> vec_bkgdparnames = getProperty("BackgroundParameterNames");
    vector<double> vec_bkgdparvalues = getProperty("BackgroundParameterValues");
    if (vec_bkgdparnames.size() != vec_bkgdparvalues.size() || vec_bkgdparnames.size() == 0)
    {
      throw runtime_error("Input background properties' arrays are not correct!");
    }

    // Set parameter values
    for (size_t i = 0; i < vec_bkgdparnames.size(); ++i)
    {
      g_log.information() << "[DB] Set to background: " << vec_bkgdparnames[i]
                          << " = " << vec_bkgdparvalues[i] << "\n";
      m_bkgdFunc->setParameter(vec_bkgdparnames[i], vec_bkgdparvalues[i]);
    }

    // Generate peak function
    string peaktype = getPropertyValue("PeakFunctionType");
    m_peakFunc = boost::dynamic_pointer_cast<IPeakFunction>(
          FunctionFactory::Instance().createFunction(peaktype));
    g_log.information() << "[DB] Create peak function of type " << peaktype << "\n";

    // Given by arrays
    vector<string> vec_peakparnames = getProperty("PeakParameterNames");
    vector<double> vec_peakparvalues = getProperty("PeakParameterValues");
    if (vec_peakparnames.size() != vec_peakparvalues.size() || vec_peakparnames.size() == 0)
    {
      throw runtime_error("Input peak properties' arrays are not correct!");
    }
    else
    {
      for (size_t i = 0; i < vec_peakparnames.size(); ++i)
        g_log.information() << "[DB] Input peak parameter name (" << i << ") = " << vec_peakparnames[i] << "\n";
    }

    // Set peak parameter values
    for (size_t i = 0; i < vec_bkgdparnames.size(); ++i)
    {
      m_peakFunc->setParameter(vec_peakparnames[i], vec_peakparvalues[i]);
    }

    g_log.debug("Finished running 'createFunction'. ");

    return;
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

    m_costFunction = "Least squares";

    return;
  }

  //----------------------------------------------------------------------------------------------
  /// Fit peak in one step
  void FitPeak::fitPeakOneStep()
  {
    // Set up a composite function
    CompositeFunction_sptr compfunc = boost::make_shared<CompositeFunction>();
    compfunc->addFunction(m_peakFunc);
    compfunc->addFunction(m_bkgdFunc);

    // Set up a list of guessed FWHM
    // Calculate guessed FWHM
    std::vector<double> vec_FWHM;
    setupGuessedFWHM(vec_FWHM);

    // Store starting setup
    map<string, double> temperrormap;
    push(m_peakFunc, m_bkupPeakFunc, temperrormap);
    push(m_bkgdFunc, m_bkupBkgdFunc, temperrormap);

    // Fit with different starting values of peak width
    for (size_t i = 0; i < vec_FWHM.size(); ++i)
    {
      // set FWHM
      m_peakFunc->setFwhm(vec_FWHM[i]);

      // fit and process result
      double goodndess = fitFunctionSD(compfunc, m_dataWS, m_wsIndex, m_minFitX, m_maxFitX, false);
      processNStoreFitResult(goodndess, true);

      // restore the function parameters
      pop(m_bkupPeakFunc, m_peakFunc);
      pop(m_bkupBkgdFunc, m_bkgdFunc);
    }

    return;
  }

  //----------------------------------------------------------------------------------------------
  /** Fit peak in a robust manner.  Multiple fit will be
    */
  void FitPeak::fitPeakMultipleStep()
  {
    g_log.information("Starting fitPeakMultipleStep(). ");

    // Fit background
    // FIXME : Which one to use?
#if 1
    m_bkgdFunc = fitBackground(m_bkgdFunc);
#else
    m_bkgdFunc = fitBackground(m_dataWS, m_bkgdFunc, vec_FittedBkgd);
#endif
    g_log.information("fitPeakMultipleStep().2 Finished. ");

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
      g_log.information() << "Round " << i << " of " << vec_FWHM.size() << ". Using proposed FWHM = "
                          << vec_FWHM[i] << "\n";
      m_peakFunc->setFwhm(vec_FWHM[i]);

      // Fit
      string error;
      double rwp = fitPeakFuncion(m_peakFunc, m_dataWS, m_wsIndex, m_minFitX, m_maxFitX, error);

      // Store result
      processNStoreFitResult(rwp,false);
    }

    // Make a combo fit

    // Get best fitting peak function
    pop(m_bestPeakFunc, m_peakFunc);
    g_log.information() << "Best peak function fitted: " << m_peakFunc->asString() << ".\n";

    // Recover the original Y value from pure peak data range
    recoverOriginalData();

    m_finalGoodnessValue = fitCompositeFunction(m_peakFunc, m_bkgdFunc, m_dataWS, m_wsIndex, m_minFitX, m_maxFitX);
    g_log.information() << "Final goodness = " << m_finalGoodnessValue << "\n";

    return;
  }


  //----------------------------------------------------------------------------------------------
  /** Check input data and get some information parameters
    */
  void FitPeak::prescreenInputData()
  {
    g_log.information("Running prescreenInputData. ");

    // Check functions
    if (!m_peakFunc || !m_bkgdFunc)
      throw runtime_error("Either peak function or background function has not been set up.");

    // Check validity on peak centre
    double centre_guess = m_peakFunc->centre();
    g_log.information() << "Fit Peak with given window:  Guessed center = " << centre_guess
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

    g_log.information("Finished running prescreenInputData. ");

    return;
  }

  //----------------------------------------------------------------------------------------------
  /** Set up the output workspaces
    * including (1) data workspace (2) function parameter workspace
    */
  void FitPeak::setupOutput()
  {
    // Data workspace
    // TODO - Make it a 3 spectra output (original, model and difference).  But only with FitWindow
    size_t nspec = 1;
    size_t sizex = m_dataWS->readX(m_wsIndex).size();
    size_t sizey = m_dataWS->readY(m_wsIndex).size();
    MatrixWorkspace_sptr outws = boost::dynamic_pointer_cast<MatrixWorkspace>(
          WorkspaceFactory::Instance().create("Workspace2D", nspec, sizex, sizey));
    FunctionDomain1DVector domain(m_dataWS->readX(m_wsIndex));
    FunctionValues values(domain);

    CompositeFunction_sptr compfunc = boost::make_shared<CompositeFunction>();
    compfunc->addFunction(m_peakFunc);
    compfunc->addFunction(m_bkgdFunc);
    compfunc->function(domain, values);
    for (size_t i = 0; i < sizex; ++i)
      outws->dataX(0)[i] = domain[i];
    for (size_t i = 0; i < sizey; ++i)
      outws->dataY(0)[i] = values[i];

    setProperty("OutputWorkspace", outws);

    // Function parameter table workspaces
    TableWorkspace_sptr peaktablews = genOutputTableWS(m_peakFunc, m_fitErrorPeakFunc, m_bkgdFunc, m_fitErrorBkgdFunc);
    setProperty("ParameterTableWorkspace", peaktablews);

    // Parameter vector
    // FIXME - This is not correct!
    vector<double> vec_fitpeak;
    vec_fitpeak.push_back(1.0);
    vec_fitpeak.push_back(2.0);
    vec_fitpeak.push_back(3.0);
    setProperty("FittedPeakParameterValues", vec_fitpeak);

    // TODO - Add FittedBackgroundParameterValues

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

#if 0
    for (size_t i = 0; i < domain.size(); ++i)
    {
      g_log.notice() << domain[i] << "\t\t" << bkgdvalues[i] << "\n";
    }
    throw runtime_error("Check here!");
#endif

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
        g_log.information() << "Fx330 i_width = " << iwidth << ", i_left = " << ileftside << ", i_right = "
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
      // g_log.debug() << "Error(" << funcparnames[i] << ") = " << parerror << "\n";
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
  double FitPeak::fitPeakFuncion(API::IPeakFunction_sptr peakfunc, MatrixWorkspace_sptr dataws,
                                 size_t wsindex, double startx, double endx, std::string& errorreason)
  {
    // FIXME - Shall I use errorreason?
    // Check validity and debug output
    if (!peakfunc)
      throw std::runtime_error("fitPeakFunction's input peakfunc has not been initialized.");
    else
      g_log.debug() << "Function (to fit): " << peakfunc->asString() << "  From "
                    << startx << "  to " << endx << ".\n";

    if (DEBUG219)
    {
      std::stringstream dbss;
      dbss << "Fit data workspace spectrum " << wsindex << ".  Parameters: ";
      std::vector<std::string> comparnames = peakfunc->getParameterNames();
      for (size_t i = 0; i < comparnames.size(); ++i)
        dbss << comparnames[i] << ", ";
      g_log.information(dbss.str());
    }

    double goodness = fitFunctionSD(peakfunc, dataws, wsindex, startx, endx, false);
    g_log.information() << "[D] Goodness-Fit = " << goodness << "\n";

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

    g_log.information() << "[D] Current peak height = " << curpeakheight << "\n";

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
    g_log.information() << "[D] Maximum Y value between " << startx << " and " << endx << " is "
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
    g_log.information() << "[D] Pre-fit: Goodness = " << goodness_init << ".\n";

    map<string, double> bkuppeakmap, bkupbkgdmap, bkuppeakerrormap, bkupbkgderrormap;
    push(peakfunc, bkuppeakmap, bkuppeakerrormap);
    push(bkgdfunc, bkupbkgdmap, bkupbkgderrormap);

    // Fit
    modecal = false;
    g_log.information("[D] Start to evaluate composite function. ");
    double goodness = fitFunctionSD(compfunc, dataws, wsindex, startx, endx, modecal);
    string errorreason;
    goodness = checkFittedPeak(peakfunc, goodness, errorreason);

    double goodness_final;
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
    g_log.notice("[D] Start to recover data.");
    MantidVec& dataY = m_dataWS->dataY(m_wsIndex);
    MantidVec& dataE = m_dataWS->dataE(m_wsIndex);

    copy(m_vecybkup.begin(), m_vecybkup.end(), dataY.begin() + i_minFitX);
    copy(m_vecebkup.begin(), m_vecebkup.end(), dataE.begin() + i_minFitX);

    g_log.notice("[D] Finished recovering data.");

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

    // Execute fit and get result of fitting background
    g_log.information() << "Fit function: " << fit->asString() << ".\n";

#if 0
    CompositeFunction_sptr comfunc = boost::dynamic_pointer_cast<CompositeFunction>(fitfunc);
    bool plot = false;
    if (comfunc)
      plot = true;
    if (plot)
    {
      for (size_t i = 0; i < dataws->readX(wsindex).size(); ++i)
      {
        g_log.information() << dataws->readX(wsindex)[i] << "\t\t" << dataws->readY(wsindex)[i] << "\t\t"
                            << dataws->readE(wsindex)[i] << "\n";
      }
    }
#endif

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

      // FIXME - This is a temporary debug output. Should remove later
      // ROMAN: I tried to get error from the fitted function.  But all that I've got is zero.
      for (size_t i = 0; i < fitfunc->getParameterNames().size(); ++i)
      {
        g_log.notice() << "[For Roman] Error of parameter " << fitfunc->getParameterNames()[i]
                       << " = " << fitfunc->getError(i) << "\n";
      }

#if 0
      API::MatrixWorkspace_sptr outws = fit->getProperty("OutputWorkspace");
      const MantidVec& vec_fitY = outws->readY(wsindex);
#endif
    }

    // Release the ties
    if (calmode)
    {
      for (size_t i = 0; i < parnames.size(); ++i)
        fitfunc->unfix(i);
    }

    g_log.information() << "Fit function " << fitfunc->asString() << ": Fit-status = " << fitStatus
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

    // ROMAN: I tried to fit the background with multiple domain.  But "Start_1" and "End_1" are not
    //        recognized.  Do you know how to set it up in C++?  I failed to use the way to set up in Python
    //        to C++
#if 0
    // This use multi-domain; but does not know how to set up
    boost::shared_ptr<MultiDomainFunction> funcmd = boost::make_shared<MultiDomainFunction>();
    funcmd->addFunction(fitfunc);
    funcmd->addFunction(fitfunc);



    g_log.information() << "Input workspace size = " << dataws->size() << ".\n";

    // Set the properties
    fit->setProperty("Function", fitfunc);
    fit->setProperty("InputWorkspace", dataws);
    fit->setProperty("WorkspaceIndex", static_cast<int>(wsindex));
    fit->setProperty("StartX", xmin[0]);
    fit->setProperty("EndX", xmax[0]);
    // FIXME - There is not 'InputWorkspace_1' or 'WorkspaceIndex_1' in fit
    fit->setProperty("InputWorkspace_1", dataws);
    fit->setProperty("WorkspaceIndex_1", static_cast<int>(wsindex));
    fit->setProperty("StartX_1", xmin[1]);
    fit->setProperty("EndX_1", xmax[1]);

    fit->setProperty("MaxIterations", 50);
    fit->setProperty("Minimizer", m_minimizer);
    fit->setProperty("CostFunction", "Least squares");

#else
    // FIXME - This is a temp solution
    vector<double> vecx, vecy, vece;
    const MantidVec& vecX = dataws->readX(wsindex);
    const MantidVec& vecY = dataws->readY(wsindex);
    const MantidVec& vecE = dataws->readE(wsindex);
    for (size_t i = 0; i < vec_xmin.size(); ++i)
    {
      double xmin = vec_xmin[i];
      double xmax = vec_xmax[i];
      size_t ixmin = getVectorIndex(vecX, xmin);
      size_t ixmax = getVectorIndex(vecX, xmax);
      for (size_t j = ixmin; j <= ixmax; ++j)
      {
        vecx.push_back(vecX[j]);
        vecy.push_back(vecY[j]);
        vece.push_back(vecE[j]);
      }
    }

    MatrixWorkspace_sptr tempbkgdws = boost::dynamic_pointer_cast<MatrixWorkspace>(
          WorkspaceFactory::Instance().create("Workspace2D", 1, vecx.size(), vecy.size()));

    MantidVec& dataX = tempbkgdws->dataX(0);
    MantidVec& dataY = tempbkgdws->dataY(0);
    MantidVec& dataE = tempbkgdws->dataE(0);

    for (size_t i = 0; i < vecx.size(); ++i)
    {
      dataX[i] = vecx[i];
      dataY[i] = vecy[i];
      dataE[i] = vece[i];
    }

    fit->setProperty("Function", fitfunc);
    fit->setProperty("InputWorkspace", tempbkgdws);
    fit->setProperty("WorkspaceIndex", static_cast<int>(wsindex));
    fit->setProperty("StartX", vecx.front());
    fit->setProperty("EndX", vecx.back());

    fit->setProperty("MaxIterations", 50);
    fit->setProperty("Minimizer", m_minimizer);
    fit->setProperty("CostFunction", "Least squares");
#endif

    // Execute
    fit->execute();
    if (!fit->isExecuted())
    {
      throw runtime_error("Fit is not executed on multi-domain function/data. ");
    }

    // Retrieve result
    std::string fitStatus = fit->getProperty("OutputStatus");
    g_log.information() << "Multi-domain fit status: " << fitStatus << ".\n";

    double chi2 = EMPTY_DBL();
    if (fitStatus == "success")
    {
      chi2 = fit->getProperty("OutputChi2overDoF");
      g_log.information() << "Multi-domain fit chi^2 = " << chi2 << ".\n";
    }

    return chi2;
  }

  //----------------------------------------------------------------------------------------------
  /** Generate table workspace
    */
  TableWorkspace_sptr FitPeak::genOutputTableWS(IFunction_sptr peakfunc, map<string, double> peakerrormap,
                                                IFunction_sptr bkgdfunc, map<string, double> bkgderrormap)
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
    newrow << "Peak";
    vector<string> peakparnames = peakfunc->getParameterNames();
    for (size_t i = 0; i < peakparnames.size(); ++i)
    {
      string& parname = peakparnames[i];
      double parvalue = peakfunc->getParameter(parname);
      double error = peakerrormap[parname];
      newrow = outtablews->appendRow();
      newrow << parname << parvalue << error;
    }

    // Set background paraemters
    newrow = outtablews->appendRow();
    newrow << "Background";
    vector<string> bkgdparnames = bkgdfunc->getParameterNames();
    for (size_t i = 0; i < bkgdparnames.size(); ++i)
    {
      string& parname = bkgdparnames[i];
      double parvalue = bkgdfunc->getParameter(parname);
      double error = bkgderrormap[parname];
      newrow = outtablews->appendRow();
      newrow << parname << parvalue << error;
    }

    return outtablews;
  }

} // namespace Algorithms
} // namespace Mantid
