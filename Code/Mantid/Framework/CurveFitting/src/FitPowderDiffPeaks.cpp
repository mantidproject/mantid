#include "MantidCurveFitting/FitPowderDiffPeaks.h"

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
#include "MantidCurveFitting/ThermalNeutronDtoTOFFunction.h"
#include "MantidCurveFitting/Polynomial.h"
#include "MantidCurveFitting/BoundaryConstraint.h"
#include "MantidCurveFitting/Gaussian.h"

#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/split.hpp>

#include <fstream>
#include <iomanip>

/// Factor on FWHM for searching a peak
#define PEAKRANGEFACTOR 20.0
/// Factor on FWHM for excluding peak to fit background
#define EXCLUDEPEAKRANGEFACTOR 8.0

using namespace Mantid;
using namespace Mantid::API;
using namespace Mantid::Kernel;
using namespace Mantid::DataObjects;

using namespace std;

namespace Mantid
{
namespace CurveFitting
{

  DECLARE_ALGORITHM(FitPowderDiffPeaks)

  //----------------------------------------------------------------------------------------------
  /** Constructor
   */
  FitPowderDiffPeaks::FitPowderDiffPeaks()
  {
  }
    
  //----------------------------------------------------------------------------------------------
  /** Destructor
   */
  FitPowderDiffPeaks::~FitPowderDiffPeaks()
  {
  }
  
  //----------------------------------------------------------------------------------------------
  /** Set up documention
    */
  void FitPowderDiffPeaks::initDocs()
  {
    setWikiSummary("Fit peaks in powder diffraction pattern. ");
    setOptionalMessage("Fit peaks in powder diffraction pattern. ");
  }

  //----------------------------------------------------------------------------------------------
  /** Parameter declaration
   */
  void FitPowderDiffPeaks::init()
  {
    // Input data workspace
    declareProperty(new API::WorkspaceProperty<API::MatrixWorkspace>("InputWorkspace", "Anonymous", Direction::Input),
                    "Input workspace for data (diffraction pattern). ");

    // Output workspace
    declareProperty(new API::WorkspaceProperty<DataObjects::Workspace2D>("OutputWorkspace", "Anonymous2", Direction::Output),
                    "Output Workspace2D for the fitted peaks. ");

    // Input/output peaks table workspace
    declareProperty(new API::WorkspaceProperty<DataObjects::TableWorkspace>("PeaksParametersWorkspace", "AnonymousPeak", Direction::Input),
                    "TableWorkspace containg all peaks' parameters.");

    // Input and output instrument parameters table workspace
    declareProperty(new API::WorkspaceProperty<DataObjects::TableWorkspace>("InstrumentParametersWorkspace", "AnonymousInstrument", Direction::InOut),
                    "TableWorkspace containg instrument's parameters.");

    // Workspace to output fitted peak parameters
    declareProperty(new WorkspaceProperty<TableWorkspace>("OutputPeaksParameterWorkspace", "AnonymousOut2", Direction::Output),
                    "Output TableWorkspace containing the fitted peak parameters for each peak.");

    // Workspace index of the
    declareProperty("WorkspaceIndex", 0, "Worskpace index for the data to refine against.");

    // Range of the peaks to fit
    declareProperty("MinTOF", EMPTY_DBL(), "Minimum TOF to fit peaks.  ");
    declareProperty("MaxTOF", EMPTY_DBL(), "Maximum TOF to fit peaks.  ");

    return;
  }


  //----------------------------------------------------------------------------------------------
  /** Main execution
   */
  void FitPowderDiffPeaks::exec()
  {
    // 1. Get input
    dataWS = this->getProperty("InputWorkspace");

    workspaceindex = this->getProperty("WorkspaceIndex");
    if (workspaceindex < 0 || workspaceindex > static_cast<int>(dataWS->getNumberHistograms()))
    {
      g_log.error() << "Input workspace = " << workspaceindex << " is out of range [0, "
                    << dataWS->getNumberHistograms() << std::endl;
      throw std::invalid_argument("Input workspaceindex is out of range.");
    }

    DataObjects::TableWorkspace_sptr peakWS = this->getProperty("PeaksParametersWorkspace");
    DataObjects::TableWorkspace_sptr parameterWS = this->getProperty("InstrumentParametersWorkspace");

    double tofmin = getProperty("MinTOF");
    double tofmax = getProperty("MaxTOF");
    if (tofmin == EMPTY_DBL())
    {
      tofmin = dataWS->readX(workspaceindex)[0];
    }
    if (tofmax == EMPTY_DBL())
    {
      tofmax = dataWS->readX(workspaceindex).back();
    }

    // 2. Crop input workspace
    cropWorkspace(tofmin, tofmax);

    // 3. Parse input table workspace
    genPeaksFromTable(peakWS, mPeaks);
    importParametersFromTable(parameterWS, mFuncParameters);

    // 4. Fit peaks & get peak centers
    std::vector<std::vector<int> > goodfitpeaks;
    std::vector<double> goodfitchi2;
    vector<pair<double, vector<int> > > inp_tofhkls;
    map<vector<int>, BackToBackExponential_sptr>::iterator peakiter;
    for (peakiter = mPeaks.begin(); peakiter != mPeaks.end(); ++peakiter)
    {
      vector<int> hkl = peakiter->first;
      BackToBackExponential_sptr peak = peakiter->second;
      double center = peak->centre();
      inp_tofhkls.push_back(make_pair(center, hkl));
    }
    fitPeaks(workspaceindex, goodfitpeaks, goodfitchi2);

    // 5. Create Output

    // a) Create a Table workspace for peak profile
    TableWorkspace_sptr outputpeaksws = genPeakParametersWorkspace(goodfitpeaks, goodfitchi2);
    setProperty("OutputPeaksParameterWorkspace", outputpeaksws);

    // b) Create output data workspace (as a middle stage product)
    Workspace2D_sptr outdataws = genOutputFittedPatternWorkspace(mPeakData, workspaceindex);
    setProperty("OutputWorkspace", outdataws);

    return;
  }

  //----------------------------------------------------------------------------------------------

