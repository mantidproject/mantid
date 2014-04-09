/*WIKI*

== Description ==

This algorithm requires a workspace that is both in d-spacing, but has also been preprocessed by the [[CrossCorrelate]] algorithm.  In this first step you select one spectrum to be the reference spectrum and all of the other spectrum are cross correlated against it.  Each output spectrum then contains a peak whose location defines the offset from the reference spectrum.

The algorithm iterates over each spectrum in the workspace and fits a [[Gaussian]] function to the reference peak.  The fit is used to calculate the centre of the fitted peak, and the offset is then calculated as:

This is then written into a [[CalFile|.cal file]] for every detector that contributes to that spectrum.  All of the entries in the cal file are initially set to both be included, but also to all group into a single group on [[DiffractionFocussing]].  The [[CreateCalFileByNames]] algorithm can be used to alter the grouping in the cal file.

=== Fit for peak offset ===
The algorithm to calculate offset of peaks' positions is to
minimize a cost function as
:<math> \sum_{p} |X_{0, p} - (1+offset)\cdot X_{0, p}|/\chi^2_{p} </math>
, which p is the index of a peak whose position is within MinD and MaxD.

==== Spectra to mask ====
* Empty spectrum marked as "empty det"

* Spectrum with counts less than 1.0E^-3 in defined d-range as "dead det"

* Calculated offset exceeds the user-defined maximum offset.

==== Criteria on peaks ====
The (fitted) peak must meet a series of criteria to be used to fit spectrum's offset.

A peak will not be used if
* its centre is out of pre-defined d-range, i.e., MinD and MaxD;
* its centre is out of fitting window if it is defined;
* its <math>\chi^2</math> of peak fitting is larger than pre-defined maximum value;
* its height is lower than pre-defined lowest peak height;
* its signal/noise ratio is less than 5 <math> H\cdot FWHM\_To\_SIGMA/width < 5</math>;
* its height is not outside of error bars of background <math> H < \sqrt{H + B}/2 </math>;
* its z-value on <math>\frac{\delta d}{d} </math> is larger than 2.0.

=== Generate fit window ===
* Required parameter: maxWidth.  If it is not given, i.e., less or equal to zero, then there won't be any window defined;
* Definition of fit window for peaks indexed from 0 to N-1
** Peak 0: window = Min((X0_0-dmin), maxWidth), Min((X0_1-X0_0)/2, maxWidth)
** Peak i (0 < i < N-1): window = Min((X0_i-X0_{i-1})/2, maxWidth), Min((X0_1-X0_0)/2, maxWidth)
** Peak N-1: window = Min((X0_i-X0_{i-1})/2, maxWidth), Min((dmax-X0_i), maxWidth)
where X0_i is the centre of i-th peak.


== Fitting Quality ==
GetDetOffsetsMultiPeaks have 2 levels of fitting.  First it will call FindPeaks to fit Bragg peaks within d-range.
Then it will fit offsets from the peak positions obtained in the previous step.
Therefore, the performance of FindPeaks is critical to this algorithm.
It is necessary to output values reflecting the goodness of fitting of this algorithm to users.

=== Number of spectra that are NOT masked ===
A spectrum will be masked if it is a dead pixel, has an empty detector or has no peak that can be fit with given peak positions.
The performance of ''FindPeaks'' affects the third criteria.
A better algorithm to find and fit peaks may save some spectrum with relatively much fewer events received, i.e., poorer signal.

=== <math>\chi^2</math> of the offset fitting function ===
The goodness of fit, <math>\chi^2_{iws}</math>, of the offset fitting function
:<math> \sum_{p} |X_{0, p} - (1+offset)X_{0, p}|/\chi^2_{p} </math>
is an important measure of fitting quality on each spectrum (indexed as iws).

=== Deviation of highest peaks ===
We observed that in some situation, the calibrated peaks' positions of some spectra are far off to the targeted peak positions,
while goodness of fit such as <math>\chi^2</math> are still good.
It is usally caused by the bad fit of one or two peaks in that spectrum,
which feeds some erroreous peak positions to peak offset fitting function.

This type of bad fitting is very easily identified by visualization,
because the shift of peaks from the correct positions is significant in fill plot.

Therefore, deviation of highest peak if spectrum i, <math>D_{i}</math> is defined as:
:<math> D_{i} = |X^{(o)}\cdots(1+offset) - X^{(c)}|</math>
where <math>X^{(o)}</math> is the fitted centre of the highest peak of spectrum i,
and <math>X^{(c)}</math> is the theoretical centre of this peak.

=== Collective quantities to illustrate goodness of fitting (still in developement) ===
Be noticed that the idea of this section is still under development and has not been implemented  yet.

On the other hand, since GetDetOffsetsMultiPeaks always operates on an EventWorkspace with thousands
or several ten thousands of spectra,
it is very hard to tell the quality of fitting by looking at <math>\chi^2_{iws}</math> of all spectra.
Hence, Here are two other parameters are defined for comparison of results.

: <math>g_1 = \frac{\sum_{s}D_{s}^2}{N_{nm}}</math>
, where s is the index of any unmasked spectrum and <math>N_{mn}</math> is the number of unmasked spectra;

: <math>g_2 = \frac{\sum_{s}D_{s}^2\cdot H_{s}^2}{N_{nm}}</math>,
where <math>H_{s}</math> is the height of highest peak of spectrum s.

=== Standard error on offset ===
The offset in unit of d-spacing differs is proportional to peak's position by definition:
:<math> X_0^{(f)} = X_0^{(o)} * (1+offset) </math>
where <math>X_0^{(f)}</math> is the focussed peak position, and <math>X_0^{(o)}</math> is the observed peak position by fitting.

As different spectrum covers different d-space range, the highest peak differs.
Therefore, the error of offset should be normalized by the peak's position.
:<math> E = (X_0^{(f)} - X_0^{(o)}*(1+offset))/X_0^{(f)} = 1 - \frac{X_0^{(o)}}{X_0^{(f)}}\cdot(1+offset) </math>
And it is unitless.

By this mean, the error of all peaks should be close if they are fitted correctly.


== Usage ==
'''Python'''

OutputW,NumberPeaksFitted,Mask = GetDetOffsetsMultiPeaks("InputW",0.01,2.0,1.8,2.2,"output.cal")


*WIKI*/
#include "MantidAlgorithms/GetDetOffsetsMultiPeaks.h"
#include "MantidAlgorithms/GSLFunctions.h"
#include "MantidAPI/FileProperty.h"
#include "MantidAPI/FunctionFactory.h"
#include "MantidAPI/WorkspaceValidators.h"
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
      * cost = \sum_{p}|d^0_p - (1+offset)*d^{(f)}_p|/\chi_p, where d^{(f)} is within minD and maxD
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
      std::vector<double> chisq(n);
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
        chisq[i] = p[i+2*n+3];
      }

      double offset = gsl_vector_get(v, 0);
      double errsum = 0.0;
      for (size_t i = 0; i < n; ++i)
      {
        // Get references to the data
        //See formula in AlignDetectors
        double peakPosMeas = (1.+offset)*peakPosFitted[i];
        if(peakPosFitted[i] > minD && peakPosFitted[i] < maxD)
          errsum += std::fabs(peakPosToFit[i]-peakPosMeas)/chisq[i];
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
  /** Sets documentation strings for this algorithm
    */
  void GetDetOffsetsMultiPeaks::initDocs()
  {
    this->setWikiSummary("Creates an [[OffsetsWorkspace]] containing offsets for each detector. "
                         "You can then save these to a .cal file using SaveCalFile.");
    this->setOptionalMessage("Creates an OffsetsWorkspace containing offsets for each detector. "
                             "You can then save these to a .cal file using SaveCalFile.");
  }

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
    //Disable default gsl error handler (which is to call abort!)
    gsl_set_error_handler_off();

    auto tablewsprop = new WorkspaceProperty<TableWorkspace>("SpectraFitInfoTableWorkspace", "FitInfoTable", Direction::Output);
    declareProperty(tablewsprop, "Name of the output table workspace containing spectra peak fit information.");

    auto offsetwsprop = new WorkspaceProperty<TableWorkspace>("PeaksOffsetTableWorkspace", "PeakOffsetTable", Direction::Output);
    declareProperty(offsetwsprop, "Name of an output table workspace containing peaks' offset data.");

  }

  //----------------------------------------------------------------------------------------------
  /** Process input and output properties
    */
  void GetDetOffsetsMultiPeaks::processProperties()
  {
    inputW=getProperty("InputWorkspace");

    // determine min/max d-spacing of the workspace
    double wkspDmin, wkspDmax;
    inputW->getXMinMax(wkspDmin, wkspDmax);

    // the peak positions and where to fit
    m_peakPositions = getProperty("DReference");
    std::sort(m_peakPositions.begin(), m_peakPositions.end());

    // Fit windows
    double maxwidth = getProperty("FitWindowMaxWidth");
    m_fitWindows = generateWindows(wkspDmin, wkspDmax, m_peakPositions, maxwidth);

    // Debug otuput
    std::stringstream infoss;
    infoss << "Fit Windows : ";
    if (m_fitWindows.empty())
    {
      infoss << "(empty)";
    }
    else
    {
      for (std::vector<double>::const_iterator it = m_fitWindows.begin(); it != m_fitWindows.end(); ++it)
        infoss << *it << " ";
    }
    g_log.information(infoss.str());

    if (m_fitWindows.size() == 0)
    {
      g_log.warning() << "Input FitWindowMaxWidth = " << maxwidth
                      << "  No FitWidows will be generated." << "\n";
    }

    // Some shortcuts for event workspaces
    eventW = boost::dynamic_pointer_cast<const EventWorkspace>( inputW );
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
    maxOffset=getProperty("MaxOffset");

    // Create output workspaces
    outputW = boost::make_shared<OffsetsWorkspace>(inputW->getInstrument());
    outputNP = boost::make_shared<OffsetsWorkspace>(inputW->getInstrument());
    MatrixWorkspace_sptr tempmaskws(new MaskWorkspace(inputW->getInstrument()));
    maskWS = tempmaskws;

    return;
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
    size_t numspec = inputW->getNumberHistograms();

    // Process output workspaces: output information table and peak offset
    m_infoTableWS = createOutputInfoTable(numspec);
    setProperty("SpectraFitInfoTableWorkspace", m_infoTableWS);

    m_peakOffsetTableWS = createOutputPeakOffsetTable(numspec);
    setProperty("PeaksOffsetTableWorkspace", m_peakOffsetTableWS);

    //*************************************************************************
    // Calculate offset of each detector
    //*************************************************************************
    int nspec=static_cast<int>(inputW->getNumberHistograms());
    //To get the workspace index from the detector ID
    const detid2index_map pixel_to_wi = maskWS->getDetectorIDToWorkspaceIndexMap(true);

    // Fit all the spectra with a gaussian
    Progress prog(this, 0, 1.0, nspec);

    // cppcheck-suppress syntaxError
    PRAGMA_OMP(parallel for schedule(dynamic, 1) )
    for (int wi=0;wi<nspec;++wi)
    {
      PARALLEL_START_INTERUPT_REGION

      std::vector<double> fittedpeakpositions, tofitpeakpositions;
      FitPeakOffsetResult offsetresult = calculatePeakOffset(wi, fittedpeakpositions, tofitpeakpositions);

      // Get the list of detectors in this pixel
      const std::set<detid_t> & dets = inputW->getSpectrum(wi)->getDetectorIDs();

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
          if ( mapEntry == pixel_to_wi.end() )
            continue;
          const size_t workspaceIndex = mapEntry->second;
          if (offsetresult.mask == 1.)
          {
            // Being masked
            maskWS->maskWorkspaceIndex(workspaceIndex);
            maskWS->dataY(workspaceIndex)[0] = offsetresult.mask;
          }
          else
          {
            // Using the detector
            maskWS->dataY(workspaceIndex)[0] = offsetresult.mask;
          }
        } // ENDFOR (detectors)

        addInfoToReportWS(wi, offsetresult, tofitpeakpositions, fittedpeakpositions);

      } // End of critical region


      prog.report();
      PARALLEL_END_INTERUPT_REGION
    }
    PARALLEL_CHECK_INTERUPT_REGION

    // Return the output
    setProperty("OutputWorkspace",outputW);
    setProperty("NumberPeaksWorkspace",outputNP);
    setProperty("MaskWorkspace",maskWS);

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

    // Clean peak offset table workspace
    removeEmptyRowsFromPeakOffsetTable();

    // Make summary
    makeFitSummary();

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
    // fr.chisqSum = 0.0;

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
      const MantidVec& Y = inputW->readY(wi);
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
      size_t nparams;
      double minD, maxD;
      int i_highestpeak;
      fr.numpeaksindrange = fitSpectra(wi, inputW, m_peakPositions, m_fitWindows,
                                       nparams, minD, maxD, vec_peakPosRef, vec_peakPosFitted,
                                       vec_fitChi2, i_highestpeak);
      fr.numpeakstofit = static_cast<int>(m_peakPositions.size());
      fr.numpeaksfitted = static_cast<int>(vec_peakPosFitted.size());

      // Fit offset
      if (nparams > 0 && fr.numpeaksindrange > 0)
      {
        fitPeaksOffset(nparams, minD, maxD, vec_peakPosRef, vec_peakPosFitted, vec_fitChi2, fr);

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
        g_log.debug() << "Spectra " << wi << " has 0 parameter for it.  Set to bad_offset." << ".\n";
        fr.offset = BAD_OFFSET;
        fr.fitoffsetstatus = "no peaks";
      }
    }

    // Final check offset
    fr.mask = 0.0;
    if (std::abs(fr.offset) > maxOffset)
    {
      fr.mask = 1.0;
      fr.offset = 0.0;
      if (fr.fitoffsetstatus == "success")
        fr.fitoffsetstatus = "exceed max offset";
    }

    return fr;
  } /// ENDFUNCTION: GetDetOffsetsMultiPeaks

  //----------------------------------------------------------------------------------------------
  /** Fit peaks' offset by minimize the fitting function
    */
  void GetDetOffsetsMultiPeaks::fitPeaksOffset(const size_t inpnparams, const double minD, const double maxD,
                                               const std::vector<double>& vec_peakPosRef,
                                               const std::vector<double>& vec_peakPosFitted,
                                               const std::vector<double>& vec_fitChi2,
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
      params[i+3+2*nparams] = vec_fitChi2[i];
      fitresult.chisqSum += vec_fitChi2[i];
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
    gsl_vector_free(x);
    gsl_vector_free(ss);
    gsl_multimin_fminimizer_free (s);

    fitresult.fitoffsetstatus = reportOfDiffractionEventCalibrateDetectors;
    fitresult.chi2 = s->fval;

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
    void deletePeaks2(std::vector<size_t> &banned,
                      std::vector<double> &peakPosToFit,
                      std::vector<double> &peakPosFitted,
                      std::vector<double> &peakHighFitted,
                      std::vector<double> &chisq)
    {
      if (banned.empty())
        return;

      for (std::vector<size_t>::const_reverse_iterator it = banned.rbegin(); it != banned.rend(); ++it)
      {
          peakPosToFit.erase(peakPosToFit.begin() + (*it));
          peakPosFitted.erase(peakPosFitted.begin() + (*it));
          peakHighFitted.erase(peakHighFitted.begin() + (*it));
          chisq.erase(chisq.begin() + (*it));
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
    * @param i_highestpeak:: index of the highest peak among all peaks
    * @return The number of peaks in range
    */
  int GetDetOffsetsMultiPeaks::fitSpectra(const int64_t wi, MatrixWorkspace_sptr inputW,
                                          const std::vector<double> &peakPositions,
                                          const std::vector<double> &fitWindows, size_t &nparams,
                                          double &minD, double &maxD,
                                          std::vector<double>&peakPosToFit, std::vector<double>&peakPosFitted,
                                          std::vector<double> &chisq, int& i_highestpeak)
  {
    // default overall fit range is the whole spectrum
    const MantidVec & X = inputW->readX(wi);
    minD = X.front();
    maxD = X.back();

    // trim in the edges based on where the data turns off of zero
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
      std::cout << "Stuff went wrong with wkspIndex=" << wi
                << " specIndex=" <<inputW->getSpectrum(wi)->getSpectrumNo() << std::endl;
    size_t maxDindex = Y.size()-1;
    for (; maxDindex > minDindex; --maxDindex)
    {
      if (Y[maxDindex] > 0.)
      {
        maxD = X[maxDindex];
        break;
      }
    }
    g_log.debug() << "D-RANGE[" << inputW->getSpectrum(wi)->getSpectrumNo() << "]: "
                  << minD << " -> " << maxD << "\n";

    // setup the fit windows
    bool useFitWindows = (!fitWindows.empty());
    std::vector<double> fitWindowsToUse;
    for (int i = 0; i < static_cast<int>(peakPositions.size()); ++i)
    {
      if((peakPositions[i] > minD) && (peakPositions[i] < maxD))
      {
        if (useFitWindows)
        {
          fitWindowsToUse.push_back(std::max(fitWindows[2*i], minD));
          fitWindowsToUse.push_back(std::min(fitWindows[2*i+1], maxD));
        }
        peakPosToFit.push_back(peakPositions[i]);
      }
    }
    int numPeaksInRange = static_cast<int>(peakPosToFit.size());
    if (numPeaksInRange == 0) return 0;

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
    findpeaks->executeAsChildAlg();

    // Collect fitting resutl of all peaks
    ITableWorkspace_sptr peakslist = findpeaks->getProperty("PeaksList");

    std::vector<double> peakHeightFitted;
    std::vector<double> tmpPeakPosToFit;
    generatePeaksList(peakslist, static_cast<int>(wi), peakPosToFit, tmpPeakPosToFit, peakPosFitted, peakHeightFitted, chisq,
                      useFitWindows, fitWindowsToUse, minD, maxD);
    peakPosToFit = tmpPeakPosToFit;

    nparams = peakPosFitted.size();

    // Find the highest peak
    i_highestpeak = -1;
    double maxheight = 0;
    for (int i = 0; i < static_cast<int>(peakPosFitted.size()); ++i)
    {
      double tmpheight = peakHeightFitted[i];
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
    */
  void GetDetOffsetsMultiPeaks::generatePeaksList(const API::ITableWorkspace_sptr &peakslist, int wi,
                                                  const std::vector<double>& peakPositionRef,
                                                  std::vector<double>& peakPosToFit,
                                                  std::vector<double>& peakPosFitted,
                                                  std::vector<double>& peakHeightFitted,
                                                  std::vector<double>& chisq,
                                                  bool useFitWindows, const std::vector<double>& fitWindowsToUse,
                                                  const double minD, const double maxD)
  {
    // FIXME - Need to make sure that the peakPositionRef and peakslist have the same order of peaks
    std::vector<double> vec_widthDivPos;

    for (size_t i = 0; i < peakslist->rowCount(); ++i)
    {
      // peak value
      double centre = peakslist->getRef<double>("centre",i);
      double width = peakslist->getRef<double>("width",i);
      double height = peakslist->getRef<double>("height", i);
      double chi2 = peakslist->getRef<double>("chi2",i);

      // Identify whether this peak would be accepted to optimize offset
      // (a) peak position within D-range
      if (centre <= minD || centre >= maxD)
      {
        g_log.debug() << " wi = " << wi << " c = " << centre << " out of D-range "
                      << "\n";
        continue;
      }

      // (b) peak position inside peak window
      if (useFitWindows)
      {
        if (centre <= fitWindowsToUse[2*i] || centre >= fitWindowsToUse[2*i+1])
        {
          g_log.debug() << " wi = " << wi << " c = " << centre << " out of fit window "
                        << "\n";
          continue;
        }
      }

      // (c) check chi-square
      if (chi2 > m_maxChiSq || chi2 < 0)
      {
        g_log.debug() << " wi = " << wi << " c = " << centre << " chi2 = " << chi2
                      << ": Too large" << "\n";
        continue;
      }

      // (d) check peak height
      if (height < m_minPeakHeight)
      {
        g_log.debug() << " wi = " << wi << " c = " << centre << " h = " << height
                      << ": Too low " << "\n";
        continue;
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

      // (g) calculate width/pos as to determine the (z-value) for constant "width" - (delta d)/d
      double widthdevpos = width/centre;
      vec_widthDivPos.push_back(widthdevpos);

      g_log.debug() << " h:" << height << " c:" << centre << " w:" << (width/(2.*std::sqrt(2.*std::log(2.))))
                    << " b:" << background << " chisq:" << chi2 << "\n";

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
    for (size_t i = 0; i < peakPosFitted.size(); ++i)
    {
      if (Zscore[i] > 2.0)
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

      deletePeaks2(banned, peakPosToFit, peakPosFitted, peakHeightFitted, chisq);
    }

    return;
  }


  //----------------------------------------------------------------------------------------------
  /** Create information table workspace for output
    */
  TableWorkspace_sptr GetDetOffsetsMultiPeaks::createOutputInfoTable(size_t numspec)
  {
    // Create table workspace and set columns
    TableWorkspace_sptr infoTableWS = boost::make_shared<TableWorkspace>();

    infoTableWS->addColumn("int", "WorkspaceIndex");
    infoTableWS->addColumn("int", "NumberPeaksFitted");
    infoTableWS->addColumn("int", "NumberPeaksInRange");
    infoTableWS->addColumn("str", "OffsetFitStatus");
    infoTableWS->addColumn("double", "ChiSquare");
    infoTableWS->addColumn("double", "Offset");
    infoTableWS->addColumn("double", "HighestPeakPosition");
    infoTableWS->addColumn("double", "HighestPeakDeviation");

    // Add rows
    for (size_t i = 0; i < numspec; ++i)
    {
      TableRow newrow = infoTableWS->appendRow();
      newrow << static_cast<int>(i);
    }

    return infoTableWS;
  }

  //----------------------------------------------------------------------------------------------
  /** Create peak offset table workspace for output
    */
  TableWorkspace_sptr GetDetOffsetsMultiPeaks::createOutputPeakOffsetTable(size_t numspec)
  {
    // Craete table workspace and set columns
    TableWorkspace_sptr peakOffsetTableWS = boost::make_shared<TableWorkspace>();

    peakOffsetTableWS->addColumn("int", "WorkspaceIndex");
    for (size_t i = 0; i < m_peakPositions.size(); ++i)
    {
      std::stringstream namess;
      namess << "@" << std::setprecision(5) << m_peakPositions[i];
      peakOffsetTableWS->addColumn("str", namess.str());
    }
    peakOffsetTableWS->addColumn("double", "OffsetDeviation");

    // Add rows
    for (size_t i = 0; i < numspec; ++i)
    {
      TableRow newrow = peakOffsetTableWS->appendRow();
      newrow << static_cast<int>(i);
    }

    return peakOffsetTableWS;
  }

  //----------------------------------------------------------------------------------------------
  /** Add result of offset-calculation to information table workspaces
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
