#include "MantidAlgorithms/GetDetOffsetsMultiPeaks.h"
#include "MantidAlgorithms/GSLFunctions.h"
#include "MantidAPI/FileProperty.h"
#include "MantidAPI/FunctionFactory.h"
#include "MantidAPI/WorkspaceValidators.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidAPI/ITableWorkspace.h"
#include "MantidAPI/IPeakFunction.h"
#include "MantidAPI/IBackgroundFunction.h"
#include "MantidAPI/CompositeFunction.h"
#include "MantidAPI/TableRow.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/ListValidator.h"
#include "MantidKernel/VectorHelper.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidDataObjects/MaskWorkspace.h"
#include "MantidDataObjects/OffsetsWorkspace.h"
#include "MantidKernel/Statistics.h"
#include <boost/math/special_functions/fpclassify.hpp>
#include <fstream>
#include <iomanip>
#include <ostream>
#include <sstream>

namespace Mantid
{
namespace Algorithms
{
  namespace
  {
    /// Factor to convert full width half max to sigma for calculations of I/sigma.
    const double FWHM_TO_SIGMA = 2.0*sqrt(2.0*std::log(2.0));
    const double BAD_OFFSET(1000.); // mark things that didn't work with this

    //--------------------------------------------------------------------------------------------
    /** Helper function for calculating costs in gsl.
      * cost = \sum_{p}|d^0_p - (1+offset)*d^{(f)}_p|\cdot H^2_p, where d^{(f)} is within minD and maxD
       * @param v Vector of offsets.
       * @param params Array of input parameters.
       * @returns Sum of the errors.
       */
    double gsl_costFunction(const gsl_vector *v, void *params)
    {
      // FIXME - there is no need to use vectors peakPosToFit, peakPosFitted and chisq
      double *p = (double *)params;
      size_t n = static_cast<size_t>(p[0]);
      std::vector<double> peakPosToFit(n);
      std::vector<double> peakPosFitted(n);
      std::vector<double> height2(n);
      double minD = p[1];
      double maxD = p[2];
      for (size_t i = 0; i < n; i++)
      {
        peakPosToFit[i] = p[i+3];
      }
      for (size_t i = 0; i < n; i++)
      {
        peakPosFitted[i] = p[i+n+3];
      }
      for (size_t i = 0; i < n; i++)
      {
        height2[i] = p[i+2*n+3];
      }

      double offset = gsl_vector_get(v, 0);
      double errsum = 0.0;
      for (size_t i = 0; i < n; ++i)
      {
        // Get references to the data
        //See formula in AlignDetectors
        double peakPosMeas = (1.+offset)*peakPosFitted[i];
        if(peakPosFitted[i] > minD && peakPosFitted[i] < maxD)
          errsum += std::fabs(peakPosToFit[i]-peakPosMeas)*height2[i];
      }
      return errsum;
    }
  }

  //----------------------------------------------------------------------------------------------
  /** The windows should be half of the distance between the peaks of maxWidth, whichever is smaller.
     * @param dmin :: The minimum d-spacing for the workspace
     * @param dmax :: The maximum d-spacing for the workspace
     * @param vec_peakcentre :: The list of peaks to generate windows for
     * @param maxWidth :: The maximum width of a window
     * @return The list of windows for each peak
     */
  std::vector<double> generateWindows(const double dmin, const double dmax,
                                      const std::vector<double> &vec_peakcentre, const double maxWidth)
  {
    if (maxWidth <= 0.)
    {
      return std::vector<double>(); // empty vector because this is turned off
    }

    std::size_t numPeaks = vec_peakcentre.size();
    std::vector<double> windows(2*numPeaks);
    double widthLeft;
    double widthRight;
    for (std::size_t i = 0; i < numPeaks; i++)
    {
      if (i == 0)
        widthLeft = vec_peakcentre[i] - dmin;
      else
        widthLeft = .5 * (vec_peakcentre[i] - vec_peakcentre[i-1]);
      if (i + 1 == numPeaks)
        widthRight = dmax - vec_peakcentre[i];
      else
        widthRight = .5 * (vec_peakcentre[i+1] - vec_peakcentre[i]);

      if (maxWidth > 0)
      {
        widthLeft  = std::min(widthLeft, maxWidth);
        widthRight = std::min(widthRight, maxWidth);
      }

      windows[2*i]   = vec_peakcentre[i] - widthLeft;
      windows[2*i+1] = vec_peakcentre[i] + widthRight;
    }

    return windows;
  }

  using namespace Kernel;
  using namespace API;
  using std::size_t;
  using namespace DataObjects;

  // Register the class into the algorithm factory
  DECLARE_ALGORITHM(GetDetOffsetsMultiPeaks)

  //----------------------------------------------------------------------------------------------
  /** Constructor
    */
  GetDetOffsetsMultiPeaks::GetDetOffsetsMultiPeaks() :
    API::Algorithm()
  {}

  //----------------------------------------------------------------------------------------------
  /** Destructor
    */
  GetDetOffsetsMultiPeaks::~GetDetOffsetsMultiPeaks()
  {}
  
