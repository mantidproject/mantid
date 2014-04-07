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

#include "MantidAlgorithms/FitPeak.h"
#include "MantidAPI/CostFunctionFactory.h"
#include "MantidAPI/FunctionFactory.h"
#include "MantidAPI/FuncMinimizerFactory.h"
#include "MantidAPI/TableRow.h"
#include "MantidAPI/WorkspaceValidators.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/StartsWithValidator.h"
#include "MantidKernel/VectorHelper.h"

// #include "MantidAPI/CompositeFunction.h"
// #include "MantidAPI/ConstraintFactory.h"

#include <boost/algorithm/string.hpp>
#include <iostream>
#include <numeric>
#include "MantidKernel/BoundedValidator.h"
#include "MantidKernel/ListValidator.h"
#include "MantidAPI/TableRow.h"

#include <fstream>

using namespace Mantid;
using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace Mantid::DataObjects;

// const double MINHEIGHT = 2.00000001;

namespace Mantid
{
namespace Algorithms
{

  // Register the algorithm into the AlgorithmFactory
  DECLARE_ALGORITHM(FindPeaks)

  //----------------------------------------------------------------------------------------------
  /** Constructor
    */
  FindPeaks::FindPeaks() : API::Algorithm(), m_progress(NULL)
  {
    m_minimizer = "Levenberg-MarquardtMD";
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

    auto mustBeNonNegative = boost::make_shared<BoundedValidator<int> >();
    mustBeNonNegative->setLower(0);
    declareProperty("WorkspaceIndex", EMPTY_INT(), mustBeNonNegative,
                    "If set, only this spectrum will be searched for peaks (otherwise all are)");

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

    std::vector<std::string> peakNames = FunctionFactory::Instance().getFunctionNames<API::IPeakFunction>();
    declareProperty("PeakFunction", "Gaussian", boost::make_shared<StringListValidator>(peakNames));

    std::vector<std::string> bkgdtypes;
    bkgdtypes.push_back("Flat");
    bkgdtypes.push_back("Linear");
    bkgdtypes.push_back("Quadratic");
    declareProperty("BackgroundType", "Linear", boost::make_shared<StringListValidator>(bkgdtypes),
                    "Type of Background.");


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

    std::vector<std::string> costFuncOptions;
    costFuncOptions.push_back("Chi-Square");
    costFuncOptions.push_back("Rwp");
    declareProperty("CostFunction","Chi-Square",
                    Kernel::IValidator_sptr(new Kernel::ListValidator<std::string>(costFuncOptions)),
                    "Cost functions");

    std::vector<std::string> minimizerOptions = API::FuncMinimizerFactory::Instance().getKeys();

#if 0
    g_log.notice() << "Total " << minimizerOptions.size() << " minimzer types" << "\n";
    for (size_t i = 0; i < minimizerOptions.size(); ++i)
      g_log.notice() << "minimzer : " << minimizerOptions[i] << "\n";
#endif

    declareProperty("Minimizer", "Levenberg-MarquardtMD",
                    Kernel::IValidator_sptr(new Kernel::StartsWithValidator(minimizerOptions)),
                    "Minimizer to use for fitting. Minimizers available are \"Levenberg-Marquardt\", \"Simplex\","
                    "\"Conjugate gradient (Fletcher-Reeves imp.)\", \"Conjugate gradient (Polak-Ribiere imp.)\", \"BFGS\", and \"Levenberg-MarquardtMD\"");


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
    createFunctions();

    // Set up output table workspace
    generateOutputPeakParameterTable();

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
      findPeaksGivenStartingPoints(m_vecPeakCentre, m_vecFitWindows);
    }
    else
    {
      //Use Mariscotti's method to find the peak centers
      m_usePeakPositionTolerance = false;
      m_usePeakHeightTolerance = false;
      this->findPeaksUsingMariscotti();
    }

    // Set output properties
    g_log.information() << "Total " << m_outPeakTableWS->rowCount()
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

    // About Fit
    m_minimizer = getPropertyValue("Minimizer");
    m_costFunction = getPropertyValue("CostFunction");

    return;
  }

