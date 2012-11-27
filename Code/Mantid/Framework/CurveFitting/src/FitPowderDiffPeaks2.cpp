#include "MantidCurveFitting/FitPowderDiffPeaks2.h"

#include "MantidKernel/ListValidator.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/Statistics.h"
#include "MantidKernel/Statistics.h"

#include "MantidAPI/TableRow.h"
#include "MantidAPI/Column.h"
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
#include "MantidCurveFitting/BackToBackExponential.h"
#include "MantidCurveFitting/ThermalNeutronBk2BkExpConvPV.h"

#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/split.hpp>

#include <fstream>
#include <iomanip>

#include <gsl/gsl_sf_erf.h>
#include <cmath>

/// Factor on FWHM for searching a peak
#define PEAKRANGEFACTOR 20.0
/// Factor on FWHM for excluding peak to fit background
#define EXCLUDEPEAKRANGEFACTOR 8.0
/// Factor on FWHM to fit a peak
#define WINDOWSIZE 3.0

using namespace Mantid;
using namespace Mantid::API;
using namespace Mantid::Kernel;
using namespace Mantid::DataObjects;

using namespace std;

namespace Mantid
{
namespace CurveFitting
{

  DECLARE_ALGORITHM(FitPowderDiffPeaks2)

  //----------------------------------------------------------------------------------------------
  /** Constructor
   */
  FitPowderDiffPeaks2::FitPowderDiffPeaks2()
  {
  }
    
  //----------------------------------------------------------------------------------------------
  /** Destructor
   */
  FitPowderDiffPeaks2::~FitPowderDiffPeaks2()
  {
  }
  
  //----------------------------------------------------------------------------------------------
  /** Set up documention
    */
  void FitPowderDiffPeaks2::initDocs()
  {
    setWikiSummary("Fit peaks in powder diffraction pattern. ");
    setOptionalMessage("Fit peaks in powder diffraction pattern. ");
  }

  //----------------------------------------------------------------------------------------------
  /** Parameter declaration
   */
  void FitPowderDiffPeaks2::init()
  {
    // Input data workspace
    declareProperty(new WorkspaceProperty<MatrixWorkspace>("InputWorkspace", "Anonymous", Direction::Input),
                    "Input workspace for data (diffraction pattern). ");

    // Output workspace
    declareProperty(new WorkspaceProperty<Workspace2D>("OutputWorkspace", "Anonymous2", Direction::Output),
                    "Output Workspace2D for the fitted peaks. ");

    // Input/output peaks table workspace
    declareProperty(new WorkspaceProperty<TableWorkspace>("BraggPeakParameterWorkspace", "AnonymousPeak", Direction::Input),
                    "TableWorkspace containg all peaks' parameters.");

    // Input and output instrument parameters table workspace
    declareProperty(new WorkspaceProperty<TableWorkspace>("InstrumentParameterWorkspace", "AnonymousInstrument", Direction::InOut),
                    "TableWorkspace containg instrument's parameters.");

    // Workspace to output fitted peak parameters
    declareProperty(new WorkspaceProperty<TableWorkspace>("OutputBraggPeakParameterWorkspace", "AnonymousOut2", Direction::Output),
                    "Output TableWorkspace containing the fitted peak parameters for each peak.");

    // Zscore table workspace
    declareProperty(new WorkspaceProperty<TableWorkspace>("OutputZscoreWorkspace", "ZscoreTable", Direction::Output),
                    "Output TableWorkspace containing the Zscore of the fitted peak parameters. ");

    // Workspace index of the
    declareProperty("WorkspaceIndex", 0, "Worskpace index for the data to refine against.");

    // Range of the peaks to fit
    declareProperty("MinTOF", EMPTY_DBL(), "Minimum TOF to fit peaks.  ");
    declareProperty("MaxTOF", EMPTY_DBL(), "Maximum TOF to fit peaks.  ");

    vector<string> fitmodes(2);
    fitmodes[0] = "Robust";
    fitmodes[1] = "Confident";
    auto fitvalidator = boost::make_shared<StringListValidator>(fitmodes);
    declareProperty("FittingMode", "Robust", fitvalidator, "Fitting mode such that user can determine"
                    "whether the input parameters are trustful or not.");

    // Option to calculate peak position from (HKL) and d-spacing data
    declareProperty("UseGivenPeakCentreTOF", true, "Use each Bragg peak's centre in TOF given in BraggPeakParameterWorkspace."
                    "Otherwise, calculate each peak's centre from d-spacing.");

    vector<string> genpeakoptions;
    genpeakoptions.push_back("(HKL) & Calculation");
    genpeakoptions.push_back("From Bragg Peak Table");
    auto propvalidator = boost::make_shared<StringListValidator>(genpeakoptions);
    declareProperty("PeakParametersStartingValueFrom", "(HKL) & Calculation", propvalidator,
                    "Choice of how to generate starting values of Bragg peak profile parmeters.");


    // Flag to calculate and trust peak parameters from instrument
    // declareProperty("ConfidentInInstrumentParameters", false, "Option to calculate peak parameters from "
    //     "instrument parameters.");

    // Option to denote that peaks are related
    declareProperty("PeaksCorrelated", false, "Flag for fact that all peaks' corresponding profile parameters "
                    "are correlated by an analytical function");

    // Option for peak's HKL for minimum d-spacing
    auto arrayprop = new ArrayProperty<int>("MinimumHKL", "");
    declareProperty(arrayprop, "Miller index of the left most peak (peak with minimum d-spacing) to be fitted. ");

    // Number of the peaks to fit left to peak with minimum HKL
    declareProperty("NumberPeaksToFitBelowLowLimit", 0, "Number of peaks to fit with d-spacing value "
                    "less than specified minimum. ");

    // Right most peak property
    auto righthklprop = new ArrayProperty<int>("RightMostPeakHKL", "");
    declareProperty(righthklprop, "Miller index of the right most peak. "
                    "It is only required and used in RobustFit mode.");

    declareProperty("RightMostPeakLeftBound", EMPTY_DBL(), "Left bound of the right most peak. "
                    "Used in RobustFit mode.");

    declareProperty("RightMostPeakRightBound", EMPTY_DBL(), "Right bound of the right most peak. "
                    "Used in RobustFit mode.");

    return;
  }