  /** Fit each individual Bk2Bk-Exp-Conv-PV peaks
    * This part is under heavy construction, and will be applied to "FindPeaks2"
    * Output: (1) goodfitpeaks, (2) goodfitchi2
   */
  void FitPowderDiffPeaks::fitPeaks(int workspaceindex, std::vector<std::vector<int> >& goodfitpeaks,
                                                  std::vector<double>& goodfitchi2)
  {
    g_log.notice() << "[FitPeaks] Total Number of Peak = " << mPeaks.size() << std::endl;

    // 1. Clear the output vector
    goodfitpeaks.clear();

    size_t numpts = dataWS->readX(workspaceindex).size();
    mPeakData.reserve(numpts);
    for (size_t i = 0; i < numpts; ++i)
    {
      mPeakData.push_back(0.0);
    }

    // 2. Re-order the peaks
    vector<pair<double, vector<int> > > poshklvector;
    std::map<std::vector<int>, CurveFitting::BackToBackExponential_sptr>::iterator peakiter;
    for (peakiter = mPeaks.begin(); peakiter != mPeaks.end(); ++peakiter)
    {
      vector<int> hkl = peakiter->first;
      BackToBackExponential_sptr peak = peakiter->second;
      double tofh = peak->centre();
      poshklvector.push_back(std::make_pair(tofh, hkl));
    }

    std::sort(poshklvector.begin(), poshklvector.end());

    int numpeaks = static_cast<int>(poshklvector.size());
    for (int i = numpeaks-1; i >= 0; --i)
    {
      double tofh = poshklvector[i].first;
      vector<int> hkl = poshklvector[i].second;
      int hkl2 = hkl[0]*hkl[0] + hkl[1]*hkl[1] + hkl[2]*hkl[2];
      g_log.information() << "Peak (" << hkl[0] << ", " << hkl[1] << ", " << hkl[2] <<").  (HKL)^2 = "
                          << hkl2 << ".  Center = " << tofh << endl;
    }

    // 3. Fit all peaks
    BackToBackExponential_sptr peakOnRight;
    BackToBackExponential_sptr peak;
    for (int ipk = numpeaks-1; ipk >= 0; --ipk)
    {
      /* Fit each single peak BUT NOT completely independently
        */

      // a) Get hold on the peak to fit
      vector<int> hkl = poshklvector[ipk].second;
      peak = mPeaks[hkl];
      if (!peak)
      {
        stringstream errmsg;
        errmsg << "Unable to find peak (" << hkl[0] << ", " << hkl[1] << ", " << hkl[2] << ") in "
               << "the list of peaks.  Logic error in programming. ";
        g_log.error() << errmsg.str() << endl;
        throw std::logic_error(errmsg.str());
      }

      g_log.information() << std::endl << "-----------   DBx315 Fit Peak [" << hkl[0] <<", " << hkl[1]
                << ", " << hkl[2] << "] @ " << peak->centre()  << " ------------------" << std::endl;

      // b) Set up function including (i) Background (Polynomial)
      CurveFitting::Polynomial_sptr background = boost::make_shared<CurveFitting::Polynomial>
          (CurveFitting::Polynomial());
      background->setAttributeValue("n", 2);
      background->initialize();

      // c) Determine the range of fitting: assuming the input is not far off
      double fwhm = peak->fwhm();
      double leftdev = PEAKRANGEFACTOR * fwhm * 0.5;
      double rightdev = PEAKRANGEFACTOR * fwhm * 0.5;

      // d) Consider the right bound of the peak can be limited by the peak found on the right
      //    (just fitted)
      if (peakOnRight)
      {
        // If not the right most peak
        double rightpeakrange = peakOnRight->centre()-EXCLUDEPEAKRANGEFACTOR*0.5*peakOnRight->fwhm();
        double temprightdev = rightpeakrange-peak->centre();
        if (temprightdev < rightdev)
        {
          rightdev = temprightdev;
        }

        if ( rightdev < 0 && (std::fabs(rightdev) >= leftdev) )
        {
          stringstream errss;
          errss << "Peak [" << hkl[0] << ", " << hkl[1] << ", " << hkl[2] << "] deviates from guessed value too much. "
                << "This situation is not dealt with now!";
          g_log.error() << errss.str() << endl;
          throw logic_error(errss.str());
        }

        g_log.information() << "DBx221  Right Deviation = " << rightdev << "   B/C Right Peak Boundary = " << rightpeakrange << endl;
      }

      // e) Fit peak and process the result
      double chi2;
      bool fitpeakgood = this->fitPeak(peak, background, leftdev, rightdev, workspaceindex, chi2);

      if (!fitpeakgood)
      {
        // If not a good fit, ignore
        g_log.warning() << "Peak @ " << peak->getParameter("X0")
                        << " is not selected due to bad peak fitting." << endl;
        continue;
      }

      // f) Book keep the peak (index) with good fit result
      goodfitpeaks.push_back(hkl);
      goodfitchi2.push_back(chi2);

      // g) For output
      calculateSinglePeak(peak, background);

      // h) Update loop variant
      peakOnRight = peak;

    } // FOR EACH PEAK

    // 3. Clean up
    g_log.information() << "[FitPeaks] Number of peak of good chi2 = " << goodfitchi2.size() << endl;

    return;
  }

  //-------- Prototype of new designed "Peak Fitting" ---------------------------------------------------------
  //
  /** Fit a single peak including its background
    * [NEW CODE: Under Construction Still]
    *
    * Assumption:
    * 1. peak must be in the range of [input peak center - leftdev, + rightdev]
    *
    * Arguments:
    * 1. leftdev, rightdev:  search range for the peak from the estimatio (theoretical)
    * Return: chi2 ... all the other parameter should be just in peak
    */
  bool FitPowderDiffPeaks::fitPeak(CurveFitting::BackToBackExponential_sptr peak, CurveFitting::BackgroundFunction_sptr background,
                                                 double leftdev, double rightdev, size_t workspaceindex, double& chi2)
  {
    const double SHRINKFACTOR = 0.9;

    // 1. Reconstruct 1 workspace2D object.  ws 0: original data; ws 1: pure peak; ws 2: pure background; ws 3: estimated background
    Workspace2D_sptr dataws;

    size_t wsindex = 1;
    double tof_h_inp = peak->centre();
    double tof_h_obs, tof_left, tof_right;

    bool searchproperpeakrange = true;
    bool returnwitherror = false;
    size_t ishrinkcount = 0;

    while (searchproperpeakrange)
    {
      double leftbound = tof_h_inp - leftdev;
      double rightbound = tof_h_inp + rightdev;
      g_log.information() << "DBx224  Tending to build partial workspace " << leftbound << " to " << rightbound
                          << ", Right Dev = " << rightdev << endl;
      dataws = buildPartialWorkspace(dataWS, workspaceindex, leftbound, rightbound);

      // 2. Estimate background
      estimateBackground(dataws);
      subtractBackground(dataws);

      // 3. Estimate peak center and fwhm
      int errordirection;
      bool findpropermax = findMaxHeight(dataws, wsindex, dataws->readX(0)[0], dataws->readX(0).back(),
                                         tof_h_obs, tof_left, tof_right, errordirection);

      // 4. Determine whether the given peak range makes sense
      if (findpropermax)
      {
        // Successfully quit
        searchproperpeakrange = false;
      }
      else
      {
        // Something wrong. Shink the size
        if (errordirection > 0)
        {
          rightdev *= SHRINKFACTOR;
        }
        else if (errordirection < 0)
        {
          leftdev *= SHRINKFACTOR;
        }

        ishrinkcount ++;

        // Quit with error
        if (ishrinkcount >= 10)
        {
          searchproperpeakrange = false;
          returnwitherror = true;
        }
      }
    } // ENDWHILE

    if (returnwitherror)
    {
      // Unable to find a proper region.  Return!
      g_log.warning() << "Unable to find a proper region to fit for peak possibly at " << tof_h_inp << endl;
      return false;
    }

    // 5. Estimate FWHM
    double obsfwhmleft, obsfwhmright;
    bool canestimate = estimateFWHM(dataws, 1, tof_h_obs, obsfwhmleft, obsfwhmright);
    if (!canestimate)
    {
      // if estimate fails use input information
      obsfwhmleft = peak->fwhm();
      obsfwhmright = obsfwhmleft;
      g_log.warning() << "[FitPeak] Failed to estimate peak's FWHM.  Use theoretical/input value instead. " << endl;
    }

    g_log.information() << "Observe peak max recorded value @ TOF = " << tof_h_obs
                        << "  +/- " << tof_left << ", " << tof_right
                        << " in [" << dataws->readX(0)[0] << ", " << dataws->readX(0).back() << "]" << std::endl;

    // 6. Find local background
    double leftpeakbound = tof_h_obs - EXCLUDEPEAKRANGEFACTOR * obsfwhmleft * 0.5;
    double rightpeakbound = tof_h_obs + EXCLUDEPEAKRANGEFACTOR * obsfwhmright * 0.5;
    bool goodbkgdfit = doFitBackground(dataws, background, leftpeakbound, rightpeakbound);
    subtractBackground(dataws);

    //    If fit is not good
    if (!goodbkgdfit)
    {
      g_log.warning() << "Fitting background fails for peak @ " << tof_h_obs << " (observed)" << std::endl;
      return false;
    }

    // 5. Fit by Gaussian to get some starting value
    double tof_h, sigma, height;
    doFitGaussianPeak(dataws, 1, tof_h_obs, obsfwhmleft, obsfwhmright, tof_h, sigma, height);
    peak->setParameter("S", sigma);
    peak->setParameter("I", height);

    // 6. Fit peak by the result from Gaussian
    pair<bool, double> fitresult = doFitPeak(dataws, peak, tof_h_obs, obsfwhmleft, obsfwhmright);
    bool goodfit = fitresult.first;
    chi2 = fitresult.second;

    // Fit for peak 2
    // bool goodpeakfit2 = doFitPeak(dataws, peak, tof_h, obsfwhmleft, obsfwhmright);

    // Compare 2 fitting and select the better outcome

    return goodfit;
  }