  //----------------------------------------------------------------------------------------------
  /** Initialisation method. Declares properties to be used in algorithm.
     */
  void GetDetOffsetsMultiPeaks::init()
  {
    declareProperty(new WorkspaceProperty<>("InputWorkspace","",Direction::Input,
                                            boost::make_shared<WorkspaceUnitValidator>("dSpacing")),
                    "A 2D matrix workspace with X values of d-spacing");

    declareProperty(new ArrayProperty<double>("DReference"),"Enter a comma-separated list of the expected X-position of the centre of the peaks. Only peaks near these positions will be fitted." );
    
    declareProperty("FitWindowMaxWidth", 0.,
                    "Optional: The maximum width of the fitting window. If this is <=0 the windows is not specified to FindPeaks" );

    declareProperty(new WorkspaceProperty<TableWorkspace>("FitwindowTableWorkspace", "", Direction::Input, PropertyMode::Optional),
                    "Name of the input Tableworkspace containing peak fit window information for each spectrum. ");

    std::vector<std::string> peaktypes;
    peaktypes.push_back("BackToBackExponential");
    peaktypes.push_back("Gaussian");
    peaktypes.push_back("Lorentzian");
    declareProperty("PeakFunction", "Gaussian", boost::make_shared<StringListValidator>(peaktypes),
                    "Type of peak to fit");

    std::vector<std::string> bkgdtypes;
    bkgdtypes.push_back("Flat");
    bkgdtypes.push_back("Linear");
    bkgdtypes.push_back("Quadratic");
    declareProperty("BackgroundType", "Linear", boost::make_shared<StringListValidator>(bkgdtypes),
                    "Type of Background. The choice can be either Linear or Quadratic");

    declareProperty("HighBackground", true,
                    "Relatively weak peak in high background");
    declareProperty(new FileProperty("GroupingFileName","", FileProperty::OptionalSave, ".cal"),
                    "Optional: The name of the output CalFile to save the generated OffsetsWorkspace." );
    declareProperty(new WorkspaceProperty<OffsetsWorkspace>("OutputWorkspace","",Direction::Output),
                    "An output workspace containing the offsets.");
    declareProperty(new WorkspaceProperty<OffsetsWorkspace>("NumberPeaksWorkspace","NumberPeaksFitted",Direction::Output),
                    "An output workspace containing the offsets.");
    declareProperty(new WorkspaceProperty<>("MaskWorkspace", "Mask", Direction::Output),
                    "An output workspace containing the mask.");
    declareProperty("MaxOffset", 1.0, "Maximum absolute value of offsets; default is 1");
    declareProperty("MaxChiSq", 100., "Maximum chisq value for individual peak fit allowed. (Default: 100)");

    declareProperty("MinimumPeakHeight", 2.0, "Minimum value allowed for peak height.");

    declareProperty("MinimumPeakHeightObs", 0.0, "Least value of the maximum observed Y value of a peak within "
                    "specified region.  If any peak's maximum observed Y value is smaller, then "
                    "this peak will not be fit.  It is designed for EventWorkspace with integer counts.");

    //Disable default gsl error handler (which is to call abort!)
    gsl_set_error_handler_off();

    auto inpreswsprop = new WorkspaceProperty<MatrixWorkspace>("InputResolutionWorkspace", "", Direction::Input,
                                                               PropertyMode::Optional);
    declareProperty(inpreswsprop, "Name of the optional input resolution (delta(d)/d) workspace. ");

    auto tablewsprop = new WorkspaceProperty<TableWorkspace>("SpectraFitInfoTableWorkspace", "FitInfoTable", Direction::Output);
    declareProperty(tablewsprop, "Name of the output table workspace containing spectra peak fit information.");

    auto offsetwsprop = new WorkspaceProperty<TableWorkspace>("PeaksOffsetTableWorkspace", "PeakOffsetTable", Direction::Output);
    declareProperty(offsetwsprop, "Name of an output table workspace containing peaks' offset data.");

    auto ddodwsprop = new WorkspaceProperty<MatrixWorkspace>("FittedResolutionWorkspace", "ResolutionWS", Direction::Output);
    declareProperty(ddodwsprop, "Name of the resolution workspace containing delta(d)/d for each unmasked spectrum. ");

    declareProperty("MinimumResolutionFactor", 0.1, "Factor of the minimum allowed Delta(d)/d of any peak to its suggested Delta(d)/d. ");

    declareProperty("MaximumResolutionFactor", 10.0, "Factor of the maximum allowed Delta(d)/d of any peak to its suggested Delta(d)/d. ");

  }


  //-----------------------------------------------------------------------------------------
  /** Executes the algorithm
     *
     *  @throw Exception::FileError If the grouping file cannot be opened or read successfully
     */
  void GetDetOffsetsMultiPeaks::exec()
  {
    // Process input information
    processProperties();

    // Create information workspaces
    createInformationWorkspaces();

    // Calculate offset of each detector
    calculateDetectorsOffsets();

    // Return the output
    setProperty("OutputWorkspace",outputW);
    setProperty("NumberPeaksWorkspace",outputNP);
    setProperty("MaskWorkspace",maskWS);
    setProperty("FittedResolutionWorkspace", m_resolutionWS);
    setProperty("SpectraFitInfoTableWorkspace", m_infoTableWS);
    setProperty("PeaksOffsetTableWorkspace", m_peakOffsetTableWS);

    // Also save to .cal file, if requested
    std::string filename=getProperty("GroupingFileName");
    if (!filename.empty())
    {
      progress(0.9, "Saving .cal file");
      IAlgorithm_sptr childAlg = createChildAlgorithm("SaveCalFile");
      childAlg->setProperty("OffsetsWorkspace", outputW);
      childAlg->setProperty("MaskWorkspace", maskWS);
      childAlg->setPropertyValue("Filename", filename);
      childAlg->executeAsChildAlg();
    }

    // Make summary
    progress(0.92, "Making summary");
    makeFitSummary();

    return;
  }

