/*WIKI*
 This algorithm searches the specified spectra in a workspace for peaks, returning a list of the found and successfully fitted peaks. The search algorithm is described in full in reference [1]. In summary: the second difference of each spectrum is computed and smoothed. This smoothed data is then searched for patterns consistent with the presence of a peak. The list of candidate peaks found is passed to a fitting routine and those that are successfully fitted are kept and returned in the output workspace (and logged at information level).

 The output [[TableWorkspace]] contains the following columns, which reflect the fact that the peak has been fitted to a Gaussian atop a linear background: spectrum, centre, width, height, backgroundintercept & backgroundslope.

 ==== Subalgorithms used ====
 FindPeaks uses the [[SmoothData]] algorithm to, well, smooth the data - a necessary step to identify peaks in statistically fluctuating data. The [[Fit]] algorithm is used to fit candidate peaks.

 ==== Treating weak peaks vs. high background ====
 FindPeaks uses a more complicated approach to fit peaks if '''HighBackground''' is flagged. In this case, FindPeak will fit the background first, and then do a Gaussian fit the peak with the fitted background removed.  This procedure will be repeated for a couple of times with different guessed peak widths.  And the parameters of the best result is selected.  The last step is to fit the peak with a combo function including background and Gaussian by using the previously recorded best background and peak parameters as the starting values.

 ==== Criteria To Validate Peaks Found ====
 FindPeaks finds peaks by fitting a Guassian with background to a certain range in the input histogram.  [[Fit]] may not give a correct result even if chi^2 is used as criteria alone.  Thus some other criteria are provided as options to validate the result
 1. Peak position.  If peak positions are given, and trustful, then the fitted peak position must be within a short distance to the give one.
 2. Peak height.  In the certain number of trial, peak height can be used to select the best fit among various starting sigma values.

 ==== Fit Window ====
 If FitWindows is defined, then a peak's range to fit (i.e., x-min and x-max) is confined by this window.

 If FitWindows is defined, starting peak centres are NOT user's input, but found by highest value within peak window. (Is this correct???)

 ==== References ====
 # M.A.Mariscotti, ''A method for automatic identification of peaks in the presence of background and its application to spectrum analysis'', NIM '''50''' (1967) 309.

 *WIKI*/
//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAlgorithms/FindPeaks.h"
#include "MantidAPI/TableRow.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/VectorHelper.h"
#include "MantidAPI/CompositeFunction.h"
#include "MantidAPI/WorkspaceValidators.h"
#include "MantidAPI/FunctionFactory.h"
#include "MantidAPI/IPeakFunction.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidAPI/ConstraintFactory.h"
#include <boost/algorithm/string.hpp>
#include <iostream>
#include <numeric>
#include "MantidKernel/BoundedValidator.h"
#include "MantidKernel/ListValidator.h"

#include <fstream>

#define TEST 1

using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace Mantid::DataObjects;

// const double MINHEIGHT = 2.00000001;

namespace Mantid
{
namespace Algorithms
{
  //----------------------------------------------------------------------------------------------
  /** Constructor
    */
  PeakFittingRecord::PeakFittingRecord()
  {
    m_goodness = DBL_MAX;
  }

  //----------------------------------------------------------------------------------------------
  /** Destructor
    */
  PeakFittingRecord::~PeakFittingRecord()
  {

  }

  //----------------------------------------------------------------------------------------------
  /** Set
    */
  void PeakFittingRecord::set(double chi2, const std::map<std::string, double>& peakparammap,
                              const std::map<std::string, double>&  bkgdparammap)
  {
    m_goodness = chi2;
    m_peakParameterMap = peakparammap;
    m_bkgdParameterMap = bkgdparammap;

    return;
  }

  //----------------------------------------------------------------------------------------------
  /** Lower bound of a given value
    */
  size_t getLowerBound(const MantidVec& X, size_t xi, size_t xf, double value)
  {
    // FIXME Consider to use std::lower_bound()

    // 0. Check
    if (xi > xf)
      throw std::invalid_argument("getLowerBound(): xi > xf!");
    if (xf >= X.size())
      throw std::invalid_argument("getLowerBound(): xf is outside of X[].");

    // 1. Check
    if (value <= X[xi])
    {
      // at or outside of lower bound
      return xi;
    }
    else if (value >= X[xf])
    {
      // at or outside of upper bound
      return xf;
    }

    bool continuesearch = true;

    size_t ia = xi;
    size_t ib = xf;
    size_t isearch = 0;

    while (continuesearch)
    {
      // a) Found?
      if ((ia == ib) || (ib - ia) == 1)
      {
        isearch = ia;
        continuesearch = false;
      }
      else
      {
        // b) Split to half
        size_t inew = (ia + ib) / 2;
        if (value < X[inew])
        {
          // Search lower half
          ib = inew;
        }
        else if (value > X[inew])
        {
          // Search upper half
          ia = inew;
        }
        else
        {
          // Just find it
          isearch = inew;
          continuesearch = false;
        }
      }
    }

    return isearch;
  }

  // Register the algorithm into the AlgorithmFactory
  DECLARE_ALGORITHM(FindPeaks)

  //----------------------------------------------------------------------------------------------
  /** Constructor
    */
  FindPeaks::FindPeaks() : API::Algorithm(), m_progress(NULL)
  {
    m_minimizer = "Levenberg-Marquardt";
  }

  //----------------------------------------------------------------------------------------------
  /** Sets documentation strings for this algorithm
      */
  void FindPeaks::initDocs()
  {
    this->setWikiSummary("Searches for peaks in a dataset.");
    this->setOptionalMessage("Searches for peaks in a dataset.");
  }

  //----------------------------------------------------------------------------------------------
  /** Initialize and declare properties.
     */
  void FindPeaks::init()
  {
    declareProperty(new WorkspaceProperty<>("InputWorkspace", "", Direction::Input),
                    "Name of the workspace to search");

    auto min = boost::make_shared<BoundedValidator<int> >();
    min->setLower(1);
    // The estimated width of a peak in terms of number of channels
    declareProperty("FWHM", 7, min,
                    "Estimated number of points covered by the fwhm of a peak (default 7)");

    // The tolerance allowed in meeting the conditions
    declareProperty("Tolerance", 4, min,
                    "A measure of the strictness desired in meeting the condition on peak candidates,\n"
                    "Mariscotti recommends 2 (default 4)");

    declareProperty(new ArrayProperty<double>("PeakPositions"),
                    "Optional: enter a comma-separated list of the expected X-position of the centre of the peaks. Only peaks near these positions will be fitted.");

    declareProperty(new ArrayProperty<double>("FitWindows"),
                    "Optional: enter a comma-separated list of the expected X-position of windows to fit. The number of values must be exactly double the number of specified peaks.");

    std::vector<std::string> peakNames = FunctionFactory::Instance().getFunctionNames<IPeakFunction>();
    declareProperty("PeakFunction", "Gaussian", boost::make_shared<StringListValidator>(peakNames));

    std::vector<std::string> bkgdtypes;
    bkgdtypes.push_back("Flat");
    bkgdtypes.push_back("Linear");
    bkgdtypes.push_back("Quadratic");
    declareProperty("BackgroundType", "Linear", boost::make_shared<StringListValidator>(bkgdtypes),
                    "Type of Background.");

    auto mustBeNonNegative = boost::make_shared<BoundedValidator<int> >();
    mustBeNonNegative->setLower(0);
    declareProperty("WorkspaceIndex", EMPTY_INT(), mustBeNonNegative,
                    "If set, only this spectrum will be searched for peaks (otherwise all are)");

    declareProperty("HighBackground", true, "Relatively weak peak in high background");

    auto mustBePositive = boost::make_shared<BoundedValidator<int> >();
    mustBePositive->setLower(1);
    declareProperty("MinGuessedPeakWidth", 2, mustBePositive,
                    "Minimum guessed peak width for fit. It is in unit of number of pixels.");

    declareProperty("MaxGuessedPeakWidth", 10, mustBePositive,
                    "Maximum guessed peak width for fit. It is in unit of number of pixels.");

    declareProperty("GuessedPeakWidthStep", 2, mustBePositive,
                    "Step of guessed peak width. It is in unit of number of pixels.");

    auto mustBePositiveDBL = boost::make_shared<BoundedValidator<double> >();
    declareProperty("PeakPositionTolerance", EMPTY_DBL(), mustBePositiveDBL,
                    "Tolerance on the found peaks' positions against the input peak positions.  Non-positive value indicates that this option is turned off.");

    declareProperty("PeakHeightTolerance", EMPTY_DBL(),
                    "Tolerance of the ratio on the found peak's height against the local maximum.  Non-positive value turns this option off. ");

    // The found peaks in a table
    declareProperty(new WorkspaceProperty<API::ITableWorkspace>("PeaksList", "", Direction::Output),
                    "The name of the TableWorkspace in which to store the list of peaks found");

    declareProperty("RawPeakParameters", false,
                    "false generates table with effective centre/width/height parameters. true generates a table with peak function parameters");


    declareProperty("MinimumPeakHeight", DBL_MIN, "Minimum allowed peak height. ");


    // Debug Workspaces
    /*
       declareProperty(new WorkspaceProperty<API::MatrixWorkspace>("BackgroundWorkspace", "", Direction::Output),
       "Temporary Background Workspace ");
       declareProperty(new WorkspaceProperty<API::MatrixWorkspace>("TheorticBackgroundWorkspace", "", Direction::Output),
       "Temporary Background Workspace ");
       declareProperty(new WorkspaceProperty<API::MatrixWorkspace>("PeakWorkspace", "", Direction::Output),
       "Temporary Background Workspace ");
       */

    return;
  }

  //----------------------------------------------------------------------------------------------
  /** Execute the findPeaks algorithm.
    */
  void FindPeaks::exec()
  {
    // Process input
    processAlgorithmProperties();

    // Create those functions to fit
    m_backgroundFunction = createBackgroundFunction(0., 0., 0.);

    // Set up output table workspace
    m_outPeakTableWS = WorkspaceFactory::Instance().createTable("TableWorkspace");
    m_outPeakTableWS->addColumn("int", "spectrum");
    if (m_rawPeaksTable)
    {
      IFunction_sptr temp = this->createFunction(0., 0., 0., 0., 0., 0., true);

      m_numTableParams = temp->nParams();
      for (std::size_t i = 0; i < m_numTableParams; i++)
      {
        m_outPeakTableWS->addColumn("double", temp->parameterName(i));
      }
      if (m_backgroundFunction->nParams() < 3)
        m_outPeakTableWS->addColumn("double", "f1.A2");
    }
    else
    {
      m_numTableParams = 6;
      m_outPeakTableWS->addColumn("double", "centre");
      m_outPeakTableWS->addColumn("double", "width");
      m_outPeakTableWS->addColumn("double", "height");
      m_outPeakTableWS->addColumn("double", "backgroundintercept");
      m_outPeakTableWS->addColumn("double", "backgroundslope");
      m_outPeakTableWS->addColumn("double", "A2");
    }
    m_outPeakTableWS->addColumn("double", "chi2");

    // Fit
    m_searchPeakPos = false;
    if (!m_vecPeakCentre.empty())
    {
      if (!m_vecFitWindows.empty())
      {
        if (m_vecFitWindows.size() != (m_vecPeakCentre.size() * 2))
        {
          throw std::invalid_argument(
                "Number of FitWindows must be exactly twice the number of PeakPositions");
        }
        m_searchPeakPos = true;
      }
      //Perform fit with fixed start positions.
      this->findPeaksGivenStartingPoints(m_vecPeakCentre, m_vecFitWindows);
    }
    else
    {
      //Use Mariscotti's method to find the peak centers
      m_usePeakPositionTolerance = false;
      m_usePeakHeightTolerance = false;
      this->findPeaksUsingMariscotti();
    }

    // 5. Output
    g_log.information() << "Total of " << m_outPeakTableWS->rowCount()
                        << " peaks found and successfully fitted." << std::endl;
    setProperty("PeaksList", m_outPeakTableWS);

    return;
  } // END: exec()