  /** Search peak (max height) in the given range
    * Give out the range of peak center for constaint later
    * Return: boolean for whether find a reasonable maximum height.
    */
  bool FitPowderDiffPeaks::findMaxHeight(API::MatrixWorkspace_sptr dataws, size_t wsindex,
                                         double xmin, double xmax, double& center, double& centerleftbound, double& centerrightbound,
                                         int& errordirection)
  {
    bool findpropermax = true;
    errordirection = 0;

    // 1. Determine xmin, xmax range
    std::vector<double>::const_iterator viter;
    const MantidVec& X = dataws->readX(wsindex);
    viter = std::lower_bound(X.begin(), X.end(), xmin);
    size_t ixmin = size_t(viter-X.begin());
    if (ixmin != 0)
      -- ixmin;
    viter = std::lower_bound(X.begin(), X.end(), xmax);
    size_t ixmax = size_t(viter-X.begin());

    // 2. Search imax
    const MantidVec& Y = dataws->readY(wsindex);
    size_t imax = ixmin;
    double maxY = Y[ixmin];
    for (size_t i = ixmin+1; i <= ixmax; ++i)
    {
      if (Y[i] > maxY)
      {
        maxY = Y[i];
        imax = i;
      }
    }

    g_log.information() << "[FindMaxHeight] iMax = " << imax << " of total " << X.size() << " points." << std::endl;

    center = X[imax];

    // 3. Determine the range of the peaks by +/-? data points
    // a) Determine left center bound
    if (imax >= 1)
    {
      bool down3left = true;
      int iend = int(imax)-4;
      if (iend < 0)
      {
        iend = 0;
      }
      for (size_t i = imax; i > size_t(iend); --i)
      {
        if (Y[i] <= Y[i-1])
        {
          down3left = false;
          break;
        }
      }
      if (down3left)
      {
        centerleftbound = X[imax-1];
      }
      else
      {
        centerleftbound = X[iend];
      }
    }
    else
    {
      g_log.warning() << "A Peak Cannot Appear At The Low End of A Workspace. " << endl;
      findpropermax = false;
      errordirection = -1;
    }

    // b) Determine right center bound
    if (imax < X.size()-1)
    {
      bool down3right = true;
      size_t iend = imax + 4;
      if (iend >= X.size())
      {
        iend = X.size()-1;
      }
      for (size_t i = imax; i < iend; ++i)
      {
        if (Y[i] <= Y[i+1])
        {
          down3right = false;
          break;
        }
      }
      if (down3right)
      {
        centerrightbound = X[imax+1];
      }
      else
      {
        centerrightbound = X[iend];
      }
    }
    else
    {
      g_log.warning() << "A Peak Cannot Appear At The Upper End of A Workspace. " << std::endl;
      findpropermax = false;
      errordirection = 1;
    }

    return findpropermax;
  }


  /** Build a partial workspace from source
    */
  Workspace2D_sptr FitPowderDiffPeaks::buildPartialWorkspace(API::MatrixWorkspace_sptr sourcews, size_t workspaceindex,
                                                             double leftbound, double rightbound)
  {
    // 1. Check
    const MantidVec& X = sourcews->readX(workspaceindex);
    const MantidVec& Y = sourcews->readY(workspaceindex);
    const MantidVec& E = sourcews->readE(workspaceindex);

    if (leftbound >= rightbound)
    {
      stringstream errmsg;
      errmsg << "[BuildPartialWorkspace] Input left boundary = " << leftbound << " is larger than input right boundary "
             << rightbound << ".  It is not allowed. ";
      g_log.error() << errmsg.str() << endl;
      throw std::invalid_argument(errmsg.str());
    }
    if (leftbound >= X.back() || rightbound <= X[0])
    {
      throw std::invalid_argument("Boundary is out side of the input data set. ");
    }

    // 1. Determine the size of the "partial" workspace
    int ileft = static_cast<int>(std::lower_bound(X.begin(), X.end(), leftbound) - X.begin());
    if (ileft > 0)
      -- ileft;
    int iright = static_cast<int>(std::lower_bound(X.begin(), X.end(), rightbound) - X.begin());
    if (iright >= static_cast<int>(X.size()))
      -- iright;

    size_t wssize = static_cast<size_t>(iright-ileft+1);

    // 2. Build the partial workspace
    size_t nspec = 6;
    Workspace2D_sptr partws = boost::dynamic_pointer_cast<Workspace2D>
        (API::WorkspaceFactory::Instance().create("Workspace2D", nspec, wssize, wssize));

    // 3. Put data there
    for (size_t iw = 0; iw < partws->getNumberHistograms(); ++iw)
    {
      MantidVec& nX = partws->dataX(iw);
      for (size_t i = 0; i < wssize; ++i)
      {
        nX[i] = X[i+ileft];
      }
    }
    MantidVec& nY = partws->dataY(0);
    MantidVec& nE = partws->dataE(0);
    for (size_t i = 0; i < wssize; ++i)
    {
      nY[i] = Y[i+ileft];
      nE[i] = E[i+ileft];
    }

    // 4. Temp output
    stringstream wss;
    for (size_t i = 0; i < partws->readX(0).size(); ++i)
      wss << setw(10) << setprecision(6) << partws->readX(0)[i] << setw(10) << setprecision(6) << partws->readY(0)[i] << endl;
    g_log.debug() << "Partial Workspace: " << endl << wss.str() << "..................................." << endl;

    return partws;
  }