  //----------------------------------------------------------------------------------------------
  /** Process input and output properties
    */
  void GetDetOffsetsMultiPeaks::processProperties()
  {
    m_inputWS=getProperty("InputWorkspace");

    // determine min/max d-spacing of the workspace
    double wkspDmin, wkspDmax;
    m_inputWS->getXMinMax(wkspDmin, wkspDmax);

    // the peak positions and where to fit
    m_peakPositions = getProperty("DReference");
    if (m_peakPositions.size() == 0)
      throw std::runtime_error("There is no input referenced peak position.");
    std::sort(m_peakPositions.begin(), m_peakPositions.end());

    // Fit windows
    std::string fitwinwsname = getPropertyValue("FitwindowTableWorkspace");
    g_log.notice() << "FitWindowTableWorkspace name: " << fitwinwsname << "\n";
    if (fitwinwsname.size() > 0)
    {
      // Use fit window workspace for each spectrum
      TableWorkspace_sptr fitwintablews = getProperty("FitwindowTableWorkspace");
      importFitWindowTableWorkspace(fitwintablews);
      m_useFitWindowTable = true;
    }
    else
    {
      // Use property 'FitWindowMaxWidth'
      double maxwidth = getProperty("FitWindowMaxWidth");
      m_fitWindows = generateWindows(wkspDmin, wkspDmax, m_peakPositions, maxwidth);
      m_useFitWindowTable = false;

      // Debug otuput
      std::stringstream infoss;
      infoss << "Fit Windows : ";
      if (m_fitWindows.empty()) infoss << "(empty)";
      else for (std::vector<double>::const_iterator it = m_fitWindows.begin(); it != m_fitWindows.end(); ++it)
        infoss << *it << " ";
      
      g_log.information(infoss.str());

      if (m_fitWindows.size() == 0) g_log.warning() << "Input FitWindowMaxWidth = " << maxwidth
                                                    << "  No FitWidows will be generated." << "\n";
    }

    // Some shortcuts for event workspaces
    eventW = boost::dynamic_pointer_cast<const EventWorkspace>( m_inputWS );
    // bool isEvent = false;
    isEvent = false;
    if (eventW)
      isEvent = true;

    // Cache the peak and background function names
    m_peakType = this->getPropertyValue("PeakFunction");
    m_backType = this->getPropertyValue("BackgroundType");

    // The maximum allowable chisq value for an individual peak fit
    m_maxChiSq = this->getProperty("MaxChiSq");
    m_minPeakHeight = this->getProperty("MinimumPeakHeight");
    m_maxOffset=getProperty("MaxOffset");
    m_leastMaxObsY = getProperty("MinimumPeakHeightObs");

    // Create output workspaces
    outputW = boost::make_shared<OffsetsWorkspace>(m_inputWS->getInstrument());
    outputNP = boost::make_shared<OffsetsWorkspace>(m_inputWS->getInstrument());
    MatrixWorkspace_sptr tempmaskws(new MaskWorkspace(m_inputWS->getInstrument()));
    maskWS = tempmaskws;

    // Input resolution
    std::string reswsname = getPropertyValue("InputResolutionWorkspace");
    if (reswsname.size() == 0) m_hasInputResolution = false;
    else
    {
      m_inputResolutionWS = getProperty("InputResolutionWorkspace");
      m_hasInputResolution = true;

      m_minResFactor = getProperty("MinimumResolutionFactor");
      m_maxResFactor = getProperty("MaximumResolutionFactor");

      if (m_minResFactor >= m_maxResFactor)
        throw std::runtime_error("Input peak resolution boundary is 0 or negative.");

      // Check
      if (m_inputResolutionWS->getNumberHistograms() != m_inputWS->getNumberHistograms())
        throw std::runtime_error("Input workspace does not match resolution workspace. ");
    }

    return;
  }


  //-----------------------------------------------------------------------------------------
  /** Executes the algorithm
     *
     *  @throw Exception::RuntimeError If ... ...
     */
  void GetDetOffsetsMultiPeaks::importFitWindowTableWorkspace(TableWorkspace_sptr windowtablews)
  {
    // Check number of columns matches number of peaks
    size_t numcols = windowtablews->columnCount();
    size_t numpeaks = m_peakPositions.size();

    if (numcols != 2*numpeaks+1)
      throw std::runtime_error("Number of columns is not 2 times of number of referenced peaks. ");

    // Check number of spectra should be same to input workspace
    size_t numrows = windowtablews->rowCount();
    bool needuniversal = false;
    if (numrows < m_inputWS->getNumberHistograms())
      needuniversal = true;
    else if (numrows > m_inputWS->getNumberHistograms())
      throw std::runtime_error("Number of rows in table workspace is larger than number of spectra.");

    // Clear and re-size of the vector for fit windows
    m_vecFitWindow.clear();
    m_vecFitWindow.resize(m_inputWS->getNumberHistograms());

    std::vector<double> vec_univFitWindow;
    bool founduniversal = false;

    // Parse the table workspace
    for (size_t i = 0; i < numrows; ++i)
    {
      // spectrum number
      int spec = windowtablews->cell<int>(i, 0);
      if (spec >= static_cast<int>(numrows))
      {
        std::stringstream ess;
        ess << "Peak fit windows at row " << i << " has spectrum " << spec
            << ", which is out of allowed range! ";
        throw std::runtime_error(ess.str());
      }
      if (spec < 0 && founduniversal)
      {
        throw std::runtime_error("There are more than 1 universal spectrum (spec < 0) in TableWorkspace.");
      }
      else if (spec >= 0 && m_vecFitWindow[spec].size() != 0)
      {
        std::stringstream ess;
        ess << "Peak fit windows at row " << i << " has spectrum " << spec
            << ", which appears before in fit window table workspace. ";
        throw std::runtime_error(ess.str());
      }

      // fit windows
      std::vector<double> fitwindows(numcols-1);
      for (size_t j = 1; j < numcols; ++j)
      {
        double dtmp = windowtablews->cell<double>(i, j);
        fitwindows[j-1] = dtmp;
      }

      // add to vector of fit windows
      if (spec >= 0)
        m_vecFitWindow[spec] = fitwindows;
      else
      {
        vec_univFitWindow = fitwindows;
        founduniversal = true;
      }
    }

    // Check and fill if using universal
    if (needuniversal && !founduniversal)
    {
      // Invalid case
      throw std::runtime_error("Number of rows in TableWorkspace is smaller than number of spectra.  But "
                               "there is no universal fit window given!");
    }
    else if (founduniversal)
    {
      // Fill the universal
      for (size_t i = 0; i < m_inputWS->getNumberHistograms(); ++i)
        if (m_vecFitWindow[i].size() == 0)
          m_vecFitWindow[i] = vec_univFitWindow;
    }

    return;
  }