  //----------------------------------------------------------------------------------------------
  /** Process algorithm's properties
    */
  void FindPeaks::processAlgorithmProperties()
  {
    // Input workspace
    m_dataWS = getProperty("InputWorkspace");

    // WorkspaceIndex
    index = getProperty("WorkspaceIndex");
    singleSpectrum = !isEmpty(index);
    if (singleSpectrum && index >= static_cast<int>(m_dataWS->getNumberHistograms()))
    {
      g_log.error() << "The value of WorkspaceIndex provided (" << index
                    << ") is larger than the size of this workspace (" << m_dataWS->getNumberHistograms()
                    << ")\n";
      throw Kernel::Exception::IndexError(index, m_dataWS->getNumberHistograms() - 1,
                                          "FindPeaks WorkspaceIndex property");
    }

    // Peak width
    m_inputPeakFWHM = getProperty("FWHM");
    int t1 = getProperty("MinGuessedPeakWidth");
    int t2 = getProperty("MaxGuessedPeakWidth");
    int t3 = getProperty("GuessedPeakWidthStep");
    if (t1 > t2 || t1 <= 0 || t3 <= 0)
    {
      std::stringstream errss;
      errss << "User specified wrong guessed peak width parameters (must be postive and make sense). "
            << "User inputs are min = " << t1 << ", max = " << t2 << ", step = " << t3;
      g_log.error(errss.str());
      throw std::runtime_error(errss.str());
    }

    m_minGuessedPeakWidth = t1;
    m_maxGuessedPeakWidth = t2;
    m_stepGuessedPeakWidth = t3;

    m_peakPositionTolerance = getProperty("PeakPositionTolerance");
    m_usePeakPositionTolerance = true;
    if (isEmpty(m_peakPositionTolerance))
      m_usePeakPositionTolerance = false;

    m_peakHeightTolerance = getProperty("PeakHeightTolerance");
    m_usePeakHeightTolerance = true;
    if (isEmpty(m_peakHeightTolerance))
      m_usePeakHeightTolerance = false;

    // Specified peak positions, which is optional
    m_vecPeakCentre = getProperty("PeakPositions");
    m_vecFitWindows = getProperty("FitWindows");

    // Peak and ground type
    m_peakFuncType = getPropertyValue("PeakFunction");
    m_backgroundType = getPropertyValue("BackgroundType");

    // Fit algorithm
    m_highBackground = getProperty("HighBackground");

    // Peak parameters are give via a table workspace
    m_rawPeaksTable = getProperty("RawPeakParameters");

    // Minimum peak height
    m_minHeight = getProperty("MinimumPeakHeight");

    return;
  }

  //----------------------------------------------------------------------------------------------
  /** Find the start positions to fit peaks with given estimated peak centres
    * @param peakcentres :: vector of the center x-positions specified to perform fits.
    * @param fitwindows :: vector of windows around each peak. Otherwise, windows will be determined automatically.
    */
  void FindPeaks::findPeaksGivenStartingPoints(const std::vector<double> &peakcentres,
                                               const std::vector<double> &fitwindows)
  {
    bool useWindows = (!fitwindows.empty());
    std::size_t numPeaks = peakcentres.size();

    // Loop over the spectra searching for peaks
    const int start = singleSpectrum ? index : 0;
    const int end = singleSpectrum ? index + 1 : static_cast<int>(m_dataWS->getNumberHistograms());
    m_progress = new Progress(this, 0.0, 1.0, end - start);

    for (int spec = start; spec < end; ++spec)
    {
      g_log.information() << "Finding Peaks In Spectrum " << spec << std::endl;

      const MantidVec& datax = m_dataWS->readX(spec);

      for (std::size_t i = 0; i < numPeaks; i++)
      {
        //Try to fit at this center
        double x_center = peakcentres[i];

        std::stringstream infoss;
        infoss <<  " @ d = " << x_center;
        if (useWindows)
        {
          infoss << " [" << fitwindows[2 * i] << "<" << fitwindows[2 * i + 1] << "]";
        }
        g_log.information(infoss.str());

        // Check whether it is the in data range
        if (x_center > datax.front() && x_center < datax.back())
        {
          if (useWindows)
            fitPeakInWindow(m_dataWS, spec, x_center, fitwindows[2 * i], fitwindows[2 * i + 1]);
          else
            fitPeak(m_dataWS, spec, x_center, m_inputPeakFWHM);
        }

      } // loop through the peaks specified

      m_progress->report();

    } // loop over spectra

  }

  //----------------------------------------------------------------------------------------------
  /** Use the Mariscotti method to find the start positions and fit gaussian peaks
    */
  void FindPeaks::findPeaksUsingMariscotti()
  {
    //At this point the data has not been smoothed yet.
    MatrixWorkspace_sptr smoothedData = this->calculateSecondDifference(m_dataWS);

    // The optimum number of points in the smoothing, according to Mariscotti, is 0.6*fwhm
    int w = static_cast<int>(0.6 * m_inputPeakFWHM);
    // w must be odd
    if (!(w % 2))
      ++w;

    // Carry out the number of smoothing steps given by g_z (should be 5)
    for (int i = 0; i < g_z; ++i)
    {
      this->smoothData(smoothedData, w);
    }
    // Now calculate the errors on the smoothed data
    this->calculateStandardDeviation(m_dataWS, smoothedData, w);

    // Calculate n1 (Mariscotti eqn. 18)
    const double kz = 1.22; // This kz corresponds to z=5 & w=0.6*fwhm - see Mariscotti Fig. 8
    const int n1 = static_cast<int>(kz * m_inputPeakFWHM + 0.5);
    // Can't calculate n2 or n3 yet because they need i0
    const int tolerance = getProperty("Tolerance");

    //  // Temporary - to allow me to look at smoothed data
    //  setProperty("SmoothedData",smoothedData);

    // Loop over the spectra searching for peaks
    const int start = singleSpectrum ? index : 0;
    const int end = singleSpectrum ? index + 1 : static_cast<int>(smoothedData->getNumberHistograms());
    m_progress = new Progress(this, 0.0, 1.0, end - start);
    const int blocksize = static_cast<int>(smoothedData->blocksize());

    for (int k = start; k < end; ++k)
    {
      const MantidVec &S = smoothedData->readY(k);
      const MantidVec &F = smoothedData->readE(k);

      // This implements the flow chart given on page 320 of Mariscotti
      int i0 = 0, i1 = 0, i2 = 0, i3 = 0, i4 = 0, i5 = 0;
      for (int i = 1; i < blocksize; ++i)
      {

        int M = 0;
        if (S[i] > F[i])
          M = 1;
        else
        {
          S[i] > 0 ? M = 2 : M = 3;
        }

        if (S[i - 1] > F[i - 1])
        {
          switch (M)
          {
            case 3:
              i3 = i;
              /* no break */
              // intentional fall-through
            case 2:
              i2 = i - 1;
              break;
            case 1:
              // do nothing
              break;
            default:
              assert( false);
              // should never happen
              break;
          }
        }
        else if (S[i - 1] > 0)
        {
          switch (M)
          {
            case 3:
              i3 = i;
              break;
            case 2:
              // do nothing
              break;
            case 1:
              i1 = i;
              break;
            default:
              assert( false);
              // should never happen
              break;
          }
        }
        else
        {
          switch (M)
          {
            case 3:
              // do nothing
              break;
            case 2: // fall through (i.e. same action if M = 1 or 2)
            case 1:
              i5 = i - 1;
              break;
            default:
              assert( false);
              // should never happen
              break;
          }
        }

        if (i5 && i1 && i2 && i3) // If i5 has been set then we should have the full set and can check conditions
        {
          i4 = i3; // Starting point for finding i4 - calculated below
          double num = 0.0, denom = 0.0;
          for (int j = i3; j <= i5; ++j)
          {
            // Calculate i4 - it's at the minimum value of Si between i3 & i5
            if (S[j] <= S[i4])
              i4 = j;
            // Calculate sums for i0 (Mariscotti eqn. 27)
            num += j * S[j];
            denom += S[j];
          }
          i0 = static_cast<int>(num / denom);

          // Check we have a correctly ordered set of points. If not, reset and continue
          if (i1 > i2 || i2 > i3 || i3 > i4 || i5 <= i4)
          {
            i5 = 0;
            continue;
          }

          // Check if conditions are fulfilled - if any are not, loop onto the next i in the spectrum
          // Mariscotti eqn. (14)
          if (std::abs(S[i4]) < 2 * F[i4])
          {
            i5 = 0;
            continue;
          }
          // Mariscotti eqn. (19)
          if (abs(i5 - i3 + 1 - n1) > tolerance)
          {
            i5 = 0;
            continue;
          }
          // Calculate n2 (Mariscotti eqn. 20)
          int n2 = abs(static_cast<int>(0.5 * (F[i0] / S[i0]) * (n1 + tolerance) + 0.5));
          const int n2b = abs(static_cast<int>(0.5 * (F[i0] / S[i0]) * (n1 - tolerance) + 0.5));
          if (n2b > n2)
            n2 = n2b;
          // Mariscotti eqn. (21)
          const int testVal = n2 ? n2 : 1;
          if (i3 - i2 - 1 > testVal)
          {
            i5 = 0;
            continue;
          }
          // Calculate n3 (Mariscotti eqn. 22)
          int n3 = abs(static_cast<int>((n1 + tolerance) * (1 - 2 * (F[i0] / S[i0])) + 0.5));
          const int n3b = abs(static_cast<int>((n1 - tolerance) * (1 - 2 * (F[i0] / S[i0])) + 0.5));
          if (n3b < n3)
            n3 = n3b;
          // Mariscotti eqn. (23)
          if (i2 - i1 + 1 < n3)
          {
            i5 = 0;
            continue;
          }

          // If we get to here then we've identified a peak
          g_log.debug() << "Spectrum=" << k << " i0=" << i0 << " X=" << m_dataWS->readX(k)[i0] << " i1="
                        << i1 << " i2=" << i2 << " i3=" << i3 << " i4=" << i4 << " i5=" << i5 << std::endl;

          // Use i0, i2 and i4 to find out i_min and i_max, i0: right, i2: left, i4: centre
          int wssize = static_cast<int>(m_dataWS->readX(k).size());

          int iwidth = i0 - i2;
          if (iwidth <= 0)
            iwidth = 1;

          int i_min = 1;
          if (i4 > 5*iwidth)
            i_min = i4 - 5*iwidth;

          int i_max = i4 + 5*iwidth;
          if (i_max >= wssize)
            i_max = wssize - 1;

          this->fitPeak(m_dataWS, k, i_min, i_max, i4);

          // reset and go searching for the next peak
          i1 = 0, i2 = 0, i3 = 0, i4 = 0, i5 = 0;
        }

      } // loop through a single spectrum

      m_progress->report();

    } // loop over spectra

  }

