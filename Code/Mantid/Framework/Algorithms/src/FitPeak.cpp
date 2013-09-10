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
#include "MantidAPI/FunctionProperty.h"
#include "MantidAPI/FunctionFactory.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/WorkspaceProperty.h"
#include "MantidAPI/TableRow.h"
#include "MantidKernel/ListValidator.h"
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

#if 0
    declareProperty(new FunctionProperty("PeakFunction"),
                    "Peak function parameters defining the fitting function and its initial values");

    declareProperty(new FunctionProperty("BackgroundFunction"),
                    "Background function parameters defining the fitting function and its initial values");
#else
    std::vector<std::string> peakNames = FunctionFactory::Instance().getFunctionNames<IPeakFunction>();
    declareProperty("PeakFunction", "Gaussian", boost::make_shared<StringListValidator>(peakNames));

    vector<string> bkgdtypes;
    bkgdtypes.push_back("Flat");
    bkgdtypes.push_back("Linear");
    bkgdtypes.push_back("Quadratic");
    declareProperty("BackgroundType", "Linear", boost::make_shared<StringListValidator>(bkgdtypes),
                    "Type of Background.");

    declareProperty(new WorkspaceProperty<TableWorkspace>("ParameterTable", "", Direction::InOut),
                    "Name of the table workspace containing the parameter names and values. ");