  //-----------------------------------------------------------------------------------------
  /** Calculate (all) detectors' offsets
    */
  void GetDetOffsetsMultiPeaks::calculateDetectorsOffsets()
  {
    int nspec=static_cast<int>(m_inputWS->getNumberHistograms());

    //To get the workspace index from the detector ID
    const detid2index_map pixel_to_wi = maskWS->getDetectorIDToWorkspaceIndexMap(true);

    // Fit all the spectra with a gaussian
    Progress prog(this, 0, 1.0, nspec);

    // cppcheck-suppress syntaxError
    PRAGMA_OMP(parallel for schedule(dynamic, 1) )
    for (int wi=0; wi<nspec; ++wi)
    {
      PARALLEL_START_INTERUPT_REGION

      std::vector<double> fittedpeakpositions, tofitpeakpositions;
      FitPeakOffsetResult offsetresult = calculatePeakOffset(wi, fittedpeakpositions, tofitpeakpositions);

      // Get the list of detectors in this pixel
      const std::set<detid_t> & dets = m_inputWS->getSpectrum(wi)->getDetectorIDs();

      // Most of the exec time is in FitSpectra, so this critical block should not be a problem.
      PARALLEL_CRITICAL(GetDetOffsetsMultiPeaks_setValue)
      {
        // Use the same offset for all detectors from this pixel (in case of summing pixels)
        std::set<detid_t>::iterator it;
        for (it = dets.begin(); it != dets.end(); ++it)
        {
          // Set value to output peak offset workspace
          outputW->setValue(*it, offsetresult.offset, offsetresult.fitSum);

          // Set value to output peak number workspace
          outputNP->setValue(*it, offsetresult.peakPosFittedSize, offsetresult.chisqSum);

          // Set value to mask workspace
          const auto mapEntry = pixel_to_wi.find(*it);
          if ( mapEntry == pixel_to_wi.end() ) continue;

          const size_t workspaceIndex = mapEntry->second;
          if (offsetresult.mask > 0.9)
          {
            // Being masked
            maskWS->maskWorkspaceIndex(workspaceIndex);
            maskWS->dataY(workspaceIndex)[0] = offsetresult.mask;
          }
          else
          {
            // Using the detector
            maskWS->dataY(workspaceIndex)[0] = offsetresult.mask;

            // check the average value of delta(d)/d.  if it is far off the theorical value, output
            // FIXME - This warning should not appear by filtering out peaks that are too wide or narrow.
            // TODO - Delete the if statement below if it is never triggered.
            if (m_hasInputResolution)
            {
              double pixelresolution = m_inputResolutionWS->readY(wi)[0];
              if (offsetresult.resolution > 10*pixelresolution || offsetresult.resolution < 0.1*pixelresolution)
                g_log.warning() << "Spectrum " << wi << " delta(d)/d = " << offsetresult.resolution << "\n";
            }
          }
        } // ENDFOR (detectors)

        // Report offset fitting result/status
        addInfoToReportWS(wi, offsetresult, tofitpeakpositions, fittedpeakpositions);

      } // End of critical region


      prog.report();
      PARALLEL_END_INTERUPT_REGION
    }
    PARALLEL_CHECK_INTERUPT_REGION

    return;
  }

  //----------------------------------------------------------------------------------------------
  /** Calculate offset for one spectrum
    */
  FitPeakOffsetResult GetDetOffsetsMultiPeaks::calculatePeakOffset(const int wi, std::vector<double>& vec_peakPosFitted,
                                                                   std::vector<double>& vec_peakPosRef)
  {
    // Initialize the structure to return
    FitPeakOffsetResult fr;

    fr.offset = 0.0;
    fr.fitoffsetstatus = "N/A";
    fr.chi2 = -1;

    fr.fitSum = 0.0;
    fr.chisqSum = 0.0;

    fr.peakPosFittedSize = 0.0;

    fr.numpeaksfitted = 0;
    fr.numpeakstofit = 0;
    fr.numpeaksindrange = 0;

    // Checks for empty and dead detectors
    if ((isEvent) && (eventW->getEventList(wi).empty()))
    {
      // empty detector will be masked
      fr.offset = BAD_OFFSET;
      fr.fitoffsetstatus = "empty det";
    }
    else
    {
      // dead detector will be masked
      const MantidVec& Y = m_inputWS->readY(wi);
      const int YLength = static_cast<int>(Y.size());
      double sumY = 0.0;
      for (int i = 0; i < YLength; i++) sumY += Y[i];
      if (sumY < 1.e-30)
      {
        // Dead detector will be masked
        fr.offset = BAD_OFFSET;
        fr.fitoffsetstatus = "dead det";
      }
    }

    // Calculate peak offset for 'good' detector
    if (fr.offset < 10.)
    {
      // Fit peaks
      // std::vector<double> vec_peakPosRef, vec_peakPosFitted;
      std::vector<double> vec_fitChi2;
      std::vector<double> vec_peakHeights;
      size_t nparams;
      double minD, maxD;
      int i_highestpeak;
      double resolution, devresolution;
      fr.numpeaksindrange = fitSpectra(wi, m_inputWS, m_peakPositions, m_fitWindows,
                                       nparams, minD, maxD, vec_peakPosRef, vec_peakPosFitted,
                                       vec_fitChi2, vec_peakHeights, i_highestpeak,
                                       resolution, devresolution);
      fr.numpeakstofit = static_cast<int>(m_peakPositions.size());
      fr.numpeaksfitted = static_cast<int>(vec_peakPosFitted.size());
      fr.resolution = resolution;
      fr.dev_resolution = devresolution;

      // Fit offset
      if (nparams > 0 && fr.numpeaksindrange > 0)
      {
        fitPeaksOffset(nparams, minD, maxD, vec_peakPosRef, vec_peakPosFitted, vec_peakHeights, fr);

        // Deviation of calibrated position to the strong peak
        if (fr.fitoffsetstatus == "success")
        {
          double highpeakpos = vec_peakPosFitted[i_highestpeak];
          double highpeakpos_target = vec_peakPosRef[i_highestpeak];
          fr.highestpeakpos = highpeakpos;
          fr.highestpeakdev = fabs(highpeakpos*(1+fr.offset) - highpeakpos_target);
        }
        else
        {
          fr.highestpeakpos = 0.0;
          fr.highestpeakdev = -1.0;
        }
      }
      else
      {
        // Not enough peaks have been found.
        // Output warning
        std::stringstream outss;
        outss << "Spectra " << wi << " has 0 parameter for it.  Set to bad_offset." ;
        g_log.debug(outss.str());
        fr.offset = BAD_OFFSET;
        fr.fitoffsetstatus = "no peaks";
      }
    }

    // Final check offset
    fr.mask = 0.0;
    if (std::abs(fr.offset) > m_maxOffset)
    {
      std::stringstream infoss;
      infoss << "Spectrum " << wi << " has offset = " << fr.offset << ", which exceeds maximum offset "
             << m_maxOffset << ".  Spectrum is masked. ";
      g_log.information(infoss.str());

      std::stringstream msgss;
      if (fr.fitoffsetstatus == "success")
        msgss << "exceed max offset. " << "offset = " << fr.offset;
      else
        msgss << fr.fitoffsetstatus << ". " << "offset = " << fr.offset;
      fr.fitoffsetstatus = msgss.str();

      fr.mask = 1.0;
      fr.offset = 0.0;
    }

    return fr;
  } /// ENDFUNCTION: calculatePeakOffset