  //----------------------------------------------------------------------------------------------
  /** Generate a table workspace for output peak parameters
    */
  void FindPeaks::generateOutputPeakParameterTable()
  {
    m_outPeakTableWS = WorkspaceFactory::Instance().createTable("TableWorkspace");
    m_outPeakTableWS->addColumn("int", "spectrum");
    if (m_rawPeaksTable)
    {
      size_t numpeakpars = m_peakFunction->nParams();
      size_t numbkgdpars = m_backgroundFunction->nParams();
      for (size_t i = 0; i < numpeakpars; ++i)
        m_outPeakTableWS->addColumn("double", m_peakParameterNames[i]);
      for (size_t i = 0; i < numbkgdpars; ++i)
        m_outPeakTableWS->addColumn("double", m_bkgdParameterNames[i]);
      // m_outPeakTableWS->addColumn("double", "f1.A2");
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
        infoss <<  "Peak @ d = " << x_center;
        if (useWindows)
        {
          infoss << " inside fit window [" << fitwindows[2 * i] << ", " << fitwindows[2 * i + 1] << "]";
        }
        g_log.information(infoss.str());

        // Check whether it is the in data range
        if (x_center > datax.front() && x_center < datax.back())
        {
          if (useWindows)
            fitPeakInWindow(m_dataWS, spec, x_center, fitwindows[2 * i], fitwindows[2 * i + 1]);
          else
            fitPeakGivenFWHM(m_dataWS, spec, x_center, m_inputPeakFWHM);
        }
        else
        {
            g_log.warning() << "Given peak centre " << x_center << " is out side of given data's range ("
                << datax.front() << ", " << datax.back() << ").\n";
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

          this->fitSinglePeak(m_dataWS, k, i_min, i_max, i4);

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

    if (x <= vecX.front())
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
    if (leftIndex < 0 || leftIndex >= static_cast<int>(Y.size()))
      throw std::runtime_error("Left index out of boundary");

    if (rightIndex < 0 || rightIndex > static_cast<int>(Y.size()))
    {
      std::stringstream errss;
      errss << "In getMaxHeightIndex, Size(Y) = " << Y.size() << ", Left Index = " << leftIndex
            << ", Right Index = " << rightIndex;
      throw std::runtime_error(errss.str());
    }

    int right_index = rightIndex;
    if (right_index == static_cast<int>(Y.size()))
      right_index -= 1;

    double maxY = Y[leftIndex];
    int indexMax = leftIndex;
    for (int i = leftIndex + 1; i < right_index; i++)
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
    *  @param fitWidth :: A guess of the full-width-half-max of the peak, in # of bins.
    */
  void FindPeaks::fitPeakGivenFWHM(const API::MatrixWorkspace_sptr &input, const int spectrum,
                          const double center_guess, const int fitWidth)
  {
    g_log.information() << "Fit peak with guessed FWHM:  starting center = " << center_guess
                        << ", FWHM = " << fitWidth << ".\n";

    // The X axis you are looking at
    const MantidVec &vecX = input->readX(spectrum);
    const MantidVec &vecY = input->readY(spectrum);

    // Find i_center - the index of the center - The guess is within the X axis?
    int i_centre = this->getVectorIndex(vecX, center_guess);

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

    fitSinglePeak(input, spectrum, i_min, i_max, i_centre);

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
      g_log.error() << "Peak centre is on the edge of Fit window\n";
      addNonFitRecord(spectrum);
      return;
    }

    //The X axis you are looking at
    const MantidVec &vecX = input->readX(spectrum);

    //The centre index
    int i_centre = this->getVectorIndex(vecX, centre_guess);

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

    // finally do the actual fit
    // throw std::runtime_error("Debug Stop:  Need to check whether the parameters given are correct. ");
    g_log.warning("XXX Need to find out whether the input, such as i_min, i_max and i_centre are correct.");
    fitSinglePeak(input, spectrum, i_min, i_max, i_centre);

    return;
  }


  //----------------------------------------------------------------------------------------------
  /** Fit a single peak
    * This is the fundametary peak fit function used by all kinds of input
    */
  void FindPeaks::fitSinglePeak(const API::MatrixWorkspace_sptr &input, const int spectrum,
                                const int i_min, const int i_max, const int i_centre)
  {
    //-------------------------------------------------------------------------
    // Estimate peak and background parameters for better fitting
    //-------------------------------------------------------------------------

    // Estimate background
    std::vector<double> vec_bkgdparvalues0, vecpeakrange;
    findPeakBackground(input, spectrum, i_min, i_max, vec_bkgdparvalues0, vecpeakrange);

    // Estimate peak parameters
    double est_centre, est_height, est_fwhm;
    const MantidVec& vecX = input->readX(spectrum);
    const MantidVec& vecY = input->readY(spectrum);
    std::string errmsg = estimatePeakParameters(vecX, vecY, i_min, i_max, est_centre, est_height, est_fwhm);
    if (errmsg.size() > 0)
    {
      // Unable to estimate peak
      est_centre = vecX[i_centre];
      est_fwhm = 1.;
      est_height = 1.;
      g_log.error(errmsg);
    }
    else
    {
      for (size_t i = 0; i < vec_bkgdparvalues0.size(); ++i)
        m_backgroundFunction->setParameter(i, vec_bkgdparvalues0[i]);
    }

    // Set peak parameters to
    m_peakFunction->setCentre(est_centre);
    m_peakFunction->setHeight(est_height);
    m_peakFunction->setFwhm(est_fwhm);

#if 0
    std::vector<double> vec_peakparvalues0 = getStartingPeakValues();
#else

#endif

    //-------------------------------------------------------------------------
    // Fit Peak
    //-------------------------------------------------------------------------
    std::vector<double> fitwindow(2);
    fitwindow[0] = vecX[i_min];
    fitwindow[1] = vecX[i_max];
    // std::vector<double> vec_fittedpeakparvalues, vec_fittedbkgdparvalues;
#if 0
    double costfuncvalue = callFitPeak(input, spectrum, vec_peakparvalues0, vec_bkgdparvalues0, fitwindow, vecpeakrange,
                                       m_minGuessedPeakWidth, m_maxGuessedPeakWidth, m_stepGuessedPeakWidth,
                                       vec_fittedpeakparvalues, vec_fittedbkgdparvalues);
#else
    double costfuncvalue = callFitPeak(input, spectrum, m_peakFunction, m_backgroundFunction, fitwindow,
                                       vecpeakrange, m_minGuessedPeakWidth, m_maxGuessedPeakWidth, m_stepGuessedPeakWidth);


        /*callFitPeak(input, spectrum,  m_peakFunction, m_backgroundFunction, fitwindow, vecpeakrange,
                                       m_minGuessedPeakWidth, m_maxGuessedPeakWidth, m_stepGuessedPeakWidth);*/
#endif
    bool fitsuccess = false;
    if (costfuncvalue < DBL_MAX && costfuncvalue >= DBL_MIN)
      fitsuccess = true;

    //-------------------------------------------------------------------------
    // Process Fit result
    //-------------------------------------------------------------------------
    // Update output
    if (fitsuccess)
      addInfoRow(spectrum, m_peakFunction, m_backgroundFunction, m_rawPeaksTable, costfuncvalue);
    else
      addNonFitRecord(spectrum);

    return;
  }

  //----------------------------------------------------------------------------------------------
  /** make boundary/contraint string on peak's centre
    * @param peak :: Functon to put boundary on
    * @param peakleftboundary :: left boundary of peak centre
    * @param peakrightboundary :: right boundary of peak centre
    * @param composite :: indicate whether peak is a pure peak function or a composite function
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
    */


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
  std::string FindPeaks::estimatePeakParameters(const MantidVec& vecX, const MantidVec& vecY,
                                          size_t i_min, size_t i_max, double& centre, double& height, double& fwhm)
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
      return "Flat spectrum";
    }
    else if (height <= m_minHeight)
    {
      return "Fluctuation is less than minimum allowed value.";
    }

    // If maximum point is on the edge 2 points, return false.  One side of peak must have at least 3 points
    if (icentre <= i_min+1 || icentre >= i_max-1)
    {
      g_log.debug() << "Maximum value between " << vecX[i_min] << " and " << vecX[i_max] << " is located on "
             << "X = " << vecX[icentre] << "(" << icentre << ")." << "\n";
      for (size_t i = i_min; i <= i_max; ++i)
        g_log.debug() << vecX[i] << "\t\t" << vecY[i] << ".\n";

      return "Maximum value on edge";
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
      return errmsg.str();
    }

    fwhm = leftfwhm + rightfwhm;
    if (fwhm < 1.e-200) // very narrow peak
    {
      std::stringstream errmsg;
      errmsg << "Estimate peak parameters error (FWHM cannot be zero): Input data size = " << vecX.size()
             << ", Xmin = " << vecX[i_min] << "(" << i_min << "), Xmax = " << vecX[i_max] << "(" << i_max << "); "
             << "Estimated peak centre @ " << vecX[icentre] << "(" << icentre << ") with height = " << height
             << "; Lowest Y value = " << lowest
             << "; Output error: .  fwhm = " << fwhm;
      return errmsg.str();
    }

    g_log.debug() << "Estimated peak parameters: Centre = " << centre << ", Height = "
                  << height << ", FWHM = " << fwhm << ".\n";

    return std::string();
  }