  /** Estimate background for a pattern
    * Assumption: the peak must be in the data range completely
    * Algorithm: use two end data points for a linear background
    * Output: dataws spectrum 3 (workspace index 2)
    */
  void FitPowderDiffPeaks::estimateBackground(DataObjects::Workspace2D_sptr dataws)
  {
    const size_t numsamplepts = 3;

    if (dataws->dataX(0).size() < 20)
    {
      g_log.warning() << "There are two few points (" << dataws->dataX(0).size() << ") to estimate background.  The accuracy is in danger."
                      << std::endl;
    }

    // 2. Average the first and last three data points
    double y0 = 0;
    double x0 = 0;
    const MantidVec& X = dataws->readX(0);
    const MantidVec& Y = dataws->readY(0);
    for (size_t i = 0; i < numsamplepts; ++i)
    {
      x0 += X[i];
      y0 += Y[i];
    }
    x0 = x0/static_cast<double>(numsamplepts);
    y0 = y0/static_cast<double>(numsamplepts);

    double xf = 0;
    double yf = 0;
    for (size_t i = X.size()-numsamplepts; i < X.size(); ++i)
    {
      xf += X[i];
      yf += Y[i];
    }
    xf = xf/static_cast<double>(numsamplepts);
    yf = yf/static_cast<double>(numsamplepts);

    // 3. Calculate B(x) = B0 + B1*x
    double b1 = (yf - y0)/(xf - x0);
    double b0 = yf - b1*xf;

    // 4. Calcualte background
    MantidVec& bY = dataws->dataY(2);
    for (size_t i = 0; i < bY.size(); ++i)
    {
      bY[i] = b0 + b1*X[i];
    }

    return;
  }


  /** Subtract background
    * This is an operation within the specially defined data workspace for peak fitting
    * Output: spectrum 2 (workspace 1) for data with background subtracted
    */
  void FitPowderDiffPeaks::subtractBackground(Workspace2D_sptr dataws)
  {
    const MantidVec& dataY = dataws->readY(0);
    const MantidVec& bkgdY = dataws->readY(2);
    MantidVec& nobY = dataws->dataY(1);
    MantidVec& nobE = dataws->dataE(1);

    size_t numpts = dataY.size();

    for (size_t i = 0; i < numpts; ++i)
    {
      nobY[i] = dataY[i] - bkgdY[i];
      if (nobY[i] > 1.0)
      {
        nobE[i] = sqrt(nobY[i]);
      }
      else
      {
        nobE[i] = 1.0;
      }
    }

    // Debug output
    stringstream wss;
    for (size_t i = 0; i < dataws->readX(1).size(); ++i)
      wss << setw(12) << setprecision(6) << dataws->readX(1)[i]
          << setw(12) << setprecision(6) << dataws->readY(1)[i]
          << setw(12) << setprecision(6) << dataws->readE(1)[i] << endl;
    g_log.debug() << "Peak with background removed:\n" << wss.str() << "................................." << endl;

    return;
  }

  /** Estimate FWHM for the peak observed
    * Algorithm: From the top.  Get the maximum value. Calculate the half maximum value.  Find the range of X
    */
  bool FitPowderDiffPeaks::estimateFWHM(DataObjects::Workspace2D_sptr dataws, size_t wsindex,
                                        double tof_h, double& leftfwhm, double& rightfwhm)
  {
    // 1. Get the value of the Max Height
    const MantidVec& X = dataws->readX(0);
    const MantidVec& Y = dataws->readY(wsindex);

    size_t icenter = static_cast<size_t>(std::lower_bound(X.begin(), X.end(), tof_h) - X.begin());
    if (icenter <= 1 || icenter >= X.size()-2)
    {
      g_log.warning() << "Peak center is at the edge of input data.  "
                      << "It is unable to proceed the estimate of FWHM.  Quit with error!." << std::endl;
      return false;
    }

    double maxH;
    if (Y[icenter] < Y[icenter-1])
    {
      // Treat with lower_bound uncertainty
      icenter --;
      maxH = Y[icenter-1];
    }
    else
    {
      maxH = Y[icenter];
    }
    if (maxH <= 0.0)
    {
      g_log.error() << "Max height negative.  Fatal error is design of the algorithm." << std::endl;
      throw std::runtime_error("Maximum height of a diffraction peak is negative.");
    }
    double halfMax = maxH*0.5;

    // 2. Deal with left side
    bool continueloop = true;
    size_t index = icenter-1;
    while (continueloop)
    {
      if (Y[index] <= halfMax)
      {
        // Located the data points
        continueloop = false;
      }
      else if (index == 0)
      {
        // Reach the end of the boundary, but haven't found.  return with error.
        g_log.warning() << "[EstimateFWHM] The peak is not complete (left side) in the given data range." << std::endl;
        return false;
      }
      else
      {
        --index;
      }
    }
    double x0 = X[index];
    double xf = X[index+1];
    double y0 = Y[index];
    double yf = Y[index+1];

    // Formular for linear iterpolation: X = [(xf-x0)*Y - (xf*y0-x0*yf)]/(yf-y0)
    double xl = linearInterpolateX(x0, xf, y0, yf, halfMax);

    leftfwhm = 2.0*(tof_h-xl);

    // 3. Deal with right side
    continueloop = true;
    index = icenter+1;
    while (continueloop)
    {
      if (Y[index] <= halfMax)
      {
        // Located the data points
        continueloop = false;
      }
      else if (index == Y.size()-1)
      {
        // Reach the end of the boundary, but haven't found.  return with error.
        g_log.warning() << "[EstimateFWHM] The peak is not complete (right side) in the given data range." << std::endl;
        return false;
      }
      else
      {
        ++index;
      }
    }
    x0 = X[index-1];
    xf = X[index];
    y0 = Y[index-1];
    yf = Y[index];

    // Formular for linear iterpolation: X = [(xf-x0)*Y - (xf*y0-x0*yf)]/(yf-y0)
    double xr = linearInterpolateX(x0, xf, y0, yf, halfMax);

    rightfwhm = 2.0*(xr-tof_h);

    // 4. Debug out
    g_log.debug() << "[Estimate FWHM] Left FWHM = " << leftfwhm << ", Right FWHM = " << rightfwhm << endl;

    return true;
  }