  //----------------------------------------------------------------------------------------------
  /** Fit peaks' offset by minimize the fitting function
    */
  void GetDetOffsetsMultiPeaks::fitPeaksOffset(const size_t inpnparams, const double minD, const double maxD,
                                               const std::vector<double>& vec_peakPosRef,
                                               const std::vector<double>& vec_peakPosFitted,
                                               const std::vector<double>& vec_peakHeights,
                                               FitPeakOffsetResult& fitresult)
  {
    // Set up array for minimization/optimization by GSL library
    size_t nparams = inpnparams;
    if(nparams > 50)
      nparams = 50;

    double params[153];
    params[0] = static_cast<double>(nparams);
    params[1] = minD;
    params[2] = maxD;
    for (size_t i = 0; i < nparams; i++)
    {
      params[i+3] = vec_peakPosRef[i];
    }
    for (size_t i = 0; i < nparams; i++)
    {
      params[i+3+nparams] = vec_peakPosFitted[i];
    }

    // the reason to put these codes here is that nparams may be altered in this method
    fitresult.peakPosFittedSize = static_cast<double>(vec_peakPosFitted.size());
    for (size_t i = 0; i < nparams; i++)
    {
      params[i+3+2*nparams] = (vec_peakHeights[i]*vec_peakHeights[i]); // vec_fitChi2[i];
      fitresult.chisqSum += 1./(vec_peakHeights[i]*vec_peakHeights[i]); // vec_fitChi2[i];
    }

    // Set up GSL minimzer
    const gsl_multimin_fminimizer_type *T = gsl_multimin_fminimizer_nmsimplex;
    gsl_multimin_fminimizer *s = NULL;
    gsl_vector *ss, *x;
    gsl_multimin_function minex_func;

    // Finally do the fitting
    size_t nopt = 1;
    size_t iter = 0;
    int status = 0;
    double size;

    /* Starting point */
    x = gsl_vector_alloc (nopt);
    gsl_vector_set_all (x, 0.0);

    /* Set initial step sizes to 0.001 */
    ss = gsl_vector_alloc (nopt);
    gsl_vector_set_all (ss, 0.001);

    /* Initialize method and iterate */
    minex_func.n = nopt;
    minex_func.f = &gsl_costFunction;
    minex_func.params = &params;

    s = gsl_multimin_fminimizer_alloc (T, nopt);
    gsl_multimin_fminimizer_set (s, &minex_func, x, ss);

    do
    {
      iter++;
      status = gsl_multimin_fminimizer_iterate(s);
      if (status)
        break;

      size = gsl_multimin_fminimizer_size (s);
      status = gsl_multimin_test_size (size, 1e-4);

    }
    while (status == GSL_CONTINUE && iter < 50);

    // Output summary to log file
    std::string reportOfDiffractionEventCalibrateDetectors = gsl_strerror(status);
    /*
    g_log.debug() << " Workspace Index = " << wi <<
                     " Method used = " << " Simplex" <<
                     " Iteration = " << iter <<
                     " Status = " << reportOfDiffractionEventCalibrateDetectors <<
                     " Minimize Sum = " << s->fval <<
                     " Offset   = " << gsl_vector_get (s->x, 0) << "  \n";
    */
    fitresult.offset = gsl_vector_get (s->x, 0);
    fitresult.fitSum = s->fval;

    fitresult.fitoffsetstatus = reportOfDiffractionEventCalibrateDetectors;
    fitresult.chi2 = s->fval;

    gsl_vector_free(x);
    gsl_vector_free(ss);
    gsl_multimin_fminimizer_free (s);
    return;
  }


  //----------------------------------------------------------------------------------------------
  namespace { // anonymous namespace to keep the function here

    /**
     * @brief deletePeaks Delete the banned peaks
     *
     * @param banned The indexes of peaks to delete.
     * @param peakPosToFit   Delete elements of this array.
     * @param peakPosFitted  Delete elements of this array.
     * @param peakHighFitted Delete elements of this array.
     * @param chisq          Delete elements of this array.
     */
    void deletePeaks(std::vector<size_t> &banned,
                      std::vector<double> &peakPosToFit,
                      std::vector<double> &peakPosFitted,
                      std::vector<double> &peakHighFitted,
                      std::vector<double> &chisq,
                      std::vector<double> &vecDeltaDovD)
    {
      if (banned.empty())
        return;

      for (std::vector<size_t>::const_reverse_iterator it = banned.rbegin(); it != banned.rend(); ++it)
      {
          peakPosToFit.erase(peakPosToFit.begin() + (*it));
          peakPosFitted.erase(peakPosFitted.begin() + (*it));
          peakHighFitted.erase(peakHighFitted.begin() + (*it));
          chisq.erase(chisq.begin() + (*it));
          vecDeltaDovD.erase(vecDeltaDovD.begin() + (*it));
      }
      banned.clear();

      return;
    }

  }