#endif

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

    return;
  }

  //----------------------------------------------------------------------------------------------
  /** Declare properties
   */
  void FitPeak::exec()
  {
    // Get input properties
    processProperties();

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

    return;
  }

  //----------------------------------------------------------------------------------------------
  /** Create functions from input properties
    */
  void FitPeak::createFunctions()
  {
    // Generate peak function
    m_peakFuncType = getPropertyValue("PeakFunction");
    m_peakFunc = boost::dynamic_pointer_cast<IPeakFunction>(
          FunctionFactory::Instance().createFunction(m_peakFuncType));

    // Generate background function
    m_backgroundType = getPropertyValue("BackgroundFunction");
    m_bkgdFunc = boost::dynamic_pointer_cast<IBackgroundFunction>(
          FunctionFactory::Instance().createFunction(m_backgroundType));

    // Parse Tableworkspace (parameters values for input)
    m_parameterTableWS = getProperty("ParameterTable");

    // vector<string> peakparnames = m_peakFunc->getParameterNames();
    // vector<string> bkgdparnames = m_bkgdFunc->getParameterNames();

    size_t numrows = m_parameterTableWS->rowCount();
    for (size_t i = 0; i < numrows; ++i)
    {
      TableRow row = m_parameterTableWS->getRow(i);
      string parname;
      double parvalue;
      row >> parname >> parvalue;
      // FIXME - Not sure if set a non-existing parameter can crash the code or be very expensive!
      m_peakFunc->setParameter(parname, parvalue);
      m_bkgdFunc->setParameter(parname, parvalue);
    }

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
  /** Check input data and get some information parameters
    */
  void FitPeak::prescreenInputData()
  {
    // Peak related
    throw runtime_error("Neither Peak function nor background has not been set. ");

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

    return;
  }

  //----------------------------------------------------------------------------------------------
  /** Fit function in single domain
    * @exception :: (1) Fit cannot be called. (2) Fit.isExecuted is false (cannot be executed)
    * @return :: chi^2 or Rwp depending on input.  If fit is not SUCCESSFUL, return DBL_MAX
    */
  double FitPeak::fitFunctionSD(IFunction_sptr fitfunc, MatrixWorkspace_const_sptr dataws,
                              size_t wsindex, double xmin, double xmax,
                              vector<double>& vec_caldata)
  {
    // Check:
    if (vec_caldata.size() != dataws->readY(wsindex).size())
      throw runtime_error("vec_caldata must have the same size as dataws.Y. ");

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
    fit->setProperty("MaxIterations", 50);
    fit->setProperty("StartX", xmin);
    fit->setProperty("EndX", xmax);
    fit->setProperty("Minimizer", m_minimizer);
    // FIXME -
    fit->setProperty("CostFunction", m_costFunction);

    // Execute fit and get result of fitting background
    fit->executeAsChildAlg();
    if (!fit->isExecuted())
    {
      g_log.error("Fit for background is not executed. ");
      throw std::runtime_error("Fit for background is not executed. ");
    }

    // Retrieve result
    std::string fitStatus = fit->getProperty("OutputStatus");
    double chi2 = EMPTY_DBL();
    if (fitStatus == "success")
    {
      chi2 = fit->getProperty("OutputChi2overDoF");
      fitfunc = fit->getProperty("Function");

      API::MatrixWorkspace_sptr outws = fit->getProperty("OutputWorkspace");

      const MantidVec& vec_fitY = outws->readY(wsindex);
      for (size_t i = i_minFitX; i < i_maxFitX; ++i)
      {
        vec_caldata[i] = vec_fitY[i];
      }
    }

    g_log.debug() << "Fit function " << fitfunc->asString() << ": Fit-status = " << fitStatus
                  << ", chi^2 = " << chi2 << ".\n";

    return chi2;
  }

  //----------------------------------------------------------------------------------------------
  /** Fit function in multi-domain
    */
  double FitPeak::fitFunctionMD(IFunction_sptr fitfunc, MatrixWorkspace_const_sptr dataws,
                                size_t wsindex, vector<double> xmin, vector<double> xmax,
                                vector<double>& vec_caldata)
  {
    boost::shared_ptr<MultiDomainFunction> funcmd = boost::shared_ptr<MultiDomainFunction>();
    funcmd->addFunction(fitfunc);
    funcmd->addFunction(fitfunc);

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
    fit->setProperty("StartX", xmin[1]);
    fit->setProperty("EndX", xmax[1]);
    fit->setProperty("InputWorkspace_1", dataws);
    fit->setProperty("WorkspaceIndex_1", static_cast<int>(wsindex));
    fit->setProperty("StartX_1", xmin[1]);
    fit->setProperty("EndX_1", xmax[1]);
    fit->setProperty("MaxIterations", 50);
    fit->setProperty("Minimizer", m_minimizer);
    fit->setProperty("CostFunction", "Least squares");




    return DBL_MAX;
  }


  //----------------------------------------------------------------------------------------------
  /** Fit background with multiple domain
    */
  IBackgroundFunction_sptr FitPeak::fitBackground(IBackgroundFunction_sptr bkgdfunc)
  {
    std::vector<double> vec_bkgd;

    push(bkgdfunc, m_bkupBkgdFunc);

    double chi2 = fitFunctionMD(bkgdfunc, m_dataWS, m_wsIndex, m_minFitX, m_maxFitX, vec_bkgd);

    if (chi2 > DBL_MAX-1)
    {
      pop(m_bkupBkgdFunc, bkgdfunc);
    }

    return bkgdfunc;
  }

  //----------------------------------------------------------------------------------------------
  /** Make a pure peak WS in the fit window region
    */
  void FitPeak::makePurePeakWS(const std::vector<double>& vec_bkgd)
  {
    MantidVec& vecY = m_dataWS->dataY(m_wsIndex);
    MantidVec& vecE = m_dataWS->dataE(m_wsIndex);
    for (size_t i = i_minFitX; i < i_maxFitX; ++i)
    {
      double y = vecY[i];
      y -= vec_bkgd[i];
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
  /** Fit peak in a robust manner.  Multiple fit will be
    */
  void FitPeak::fitPeakMultipleStep()
  {
    // Create 2 backup vector
    vector<double> bkupY, bkupE, vec_FittedBkgd;
    backupOriginalData(bkupY, bkupE);

    // Fit background
    // FIXME - fit bakcground related to multiple domain.  Read Roman's email ...
#if 1
    m_bkgdFunc = fitBackground(m_bkgdFunc);
#else
    m_bkgdFunc = fitBackground(m_dataWS, m_bkgdFunc, vec_FittedBkgd);
#endif

    // Make pure peak
    makePurePeakWS(vec_FittedBkgd);

    // Calculate guessed FWHM
    std::vector<double> vec_FWHM;
    setupGuessedFWHM(vec_FWHM);

    // Fit

    // Store starting setup
    push(m_peakFunc, m_bkupPeakFunc);

    for (size_t i = 0; i < vec_FWHM.size(); ++i)
    {
      // Restore
      if (i > 0)
        pop(m_bkupPeakFunc, m_peakFunc);

      // Set FWHM
      m_peakFunc->setFwhm(vec_FWHM[i]);

      // Fit
      double rwp = fitPeakFuncion(m_peakFunc, m_dataWS, m_wsIndex, m_minFitX, m_maxFitX);

      // Store result
      processNStoreFitResult(rwp);
    }

    // Make a combo fit
    pop(m_bestPeakFunc, m_peakFunc);
    fitCompositeFunction(m_peakFunc, m_bkgdFunc, m_dataWS, m_wsIndex, m_minFitX, m_maxFitX);

    return;
  }

  //----------------------------------------------------------------------------------------------
  /** Process and store fit result
    */
  void FitPeak::processNStoreFitResult(double rwp)
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
    if (rwp < m_bestRwp)
    {
      push(m_peakFunc, m_bestPeakFunc);
    }

    return;
  }

  //----------------------------------------------------------------------------------------------
  /** Push/store a fit result
    */
  void FitPeak::push(IFunction_const_sptr func, std::map<std::string, double>& funcparammap)
  {
    // Clear map
    funcparammap.clear();

    // Set up
    vector<string> funcparnames = func->getParameterNames();
    size_t nParam = funcparnames.size();
    for (size_t i = 0; i < nParam; ++i)
    {
      double parvalue = func->getParameter(i);
      funcparammap.insert(make_pair(funcparnames[i], parvalue));
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
  /** Fit peak function (only).
    * In this function, the fit result will be examined if fit is 'successful' in order to rule out
    * some fit with unphysical result.
    * @return :: chi-square/Rwp
    */
  double FitPeak::fitPeakFuncion(API::IPeakFunction_sptr peakfunc, MatrixWorkspace_const_sptr dataws,
                                 size_t wsindex, double startx, double endx, std::string& errorreason)
  {
    // Check validity and debug output
    if (peakfunc)
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
      g_log.debug(dbss.str());
    }

    vector<double> vec_calY;
    double goodness = fitFunctionSD(peakfunc, dataws, wsindex, startx, endx, vec_calY);
    if (goodness < EMPTY_DBL)
    {
      // Fit is successful.  Check whether the fit result is physical
      stringstream errorss;
      double peakcentre = peakfunc->centre();
      if (peakcentre < m_minPeakX || peakcentre > m_maxPeakX)
      {
        errorss << "Peak centre (at " << peakcentre << " ) is out of specified range )"
                << m_minPeakX << ", " << m_maxPeakX << "). ";
        goodness = DBL_MAX;
      }

      double peakheight = peakfunc->height();
      if (peakheight < 0)
      {
        errorss << "Peak height (" << peakheight << ") is negative. ";
        goodness = DBL_MAX;
      }
      double peakfwhm = peakfunc->fwhm();
      if (peakfwhm > (m_maxFitX - m_minFitX) * MAGICNUMBER)
      {
        errorss << "Peak width is unreasonably wide. ";
        goodness = DBL_MAX;
      }
      errorreason = errorss.str();
    }
    else
    {
      // Fit is not successful
      errorreason = "Fit() on peak function is NOT successful.";
    }

    return goodness;
  }

  //----------------------------------------------------------------------------------------------
  /** Fit peak function and background function as composite function
    * @return :: Rwp/chi2
    */
  double FitPeak::fitCompositeFunction(API::IPeakFunction_sptr peakfunc, API::IBackgroundFunction_sptr bkgdfunc,
                                     API::MatrixWorkspace_const_sptr dataws, size_t wsindex,
                                     double startx, double endx)
  {
    boost::shared_ptr<CompositeFunction> compfunc = boost::shared_ptr<CompositeFunction>();
    compfunc->addFunction(peakfunc);
    compfunc->addFunction(bkgdfunc);

    // Do calculation for starting chi^2/Rwp
    // FIXME - Whether there is any way to retrieve this information from Fit()?
    vector<double> vec_calY;
    bool modecal = true;
    double goodness_init = fitFunctionSD(compfunc, dataws, wsindex, startx, endx, vec_calY, modecal);
    push(peakfunc);
    push(bkgdfunc);

    // Fit
    modecal = false;
    double goodness = fitFunctionSD(compfunc, dataws, wsindex, startx, endx, vec_calY, modecal);
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
      goodness = goodness_init;
      errorreason = "";
      g_log.information("Fit peak/background composite function FAILS to render a better solution.");
      pop(peakfunc);
      pop(bkgdfunc);
    }
    else
    {
      g_log.information("Fit peak-background function fails in all approaches! ");
    }

    return goodness;
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

  /// Backup data
  void FitPeak::backupOriginalData(std::vector<double>& vecy, std::vector<double>& vece)
  {
    m_vecybkup.assign(vecy.begin(), vecy.end());
    m_vecebkup.assign(vece.begin(), vece.end());

    return;
  }

  /// Fit peak in one step
  void FitPeak::fitPeakOneStep()
  {
    // FIXME - Implement ASAP
    throw runtime_error("ASAP");
  }

  void FitPeakRemains()
  {
#if 0

    // Create a pure peak workspace (Workspace2D)


    // Estimate/observe peak parameters
    const MantidVec& peakX = peakws->readX(0);
    const MantidVec& peakY = peakws->readY(0);

    double g_centre, g_height, g_fwhm;
    std::string errormessage
        = estimatePeakParameters(peakX, peakY, 0, numpts-1, g_centre, g_height, g_fwhm);
    if (!errormessage.empty())
    {
      g_log.debug() << errormessage << "\n";
      addNonFitRecord(spectrum);
      return;
    }

    // Fit with loop upon specified FWHM range
    g_log.information("\nFitting peak by trying different peak width.");
    std::vector<double> vec_FWHM;
    for (int iwidth = m_minGuessedPeakWidth; iwidth <= m_maxGuessedPeakWidth; iwidth +=
         m_stepGuessedPeakWidth)
    {
      int peakwssize = static_cast<int>(peakX.size());
      // There are 3 possible situation: peak at left edge, peak in proper range, peak at righ edge
      // There should no guessed
      int ileftside = (i_centre - i_min) - iwidth/2;
      if (ileftside < 0)
        ileftside = 0;

      int irightside = (i_centre - i_min) + iwidth/2;
      if (irightside >= peakwssize)
        irightside = peakwssize-1;

      double in_fwhm = peakX[irightside] - peakX[ileftside];
      if (in_fwhm < 1.0E-20)
      {
        g_log.warning() << "It is impossible to have zero peak width as iCentre = "
                        << i_centre << ", iMin = "
                        << i_min << ", iWidth = " << iwidth << "\n"
                        << "More information: Spectrum = " << spectrum << "; Range of X is "
                        << rawX[0] << ", " << rawX.back()
                        << "; Peak centre = " << rawX[i_centre];
      }
      else
      {
        g_log.debug() << "Fx330 i_width = " << iwidth << ", i_left = " << ileftside << ", i_right = "
                      << irightside << ", FWHM = " << in_fwhm << ".\n";
      }

      vec_FWHM.push_back(in_fwhm);
    }

    // Create peak function
    IPeakFunction_sptr peakfunc = boost::dynamic_pointer_cast<IPeakFunction>(
          FunctionFactory::Instance().createFunction(m_peakFuncType));

    double in_centre = peakX[i_centre - i_min];
    double peakleftbound = peakX.front();
    double peakrightbound = peakX.back();
    PeakFittingRecord fitresult1 = multiFitPeakBackground(peakws, 0, input, spectrum, peakfunc, in_centre, g_height,vec_FWHM,
                                                          peakleftbound, peakrightbound, user_centre);

    // Fit upon observation
    g_log.information("\nFitting peak with starting value from observation. ");

    g_log.debug() << "[DB_Bkgd] Reset A0/A1 to: A0 = " << in_bg0 << ", A1 = " << in_bg1
                  << ", A2 = " << in_bg2 << "\n";
    m_backgroundFunction->setParameter("A0", in_bg0);
    if (m_backgroundFunction->nParams() > 1)
    {
      m_backgroundFunction->setParameter("A1", in_bg1);
      if (m_backgroundFunction->nParams() > 2)
        m_backgroundFunction->setParameter("A2", in_bg2);
    }

    in_centre = g_centre;
    peakleftbound = g_centre - 3*g_fwhm;
    peakrightbound = g_centre + 3*g_fwhm;
    vec_FWHM.clear();
    vec_FWHM.push_back(g_fwhm);
    PeakFittingRecord fitresult2 = multiFitPeakBackground(peakws, 0, input, spectrum, peakfunc, in_centre, g_height,vec_FWHM,
                                                          peakleftbound, peakrightbound, user_centre);

    // Compare results and add result to row
    double windowsize = rawX[i_max] - rawX[i_min];
    processFitResult(fitresult1, fitresult2, peakfunc, m_backgroundFunction, spectrum, i_min, i_max, windowsize);

    return;


#endif
  }
} // namespace Algorithms
} // namespace Mantid