  /** Fit background function by removing the peak properly
    */
  bool FitPowderDiffPeaks::doFitBackground(DataObjects::Workspace2D_sptr dataws, BackgroundFunction_sptr background,
                                           double leftpeakbound, double rightpeakbound)
  {
    // 1. Get X and Y vector. Spectrum 3 (workspace index 2) is for background
    const MantidVec& X = dataws->readX(2);
    const MantidVec& dataY = dataws->readY(0);
    const MantidVec& dataE = dataws->readE(0);
    MantidVec& bY = dataws->dataY(2);
    MantidVec& bE = dataws->dataE(2);

    // 2. Remove peak by linear iterpolation
    size_t ileft = static_cast<size_t>(std::lower_bound(X.begin(), X.end(), leftpeakbound)-X.begin());
    if (ileft == 0)
    {
      ++ileft;
    }
    size_t iright = static_cast<size_t>(std::lower_bound(X.begin(), X.end(), rightpeakbound)-X.begin());
    if (iright >= X.size())
    {
      iright = X.size()-1;
    }
    g_log.information() << "[FitBackground] iLeft = " << ileft << ", iRight = " << iright << ".  Total Points = " << X.size() << endl;

    // a) Left to peak
    for (size_t i = 0; i < ileft; ++i)
    {
      bY[i] = dataY[i];
      bE[i] = dataE[i];
    }

    // b) Under peak
    double x0 = X[ileft-1];
    double y0 = dataY[ileft-1];
    double xf = X[iright];
    double yf = dataY[iright];
    for (size_t i = ileft; i < iright; ++i)
    {
      double x = X[i];
      double y = linearInterpolateY(x0, xf, y0, yf, x);
      bY[i] = y;
      if (y > 1.0)
        bE[i] = sqrt(y);
      else
        bE[i] = 1.0;
    }

    // c) Right to peak
    for (size_t i = iright; i < bY.size(); ++i)
    {
      bY[i] = dataY[i];
      bE[i] = dataE[i];
    }

    // 3. Fit
    API::IAlgorithm_sptr fitalg = this->createSubAlgorithm("Fit", -1.0, -1.0, true);
    fitalg->initialize();

    g_log.information() << "Function To Fit: " << background->asString() << ".  Number of points  to fit =  "
                        << X.size() << std::endl;

    // b) Set property
    fitalg->setProperty("Function", boost::shared_ptr<API::IFunction>(background));
    fitalg->setProperty("InputWorkspace", dataws);
    fitalg->setProperty("WorkspaceIndex", 2);
    fitalg->setProperty("Minimizer", "Levenberg-MarquardtMD");
    fitalg->setProperty("CostFunction", "Least squares");
    fitalg->setProperty("MaxIterations", 1000);
    fitalg->setProperty("Output", "FitBackground");

    // c) Execute
    bool successfulfit = fitalg->execute();
    if (!fitalg->isExecuted() || ! successfulfit)
    {
      // Early return due to bad fit
      g_log.warning() << "Fitting background function failed. " << std::endl;
      return false;
    }

    // d) Analyze the fitting result
    double chi2 = fitalg->getProperty("OutputChi2overDoF");
    std::string fitstatus = fitalg->getProperty("OutputStatus");

    API::ITableWorkspace_sptr paramws = fitalg->getProperty("OutputParameters");
    std::string infofit = parseFitParameterWorkspace(paramws);

    g_log.information() << "Fit Linear Background: result:  Chi^2 = " << chi2
                        << " Fit Status = " << fitstatus << std::endl << infofit << std::endl;

    API::MatrixWorkspace_sptr outdataws =
        fitalg->getProperty("OutputWorkspace");
    const MantidVec& fitB = outdataws->readY(1);

    // e) Debug output
    std::stringstream datass;
    for (size_t i = 0; i < fitB.size(); ++i)
    {
      datass << outdataws->readX(1)[i] << setw(5) << " " << fitB[i] << "  " << bY[i] << std::endl;
    }
    g_log.debug() << "Fitted Background:  Index, Fitted, Raw\n" << datass.str() << "........................." << std::endl;

    // f) Replace the background by fitted background
    for (size_t i = 0; i < fitB.size(); ++i)
    {
      bY[i] = fitB[i];
    }

    return true;

  }

  /** Fit peak without background i.e, with background removed
    * Arguments:
    *  - tof_h:     estimated/guessed center of the peak
    *  - leftfwhm:  half peak width of the left side;
    *  - rightfwhm: half peak width of the right side;
    */
  std::pair<bool, double> FitPowderDiffPeaks::doFitPeak(Workspace2D_sptr dataws, CurveFitting::BackToBackExponential_sptr peakfunction,
                                                        double tof_h, double leftfwhm, double rightfwhm)
  {
    // FIXME: This should be a more flexible variable later.
    size_t numcycles = 2;

    // 1. Set peak's parameters, boundaries and ties
    peakfunction->setParameter("X0", tof_h);

    double centerleftend = tof_h-leftfwhm*0.5;
    double centerrightend = tof_h+rightfwhm*0.5;
    CurveFitting::BoundaryConstraint* centerbound =
        new CurveFitting::BoundaryConstraint(peakfunction.get(),"X0", centerleftend, centerrightend, false);
    peakfunction->addConstraint(centerbound);

    g_log.information() << "[DoFitPeak] Peak Center Boundary = " << centerleftend << ", " << centerrightend << endl;

    // 2. Set up multiple step fitting
    vector<std::string> parameternames = peakfunction->getParameterNames();
    std::vector<std::set<std::string> > parameters2fit;  // name, step

    // step 0
    std::set<std::string> parameterset0;
    parameterset0.insert("X0");
    parameterset0.insert("I");
    parameterset0.insert("A");
    parameterset0.insert("B");
    parameterset0.insert("S");
    parameters2fit.push_back(parameterset0);

    // step 1
    std::set<std::string> parameterset1;
    parameterset1.insert("X0");
    parameterset1.insert("I");
    parameterset1.insert("S");
    parameters2fit.push_back(parameterset1);
    /**/

    // 3. Fit in cycles
    double chi2;
    for (size_t icycle = 0; icycle < numcycles; ++icycle)
    {
      size_t numsteps = parameters2fit.size();

      for (size_t istep = 0; istep < numsteps; ++istep)
      {
        // -------------------  One Fit Iteration -----------------------
        set<string> parameter2fitset = parameters2fit[istep];

        // a) Set or remove tie
        for (size_t iparam = 0; iparam < parameternames.size(); ++iparam)
        {
          string parname = parameternames[iparam];
          peakfunction->removeTie(parname);

          size_t parcount = parameter2fitset.count(parname);
          if (parcount == 0)
          {
            // Set tie
            double parvalue = peakfunction->getParameter(parname);
            stringstream valss;
            valss << parvalue;
            string valuestring = valss.str();
            peakfunction->tie(parname, valuestring);
          }
        } // ENDFOREACH parameter

        // b) Fit
        g_log.information() << "[doFitPeak] Cycle " << icycle << ", Step " << istep << ": " << peakfunction->asString() << std::endl;

        /* Debug output to test the function
        for (size_t i = 0; i < dataws->readX(1).size(); ++i)
          cout << dataws->readX(1)[i] << "  " << dataws->readY(1)[i] << "  " << dataws->readE(1)[i] << endl;
          */

        API::IAlgorithm_sptr fitalg = createSubAlgorithm("Fit", -1, -1, true);
        fitalg->initialize();

        fitalg->setProperty("Function", boost::dynamic_pointer_cast<API::IFunction>(peakfunction));
        fitalg->setProperty("InputWorkspace", dataws);
        fitalg->setProperty("WorkspaceIndex", 1);
        fitalg->setProperty("Minimizer", "Levenberg-MarquardtMD");
        fitalg->setProperty("CostFunction", "Least squares");
        fitalg->setProperty("MaxIterations", 1000);
        fitalg->setProperty("Output", "FitPeak");

        // c)  Result
        bool successfulfit = fitalg->execute();
        if (successfulfit)
        {
          std::string fitresult = parseFitResult(fitalg, chi2);
          g_log.information() << "[Fit Peak Cycle " << icycle << "] Result:\n" << fitresult << endl;

          API::ITableWorkspace_sptr paramws = fitalg->getProperty("OutputParameters");
          std::string infofit = parseFitParameterWorkspace(paramws);
          g_log.information() << "Fitted Parameters: " << endl << infofit << endl;

          // DB output for data
          API::MatrixWorkspace_sptr outdataws = fitalg->getProperty("OutputWorkspace");
          const MantidVec& allX = outdataws->readX(0);
          const MantidVec& fitY = outdataws->readY(1);
          const MantidVec& rawY = outdataws->readY(0);

          std::stringstream datass;
          for (size_t i = 0; i < fitY.size(); ++i)
          {
            datass << allX[i] << setw(5) << " " << fitY[i] << "  " << rawY[i] << std::endl;
          }
          g_log.debug() << "Fitted Peak " << icycle << "  " << istep << ":  X \tFitY\tRawY\n"
                        << datass.str() << "........................." << std::endl;
        }
      } // ENDFOR FIT Cycle

    } // Fit cycles

    pair<bool, double> returnvalue = make_pair(true, chi2);

    return returnvalue;
  }