  //----------------------------------------------------------------------------------------------
  /** Main execution
   */
  void FitPowderDiffPeaks2::exec()
  {
    // 1. Get input
    // data workspace
    m_dataWS = this->getProperty("InputWorkspace");
    m_wsIndex = this->getProperty("WorkspaceIndex");
    if (m_wsIndex < 0 || m_wsIndex > static_cast<int>(m_dataWS->getNumberHistograms()))
    {
      stringstream errss;
      errss << "Input workspace = " << m_wsIndex << " is out of range [0, " << m_dataWS->getNumberHistograms();
      g_log.error(errss.str());
      throw std::invalid_argument(errss.str());
    }

    // table workspaces for parameters
    DataObjects::TableWorkspace_sptr peakWS = this->getProperty("BraggPeakParameterWorkspace");
    DataObjects::TableWorkspace_sptr parameterWS = this->getProperty("InstrumentParameterWorkspace");

    // fitting range
    double tofmin = getProperty("MinTOF");
    double tofmax = getProperty("MaxTOF");
    if (tofmin == EMPTY_DBL())
      tofmin = m_dataWS->readX(m_wsIndex)[0];
    if (tofmax == EMPTY_DBL())
      tofmax = m_dataWS->readX(m_wsIndex).back();

    m_minimumHKL = getProperty("MinimumHKL");
    m_numPeaksLowerToMin = getProperty("NumberPeaksToFitBelowLowLimit");

    // fitting algorithm option
    string fitmode = getProperty("FittingMode");
    if (fitmode.compare("Robust") == 0)
    {
      m_fitMode = ROBUSTFIT;
    }
    else if (fitmode.compare("Confident") == 0)
    {
      m_fitMode = TRUSTINPUTFIT;
    }
    else
    {
      throw runtime_error("Input fit mode can only accept either Robust or Confident. ");
    }

    m_useGivenTOFh = getProperty("UseGivenPeakCentreTOF");
    // m_confidentInInstrumentParameters = getProperty("ConfidentInInstrumentParameters");

    // peak parameter generation option
    string genpeakparamalg = getProperty("PeakParametersStartingValueFrom");
    if (genpeakparamalg.compare("(HKL) & Calculation") == 0)
    {
      m_genPeakStartingValue = HKLCALCULATION;
    }
    else if (genpeakparamalg.compare("From Bragg Peak Table") == 0)
    {
      m_genPeakStartingValue = FROMBRAGGTABLE;
    }
    else
    {
      throw runtime_error("Input option from PeakParametersStaringValueFrom is not supported.");
    }

    // Right most peak information
    m_rightmostPeakHKL = getProperty("RightMostPeakHKL");
    m_rightmostPeakLeftBound = getProperty("RightMostPeakLeftBound");
    m_rightmostPeakRightBound = getProperty("RightMostPeakRightBound");

    if (m_fitMode == ROBUSTFIT)
    {
      if (m_rightmostPeakHKL.size() == 0 || m_rightmostPeakLeftBound == EMPTY_DBL() ||
          m_rightmostPeakRightBound == EMPTY_DBL())
      {
        stringstream errss;
        errss << "If fit mode is 'RobustFit', then user must specify all 3 properties of right most peak "
              << "(1) Miller Index   (given size  = " << m_rightmostPeakHKL.size() << "), "
              << "(2) Left boundary  (given value = " << m_rightmostPeakLeftBound << "), "
              << "(3) Right boundary (given value = " << m_rightmostPeakRightBound << "). ";
        g_log.error(errss.str());
        throw runtime_error(errss.str());
      }
    }

    // 2. Crop input workspace
    cropWorkspace(tofmin, tofmax);

    // 3. Parse input table workspace
    importInstrumentParameterFromTable(parameterWS);
    genPeaksFromTable(peakWS);

    // 4. Fit peaks & get peak centers
#if 1
    m_indexGoodFitPeaks.clear();
    m_chi2GoodFitPeaks.clear();
    size_t numpts = m_dataWS->readX(m_wsIndex).size();
    m_peakData.reserve(numpts);
    for (size_t i = 0; i < numpts; ++i)
      m_peakData.push_back(0.0);

    g_log.notice() << "[FitPeaks] Total Number of Peak = " << m_peaks.size() << std::endl;
    if (m_fitMode == ROBUSTFIT)
    {
      fitPeaksRobust();
    }
    else
    {
      fitPeaksTrustInput();
    }
#else
    Replaced
    vector<pair<double, vector<int> > > inp_tofhkls;
    map<vector<int>, BackToBackExponential_sptr>::iterator peakiter;
    for (peakiter = m_peaksmap.begin(); peakiter != m_peaksmap.end(); ++peakiter)
    {
      vector<int> hkl = peakiter->first;
      BackToBackExponential_sptr peak = peakiter->second;
      double center = peak->centre();
      inp_tofhkls.push_back(make_pair(center, hkl));
    }
    fitPeaks(workspaceindex, m_indexGoodFitPeaks, m_chi2GoodFitPeaks);
#endif

    // 5. Create Output
    // a) Create a Table workspace for peak profile
    pair<TableWorkspace_sptr, TableWorkspace_sptr> tables = genPeakParametersWorkspace(m_indexGoodFitPeaks, m_chi2GoodFitPeaks);
    TableWorkspace_sptr outputpeaksws = tables.first;
    TableWorkspace_sptr ztablews = tables.second;
    setProperty("OutputBraggPeakParameterWorkspace", outputpeaksws);
    setProperty("OutputZscoreWorkspace", ztablews);

    // b) Create output data workspace (as a middle stage product)
    Workspace2D_sptr outdataws = genOutputFittedPatternWorkspace(m_peakData, m_wsIndex);
    setProperty("OutputWorkspace", outdataws);

    return;
  }

  //=================================  Fit PeakS ===============================
  //----------------------------------------------------------------------------
  /** Fit peaks in Robust mode.
    * Prerequisite:
    * 1. There are not any peaks that overlap to others;
    * Algorithm: All peaks are fit individually
    * Challenge:
    *   1. Starting geometry parameters can be off
    *   2. Peak profile parameters cannot be trusted at all.
    */
  void FitPowderDiffPeaks2::fitPeaksRobust()
  {
    // I. Prepare
    BackToBackExponential_sptr rightpeak;
    bool isrightmost = true;
    size_t numpeaks = m_peaks.size();

    // II. Create local background function.
    Polynomial_sptr backgroundfunction = boost::make_shared<Polynomial>(Polynomial());
    backgroundfunction->setAttributeValue("n", 2);
    backgroundfunction->initialize();

    // III. Fit peaks
    double firstpeakheight = -1;
    double chi2;

    for (int peakindex = static_cast<int>(numpeaks)-1; peakindex >= 0; --peakindex)
    {
      vector<int> peakhkl = m_peaks[peakindex].second.first;
      BackToBackExponential_sptr thispeak = m_peaks[peakindex].second.second;

      double peakleftbound, peakrightbound;
      stringstream infoss;
      if (isrightmost && peakhkl == m_rightmostPeakHKL)
      {
        // It is the specified right most peak.  Estimate background, peak height, fwhm, ...
        // 1. Determine the starting value of the peak
        peakleftbound = m_rightmostPeakLeftBound;
        peakrightbound = m_rightmostPeakRightBound;

        infoss << "[DBx102] The " << numpeaks-1-peakindex << "-th rightmost peak's miller index = "
               << peakhkl[0] << ", " << peakhkl[1] << ", " << peakhkl[2] << ", predicted at TOF = "
               << thispeak->centre() << ";  User specify boundary = [" << peakleftbound
               << ", " << peakrightbound  << "].";
        cout << infoss.str() << endl;

        fitSinglePeakRobust(thispeak, boost::dynamic_pointer_cast<BackgroundFunction>(backgroundfunction),
                            peakleftbound, peakrightbound, chi2);

        firstpeakheight = thispeak->height();
        rightpeak = thispeak;
        isrightmost = false;

      }
      else if (!isrightmost)
      {
        // All peaks but not the right most peak
        // 1. Validate inputs
        if (peakindex == static_cast<int>(numpeaks)-1)
          throw runtime_error("Impossible to have peak index as the right most peak here!");

        // 2. Determine the peak range
        double rightpeakshift = rightpeak->centre() - m_inputPeakCentres[peakindex+1];
        double thiscentre = thispeak->centre();
        double rightfwhm = rightpeak->fwhm();
        if (rightpeakshift > 0)
        {
          // tend to shift to right
          peakleftbound = thiscentre - rightfwhm;
          peakrightbound = thiscentre + rightfwhm + rightpeakshift;
        }
        else
        {
          // tendency to shift to left
          peakleftbound = thiscentre - rightfwhm - rightpeakshift;
          peakrightbound = thiscentre + rightfwhm;
        }
        if (peakrightbound > rightpeak->centre() - 3*rightpeak->fwhm())
        {
          // the search of peak's right end shouldn't exceed the left tail of its real right peak!
          // Remember this is robust mode.  Any 2 adjacent peaks should be faw enough.
          peakrightbound = rightpeak->centre() - 3*rightpeak->fwhm();

        }

        // 3. Fit peak
        bool fitgood = fitSinglePeakRefRight(thispeak, backgroundfunction, rightpeak, peakleftbound,
                                             peakrightbound, chi2);

        // -1
        // FIXME: replace magic number 10 with a more reasonable variable; and another magic number 2.0
        // Put the latest rightpeak if the fitting is good and peak height is reasonable.
        if (thispeak->height() >= firstpeakheight/10.0 && thispeak->fwhm() <= rightpeak->fwhm()*2.0)
        {
          rightpeak = thispeak;
        }

      }
      else
      {
        // It is right to the specified right most peak.  Skip to next peak
        infoss << "[DBx102] The " << numpeaks-1-peakindex << "-th rightmost peak's miller index = "
               << peakhkl[0] << ", " << peakhkl[1] << ", " << peakhkl[2] << ", predicted at TOF = "
               << thispeak->centre() << "; "
               << "User specify right most peak's miller index = "
               << m_rightmostPeakHKL[0] << ", " << m_rightmostPeakHKL[1] << ", " << m_rightmostPeakHKL[2]
               << ".  ";
        cout << infoss.str() << endl;
        continue;
      }
    } // ENDFOR Peaks

    return;
  }

