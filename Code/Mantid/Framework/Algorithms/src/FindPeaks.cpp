/*WIKI*
 This algorithm searches the specified spectra in a workspace for peaks, returning a list of the found and successfully fitted peaks. The search algorithm is described in full in reference [1]. In summary: the second difference of each spectrum is computed and smoothed. This smoothed data is then searched for patterns consistent with the presence of a peak. The list of candidate peaks found is passed to a fitting routine and those that are successfully fitted are kept and returned in the output workspace (and logged at information level).

 The output [[TableWorkspace]] contains the following columns, which reflect the fact that the peak has been fitted to a Gaussian atop a linear background: spectrum, centre, width, height, backgroundintercept & backgroundslope.

 ====Subalgorithms used====
 FindPeaks uses the [[SmoothData]] algorithm to, well, smooth the data - a necessary step to identify peaks in statistically fluctuating data. The [[Fit]] algorithm is used to fit candidate peaks.

 ====Treating weak peaks vs. high background====
 FindPeaks uses a more complicated approach to fit peaks if '''HighBackground''' is flagged. In this case, FindPeak will fit the background first, and then do a Gaussian fit the peak with the fitted background removed.  This procedure will be repeated for a couple of times with different guessed peak widths.  And the parameters of the best result is selected.  The last step is to fit the peak with a combo function including background and Gaussian by using the previously recorded best background and peak parameters as the starting values.

 ==== Criteria To Validate Peaks Found ====
 FindPeaks finds peaks by fitting a Guassian with background to a certain range in the input histogram.  [[Fit]] may not give a correct result even if chi^2 is used as criteria alone.  Thus some other criteria are provided as options to validate the result
 1. Peak position.  If peak positions are given, and trustful, then the fitted peak position must be within a short distance to the give one.
 2. Peak height.  In the certain number of trial, peak height can be used to select the best fit among various starting sigma values.

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
// #include "MantidCurveFitting/BoundaryConstraint.h"
#include <boost/algorithm/string.hpp>
#include <iostream>
#include <numeric>
#include "MantidKernel/BoundedValidator.h"
#include "MantidKernel/ListValidator.h"

#include <fstream>

using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace Mantid::DataObjects;

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

    declareProperty(new WorkspaceProperty<Workspace2D>("OutputWorkspace", "", Direction::Output, PropertyMode::Optional),
                    "Name of the output workspace containing original data and fitted peaks.");

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
#if 1
    std::vector<std::string> tablecolnames = m_outPeakTableWS->getColumnNames();
    for (size_t i = 0; i < tablecolnames.size(); ++i)
      g_log.information() << "Table column " << i << ": " << tablecolnames[i] << ".\n";
#endif

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

    std::string outwsname = getPropertyValue("OutputWorkspace");
    g_log.information() << "Output workspace name is " << outwsname << " (length = "
                        << outwsname.size() << "). " << "\n";
    if (outwsname.size() > 0)
    {
      API::MatrixWorkspace_sptr outWS = createOutputDataWorkspace();
      setProperty("OutputWorkspace", outWS);
    }
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
    if (t1 > t2)
    {
      std::stringstream errss;
      errss << "User specified minimum guessed peak with (" << t1 << ") is greater than "
            << "maximum guessed peak width (" << t2 << ").";
      g_log.error(errss.str());
      throw std::runtime_error(errss.str());
    }

    minGuessedPeakWidth = static_cast<unsigned int>(t1);
    maxGuessedPeakWidth = static_cast<unsigned int>(t2);
    stepGuessedPeakWidth = static_cast<unsigned int>(t3);

    m_peakPositionTolerance = getProperty("PeakPositionTolerance");
    m_usePeakPositionTolerance = true;
    if (m_peakPositionTolerance == EMPTY_DBL())
      m_usePeakPositionTolerance = false;

    m_peakHeightTolerance = getProperty("PeakHeightTolerance");
    m_usePeakHeightTolerance = true;
    if (m_peakHeightTolerance == EMPTY_DBL())
      m_usePeakHeightTolerance = false;

    // b) Get the specified peak positions, which is optional
    m_vecPeakCentre = getProperty("PeakPositions");
    m_vecFitWindows = getProperty("FitWindows");

    // c) Peak and Background
    m_peakFuncType = getPropertyValue("PeakFunction");
    m_backgroundType = getPropertyValue("BackgroundType");

    // d) Choice of fitting approach
    m_highBackground = getProperty("HighBackground");

    // Peak parameters are give via a table workspace
    m_rawPeaksTable = getProperty("RawPeakParameters");

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
            fitPeak(m_dataWS, spec, x_center, fitwindows[2 * i], fitwindows[2 * i + 1]);
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

          this->fitPeak(m_dataWS, k, i0, i2, i4);

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
      // static_cast<int>(getLowerBound(vecX, 0, vecX.size() - 1, x));

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

    //The indices
    int i_left, i_right, i_center;

    //The X axis you are looking at
    const MantidVec &X = input->readX(spectrum);

    // 1. find i_center - the index of the center - The guess is within the X axis?
    i_center = this->getVectorIndex(X, center_guess);

    // 2. Determine the fitting range X[]
    i_left = i_center - FWHM_guess / 2;
    if (i_left < 0)
    {
      i_left = 0;
    }
    i_right = i_left + FWHM_guess;
    if (i_right >= static_cast<int>(X.size()))
    {
      i_right = static_cast<int>(X.size()) - 1;
    }

    g_log.debug() << "FindPeaks.fitPeak(): Fitting range = " << X[i_left] << ",  " << X[i_right]
                  << std::endl;

    this->fitPeak(input, spectrum, i_right, i_left, i_center);

    return;

  }

  //----------------------------------------------------------------------------------------------
  /** Attempts to fit a candidate peak with a given window of where peak resides
    *
    *  @param input    The input workspace
    *  @param spectrum The spectrum index of the peak (is actually the WorkspaceIndex)
    *  @param centre_guess ::  Channel number of peak candidate i0 - the higher side of the peak (right side)
    *  @param left     Channel number of peak candidate i2 - the lower side of the peak (left side)
    *  @param right    Channel number of peak candidate i4 - the center of the peak
    */
  void FindPeaks::fitPeak(const API::MatrixWorkspace_sptr &input, const int spectrum,
                          const double centre_guess, const double left, const double right)
  {
    g_log.information() << "Fit Peak with given window:  Guessed center = " << centre_guess << "  left = " << left
                        << ", right = " << right << "\n";

    //The X axis you are looking at
    const MantidVec &X = input->readX(spectrum);

    //The centre index
    int i_centre = this->getVectorIndex(X, centre_guess);

    //The left index
    int i_left;
      if (left < X.front())
      {
        i_left = 0;
      }
      else
      {
        i_left = static_cast<int>(getLowerBound(X, 0, X.size() - 1, left));
      }
      if (i_left > i_centre)
      {
        i_left = i_centre - 1;
        if (i_left < 0)
          i_left = 0;
      }

      //The right index
      int i_right;
      if (right > X.back())
      {
        i_right = static_cast<int>(X.size() - 1);
      }
      else
      {
        i_right = static_cast<int>(getLowerBound(X, 0, X.size() - 1, right));
      }
      if (i_right < i_centre)
      {
        i_right = i_centre + 1;
        if (i_right > static_cast<int>(X.size() - 1))
          i_right = static_cast<int>(X.size() - 1);
      }

      // look for the heigh point
      if (m_searchPeakPos)
        i_centre = getMaxHeightIndex(input->readY(spectrum), i_left, i_right);

      // finally do the actual fit
      this->fitPeak(input, spectrum, i_right, i_left, i_centre);

      return;
    }

  //----------------------------------------------------------------------------------------------
  /** Attempts to fit a candidate peak
    *  This is the core fitPeak() to call in the fitPeak hierarchy
    *
    *  @param input ::    The input workspace
    *  @param spectrum :: The spectrum index of the peak (is actually the WorkspaceIndex)
    *  @param i0 ::       Channel number of peak candidate i0 - the higher side of the peak (right side)
    *  @param i2 ::       Channel number of peak candidate i2 - the lower side of the peak (left side)
    *  @param i4 ::       Channel number of peak candidate i4 - the center of the peak
    */
  void FindPeaks::fitPeak(const API::MatrixWorkspace_sptr &input, const int spectrum, const int i0,
                          const int i2, const int i4)
  {
    const MantidVec &X = input->readX(spectrum);
    const MantidVec &Y = input->readY(spectrum);

    g_log.debug() << "Fit Peak @ " << X[i4] << "  of Spectrum " << spectrum << "  Peak In Range "
                  << X[i2] << ", " << X[i0] << "  [i0,i2,i4]=[" << i0 << "," << i2 << "," << i4 << "]\n";

    // Get the initial estimate of the width, in # of bins
    const int fitWidth = i0 - i2;

    // See Mariscotti eqn. 20. Using l=1 for bg0/bg1 - correspond to p6 & p7 in paper.
    unsigned int i_min = 1;
    if (i0 > static_cast<int>(5 * fitWidth))
      i_min = i0 - 5 * fitWidth;
    unsigned int i_max = i0 + 5 * fitWidth;
    // Bounds checks
    if (i_min < 1)
      i_min = 1;
    if (i_max >= Y.size() - 1)
      i_max = static_cast<unsigned int>(Y.size() - 2); // TODO this is dangerous

    g_log.debug() << "Background + Peak -- Bounds = " << X[i_min] << ", " << X[i_max] << std::endl;

    // Estimate height, boundary, and etc for fitting
    const double bg_lowerSum = Y[i_min - 1] + Y[i_min] + Y[i_min + 1];
    const double bg_upperSum = Y[i_max - 1] + Y[i_max] + Y[i_max + 1];
    double in_bg0 = (bg_lowerSum + bg_upperSum) / 6.0;
    double in_bg1 = (bg_upperSum - bg_lowerSum) / (3.0 * (i_max - i_min + 1));
    double in_bg2 = 0.0;

    // TODO max guessed width = 10 is good for SNS.  But it may be broken in extreme case
    if (!m_highBackground)
    {

      /** Not high background.  Fit background and peak together
         *  The original Method
         */
      fitPeakOneStep(input, spectrum, i0, i2, i4, in_bg0, in_bg1, in_bg2);

    } // // not high background
    else
    {

      /** High background
         **/

      fitPeakHighBackground(input, spectrum, i0, i2, i4, i_min, i_max, in_bg0, in_bg1, in_bg2);

    } // if high background

    g_log.debug() << "Fit Peak Over" << std::endl;

    return;
  }

  //----------------------------------------------------------------------------------------------
  /** Fit 1 peak in one step, i.e., one function combining both Gaussian and background
    *
    * @param  X, Y, Z: MantidVec&
    * @param i0: bin index of right end of peak
    * @param i2: bin index of left end of peak
    * @param i4: bin index of center of peak
    * @param i_min: bin index of left bound of fit range
    * @param i_max: bin index of right bound of fit range
    * @param in_bg0: guessed value of a0
    * @param in_bg1: guessed value of a1
    * @param in_bg2: guessed value of a2
    * @param backgroundtype: type of background (linear or quadratic)
    */
  void FindPeaks::fitPeakOneStep(const API::MatrixWorkspace_sptr &input, const int spectrum,
                                 const int& i0, const int& i2, const int& i4, const double& in_bg0, const double& in_bg1,
                                 const double& in_bg2)
  {
    g_log.information("Fitting Peak in one-step approach");

    const MantidVec &X = input->readX(spectrum);
    const MantidVec &Y = input->readY(spectrum);

    const double in_height = Y[i4] - in_bg0;
    const double in_centre = input->isHistogramData() ? 0.5 * (X[i4] + X[i4 + 1]) : X[i4];

    double mincost = 1.0E10;
    std::vector<double> bestparams, bestRawParams;

    // 1. Loop around
    for (unsigned int width = minGuessedPeakWidth; width <= maxGuessedPeakWidth; width +=
         stepGuessedPeakWidth)
    {

      // a) Set up Child Algorithm Fit
      IAlgorithm_sptr fit;
      try
      {
        // Fitting the candidate peaks to a Gaussian
        fit = createChildAlgorithm("Fit", -1, -1, true);
      } catch (Exception::NotFoundError &)
      {
        g_log.error("The StripPeaks algorithm requires the CurveFitting library");
        throw;
      }

      // b) Guess sigma
      const double in_sigma = (i0 + width < X.size()) ? X[i0 + width] - X[i0] : 0.;
      IFunction_sptr fitFunction = this->createFunction(in_height, in_centre, in_sigma, in_bg0, in_bg1,
                                                        in_bg2, true);
      g_log.debug() << "  Function: " << fitFunction->asString() << "; Background Type = "
                    << m_backgroundType << std::endl;

      // d) complete fit
      double windowSize = 5. * fabs(X[i0] - X[i2]);
      g_log.debug() << "  Window: " << (in_centre - windowSize) << " to " << (in_centre + windowSize)
                    << "\n";
      fit->setProperty("Function", fitFunction);
      fit->setProperty("InputWorkspace", input);
      fit->setProperty("WorkspaceIndex", spectrum);
      fit->setProperty("MaxIterations", 50);
      fit->setProperty("StartX", (in_centre - windowSize)); //(X[i0] - 5 * (X[i0] - X[i2])));
      fit->setProperty("EndX", (in_centre + windowSize)); //(X[i0] + 5 * (X[i0] - X[i2])));
      fit->setProperty("Minimizer", "Levenberg-Marquardt");
      fit->setProperty("CostFunction", "Least squares");

      // e) Fit and get result
      fit->executeAsChildAlg();

      this->updateFitResults(fit, bestparams, bestRawParams, mincost, in_centre, in_height);

    } // ENDFOR: Loop over "width"

    // Update output
    if (bestparams.size() > 1)
      addInfoRow(spectrum, bestparams, bestRawParams, mincost,
             (bestparams[0] < X.front() || bestparams[0] > X.back()));
    else
      addInfoRow(spectrum, bestparams, bestRawParams, mincost, true);

    // Update collection of peaks
    IFunction_sptr fitFunction = createFunction(0.0, 0.0, 0.0, 0.0, 0.0, 0.0, true);
    for (size_t i = 0; i < bestRawParams.size(); ++i)
      fitFunction->setParameter(i, bestRawParams[i]);
    addFittedFunction(fitFunction, i2, i0);

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
    * @param ileft: bin index of right end of peak
    * @param iright: bin index of left end of peak
    * @param icentre: bin index of center of peak
    * @param i_min: bin index of left bound of fit range
    * @param i_max: bin index of right bound of fit range
    * @param in_bg0: guessed value of a0
    * @param in_bg1: guessed value of a1
    * @param in_bg2: guessed value of a2
    */
  void FindPeaks::fitPeakHighBackground(const API::MatrixWorkspace_sptr &input, const int spectrum,
                                        const int& iright, const int& ileft, const int& icentre,
                                        const unsigned int& i_min, const unsigned int& i_max,
                                        double& in_bg0, double& in_bg1, double& in_bg2)
  {
    g_log.information() << "Fitting a peak assumed at " << input->dataX(spectrum)[icentre]
                        << " by high-background approach\n";

    // Prepare
    const MantidVec &X = input->readX(spectrum);
    const MantidVec &Y = input->readY(spectrum);

    // Estimate linear background: output-> m_backgroundFunction
    estimateLinearBackground(X, Y, i_min, i_max, in_bg0, in_bg2, in_bg2);

    // Create a pure peak workspace (Workspace2D)
    size_t numpts = i_max-i_min+1;
    API::MatrixWorkspace_sptr peakws =
        API::WorkspaceFactory::Instance().create("Workspace2D", 1, numpts, numpts);

    //    set up x-axis first
    MantidVec& dataX = peakws->dataX(0);
    MantidVec& dataY = peakws->dataY(0);
    MantidVec& dataE = peakws->dataE(0);
    for (size_t i = 0; i < numpts; ++i)
    {
      dataX[i] = X[i_min+i];
    }
    //    set up Y/E as pure peak
    FunctionDomain1DVector domain(dataX);
    FunctionValues backgroundvalues(domain);
    m_backgroundFunction->function(domain, backgroundvalues);

    for (size_t i = 0; i < numpts; ++i)
    {
      dataY[i] = Y[i_min+i] - backgroundvalues[i];
      if (dataY[i] < 0)
        dataY[i] = 0.;
      if (dataY[i] >= 1.0)
        dataE[i] = sqrt(dataY[i]);
      else
        dataE[i] = 1.0;
    }

    // Estimate/observe peak parameters
    const MantidVec& vecX = peakws->readX(0);
    const MantidVec& vecY = peakws->readY(0);

    double g_centre, g_height, g_fwhm;
    estimatePeakParameters(vecX, vecY, 0, numpts-1, g_centre, g_height, g_fwhm);

    // Create peak function
    IPeakFunction_sptr peakfunc = boost::dynamic_pointer_cast<IPeakFunction>(
          FunctionFactory::Instance().createFunction(m_peakFuncType));

    // Fit with loop upon specified FWHM range
    std::vector<double> vec_inSigma;
    for (unsigned int iwidth = minGuessedPeakWidth; iwidth <= maxGuessedPeakWidth; iwidth +=
         stepGuessedPeakWidth)
    {
      double in_sigma = (icentre + iwidth - i_min < vecX.size()) ? vecX[icentre + iwidth - i_min] - vecX[icentre - i_min] : 0.;
      if (in_sigma > 1.0E-20)
        vec_inSigma.push_back(in_sigma);
    }

    double in_centre = vecX[icentre - i_min];
    double peakleftbound = vecX[ileft - i_min];
    double peakrightbound = vecX[iright - i_min];
    PeakFittingRecord fitresult1 = multiFitPeakBackground(peakws, 0, input, spectrum, peakfunc, in_centre, g_height,vec_inSigma,
                                                          peakleftbound, peakrightbound);

    // Fit upon observation
    m_backgroundFunction->setParameter("A0", in_bg0);
    m_backgroundFunction->setParameter("A1", in_bg1);

    in_centre = g_centre;
    peakleftbound = g_centre - 3*g_fwhm;
    peakrightbound = g_centre + 3*g_fwhm;
    vec_inSigma.clear();
    vec_inSigma.push_back(g_fwhm);
    PeakFittingRecord fitresult2 = multiFitPeakBackground(peakws, 0, input, spectrum, peakfunc, in_centre, g_height,vec_inSigma,
                                                  peakleftbound, peakrightbound);

    // Compare results and add result to row
    processFitResult(fitresult1, fitresult2, peakfunc, m_backgroundFunction, spectrum, ileft, iright);

    return;
  }

  //----------------------------------------------------------------------------------------------
  /** Fit a single peak with given peak parameters as starting point
    */
  PeakFittingRecord FindPeaks::multiFitPeakBackground(MatrixWorkspace_sptr purepeakws, int purepeakindex,
                                                      MatrixWorkspace_sptr dataws, int datawsindex,
                                                      IPeakFunction_sptr peak,
                                                      double in_centre, double in_height, std::vector<double> in_sigmas,
                                                      double peakleftboundary, double peakrightboundary)
  {
    g_log.information() << "Fit peak with " << in_sigmas.size() << " starting sigmas.\n";

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

    for (size_t i = 0; i < in_sigmas.size(); ++i)
    {
      // (Re)set peak parameters
      peak->setCentre(in_centre);
      peak->setHeight(in_height);
      peak->setFwhm(in_sigmas[i]);

      double in_rwp;
      double rwp1 = fitPeakBackgroundFunction(peak, purepeakws, purepeakindex, startx, endx, peakcentreconstraint,
                                              in_rwp);
      vecRwp.push_back(rwp1);
      std::map<std::string, double> parameters = getFunctionParameters(peak);
      vecParameters.push_back(parameters);
    }

    // Set again to best result so far
    int bestindex = getBestResult(vecRwp);
    if (bestindex < 0)
    {
      // All fit attempts are failed.  Return with a FAIL record
      PeakFittingRecord failrecord;
      std::map<std::string, double> bkgdmap0 = getFunctionParameters(m_backgroundFunction);
      failrecord.set(DBL_MAX,  vecParameters[bestindex], bkgdmap0);
      return failrecord;
    }

    // Fit is good.
    setFunctionParameterValue(peak, vecParameters[bestindex]);
    g_log.information() << "Best fit result is No. " << bestindex << " with guess sigma = "
                        << in_sigmas[bestindex] << ".\n";

#if 1
    std::ofstream of1;
    std::stringstream filenamess;
    filenamess << "purepeak_" << in_sigmas.size() << ".dat";
    of1.open(filenamess.str());
    FunctionDomain1DVector ppdomain(purepeakws->readX(purepeakindex));
    FunctionValues ppvalues(ppdomain);
    peak->function(ppdomain, ppvalues);
    for (size_t i = 0; i < ppdomain.size(); ++i)
      of1 << ppdomain[i] << "\t\t" << purepeakws->readY(purepeakindex)[i] << "\t\t"
          << ppvalues[i] << ".\n";
    of1.close();
#endif

    // Fit background with better esitmation on peak (: m_backgroundFunction)
    //   Unfix background parameters
    size_t numbkgdparams = m_backgroundFunction->nParams();
    for (size_t i = 0; i < numbkgdparams; ++i)
      m_backgroundFunction->unfix(i);

    const MantidVec& X = dataws->readX(datawsindex);
    const MantidVec& Y = dataws->readY(datawsindex);
    const MantidVec& E = dataws->readE(datawsindex);
    int ileft = getVectorIndex(X, peak->centre()-3.0*peak->fwhm());
    int iright = getVectorIndex(X, peak->centre()+3.0*peak->fwhm());
    int i_min = getVectorIndex(X, purepeakws->readX(purepeakindex)[0]);
    int i_max = getVectorIndex(X, purepeakws->readX(purepeakindex).back());
    double bkgdchi2;  // in_bg0, in_bg1, in_bg2,
    bool goodfit = fitBackground(X, Y, E,  ileft, iright, i_min, i_max, bkgdchi2);
    if (!goodfit)
    {
      g_log.warning("Fitting background by excluding peak failed.");
    }
    std::map<std::string, double> bkgdmap1 = getFunctionParameters(m_backgroundFunction);

#if 1
    FunctionDomain1DVector compdomain(X);
    FunctionValues compvalues(compdomain);
    compfunc->function(compdomain, compvalues);
    std::ofstream of2;
    std::stringstream filenamess2;
    filenamess2 << "rawpeak_" << in_sigmas.size() << ".dat";
    of2.open(filenamess2.str());
    for (int i = i_min; i < i_max; ++i)
      of2 << X[i] << "\t\t" << Y[i] << "\t\t" << compvalues[i] << "\n";
    of2.close();
#endif

    // Fit with new background and every data points
    peakcentreconstraint = makePeakCentreConstraint(peak, peakleftboundary, peakrightboundary, true);
    double rwp1best;
    double rwp2 = fitPeakBackgroundFunction(compfunc, dataws, datawsindex, startx, endx, peakcentreconstraint,
                                            rwp1best);
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
  /** Set parameters values to a peak function
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
  /** Compare 2 fit results and record the better one
    */
  void FindPeaks::processFitResult(PeakFittingRecord& r1, PeakFittingRecord& r2, IPeakFunction_sptr peak,
                                   IFunction_sptr bkgdfunc, int spectrum, unsigned int ileft, unsigned int iright)
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

    // Set outpout information
    addInfoRow(spectrum, params, rawparams, finalrwp, fitfail);

    // Add function to list
    if (!fitfail)
    {
      API::CompositeFunction_sptr fitFunction(new API::CompositeFunction());
      fitFunction->addFunction(peak);
      fitFunction->addFunction(bkgdfunc);
      addFittedFunction(fitFunction, ileft, iright);
    }

    return;
  }


  //----------------------------------------------------------------------------------------------
  /** Get best result from a set of fitting result
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
    * Assumption: pure peak workspace with background removed
    * @param vecX :: vector of X-axis
    * @param vecY :: vector of Y-axis
    * @param i_min :: start
    * @param i_max :: end
    * @param centre :: estimated peak centre (maximum position)
    * @param height :: maximum
    * @param fwhm :: 2 fwhm
    */
  bool FindPeaks::estimatePeakParameters(const MantidVec& vecX, const MantidVec& vecY,
                                         size_t i_min, size_t i_max, double& centre, double& height, double& fwhm)
  {
    // Search for maximum
    size_t icentre = i_min;
    centre = vecX[i_min];
    height = vecY[i_min];
    for (size_t i = i_min+1; i <= i_max; ++i)
    {
      double y = vecY[i];
      if (y > height)
      {
        icentre = i;
        centre = vecX[i];
        height = y;
      }
    }

    // If maximum point is on the edge, return false
    if (icentre == i_min || icentre == i_max)
      return false;

    // Search for half-maximum: no need to very precise
    double leftfwhm = -1;
    for (int i = static_cast<int>(icentre)-1; i >= 0; --i)
    {
      double yh = vecY[i+1];
      double yl = vecY[i];
      if (yh > 0.5*height && yl <= 0.5*height)
      {
        leftfwhm = 0.5*(vecX[i] + vecX[i+1]);
      }
    }

    double rightfwhm = -1;
    for (size_t i = icentre+1; i <= i_max; ++i)
    {
      double yh = vecY[i-1];
      double yl = vecY[i];
      if (yh > 0.5*height && yl <= 0.5*height)
      {
        rightfwhm = 0.5*(vecX[i] + vecX[i-1]);
      }
    }

    if (leftfwhm <= 0 || rightfwhm <= 0)
      throw std::runtime_error("Programming logic error.  FWHM cannot be zero.");

    fwhm = leftfwhm + rightfwhm;

    g_log.information() << "Estimated peak parameters: Centre = " << centre << ", Height = "
                        << height << ", FWHM = " << fwhm << ".\n";

    return true;
  }


  //----------------------------------------------------------------------------------------------
  /** Estimate linear background
    * @param X ::
    * @param Y ::
    * @param i_min ::
    * @param i_max ::
    * @param out_bg0 ::
    * @param out_bg1 ::
    * @param out_bg2 ::
    */
  void FindPeaks::estimateLinearBackground(const MantidVec& X, const MantidVec& Y, const unsigned  i_min, const unsigned  i_max,
                                           double& out_bg0, double& out_bg1, double& out_bg2)
  {
    // Validate input
    if (i_min >= i_max)
      throw std::runtime_error("i_min cannot larger or equal to i_max");

    // FIXME - THIS IS A MAGI NUMBER
    const size_t MAGICNUMBER = 8;
    size_t numavg;
    if (i_max - i_min > MAGICNUMBER)
      numavg = 2;
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

    // Esitmate
    out_bg2 = 0.;
    out_bg1 = (y0-yf)/(x0-xf);
    out_bg0 = (xf*y0-x0*yf)/(xf-x0);

    m_backgroundFunction->setParameter("A0", out_bg0);
    m_backgroundFunction->setParameter("A1", out_bg1);
    if (m_backgroundFunction->nParams() > 2)
      m_backgroundFunction->setParameter("A2", 0.0);

    g_log.information() << "Estimated background: A0 = " << out_bg0 << ", A1 = "
                        << out_bg1 << ".\n";

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
  void FindPeaks::addInfoRow(const int spectrum, const std::vector<double> &params,
                             const std::vector<double> &rawParams, const double mincost, bool error)
  {
    API::TableRow t = m_outPeakTableWS->appendRow();
    t << spectrum;

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
      std::stringstream badss;
      badss << "No Good Fit Obtained! Chi2 = " << mincost << ". ";
      badss << "Possible reason: (1) Fit error = " << error << ", (2) params.size = " << params.size()
            << ", (3) rawParams.size():" << rawParams.size() << ". (Output with raw parameter = "
            << m_rawPeaksTable << ").";
      g_log.warning(badss.str());
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
          g_log.information() << (*it) << " ";
        }
      }
      else
      {
        for (std::vector<double>::const_iterator it = params.begin(); it != params.end(); ++it)
        {
          t << (*it);
          g_log.information() << (*it) << " ";
        }
      }

      t << mincost;
      g_log.information() << "Chi2 = " << mincost << "\n";
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
  /** Check the results of the fit algorithm to see if they make sense and update the best parameters.
    */
  void FindPeaks::updateFitResults(API::IAlgorithm_sptr fitAlg, std::vector<double> &bestEffparams,
                                   std::vector<double> &bestRawparams, double &mincost, const double expPeakPos,
                                   const double expPeakHeight)
  {
    // check the results of the fit status
    std::string fitStatus = fitAlg->getProperty("OutputStatus");
    bool allowedfailure = (fitStatus.find("cannot") < fitStatus.size())
        && (fitStatus.find("tolerance") < fitStatus.size());
    if (fitStatus.compare("success") != 0 && !allowedfailure)
    {
      g_log.debug() << "Fit Status = " << fitStatus << ".  Not to update fit result" << std::endl;
      return;
    }

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
#if 1
    fitFunc->addFunction(background);
#else
    fitFunc->addFunction(m_backgroundFunction);
#endif

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
    for (size_t i = 0; i < numpts; ++i)
    {
      double cal_i = values[i];
      double obs_i = partY[i];
      double sigma = 1.0;
      double weight = 1.0/(sigma*sigma);
      double diff = obs_i - cal_i;

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
  void FindPeaks::addFittedFunction(IFunction_sptr fitfunction, unsigned int ileft, unsigned int iright)
  {
    IFunction_sptr copyfunc = createFunction(0., 0., 0., 0., 0., 0., true);
    std::vector<std::string> funparnames = fitfunction->getParameterNames();
    for (size_t i = 0; i < funparnames.size(); ++i)
    {
      std::string parname = funparnames[i];
      copyfunc->setParameter(parname, fitfunction->getParameter(parname));
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
      std::stringstream errss;
      errss << "Size of workspace to fit for background = " << newX.size()
            << ". It is too small to proceed. ";
      errss << "Input i_min = " << imin << ",i_max = " << imax << ", i_left = " << ileft
            << ", i_right = " << iright;
      g_log.error(errss.str());

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

    g_log.information() << "Background Type = " << m_backgroundType << "  Function: "
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
    fit->setProperty("Minimizer", "Levenberg-Marquardt");
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
  /** Fit a (peak) function
    *
    * @return RWP
    */
  double FindPeaks::fitPeakBackgroundFunction(IFunction_sptr peakbgkgdfunc,
                                              MatrixWorkspace_sptr dataws, size_t wsindex,
                                              double startx, double endx, std::string constraint, double& init_rwp)
  {
    std::stringstream dbss;
    dbss << "Fit data workspace spectrum " << wsindex << ".  Parameters: ";
    std::vector<std::string> comparnames = peakbgkgdfunc->getParameterNames();
    for (size_t i = 0; i < comparnames.size(); ++i)
      dbss << comparnames[i] << ", ";
    g_log.information(dbss.str());

    // Starting chi-square
    init_rwp = calculateFunctionRwp(peakbgkgdfunc, dataws, wsindex, startx, endx);

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
    gfit->setProperty("Function", peakbgkgdfunc);
    gfit->setProperty("InputWorkspace", dataws);
    gfit->setProperty("WorkspaceIndex", static_cast<int>(wsindex));
    gfit->setProperty("MaxIterations", 50);
    gfit->setProperty("StartX", startx);
    gfit->setProperty("EndX", endx);
    if (constraint.size() > 0)
      gfit->setProperty("Constraints", constraint);
    // FIXME - Minimizer should be more flexible
    gfit->setProperty("Minimizer", "Levenberg-Marquardt");
    gfit->setProperty("CostFunction", "Least squares");

    g_log.debug() << "Function (to fit): " << peakbgkgdfunc->asString() << "  From "
                  << startx << "  to " << endx << ".\n";

    // Fit
    gfit->executeAsChildAlg();
    if (!gfit->isExecuted())
    {
      g_log.error("Fit is not executed correctly.");
      return DBL_MAX;
    }

    // Analyze result
    std::string fitpeakstatus = gfit->getProperty("OutputStatus");
    double final_rwp = calculateFunctionRwp(peakbgkgdfunc, dataws, wsindex, startx, endx);

    // FIXME : m_peakFunction should be same as peak function in composition function
    g_log.information() << "Fit Peak (+background) Status = " << fitpeakstatus << ". Starting Rwp = "
                        << init_rwp << ".  Fitted Rwp = " << final_rwp << ".\n";
#if 1
    m_peakFunction = gfit->getProperty("Function");
    // updateFitResults(gfit, bestparams, bestRawParams, mincost, X[i4], in_height);

    std::vector<std::string> parnames = m_peakFunction->getParameterNames();
    std::stringstream dbss2;
    for (size_t i = 0; i < parnames.size(); ++i)
      dbss2 << parnames[i] << "\t: " << "Input Function = " << peakbgkgdfunc->getParameter(parnames[i])
            << ", Output Function = " << m_peakFunction->getParameter(parnames[i]) << ".\n";
    g_log.information(dbss2.str());
#endif

    return final_rwp;

#if 0
    DO NOT KNOW HOW TO DEAL WITH THEM!

    // check to see if the last one went through
    if (bestparams.empty())
    {
      this->addInfoRow(spectrum, bestparams, bestRawParams, mincost, false);
      return;
    }

    // h) Fit again with everything altogether
    IAlgorithm_sptr lastfit;
    try
    {
      // Fitting the candidate peaks to a Gaussian
      lastfit = createChildAlgorithm("Fit", -1, -1, true);
    } catch (Exception::NotFoundError &)
    {
      g_log.error("The StripPeaks algorithm requires the CurveFitting library");
      throw;
    }

    // c) Set initial fitting parameters
    IFunction_sptr peakAndBackgroundFunction = this->createFunction(bestparams[2], bestparams[0],
                                                                    bestparams[1], bestparams[3], bestparams[4], bestparams[5], true);
    g_log.debug() << "(High Background) Final Fit Function: " << peakAndBackgroundFunction->asString()
                  << std::endl;

    // d) complete fit
    lastfit->setProperty("Function", peakAndBackgroundFunction);
    lastfit->setProperty("InputWorkspace", input);
    lastfit->setProperty("WorkspaceIndex", spectrum);
    lastfit->setProperty("MaxIterations", 50);
    lastfit->setProperty("StartX", in_centre - windowSize);
    lastfit->setProperty("EndX", in_centre + windowSize);
    lastfit->setProperty("Minimizer", "Levenberg-Marquardt");
    lastfit->setProperty("CostFunction", "Least squares");

    // e) Fit and get result
    lastfit->executeAsChildAlg();

    this->updateFitResults(lastfit, bestparams, bestRawParams, mincost, in_centre, in_height);

    if (!bestparams.empty())
      this->addInfoRow(spectrum, bestparams, bestRawParams, mincost,
                       (bestparams[0] < X.front() || bestparams[0] > X.back()));
    else
      this->addInfoRow(spectrum, bestparams, bestRawParams, mincost, true);
#endif

  }

  //----------------------------------------------------------------------------------------------
  API::MatrixWorkspace_sptr FindPeaks::createOutputDataWorkspace()
  {
    size_t lenX = m_dataWS->readX(0).size();
    size_t lenY = m_dataWS->readY(0).size();

    MatrixWorkspace_sptr outws = WorkspaceFactory::Instance().create(m_dataWS, 1, lenX, lenY);

    const MantidVec& vecInX = m_dataWS->readX(0);
    MantidVec& vecX = outws->dataX(0);
    MantidVec& vecY = outws->dataY(0);

    // X - axis
    for (size_t i = 0; i < vecX.size(); ++i)
    {
      vecX[i] = vecInX[i];
    }
    for (size_t i = 0; i < vecY.size(); ++i)
    {
      vecY[i] = 0.;
    }

    // Calculation
    g_log.information() << "Plot total " << m_fitFunctions.size() << " functions" << ".\n";
    FunctionDomain1DVector domain(vecX);
    FunctionValues values(domain);

    for (size_t fi = 0; fi < m_fitFunctions.size(); ++fi)
    {
      // calcualte function
      IFunction_sptr tfunc = m_fitFunctions[fi];
      tfunc->function(domain, values);

      // determine range to apply
      unsigned int ileft = m_peakLeftIndexes[fi];
      unsigned int iright = m_peakRightIndexes[fi];
      unsigned int idelta = iright-ileft;
      size_t i_left, i_right;
      if (ileft > idelta)
        i_left = static_cast<size_t>(ileft - idelta);
      else
        i_left = 0;
      if (static_cast<size_t>(iright) < vecY.size() - 2*idelta - 1)
        i_right = static_cast<size_t>(iright + 2*idelta);
      else
        i_right = vecY.size()-1;

      for (size_t i = i_left; i <= i_right; ++i)
      {
        vecY[i] = values[i];
      }

      g_log.information() << "Peak " << fi << " Range = " << vecX[i_left]
                          << ", " << vecX[i_right] << ".\n";

    }

    return outws;
  }



} // namespace Algorithms
} // namespace Mantid


// 0.5044, 0.5191, 0.535, 0.5526, 0.5936, 0.6178, 0.6453, 0.6768, 0.7134, 0.7566, 0.8089, 0.8737, 0.9571, 1.0701, 1.2356, 1.5133, 2.1401