  /** Fit background-removed peak by Gaussian
    */
  bool FitPowderDiffPeaks::doFitGaussianPeak(DataObjects::Workspace2D_sptr dataws, size_t workspaceindex,
                                             double in_center, double leftfwhm, double rightfwhm,
                                             double& center, double& sigma, double& height)
  {
    // 1. Estimate
    const MantidVec& X = dataws->readX(workspaceindex);
    const MantidVec& Y = dataws->readY(workspaceindex);

    height = 0;
    for (size_t i = 1; i < X.size(); ++i)
    {
      height += (X[i]-X[i-1])*Y[i];
    }
    sigma = (leftfwhm+rightfwhm)*0.5;

    // 2. Use factory to generate Gaussian
    auto temppeak = API::FunctionFactory::Instance().createFunction("Gaussian");
    auto gaussianpeak = boost::dynamic_pointer_cast<API::IPeakFunction>(temppeak);
    gaussianpeak->setHeight(height);
    gaussianpeak->setCentre(in_center);
    gaussianpeak->setFwhm(sigma);

    // b) Constraint
    double centerleftend = in_center-leftfwhm*0.5;
    double centerrightend = in_center+rightfwhm*0.5;
    CurveFitting::BoundaryConstraint* centerbound =
        new CurveFitting::BoundaryConstraint(gaussianpeak.get(),"PeakCentre", centerleftend, centerrightend, false);
    gaussianpeak->addConstraint(centerbound);

    // 3. Fit
    API::IAlgorithm_sptr fitalg = createSubAlgorithm("Fit", -1, -1, true);
    fitalg->initialize();

    fitalg->setProperty("Function", boost::dynamic_pointer_cast<API::IFunction>(gaussianpeak));
    fitalg->setProperty("InputWorkspace", dataws);
    fitalg->setProperty("WorkspaceIndex", 1);
    fitalg->setProperty("Minimizer", "Levenberg-MarquardtMD");
    fitalg->setProperty("CostFunction", "Least squares");
    fitalg->setProperty("MaxIterations", 1000);
    fitalg->setProperty("Output", "FitGaussianPeak");

    // iv)  Result
    bool successfulfit = fitalg->execute();
    if (!fitalg->isExecuted() || ! successfulfit)
    {
        // Early return due to bad fit
      g_log.warning() << "Fitting Gaussian peak for peak around " << gaussianpeak->centre() << std::endl;
      return false;
    }

    double chi2;
    std::string fitresult = parseFitResult(fitalg, chi2);
    g_log.information() << "[Fit Gaussian Peak] Result:\n" << fitresult << endl;

    // 4. Get result
    center = gaussianpeak->centre();
    height = gaussianpeak->height();
    double fwhm = gaussianpeak->fwhm();
    if (fwhm <= 0.0)
    {
      return false;
    }
    sigma = fwhm/2.35;

    // 5. Debug output
    API::ITableWorkspace_sptr paramws = fitalg->getProperty("OutputParameters");
    std::string infofit = parseFitParameterWorkspace(paramws);
    g_log.information() << "Fitted Gaussian Parameters: " << endl << infofit << endl;

    // DB output for data
    /*
    API::MatrixWorkspace_sptr outdataws = fitalg->getProperty("OutputWorkspace");
    const MantidVec& allX = outdataws->readX(0);
    const MantidVec& fitY = outdataws->readY(1);
    const MantidVec& rawY = outdataws->readY(0);

    std::stringstream datass;
    for (size_t i = 0; i < fitY.size(); ++i)
    {
      datass << allX[i] << setw(5) << " " << fitY[i] << "  " << rawY[i] << std::endl;
    }
    std::cout << "Fitted Gaussian Peak:  Index, Fittet, Raw\n" << datass.str() << "........................." << std::endl;
    */

    return true;
  }

  /** Parse fit result
    */
  std::string FitPowderDiffPeaks::parseFitResult(API::IAlgorithm_sptr fitalg, double& chi2)
  {
    stringstream rss;

    chi2 = fitalg->getProperty("OutputChi2overDoF");
    string fitstatus = fitalg->getProperty("OutputStatus");

    rss << "  [Algorithm Fit]:  Chi^2 = " << chi2
        << "; Fit Status = " << fitstatus;

    return rss.str();
  }

  /** Parse parameter workspace returned from Fit()
    */
  std::string FitPowderDiffPeaks::parseFitParameterWorkspace(API::ITableWorkspace_sptr paramws)
  {
    // 1. Check
    if (!paramws)
    {
      g_log.warning() << "Input table workspace is NULL.  " << std::endl;
      return "";
    }

    // 2. Parse
    std::stringstream msgss;
    size_t numrows = paramws->rowCount();
    for (size_t i = 0; i < numrows; ++i)
    {
      API::TableRow row = paramws->getRow(i);
      std::string parname;
      double parvalue, parerror;
      row >> parname >> parvalue >> parerror;

      msgss << parname << " = " << setw(10) << setprecision(5) << parvalue
            << " +/- " << setw(10) << setprecision(5) << parerror << std::endl;
    }

    return msgss.str();
  }

  //----------------------------------------------------------------------------------------------