  //-----------------------------------------------------------------------------------------
  /** Calls Gaussian1D as a child algorithm to fit the offset peak in a spectrum
    *
    * @param wi :: The Workspace Index to fit.
    * @param inputW :: Input workspace.
    * @param peakPositions :: Peak positions.
    * @param fitWindows :: Fit windows.
    * @param nparams :: Number of parameters.
    * @param minD :: Min distance.
    * @param maxD :: Max distance.
    * @param peakPosToFit :: Actual peak positions to fit (output).
    * @param peakPosFitted :: Actual peak positions fitted (output).
    * @param chisq :: chisq.
    * @param peakHeights :: vector for fitted heights of peaks
    * @param i_highestpeak:: index of the highest peak among all peaks
    * @param resolution :: spectrum's resolution delta(d)/d
    * @param dev_resolution :: standard deviation resolution
    * @return The number of peaks in range
    */
  int GetDetOffsetsMultiPeaks::fitSpectra(const int64_t wi, MatrixWorkspace_sptr inputW,
                                          const std::vector<double> &peakPositions,
                                          const std::vector<double> &fitWindows, size_t &nparams,
                                          double &minD, double &maxD,
                                          std::vector<double>&peakPosToFit, std::vector<double>&peakPosFitted,
                                          std::vector<double> &chisq,
                                          std::vector<double> &peakHeights, int& i_highestpeak,
                                          double& resolution, double& dev_resolution)
  {
    // Default overall fit range is the whole spectrum
    const MantidVec & X = inputW->readX(wi);
    minD = X.front();
    maxD = X.back();

    // Trim in the edges based on where the data turns off of zero
    const MantidVec & Y = inputW->readY(wi);
    size_t minDindex = 0;
    for (; minDindex < Y.size(); ++minDindex)
    {
      if (Y[minDindex] > 0.)
      {
        minD = X[minDindex];
        break;
      }
    }
    if (minD >= maxD)
    {
      // throw if minD >= maxD
      std::stringstream ess;
      ess << "Stuff went wrong with wkspIndex=" << wi
          << " specIndex=" <<inputW->getSpectrum(wi)->getSpectrumNo();
      throw std::runtime_error(ess.str());
    }

    size_t maxDindex = Y.size()-1;
    for (; maxDindex > minDindex; --maxDindex)
    {
      if (Y[maxDindex] > 0.)
      {
        maxD = X[maxDindex];
        break;
      }
    }
    std::stringstream dbss;
    dbss << "D-RANGE[" << inputW->getSpectrum(wi)->getSpectrumNo() << "]: "
         << minD << " -> " << maxD;
    g_log.debug(dbss.str());

    // Setup the fit windows
    bool useFitWindows = (!fitWindows.empty());
    std::vector<double> fitWindowsToUse;
    for (int i = 0; i < static_cast<int>(peakPositions.size()); ++i)
    {
      if((peakPositions[i] > minD) && (peakPositions[i] < maxD))
      {
        if (m_useFitWindowTable)
        {
          fitWindowsToUse.push_back(std::max(m_vecFitWindow[wi][2*i], minD));
          fitWindowsToUse.push_back(std::min(m_vecFitWindow[wi][2*i+1], maxD));
        }
        else if (useFitWindows)
        {
          fitWindowsToUse.push_back(std::max(fitWindows[2*i], minD));
          fitWindowsToUse.push_back(std::min(fitWindows[2*i+1], maxD));
        }
        peakPosToFit.push_back(peakPositions[i]);
      }
    }
    int numPeaksInRange = static_cast<int>(peakPosToFit.size());
    if (numPeaksInRange == 0)
    {
      std::stringstream outss;
      outss << "Spectrum " << wi << " has no peak in range (" << minD << ", " << maxD << ")";
      g_log.information(outss.str()) ;
      return 0;
    }

    // Fit peaks
    API::IAlgorithm_sptr findpeaks = createChildAlgorithm("FindPeaks", -1, -1, false);
    findpeaks->setProperty("InputWorkspace", inputW);
    findpeaks->setProperty<int>("FWHM",7);
    findpeaks->setProperty<int>("Tolerance",4);
    // FindPeaks will do the checking on the validity of WorkspaceIndex
    findpeaks->setProperty("WorkspaceIndex",static_cast<int>(wi));

    //Get the specified peak positions, which is optional
    findpeaks->setProperty("PeakPositions", peakPosToFit);
    if (useFitWindows)
      findpeaks->setProperty("FitWindows", fitWindowsToUse);
    findpeaks->setProperty<std::string>("PeakFunction", m_peakType);
    findpeaks->setProperty<std::string>("BackgroundType", m_backType);
    findpeaks->setProperty<bool>("HighBackground", this->getProperty("HighBackground"));
    findpeaks->setProperty<int>("MinGuessedPeakWidth",4);
    findpeaks->setProperty<int>("MaxGuessedPeakWidth",4);
    findpeaks->setProperty<double>("MinimumPeakHeight", m_minPeakHeight);
    findpeaks->setProperty("StartFromObservedPeakCentre", true);
    findpeaks->executeAsChildAlg();

    // Collect fitting resutl of all peaks
    ITableWorkspace_sptr peakslist = findpeaks->getProperty("PeaksList");

    // use tmpPeakPosToFit to shuffle the vectors
    std::vector<double> tmpPeakPosToFit;
    generatePeaksList(peakslist, static_cast<int>(wi), peakPosToFit, tmpPeakPosToFit, peakPosFitted, peakHeights, chisq,
                      (useFitWindows || m_useFitWindowTable), fitWindowsToUse, minD, maxD, resolution, dev_resolution);
    peakPosToFit = tmpPeakPosToFit;

    nparams = peakPosFitted.size();

    // Find the highest peak
    i_highestpeak = -1;
    double maxheight = 0;
    for (int i = 0; i < static_cast<int>(peakPosFitted.size()); ++i)
    {
      double tmpheight = peakHeights[i];
      if (tmpheight > maxheight)
        {
          maxheight = tmpheight;
          i_highestpeak = i;
        }
    }

    return numPeaksInRange;
  }