  //----------------------------------------------------------------------------
  /** Fit each individual Bk2Bk-Exp-Conv-PV peaks
    * This part is under heavy construction, and will be applied to "FindPeaks2"
    * Notice: In this case, peaks with overlap will be treated!
    * Output: (1) goodfitpeaks, (2) goodfitchi2
   */
  void FitPowderDiffPeaks2::fitPeaksTrustInput()
  {
    int ipeak = static_cast<int>(m_peaks.size())-1;

    BackToBackExponential_sptr rightpeak = m_peaks[ipeak].second.second;

    while (ipeak >= 0)
    {
      // 1. Make a peak group
      vector<BackToBackExponential_sptr> peaks;

      bool makegroup = true;
      while (makegroup)
      {
        // Assumption/Fact: the right peak is far away.  or they will be in same peak group
        BackToBackExponential_sptr thispeak = m_peaks[ipeak].second.second;
        peaks.push_back(thispeak);

        --ipeak;

        if (ipeak < 0)
        {
          // this is last peak.  next peak does not exist
          makegroup = false;
        }
        else
        {
          // this is not the last peak.  search the left one.
          double thispeakleftbound = thispeak->centre() - thispeak->fwhm()*4.0;
          BackToBackExponential_sptr leftpeak = m_peaks[ipeak].second.second;
          double leftpeakrightbound = leftpeak->centre() + leftpeak->fwhm()*4.0;
          if (thispeakleftbound > leftpeakrightbound)
          {
            // This peak and next peak is faw enough!
            makegroup = false;
          }
        }
      } // while

      if (peaks.size() == 1)
      {
        // Only 1 peak
        fitSinglePeakConfident(peaks[0]);
      }
      else
      {
        fitOverlappedPeaks(peaks, rightpeak->fwhm());
        throw runtime_error("Requiring a good result processing function for 'rightpeak'");
        rightpeak = peaks[0];
      }
    } // ENDWHILE

    // 2. Output

    g_log.information() << "DBx415: Number of good fit peaks = " << m_indexGoodFitPeaks .size() << endl;

    // 3. Clean up
    g_log.information() << "[FitPeaks] Number of peak of good chi2 = " << m_chi2GoodFitPeaks.size() << endl;

    return;
  }

  //=================================  Fit 1 Peak ===============================
  //-----------------------------------------------------------------------------
  /** Fit a single peak including its background by a robust algorithm
    * Algorithm will
    *  1. Locate Maximum
    *  2.
    *
    * Assumption:
    * 1. peak must be in the range of [input peak center - leftdev, + rightdev]
    *
    * Prerequisit:
    * ---- NONE!
    *
    * Arguments
    * @param chi2   :  (output) chi square of the fit result
    *
    * Arguments:
    * 1. leftdev, rightdev:  search range for the peak from the estimatio (theoretical)
    * Return: chi2 ... all the other parameter should be just in peak
    */
  bool FitPowderDiffPeaks2::fitSinglePeakRobust(BackToBackExponential_sptr peak, BackgroundFunction_sptr backgroundfunction,
                                                double peakleftbound, double peakrightbound, double& chi2)
  {
    // 1. Build partial workspace
    Workspace2D_sptr peakws = buildPartialWorkspace(m_dataWS, m_wsIndex, peakleftbound, peakrightbound);

    //---------------------  Tested Above This Line! ---------------------------------

    // 2. Estimate and remove background
    size_t rawdata_wsindex = 0;
    size_t estbkgd_wsindex = 2;
    size_t peak_wsindex = 1;
    estimateBackgroundCoarse(peakws, backgroundfunction, rawdata_wsindex, estbkgd_wsindex, peak_wsindex);

    // 3. Estimate FWHM, peak centre, and height
    double centre, fwhm, height;
    string errmsg;
    bool pass = estimatePeakParameters(peakws, 1, centre, fwhm, height, errmsg);
    if (!pass)
    {
      // If estiamtion fails, quit b/c first/rightmost peak must be fitted.
      g_log.error(errmsg);
      throw runtime_error(errmsg);
    }

    // 4. Fit by Gaussian to get some starting value
    double tof_h, sigma;
    doFitGaussianPeak(peakws, peak_wsindex, centre, fwhm, fwhm, tof_h, sigma, height);

    // 5. Set the parameter for
    peak->setParameter("S", sigma);
    peak->setParameter("I", height);
    peak->setParameter("A", 1.0);
    peak->setParameter("B", 1.0);
    peak->setParameter("X0", tof_h);

    // 6. Fit peak by the result from Gaussian
    pair<bool, double> fitresult = doFitPeak(peakws, peak, fwhm);
    bool goodfit = fitresult.first;
    chi2 = fitresult.second;

    return goodfit;
  }

  /** Fit the non-right most peak in robust fit mode.  The peak parameters are from right peak
    * Algorithm:
    *  1. Find max height in range of (peakleftbound, peakrightbound)
    *  2. Estimate background
    *  3. Re-find max height in data w/ background removed;
    *  4. Build partial workspace based on the new centre
    *  5. Fit!
    */
  bool FitPowderDiffPeaks2::fitSinglePeakRefRight(BackToBackExponential_sptr peak, BackgroundFunction_sptr backgroundfunction,
                                                  BackToBackExponential_sptr rightpeak, double searchpeakleftbound,
                                                  double searchpeakrightbound, double& chi2)
  {
    // FIXME: Be careful about the right range not get overlapped with right peak's left wing!!!
    // 1. Search
    const MantidVec& vecX = m_dataWS->readX(m_wsIndex);


    // 3. Build partial data workspace for fit
    size_t icentre = findMaxValue(m_dataWS, m_wsIndex, searchpeakleftbound, searchpeakrightbound);
    double peakleftbound = vecX[icentre] - 4.0*rightpeak->fwhm();
    double peakrightbound = vecX[icentre] + 4.0*rightpeak->fwhm();
    Workspace2D_sptr peakws = buildPartialWorkspace(m_dataWS, m_wsIndex, peakleftbound, peakrightbound);

    // 4. Estimate and remove background
    estimateBackgroundCoarse(peakws, backgroundfunction, 0, 2, 1);

    // 5. Set up peak parameters
    peak->setCentre(vecX[icentre]);
    peak->setParameter("A", rightpeak->getParameter("A"));
    peak->setParameter("B", rightpeak->getParameter("B"));
    peak->setParameter("S", rightpeak->getParameter("C"));

    // 6. Fit
    //    FIXME: An appropriate minimzer should be decided here!
    double dampingfactor = 0.8;
    g_log.warning("Still don't know what the strategy is to fit these peaks!");
    fitSinglePeakConfident(peak);
    // fitSnglePeakConfident(peakws, thispeak, dampingfactor);

  }

  //-----------------------------------------------------------------------------
  /** Fit peak with confidence of the centre
    */
  bool FitPowderDiffPeaks2::fitSinglePeakConfident(BackToBackExponential_sptr peak)
  {
    // TODO: Whether this function can be merged with the next one?

    // 1. Get partial workspace and etc.
    double inp_tofh = peak->centre();
    double inp_fwhm = peak->fwhm();

    double leftbound = inp_tofh - 3.0*inp_fwhm;
    double rightbound = inp_tofh + 3.0*inp_fwhm;

    double windowsize = 0.5; // 1/2 of fwhm
#if 0
    Workspace2D_sptr dataws = estimatePeakRange(peak, windowsize);
#else
    throw runtime_error("Make this work!");
#endif

    // 2. Initial fit for background
#if 0
    fitBackground(dataws, background);
#else
    throw runtime_error("Make this work!");
#endif

    // 3. Build fit
#if 0
    doFitPeak(dataws, peak);
#else
    throw runtime_error("Make this work!");
#endif

    // 4. Fit all
#if 0
    doFitPeakBackground(dataws, peak, dampingfactor);
#else
    throw runtime_error("Make this work!");
#endif

    bool goodfit;

    return goodfit;
  }