  //----------------------------------------------------------------------------------------------
  /** Calculates the second difference of the data (Y values) in a workspace.
    *  Done according to equation (3) in Mariscotti: \f$ S_i = N_{i+1} - 2N_i + N_{i+1} \f$.
    *  In the output workspace, the 2nd difference is in Y, X is unchanged and E is zero.
    *  @param input :: The workspace to calculate the second difference of
    *  @return A workspace containing the second difference
    */
  API::MatrixWorkspace_sptr FindPeaks::calculateSecondDifference(
      const API::MatrixWorkspace_const_sptr &input)
  {
    // We need a new workspace the same size as the input ont
    MatrixWorkspace_sptr diffed = WorkspaceFactory::Instance().create(input);

    const size_t numHists = input->getNumberHistograms();
    const size_t blocksize = input->blocksize();

    // Loop over spectra
    for (size_t i = 0; i < size_t(numHists); ++i)
    {
      // Copy over the X values
      diffed->dataX(i) = input->readX(i);

      const MantidVec &Y = input->readY(i);
      MantidVec &S = diffed->dataY(i);
      // Go through each spectrum calculating the second difference at each point
      // First and last points in each spectrum left as zero (you'd never be able to find peaks that close to the edge anyway)
      for (size_t j = 1; j < blocksize - 1; ++j)
      {
        S[j] = Y[j - 1] - 2 * Y[j] + Y[j + 1];
      }
    }

    return diffed;
  }

  //----------------------------------------------------------------------------------------------
  /** Calls the SmoothData algorithm as a Child Algorithm on a workspace.
    * It is used in Mariscotti
    *  @param WS :: The workspace containing the data to be smoothed. The smoothed result will be stored in this pointer.
    *  @param w ::  The number of data points which should contribute to each smoothed point
    */
  void FindPeaks::smoothData(API::MatrixWorkspace_sptr &WS, const int &w)
  {
    g_log.information("Smoothing the input data");
    IAlgorithm_sptr smooth = createChildAlgorithm("SmoothData");
    smooth->setProperty("InputWorkspace", WS);
    // The number of points which contribute to each smoothed point
    std::vector<int> wvec;
    wvec.push_back(w);
    smooth->setProperty("NPoints", wvec);
    smooth->executeAsChildAlg();
    // Get back the result
    WS = smooth->getProperty("OutputWorkspace");
  }

  //----------------------------------------------------------------------------------------------
  /** Calculates the statistical error on the smoothed data.
    *  Uses Mariscotti equation (11), amended to use errors of input data rather than sqrt(Y).
    *  @param input ::    The input data to the algorithm
    *  @param smoothed :: The smoothed dataBackgroud type is not supported in FindPeak.cpp
    *  @param w ::        The value of w (the size of the smoothing 'window')
    *  @throw std::invalid_argument if w is greater than 19
    */
  void FindPeaks::calculateStandardDeviation(const API::MatrixWorkspace_const_sptr &input,
                                             const API::MatrixWorkspace_sptr &smoothed, const int &w)
  {
    // Guard against anyone changing the value of z, which would mean different phi values were needed (see Marriscotti p.312)
    assert( g_z == 5);
    // Have to adjust for fact that I normalise Si (unlike the paper)
    const int factor = static_cast<int>(std::pow(static_cast<double>(w), g_z));

    const double constant = sqrt(static_cast<double>(this->computePhi(w))) / factor;

    const size_t numHists = smoothed->getNumberHistograms();
    const size_t blocksize = smoothed->blocksize();
    for (size_t i = 0; i < size_t(numHists); ++i)
    {
      const MantidVec &E = input->readE(i);
      MantidVec &Fi = smoothed->dataE(i);

      for (size_t j = 0; j < blocksize; ++j)
      {
        Fi[j] = constant * E[j];
      }
    }
  }

  //----------------------------------------------------------------------------------------------
  /** Calculates the coefficient phi which goes into the calculation of the error on the smoothed data
    *  Uses Mariscotti equation (11). Pinched from the GeneralisedSecondDifference code.
    *  Can return a very big number, hence the type.
    *  @param  w The value of w (the size of the smoothing 'window')
    *  @return The value of phi(g_z,w)
    */
  long long FindPeaks::computePhi(const int& w) const
  {
    const int m = (w - 1) / 2;
    int zz = 0;
    int max_index_prev = 1;
    int n_el_prev = 3;
    std::vector<long long> previous(n_el_prev);
    previous[0] = 1;
    previous[1] = -2;
    previous[2] = 1;

    // Can't happen at present
    if (g_z == 0)
      return std::accumulate(previous.begin(), previous.end(), static_cast<long long>(0),
                             VectorHelper::SumSquares<long long>());

    std::vector<long long> next;
    // Calculate the Cij iteratively.
    do
    {
      zz++;
      int max_index = zz * m + 1;
      int n_el = 2 * max_index + 1;
      next.resize(n_el);
      std::fill(next.begin(), next.end(), 0);
      for (int i = 0; i < n_el; ++i)
      {
        int delta = -max_index + i;
        for (int l = delta - m; l <= delta + m; l++)
        {
          int index = l + max_index_prev;
          if (index >= 0 && index < n_el_prev)
            next[i] += previous[index];
        }
      }
      previous.resize(n_el);
      std::copy(next.begin(), next.end(), previous.begin());
      max_index_prev = max_index;
      n_el_prev = n_el;
    } while (zz != g_z);

    const long long retval = std::accumulate(previous.begin(), previous.end(),
                                             static_cast<long long>(0), VectorHelper::SumSquares<long long>());
    g_log.debug() << "FindPeaks::computePhi - calculated value = " << retval << "\n";
    return retval;
  }

  //----------------------------------------------------------------------------------------------
  /** Find the index of a value (or nearest) in a given sorted vector (vector of x axis)
    * @param X :: vector
    * @param centre :: value to search
    */
  int FindPeaks::getVectorIndex(const MantidVec &vecX, double x)
  {
    int index;

    if (x <= vecX[0])
    {
      // Left or equal to lower boundary
      index = 0;
    }
    else if (x >= vecX.back())
    {
      // Right or equal to upper boundary
      index = static_cast<int>(vecX.size()) - 1;
    }
    else
    {
      // within the range
      index = static_cast<int>(std::lower_bound(vecX.begin(), vecX.end(), x) - vecX.begin());

      // check lower boundary
      if (index == 0)
      {
        std::stringstream errss;
        errss << "Returned index = 0 for x = " << x << " with X[0] = " << vecX[0]
              << ". This situation is ruled out in this algorithm.";
        g_log.error(errss.str());
        throw std::runtime_error(errss.str());
      }
      else if (x < vecX[index-1] || x > vecX[index])
      {
        std::stringstream errss;
        errss << "Returned x = " << x << " is not between " << vecX[index-1] << " and "
              << vecX[index] << ", which are returned by lower_bound.";
        g_log.error(errss.str());
        throw std::runtime_error(errss.str());
      }

      // Find the index of the nearest value to return
      if (x - vecX[index-1] < vecX[index] - x)
        -- index;
    }

    return index;
  }

  //----------------------------------------------------------------------------------------------
  /** Get index of the maximum value in a vector.
    * @param Y :: vector to search for the max
    * @param leftIndex :: left boundary (reached)
    * @param rightIndex :: right boundary (not reached)
    */
  int getMaxHeightIndex(const MantidVec &Y, const int leftIndex, const int rightIndex)
  {
    double maxY = Y[leftIndex];
    int indexMax = leftIndex;
    for (int i = leftIndex + 1; i < rightIndex; i++)
    {
      if (Y[i] > maxY)
      {
        maxY = Y[i];
        indexMax = i;
      }
    }

    return indexMax;
  }

  //----------------------------------------------------------------------------------------------
  /** Attempts to fit a candidate peak given a center and width guess.
    * (This is not the CORE fit peak method)
    *
    *  @param input ::    The input workspace
    *  @param spectrum :: The spectrum index of the peak (is actually the WorkspaceIndex)
    *  @param center_guess :: A guess of the X-value of the center of the peak, in whatever units of the X-axis of the workspace.
    *  @param FWHM_guess :: A guess of the full-width-half-max of the peak, in # of bins.
    */
  void FindPeaks::fitPeak(const API::MatrixWorkspace_sptr &input, const int spectrum,
                          const double center_guess, const int FWHM_guess)
  {
    g_log.information() << "Fit peak with guessed FWHM:  starting center = " << center_guess
                        << ", FWHM = " << FWHM_guess << ".\n";

    // The indices
    int i_centre;

    // The X axis you are looking at
    const MantidVec &vecX = input->readX(spectrum);
    const MantidVec &vecY = input->readY(spectrum);

    // Find i_center - the index of the center - The guess is within the X axis?
    i_centre = this->getVectorIndex(vecX, center_guess);

    const int fitWidth = FWHM_guess;

    // See Mariscotti eqn. 20. Using l=1 for bg0/bg1 - correspond to p6 & p7 in paper.
    int i_min = 1;
    if (i_centre > 5 * fitWidth)
      i_min = i_centre - 5 * fitWidth;
    if (i_min < 1)
      i_min = 1;

    int i_max = i_centre + 5 * fitWidth;
    if (i_max >= static_cast<int>(vecX.size())-1)
      i_max = static_cast<int>(vecY.size())-2;

    // Check
    if (i_max - i_min <= 0)
      throw std::runtime_error("Impossible to i_min >= i_max.");

    g_log.debug() << "Background + Peak -- Bounds Fit Range = " << vecX[i_min]
                  << ", " << vecX[i_max] << ".\n";

    fitPeak(input, spectrum, i_min, i_max, i_centre);

    return;

  }

  //----------------------------------------------------------------------------------------------
  /** Attempts to fit a candidate peak with a given window of where peak resides
    *
    *  @param input    The input workspace
    *  @param spectrum The spectrum index of the peak (is actually the WorkspaceIndex)
    *  @param centre_guess ::  Channel number of peak candidate i0 - the higher side of the peak (right side)
    *  @param xmin    Minimum x value to find the peak
    *  @param xmax    Maximum x value to find the peak
    */
  void FindPeaks::fitPeakInWindow(const API::MatrixWorkspace_sptr &input, const int spectrum,
                                  const double centre_guess, const double xmin, const double xmax)
  {
    // Check
    g_log.information() << "Fit Peak with given window:  Guessed center = " << centre_guess
                        << "  x-min = " << xmin
                        << ", x-max = " << xmax << "\n";
    if (xmin >= centre_guess || xmax <= centre_guess)
    {
      throw std::runtime_error("Peak centre is on the edge of Fit window. ");
    }

    //The X axis you are looking at
    const MantidVec &vecX = input->readX(spectrum);

    //The centre index
    int i_centre = this->getVectorIndex(vecX, centre_guess);

    //The left index
    int i_min = getVectorIndex(vecX, xmin);
    if (i_min >= i_centre)
    {
      std::stringstream errss;
      errss << "Input peak centre @ " << centre_guess << " is out side of minimum x = "
            << xmin << ".  Input X ragne = " << vecX.front() << ", " << vecX.back();
      g_log.error(errss.str());
      throw std::runtime_error(errss.str());
    }

    //The right index
    int i_max = getVectorIndex(vecX, xmax);
    if (i_max < i_centre)
    {
      std::stringstream errss;
      errss << "Input peak centre @ " << centre_guess << " is out side of maximum x = "
            << xmax;
      g_log.error(errss.str());
      throw std::runtime_error(errss.str());
    }

    // look for the heigh point
    if (m_searchPeakPos)
    {
      i_centre = getMaxHeightIndex(input->readY(spectrum), i_min, i_max);
      if (i_centre == i_min)
        ++ i_centre;
      else if (i_centre == i_max)
        -- i_centre;
    }

    // finally do the actual fit
    fitPeak(input, spectrum, i_min, i_max, i_centre);

    return;
  }

