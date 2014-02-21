/*WIKI*

This algorithm requires a workspace that is both in d-spacing, but has also been preprocessed by the [[CrossCorrelate]] algorithm.  In this first step you select one spectrum to be the reference spectrum and all of the other spectrum are cross correlated against it.  Each output spectrum then contains a peak whose location defines the offset from the reference spectrum.

The algorithm iterates over each spectrum in the workspace and fits a [[Gaussian]] function to the reference peak.  The fit is used to calculate the centre of the fitted peak, and the offset is then calculated as:

This is then written into a [[CalFile|.cal file]] for every detector that contributes to that spectrum.  All of the entries in the cal file are initially set to both be included, but also to all group into a single group on [[DiffractionFocussing]].  The [[CreateCalFileByNames]] algorithm can be used to alter the grouping in the cal file.

== Fit for peak offset ==
The algorithm to calculate offset of peaks' positions is to
minimize a cost function as
<math> \sum_{pi} |X_{0, pi} - (1+offset)X_{0, pi}|/chi^2_{pi} </math>
, which pi is the index of a peak whose position is within MinD and MaxD.

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
The goodness of fit, <math>\chi^2_{iws}</math>, of the offset fitting function <math> \sum_{pi} |X_{0, pi} - (1+offset)X_{0, pi}|/chi^2_{pi} </math>
is an important measure of fitting quality on each spectrum (indexed as iws).

On the other hand, since GetDetOffsetsMultiPeaks always operates on an EventWorkspace with thousands or several ten thousands of spectra,
it is very hard to tell the quality of fitting by looking at <math>\chi^2_{iws}</math> of all spectra.
Hence, Here are two other parameters are defined for comparison of results.
1. <math>g_1</math> = \frac{\sum_{iws}\chi^2_{iws}}{N_{nm}}</math>, where iws is the index of any unmasked spectrum and <math>N_{mn}</math> is the number of unmasked spectra;

2. <math>g_2</math> = \frac{\sum_{iws}\chi^2_{iws}/p_{iws}}{N_{nm}}</math>, where <math>p_{iws}</math> is the number of peaks to fit for offset of spectrum iws.

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

For a collective view,
* <math>d_1</math> = \frac{\sum_{iws}D_i^2}{N_{nm}}</math>, where iws is the index of any unmasked spectrum and <math>N_{mn}</math> is the number of unmasked spectra;

* <math>g_2</math> = \frac{\sum_{iws}D^2_{iws}\cdot\H_{iws}^2}}{N_{nm}}</math>, where <math>H_{iws}</math> is the height of highest peak of spectrum iws. Be noted that this value is not normalized.

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

  // Register the class into the algorithm factory
  DECLARE_ALGORITHM(GetDetOffsetsMultiPeaks)

  /// Sets documentation strings for this algorithm
  void GetDetOffsetsMultiPeaks::initDocs()
  {
    this->setWikiSummary("Creates an [[OffsetsWorkspace]] containing offsets for each detector. You can then save these to a .cal file using SaveCalFile.");
    this->setOptionalMessage("Creates an OffsetsWorkspace containing offsets for each detector. You can then save these to a .cal file using SaveCalFile.");
  }

  using namespace Kernel;
  using namespace API;
  using std::size_t;
  using namespace DataObjects;

  /// Constructor
  GetDetOffsetsMultiPeaks::GetDetOffsetsMultiPeaks() :
    API::Algorithm()
  {}

  /// Destructor
  GetDetOffsetsMultiPeaks::~GetDetOffsetsMultiPeaks()
  {}
  

  //-----------------------------------------------------------------------------------------
  /** Initialisation method. Declares properties to be used in algorithm.
     */
  void GetDetOffsetsMultiPeaks::init()
  {

    declareProperty(new WorkspaceProperty<>("InputWorkspace","",Direction::Input,
                                            boost::make_shared<WorkspaceUnitValidator>("dSpacing")),"A 2D matrix workspace with X values of d-spacing");

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

    declareProperty("OutputFitSummary", false, "If specified, a summary on fitting peak summary is made as log's notice level. ");

  }

  //----------------------------------------------------------------------------------------------
  /**
     * The windows should be half of the distance between the peaks of maxWidth, whichever is smaller.
     * @param dmin The minimum d-spacing for the workspace
     * @param dmax The maximum d-spacing for the workspace
     * @param peaks The list of peaks to generate windows for
     * @param maxWidth The maximum width of a window
     * @return The list of windows for each peak
     */
  std::vector<double> generateWindows(const double dmin, const double dmax,
                                      const std::vector<double> &peaks, const double maxWidth)
  {
    if (maxWidth <= 0.)
    {
      return std::vector<double>(); // empty vector because this is turned off
    }

    std::size_t numPeaks = peaks.size();
    std::vector<double> windows(2*numPeaks);
    double widthLeft;
    double widthRight;
    for (std::size_t i = 0; i < numPeaks; i++)
    {
      if (i == 0)
        widthLeft = peaks[i] - dmin;
      else
        widthLeft = .5 * (peaks[i] - peaks[i-1]);
      if (i + 1 == numPeaks)
        widthRight = dmax - peaks[i];
      else
        widthRight = .5 * (peaks[i+1] - peaks[i]);

      if (maxWidth > 0)
      {
        widthLeft  = std::min(widthLeft, maxWidth);
        widthRight = std::min(widthRight, maxWidth);
      }

      windows[2*i]   = peaks[i] - widthLeft;
      windows[2*i+1] = peaks[i] + widthRight;
    }

    return windows;
  }


  void GetDetOffsetsMultiPeaks::processProperties()
  {
    // MatrixWorkspace_sptr inputW=getProperty("InputWorkspace");
    inputW=getProperty("InputWorkspace");
    maxOffset=getProperty("MaxOffset");

    // determine min/max d-spacing of the workspace
    double wkspDmin, wkspDmax;
    inputW->getXMinMax(wkspDmin, wkspDmax);


    // the peak positions and where to fit
    // std::vector<double> peakPositions = getProperty("DReference");
    peakPositions = getProperty("DReference");
    std::sort(peakPositions.begin(), peakPositions.end());
    // std::vector<double> fitWindows = generateWindows(wkspDmin, wkspDmax, peakPositions, this->getProperty("FitWindowMaxWidth"));
    fitWindows = generateWindows(wkspDmin, wkspDmax, peakPositions, this->getProperty("FitWindowMaxWidth"));
    g_log.information() << "windows : ";
    if (fitWindows.empty())
    {
      g_log.information() << "empty\n";
    }
    else
    {
      for (std::vector<double>::const_iterator it = fitWindows.begin(); it != fitWindows.end(); ++it)
        g_log.information() << *it << " ";
      g_log.information() << "\n";
    }

    // some shortcuts for event workspaces
    // EventWorkspace_const_sptr eventW = boost::dynamic_pointer_cast<const EventWorkspace>( inputW );
    eventW = boost::dynamic_pointer_cast<const EventWorkspace>( inputW );
    // bool isEvent = false;
    isEvent = false;
    if (eventW)
      isEvent = true;

    // cache the peak and background function names
    m_peakType = this->getPropertyValue("PeakFunction");
    m_backType = this->getPropertyValue("BackgroundType");
    // the maximum allowable chisq value for an individual peak fit
    m_maxChiSq = this->getProperty("MaxChiSq");
    m_minPeakHeight = this->getProperty("MinimumPeakHeight");

    // Create output workspaces
    outputW = boost::make_shared<OffsetsWorkspace>(inputW->getInstrument());
    // outputNP = boost::make_shared<OffsetsWorkspace>(OffsetsWorkspace(inputW->getInstrument()));
    outputNP = boost::make_shared<OffsetsWorkspace>(inputW->getInstrument());
    MatrixWorkspace_sptr tempmaskws(new MaskWorkspace(inputW->getInstrument()));
    maskWS = tempmaskws;

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

    // Process output workspaces: output information table and peak offset
    m_infoTableWS = createOutputInfoTable();
    setProperty("SpectraFitInfoTableWorkspace", m_infoTableWS);

    m_peakOffsetTableWS = createOutputPeakOffsetTable();
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

        // Add report to TableWorkspace
        // FIXME - In initialization, add all row.  Here, use cell(i, j, value) directly
        //         in order to output workspaces in numerical order
        TableRow newrow = m_infoTableWS->appendRow();
        newrow << wi << offsetresult.numpeaksfitted << offsetresult.numpeaksindrange
               << offsetresult.fitoffsetstatus << offsetresult.chi2 << offsetresult.offset
               << offsetresult.highestpeakpos << offsetresult.highestpeakdev;

        // Add report to offset info table
        // FIXME - This does not make much sense...
        // TODO - Best way is to re-fit the spectra with given offset and see how good
        //        the peak positions are matching the theoretical value
        // Record: (found peak position) - (target peak position)
        // FIXME/TODO - Add all rows in initialization.  In this step, use setCell(i, j, value)
        TableRow newrow2 = m_peakOffsetTableWS->appendRow();
        newrow2 << wi;
        if (offsetresult.numpeaksfitted > 0)
        {
          std::vector<bool> haspeakvec(offsetresult.numpeakstofit, false);
          std::vector<double> deltavec(offsetresult.numpeakstofit, 0.0);
          for (int i = 0; i < offsetresult.numpeaksfitted; ++i)
          {
            double peakcentre = tofitpeakpositions[i];
            int index = static_cast<int>(
                  std::lower_bound(peakPositions.begin(), peakPositions.end(), peakcentre)
                  - peakPositions.begin());
            if (index > 0 && (peakPositions[index]-peakcentre > peakcentre-peakPositions[index-1]))
            {
              --index;
            }
            haspeakvec[index] = true;
            deltavec[index] = peakcentre - fittedpeakpositions[i];
          }

          double sumdelta1 = 0.0;
          double sumdelta2 = 0.0;
          double numdelta = 0.0;
          for (int i = 0; i < offsetresult.numpeakstofit; ++i)
          {
            if (haspeakvec[i])
            {
              std::stringstream ss;
              ss << deltavec[i];
              newrow2 << ss.str();

              sumdelta1 += deltavec[i];
              sumdelta2 += deltavec[i] * deltavec[i];
              numdelta += 1.0;
            }
            else
            {
              newrow2 << "";
            }
          }

          // Final statistic
          double stddev = sumdelta2/numdelta - (sumdelta1/numdelta)*(sumdelta1/numdelta);
          newrow2 << stddev;

        } // ENDIF (numpeaksfitted > 0)

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

    // Make summary
    bool dosummary = getProperty("OutputFitSummary");
    if (dosummary)
      makeFitSummary();

    return;
  }

  //-----------------------------------------------------------------------------------------
  namespace { // anonymous namespace to keep the function here
    /**
     * @brief deletePeaks Delete the banned peaks
     *
     * @param banned The indexes of peaks to delete.
     * @param peakPosToFit   Delete elements of this array.
     * @param peakPosFitted  Delete elements of this array.
     * @param peakWidFitted  Delete elements of this array.
     * @param peakHighFitted Delete elements of this array.
     * @param peakBackground Delete elements of this array.
     * @param chisq          Delete elements of this array.
     */
    void deletePeaks(std::vector<size_t> &banned,
                     std::vector<double>&peakPosToFit,
                     std::vector<double>&peakPosFitted,
                     std::vector<double> &peakWidFitted,
                     std::vector<double> &peakHighFitted,
                     std::vector<double> &peakBackground,
                     std::vector<double> &chisq)
    {
      if (banned.empty())
        return;

      for (std::vector<size_t>::const_reverse_iterator it = banned.rbegin(); it != banned.rend(); ++it)
      {
          peakPosToFit.erase(peakPosToFit.begin() + (*it));
          peakPosFitted.erase(peakPosFitted.begin() + (*it));
          peakWidFitted.erase(peakWidFitted.begin() + (*it));
          peakHighFitted.erase(peakHighFitted.begin() + (*it));
          peakBackground.erase(peakBackground.begin() + (*it));
          chisq.erase(chisq.begin() + (*it));
      }
      banned.clear();
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
    * @param peakPosToFit :: Peak positions to fit (output).
    * @param peakPosFitted :: Peak positions fitted (output).
    * @param chisq :: chisq.
    * @return The number of peaks in range
    */
  int GetDetOffsetsMultiPeaks::fitSpectra(const int64_t wi, MatrixWorkspace_sptr inputW, const std::vector<double> &peakPositions,
                                           const std::vector<double> &fitWindows, size_t &nparams, double &minD, double &maxD,
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
    std::vector<size_t> banned;
    std::vector<double> peakWidFitted;
    std::vector<double> peakHighFitted;
    std::vector<double> peakBackground;
    for (size_t i = 0; i < peakslist->rowCount(); ++i)
    {
      // peak value
      double centre = peakslist->getRef<double>("centre",i);
      double width = peakslist->getRef<double>("width",i);
      double height = peakslist->getRef<double>("height", i);

      // background value
      double back_intercept = peakslist->getRef<double>("backgroundintercept", i);
      double back_slope = peakslist->getRef<double>("backgroundslope", i);
      double back_quad = peakslist->getRef<double>("A2", i);
      double background = back_intercept + back_slope * centre
          + back_quad * centre * centre;

      // goodness of fit
      double chi2 = peakslist->getRef<double>("chi2",i);

      g_log.debug() << " h:" << height << " c:" << centre << " w:" << (width/(2.*std::sqrt(2.*std::log(2.))))
                    << " b:" << background << " chisq:" << chi2 << "\n";

      // Get references to the data
      peakPosFitted.push_back(centre);
      peakWidFitted.push_back(width);
      peakHighFitted.push_back(height);
      peakBackground.push_back(background);
      chisq.push_back(chi2);
    }

    // first remove things that just didn't fit (center outside of window, bad chisq, ...)
    for (size_t i = 0; i < peakslist->rowCount(); ++i)
    {
      if (peakPosFitted[i] <= minD || peakPosFitted[i] >= maxD)
      {
        // ban peaks from big D-range
        banned.push_back(i);
        continue;
      }
      else if (useFitWindows) // be more restrictive if fit windows were specified
      {
        if (peakPosFitted[i] <= fitWindowsToUse[2*i]
            || peakPosFitted[i] >= fitWindowsToUse[2*i+1])
        {
          // ban peaks from fit window
          banned.push_back(i);
          continue;
        }
      }
      if (chisq[i] > m_maxChiSq || chisq[i] < 0.)
      {
        // ban peaks from HUGE (or negative) chi-square
        banned.push_back(i);
        continue;
      }
      if (peakHighFitted[i] < m_minPeakHeight)
      {
        // ban peaks that don't meet the minimum height requirement
        banned.push_back(i);
        continue;
      }
    }
    // delete banned peaks
    if (!banned.empty())
      g_log.debug() << "Deleting " << banned.size() << " of " << peakPosFitted.size()
                    << " peaks in wkspindex = " << wi << "\n";

    deletePeaks(banned, peakPosToFit, peakPosFitted,
                peakWidFitted, peakHighFitted, peakBackground,
                chisq);

    // ban peaks that are low intensity compared to their widths
    for (size_t i = 0; i < peakWidFitted.size(); ++i)
    {
      if (peakHighFitted[i]  * FWHM_TO_SIGMA / peakWidFitted[i] < 5.)
      {
        g_log.debug() << "Banning peak at " << peakPosFitted[i] << " in wkspindex = " << wi
                      << " I/sigma = " << (peakHighFitted[i] * FWHM_TO_SIGMA / peakWidFitted[i]) << "\n";
        banned.push_back(i);
        continue;
      }
    }
    // delete banned peaks
    if (!banned.empty())
      g_log.debug() << "Deleting " << banned.size() << " of " << peakPosFitted.size()
                    << " peaks in wkspindex = " << wi << "\n";
      deletePeaks(banned, peakPosToFit, peakPosFitted,
                  peakWidFitted, peakHighFitted, peakBackground,
                  chisq);

      // determine the (z-value) for constant "width" - (delta d)/d
      std::vector<double> widthDivPos(peakWidFitted.size(), 0.); // DELETEME
      for (size_t i = 0; i < peakWidFitted.size(); ++i)
      {
        widthDivPos[i] = peakWidFitted[i] / peakPosFitted[i]; // DELETEME
      }
      std::vector<double> Zscore = getZscore(widthDivPos);
      for (size_t i = 0; i < peakWidFitted.size(); ++i)
      {
        if (Zscore[i] > 2.0)
        {
          g_log.debug() << "Banning peak at " << peakPosFitted[i] << " in wkspindex = " << wi
                         << " sigma/d = " << widthDivPos[i] << "\n";
          banned.push_back(i);
          continue;
        }
      }
      // delete banned peaks
      if (!banned.empty())
        g_log.debug() << "Deleting " << banned.size() << " of " << peakPosFitted.size()
                      << " peaks in wkspindex = " << wi << "\n";
      deletePeaks(banned, peakPosToFit, peakPosFitted,
                  peakWidFitted, peakHighFitted, peakBackground,
                  chisq);

      // ban peaks that are not outside of error bars for the background
      for (size_t i = 0; i < peakWidFitted.size(); ++i)
      {
        if (peakHighFitted[i] < 0.5 * std::sqrt(peakHighFitted[i] + peakBackground[i]))
        {
          g_log.debug() << "Banning peak at " << peakPosFitted[i] << " in wkspindex = " << wi
                         << " " << peakHighFitted[i] << " < "
                         << 0.5 * std::sqrt(peakHighFitted[i] + peakBackground[i]) << "\n";
          banned.push_back(i);
          continue;
        }
      }
      // delete banned peaks
      if (!banned.empty())
        g_log.debug() << "Deleting " << banned.size() << " of " << peakPosFitted.size()
                      << " peaks in wkspindex = " << wi << "\n";
      deletePeaks(banned, peakPosToFit, peakPosFitted,
                  peakWidFitted, peakHighFitted, peakBackground,
                  chisq);

      nparams = peakPosFitted.size();

      // Find the highest peak
      i_highestpeak = -1;
      double maxheight = 0;
      for (int i = 0; i < static_cast<int>(peakPosFitted.size()); ++i)
      {
        double tmpheight = peakHighFitted[i];
        if (tmpheight > maxheight)
        {
          maxheight = tmpheight;
          i_highestpeak = i;
        }
      }

      return numPeaksInRange;
    }

  //----------------------------------------------------------------------------------------------
  /** Create information table workspace for output
    */
  TableWorkspace_sptr GetDetOffsetsMultiPeaks::createOutputInfoTable()
  {
    auto infoTableWS = boost::make_shared<TableWorkspace>();

    infoTableWS->addColumn("int", "WorkspaceIndex");
    infoTableWS->addColumn("int", "NumberPeaksFitted");
    infoTableWS->addColumn("int", "NumberPeaksInRange");
    infoTableWS->addColumn("str", "OffsetFitStatus");
    infoTableWS->addColumn("double", "ChiSquare");
    infoTableWS->addColumn("double", "Offset");
    infoTableWS->addColumn("double", "HighestPeakPosition");
    infoTableWS->addColumn("double", "HighestPeakDeviation");

    return infoTableWS;
  }

  //----------------------------------------------------------------------------------------------
  /** Create peak offset table workspace for output
    */
  TableWorkspace_sptr GetDetOffsetsMultiPeaks::createOutputPeakOffsetTable()
  {
    auto m_peakOffsetTableWS = boost::make_shared<TableWorkspace>();
    m_peakOffsetTableWS->addColumn("int", "WorkspaceIndex");
    for (size_t i = 0; i < peakPositions.size(); ++i)
    {
      std::stringstream namess;
      namess << "@" << std::setprecision(5) << peakPositions[i];
      m_peakOffsetTableWS->addColumn("str", namess.str());
    }
    m_peakOffsetTableWS->addColumn("double", "OffsetDeviation");

    return m_peakOffsetTableWS;
  }

  //----------------------------------------------------------------------------------------------
  /** Calculate offset for one spectrum
    */
  FitPeakOffsetResult GetDetOffsetsMultiPeaks::calculatePeakOffset(const int wi, std::vector<double>& fittedpeakpositions, std::vector<double>& tofitpeakpositions)
  {
	// Initialize the structure to return
	FitPeakOffsetResult fr;

    fr.offset = 0.0;
    fr.fitoffsetstatus = "N/A";
    fr.chi2 = -1;

    fr.fitSum = 0.0;
    fr.chisqSum = 0.0;

    // Number of peak fitted of this spectrum
    fr.peakPosFittedSize = 0.0;

    fr.numpeaksfitted = 0;
    fr.numpeakstofit = 0;
    fr.numpeaksindrange = 0;

    // checks for dead detectors
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
        fr.offset=BAD_OFFSET;
        fr.fitoffsetstatus = "dead det";
      }
    }

    if (fr.offset < 10.)
    {
      // Fit the peak
      std::vector<double> peakPosToFit, peakPosFitted, chisq;
      size_t nparams;
      double minD, maxD;
      int i_highestpeak;
      fr.numpeaksindrange =
          fitSpectra(wi, inputW, peakPositions, fitWindows, nparams, minD, maxD,
                     peakPosToFit, peakPosFitted, chisq, i_highestpeak);
      fr.numpeakstofit = static_cast<int>(peakPositions.size());
      fr.numpeaksfitted = static_cast<int>(peakPosFitted.size());
      fittedpeakpositions = peakPosFitted;
      tofitpeakpositions = peakPosToFit;

      // Fit offset
      if (nparams > 0 && fr.numpeaksindrange > 0)
      {
        //double * params = new double[2*nparams+1];
        double params[153];
        if(nparams > 50) nparams = 50;
        params[0] = static_cast<double>(nparams);
        params[1] = minD;
        params[2] = maxD;
        for (size_t i = 0; i < nparams; i++)
        {
          params[i+3] = peakPosToFit[i];
        }
        for (size_t i = 0; i < nparams; i++)
        {
          params[i+3+nparams] = peakPosFitted[i];
        }
        fr.peakPosFittedSize = static_cast<double>(peakPosFitted.size());
        for (size_t i = 0; i < nparams; i++)
        {
          params[i+3+2*nparams] = chisq[i];
          fr.chisqSum += chisq[i];
        }

        const gsl_multimin_fminimizer_type *T = gsl_multimin_fminimizer_nmsimplex;
        gsl_multimin_fminimizer *s = NULL;
        gsl_vector *ss, *x;
        gsl_multimin_function minex_func;

        // finally do the fitting
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
        g_log.debug() << " Workspace Index = " << wi <<
          " Method used = " << " Simplex" <<
          " Iteration = " << iter <<
          " Status = " << reportOfDiffractionEventCalibrateDetectors <<
          " Minimize Sum = " << s->fval <<
          " Offset   = " << gsl_vector_get (s->x, 0) << "  \n";
        fr.offset = gsl_vector_get (s->x, 0);
        fr.fitSum = s->fval;
        gsl_vector_free(x);
        gsl_vector_free(ss);
        gsl_multimin_fminimizer_free (s);

        fr.fitoffsetstatus = reportOfDiffractionEventCalibrateDetectors;
        fr.chi2 = s->fval;

        // Deviation of calibrated position to the strong peak
        double highpeakpos = peakPosFitted[i_highestpeak];
        double highpeakpos_target = peakPosToFit[i_highestpeak];
        if (fr.fitoffsetstatus == "success")
        {
          fr.highestpeakpos  = highpeakpos;
          fr.highestpeakdev = fabs(highpeakpos*(1+fr.offset) - highpeakpos_target);
        }
        else
        {
          fr.highestpeakpos = 0.0;
          fr.highestpeakdev = -1.0;
        }

        // FIXME - Whether chi2 is normalized by number of peaks entering minimization
        //delete [] params;
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
    fr.mask=0.0;
    if (std::abs(fr.offset) > maxOffset)
    {
      fr.offset = 0.0;
      fr.mask = 1.0;
    }

    return fr;
  } /// ENDFUNCTION: GetDetOffsetsMultiPeaks

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