  //---------------------------------------------------------------------------
  /** Fit peak with trustful peak parameters
    */
  bool FitPowderDiffPeaks2::fitPeakConfident(Workspace2D_sptr dataws, BackToBackExponential_sptr peak,
                                             BackgroundFunction_sptr backgroundfunction)
  {
    // 1. Set up contraint to the peak
    double leftbound = peak->centre() - peak->fwhm();
    double rightbound = peak->centre() + peak->fwhm();
    BoundaryConstraint* bc = new BoundaryConstraint(peak.get(), "X0", leftbound, rightbound);
    peak->addConstraint(bc);

    // 2. Estimate and remove bacground
    estimateBackgroundCoarse(dataws, backgroundfunction, 0, 2, 1);

    // 3. Do fit
    IAlgorithm_sptr fitalg = createSubAlgorithm("Fit");
    fitalg->initialize();

    fitalg->setProperty("Function", boost::shared_ptr<API::IFunction>(peak));
    fitalg->setProperty("InputWorkspace", dataws);
    fitalg->setProperty("WorkspaceIndex", 1);
    fitalg->setProperty("Minimizer", "Damping");
    fitalg->setProperty("CostFunction", "Least squares");
    fitalg->setProperty("MaxIterations", 1000);
    fitalg->setProperty("Output", "FitPeak");

    fitalg->execute();

    // 4. Parse fit result
    throw runtime_error("Implement parseFitResult() ASAP. ");
    // parseFitResult(fitalg, "FitPeak");

    return true;
  }



  //=======================  Fit 1 Set of Overlapped Peaks ======================

  //----------------------------------------------------------------------------
  /** Fit peaks in the same group (i.e., single peak or overlapped peaks).
    * It is of the same level as fit single peak
      */
  // FIXME: Who is using this function????
  void FitPowderDiffPeaks2::fitPeaksGroup(vector<size_t> peakindexes)
  {
    // 1) Get hold on the peak to fit
    BackToBackExponential_sptr leftpeak = m_peaks[peakindexes.back()].second.second;
    BackToBackExponential_sptr rightpeak = m_peaks[peakindexes[0]].second.second;

    double leftdev = PEAKRANGEFACTOR * leftpeak->fwhm() * 0.5;
    double rightdev = PEAKRANGEFACTOR * rightpeak->fwhm() * 0.5;

    // 2) Set up background function
    // TODO Make the order of the background more flexible
    CurveFitting::Polynomial_sptr background = boost::make_shared<CurveFitting::Polynomial>
        (CurveFitting::Polynomial());
    background->setAttributeValue("n", 1);
    background->initialize();

    // 3) Fit
    vector<double> chi2s;
    vector<bool> fitresults;

    if (m_fitMode == ROBUSTFIT)
    {
      // RobustFit mode, always 1 peak
      double chi2;
      bool fitresult = fitSinglePeakRobust(leftpeak, background, leftdev, rightdev, chi2);
      fitresults.push_back(fitresult);
      chi2s.push_back(chi2);
    }
    else if (m_fitMode == TRUSTINPUTFIT)
    {
      //
      BackToBackExponential_sptr peakonright;
      if (peakindexes[0] < m_peaks.size()-1)
      {
        // right  most peak is not global right most peak and thus fitted
        peakonright = m_peaks[peakindexes[0]+1].second.second;
      }
      else
      {
        // right most peak is the global right most peak
        peakonright = m_peaks[peakindexes[0]].second.second;
      }

      // Peaks are correlated
      if (peakindexes.size() == 1)
      {
        double chi2;
        bool fitresult;
#if 0
        fitresult = correlateFitSinglePeak(leftpeak, peakonright, background, leftdev, rightdev,
                                                workspaceindex, chi2);
#else
        throw runtime_error("Still trying to make a plan.");
#endif
        fitresults.push_back(fitresult);
        chi2s.push_back(chi2);
      }
      else
      {
#if 0
        fitresults = correlateFitPeaks(peakindexes, peakonright, background, leftdev, rightdev, workspaceindex, chi2s);
#else
        throw runtime_error("Still trying to make a plan.");
#endif
      }
    }

    // 4. Process result
    for (size_t i = 0; i < peakindexes.size(); ++i)
    {
      bool fitgood = fitresults[i];
      double chi2 = chi2s[i];
      if (fitgood)
      {
        m_indexGoodFitPeaks.push_back(peakindexes[i]);
        m_chi2GoodFitPeaks.push_back(chi2);
      }
      else
      {
        // If not a good fit, ignore
        BackToBackExponential_sptr peak = m_peaks[peakindexes[i]].second.second;
        vector<int> hkl = m_peaks[peakindexes[i]].second.first;
        g_log.warning() << "Peak (" << hkl[0] << ", " << hkl[1] << ", "
                        << hkl[2] << ") TOF = " << peak->getParameter("X0")
                        << " is not selected due to bad peak fitting." << endl;
      }
    }

    return;
  }