  /** Create a Workspace2D for fitted peaks (pattern)
    */
  Workspace2D_sptr FitPowderDiffPeaks::genOutputFittedPatternWorkspace(std::vector<double> pattern, int workspaceindex)
  {
    // 1. Init
    const MantidVec& X = dataWS->readX(workspaceindex);
    const MantidVec& Y = dataWS->readY(workspaceindex);

    if (pattern.size() != X.size())
    {
      stringstream errmsg;
      errmsg << "Input pattern (" << pattern.size() << ") and algorithm's input workspace ("
             << X.size() << ") have different size. ";
      g_log.error() << errmsg.str() << endl;
      throw std::logic_error(errmsg.str());
    }

    size_t numpts = X.size();

    // 2. Create data workspace
    Workspace2D_sptr dataws = boost::dynamic_pointer_cast<Workspace2D>(
          WorkspaceFactory::Instance().create("Workspace2D", 3, pattern.size(), pattern.size()));

    // 3. Set up
    for (size_t iw = 0; iw < 3; ++iw)
    {
      MantidVec& newX = dataws->dataX(iw);
      for (size_t i = 0; i < numpts; ++i)
      {
        newX[i] = X[i];
      }
    }

    MantidVec& newY0 = dataws->dataY(0);
    MantidVec& newY1 = dataws->dataY(1);
    MantidVec& newY2 = dataws->dataY(2);
    for (size_t i = 0; i < numpts; ++i)
    {
      newY0[i] = Y[i];
      newY1[i] = pattern[i];
      newY2[i] = Y[i] - pattern[i];
    }

    // 4. Debug
    // FIXME Remove this section after unit test is finished.
    std::ofstream ofile;
    ofile.open("fittedpeaks.dat");
    for (size_t i = 0; i < numpts; ++i)
    {
      ofile << setw(12) << setprecision(5) << X[i]
            << setw(12) << setprecision(5) << pattern[i]
            << setw(12) << setprecision(5) << dataws->readY(0)[i]
            << setw(12) << setprecision(5) << dataws->readY(2)[i] << endl;
    }
    ofile.close();

    return dataws;
  }


  /** Calcualte the value of a single peak in a given range.
    * Output is send to global data structure: mPeakData
    */
  void FitPowderDiffPeaks::calculateSinglePeak(
      CurveFitting::BackToBackExponential_sptr peak, CurveFitting::BackgroundFunction_sptr background)
  {
    // i.   Determine the range of X (tof)
    double fwhm = peak->fwhm();
    double tof_h = peak->centre();
    double leftbound = tof_h - EXCLUDEPEAKRANGEFACTOR * fwhm;
    double rightbound = tof_h + EXCLUDEPEAKRANGEFACTOR * fwhm;

    // cout << "DBx128 Peak @ " << peak->centre() << " Range [" << leftbound << ", " << rightbound << "]" << endl;

    // ii.  Generate the vector for X-axis
    std::vector<double> tofs;
    std::vector<size_t> itofs;
    std::vector<double>::const_iterator vit;
    vit = std::lower_bound(dataWS->readX(workspaceindex).begin(), dataWS->readX(workspaceindex).end(), leftbound);
    size_t istart = size_t(vit-dataWS->readX(workspaceindex).begin());
    vit = std::lower_bound(dataWS->readX(workspaceindex).begin(), dataWS->readX(workspaceindex).end(), rightbound);
    size_t iend = size_t(vit-dataWS->readX(workspaceindex).begin());
    for (size_t i = istart; i < iend; ++i)
    {
      itofs.push_back(i);
      tofs.push_back(dataWS->readX(workspaceindex)[i]);
    }

    // iii. Generate a composite function
    API::CompositeFunction tempfunc;
    API::CompositeFunction_sptr compfunction = boost::make_shared<API::CompositeFunction>
        (tempfunc);
    compfunction->addFunction(peak);
    compfunction->addFunction(background);

    // iv.  Calculate the composite function
    API::FunctionDomain1DVector domain(tofs);
    API::FunctionValues values(domain);
    compfunction->function(domain, values);

    // v.   Book keep the result
    for (size_t i = istart; i < iend; ++i)
    {
      mPeakData[i] = values[i-istart];
    }

    /*
    int hkl2 = hkl[0]*hkl[0] + hkl[1]*hkl[1] + hkl[2]*hkl[2];
    mPeakData.insert(std::make_pair(hkl2, std::make_pair(itofs, values)));
    */

    return;
  }

  /** Generate a TableWorkspace for peaks with good fitting.
    * Table has column as H, K, L, d_h, X0, A(lpha), B(eta), S(igma), Chi2
    * Each row is a peak
    */
  TableWorkspace_sptr FitPowderDiffPeaks::genPeakParametersWorkspace(
      std::vector<std::vector<int> > goodfitpeaks, std::vector<double> goodfitchi2s)
  {
    // 1. Generate the TableWorkspace
    TableWorkspace* tablewsptr = new TableWorkspace();
    TableWorkspace_sptr tablews = TableWorkspace_sptr(tablewsptr);

    stringstream outbuf;

    tablews->addColumn("int", "H");
    tablews->addColumn("int", "K");
    tablews->addColumn("int", "L");

    tablews->addColumn("double", "d_h");
    tablews->addColumn("double", "TOF_h");
    tablews->addColumn("double", "Height");
    tablews->addColumn("double", "Alpha");
    tablews->addColumn("double", "Beta");
    tablews->addColumn("double", "Sigma");
    tablews->addColumn("double", "Chi2");

    outbuf << setw(10) << "H" << setw(10) << "K" << setw(10) << "L"
           << setw(10) << "d_h" << setw(10) << "X0" << setw(10) << "I"
           << setw(10) << "A" << setw(10) << "B" << setw(10) << "S" << setw(10) << "Chi2" << endl;

    // 2. Calculate d-values for each picked peak
    std::vector<std::pair<double, std::vector<int> > > dhkls;
    std::vector<std::vector<int> >::iterator peakiter;
    for (peakiter = goodfitpeaks.begin(); peakiter != goodfitpeaks.end(); ++peakiter)
    {
      std::vector<int> hkl = *peakiter;
      double d_h = calculateDspaceValue(hkl);
      dhkls.push_back(std::make_pair(d_h, hkl));
    }

    // 3. Sort peak indexes by d-spacing value
    std::sort(dhkls.begin(), dhkls.end());

    // 4. Add the peak data to each row
    for (size_t i = 0; i < dhkls.size(); ++i)
    {
      CurveFitting::BackToBackExponential_sptr peak = mPeaks[dhkls[i].second];

      TableRow newrow = tablews->appendRow();

      // i. H, K, L, d_h
      double dh = dhkls[i].first;
      std::vector<int> hkl = dhkls[i].second;

      newrow << hkl[0] << hkl[1] << hkl[2] << dh;

      outbuf << setw(10) << hkl[0] << setw(10) << hkl[1] << setw(10) << hkl[2]
             << setw(10) << setprecision(5) << dh;

      // ii. A, B, I, S, X0
      double p_a = peak->getParameter("A");
      double p_b = peak->getParameter("B");
      double p_i = peak->getParameter("I");
      double p_x = peak->getParameter("X0");
      double p_s = peak->getParameter("S");

      newrow << p_x << p_i << p_a << p_b << p_s;

      outbuf << setw(10) << setprecision(5) << p_x
             << setw(10) << setprecision(5) << p_i
             << setw(10) << setprecision(5) << p_a
             << setw(10) << setprecision(5) << p_b
             << setw(10) << setprecision(5) << p_s;

      // iii. chi2
      double chi2 = goodfitchi2s[i];

      newrow << chi2;

      outbuf << setw(10) << setprecision(5) << chi2 << endl;

    } // FOREACH Peak

    // 5. Debug output
    //    FIXME Remove this part after unit test is completed and passed.
    ofstream ofile;
    ofile.open("fittedparameters.txt");
    ofile << outbuf.str();
    ofile.close();

    return tablews;
  }