  //----------------------------------------------------------------------------------------------
  /** Attempts to fit a candidate peak
    *  This is the core fitPeak() to call in the fitPeak hierarchy
    *
    *  @param input ::    The input workspace
    *  @param spectrum :: The spectrum index of the peak (is actually the WorkspaceIndex)
    *  @param i_max ::    Channel number of peak candidate i0 - the higher side of the peak (right side)
    *  @param i_min ::    Channel number of peak candidate i2 - the lower side of the peak (left side)
    *  @param i_centre :  Channel number of peak candidate i4 - the center of the peak
    */
  void FindPeaks::fitPeak(const API::MatrixWorkspace_sptr &input, const int spectrum, const int i_min,
                          const int i_max, const int i_centre)
  {
    const MantidVec &vecX = input->readX(spectrum);
    const MantidVec &vecY = input->readY(spectrum);

    g_log.information() << "Fit Peak @ " << vecX[i_centre] << "  of Spectrum " << spectrum << ". Fit peak In Range "
                        << vecX[i_min] << ", " << vecX[i_max] << "  i_min = " << i_min << ", i_max = "
                        << i_max << ", i_centre = " << i_centre << ".\n";

    // Estimate height, boundary, and etc for fitting
    // FIXME - Does this make sense?
    double bg_lowerSum;
    if (i_min > 0)
      bg_lowerSum = vecY[i_min - 1] + vecY[i_min] + vecY[i_min + 1];
    else
      bg_lowerSum = vecY[i_min + 2] + vecY[i_min] + vecY[i_min + 1];

    double bg_upperSum;
    if (i_max < static_cast<int>(vecY.size())-1)
    {
      bg_upperSum = vecY[i_max - 1] + vecY[i_max] + vecY[i_max + 1];
    }
    else
    {
      bg_upperSum = vecY[i_max - 1] + vecY[i_max] + vecY[i_max -2];
    }

    double in_bg0 = (bg_lowerSum + bg_upperSum) / 6.0;
    double in_bg1 = (bg_upperSum - bg_lowerSum) / (3.0 * static_cast<double>(i_max - i_min + 1));
    double in_bg2 = 0.0;

    if (!m_highBackground)
    {
      // Not high background.  Fit background and peak together (The original Method)
      //  fitPeakOneStep(input, spectrum, i0, i2, i4, in_bg0, in_bg1, in_bg2);
      fitPeakOneStep(input, spectrum, i_min, i_max, i_centre, in_bg0, in_bg1, in_bg2);
    }
    else
    {
      // High background
      fitPeakHighBackground(input, spectrum, i_centre, i_min, i_max, in_bg0, in_bg1, in_bg2);

    } // if high background

    g_log.debug() << "Fit Peak Over" << std::endl;

    return;
  }

  //----------------------------------------------------------------------------------------------
  /** Fit 1 peak in one step, i.e., one function combining both Gaussian and background
    *
    * @param input:: input Matrix workspace
    * @param spectrum :: workspace index of the spectrum to find peaks
    * @param i_max:: bin index of right end of data to find peak
    * @param i_min:: bin index of left end of data to find peak
    * @param i_centre:: bin index of center of peak
    * @param in_bg0: guessed value of a0 (in/out)
    * @param in_bg1: guessed value of a1 (in/out)
    * @param in_bg2: guessed value of a2 (in/out)
    */
  void FindPeaks::fitPeakOneStep(const API::MatrixWorkspace_sptr &input, const int spectrum,
                                 const int& i_min, const int& i_max, const int& i_centre,
                                 const double& in_bg0, const double& in_bg1, const double& in_bg2)
  {
    g_log.information("Fitting Peak in a one-step approach. ");

    const MantidVec &vecX = input->readX(spectrum);
    const MantidVec &vecY = input->readY(spectrum);

    // Initialize some starting values
    // FIXME - This only works for flat background!
    const double in_height = vecY[i_centre] - in_bg0;
    const double in_centre = input->isHistogramData() ? 0.5 * (vecX[i_centre] + vecX[i_centre + 1]) : vecX[i_centre];

    double mincost = DBL_MAX;
    std::vector<double> bestparams, bestRawParams;

    // Loop around peak width
    for (int width = m_minGuessedPeakWidth; width <= m_maxGuessedPeakWidth; width +=
         m_stepGuessedPeakWidth)
    {
      // Set up Child Algorithm Fit
      IAlgorithm_sptr fit;
      try
      {
        // Fitting the candidate peaks to a Gaussian
        fit = createChildAlgorithm("Fit", -1, -1, true);
      }
      catch (Exception::NotFoundError &)
      {
        std::string errorstr("FindPeaks algorithm requires the CurveFitting library");
        g_log.error(errorstr);
        throw std::runtime_error(errorstr);
      }

      // Calculate the guessed sigma
      // const double in_sigma = (i0 + width < vecX.size()) ? vecX[i0 + width] - vecX[i0] : 0.;
      int vecsize = static_cast<int>(vecX.size());
      const int i_right = (i_centre + width < vecsize) ? i_centre + width : vecsize-1;
      const int i_left = (i_centre - width >= 0) ? i_centre - width : 0;
      const double in_sigma = vecX[i_right] - vecX[i_left];

      // Create function to fit
      IFunction_sptr fitFunction = createFunction(in_height, in_centre, in_sigma, in_bg0, in_bg1,
                                                  in_bg2, true);
      g_log.debug() << "  Function: " << fitFunction->asString() << "; Background Type = "
                    << m_backgroundType << std::endl;

      // Set up Fit algorithm
      fit->setProperty("Function", fitFunction);
      fit->setProperty("InputWorkspace", input);
      fit->setProperty("WorkspaceIndex", spectrum);
      fit->setProperty("MaxIterations", 50);
      fit->setProperty("StartX", vecX[i_min]); //(X[i0] - 5 * (X[i0] - X[i2])));
      fit->setProperty("EndX", vecX[i_max]); //(X[i0] + 5 * (X[i0] - X[i2])));
      fit->setProperty("Minimizer", m_minimizer);
      fit->setProperty("CostFunction", "Least squares");

      // e) Fit and get result
      fit->executeAsChildAlg();

      updateFitResults(fit, bestparams, bestRawParams, mincost, in_centre, in_height);

    } // ENDFOR: Loop over "width"

    // Update output
    if (bestparams.size() > 1)
      addInfoRow(spectrum, bestparams, bestRawParams, mincost,
             (bestparams[0] < vecX.front() || bestparams[0] > vecX.back()));
    else
      addInfoRow(spectrum, bestparams, bestRawParams, mincost, true);

    // Update collection of peaks
    IFunction_sptr fitFunction = createFunction(0.0, 0.0, 0.0, 0.0, 0.0, 0.0, true);
    for (size_t i = 0; i < bestRawParams.size(); ++i)
      fitFunction->setParameter(i, bestRawParams[i]);
    addFittedFunction(fitFunction, i_min, i_max);

    return;
  }