  /** Fit peak without background i.e, with background removed
    * Prerequisit:
    * 1. Peak parameters are set up to the peak function
    * 2. Background is removed
    *
    * Arguments:
    *  - tof_h:     estimated/guessed center of the peak
    *  - leftfwhm:  half peak width of the left side;
    *  - rightfwhm: half peak width of the right side;
    */
  std::pair<bool, double> FitPowderDiffPeaks2::doFitPeak(Workspace2D_sptr dataws,BackToBackExponential_sptr peakfunction,
                                                         double guessedfwhm)
  {
    // FIXME: This should be a more flexible variable later.
    size_t numcycles = 2;

    // 1. Set peak's parameters, boundaries and ties
    double tof_h = peakfunction->centre();

    double centerleftend = tof_h - guessedfwhm*0.5;
    double centerrightend = tof_h - guessedfwhm*0.5;
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
  bool FitPowderDiffPeaks2::doFitGaussianPeak(DataObjects::Workspace2D_sptr dataws, size_t workspaceindex,
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


  //-----------------------------------------------------------------------------
  /** Fit peaks with confidence in fwhm and etc.
    * @param gfwhm : guessed fwhm
    */
  bool FitPowderDiffPeaks2::fitOverlappedPeaks(vector<BackToBackExponential_sptr> peaks, double gfwhm)
  {
    // 1. Get peaks
#if 0
    make sure these are not necessary! Such as sort
    sort(peakindexes.begin(), peakindexes.end());

    vector<BackToBackExponential_sptr> peaks;
    for (size_t i = peakindexes.size()-1; i >= 0; --i)
    {
      BackToBackExponential_sptr peak = m_peaks[peakindexes[i]].second.second;
      peaks.push_back(peak);
    }
#endif

    double leftpeakcentre = m_dataWS->readX(m_wsIndex).back();
    double rightpeakcentre = m_dataWS->readX(m_wsIndex)[0];
    for (size_t ipk = 0; ipk < peaks.size(); ++ipk)
    {
      double peakcentre = peaks[ipk]->centre();
      if (peakcentre < leftpeakcentre)
        leftpeakcentre = peakcentre;
      if (peakcentre > rightpeakcentre)
        rightpeakcentre = peakcentre;
    }

    // 2. Boundary
    double leftbound = leftpeakcentre - gfwhm*3.0;
    double rightbound = rightpeakcentre + gfwhm*3.0;

    Workspace2D_sptr dataws = buildPartialWorkspace(m_dataWS, m_wsIndex, leftbound, rightbound);

    // 3. Fit (roughtly) for background
#if 0
    fitBackground(dataws, background);
#else
    throw runtime_error("Make this work!");
#endif

    // 4. Build composite function
    CompositeFunction_sptr peaksfunction(new CompositeFunction());
    for (size_t i = 0; i < peaks.size(); ++i)
      peaksfunction->addFunction(peaks[i]);

    // 5. Constrain
#if 0
    doFitPeaks(peaksfunction, dampingfactor);
#else
    throw runtime_error("Make this part work! ASAP!");
#endif
  }


  /** Calcualte the value of a group of peaks (sharing one background) in a given range.
    * Output is written to/recorded by global data structure: mPeakData
    */
  void FitPowderDiffPeaks2::calculate1PeakGroup(vector<size_t> peakindexes, BackgroundFunction_sptr background)
  {
    // 1. Left and right bound
    BackToBackExponential_sptr leftpeak = m_peaks[peakindexes.back()].second.second;
    double leftbound = leftpeak->centre() - 3.0*leftpeak->fwhm();
    BackToBackExponential_sptr rightpeak = m_peaks[peakindexes[0]].second.second;
    double rightbound = rightpeak->centre() + 3.0*rightpeak->fwhm();

    // cout << "DBx128 Peak @ " << peak->centre() << " Range [" << leftbound << ", " << rightbound << "]" << endl;

    // 2. Generate the vector for X-axis
    std::vector<double> tofs;
    std::vector<size_t> itofs;
    std::vector<double>::const_iterator vit;
    vit = std::lower_bound(m_dataWS->readX(m_wsIndex).begin(), m_dataWS->readX(m_wsIndex).end(), leftbound);
    size_t istart = size_t(vit-m_dataWS->readX(m_wsIndex).begin());
    vit = std::lower_bound(m_dataWS->readX(m_wsIndex).begin(), m_dataWS->readX(m_wsIndex).end(), rightbound);
    size_t iend = size_t(vit-m_dataWS->readX(m_wsIndex).begin());
    for (size_t i = istart; i < iend; ++i)
    {
      itofs.push_back(i);
      tofs.push_back(m_dataWS->readX(m_wsIndex)[i]);
    }

    // Check validity
    if (tofs.size() == 0)
    {
      g_log.warning() << "[CalculateSinglePeak] Domain Size (number of TOF points) = 0" << endl;
      return;
    }

    // 3. Generate a composite function
    API::CompositeFunction tempfunc;
    API::CompositeFunction_sptr compfunction = boost::make_shared<API::CompositeFunction>
        (tempfunc);
    for (size_t i = 0; i < peakindexes.size(); ++i)
    {
      BackToBackExponential_sptr peak = m_peaks[peakindexes[i]].second.second;
      compfunction->addFunction(peak);
    }
    compfunction->addFunction(background);

    // 4.  Calculate the composite function
    API::FunctionDomain1DVector domain(tofs);
    API::FunctionValues values(domain);

    g_log.information() << "DBx419 [CalcualteSinglePeak]  Domain Size = " << domain.size() << endl;

    compfunction->function(domain, values);

    // 5.   Book keep the result
    for (size_t i = istart; i < iend; ++i)
    {
      m_peakData[i] = values[i-istart];
    }

    return;
  }

  //=========================  Operation on Background  ========================
  //----------------------------------------------------------------------------
  // TODO: DISABLE metod estimateBackground() and replace it with the external
  //       method estimateBackgroundCoarse()
  //       Notice any broken in the code.
#if 0
  /** Estimate background for a pattern
    * Assumption: the peak must be in the data range completely
    * Algorithm: use two end data points for a linear background
    * Output: dataws spectrum 3 (workspace index 2)
    */
  void FitPowderDiffPeaks2::estimateBackground(DataObjects::Workspace2D_sptr dataws)
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
#endif

  //----------------------------------------------------------------------------
  /** Subtract background
    * This is an operation within the specially defined data workspace for peak fitting
    * Output: spectrum 2 (workspace 1) for data with background subtracted
    */
  void FitPowderDiffPeaks2::subtractBackground(Workspace2D_sptr dataws)
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

  //===========================  Process Fit Result  ===========================
  //----------------------------------------------------------------------------
  /** Parse fit result
    */
  std::string FitPowderDiffPeaks2::parseFitResult(API::IAlgorithm_sptr fitalg, double& chi2)
  {
    stringstream rss;

    chi2 = fitalg->getProperty("OutputChi2overDoF");
    string fitstatus = fitalg->getProperty("OutputStatus");

    rss << "  [Algorithm Fit]:  Chi^2 = " << chi2
        << "; Fit Status = " << fitstatus;

    return rss.str();
  }


  //----------------------------------------------------------------------------
  /** Parse parameter workspace returned from Fit()
    */
  std::string FitPowderDiffPeaks2::parseFitParameterWorkspace(API::ITableWorkspace_sptr paramws)
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

  //===========================  Process Input/Output  =========================
  //----------------------------------------------------------------------------
  /** Import TableWorkspace containing the parameters for fitting
   * the diffrotometer geometry parameters
   */
  void FitPowderDiffPeaks2::importInstrumentParameterFromTable(DataObjects::TableWorkspace_sptr parameterWS)
  {
    // 1. Check column orders
    std::vector<std::string> colnames = parameterWS->getColumnNames();
    if (colnames.size() < 2)
    {
      stringstream errss;
      errss << "Input parameter table workspace does not have enough number of columns. "
            << " Number of columns = " << colnames.size() << " >= 2 as required. ";
      g_log.error(errss.str());
      throw std::runtime_error(errss.str());
    }

    if (colnames[0].compare("Name") != 0 || colnames[1].compare("Value") != 0)
    {
      stringstream errss;
      errss << "Input parameter table workspace does not have the columns in order as  "
            << "Name, Value and etc. ";
      g_log.error(errss.str());
      throw std::runtime_error(errss.str());
    }

    // 2. Import data to maps
    std::string parname;
    double value;
    size_t numrows = parameterWS->rowCount();
    m_instrumentParmaeters.clear();

    for (size_t ir = 0; ir < numrows; ++ir)
    {
      API::TableRow trow = parameterWS->getRow(ir);
      trow >> parname >> value;
      m_instrumentParmaeters.insert(std::make_pair(parname, value));
      cout << "[DBx211] Import parameter " << parname << ": " << value << endl;
    }

    return;
  }

  /** Import Bragg peak table workspace
    */
  void FitPowderDiffPeaks2::parseBraggPeakTable(TableWorkspace_sptr peakws, vector<map<string, double> >& parammaps,
                                               vector<map<string, int> >& hklmaps)
  {
    // 1. Get columns' types and names
    vector<string> paramnames = peakws->getColumnNames();
    size_t numcols = paramnames.size();
    vector<string> coltypes(numcols);
    for (size_t i= 0; i < numcols; ++i)
    {
      Column_sptr col =peakws->getColumn(i);
      string coltype = col->type();
      coltypes[i] = coltype;
    }

    // 2. Parse table rows
    size_t numrows = peakws->rowCount();
    for (size_t irow = 0; irow < numrows; ++irow)
    {
      // 1. Create map
      map<string, int> intmap;
      map<string, double> doublemap;

      // 2. Parse
      for (size_t icol = 0; icol < numcols; ++icol)
      {
        string coltype = coltypes[icol];
        string colname = paramnames[icol];

        if (coltype.compare("int") == 0)
        {
          // Integer
          int temp = peakws->cell<int>(irow, icol);
          intmap.insert(make_pair(colname, temp));
        }
        else if (coltype.compare("double") == 0)
        {
          // Double
          double temp = peakws->cell<double>(irow, icol);
          doublemap.insert(make_pair(colname, temp));
        }

      } // ENDFOR Column

     parammaps.push_back(doublemap);
     hklmaps.push_back(intmap);

    } // ENDFOR Row

    g_log.information() << "Import " << hklmaps.size() << " entries from Bragg peak TableWorkspace "
                        << peakws->name() << endl;

    return;
  }

  //----------------------------------------------------------------------------
  /** Create a Workspace2D for fitted peaks (pattern) and also the workspace for Zscores!
    */
  Workspace2D_sptr FitPowderDiffPeaks2::genOutputFittedPatternWorkspace(std::vector<double> pattern, int workspaceindex)
  {
    // 1. Init
    const MantidVec& X = m_dataWS->readX(workspaceindex);
    const MantidVec& Y = m_dataWS->readY(workspaceindex);

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
          WorkspaceFactory::Instance().create("Workspace2D", 5, pattern.size(), pattern.size()));

    // 3. Set up
    for (size_t iw = 0; iw < 5; ++iw)
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

  //----------------------------------------------------------------------------
  /** Generate a TableWorkspace for peaks with good fitting.
    * Table has column as H, K, L, d_h, X0, A(lpha), B(eta), S(igma), Chi2
    * Each row is a peak
    */
  pair<TableWorkspace_sptr, TableWorkspace_sptr> FitPowderDiffPeaks2::genPeakParametersWorkspace(
      vector<size_t> goodfitpeaks, std::vector<double> goodfitchi2s)
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
#if 0
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
#endif


    // 4. Add the peak data to each row
    vector<double> vectofh, vecalpha, vecbeta, vecsigma;
    for (size_t i = 0; i < goodfitpeaks.size(); ++i)
    {
      throw runtime_error("The iterator here should be written to m_peaks'.");
      CurveFitting::BackToBackExponential_sptr peak = m_peaks[0].second.second;
#if 0
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

      vectofh.push_back(p_x);
      vecalpha.push_back(p_a);
      vecbeta.push_back(p_b);
      vecsigma.push_back(p_s);

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
#endif

    } // FOREACH Peak

    // 5. Debug output
    //    FIXME Remove this part after unit test is completed and passed.
    ofstream ofile;
    ofile.open("fittedparameters.txt");
    ofile << outbuf.str();
    ofile.close();

    // 6. Start to calculate Z-scores
    vector<double> zcentres = Kernel::getZscore(vectofh);
    vector<double> zalphas = Kernel::getZscore(vecalpha);
    vector<double> zbetas = Kernel::getZscore(vecbeta);
    vector<double> zsigma = Kernel::getZscore(vecsigma);

    // 7. Build table workspace for Z scores
    TableWorkspace* ztablewsptr = new TableWorkspace();
    TableWorkspace_sptr ztablews = TableWorkspace_sptr(ztablewsptr);

    ztablews->addColumn("int", "H");
    ztablews->addColumn("int", "K");
    ztablews->addColumn("int", "L");

    ztablews->addColumn("double", "d_h");
    ztablews->addColumn("double", "Z_TOF_h");
    ztablews->addColumn("double", "Z_Alpha");
    ztablews->addColumn("double", "Z_Beta");
    ztablews->addColumn("double", "Z_Sigma");

    // 8. Set values
    for (size_t i = 0; i < goodfitpeaks.size(); ++i)
    {
      TableRow newrow = ztablews->appendRow();

#if 0
      // i. H, K, L, d_h
      double dh = dhkls[i].first;
      std::vector<int> hkl = dhkls[i].second;

      newrow << hkl[0] << hkl[1] << hkl[2] << dh;

      // ii. Z scores
      double p_x = zcentres[i];
      double p_a = zalphas[i];
      double p_b = zbetas[i];
      double p_s = zsigma[i];

      newrow << p_x << p_a << p_b << p_s;
#else
      throw runtime_error("Re-write this part!");
#endif
    }

    return make_pair(tablews, ztablews);
  }

 //----------------------------------------------------------------------------------------------
  /** Genearte peaks from input workspace;
    * Each peak within requirement will put into both (1) m_peaks and (2) m_peaksmap
    */
  void FitPowderDiffPeaks2::genPeaksFromTable(TableWorkspace_sptr peakparamws)
  {
    // 1. Check and clear input and output
    if (!peakparamws)
    {
      stringstream errss;
      errss << "Input tableworkspace for peak parameters is invalid!";
      g_log.error(errss.str());
      throw std::invalid_argument(errss.str());
    }

    m_peaks.clear();

    // Give name to peak parameters
    BackToBackExponential tempeak;
    mPeakParameterNames = tempeak.getParameterNames();
    mPeakParameterNames.push_back("S2");

    // 2. Parse TableWorkspace
    vector<map<std::string, double> > peakparametermaps;
    vector<map<std::string, int> > peakhkls;
    this->parseBraggPeakTable(peakparamws, peakparametermaps, peakhkls);

    // 3. Create a map to convert the Bragg peak Table paramter name to BackToBackExp
    map<string, string> bk2bk2braggmap;
    bk2bk2braggmap.insert(make_pair("A", "Alpha"));
    bk2bk2braggmap.insert(make_pair("B", "Beta"));
    bk2bk2braggmap.insert(make_pair("X0", "TOF_h"));
    bk2bk2braggmap.insert(make_pair("I", "Height"));
    bk2bk2braggmap.insert(make_pair("S","Sigma"));
    bk2bk2braggmap.insert(make_pair("S2", "Sigma2"));

    // 4. Generate Peaks       
    size_t numbadrows = 0;
    size_t numrows = peakparamws->rowCount();
    for (size_t ir = 0; ir < numrows; ++ir)
    {
      double d_h;
      vector<int> hkl;
      bool good;
      BackToBackExponential_sptr newpeak = genPeak(peakhkls[ir], peakparametermaps[ir], bk2bk2braggmap, good, hkl, d_h);

      if (good)
      {
        m_peaks.push_back(make_pair(d_h, make_pair(hkl, newpeak)));
      }
      else
      {
        ++ numbadrows;
      }
    } // ENDFOR Each potential peak

    // 5. Sort and delete peaks out of range
    sort(m_peaks.begin(), m_peaks.end());

    // a) Remove all peaks outside of tof_min and tof_max
    double tofmin = m_dataWS->readX(m_wsIndex)[0];
    double tofmax = m_dataWS->readX(m_wsIndex).back();

    vector<pair<double, pair<vector<int>, BackToBackExponential_sptr> > >::iterator deliter;
    for (deliter = m_peaks.begin(); deliter != m_peaks.end(); ++deliter)
    {
      double d_h = deliter->first;
      vector<int> hkl = deliter->second.first;
      cout << "[DBx441] Check Peak (" << hkl[0] << ", " << hkl[1] << ", " << hkl[2] << ") @ d = " << d_h << endl;

      BackToBackExponential_sptr peak = deliter->second.second;
      double tofh = peak->getParameter("X0");
      if (tofh < tofmin || tofh > tofmax)
      {
        deliter = m_peaks.erase(deliter);
        cout << "[DBx453] \t\tDelete Peak (" << hkl[0] << ", " << hkl[1] << ", " << hkl[2] << ") @ d = " << d_h
             << ", TOF = " << tofh << endl;
        if (deliter == m_peaks.end())
        {
          break;
        }
      }
    }

    // b) Remove peaks lower than minimum
    if (m_minimumHKL.size() == 3)
    {
      // Only keep peaks from and above minimum HKL
      for (deliter = m_peaks.begin(); deliter != m_peaks.end(); ++deliter)
      {
        vector<int> hkl = deliter->second.first;
        if (hkl == m_minimumHKL)
        {
          break;
        }
      }
      if (deliter != m_peaks.end())
      {
        // Find the real minum
        int indminhkl = static_cast<int>(deliter-m_peaks.begin());
        int ind1stpeak = indminhkl - m_numPeaksLowerToMin;
        if (ind1stpeak > 0)
        {
          deliter = m_peaks.begin() + ind1stpeak;
          m_peaks.erase(m_peaks.begin(), deliter);
        }
      }
      else
      {
        // Miminum HKL peak does not exist
        vector<int> hkl = m_minimumHKL;
        g_log.warning() << "Minimum peak " << hkl[0] << ", " << hkl[1] << ", " << hkl[2]
                        << " does not exit. " << endl;
      }
    }

    // 6. Keep some input information
    for (size_t i = 0; i < m_peaks.size(); ++i)
    {
      double pheight = m_peaks[i].second.second->height();
      m_inputPeakCentres.push_back(pheight);
    }

    stringstream dbout;
    for (deliter = m_peaks.begin(); deliter != m_peaks.end(); ++ deliter)
    {
      vector<int> hkl = deliter->second.first;
      double d_h = deliter->first;
      double tof_h = deliter->second.second->centre();
      dbout << "Peak (" << hkl[0] << ", " << hkl[1] << ", " << hkl[2] << ") @ d = " << d_h
            << ", TOF = " << tof_h << endl;
    }
    cout << "[DBx531] Peaks To Fit:  Number of peaks = " << m_peaks.size() << endl << dbout.str();

    return;
  }

  //-----------------------------------------------------------------------------------------
  /** Generate a peak
    */
  BackToBackExponential_sptr FitPowderDiffPeaks2::genPeak(map<string, int> hklmap, map<string, double> parammap,
                                                          map<string, string> bk2bk2braggmap, bool &good,
                                                          vector<int>& hkl, double& d_h)
  {
    // 1. Generate peak whatever
    CurveFitting::BackToBackExponential newpeak;
    newpeak.initialize();
    BackToBackExponential_sptr newpeakptr = boost::make_shared<BackToBackExponential>(newpeak);

    // 2. Get basic information: HKL
    good = getHKLFromMap(hklmap, hkl);
    if (!good)
    {
      // Ignore and return
      return newpeakptr;
    }
    else
    {
      cout << "[DBx426] Generate Peak (" << hkl[0] << ", " << hkl[1] << ", " << hkl[2] << ")... ..."
           << endl;
    }

    // 3. Calcualte d-spacing
    double latticesize = getParameter("LatticeConstant");
    if (latticesize == EMPTY_DBL())
    {
      throw runtime_error("Input instrument table workspace lacks LatticeConstant. "
                          "Unable to complete processing.");
    }
    d_h = calCubicDSpace(latticesize, hkl[0], hkl[1], hkl[2]);
    if ( (d_h != d_h) || (d_h < -DBL_MAX) || (d_h > DBL_MAX) )
    {
      stringstream warnss;
      warnss << "Peak with Miller Index = " << hkl[0] << ", " << hkl[1] << ", " << hkl[2]
             << " has unphysical d-spacing value = " << d_h;
      g_log.warning(warnss.str());
      good = false;
      return newpeakptr;
    }

    // 4. Set the peak parameters from 2 methods
    if (m_genPeakStartingValue == HKLCALCULATION)
    {
      // a) Use Bragg peak table's (HKL) and calculate the peak parameters
      double alph0 = getParameter("Alph0");
      double alph1 = getParameter("Alph1");
      double alph0t = getParameter("Alph0t");
      double alph1t = getParameter("Alph1t");
      double beta0 = getParameter("Beta0");
      double beta1 = getParameter("Beta1");
      double beta0t = getParameter("Beta0t");
      double beta1t = getParameter("Beta1t");
      double sig0 = getParameter("Sig0");
      double sig1 = getParameter("Sig1");
      double sig2 = getParameter("Sig2");
      double tcross = getParameter("Tcross");
      double width = getParameter("Width");
      double dtt1 = getParameter("Dtt1");
      double dtt1t = getParameter("Dtt1t");
      double dtt2t = getParameter("Dtt2t");
      double zero = getParameter("Zero");
      double zerot = getParameter("Zerot");

      // b) Check validity and make choice
      if (tcross == EMPTY_DBL() || width == EMPTY_DBL() || dtt1 == EMPTY_DBL() ||
          dtt1t == EMPTY_DBL() || dtt2t == EMPTY_DBL() || zero == EMPTY_DBL() || zerot == EMPTY_DBL())
      {
        stringstream errss;
        errss << "In input InstrumentParameterTable, one of the following is not given.  Unable to process. " << endl;
        errss << "Tcross = " << tcross << "; Width = " << width << ", Dtt1 = " << dtt1 << ", Dtt1t = " << dtt1t << endl;
        errss << "Dtt2t = " << dtt2t << ", Zero = " << zero << ", Zerot = " << zerot;

        g_log.error(errss.str());
        throw runtime_error(errss.str());
      }

      bool caltofonly = false;
      if (alph0 == EMPTY_DBL() || alph1 == EMPTY_DBL() || alph0t == EMPTY_DBL() || alph1t == EMPTY_DBL() ||
          beta0 == EMPTY_DBL() || beta1 == EMPTY_DBL() || beta0t == EMPTY_DBL() || beta1t == EMPTY_DBL() ||
          sig0 == EMPTY_DBL() || sig1 == EMPTY_DBL() || sig2 == EMPTY_DBL())
      {
        cout << "[DBx343] Incomplete in instrument-peak profile parameters. Use (HKL) only!" << endl;
        caltofonly = true;
      }

      if (caltofonly)
      {
        // c) Calcualte d->TOF only
        double tof_h = calThermalNeutronTOF(d_h, dtt1, dtt1t, dtt2t, zero, zerot, width, tcross);
        newpeakptr->setCentre(tof_h);
      }
      else
      {
        // d) Calculate a lot of peak parameters
        ThermalNeutronBk2BkExpConvPV tnb2bfunc;
        tnb2bfunc.initialize();
        tnb2bfunc.setMillerIndex(hkl[0], hkl[1], hkl[2]);
        std::map<std::string, double>::iterator miter;
        for (miter = m_instrumentParmaeters.begin(); miter != m_instrumentParmaeters.end(); ++miter)
        {
          string parname = miter->first;
          double parvalue = miter->second;
          tnb2bfunc.setParameter(parname, parvalue);
        }
        double tof_h, alpha, beta, sigma2, eta, H, gamma, N;
        tnb2bfunc.calculateParameters(d_h, tof_h, eta, alpha, beta, H, sigma2, gamma, N, false);

        newpeakptr->setParameter("A", alpha);
        newpeakptr->setParameter("B", beta);
        newpeakptr->setParameter("S", sqrt(sigma2));
        newpeakptr->setParameter("X0", tof_h);
      }
    }
    else if (m_genPeakStartingValue == FROMBRAGGTABLE)
    {
      // e) Import from input table workspace
      vector<string>::iterator nameiter;
      for (nameiter=mPeakParameterNames.begin(); nameiter!=mPeakParameterNames.end(); ++nameiter)
      {
        // BackToBackExponential parameter name
        string b2bexpname = *nameiter;
        // Map to instrument parameter
        map<string, string>::iterator refiter;
        refiter = bk2bk2braggmap.find(b2bexpname);
        if (refiter == bk2bk2braggmap.end())
          throw runtime_error("Programming error!");
        string instparname = refiter->second;
        // Search in Bragg peak table
        map<string, double>::iterator miter;
        miter = parammap.find(instparname);
        if (miter != parammap.end())
        {
          // Parameter exist in input
          double parvalue = miter->second;
          if (b2bexpname.compare("S2") == 0)
          {
            newpeak.setParameter("S", sqrt(parvalue));
          }
          else
          {
            newpeak.setParameter(b2bexpname, parvalue);
          }
        }
      } // FOR PEAK TABLE CELL
    }

    good = true;

    return newpeakptr;
  }

  //===========================  Auxiliary Functions ===========================
  //----------------------------------------------------------------------------
  /** Get (HKL) from a map
    * Return false if the information is incomplete
    */
  bool FitPowderDiffPeaks2::getHKLFromMap(map<string, int> intmap, vector<int>& hkl)
  {
    vector<string> strhkl(3);
    strhkl[0] = "H"; strhkl[1] = "K"; strhkl[2] = "L";

    hkl.clear();

    map<string, int>::iterator miter;
    for (size_t i = 0; i < 3; ++i)
    {
      string parname = strhkl[i];
      miter = intmap.find(parname);
      if (miter == intmap.end())
        return false;

      hkl.push_back(miter->second);
    }

    return true;
  }

  //----------------------------------------------------------------------------
  /** Crop data workspace: the original workspace will not be affected
      */
  void FitPowderDiffPeaks2::cropWorkspace(double tofmin, double tofmax)
  {
    API::IAlgorithm_sptr cropalg = this->createSubAlgorithm("CropWorkspace", -1, -1, true);
    cropalg->initialize();

    cropalg->setProperty("InputWorkspace", m_dataWS);
    cropalg->setPropertyValue("OutputWorkspace", "MyData");
    cropalg->setProperty("XMin", tofmin);
    cropalg->setProperty("XMax", tofmax);

    bool cropstatus = cropalg->execute();
	  
	std::stringstream errmsg;
    if (!cropstatus)
    {
      errmsg << "DBx309 Cropping workspace unsuccessful.  Fatal Error. Quit!";
      g_log.error() << errmsg.str() << std::endl;
      throw std::runtime_error(errmsg.str());
    }

    m_dataWS = cropalg->getProperty("OutputWorkspace");
    if (!m_dataWS)
    {
	  errmsg << "Unable to retrieve a Workspace2D object from subalgorithm Crop.";
		g_log.error(errmsg.str());
		throw std::runtime_error(errmsg.str());
    }
    else
    {
      cout << "[DBx211] Cropped Workspace Range: " << m_dataWS->readX(m_wsIndex)[0] << ", "
           << m_dataWS->readX(m_wsIndex).back() << endl;
    }

    return;
  }

  //----------------------------------------------------------------------------------------------
  /** Get parameter value from m_instrumentParameters
    * Exception: throw runtime error if there is no such parameter
    */
  double FitPowderDiffPeaks2::getParameter(string parname)
  {
    map<string, double>::iterator mapiter;
    mapiter = m_instrumentParmaeters.find(parname);

    if (mapiter == m_instrumentParmaeters.end())
    {
      stringstream errss;
      errss << "Instrument parameter map (having " << m_instrumentParmaeters.size() << " entries) "
            << "does not have parameter " << parname << ". ";
      g_log.warning(errss.str());
      return (EMPTY_DBL());
    }

    return mapiter->second;
  }


  //-------- External Functions -------------------------------------------------------------------------
  /** Build a partial workspace from original data workspace
    */
  Workspace2D_sptr buildPartialWorkspace(API::MatrixWorkspace_sptr sourcews, size_t workspaceindex,
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
      throw std::invalid_argument(errmsg.str());
    }
    if (leftbound >= X.back() || rightbound <= X[0])
    {
      throw std::invalid_argument("Boundary is out side of the input data set. ");
    }

    // 2. Determine the size of the "partial" workspace
    int ileft = static_cast<int>(std::lower_bound(X.begin(), X.end(), leftbound) - X.begin());
    if (ileft > 0)
      -- ileft;
    int iright = static_cast<int>(std::lower_bound(X.begin(), X.end(), rightbound) - X.begin());
    if (iright >= static_cast<int>(X.size()))
      iright = static_cast<int>(X.size()-1);

    size_t wssize = static_cast<size_t>(iright-ileft+1);

    // 3. Build the partial workspace
    size_t nspec = 6;
    Workspace2D_sptr partws = boost::dynamic_pointer_cast<Workspace2D>
        (API::WorkspaceFactory::Instance().create("Workspace2D", nspec, wssize, wssize));

    // 4. Put data there
    // TODO: Try to use vector copier
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

    // 5. Debug output
    stringstream wss;
    for (size_t i = 0; i < partws->readX(0).size(); ++i)
      wss << setw(10) << setprecision(6) << partws->readX(0)[i] << setw(10) << setprecision(6) << partws->readY(0)[i] << endl;
    cout << "[DBx109] Partial Workspace: " << endl << wss.str() << "..................................." << endl;

    return partws;
  }