 //----------------------------------------------------------------------------------------------
  /** Genearte peaks from input workspace
    */
  void FitPowderDiffPeaks::genPeaksFromTable(
      DataObjects::TableWorkspace_sptr peakparamws, std::map<std::vector<int>, CurveFitting::BackToBackExponential_sptr>& peaks)
  {
    // 1. Check and clear input and output
    if (!peakparamws)
    {
      g_log.error() << "Input tableworkspace for peak parameters is invalid!" << std::endl;
      throw std::invalid_argument("Invalid input table workspace for peak parameters");
    }

    //  Clear output
    peaks.clear();

    // 2. Parse table workspace to 2 vector of maps
    vector<map<std::string, double> > peakparametermaps;
    vector<map<std::string, int> > peakhkls;
    vector<string> paramnames = peakparamws->getColumnNames();

    set<size_t> intindexes;
    set<size_t> dblindexes;
    for (size_t i = 0; i < paramnames.size(); ++i)
    {
      string parname = paramnames[i];
      if (parname.compare("H") == 0 || parname.compare("K") == 0 || parname.compare("L") == 0)
      {
        intindexes.insert(i);
      }
      else if (parname.compare("Alpha") == 0 || parname.compare("Beta") == 0 || parname.compare("Sigma2") == 0
               || parname.compare("Height") == 0 || parname.compare("TOF_h") == 0)
      {
        dblindexes.insert(i);
      }
    }

    double tempvalue;
    int tempint;

    size_t numrows = peakparamws->rowCount();
    size_t numcols = peakparamws->columnCount();

    for (size_t ir = 0; ir < numrows; ++ir)
    {
      // For each row
      API::TableRow paramrow = peakparamws->getRow(ir);
      map<string, double> temparameters;
      map<string, int> temphkl;

      for (size_t i = 0; i < numcols; ++i)
      {
        string parname = paramnames[i];
        try
        {
          if (intindexes.count(i) > 0)
          {
            paramrow >> tempint;
            temphkl.insert(std::make_pair(parname, tempint));
          }
          else if (dblindexes.count(i) > 0)
          {
            paramrow >> tempvalue;
            temparameters.insert(make_pair(parname, tempvalue));
          }
        }
        catch (std::runtime_error err)
        {
          g_log.error() << "TableRow " << ir << " Column " << i << " (" << peakparamws->getColumnNames()[i]
                        << ") Type mismatch." << endl;
          throw runtime_error("TableRow type mistmatch. ");
        }
      } // ENDFOREACH Column in a row

      // Add to map
      peakparametermaps.push_back(temparameters);
      peakhkls.push_back(temphkl);
    }// END FOR ROW

    g_log.notice() << "[GeneratePeaks] Create peak (parameters) map of " << peakhkls.size() << " entries." << endl;

    // 3. Generate Peaks
    double tofmin = dataWS->readX(workspaceindex)[0];
    double tofmax = dataWS->readX(workspaceindex).back();
    for (size_t ir = 0; ir < numrows; ++ir)
    {
      double center = peakparametermaps[ir]["TOF_h"];

      if (center > tofmin && center < tofmax)
      {
        // Generate peak if and only if its center is within TOF range
        CurveFitting::BackToBackExponential newpeak;
        newpeak.initialize();

        // Set up parameters
        std::vector<string> parnames = newpeak.getParameterNames();
        for (size_t i = 0; i < parnames.size(); ++i)
        {
          string parname = parnames[i];
          // Convert the input parameter name to BackToBackExponential's parameter name
          string inparname;
          if (parname.compare("A") == 0)
          {
            inparname = "Alpha";
          }
          else if (parname.compare("B") == 0)
          {
            inparname = "Beta";
          }
          else if (parname.compare("X0") == 0)
          {
            inparname = "TOF_h";
          }
          else if (parname.compare("I") == 0)
          {
            inparname = "Height";
          }
          else if (parname.compare("S") == 0)
          {
            inparname = "Sigma2";
          }
          double parvalue = peakparametermaps[ir][inparname];
          if (parname.compare("S") == 0)
            parvalue = sqrt(parvalue);

          newpeak.setParameter(parname, parvalue);
        }

        // Make to share pointer and set to instance data structure (map)
        CurveFitting::BackToBackExponential_sptr newpeakptr = boost::make_shared<CurveFitting::BackToBackExponential>(newpeak);

        std::vector<int> hkl;
        int h = peakhkls[ir]["H"];
        int k = peakhkls[ir]["K"];
        int l = peakhkls[ir]["L"];
        hkl.push_back(h); hkl.push_back(k); hkl.push_back(l);

        peaks.insert(std::make_pair(hkl, newpeakptr));

        g_log.information() << "[GeneratePeaks] Peak " << ir << " Input Center = " << setw(10) << setprecision(6) << center
                            << ".  Allowed Region = ["
                            << tofmin << ", " << tofmax << "].  Number of peaks = " << peaks.size() << endl;
      }
      else
      {
        g_log.information() << "[GeneratePeaks] Peak " << ir << " Input Center = " << center << ".  Allowed Region = ["
                            << tofmin << ", " << tofmax << "]" << ".  Out of Range" << endl;
      }

    } // ENDFOR Each potential peak

    return;
  }

  /** Import TableWorkspace containing the parameters for fitting
   * the diffrotometer geometry parameters
   */
  void FitPowderDiffPeaks::importParametersFromTable(
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
        API::TableRow trow = parameterWS->getRow(ir);
        trow >> parname >> value;
        parameters.insert(std::make_pair(parname, value));
    }

    return;
  }

  /** Crop data workspace
      */
  void FitPowderDiffPeaks::cropWorkspace(double tofmin, double tofmax)
  {
    API::IAlgorithm_sptr cropalg = this->createSubAlgorithm("CropWorkspace", -1, -1, true);
    cropalg->initialize();

    cropalg->setProperty("InputWorkspace", dataWS);
    cropalg->setPropertyValue("OutputWorkspace", "MyData");
    cropalg->setProperty("XMin", tofmin);
    cropalg->setProperty("XMax", tofmax);

    bool cropstatus = cropalg->execute();
    if (!cropstatus)
    {
      std::stringstream errmsg;
      errmsg << "DBx309 Cropping workspace unsuccessful.  Fatal Error. Quit!";
      g_log.error() << errmsg.str() << std::endl;
      throw std::runtime_error(errmsg.str());
    }

    dataWS = cropalg->getProperty("OutputWorkspace");
    if (!dataWS)
    {
      g_log.error() << "Unable to retrieve a Workspace2D object from subalgorithm Crop." << std::endl;
    }

    return;
  }

  //----------------------------------------------------------------------------------------------
  /** Calculate peak's d-spacing
    */
  double FitPowderDiffPeaks::calculateDspaceValue(std::vector<int> hkl)
  {
    g_log.information() << "HKL = " << hkl[0] << hkl[1] <<  hkl[2] << std::endl;

    // FIXME  It only works for the assumption that the lattice is cubical
    double lattice = mFuncParameters["LatticeConstant"];
    double h = static_cast<double>(hkl[0]);
    double k = static_cast<double>(hkl[1]);
    double l = static_cast<double>(hkl[2]);

    double d = lattice/sqrt(h*h+k*k+l*l);

    return d;
  }

} // namespace CurveFitting
} // namespace Mantid
