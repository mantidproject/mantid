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
// #include "FitPeak.h"
#include "MantidAPI/FunctionProperty.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/WorkspaceProperty.h"
#include "MantidDataObjects/TableWorkspace.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/BoundedValidator.h"

using namespace Mantid;
using namespace Mantid::API;
using namespace Mantid::DataObjects;
using namespace Mantid::Kernel;

using namespace std;

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

    declareProperty(new FunctionProperty("PeakFunction"),
                    "Peak function parameters defining the fitting function and its initial values");

    declareProperty(new FunctionProperty("BackgroundFunction"),
                    "Background function parameters defining the fitting function and its initial values");

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

    declareProperty("GuessedPeakWidthStep", 2, mustBePositive,
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



    return;
  }

  //----------------------------------------------------------------------------------------------
  /** Process input propeties
    */
  void FitPeak::processProperties()
  {
    // Data workspace (input)
    m_dataWS = getProperty("InputWorkspace");

    // Fit window
    vector<double> fitwindow = getProperty("FitWindow");
    if (fitwindow.size() != 2)
    {
      throw runtime_error("Must enter 2 and only 2 items in fit window. ");
    }
    m_minFitX = fitwindow[0];
    m_maxFitX = fitwindow[1];
    if (m_maxFitX <= m_minFitX)
    {
      stringstream errss;
      errss << "Minimum X (" << m_minFitX << ") is larger and equal to maximum X ("
            << m_maxFitX << ") to fit.  It is not allowed. ";
      g_log.error(errss.str());
      throw runtime_error(errss.str());
    }

    // Peak range
    vetor<double> peakrange = getProperty("PeakRange");
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

    //
    m_fitBkgdFirst = getProperty("FitBackgroundFirst");

    //
    m_outputRawParams = getProperty("RawParams");

    // Scheme
    m_fwhmFitStep = getProperty("GuessedPeakWidthStep");
    if (isEmpty(m_fwhmFitStep))
      m_fitWithStepPeakWidth = false;
    else
      m_fitWithStepPeakWidth = true;

    m_peakPositionTolerance = getProperty("PeakPositionTolerance");
    if (isEmpty(m_peakPositionTolerance))
      m_usePeakPositionTolerance = false;
    else
      m_usePeakPositionTolerance = true;

    return;
  }

  void FitPeak::prescreenInput()
  {
    // Check
    g_log.information() << "Fit Peak with given window:  Guessed center = " << centre_guess
                        << "  x-min = " << xmin
                        << ", x-max = " << xmax << "\n";
    if (xmin >= centre_guess || xmax <= centre_guess)
    {
      g_log.error() << "Peak centre is on the edge of Fit window\n";
      addNonFitRecord(spectrum);
      return;
    }

    //The left index
    int i_min = getVectorIndex(vecX, xmin);
    if (i_min >= i_centre)
    {
      g_log.error() << "Input peak centre @ " << centre_guess << " is out side of minimum x = "
                    << xmin << ".  Input X ragne = " << vecX.front() << ", " << vecX.back() << "\n";
      addNonFitRecord(spectrum);
      return;
    }

    //The right index
    int i_max = getVectorIndex(vecX, xmax);
    if (i_max < i_centre)
    {
      g_log.error() << "Input peak centre @ " << centre_guess << " is out side of maximum x = "
            << xmax << "\n";
      addNonFitRecord(spectrum);
      return;
    }
    //
    // Check that the indices provided are sensible
    if (i_min >= i_centre || i_max <= i_centre || i_min < 0)
    {
      g_log.error() << "FitPeakHightBackground has erroreous input.  i_min = " << i_min << ", i_centre = "
                    << i_centre << ", i_max = " << i_max << "\n";
      addNonFitRecord(spectrum);
      return;
    }

    // calculate the number of points in the fit window
    int numpts = i_max-i_min+1;
    if (numpts <= 0)
    {
      g_log.error() << "FitPeakHighBackground.  Pure peak workspace size <= 0.\n";
      addNonFitRecord(spectrum);
      return;
    }

    return;
  }

  //----------------------------------------------------------------------------------------------
  /** ... ...
    */
  void FitPeak::fitPeakInFitWindow()
  {
    // Check
    g_log.information() << "Fit Peak with given fit window:  Guessed center = " << m_peakFunc->centre()
                        << "  x-min = " << m_minFitX
                        << ", x-max = " << m_maxFitX << "\n";

    if (m_fitBkgdFirst)
    {
      fitPeakMultipleStep();
    }
    else
    {
      fitPeakOneStep();
    }

    return;
  }

  //----------------------------------------------------------------------------------------------
  /** Fit function
    */
  double FitPeak::fitFunction(IFunction_sptr fitfunc, MatrixWorkspace_const_sptr dataws,
                              size_t wsindex, double xmin, double xmax,
                              vector<double>& vec_caldata)
  {
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
    fit->setProperty("CostFunction", "Least squares");

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

      const MantidVec& vec_fitY = fit->getProperty("OutputWorkspace")->readX(wsindex);
      for (size_t i = i_minFitX; i < i_maxFitY; ++i)
      {
        vec_caldata[i-i_minFitX] = vec_fitY[i];
      }
    }

    g_log.debug() << "Fit function " << fitfunc->toString() << ": Fit-status = " << fitStatus
                  << ", chi^2 = " << chi2 << ".\n";

    return chi2;
  }

  //----------------------------------------------------------------------------------------------
  /** Fit background with multiple domain
    */
  IBackgroundFunction_sptr FitPeak::fitBackground(IBackgroundFunction_sptr bkgdfunc)
  {
    std::vector<double> vec_bkgd;
    double chi2 = fitFunction(bkgdfunc, m_dataWS, m_wsIndex, m_minFitX, m_maxFitX, vec_bkgd);

    if (chi2 > DBL_MAX-1)
    {
      // Unsuccessful
      bkgdfunc->setParameter("A0", in_bg0);
      if (numparams >= 2)
        bkgdfunc->setParameter("A1", in_bg1);
      if (numparams >= 3)
        bkgdfunc->setParameter("A2", in_bg2);
    }

    return bkgdfunc;
  }

  //----------------------------------------------------------------------------------------------
  /** Make a pure peak WS in the fit window region
    */
  void FitPeak::makePurePeakWS(const std::vector<double>& vec_bkgd)
  {
    MantidVec& vecY = m_dataWS->dataY();
    MantidVec& vecE = m_dataWS->dataE();
    for (size_t i = i_minFitW; i < i_maxFitW; ++i)
    {
      double y = vecY[i];
      y -= vec_bkgd[i-i_minFitW];
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

    for (int iwidth = m_minGuessedPeakWidth; iwidth <= m_maxGuessedPeakWidth; iwidth +=
         m_stepGuessedPeakWidth)
    {
      int peakwssize = static_cast<int>(peakX.size());
      // There are 3 possible situation: peak at left edge, peak in proper range, peak at righ edge
      int ileftside = i_centre - iwidth/2;
      if (ileftside < 0)
        ileftside = 0;

      int irightside = i_centre + iwidth/2;
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


  }


  //----------------------------------------------------------------------------------------------
  /** Fit peak in a robust manner.  Multiple fit will be
    */
  void FitPeak::fitPeakMultipleStep()
  {
    // get the original data
    const MantidVec &dataX = m_dataWS->readX(spectrum);
    const MantidVec &dataY = m_dataWS->readY(spectrum);

    // Create 2 backup vector
    vector<double> bkupY, bkupE, vec_FittedBkgd;
    backupOriginalData(bkupY, bkupE);

    // Fit background
    m_bkgdFunc = fitBackground(m_dataWS, m_bkgdFunc, vec_FittedBkgd);

    // Make pure peak
    makePurePeakWS(m_dataWS, vec_FittedBkgd);

    // Calculate guessed FWHM
    std::vector<double> vec_FWHM;
    setupGuessedFWHM(vec_FWHM);

    // Fit

    // Store starting setup
    push(m_peakFunc, m_presetPeak);

    for (size_t i = 0; i < vec_FWHM.size(); ++i)
    {
      // Restore
      if (i > 0)
        pop(m_peakFunc, m_presetPeak);

      // Set FWHM
      m_peakFunc->setFWHM(vec_FWHM[i]);

      // Fit
      double rwp = fitPeakFunction();

      // Store result
      processNStoreFitResult(rwp);
    }

    // Make a combo fit
    pop(m_bestPeak, m_peakFunc);
    fitCompositeFunction();

    return;
  }

  //----------------------------------------------------------------------------------------------
  /** Process and store fit result
    */
  void FitPeak::processNStoreFitResult(double rwp)
  {
    bool fitsuccess = true;

    if (rwp < DBL_MAX)
    {
      // A valid returned value RWP
      string failreason("");

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
    if (rwp < m_bestRWP)
    {
      push(m_peakFunc, m_bestFitPeak);
    }

    return;
  }

  //----------------------------------------------------------------------------------------------
  /** Push/store a fit result
    */
  void FitPeak::push(IFunction_sptr func, std::map<std::string, double>& funcparammap)
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