  //-----------------------------------------------------------------------------------------------------------
  /** Estimate background for a pattern in a coarse mode
    * Assumption: the peak must be in the data range completely
    * Algorithm: use two end data points for a linear background
    * Output: dataws spectrum 3 (workspace index 2)
    */
  void estimateBackgroundCoarse(DataObjects::Workspace2D_sptr dataws, BackgroundFunction_sptr background,
                                size_t wsindexraw, size_t wsindexbkgd, size_t wsindexpeak)
  {
    // 1. Get prepared
    if (dataws->getNumberHistograms() < 3)
    {
      stringstream errss;
      errss << "Function estimateBackgroundCoase() requires input Workspace2D has at least 3 spectra."
            << "Present input has " << dataws->getNumberHistograms() << " spectra.";
      throw runtime_error(errss.str());
    }
    const MantidVec& X = dataws->readX(wsindexraw);
    const MantidVec& Y = dataws->readY(wsindexraw);

    // TODO: This is a magic number!
    size_t numsamplepts = 2;
    if (X.size() <= 10)
    {
      // Make it at minimum to estimate background
      numsamplepts = 1;
    }

    // 2. Average the first and last three data points
    double y0 = 0;
    double x0 = 0;

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

    background->setParameter("A0", b0);
    background->setParameter("A1", b1);

    // 4. Calcualte background
    FunctionDomain1DVector domain(X);
    FunctionValues values(domain);
    background->function(domain, values);

    MantidVec& bY = dataws->dataY(wsindexbkgd);
    MantidVec& pY = dataws->dataY(wsindexpeak);
    for (size_t i = 0; i < bY.size(); ++i)
    {
      bY[i] = values[i];
      pY[i] = Y[i] - bY[i];
    }

    return;
  }