  //----------------------------------------------------------------------------------------------
  /** Fit peak with high background
    *
    * About fit algorithm
    * (1) Get 2 ends of workspace to estimate background
    *   - assuming it is linear or quasilinear;
    *   - if the background noise level is comparible to peak height, then the peak is not worth fitting!
    *     in the case that the the peak height is dimmed by making a straight line between 2 end points
    *   - if the peak shifts off the given range, then there is NO hope
    * (2) Estimate peak height
    * (3) Fit scheme 1:
    *   a. Loop over specified peak's FWHM range to fit peak with fixed background
    *   b. Choose the best result and fit the background again
    *   c. Fit peak with every open
    * (4) Fit peak with observed/estimated peak parameters
    *   a. Fit pure peak
    *   b. Fit pure background
    *   c. Fit them all
    * (5) Choose the better result between (3) and (4)
    *
    * About book keeping
    * - Each fit will return
    *
    * @param input :: matrix workspace to fit with
    * @param spectrum :: workspace index of the spetrum to fit with
    * @param i_centre: bin index of center of peak (raw/input data workspace)
    * @param i_min: bin index of left bound of fit range (raw/input data workspace)
    * @param i_max: bin index of right bound of fit range (raw/input data workspace)
    * @param in_bg0: guessed value of a0 (output)
    * @param in_bg1: guessed value of a1 (output)
    * @param in_bg2: guessed value of a2 (output)
    */
  void FindPeaks::fitPeakHighBackground(const API::MatrixWorkspace_sptr &input, const size_t spectrum,
                                        const int& i_centre, const int& i_min, const int& i_max,
                                        double& in_bg0, double& in_bg1, double& in_bg2)
  {
    g_log.information() << "Fitting a peak assumed at " << input->dataX(spectrum)[i_centre]
                        << " (index = " << i_centre << ") by high-background approach. \n";

    // Check
    if (i_min >= i_centre || i_max <= i_centre || i_min < 0)
    {
      std::stringstream errss;
      errss << "FitPeakHightBackground has erroreous input.  i_min = " << i_min << ", i_centre = "
            << i_centre << ", i_max = " << i_max;
      g_log.error(errss.str());
      throw std::runtime_error(errss.str());
    }
    double user_centre = input->readX(spectrum)[i_centre];

    // Prepare
    const MantidVec &rawX = input->readX(spectrum);
    const MantidVec &rawY = input->readY(spectrum);

    // Estimate linear background: output-> m_backgroundFunction
    if (m_backgroundFunction->nParams() == 1)
    {
      // Flat background
      estimateFlatBackground(rawY, i_min, i_max, in_bg0, in_bg1, in_bg2);
    }
    else
    {
      estimateLinearBackground(rawX, rawY, i_min, i_max, in_bg0, in_bg1, in_bg2);
    }

    // Create a pure peak workspace (Workspace2D)
    int numpts = i_max-i_min+1;
    if (numpts <= 0)
      throw std::runtime_error("FitPeakHighBackground.  Pure peak workspace size <= 0.");

    size_t sizex = static_cast<size_t>(numpts);
    size_t sizey = sizex;
    API::MatrixWorkspace_sptr peakws =
        API::WorkspaceFactory::Instance().create("Workspace2D", 1,  sizex, sizey);

    //    set up x-axis first
    MantidVec& dataX = peakws->dataX(0);
    dataX.assign(rawX.begin()+i_min, rawX.begin()+i_min+numpts);

    //    set up Y/E as pure peak
    FunctionDomain1DVector domain(dataX);
    FunctionValues backgroundvalues(domain);
    m_backgroundFunction->function(domain, backgroundvalues);

    MantidVec& dataY = peakws->dataY(0);
    for (int i = 0; i < numpts; ++i)
    {
      dataY[i] = rawY[i_min+i] - backgroundvalues[i];
      if (dataY[i] < 0)
        dataY[i] = 0.;
    }
    MantidVec& dataE = peakws->dataE(0);
    dataE.assign(numpts, 1.);

    // Estimate/observe peak parameters
    const MantidVec& peakX = peakws->readX(0);
    const MantidVec& peakY = peakws->readY(0);

    double g_centre, g_height, g_fwhm;
    std::string errormessage;
    bool goodestimate = estimatePeakParameters(peakX, peakY, 0, numpts-1, g_centre, g_height, g_fwhm, errormessage);
    if (!goodestimate)
    {
      if (errormessage.compare("Flat spectrum") == 0)
      {
        // Flat spectrum. No fit
        addNonFitRecord(spectrum);
        return;
      }
      else if (errormessage.compare("Fluctuation is less than minimum allowed value.") == 0)
      {
        addNonFitRecord(spectrum);
        return;
      }
      else if (errormessage.compare("Maximum value on edge") == 0)
      {
        addNonFitRecord(spectrum);
        return;
      }
      else
      {
        // Peak is on the edge.  It is not possible to fit!
        g_log.warning() << "Unable to estimate peak parameter for "
                        << "Spectrum " << spectrum << ": Atttemp to find peak between "
                        << rawX[i_min] << "(i = " << i_min
                        << ") and " << rawX[i_max] << "(i = " << i_max << ")."
                        << "Assumed peak is at " << user_centre << "."
                        << "\nError reason: " << errormessage;
        /*
        errmsg << "Background: " << m_backgroundFunction->asString() << "; Background points are as follow.\n";
        for (size_t i = 0; i < static_cast<size_t>(numpts); ++i)
        {
          errmsg << domain[i] << "\t\t" << backgroundvalues[i] << "\n";
        }
        */

        addNonFitRecord(spectrum);
        return;
      }
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

    g_log.debug() << "[DB_Bkgd] Reset A0/A1 to: A0 = " << in_bg0 << ", A1 = " << in_bg1 << ".\n";
    m_backgroundFunction->setParameter("A0", in_bg0);
    if (m_backgroundFunction->nParams() > 1)
      m_backgroundFunction->setParameter("A1", in_bg1);

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
  }

  //----------------------------------------------------------------------------------------------
  /** Fit a single peak with given peak parameters as starting point
    * @param purepeakws :: data workspace containg peak with background removed
    * @param purepeakindex :: workspace index for purepeakws
    * @param dataws :: raw data workspace
    * @param datawsindex :: workspace index of the spectrum in raw data workspace
    * @param peak :: peak function to fit
    * @param in_centre :: starting value of peak centre
    * @param in_height :: starting value of peak height
    * @param in_fhwm :: starting value of peak width
    * @param peakleftboundary :: left boundary of peak
    * @param peakrightboundary :: right boundary of peak
    * @param user_centre :: peak centre input by user
    */
  PeakFittingRecord FindPeaks::multiFitPeakBackground(MatrixWorkspace_sptr purepeakws, size_t purepeakindex,
                                                      MatrixWorkspace_sptr dataws, size_t datawsindex,
                                                      IPeakFunction_sptr peak,
                                                      double in_centre, double in_height, std::vector<double> in_fwhms,
                                                      double peakleftboundary, double peakrightboundary, double user_centre)
  {
    g_log.information() << "Fit peak with " << in_fwhms.size() << " starting sigmas.\n";
    if (in_fwhms.size() == 0)
    {
      std::stringstream errss;
      errss << "Caller input an empty vector for sigma for peak near " << in_centre;
      g_log.error(errss.str());
      throw std::runtime_error(errss.str());
    }

    // Define some data structure
    std::vector<double> vecRwp;
    std::vector<std::map<std::string, double> > vecParameters;

    // Create composite function
    CompositeFunction_sptr compfunc(new CompositeFunction());
    compfunc->addFunction(peak);
    compfunc->addFunction(m_backgroundFunction);

    // Fit PEAK function without background
    std::string peakcentreconstraint = makePeakCentreConstraint(peak, peakleftboundary, peakrightboundary, false);
    double startx = purepeakws->readX(purepeakindex)[0];
    double endx = purepeakws->readX(purepeakindex).back();

    for (size_t i = 0; i < in_fwhms.size(); ++i)
    {
      // (Re)set peak parameters
      peak->setCentre(in_centre);
      peak->setHeight(in_height);
      peak->setFwhm(in_fwhms[i]);

      // double startx = in_centre - 2.*in_fwhms[i];
      // double endx = in_centre + 2.*in_fwhms[i];

      double in_rwp;
      double rwp1 = fitPeakBackgroundFunction(peak, purepeakws, purepeakindex, startx, endx, peakcentreconstraint,
                                              in_rwp);

      // Check boundaries.  GSL fit may not be able to limit the value in the boundary
      std::string failreason;
      if (rwp1 < DBL_MAX)
      {
        double f_height = peak->height();
        double f_centre = peak->centre();

        if (f_height <= 0.)
        {
          rwp1 = DBL_MAX;
          failreason = "Negative peak height.";
        }
        else if (f_centre < peakleftboundary || f_centre > peakrightboundary)
        {
          rwp1 = DBL_MAX;
          failreason = "Peak centre out of constraint range.";
        }
        else if (m_usePeakPositionTolerance)
        {
          if (fabs(f_centre - user_centre) > m_peakPositionTolerance)
          {
            rwp1 = DBL_MAX;
            failreason = "Peak centre out of tolerance";
          }
        }
      }
      else
      {
        failreason = "(Single-step) Fit returns a DBL_MAX.";
      }

      vecRwp.push_back(rwp1);
      std::map<std::string, double> parameters = getFunctionParameters(peak);
      vecParameters.push_back(parameters);

      // Debug output
      std::stringstream dbss;
      dbss << "\tFit pure peak (result).  Starting Sigma = " << in_fwhms[i] << ": Rwp = " << rwp1 << ", "
           << "Centre = " << peak->centre() << ", Height = " << peak->height() << ", Sigma = "
           << peak->fwhm() << ". ";
      if (failreason.size() > 0)
        dbss << "Cause of failure: " << failreason;
      g_log.debug(dbss.str());
    }

    // Set again to best result so far
    int bestindex = getBestResult(vecRwp);
    if (bestindex < 0 && vecParameters.size() > 0)
    {
      // All fit attempts are failed.  Return with a FAIL record
      g_log.debug("Failed to fit peak without background. ");
      PeakFittingRecord failrecord;
      std::map<std::string, double> bkgdmap0 = getFunctionParameters(m_backgroundFunction);
      failrecord.set(DBL_MAX, vecParameters[0], bkgdmap0);
      return failrecord;
    }
    else
    {
      if (vecParameters.size() == 0)
      {
        g_log.error("Vector of parameters has size 0.  It is a very unlikely situation.");
        throw std::runtime_error("Vector of parameters has size 0.  It is a very unlikely situation.");
      }
    }

    // Fit is good.
    setFunctionParameterValue(peak, vecParameters[bestindex]);
    g_log.information() << "Best fit result is No. " << bestindex << " with guess sigma = "
                        << in_fwhms[bestindex] << ".\n";

    // Fit background with better esitmation on peak (: m_backgroundFunction)
    //   Unfix background parameters
    g_log.information("\tFit background from fitted peak.");
    size_t numbkgdparams = m_backgroundFunction->nParams();
    for (size_t i = 0; i < numbkgdparams; ++i)
      m_backgroundFunction->unfix(i);

    const MantidVec& rawX = dataws->readX(datawsindex);
    const MantidVec& rawY = dataws->readY(datawsindex);
    const MantidVec& rawE = dataws->readE(datawsindex);
    int ileft = getVectorIndex(rawX, peak->centre()-3.0*peak->fwhm());
    int iright = getVectorIndex(rawX, peak->centre()+3.0*peak->fwhm());
    int i_min = getVectorIndex(rawX, purepeakws->readX(purepeakindex)[0]);
    int i_max = getVectorIndex(rawX, purepeakws->readX(purepeakindex).back());
    double bkgdchi2;  // in_bg0, in_bg1, in_bg2,
    g_log.debug() << "F1119 Fit background: iLeft = " << ileft << ", iRight = " << iright
                  << ", iMin = " << i_min << ", iMax = " << i_max << ".\n";
    bool goodfit = fitBackground(rawX, rawY, rawE,  ileft, iright, i_min, i_max, bkgdchi2);
    if (!goodfit)
    {
      g_log.warning("Fitting background by excluding peak failed.");
    }
    std::map<std::string, double> bkgdmap1 = getFunctionParameters(m_backgroundFunction);

    // Fit with new background and every data points
    peakcentreconstraint = makePeakCentreConstraint(peak, peakleftboundary, peakrightboundary, true);
    double rwp1best;
    // double startx = peak->centre()-2.*peak->fwhm();
    // double endx = peak->centre()+2.*peak->fwhm();
    double rwp2 = fitPeakBackgroundFunction(compfunc, dataws, datawsindex, startx, endx, peakcentreconstraint,
                                            rwp1best);
    std::string failreason;
    if (rwp2 < DBL_MAX)
    {
      double f_height = peak->height();
      double f_centre = peak->centre();

      if (f_height <= 0.)
      {
        rwp2 = DBL_MAX;
        failreason = "Negative peak height.";
      }
      else if (f_centre < peakleftboundary || f_centre > peakrightboundary)
      {
        rwp2 = DBL_MAX;
        failreason = "Peak centre out of constraint range.";
      }
      else if (m_usePeakPositionTolerance)
      {
        if (fabs(f_centre - user_centre) > m_peakPositionTolerance)
        {
          rwp2 = DBL_MAX;
          failreason = "Peak centre out of tolerance";
        }
      }
    }
    else
    {
      failreason = "(Single-step) Fit returns a DBL_MAX.";
    }

    g_log.debug() << "[Fx418] Fit (2) failed due to reason " << failreason << ".\n";

    std::map<std::string, double> parameters = getFunctionParameters(peak);
    std::map<std::string, double> bkgdmap2 = getFunctionParameters(m_backgroundFunction);
    vecParameters.push_back(parameters);

    PeakFittingRecord frd;
    if (rwp1best < rwp2)
    {
      frd.set(rwp1best, vecParameters[bestindex], bkgdmap1);
    }
    else
    {
      frd.set(rwp2, vecParameters.back(), bkgdmap2);
    }

    return frd;
  } // END-FUNCTION: fitPeakHighBackground()

  //----------------------------------------------------------------------------------------------
  /** Get function parameters
    * @param func :: an IFunction object
    * @return :: a map having pairs of string (parameter name) and double (parameter value)
    */
  std::map<std::string, double> FindPeaks::getFunctionParameters(IFunction_sptr func)
  {
    std::map<std::string, double> parammap;
    std::vector<std::string> parnames = func->getParameterNames();
    for (size_t i = 0; i < parnames.size(); ++i)
    {
      std::string parname = parnames[i];
      double parvalue = func->getParameter(parname);
      parammap.insert(std::make_pair(parname, parvalue));
    }

    return parammap;
  }

  //----------------------------------------------------------------------------------------------
  /** Set parameters values to a function via a map
    * @param function :: IFunction object
    * @param parvalues :: map for string (parameter name) and double (parameter value)
    */
  void FindPeaks::setFunctionParameterValue(API::IFunction_sptr function, std::map<std::string, double> parvalues)
  {
    std::map<std::string, double>::iterator piter;
    for (piter = parvalues.begin(); piter != parvalues.end(); ++piter)
    {
      std::string parname = piter->first;
      double parvalue = piter->second;
      function->setParameter(parname, parvalue);
    }

    return;
  }


  //----------------------------------------------------------------------------------------------
  /** Compare 2 fit results and record the better one in output table workspace
    * @param r1 :: FitRecord 1
    * @param r2 :: FitRecord 2
    * @param peak :: peak function that is fitted
    * @param bkgdfunc :: background function that is fitted
    * @param spectrum :: spectrum where peak is
    * @param imin :: index of peak's left boundary point in input workspace
    * @param imax :: index of peak's right boundary point in input workspace
    * @param windowsize :: size of the window (range) for fitting the peak
    */
  void FindPeaks::processFitResult(PeakFittingRecord& r1, PeakFittingRecord& r2, IPeakFunction_sptr peak,
                                   IFunction_sptr bkgdfunc, size_t spectrum, size_t imin, size_t imax, double windowsize)
  {
    // Select a better result
    PeakFittingRecord bestR;

    if (r1.getChiSquare() < r2.getChiSquare())
    {
      g_log.information("Loop on FWHM renders a better result.");
      bestR = r1;
    }
    else
    {
      g_log.information("Estimating FWHM by observation renders a better result.");
      bestR = r2;
    }
    setFunctionParameterValue(peak, bestR.getPeakParameters());
    setFunctionParameterValue(bkgdfunc, bestR.getBackgroundParameters());

    // Is it a failed fit?
    double finalrwp = bestR.getChiSquare();
    bool fitfail = false;
    if (finalrwp > DBL_MAX-1.0)
      fitfail = true;

    // Set up parameters
    std::vector<double> params, rawparams;
    if (m_rawPeaksTable)
    {
      // Set up raw parameter table
      for (size_t i = 0; i < peak->nParams(); ++i)
      {
        double parvalue = peak->getParameter(i);
        rawparams.push_back(parvalue);
      }

      // Background.  nParams may be fewer than 3
      for (size_t i = 0; i < bkgdfunc->nParams(); ++i)
      {
        double parvalue = bkgdfunc->getParameter(i);
        rawparams.push_back(parvalue);
      }
      for (size_t i = bkgdfunc->nParams(); i < 3; ++i)
        rawparams.push_back(0.0);
    }
    else
    {
      // Set up parameter table as centre, width, height, A0, A1, A2
      double centre = peak->centre();
      double width = peak->fwhm();
      double height = peak->height();
      params.push_back(centre);
      params.push_back(width);
      params.push_back(height);

      // Background.  nParams may be fewer than 3
      for (size_t i = 0; i < bkgdfunc->nParams(); ++i)
      {
        double parvalue = bkgdfunc->getParameter(i);
        params.push_back(parvalue);
      }
      for (size_t i = bkgdfunc->nParams(); i < 3; ++i)
        params.push_back(0.0);
    }

    double peakwidth = peak->fwhm();
    // Use negative Rwp for super wide peak (apparently a false one)
    if (peakwidth > windowsize)
      finalrwp = -1*finalrwp;

    // Set outpout information
    addInfoRow(spectrum, params, rawparams, finalrwp, fitfail);

    // Add function to list
    if (!fitfail)
    {
      API::CompositeFunction_sptr fitFunction(new API::CompositeFunction());
      fitFunction->addFunction(peak);
      fitFunction->addFunction(bkgdfunc);
      addFittedFunction(fitFunction, imin, imax);
    }

    return;
  }


  //----------------------------------------------------------------------------------------------
  /** Find the index of the best Rwp in a vector for Rwps
    * @param vecRwp :: vector of double for Rwp
    */
  int FindPeaks::getBestResult(std::vector<double> vecRwp)
  {
    // Find best Rwp
    int bestrwpindex = -1;
    double bestrwp = DBL_MAX;
    for (int i = 0; i < static_cast<int>(vecRwp.size()); ++i)
    {
      if (vecRwp[i] < bestrwp)
      {
        bestrwp = vecRwp[i];
        bestrwpindex = i;
      }
    }

    return bestrwpindex;
  }

  //----------------------------------------------------------------------------------------------
  /** make boundary/contraint string on peak's centre
    * @param peak :: Functon to put boundary on
    * @param peakleftboundary :: left boundary of peak centre
    * @param peakrightboundary :: right boundary of peak centre
    * @param composite :: indicate whether peak is a pure peak function or a composite function
    */
  std::string FindPeaks::makePeakCentreConstraint(IFunction_sptr peak, double peakleftboundary,
                                                  double peakrightboundary, bool composite)
  {
    // FIXME - This only works with Gaussian because the name convension

    std::vector<std::string> parnames = peak->getParameterNames();
    bool usex0 = false;
    bool usecentre = false;
    for (size_t i = 0; i < parnames.size(); ++i)
      if (parnames[i].compare("X0") == 0)
        usex0 = true;
      else if (parnames[i].compare("PeakCentre") == 0)
        usecentre = true;

    // FIXME - This is a
    std::string centrename;
    if (usex0)
    {
      centrename = "X0";
    }
    else if (usecentre)
    {
      centrename = "PeakCentre";
    }
    else
    {
      g_log.warning() << "Peak function of type " << peak->name() << " has unsupported name for peak centre.\n";
      std::stringstream namess;
      for (size_t i = 0; i < parnames.size(); ++i)
      {
        namess << parnames[i] << ", ";
      }
      g_log.warning(namess.str());
    }

    std::stringstream bcss;
    if (usex0 || usecentre)
    {
      bcss << peakleftboundary << " < ";
      if (composite)
        bcss << "f0.";
      bcss << centrename << " < " << peakrightboundary;
    }

    return bcss.str();
  }



  //----------------------------------------------------------------------------------------------
  /** Estimate peak parameters
    * Assumption: pure peak workspace with background removed (but it might not be true...)
    * @param vecX :: vector of X-axis
    * @param vecY :: vector of Y-axis
    * @param i_min :: start
    * @param i_max :: end
    * @param centre :: estimated peak centre (maximum position)
    * @param height :: maximum
    * @param fwhm :: 2 fwhm
    * @param error :: reason for estimating peak parameters error.
    */
  bool FindPeaks::estimatePeakParameters(const MantidVec& vecX, const MantidVec& vecY,
                                         size_t i_min, size_t i_max, double& centre, double& height, double& fwhm,
                                         std::string& error)
  {
    // Search for maximum
    size_t icentre = i_min;
    centre = vecX[i_min];
    double highest = vecY[i_min];
    double lowest = vecY[i_min];
    for (size_t i = i_min+1; i <= i_max; ++i)
    {
      double y = vecY[i];
      if (y > highest)
      {
        icentre = i;
        centre = vecX[i];
        highest = y;
      }
      else if (y < lowest)
      {
        lowest = y;
      }
    }

    height = highest - lowest;
    if (height == 0)
    {
      error = "Flat spectrum";
      return false;
    }
    else if (height <= m_minHeight)
    {
      error = "Fluctuation is less than minimum allowed value.";
      return false;
    }

    // If maximum point is on the edge 2 points, return false.  One side of peak must have at least 3 points
    if (icentre <= i_min+1 || icentre >= i_max-1)
    {
      g_log.debug() << "Maximum value between " << vecX[i_min] << " and " << vecX[i_max] << " is located on "
             << "X = " << vecX[icentre] << "(" << icentre << ")." << "\n";
      for (size_t i = i_min; i <= i_max; ++i)
        g_log.debug() << vecX[i] << "\t\t" << vecY[i] << ".\n";

      error = "Maximum value on edge";
      return false;
    }

    // Search for half-maximum: no need to very precise

    // Slope at the left side of peak.
    double leftfwhm = -1;
    for (int i = static_cast<int>(icentre)-1; i >= 0; --i)
    {
      double yright = vecY[i+1];
      double yleft = vecY[i];
      if (yright-lowest > 0.5*height && yleft-lowest <= 0.5*height)
      {
        // Ideal case
        leftfwhm = centre - 0.5*(vecX[i] + vecX[i+1]);
      }
    }

    // Slope at the right side of peak
    double rightfwhm = -1;
    for (size_t i = icentre+1; i <= i_max; ++i)
    {
      double yleft = vecY[i-1];
      double yright = vecY[i];
      if (yleft-lowest > 0.5*height && yright-lowest <= 0.5*height)
      {
        rightfwhm = 0.5*(vecX[i] + vecX[i-1]) - centre;
      }
    }

    if (leftfwhm <= 0 || rightfwhm <= 0)
    {
      std::stringstream errmsg;
      errmsg << "Estimate peak parameters error (FWHM cannot be zero): Input data size = " << vecX.size()
             << ", Xmin = " << vecX[i_min] << "(" << i_min << "), Xmax = " << vecX[i_max] << "(" << i_max << "); "
             << "Estimated peak centre @ " << vecX[icentre] << "(" << icentre << ") with height = " << height
             << "; Lowest Y value = " << lowest
             << "; Output error: .  leftfwhm = " << leftfwhm << ", right fwhm = " << rightfwhm << ".\n";
      /*
      for (size_t i = i_min; i <= i_max; ++i)
        errmsg << vecX[i] << "\t\t" << vecY[i] << ".\n";
        */
      error = errmsg.str();
      g_log.warning(error);
      return false;
    }

    fwhm = leftfwhm + rightfwhm;

    g_log.debug() << "Estimated peak parameters: Centre = " << centre << ", Height = "
                  << height << ", FWHM = " << fwhm << ".\n";

    return true;
  }


  //----------------------------------------------------------------------------------------------
  /** Estimate linear background
    * @param X :: vec for X
    * @param Y :: vec for Y
    * @param i_min :: index of minimum in X to estimate background
    * @param i_max :: index of maximum in X to estimate background
    * @param out_bg0 :: interception
    * @param out_bg1 :: slope
    * @param out_bg2 :: a2 = 0
    */
  void FindPeaks::estimateLinearBackground(const MantidVec& X, const MantidVec& Y, const size_t i_min, const size_t i_max,
                                           double& out_bg0, double& out_bg1, double& out_bg2)
  {
    // Validate input
    if (i_min >= i_max)
      throw std::runtime_error("i_min cannot larger or equal to i_max");

    // FIXME - THIS IS A MAGIC NUMBER
    const size_t MAGICNUMBER = 12;
    size_t numavg;
    if (i_max - i_min > MAGICNUMBER)
      numavg = 3;
    else
      numavg = 1;

    // Get (x0, y0) and (xf, yf)
    double x0, y0, xf, yf;

    x0 = 0.0;
    y0 = 0.0;
    xf = 0.0;
    yf = 0.0;
    for (size_t i = 0; i < numavg; ++i)
    {
      x0 += X[i_min+i];
      y0 += Y[i_min+i];

      xf += X[i_max-i];
      yf += Y[i_max-i];
    }
    x0 = x0 / static_cast<double>(numavg);
    y0 = y0 / static_cast<double>(numavg);
    xf = xf / static_cast<double>(numavg);
    yf = yf / static_cast<double>(numavg);

    // g_log.debug() << "[F1145] Spec = " << specdb << "(X0, Y0) = " << x0 << ", " << y0 << "; (Xf, Yf) = "
    //              << xf << ", " << yf << ". (Averaged from " << numavg << " background points.)" << "\n";

    // Esitmate
    out_bg2 = 0.;
    out_bg1 = (y0-yf)/(x0-xf);
    out_bg0 = (xf*y0-x0*yf)/(xf-x0);

    m_backgroundFunction->setParameter("A0", out_bg0);
    if (m_backgroundFunction->nParams() > 1)
    {
      m_backgroundFunction->setParameter("A1", out_bg1);
      if (m_backgroundFunction->nParams() > 2)
        m_backgroundFunction->setParameter("A2", 0.0);
    }

    g_log.debug() << "Estimated background: A0 = " << out_bg0 << ", A1 = "
                        << out_bg1 << ".\n";

    return;
  }

  //----------------------------------------------------------------------------------------------
  /** Estimate flat background
    * @param Y :: vec for Y
    * @param i_min :: index of minimum in X to estimate background
    * @param i_max :: index of maximum in X to estimate background
    * @param out_bg0 :: interception
    * @param out_bg1 :: a1 = 0
    * @param out_bg2 :: a2 = 0
    */
  void FindPeaks::estimateFlatBackground(const MantidVec& Y, const size_t i_min, const size_t i_max,
                                         double& out_bg0, double& out_bg1, double& out_bg2)
  {
    // Validate input
    if (i_min >= i_max)
      throw std::runtime_error("i_min cannot larger or equal to i_max");

    // FIXME - THIS IS A MAGI NUMBER
    const size_t MAGICNUMBER = 12;
    size_t numavg;
    if (i_max - i_min > MAGICNUMBER)
      numavg = 3;
    else
      numavg = 1;

    // Get (x0, y0) and (xf, yf)
    double y0, yf;

    y0 = 0.0;
    yf = 0.0;
    for (size_t i = 0; i < numavg; ++i)
    {
      y0 += Y[i_min+i];
      yf += Y[i_max-i];
    }
    y0 = y0 / static_cast<double>(numavg);
    yf = yf / static_cast<double>(numavg);

    // Esitmate
    out_bg2 = 0.;
    out_bg1 = 0.;
    out_bg0 = 0.5*(y0 + yf);

    m_backgroundFunction->setParameter("A0", out_bg0);
    if (m_backgroundFunction->nParams() > 1)
    {
      m_backgroundFunction->setParameter("A1", 0.0);
      if (m_backgroundFunction->nParams() > 2)
        m_backgroundFunction->setParameter("A2", 0.0);
    }

    g_log.debug() << "Estimated flat background: A0 = " << out_bg0 << ".\n";

    return;
  }


  //----------------------------------------------------------------------------------------------
  /** Add the fit record (failure) to output workspace
    * @param spectrum :: spectrum where the peak is
    */
  void FindPeaks::addNonFitRecord(const size_t spectrum)
  {
    // Add a new row
    API::TableRow t = m_outPeakTableWS->appendRow();

    // 1st column
    t << static_cast<int>(spectrum);

    // Parameters
    for (std::size_t i = 0; i < m_numTableParams; i++)
      t << 0.;

    // HUGE chi-square
    t << DBL_MAX;

    return;
  }


  //----------------------------------------------------------------------------------------------
  /** Add a row to the output table workspace.
    * @param spectrum number
    * @param params The effective peak/background parameters
    * @param rawParams The raw peak/background parameters
    * @param mincost Chi2 value for this set of parameters
    * @param error Whether or not the fit ended in an error.
    */
  void FindPeaks::addInfoRow(const size_t spectrum, const std::vector<double> &params,
                             const std::vector<double> &rawParams, const double mincost, bool error)
  {
    API::TableRow t = m_outPeakTableWS->appendRow();
    t << static_cast<int>(spectrum);

    // Is bad fit?
    bool isbadfit;
    if (error)
      isbadfit = true;
    else if (m_rawPeaksTable && rawParams.empty())
      isbadfit = true;
    else if (!m_rawPeaksTable && params.size() < 4)
      isbadfit = true;
    else
      isbadfit = false;

    if (isbadfit)    // Bad fit
    {
      g_log.warning() << "No Good Fit Obtained! Chi2 = " << mincost << ". ";
      g_log.warning() << "Possible reason: (1) Fit error = " << error << ", (2) params.size = " << params.size()
            << ", (3) rawParams.size():" << rawParams.size() << ". (Output with raw parameter = "
            << m_rawPeaksTable << ").";

      for (std::size_t i = 0; i < m_numTableParams; i++)
        t << 0.;
      t << 1.e10; // bad chisq value
    }
    else    // Good fit
    {
      if (m_rawPeaksTable)
      {
        for (std::vector<double>::const_iterator it = rawParams.begin(); it != rawParams.end(); ++it)
        {
          t << (*it);
          g_log.debug() << (*it) << " ";
        }
      }
      else
      {
        for (std::vector<double>::const_iterator it = params.begin(); it != params.end(); ++it)
        {
          t << (*it);
          g_log.debug() << (*it) << " ";
        }
      }

      t << mincost;
      g_log.debug() << "Chi2 = " << mincost << "\n";
    }
  }

  //----------------------------------------------------------------------------------------------
  /** Get the parameter lists as appropriate using the supplied function abstraction.
    * @param compositeFunc The function to get information from.
    * @param effParams This will always be centre, width, height, backA0, backA1, backA2 reguarless of how many
    * parameters the function actually has.
    * @param rawParams The actual parameters of the fit function.
    */
  void getComponentFunctions(IFunction_sptr compositeFunc, std::vector<double> &effParams,
                             std::vector<double> &rawParams)
  {
    // clear out old parameters
    effParams.clear();
    rawParams.clear();

    // convert the input into a composite function
    boost::shared_ptr<CompositeFunction> composite = boost::dynamic_pointer_cast<CompositeFunction>(
          compositeFunc);
    if (!composite)
      throw std::runtime_error("Cannot update parameters from non-composite function");

    // dump out the raw parameters
    for (std::size_t i = 0; i < composite->nParams(); i++)
    {
      rawParams.push_back(composite->getParameter(i));
    }

    // get the effective peak parameters
    effParams.resize(6);
    boost::shared_ptr<IPeakFunction> peakFunc;
    IFunction_sptr backFunc;
    for (std::size_t i = 0; i < composite->nFunctions(); i++)
    {
      auto func = composite->getFunction(i);
      if (dynamic_cast<IPeakFunction *>(func.get()))
      {
        peakFunc = boost::dynamic_pointer_cast<IPeakFunction>(func);
      }
      else if (dynamic_cast<IFunction *>(func.get()))
      {
        backFunc = boost::dynamic_pointer_cast<IFunction>(func);
      }
      // else fall through
    }
    if (peakFunc)
    {
      effParams[0] = peakFunc->centre();
      effParams[1] = peakFunc->fwhm();
      effParams[2] = peakFunc->height();
    }
    if (backFunc)
    {
      for (std::size_t i = 0; i < backFunc->nParams(); i++)
      {
        effParams[3 + i] = backFunc->getParameter(i);
      }
    }

    return;
  }

  //----------------------------------------------------------------------------------------------
  /** Check the GSL fit status message to determine whether the fit is successful or not
    * @param fitstatus :: string of status returned from Fit
    */
  bool FindPeaks::isFitSuccessful(std::string fitstatus)
  {
    // Additional message other than 'success'
    bool allowedfailure = (fitstatus.find("cannot") < fitstatus.size())
        && (fitstatus.find("tolerance") < fitstatus.size());

    // Success or 'cannot...tolerance'?
    bool isfitgood;
    if (fitstatus.compare("success") == 0 || allowedfailure)
    {
      isfitgood = true;
    }
    else
    {
      isfitgood = false;
      g_log.debug() << "Fit Status = " << fitstatus << ".  Not to update fit result" << std::endl;
    }

    return isfitgood;
  }

  //----------------------------------------------------------------------------------------------
  /** Check the results of the fit algorithm to see if they make sense and update the best parameters.
    * @param fitAlg :: algorithm object
    * @param bestEffparams :: vector of double for best effective parameters
    * @param bestRarparams :: vector double for raw function parameters
    * @param mincost :: chi square
    * @param expPeakPos :: double as expected peak position
    * @param expPeakHeight :: double as expected peak height
    */
  void FindPeaks::updateFitResults(API::IAlgorithm_sptr fitAlg, std::vector<double> &bestEffparams,
                                   std::vector<double> &bestRawparams, double &mincost, const double expPeakPos,
                                   const double expPeakHeight)
  {
    // check the results of the fit status
    std::string fitStatus = fitAlg->getProperty("OutputStatus");
    bool isfitgood = isFitSuccessful(fitStatus);
    if (!isfitgood)
      return;

    // check that chi2 got better
    const double chi2 = fitAlg->getProperty("OutputChi2overDoF");
    g_log.debug() << "Fit Status = " << fitStatus << ", chi2 = " << chi2 << std::endl;
    if (chi2 > mincost)
    {
      return;
    }

    // get out the parameter names
    std::vector<double> tempEffectiveParams, tempRawParams;
    getComponentFunctions(fitAlg->getProperty("Function"), tempEffectiveParams, tempRawParams);

    // check the height
    double height = tempEffectiveParams[2];
    if (height <= 0)
    { // Height must be strictly positive
      g_log.debug() << "Fitted height = " << height << ".  It is a wrong fit!" << "\n";
      return;
    }

    // check the height tolerance
    if (m_usePeakHeightTolerance && height > expPeakHeight * m_peakHeightTolerance)
    {
      g_log.debug() << "Failed peak height tolerance test\n";
      return;
    }

    // check the peak position tolerance
    if (m_usePeakPositionTolerance
        && fabs(tempEffectiveParams[0] - expPeakPos) > m_peakPositionTolerance)
    {
      g_log.debug() << "Faile peak position tolerance test\n";
      return;
    }

    // check for NaNs
    for (std::vector<double>::const_iterator it = tempEffectiveParams.begin();
         it != tempEffectiveParams.end(); ++it)
    {
      if ((*it) != (*it))
      {
        g_log.debug() << "NaN detected in the results of peak fitting. Peak ignored.\n";
        return;
      }
    }
    for (std::vector<double>::const_iterator it = tempRawParams.begin(); it != tempRawParams.end();
         ++it)
    {
      if ((*it) != (*it))
      {
        g_log.debug() << "NaN detected in the results of peak fitting. Peak ignored.\n";
        return;
      }
    }

    // all the checks passed, update the parameters
    mincost = chi2;
    bestEffparams.assign(tempEffectiveParams.begin(), tempEffectiveParams.end());
    bestRawparams.assign(tempRawParams.begin(), tempRawParams.end());
  }

  //----------------------------------------------------------------------------------------------
  /** Create a background function
    * @param a0, a1, a2  Variables dependent on background order.
    */
  IFunction_sptr FindPeaks::createBackgroundFunction(const double a0, const double a1, const double a2)
  {
    // FIXME  Need to have a uniformed routine to name background function
    std::string backgroundposix("");
    if (m_backgroundType.compare("Quadratic"))
    {
      backgroundposix = "Background";
    }
    auto background = API::FunctionFactory::Instance().createFunction(
          m_backgroundType + backgroundposix);
    int order = this->getBackgroundOrder();
    background->setParameter("A0", a0);
    if (order > 0)
    {
      background->setParameter("A1", a1);
      if (order > 1)
        background->setParameter("A2", a2);
    }

    return background;
  }



  //----------------------------------------------------------------------------------------------
  /** Create a function for fitting.
    * @param height Height
    * @param centre Centre
    * @param sigma Sigma
    * @param a0, a1, a2  Variables dependent on background order.
    * @param withPeak If this is set to false then return only a background function.
    * @return The requested function to fit.
    */
  IFunction_sptr FindPeaks::createFunction(const double height, const double centre,
                                           const double sigma, const double a0, const double a1, const double a2,
                                           const bool withPeak)
  {
    // setup the background
    // FIXME  Need to have a uniformed routine to name background function
    std::string backgroundposix("");
    if (m_backgroundType.compare("Quadratic"))
    {
      backgroundposix = "Background";
    }
    auto background = API::FunctionFactory::Instance().createFunction(
          m_backgroundType + backgroundposix);
    int order = this->getBackgroundOrder();
    background->setParameter("A0", a0);
    if (order > 0)
    {
      background->setParameter("A1", a1);
      if (order > 1)
        background->setParameter("A2", a2);
    }

    // just return the background if there is no need for a peak
    if (!withPeak)
    {
      return background;
    }

    // setup the peak
    auto tempPeakFunc = API::FunctionFactory::Instance().createFunction(m_peakFuncType);
    auto peakFunc = boost::dynamic_pointer_cast<IPeakFunction>(tempPeakFunc);
    peakFunc->setHeight(height);
    peakFunc->setCentre(centre);
    peakFunc->setFwhm(sigma);

    // put the two together and return
    CompositeFunction* fitFunc = new CompositeFunction();
    fitFunc->addFunction(peakFunc);
    fitFunc->addFunction(background);

    return boost::shared_ptr<IFunction>(fitFunc);
  }

  //----------------------------------------------------------------------------------------------
  /** @return The order of the polynomial for the bacground fit.
    */
  int FindPeaks::getBackgroundOrder()
  {
    if (m_backgroundType.compare("Linear") == 0)
      return 1;
    else if (m_backgroundType.compare("Quadratic") == 0)
      return 2;
    else
      return 0;
  }

  //----------------------------------------------------------------------------------------------
  /** Calulate a function with given data range, and its goodness of fit, Rwp.
    * Warning: use flat standard error 1.0 for fitting PEAKS
    * @param function :: function object
    * @param dataws :: data workspace
    * @param wsindex :: workspace index in data workspace for function to fit
    * @param startx :: x min
    * @param endx :: x max
    */
  double FindPeaks::calculateFunctionRwp(IFunction_sptr function, MatrixWorkspace_sptr dataws,
                                         size_t wsindex, double startx, double endx)
  {
    // Construct a new vector
    const MantidVec& vecX = dataws->readX(wsindex);
    const MantidVec& vecY = dataws->readY(wsindex);

    std::vector<double>::const_iterator beginX, endX, beginY, endY;
    beginX = std::lower_bound(vecX.begin(), vecX.end(), startx);
    endX = std::lower_bound(vecX.begin(), vecX.end(), endx);
    beginY = vecY.begin() + static_cast<int>(beginX-vecX.begin());
    endY = vecY.begin() + static_cast<int>(endX-vecX.begin());

    std::vector<double> partX, partY;
    partX.assign(beginX, endX);
    partY.assign(beginY, endY);

    // Calculate function
    FunctionDomain1DVector domain(partX);
    FunctionValues values(domain);
    function->function(domain, values);

    // Calculate Rwp
    double sumnom = 0;
    double sumdenom = 0;
    double sumrpnom = 0;
    double sumrpdenom = 0;

    size_t numpts = domain.size();
    double cal_i;
    double obs_i;
    double sigma;
    double weight;
    double diff;
    for (size_t i = 0; i < numpts; ++i)
    {
      cal_i = values[i];
      obs_i = partY[i];
      sigma = 1.0;
      weight = 1.0/(sigma*sigma);
      diff = obs_i - cal_i;

      sumrpnom += fabs(diff);
      sumrpdenom += fabs(obs_i);

      sumnom += weight*diff*diff;
      sumdenom += weight*obs_i*obs_i;
    }

    // double rp = (sumrpnom/sumrpdenom);
    double rwp = std::sqrt(sumnom/sumdenom);

    return rwp;
  }

  //----------------------------------------------------------------------------------------------
  /** Add fitted function to a vector of all peak functions that are fitted
    * @param fitfunction :: function to get recorded
    * @param ileft :: index of peak's left boundary point in input workspace
    * @param iright :: index of peak's right boundary point in input workspace
    */
  void FindPeaks::addFittedFunction(IFunction_sptr fitfunction, size_t ileft, size_t iright)
  {
    IFunction_sptr copyfunc = createFunction(0., 0., 0., 0., 0., 0., true);
    std::vector<std::string> funparnames = fitfunction->getParameterNames();
    for (size_t i = 0; i < funparnames.size(); ++i)
    {
      copyfunc->setParameter(funparnames[i], fitfunction->getParameter(funparnames[i]));
    }
    m_fitFunctions.push_back(copyfunc);

    m_peakLeftIndexes.push_back(ileft);
    m_peakRightIndexes.push_back(iright);

    return;
  }

  //----------------------------------------------------------------------------------------------
  /** Fit for background by creating a workspace excluding peak region
    * Assumption: m_backgroundFunction has been set up with reasonable starting value for
    *             background order parameters
    * startX = in_centre - windowSize;
    * endX = in_centre + windowSize;
    * fitHightbackground(backgroundFunction, newX, newY, newE, startx, endx,
    * @return :: chi-square
    */
  bool FindPeaks::fitBackground(const MantidVec& X, const MantidVec& Y, const MantidVec& E,
                                size_t ileft, size_t iright, size_t imin, size_t imax,
                                double& chi2)
  {
    // Store original information
    size_t numparams = m_backgroundFunction->nParams();
    double in_bg0 = m_backgroundFunction->getParameter("A0");
    double in_bg1 = 0.;
    if (numparams >= 2)
      in_bg1 = m_backgroundFunction->getParameter("A1");
    double in_bg2 = 0.;
    if (numparams >= 3)
      in_bg2 = m_backgroundFunction->getParameter("A2");

    g_log.debug() << "F1120 Fit background Input: " << "A0 = " << in_bg0 << ", A1 = " << in_bg1
                  << ", A2 = " << in_bg2 << ".  Range = " << X[ileft] << ", " << X[iright] << ".\n";

    // Construct a workspace to fit for background.  The region within fit window is removed
    std::vector<double> newX, newY, newE;
    for (size_t i = imin; i <= imax; i++)
    {
      if (i > size_t(iright) || i < size_t(ileft))
      {
        newX.push_back(X[i]);
        newY.push_back(Y[i]);
        newE.push_back(E[i]);
      }
    }
    size_t numpts = newX.size();

    if (numpts < 3)
    {
      g_log.warning() << "Size of workspace to fit for background = " << newX.size()
                      << ". It is too small to proceed. ";
      g_log.warning() << "Input i_min = " << imin << ",i_max = " << imax << ", i_left = " << ileft
                      << ", i_right = " << iright;

      return false;
    }

    // Construct a background data workspace for fit
    MatrixWorkspace_sptr bkgdWS =
        API::WorkspaceFactory::Instance().create("Workspace2D", 1, newX.size(), newY.size());

    MantidVec& wsX = bkgdWS->dataX(0);
    MantidVec& wsY = bkgdWS->dataY(0);
    MantidVec& wsE = bkgdWS->dataE(0);
    for (size_t i = 0; i < newY.size(); i++)
    {
      wsX[i] = newX[i];
      wsY[i] = newY[i];
      wsE[i] = newE[i];
    }

    // Fit range
    double startx = newX[0];
    double endx = newX.back();

    g_log.debug() << "Background Type = " << m_backgroundType << "  Function: "
                  << m_backgroundFunction->asString() << "  StartX = "
                  << startx << " EndX = " << endx << ".\n";

    // Set up the background fitting
    IAlgorithm_sptr fit;
    try
    {
      fit = createChildAlgorithm("Fit", -1, -1, true);
    }
    catch (Exception::NotFoundError &)
    {
      std::stringstream errss;
      errss << "The StripPeaks algorithm requires the CurveFitting library";
      g_log.error(errss.str());
      throw std::runtime_error(errss.str());
    }

    fit->setProperty("Function", m_backgroundFunction);
    fit->setProperty("InputWorkspace", bkgdWS);
    fit->setProperty("WorkspaceIndex", 0);
    fit->setProperty("MaxIterations", 50);
    fit->setProperty("StartX", startx);
    fit->setProperty("EndX", endx);
    fit->setProperty("Minimizer", m_minimizer);
    fit->setProperty("CostFunction", "Least squares");

    // Execute fit and get result of fitting background
    fit->executeAsChildAlg();
    if (!fit->isExecuted())
    {
      g_log.error("Fit for background is not executed. ");
      throw std::runtime_error("Fit for background is not executed. ");
    }

    std::string fitStatus = fit->getProperty("OutputStatus");
    m_backgroundFunction = fit->getProperty("Function");

    g_log.debug() << "(HighBackground) Fit Background Function.  Fit Status = " << fitStatus
                  << std::endl;

    // Check fiting status
    bool allowedfailure = fitStatus.find("cannot") > 0 && fitStatus.find("tolerance") > 0;

    double bkgdchi2;
    if (fitStatus.compare("success") == 0 || allowedfailure)
    {
      // good fit assumed
      bkgdchi2 = fit->getProperty("OutputChi2overDoF");
    }
    else
    {
      // set background to zero background
      // Create background function
      m_backgroundFunction->setParameter("A0", in_bg0);
      if (numparams >= 2)
        m_backgroundFunction->setParameter("A1", in_bg1);
      if (numparams >= 3)
        m_backgroundFunction->setParameter("A2", in_bg2);

      bkgdchi2 = DBL_MAX;
    }

    chi2 = bkgdchi2;

    return true;
  }

  //----------------------------------------------------------------------------------------------
  /** Fit a peak (and background) function
    *
    * @param peakbkgdfunc :: peak+background function (composite)
    * @param dataws :: data workspace with data to fit
    * @param wsindex :: workspace index of the spectrum
    * @param startx :: x range
    * @param endx :: x range
    * @param constraint :: constraint in format of string
    * @param init_rwp :: starting rwp
    * @return RWP
    */
  double FindPeaks::fitPeakBackgroundFunction(IFunction_sptr peakbkgdfunc,
                                              MatrixWorkspace_sptr dataws, size_t wsindex,
                                              double startx, double endx, std::string constraint, double& init_rwp)
  {
    // FIXME - Constraint is not used for better performance at this moment.
    UNUSED_ARG(constraint);

    if (!peakbkgdfunc)
      throw std::runtime_error("FitPeakBackgroundFunction has a null peak input.");
    else
      g_log.debug() << "Function (to fit): " << peakbkgdfunc->asString() << "  From "
                    << startx << "  to " << endx << ".\n";

    std::stringstream dbss;
    dbss << "Fit data workspace spectrum " << wsindex << ".  Parameters: ";
    std::vector<std::string> comparnames = peakbkgdfunc->getParameterNames();
    for (size_t i = 0; i < comparnames.size(); ++i)
      dbss << comparnames[i] << ", ";
    g_log.debug(dbss.str());

    // Starting chi-square
    init_rwp = calculateFunctionRwp(peakbkgdfunc, dataws, wsindex, startx, endx);

    // Create Child Algorithm Fit
    IAlgorithm_sptr gfit;
    try
    {
      gfit = createChildAlgorithm("Fit", -1, -1, true);
    }
    catch (Exception::NotFoundError &)
    {
      g_log.error("The FindPeaks algorithm requires the CurveFitting library");
      throw std::runtime_error("The FindPeaks algorithm requires the CurveFitting library");
    }

    // Set up fit
    gfit->setProperty("Function", peakbkgdfunc);
    gfit->setProperty("InputWorkspace", dataws);
    gfit->setProperty("WorkspaceIndex", static_cast<int>(wsindex));
    gfit->setProperty("MaxIterations", 50);
    gfit->setProperty("StartX", startx);
    gfit->setProperty("EndX", endx);
    //if (constraint.size() > 0)
    // gfit->setProperty("Constraints", constraint);
    gfit->setProperty("Minimizer", m_minimizer);
    gfit->setProperty("CostFunction", "Least squares");

    // Fit
    gfit->executeAsChildAlg();
    if (!gfit->isExecuted())
    {
      g_log.error("Fit is not executed correctly.");
      return DBL_MAX;
    }
    else
    {
      g_log.debug("Fit is executed. ");
    }

    // Analyze result
    std::string fitpeakstatus = gfit->getProperty("OutputStatus");
    bool isfitgood = isFitSuccessful(fitpeakstatus);

    double final_rwp;
    if (isfitgood)
    {
      final_rwp = calculateFunctionRwp(peakbkgdfunc, dataws, wsindex, startx, endx);

      std::stringstream dbss;

      std::vector<std::string> parnames = peakbkgdfunc->getParameterNames();
      dbss << "[Fx357] Fit Peak (+background) Status = " << fitpeakstatus << ". Starting Rwp = "
           << init_rwp << ".  Final Rwp = " << final_rwp << ".\n";
      for (size_t i = 0; i < parnames.size(); ++i)
        dbss << parnames[i] << "\t = " << peakbkgdfunc->getParameter(parnames[i]) << "\n";
      g_log.debug(dbss.str());
    }
    else
    {
      final_rwp = DBL_MAX;
    }

    return final_rwp;
  }


} // namespace Algorithms
} // namespace Mantid


// 0.5044, 0.5191, 0.535, 0.5526, 0.5936, 0.6178, 0.6453, 0.6768, 0.7134, 0.7566, 0.8089, 0.8737, 0.9571, 1.0701, 1.2356, 1.5133, 2.1401