  //----------------------------------------------------------------------------------------------
  /** Generate a list of peaks that meets= all the requirements for fitting offset
    * @param peakslist :: table workspace as the output of FindPeaks
    * @param wi :: workspace index of the spectrum
    * @param peakPositionRef :: reference peaks positions
    * @param peakPosToFit :: output of reference centres of the peaks used to fit offset
    * @param peakPosFitted :: output of fitted centres of the peaks used to fit offset
    * @param peakHeightFitted :: heights of the peaks used to fit offset
    * @param chisq :: chi squares of the peaks used to fit offset
    * @param useFitWindows :: boolean whether FitWindows is used
    * @param fitWindowsToUse :: fit windows
    * @param minD :: minimum d-spacing of the spectrum
    * @param maxD :: minimum d-spacing of the spectrum
    * @param deltaDovD :: delta(d)/d of the peak for fitting
    * @param dev_deltaDovD :: standard deviation of delta(d)/d of all the peaks in the spectrum
    */
  void GetDetOffsetsMultiPeaks::generatePeaksList(const API::ITableWorkspace_sptr &peakslist, int wi,
                                                  const std::vector<double>& peakPositionRef,
                                                  std::vector<double>& peakPosToFit,
                                                  std::vector<double>& peakPosFitted,
                                                  std::vector<double>& peakHeightFitted,
                                                  std::vector<double>& chisq,
                                                  bool useFitWindows, const std::vector<double>& fitWindowsToUse,
                                                  const double minD, const double maxD,
                                                  double& deltaDovD, double& dev_deltaDovD)
  {
    // FIXME - Need to make sure that the peakPositionRef and peakslist have the same order of peaks

    // Check
    size_t numrows = peakslist->rowCount();
    if (numrows != peakPositionRef.size())
      throw std::runtime_error("Number of peaks in PeaksList (from FindPeaks) is not same as number of "
                               "referenced peaks' positions. ");

    std::vector<double> vec_widthDivPos;
    std::vector<double> vec_offsets;

    for (size_t i = 0; i < peakslist->rowCount(); ++i)
    {
      // Get peak value
      double centre = peakslist->getRef<double>("centre",i);
      double width = peakslist->getRef<double>("width",i);
      double height = peakslist->getRef<double>("height", i);
      double chi2 = peakslist->getRef<double>("chi2",i);

      // Identify whether this peak would be accepted to optimize offset
      // - peak position within D-range
      if (centre <= minD || centre >= maxD)
      {
        std::stringstream dbss;
        dbss << " wi = " << wi << " c = " << centre << " out of D-range ";
        g_log.debug(dbss.str());
        continue;
      }

      // - rule out of peak with wrong position
      if (useFitWindows)
      {
        // outside peak fit window o
        if (centre <= fitWindowsToUse[2*i] || centre >= fitWindowsToUse[2*i+1])
        {
          std::stringstream dbss;
          dbss << " wi = " << wi << " c = " << centre << " out of fit window ";
          g_log.debug(dbss.str());
          continue;
        }
      }

      // - check chi-square
      if (chi2 > m_maxChiSq || chi2 < 0)
      {
        std::stringstream dbss;
        dbss << " wi = " << wi << " c = " << centre << " chi2 = " << chi2
             << ": Too large";
        g_log.debug(dbss.str());
        continue;
      }

      // - check peak height
      if (height < m_minPeakHeight)
      {
        g_log.debug() << " wi = " << wi << " c = " << centre << " h = " << height
                      << ": Too low " << "\n";
        continue;
      }

      // - check peak's resolution
      double widthdevpos = width/centre;
      if (m_hasInputResolution)
      {
        double recres = m_inputResolutionWS->readY(wi)[0];
        double resmax = recres*m_maxResFactor;
        double resmin = recres*m_minResFactor;
        if (widthdevpos < resmin || widthdevpos > resmax)
        {
          std::stringstream dbss;
          dbss << " wi = " << wi << " c = " << centre << " Delta(d)/d = " << widthdevpos
               << " too far away from suggested value " << recres;
          g_log.debug(dbss.str());
          continue;
        }
      }

      // background value
      double back_intercept = peakslist->getRef<double>("backgroundintercept", i);
      double back_slope = peakslist->getRef<double>("backgroundslope", i);
      double back_quad = peakslist->getRef<double>("A2", i);
      double background = back_intercept + back_slope * centre
          + back_quad * centre * centre;

      // Continue to identify whether this peak will be accepted
      // (e) peak signal/noise ratio
      if (height * FWHM_TO_SIGMA/width < 5.)
        continue;

      // (f) ban peaks that are not outside of error bars for the background
      if (height < 0.5 * std::sqrt(height + background))
        continue;

      // - calcualte offsets as to determine the (z-value)
      double offset = fabs(peakPositionRef[i]/centre - 1);
      if (offset > m_maxOffset)
      {
        std::stringstream dbss;
        dbss << " wi = " << wi << " c = " << centre << " exceeds maximum offset. ";
        g_log.debug(dbss.str());
        continue;
      }
      else vec_offsets.push_back(offset);

      // (g) calculate width/pos as to determine the (z-value) for constant "width" - (delta d)/d
      // double widthdevpos = width/centre;
      vec_widthDivPos.push_back(widthdevpos);

      // g_log.debug() << " h:" << height << " c:" << centre << " w:" << (width/(2.*std::sqrt(2.*std::log(2.))))
      //               << " b:" << background << " chisq:" << chi2 << "\n";

      // Add peak to vectors
      double refcentre = peakPositionRef[i];
      peakPosFitted.push_back(centre);
      peakPosToFit.push_back(refcentre);
      peakHeightFitted.push_back(height);
      chisq.push_back(chi2);
    }

    // Remove by Z-score on delta d/d
    std::vector<size_t> banned;
    std::vector<double> Zscore = getZscore(vec_widthDivPos);
    std::vector<double> Z_offset = getZscore(vec_offsets);
    for (size_t i = 0; i < peakPosFitted.size(); ++i)
    {
      if (Zscore[i] > 2.0 || Z_offset[i] > 2.0)
      {
        g_log.debug() << "Banning peak at " << peakPosFitted[i] << " in wkspindex = (no show)" // << wi
                      << " sigma/d = " << vec_widthDivPos[i] << "\n";
        banned.push_back(i);
        continue;
      }
    }

    // Delete banned peaks
    if (!banned.empty())
    {
      g_log.debug() << "Deleting " << banned.size() << " of " << peakPosFitted.size()
                    << " peaks in wkspindex = ??? " << "\n"; // << wi << "\n";

      deletePeaks(banned, peakPosToFit, peakPosFitted, peakHeightFitted, chisq, vec_widthDivPos);
    }

    Statistics widthDivPos = getStatistics(vec_widthDivPos);
    deltaDovD = widthDivPos.mean;
    dev_deltaDovD = widthDivPos.standard_deviation;

    return;
  }

  //----------------------------------------------------------------------------------------------
  /**
    */
  void GetDetOffsetsMultiPeaks::createInformationWorkspaces()
  {
    // Init
    size_t numspec = m_inputWS->getNumberHistograms();

    // Create output offset calculation status table
    m_infoTableWS = boost::make_shared<TableWorkspace>();

    // set up columns
    m_infoTableWS->addColumn("int", "WorkspaceIndex");
    m_infoTableWS->addColumn("int", "NumberPeaksFitted");
    m_infoTableWS->addColumn("int", "NumberPeaksInRange");
    m_infoTableWS->addColumn("str", "OffsetFitStatus");
    m_infoTableWS->addColumn("double", "ChiSquare");
    m_infoTableWS->addColumn("double", "Offset");
    m_infoTableWS->addColumn("double", "HighestPeakPosition");
    m_infoTableWS->addColumn("double", "HighestPeakDeviation");

    // add rows
    for (size_t i = 0; i < numspec; ++i)
    {
      TableRow newrow = m_infoTableWS->appendRow();
      newrow << static_cast<int>(i);
    }

    // Create output peak fitting information table
    m_peakOffsetTableWS = boost::make_shared<TableWorkspace>();

    // set up columns
    m_peakOffsetTableWS->addColumn("int", "WorkspaceIndex");
    for (size_t i = 0; i < m_peakPositions.size(); ++i)
    {
      std::stringstream namess;
      namess << "@" << std::setprecision(5) << m_peakPositions[i];
      m_peakOffsetTableWS->addColumn("str", namess.str());
    }
    m_peakOffsetTableWS->addColumn("double", "OffsetDeviation");

    // add rows
    for (size_t i = 0; i < numspec; ++i)
    {
      TableRow newrow = m_peakOffsetTableWS->appendRow();
      newrow << static_cast<int>(i);
    }

    // Create resolution (delta(d)/d) workspace
    m_resolutionWS = boost::dynamic_pointer_cast<MatrixWorkspace>(
          WorkspaceFactory::Instance().create("Workspace2D", numspec, 1, 1));

    return;
  }