  //-----------------------------------------------------------------------------------------------------------
  /** Estimate peak parameters;
    * Prerequisit:
    * (1) Background removed
    * (2) Peak is inside
    * Algorithm: From the top.  Get the maximum value. Calculate the half maximum value.  Find the range of X
    */
  bool estimatePeakParameters(Workspace2D_sptr dataws, size_t wsindex, double& centre, double& height, double& fwhm,
                              string& errmsg)
  {
    // 1. Get the value of the Max Height
    const MantidVec& X = dataws->readX(wsindex);
    const MantidVec& Y = dataws->readY(wsindex);

    // 2. The highest peak should be the centre
    size_t icentre = findMaxValue(Y);
    centre = X[icentre];
    height = Y[icentre];

    if (icentre <= 1 || icentre > X.size()-2)
    {
      stringstream errss;
      errss << "Peak center = " << centre << " is at the edge of the input workspace [" << X[0]
            << ", " << X.back()
            << ". It is unable to proceed the estimate of FWHM.  Quit with error!.";
      errmsg = errss.str();
      return false;
    }
    if (height <= 0)
    {
      stringstream errss;
      errss << "Max height = " << height << " in input workspace [" << X[0] << ", " <<  X.back()
            << " is negative.  Fatal error is design of the algorithm.";
      errmsg = errss.str();
      return false;
    }

    // 3. Calculate FWHM
    double halfMax = height*0.5;

    // a) Deal with left side
    bool continueloop = true;
    size_t index = icentre-1;
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
        stringstream errss;
        errss << "The peak is not complete (left side) in the given data range.";
        errmsg = errss.str();
        return false;
      }
      else
      {
        // Contineu to locate
        --index;
      }
    }
    double x0 = X[index];
    double xf = X[index+1];
    double y0 = Y[index];
    double yf = Y[index+1];