  //----------------------------------------------------------------------------------------------
  /** Estimate background
    * @param X :: vec for X
    * @param Y :: vec for Y
    * @param i_min :: index of minimum in X to estimate background
    * @param i_max :: index of maximum in X to estimate background
    * @param out_bg0 :: interception
    * @param out_bg1 :: slope
    * @param out_bg2 :: a2 = 0
    */
  void FindPeaks::estimateBackground(const MantidVec& X, const MantidVec& Y, const size_t i_min, const size_t i_max,
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
    if (m_backgroundFunction->nParams() > 1) // linear background
    {
      out_bg1 = (y0-yf)/(x0-xf);
      out_bg0 = (xf*y0-x0*yf)/(xf-x0);
    }
    else // flat background
    {
      out_bg1 = 0.;
      out_bg0 = 0.5*(y0 + yf);
    }

    m_backgroundFunction->setParameter("A0", out_bg0);
    if (m_backgroundFunction->nParams() > 1)
    {
      m_backgroundFunction->setParameter("A1", out_bg1);
      if (m_backgroundFunction->nParams() > 2)
        m_backgroundFunction->setParameter("A2", out_bg2);
    }

    g_log.debug() << "Estimated background: A0 = " << out_bg0 << ", A1 = "
                        << out_bg1 << ", A2 = " << out_bg2 << "\n";

    return;
  }