  //----------------------------------------------------------------------------------------------
  /** Add result of offset-calculation to information table workspaces (thread-safe)
    */
  void GetDetOffsetsMultiPeaks::addInfoToReportWS(int wi, FitPeakOffsetResult offsetresult,
                                                  const std::vector<double>& tofitpeakpositions,
                                                  const std::vector<double>& fittedpeakpositions)
  {
    // Offset calculation status
    m_infoTableWS->cell<int>(wi, 1) = offsetresult.numpeaksfitted;
    m_infoTableWS->cell<int>(wi, 2) = offsetresult.numpeaksindrange;
    m_infoTableWS->cell<std::string>(wi, 3) = offsetresult.fitoffsetstatus;
    m_infoTableWS->cell<double>(wi, 4) = offsetresult.chi2;
    m_infoTableWS->cell<double>(wi, 5) = offsetresult.offset;
    m_infoTableWS->cell<double>(wi, 6) = offsetresult.highestpeakpos;
    m_infoTableWS->cell<double>(wi, 7) = offsetresult.highestpeakdev;

    // Peak width delta(d)/d
    m_resolutionWS->dataX(wi)[0] = static_cast<double>(wi);
    if (offsetresult.fitoffsetstatus.compare("success") == 0)
    {
      // Only add successfully calculated value
      m_resolutionWS->dataY(wi)[0] = offsetresult.resolution;
      m_resolutionWS->dataE(wi)[0] = offsetresult.dev_resolution;
    }
    else
    {
      // Only add successfully calculated value
      m_resolutionWS->dataY(wi)[0] = -0.0;
      m_resolutionWS->dataE(wi)[0] = 0.0;
    }

    // Peak-fitting information:  Record: (found peak position) - (target peak position)
    int numpeaksfitted = offsetresult.numpeaksfitted;
    if (numpeaksfitted > 0)
    {
      // Not all peaks in peakOffsetTable are in tofitpeakpositions/fittedpeakpositions
      std::vector<bool> haspeakvec(offsetresult.numpeakstofit, false);
      std::vector<double> deltavec(offsetresult.numpeakstofit, 0.0);

      // to calculate deviation from peak centre
      double sumdelta1 = 0.0;
      double sumdelta2 = 0.0;
      for (int i = 0; i < numpeaksfitted; ++i)
      {
        double peakcentre = tofitpeakpositions[i];
        int index = static_cast<int>(
              std::lower_bound(m_peakPositions.begin(), m_peakPositions.end(), peakcentre)
              - m_peakPositions.begin());
        if (index > 0 && (m_peakPositions[index]-peakcentre > peakcentre-m_peakPositions[index-1]))
        {
          --index;
        }
        haspeakvec[index] = true;
        deltavec[index] = peakcentre - fittedpeakpositions[i];

        sumdelta1 += deltavec[index]/tofitpeakpositions[i];
        sumdelta2 += deltavec[index] * deltavec[index] / (tofitpeakpositions[i]*tofitpeakpositions[i]);
      }

      double numdelta = static_cast<double>(numpeaksfitted);
      double stddev = 0.;
      if (numpeaksfitted > 1)
        stddev = sqrt(sumdelta2/numdelta - (sumdelta1/numdelta)*(sumdelta1/numdelta));

      // Set the peak positions to workspace and
      for (int i = 0; i < offsetresult.numpeakstofit; ++i)
      {
        if (haspeakvec[i])
        {
          std::stringstream ss;
          ss << deltavec[i];

          int icol = i+1;
          m_peakOffsetTableWS->cell<std::string>(wi, icol) = ss.str();
        }
      }

      // Final statistic
      size_t icol = m_peakOffsetTableWS->columnCount()-1;
      m_peakOffsetTableWS->cell<double>(wi, icol) = stddev;
    }

    return;
  }

  //----------------------------------------------------------------------------------------------
  /** Clean peak offset table workspace
    */
  void GetDetOffsetsMultiPeaks::removeEmptyRowsFromPeakOffsetTable()
  {
    size_t numrows = m_infoTableWS->rowCount();
    if (m_peakOffsetTableWS->rowCount() != numrows)
    {
      g_log.warning("Peak position offset workspace has different number of rows to "
                    "that of offset fitting information workspace. "
                    "No row will be removed from peak position offset table workspace. ");
      return;
    }

    size_t icurrow = 0;
    for (size_t i = 0; i < numrows; ++i)
    {
      // Criteria 1 dev is not equal to zero
      bool removerow = false;
      int numpeakfitted = m_infoTableWS->cell<int>(i, 1);
      if (numpeakfitted == 0)
      {
        removerow = true;
      }

      // Remove row
      if (removerow)
      {
        m_peakOffsetTableWS->removeRow(icurrow);
      }
      else
      {
        // advance to next row
        ++ icurrow;
      }
    }

    return;
  }


  //----------------------------------------------------------------------------------------------
  /** Make a summary of fitting
    */
  void GetDetOffsetsMultiPeaks::makeFitSummary()
  {
    g_log.notice("Making summary... ...");
    size_t numrows = m_infoTableWS->rowCount();
    double sumchi2 = 0;
    double weight_sumchi2 = 0;
    size_t weight_numfittedpeaks = 0;
    size_t numunmasked = 0;
    for (size_t i = 0; i < numrows; ++i)
    {
      TableRow row = m_infoTableWS->getRow(i);
      int wi, numpeakfitted, numpeakinrange;
      double chi2, offset;
      std::string fitofsetstatus;
      row >> wi >> numpeakfitted >> numpeakinrange >> fitofsetstatus >> chi2 >> offset;
      if (numpeakfitted * numpeakinrange > 0)
      {
        ++ numunmasked;
        sumchi2 += chi2;
        weight_sumchi2 += static_cast<double>(numpeakfitted*chi2);
        weight_numfittedpeaks += numpeakfitted;
      }
    }

    double avgchi2 = sumchi2/static_cast<double>(numunmasked);
    double wtavgchi2 = weight_sumchi2/static_cast<double>(weight_numfittedpeaks);

    g_log.notice() << "Unmasked spectra     = " << numunmasked << " \t"
                   << "Average chi-sq       = " << avgchi2 << " \t"
                   << "Weighted avg. chi-sq = " << wtavgchi2 << "\n";

  }

} // namespace Algorithm
} // namespace Mantid