    // Formular for linear iterpolation: X = [(xf-x0)*Y - (xf*y0-x0*yf)]/(yf-y0)
    double xl = linearInterpolateX(x0, xf, y0, yf, halfMax);

    double lefthalffwhm = centre-xl;

    // 3. Deal with right side
    continueloop = true;
    index = icentre+1;
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
        stringstream errss;
        errss << "The peak is not complete (right side) in the given data range.";
        errmsg = errss.str();
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

    double righthalffwhm = xr-centre;

    // Final
    fwhm = lefthalffwhm + righthalffwhm;

    return true;
  }

  //-----------------------------------------------------------------------------------------------------------
  /** Find maximum value
    */
  size_t findMaxValue(const MantidVec Y)
  {
    size_t imax = 0;
    double maxy = Y[imax];

    for (size_t i = 0; i < Y.size(); ++i)
    {
      if (Y[i] > maxy)
      {
        maxy = Y[i];
        imax = i;
      }
    }

    return imax;
  }

  //-----------------------------------------------------------------------------------------------------------
  /** Find maximum value
    */
  size_t findMaxValue(MatrixWorkspace_sptr dataws, size_t wsindex, double leftbound, double rightbound)
  {
    const MantidVec& X = dataws->readX(wsindex);
    const MantidVec& Y = dataws->readY(wsindex);

    // 1. Determine xmin, xmax range
    std::vector<double>::const_iterator viter;

    viter = std::lower_bound(X.begin(), X.end(), leftbound);
    size_t ixmin = size_t(viter-X.begin());
    if (ixmin != 0)
      -- ixmin;
    viter = std::lower_bound(X.begin(), X.end(), rightbound);
    size_t ixmax = size_t(viter-X.begin());

    // 2. Search imax
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

    return imax;
  }


} // namespace CurveFitting
} // namespace Mantid