  //----------------------------------------------------------------------------------------------
  /** Add a row to the output table workspace.
    * @param spectrum number
    * @param params The effective peak/background parameters
    * @param rawParams The raw peak/background parameters
    * @param mincost Chi2 value for this set of parameters
    * @param error Whether or not the fit ended in an error.
    *       addInfoRow(spectrum, vec_fittedpeakparvalues, vec_fittedbkgdparvalues, m_rawPeaksTable, costfuncvalue, fitsuccess);
    * std::vector<double> &peakparams,
     *                       const std::vector<double> &bkgdparams,
    */
  void FindPeaks::addInfoRow(const size_t spectrum, const API::IPeakFunction_const_sptr& peakfunction,
                             const API::IBackgroundFunction_sptr& bkgdfunction,
                             const bool isoutputraw,
                             const double mincost)
  {
    // Check input validity
    if (mincost < DBL_MIN || mincost >= DBL_MAX-1.0E-10)
      throw std::runtime_error("Minimum cost indicates that fit fails.  This method should not be called "
                               "under this circumstance. ");

#if 0
    if (isoutputraw && (peakparams.size() != m_peakParameterNames.size()))
      throw std::runtime_error("AddInfoRow Error.  Fit is successful.  But input number "
                               "of peak parameters' value is wrong. ");

    if (isoutputraw && (bkgdparams.size() != m_bkgdParameterNames.size()))
      throw std::runtime_error("AddInfoRow Error.  Fit is successful.  But input number "
                               "of peak parameters' value is wrong. ");
#endif

    // Add fitted parameters to output table workspace
    API::TableRow t = m_outPeakTableWS->appendRow();
    t << static_cast<int>(spectrum);

    if (isoutputraw)
    {
      // Output of raw peak parameters
#if 0
      for (std::vector<double>::const_iterator it = peakparams.begin(); it != peakparams.end(); ++it)
      {
        t << (*it);
        g_log.information()<< "Add peak parameter: " << (*it) << "\n";
      }
      for (std::vector<double>::const_iterator it = bkgdparams.begin(); it != bkgdparams.end(); ++it)
      {
        t << (*it);
        g_log.information() << (*it) << " ";
      }
#endif
      size_t nparams = peakfunction->nParams();
      for (size_t i = 0; i < nparams; ++i)
      {
        t << peakfunction->getParameter(i);
      }
      nparams = bkgdfunction->nParams();
      for (size_t i = 0; i < nparams; ++i)
      {
        t << bkgdfunction->getParameter(i);
      }
    }
    else
    {
      // Output of effective peak parameters
      // Set up parameters to peak function and get effective value
      // for (size_t i = 0; i < m_peakParameterNames.size(); ++i)
      //   m_peakFunction->setParameter(m_peakParameterNames[i], peakparams[i]);

      double peakcentre = peakfunction->centre();
      double fwhm = peakfunction->fwhm();
      double height = peakfunction->height();

      t << peakcentre << fwhm << height;

      // Set up parameters to background function
      // for (size_t i = 0; i < m_bkgdParameterNames.size(); ++i)
      // m_backgroundFunction->setParameter(m_bkgdParameterNames[i], bkgdparams[i]);

      g_log.notice() << "[DBXXA] Background type = " << m_backgroundFunction->name() << "\n";

      // FIXME - Use Polynomial for all 3 background types.
      double a0 = bkgdfunction->getParameter("A0");
      double a1 = 0.;
      if (bkgdfunction->name() != "FlatBackground")
        a1 = bkgdfunction->getParameter("A1");
      double a2 = 0;
      if (bkgdfunction->name() != "LinearBackground" && bkgdfunction->name() != "FlatBackground")
        a2 = bkgdfunction->getParameter("A2");

      t << a0 << a1 << a2;

      /*
      m_numTableParams = 6;
      m_outPeakTableWS->addColumn("double", "centre");
      m_outPeakTableWS->addColumn("double", "width");
      m_outPeakTableWS->addColumn("double", "height");
      m_outPeakTableWS->addColumn("double", "backgroundintercept");
      m_outPeakTableWS->addColumn("double", "backgroundslope");
      m_outPeakTableWS->addColumn("double", "A2");
      */
#if 0
      for (std::vector<double>::const_iterator it = params.begin(); it != params.end(); ++it)
      {
        t << (*it);
        g_log.debug() << (*it) << " ";
      }
#endif
    }

    t << mincost;
    g_log.debug() << m_costFunction << " = " << mincost << "\n";


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
  /** Get the parameter lists as appropriate using the supplied function abstraction.
    * @param compositeFunc The function to get information from.
    * @param effParams This will always be centre, width, height, backA0, backA1, backA2 reguarless of how many
    * parameters the function actually has.
    * @param rawParams The actual parameters of the fit function.
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
  */

  //----------------------------------------------------------------------------------------------
  /** Create functions and related variables
    * @param height Height
    * @param centre Centre
    * @param sigma Sigma
    * @param a0, a1, a2  Variables dependent on background order.
    * @param withPeak If this is set to false then return only a background function.
    * @return The requested function to fit.
    */
  void FindPeaks::createFunctions()
  {
    // Setup the background
    // FIXME (No In This Ticket)  Need to have a uniformed routine to name background function
    std::string backgroundposix("");
    if (m_backgroundType.compare("Quadratic"))
    {
      // FlatBackground, LinearBackground, Quadratic
      backgroundposix = "Background";
    }
    g_log.information() << "About to create background function. " << "\n";
    m_backgroundFunction = boost::dynamic_pointer_cast<IBackgroundFunction>(API::FunctionFactory::Instance().createFunction(
          m_backgroundType + backgroundposix));
    g_log.information() << "Background function (" << m_backgroundFunction->name() << ") has been created. " << "\n";

    m_bkgdParameterNames = m_backgroundFunction->getParameterNames();
    m_bkgdOrder = m_backgroundFunction->nParams()-1;

    // Set up peak function
    m_peakFunction = boost::dynamic_pointer_cast<IPeakFunction>(
          API::FunctionFactory::Instance().createFunction(m_peakFuncType));
    m_peakParameterNames = m_peakFunction->getParameterNames();

    return;
  }

  //----------------------------------------------------------------------------------------------
  /** @return The order of the polynomial for the bacground fit.

  int FindPeaks::getBackgroundOrder()
  {
    if (m_backgroundType.compare("Linear") == 0)
      return 1;
    else if (m_backgroundType.compare("Quadratic") == 0)
      return 2;
    else
      return 0;
  }
    */

  //----------------------------------------------------------------------------------------------
  /** Find peak background given a certain range
    */
  void FindPeaks::findPeakBackground(const MatrixWorkspace_sptr& input, int spectrum, size_t i_min, size_t i_max,
                                     std::vector<double>& vecBkgdParamValues, std::vector<double>& vecpeakrange)
  {
    double bg0, bg1, bg2;

    const MantidVec& vecX = input->readX(spectrum);
    const MantidVec& vecY = input->readY(spectrum);

    // Call FindPeakBackground
    IAlgorithm_sptr estimate = createChildAlgorithm("FindPeakBackground");
    estimate->setProperty("InputWorkspace", input);
    // The workspace index
    std::vector<int> wivec;
    wivec.push_back(spectrum);
    estimate->setProperty("WorkspaceIndices", wivec);
    //estimate->setProperty("SigmaConstant", 1.0);
    // The workspace index
    std::vector<double> fwvec;
    fwvec.push_back(vecX[i_min]);
    fwvec.push_back(vecX[i_max]);
    estimate->setProperty("BackgroundType", m_backgroundType);
    estimate->setProperty("FitWindow", fwvec);
    estimate->executeAsChildAlg();
    // Get back the result
    Mantid::API::ITableWorkspace_sptr pealisttablews = estimate->getProperty("OutputWorkspace");

    // Determine whether to use FindPeakBackground's result.
    size_t i_peakmin, i_peakmax;
    bool usefitresult = false;
    if (pealisttablews->rowCount() > 0)
    {
      int useit = pealisttablews->Int(0, 6);
      if (useit > 0)
        usefitresult = true;
    }

    if (usefitresult)
    {
      // Use FitPeakBackgroud's reuslt
      g_log.information() << "Background fitting successful. " << " peak talbe i_x = " << pealisttablews->Int(0,1)
                          << "\n";

      // i_peakmin: must be in range. or use default
      if(pealisttablews->Int(0,1) >= static_cast<int>(i_min))
      {
        i_peakmin = pealisttablews->Int(0,1);
        if (i_peakmin > i_max)
          i_peakmin = i_min;
      }
      else
        i_peakmin = i_min;

      // i_peakmax
      if(pealisttablews->Int(0,2) >= static_cast<int>(i_min))
      {
        i_peakmax = pealisttablews->Int(0,2);
        if (i_peakmax > i_max)
          i_peakmax = i_max;
      }
      else
        i_peakmax = i_max;

      // background parameters
      bg0 = pealisttablews->Double(0,3);
      bg1 = pealisttablews->Double(0,4);
      bg2 = pealisttablews->Double(0,5);
    }
    else
    {
      g_log.information() << "Background fitting failed. " << "\n";
      // If FindPeakBackground failed!
      i_peakmin = i_min;
      i_peakmax = i_max;
      // Estimate background roughly for a failed case
      estimateBackground(vecX, vecY, i_min, i_max, bg0, bg1, bg2);
      g_log.notice() << "[DBXXW] type = " << m_backgroundType << ", spectrum " << spectrum << ", i_min = " << i_min
                     << ", Range is " << vecX[i_min] << ", " << vecX[i_max] << "\n";
    }

    g_log.notice() << "[DBXXX] imin = " << i_min << ", imax = " << i_max << ", vector X size = "
                   << vecX.size() << ", i_peakmin = " << i_peakmin << ", ipeakmax = " << i_peakmax << "\n";

    // Set output
    vecpeakrange.resize(2);
    vecpeakrange[0] = vecX[i_peakmin];
    vecpeakrange[1] = vecX[i_peakmax];

    vecBkgdParamValues.resize(m_bkgdOrder+1);
    vecBkgdParamValues[0] = bg0;
    if (m_bkgdOrder >= 1)
    {
      vecBkgdParamValues[1] = bg1;
      if (m_bkgdOrder >= 2)
        vecBkgdParamValues[2] = bg2;
    }

    g_log.information() << "Number of background parameter values = " << vecBkgdParamValues.size()
                        << ", background order = " << m_bkgdOrder << "\n";
    g_log.warning("Need to find out how the background esitmated.");

    return;
  }

  /**
    * const std::vector<double>& vec_peakparvalues0,
    * const std::vector<double>& vec_bkgdparvalues0,
    * std::vector<double>& vec_fittedpeakparvalues,
    * std::vector<double>& vec_fittedbkgdparvalues
    */
  double FindPeaks::callFitPeak(const MatrixWorkspace_sptr& dataws, int wsindex,
                                const API::IPeakFunction_sptr peakfunction,
                                const API::IBackgroundFunction_sptr backgroundfunction,
                                const std::vector<double>& vec_fitwindow,
                                const std::vector<double>& vec_peakrange, int minGuessFWHM, int maxGuessFWHM,
                                int guessedFWHMStep)
  {
#if 0
    IAlgorithm_sptr fitpeak = createChildAlgorithm("FitPeak");
    fitpeak->initialize();

    fitpeak->setProperty("InputWorkspace", dataws);
    fitpeak->setProperty("WorkspaceIndex", wsindex);
    fitpeak->setProperty("PeakFunctionType", m_peakFuncType);
    fitpeak->setProperty("PeakParameterNames", m_peakParameterNames);
    fitpeak->setProperty("PeakParameterValues", vec_peakparvalues0);
    fitpeak->setProperty("BackgroundType", m_backgroundType);
    fitpeak->setProperty("BackgroundParameterNames", m_bkgdParameterNames);
    fitpeak->setProperty("BackgroundParameterValues", vec_bkgdparvalues0);
    fitpeak->setProperty("FitWindow", vec_fitwindow);
    fitpeak->setProperty("PeakRange", vec_peakrange);
    fitpeak->setProperty("FitBackgroundFirst", m_highBackground);
    fitpeak->setProperty("RawParams", m_rawPeaksTable);
    fitpeak->setProperty("MinGuessedPeakWidth", minGuessedFWHM);
    fitpeak->setProperty("MaxGuessedPeakWidth", maxGuessFWHM);
    fitpeak->setProperty("GuessedPeakWidthStep", guessedFWHMStep);
    fitpeak->setProperty("PeakPositionTolerance",m_peakPositionTolerance);
    fitpeak->setProperty("CostFunction", m_costFunction);
    fitpeak->setProperty("Minimizer", m_minimizer);

    fitpeak->execute();
    if (!fitpeak->isExecuted())
      throw std::runtime_error("FitPeak fails to execute");

    vec_fittedpeakparvalues = fitpeak->getProperty("FittedPeakParameterValues");
    vec_fittedbkgdparvalues = fitpeak->getProperty("FittedBackgroundParameterValues");

    double costfuncvalue = fitpeak->getProperty("CostFunctionValue");
#else
    g_log.information("F1150 Fit 1 single peak.");
    double userFWHM = m_peakFunction->fwhm();
    bool fitwithsteppedfwhm = (guessedFWHMStep == 0);

    FitOneSinglePeak fitpeak;
    fitpeak.setWorskpace(dataws, wsindex);
    fitpeak.setFitWindow(vec_fitwindow[0], vec_fitwindow[1]);
    fitpeak.setFittingMethod(m_minimizer, m_costFunction);
    fitpeak.setFunctions(peakfunction, backgroundfunction);
    fitpeak.setupGuessedFWHM(userFWHM, minGuessFWHM, maxGuessFWHM, guessedFWHMStep, fitwithsteppedfwhm);
    fitpeak.setPeakRange(vec_peakrange[0], vec_peakrange[1]);

    if (m_highBackground)
      fitpeak.highBkgdFit();
    else
      fitpeak.simpleFit();

    double costfuncvalue = fitpeak.getFitCostFunctionValue();
#endif

    return costfuncvalue;
  }

  /** Get the peak parameter values from m_peakFunction and output to a list in the same
    * order of m_peakParameterNames
    */
  std::vector<double> FindPeaks::getStartingPeakValues()
  {
    size_t numpeakpars = m_peakFunction->nParams();
    std::vector<double> vec_value(numpeakpars);
    for (size_t i = 0; i < numpeakpars; ++i)
    {
      double parvalue = m_peakFunction->getParameter(i);
      vec_value[i] = parvalue;
    }

    return vec_value;
  }


} // namespace Algorithms
} // namespace Mantid


// 0.5044, 0.5191, 0.535, 0.5526, 0.5936, 0.6178, 0.6453, 0.6768, 0.7134, 0.7566, 0.8089, 0.8737, 0.9571, 1.0701, 1.2356, 1.5133, 2.1401
