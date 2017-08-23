#include "MantidAlgorithms/CalculateDetOffsetsMultiPeaks.h"
#include "MantidAPI/CompositeFunction.h"
#include "MantidAPI/FileProperty.h"
#include "MantidAPI/FunctionFactory.h"
#include "MantidAPI/FuncMinimizerFactory.h"
#include "MantidAPI/ITableWorkspace.h"
#include "MantidAPI/IPeakFunction.h"
#include "MantidAPI/IBackgroundFunction.h"
#include "MantidAPI/TableRow.h"
#include "MantidAPI/SpectrumInfo.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidAPI/WorkspaceUnitValidator.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidDataObjects/MaskWorkspace.h"
#include "MantidDataObjects/OffsetsWorkspace.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/ListValidator.h"
#include "MantidKernel/StartsWithValidator.h"
#include "MantidKernel/Statistics.h"
#include "MantidKernel/VectorHelper.h"

#include <gsl/gsl_multifit_nlin.h>
#include <gsl/gsl_multimin.h>

#include <sstream>

namespace Mantid {
namespace Algorithms {
namespace {
/// Factor to convert full width half max to sigma for calculations of I/sigma.
const double FWHM_TO_SIGMA = 2.0 * sqrt(2.0 * M_LN2);
const double BAD_OFFSET(1000.); // mark things that didn't work with this

//--------------------------------------------------------------------------------------------
/** Helper function for calculating costs in gsl.
  * cost = \sum_{p}|d^0_p - (1+offset)*d^{(f)}_p|\cdot H^2_p, where d^{(f)} is
 * within minD and maxD
   * @param v Vector of offsets.
   * @param params Array of input parameters.
   * @returns Sum of the errors.
   */
double gsl_costFunction(const gsl_vector *v, void *params) {
  // FIXME - there is no need to use vectors peakPosToFit, peakPosFitted and
  // chisq
  double *p = reinterpret_cast<double *>(params);
  size_t n = static_cast<size_t>(p[0]);
  std::vector<double> peakPosToFit(n);
  std::vector<double> peakPosFitted(n);
  std::vector<double> height2(n);
  double minD = p[1];
  double maxD = p[2];
  for (size_t i = 0; i < n; i++) {
    peakPosToFit[i] = p[i + 3];
  }
  for (size_t i = 0; i < n; i++) {
    peakPosFitted[i] = p[i + n + 3];
  }
  for (size_t i = 0; i < n; i++) {
    height2[i] = p[i + 2 * n + 3];
  }

  double offset = gsl_vector_get(v, 0);
  double errsum = 0.0;
  for (size_t i = 0; i < n; ++i) {
    // Get references to the data
    // See formula in AlignDetectors
    double peakPosMeas = (1. + offset) * peakPosFitted[i];
    if (peakPosFitted[i] > minD && peakPosFitted[i] < maxD)
      errsum += std::fabs(peakPosToFit[i] - peakPosMeas) * height2[i];
  }
  return errsum;
}
}

using namespace Kernel;
using namespace API;
using std::size_t;
using namespace DataObjects;

// Register the class into the algorithm factory
DECLARE_ALGORITHM(CalculateDetOffsetsMultiPeaks)

//----------------------------------------------------------------------------------------------
/** Constructor
  */
CalculateDetOffsetsMultiPeaks::CalculateDetOffsetsMultiPeaks()
    : m_inputWS(), m_maxChiSq(0.), m_maxOffset(0.), m_peakPositions(),
      m_minResFactor(0.),
      m_maxResFactor(0.), m_outputW(), m_outputNP(), m_maskWS(),
      m_infoTableWS(), m_peakOffsetTableWS(), m_resolutionWS(),
      m_useFitWindowTable(false), m_vecFitWindow() {}

//----------------------------------------------------------------------------------------------
/** Initialisation method. Declares properties to be used in algorithm.
   */
void CalculateDetOffsetsMultiPeaks::init() {
  declareProperty(make_unique<WorkspaceProperty<>>(
                      "InputWorkspace", "", Direction::Input),
                  "A 2D matrix workspace containing peak positions.");

  declareProperty(make_unique<ArrayProperty<double>>("DReference"),
                  "Enter a comma-separated list of the expected X-position of "
                  "the centre of the peaks. Only peaks near these positions "
                  "will be fitted.");

  declareProperty(make_unique<FileProperty>("GroupingFileName", "",
                                            FileProperty::OptionalSave, ".cal"),
                  "Optional: The name of the output CalFile to save the "
                  "generated OffsetsWorkspace.");

  declareProperty(make_unique<WorkspaceProperty<OffsetsWorkspace>>(
                      "OutputWorkspace", "", Direction::Output),
                  "An output workspace containing the offsets.");
  declareProperty(
      make_unique<WorkspaceProperty<OffsetsWorkspace>>(
          "NumberPeaksWorkspace", "NumberPeaksFitted", Direction::Output),
      "An output workspace containing the offsets.");

  declareProperty(make_unique<WorkspaceProperty<>>("MaskWorkspace", "Mask",
                                                   Direction::Output),
                  "An output workspace containing the mask.");

  declareProperty("MaxOffset", 1.0,
                  "Maximum absolute value of offsets; default is 1");

  declareProperty(
      "MaxChiSq", 100.,
      "Maximum chisq value for individual peak fit allowed. (Default: 100)");

  // Disable default gsl error handler (which is to call abort!)
  gsl_set_error_handler_off();

  declareProperty(
      make_unique<WorkspaceProperty<TableWorkspace>>(
          "SpectraFitInfoTableWorkspace", "FitInfoTable", Direction::Output),
      "Name of the output table workspace containing "
      "spectra peak fit information.");

  declareProperty(
      make_unique<WorkspaceProperty<TableWorkspace>>(
          "PeaksOffsetTableWorkspace", "PeakOffsetTable", Direction::Output),
      "Name of an output table workspace containing peaks' offset data.");

  declareProperty(
      make_unique<WorkspaceProperty<MatrixWorkspace>>(
          "FittedResolutionWorkspace", "ResolutionWS", Direction::Output),
      "Name of the resolution workspace containing "
      "delta(d)/d for each unmasked spectrum. ");

  declareProperty("MinimumResolutionFactor", 0.1,
                  "Factor of the minimum allowed Delta(d)/d of any peak to its "
                  "suggested Delta(d)/d. ");

  declareProperty("MaximumResolutionFactor", 10.0,
                  "Factor of the maximum allowed Delta(d)/d of any peak to its "
                  "suggested Delta(d)/d. ");
}

//-----------------------------------------------------------------------------------------
/** Executes the algorithm
   *
   *  @throw Exception::FileError If the grouping file cannot be opened or read
  *successfully
   */
void CalculateDetOffsetsMultiPeaks::exec() {
  // Process input information
  processProperties();

  // Create information workspaces
  createInformationWorkspaces();

  // Calculate offset of each detector
  calculateDetectorsOffsets();

  // Return the output
  setProperty("OutputWorkspace", m_outputW);
  setProperty("NumberPeaksWorkspace", m_outputNP);
  setProperty("MaskWorkspace", m_maskWS);
  setProperty("FittedResolutionWorkspace", m_resolutionWS);
  setProperty("SpectraFitInfoTableWorkspace", m_infoTableWS);
  setProperty("PeaksOffsetTableWorkspace", m_peakOffsetTableWS);

  // Also save to .cal file, if requested
  std::string filename = getProperty("GroupingFileName");
  if (!filename.empty()) {
    progress(0.9, "Saving .cal file");
    IAlgorithm_sptr childAlg = createChildAlgorithm("SaveCalFile");
    childAlg->setProperty("OffsetsWorkspace", m_outputW);
    childAlg->setProperty("MaskWorkspace", m_maskWS);
    childAlg->setPropertyValue("Filename", filename);
    childAlg->executeAsChildAlg();
  }

  // Make summary
  progress(0.92, "Making summary");
}

//----------------------------------------------------------------------------------------------
/** Process input and output properties
  */
void CalculateDetOffsetsMultiPeaks::processProperties() {
  m_inputWS = getProperty("InputWorkspace");

  // the peak positions and where to fit
  m_peakPositions = getProperty("DReference");
  if (m_peakPositions.empty())
    throw std::runtime_error("There is no input referenced peak position.");
  std::sort(m_peakPositions.begin(), m_peakPositions.end());

  // Some shortcuts for event workspaces
  m_eventW = boost::dynamic_pointer_cast<const EventWorkspace>(m_inputWS);
  // bool m_isEvent = false;
  m_isEvent = false;
  if (m_eventW)
    m_isEvent = true;

  // Cache the peak and background function names

  // The maximum allowable chisq value for an individual peak fit
  m_maxChiSq = this->getProperty("MaxChiSq");
  m_minimizer = getPropertyValue("Minimizer");
  m_maxOffset = getProperty("MaxOffset");

  // Create output workspaces
  m_outputW = boost::make_shared<OffsetsWorkspace>(m_inputWS->getInstrument());
  m_outputNP = boost::make_shared<OffsetsWorkspace>(m_inputWS->getInstrument());
  MatrixWorkspace_sptr tempmaskws =
      boost::make_shared<MaskWorkspace>(m_inputWS->getInstrument());
  m_maskWS = tempmaskws;
}

//-----------------------------------------------------------------------------------------
/** Calculate (all) detectors' offsets
  */
void CalculateDetOffsetsMultiPeaks::calculateDetectorsOffsets() {
  int nspec = static_cast<int>(m_inputWS->getNumberHistograms());

  // To get the workspace index from the detector ID
  const detid2index_map pixel_to_wi =
      m_maskWS->getDetectorIDToWorkspaceIndexMap(true);

  // Fit all the spectra with a gaussian
  Progress prog(this, 0.0, 1.0, nspec);

  auto &spectrumInfo = m_maskWS->mutableSpectrumInfo();
  // cppcheck-suppress syntaxError
    PRAGMA_OMP(parallel for schedule(dynamic, 1) )
    for (int wi = 0; wi < nspec; ++wi) {
      PARALLEL_START_INTERUPT_REGION

      std::vector<double> fittedpeakpositions, tofitpeakpositions;
      FitPeakOffsetResult offsetresult =
          calculatePeakOffset(wi, fittedpeakpositions, tofitpeakpositions);

      // Get the list of detectors in this pixel
      const auto &dets = m_inputWS->getSpectrum(wi).getDetectorIDs();

      // Most of the exec time is in FitSpectra, so this critical block should
      // not be a problem.
      PARALLEL_CRITICAL(CalculateDetOffsetsMultiPeaks_setValue) {
        // Use the same offset for all detectors from this pixel (in case of
        // summing pixels)
        std::set<detid_t>::iterator it;
        for (it = dets.begin(); it != dets.end(); ++it) {
          // Set value to output peak offset workspace
          m_outputW->setValue(*it, offsetresult.offset, offsetresult.fitSum);

          // Set value to output peak number workspace
          m_outputNP->setValue(*it, offsetresult.peakPosFittedSize,
                               offsetresult.chisqSum);

          // Set value to mask workspace
          const auto mapEntry = pixel_to_wi.find(*it);
          if (mapEntry == pixel_to_wi.end())
            continue;

          const size_t workspaceIndex = mapEntry->second;
          if (offsetresult.mask > 0.9) {
            // Being masked
            m_maskWS->getSpectrum(workspaceIndex).clearData();
            spectrumInfo.setMasked(workspaceIndex, true);
            m_maskWS->mutableY(workspaceIndex)[0] = offsetresult.mask;
          } else {
            // Using the detector
            m_maskWS->mutableY(workspaceIndex)[0] = offsetresult.mask;

          }
        } // ENDFOR (detectors)

        // Report offset fitting result/status
        addInfoToReportWS(wi, offsetresult, tofitpeakpositions,
                          fittedpeakpositions);

      } // End of critical region

      prog.report();
      PARALLEL_END_INTERUPT_REGION
    }
    PARALLEL_CHECK_INTERUPT_REGION
}

//----------------------------------------------------------------------------------------------
/** Calculate offset for one spectrum
  */
FitPeakOffsetResult CalculateDetOffsetsMultiPeaks::calculatePeakOffset(
    const int wi, std::vector<double> &vec_peakPosFitted,
    std::vector<double> &vec_peakPosRef) {
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

  fr.highestpeakpos = 0.0;
  fr.highestpeakdev = 0.0;
  fr.resolution = 0.0;
  fr.dev_resolution = 0.0;

  // Checks for empty and dead detectors
  if ((m_isEvent) && (m_eventW->getSpectrum(wi).empty())) {
    // empty detector will be masked
    fr.offset = BAD_OFFSET;
    fr.fitoffsetstatus = "empty det";
  } else {
    // dead detector will be masked
    const auto &Y = m_inputWS->y(wi);
    const int YLength = static_cast<int>(Y.size());
    double sumY = 0.0;
    size_t numNonEmptyBins = 0;
    for (int i = 0; i < YLength; i++) {
      sumY += Y[i];
      if (Y[i] > 0.)
        numNonEmptyBins += 1;
    }
    if (sumY < 1.e-30) {
      // Dead detector will be masked
      fr.offset = BAD_OFFSET;
      fr.fitoffsetstatus = "dead det";
    }
    if (numNonEmptyBins <= 3) {
      // Another dead detector check
      fr.offset = BAD_OFFSET;
      fr.fitoffsetstatus = "dead det";
    }
  }

  // Calculate peak offset for 'good' detector
  if (fr.offset < 10.) {
    // Fit peaks
    // std::vector<double> vec_peakPosRef, vec_peakPosFitted;
    std::vector<double> vec_fitChi2;
    std::vector<double> vec_peakHeights;
    size_t nparams;
    double minD, maxD;
    int i_highestpeak;
    double resolution, devresolution;
    //    fr.numpeaksindrange =
    //        fitSpectra(wi, m_inputWS, m_peakPositions, m_fitWindows, nparams,
    //        minD,
    //                   maxD, vec_peakPosRef, vec_peakPosFitted, vec_fitChi2,
    //                   vec_peakHeights, i_highestpeak, resolution,
    //                   devresolution);
    fr.numpeakstofit = static_cast<int>(m_peakPositions.size());
    fr.numpeaksfitted = static_cast<int>(vec_peakPosFitted.size());
    fr.resolution = resolution;
    fr.dev_resolution = devresolution;

    // Fit offset
    if (nparams > 0 && fr.numpeaksindrange > 0) {
      fitPeaksOffset(nparams, minD, maxD, vec_peakPosRef, vec_peakPosFitted,
                     vec_peakHeights, fr);

      // Deviation of calibrated position to the strong peak
      if (fr.fitoffsetstatus == "success") {
        double highpeakpos = vec_peakPosFitted[i_highestpeak];
        double highpeakpos_target = vec_peakPosRef[i_highestpeak];
        fr.highestpeakpos = highpeakpos;
        fr.highestpeakdev =
            fabs(highpeakpos * (1 + fr.offset) - highpeakpos_target);
      } else {
        fr.highestpeakpos = 0.0;
        fr.highestpeakdev = -1.0;
      }
    } else {
      // Not enough peaks have been found.
      // Output warning
      std::stringstream outss;
      outss << "Spectra " << wi
            << " has 0 parameter for it.  Set to bad_offset.";
      g_log.debug(outss.str());
      fr.offset = BAD_OFFSET;
      fr.fitoffsetstatus = "no peaks";
    }
  }

  // Final check offset
  fr.mask = 0.0;
  if (std::abs(fr.offset) > m_maxOffset) {
    std::stringstream infoss;
    infoss << "Spectrum " << wi << " has offset = " << fr.offset
           << ", which exceeds maximum offset " << m_maxOffset
           << ".  Spectrum is masked. ";
    g_log.information(infoss.str());

    std::stringstream msgss;
    if (fr.fitoffsetstatus == "success")
      msgss << "exceed max offset. "
            << "offset = " << fr.offset;
    else
      msgss << fr.fitoffsetstatus << ". "
            << "offset = " << fr.offset;
    fr.fitoffsetstatus = msgss.str();

    fr.mask = 1.0;
    fr.offset = 0.0;
  }

  return fr;
} /// ENDFUNCTION: calculatePeakOffset

//----------------------------------------------------------------------------------------------
/** Fit peaks' offset by minimize the fitting function
  */
void CalculateDetOffsetsMultiPeaks::fitPeaksOffset(
    const size_t inpnparams, const double minD, const double maxD,
    const std::vector<double> &vec_peakPosRef,
    const std::vector<double> &vec_peakPosFitted,
    const std::vector<double> &vec_peakHeights,
    FitPeakOffsetResult &fitresult) {
  // Set up array for minimization/optimization by GSL library
  size_t nparams = inpnparams;
  if (nparams > 50)
    nparams = 50;

  double params[153];
  params[0] = static_cast<double>(nparams);
  params[1] = minD;
  params[2] = maxD;
  for (size_t i = 0; i < nparams; i++) {
    params[i + 3] = vec_peakPosRef[i];
  }
  for (size_t i = 0; i < nparams; i++) {
    params[i + 3 + nparams] = vec_peakPosFitted[i];
  }

  // the reason to put these codes here is that nparams may be altered in this
  // method
  fitresult.peakPosFittedSize = static_cast<double>(vec_peakPosFitted.size());
  for (size_t i = 0; i < nparams; i++) {
    params[i + 3 + 2 * nparams] =
        (vec_peakHeights[i] * vec_peakHeights[i]); // vec_fitChi2[i];
    fitresult.chisqSum +=
        1. / (vec_peakHeights[i] * vec_peakHeights[i]); // vec_fitChi2[i];
  }

  // Set up GSL minimzer
  const gsl_multimin_fminimizer_type *T = gsl_multimin_fminimizer_nmsimplex;
  gsl_multimin_fminimizer *s = nullptr;
  gsl_vector *ss, *x;
  gsl_multimin_function minex_func;

  // Finally do the fitting
  size_t nopt = 1;
  size_t iter = 0;
  int status = 0;
  double size;

  /* Starting point */
  x = gsl_vector_alloc(nopt);
  gsl_vector_set_all(x, 0.0);

  /* Set initial step sizes to 0.001 */
  ss = gsl_vector_alloc(nopt);
  gsl_vector_set_all(ss, 0.001);

  /* Initialize method and iterate */
  minex_func.n = nopt;
  minex_func.f = &gsl_costFunction;
  minex_func.params = &params;

  s = gsl_multimin_fminimizer_alloc(T, nopt);
  gsl_multimin_fminimizer_set(s, &minex_func, x, ss);

  do {
    iter++;
    status = gsl_multimin_fminimizer_iterate(s);
    if (status)
      break;

    size = gsl_multimin_fminimizer_size(s);
    status = gsl_multimin_test_size(size, 1e-4);

  } while (status == GSL_CONTINUE && iter < 50);

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
  fitresult.offset = gsl_vector_get(s->x, 0);
  fitresult.fitSum = s->fval;

  fitresult.fitoffsetstatus = reportOfDiffractionEventCalibrateDetectors;
  fitresult.chi2 = s->fval;

  gsl_vector_free(x);
  gsl_vector_free(ss);
  gsl_multimin_fminimizer_free(s);
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
void deletePeaks(std::vector<size_t> &banned, std::vector<double> &peakPosToFit,
                 std::vector<double> &peakPosFitted,
                 std::vector<double> &peakHighFitted,
                 std::vector<double> &chisq,
                 std::vector<double> &vecDeltaDovD) {
  if (banned.empty())
    return;

  for (std::vector<size_t>::const_reverse_iterator it = banned.rbegin();
       it != banned.rend(); ++it) {
    peakPosToFit.erase(peakPosToFit.begin() + (*it));
    peakPosFitted.erase(peakPosFitted.begin() + (*it));
    peakHighFitted.erase(peakHighFitted.begin() + (*it));
    chisq.erase(chisq.begin() + (*it));
    vecDeltaDovD.erase(vecDeltaDovD.begin() + (*it));
  }
  banned.clear();
}
}

//----------------------------------------------------------------------------------------------
/** Generate a list of peaks that meets= all the requirements for fitting offset
  * @param peakslist :: table workspace as the output of FindPeaks
  * @param wi :: workspace index of the spectrum
  * @param peakPositionRef :: reference peaks positions
  * @param peakPosToFit :: output of reference centres of the peaks used to fit
 * offset
  * @param peakPosFitted :: output of fitted centres of the peaks used to fit
 * offset
  * @param peakHeightFitted :: heights of the peaks used to fit offset
  * @param chisq :: chi squares of the peaks used to fit offset
  * @param useFitWindows :: boolean whether FitWindows is used
  * @param fitWindowsToUse :: fit windows
  * @param minD :: minimum d-spacing of the spectrum
  * @param maxD :: minimum d-spacing of the spectrum
  * @param deltaDovD :: delta(d)/d of the peak for fitting
  * @param dev_deltaDovD :: standard deviation of delta(d)/d of all the peaks in
 * the spectrum
  */
void CalculateDetOffsetsMultiPeaks::generatePeaksList(
    const API::ITableWorkspace_sptr &peakslist, int wi,
    const std::vector<double> &peakPositionRef,
    std::vector<double> &peakPosToFit, std::vector<double> &peakPosFitted,
    std::vector<double> &peakHeightFitted, std::vector<double> &chisq,
    bool useFitWindows, const std::vector<double> &fitWindowsToUse,
    const double minD, const double maxD, double &deltaDovD,
    double &dev_deltaDovD) {
  // FIXME - Need to make sure that the peakPositionRef and peakslist have the
  // same order of peaks

  // Check
  size_t numrows = peakslist->rowCount();
  if (numrows != peakPositionRef.size()) {
    std::stringstream msg;
    msg << "Number of peaks in PeaksList (from FindPeaks=" << numrows
        << ") is not same as number of "
        << "referenced peaks' positions (" << peakPositionRef.size() << ")";
    throw std::runtime_error(msg.str());
  }

  std::vector<double> vec_widthDivPos;
  std::vector<double> vec_offsets;

  for (size_t i = 0; i < peakslist->rowCount(); ++i) {
    // Get peak value
    double centre = peakslist->getRef<double>("centre", i);
    double width = peakslist->getRef<double>("width", i);
    double height = peakslist->getRef<double>("height", i);
    double chi2 = peakslist->getRef<double>("chi2", i);

    // Identify whether this peak would be accepted to optimize offset
    // - peak position within D-range
    if (centre <= minD || centre >= maxD) {
      std::stringstream dbss;
      dbss << " wi = " << wi << " c = " << centre << " out of D-range ";
      g_log.debug(dbss.str());
      continue;
    }

    // - rule out of peak with wrong position
    if (useFitWindows) {
      // outside peak fit window o
      if (centre <= fitWindowsToUse[2 * i] ||
          centre >= fitWindowsToUse[2 * i + 1]) {
        std::stringstream dbss;
        dbss << " wi = " << wi << " c = " << centre << " out of fit window ";
        g_log.debug(dbss.str());
        continue;
      }
    }

    // - check chi-square
    if (chi2 > m_maxChiSq || chi2 < 0) {
      std::stringstream dbss;
      dbss << " wi = " << wi << " c = " << centre << " chi2 = " << chi2
           << ": Too large";
      g_log.debug(dbss.str());
      continue;
    }

    // - check peak's resolution
    double widthdevpos = width / centre;

    // background value
    double back_intercept = peakslist->getRef<double>("backgroundintercept", i);
    double back_slope = peakslist->getRef<double>("backgroundslope", i);
    double back_quad = peakslist->getRef<double>("A2", i);
    double background =
        back_intercept + back_slope * centre + back_quad * centre * centre;

    // Continue to identify whether this peak will be accepted
    // (e) peak signal/noise ratio
    if (height * FWHM_TO_SIGMA / width < 5.)
      continue;

    // (f) ban peaks that are not outside of error bars for the background
    if (height < 0.5 * std::sqrt(height + background))
      continue;

    // - calcualte offsets as to determine the (z-value)
    double offset = fabs(peakPositionRef[i] / centre - 1);
    if (offset > m_maxOffset) {
      std::stringstream dbss;
      dbss << " wi = " << wi << " c = " << centre
           << " exceeds maximum offset. ";
      g_log.debug(dbss.str());
      continue;
    } else
      vec_offsets.push_back(offset);

    // (g) calculate width/pos as to determine the (z-value) for constant
    // "width" - (delta d)/d
    // double widthdevpos = width/centre;
    vec_widthDivPos.push_back(widthdevpos);

    // g_log.debug() << " h:" << height << " c:" << centre << " w:" <<
    // (width/(2.*std::sqrt(2.*M_LN2)))
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
  for (size_t i = 0; i < peakPosFitted.size(); ++i) {
    if (Zscore[i] > 2.0 || Z_offset[i] > 2.0) {
      g_log.debug() << "Banning peak at " << peakPosFitted[i]
                    << " in wkspindex = (no show)" // << wi
                    << " sigma/d = " << vec_widthDivPos[i] << "\n";
      banned.push_back(i);
      continue;
    }
  }

  // Delete banned peaks
  if (!banned.empty()) {
    g_log.debug() << "Deleting " << banned.size() << " of "
                  << peakPosFitted.size() << " peaks in wkspindex = ??? "
                  << "\n"; // << wi << "\n";

    deletePeaks(banned, peakPosToFit, peakPosFitted, peakHeightFitted, chisq,
                vec_widthDivPos);
  }

  Statistics widthDivPos = getStatistics(vec_widthDivPos);
  deltaDovD = widthDivPos.mean;
  dev_deltaDovD = widthDivPos.standard_deviation;
}

//----------------------------------------------------------------------------------------------
/**
  */
void CalculateDetOffsetsMultiPeaks::createInformationWorkspaces() {
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
  for (size_t i = 0; i < numspec; ++i) {
    TableRow newrow = m_infoTableWS->appendRow();
    newrow << static_cast<int>(i);
  }

  // Create output peak fitting information table
  m_peakOffsetTableWS = boost::make_shared<TableWorkspace>();

  // set up columns
  m_peakOffsetTableWS->addColumn("int", "WorkspaceIndex");
  for (double m_peakPosition : m_peakPositions) {
    std::stringstream namess;
    namess << "@" << std::setprecision(5) << m_peakPosition;
    m_peakOffsetTableWS->addColumn("str", namess.str());
  }
  m_peakOffsetTableWS->addColumn("double", "OffsetDeviation");

  // add rows
  for (size_t i = 0; i < numspec; ++i) {
    TableRow newrow = m_peakOffsetTableWS->appendRow();
    newrow << static_cast<int>(i);
  }

  // Create resolution (delta(d)/d) workspace
  m_resolutionWS = boost::dynamic_pointer_cast<MatrixWorkspace>(
      WorkspaceFactory::Instance().create("Workspace2D", numspec, 1, 1));
}

//----------------------------------------------------------------------------------------------
/** Add result of offset-calculation to information table workspaces
 * (thread-safe)
  */
void CalculateDetOffsetsMultiPeaks::addInfoToReportWS(
    int wi, FitPeakOffsetResult offsetresult,
    const std::vector<double> &tofitpeakpositions,
    const std::vector<double> &fittedpeakpositions) {
  // Offset calculation status
  m_infoTableWS->cell<int>(wi, 1) = offsetresult.numpeaksfitted;
  m_infoTableWS->cell<int>(wi, 2) = offsetresult.numpeaksindrange;
  m_infoTableWS->cell<std::string>(wi, 3) = offsetresult.fitoffsetstatus;
  m_infoTableWS->cell<double>(wi, 4) = offsetresult.chi2;
  m_infoTableWS->cell<double>(wi, 5) = offsetresult.offset;
  m_infoTableWS->cell<double>(wi, 6) = offsetresult.highestpeakpos;
  m_infoTableWS->cell<double>(wi, 7) = offsetresult.highestpeakdev;

  // Peak width delta(d)/d
  m_resolutionWS->mutableX(wi)[0] = static_cast<double>(wi);
  if (offsetresult.fitoffsetstatus == "success") {
    // Only add successfully calculated value
    m_resolutionWS->mutableY(wi)[0] = offsetresult.resolution;
    m_resolutionWS->mutableE(wi)[0] = offsetresult.dev_resolution;
  } else {
    // Only add successfully calculated value
    m_resolutionWS->mutableY(wi)[0] = 0.0;
    m_resolutionWS->mutableE(wi)[0] = 0.0;
  }

  // Peak-fitting information:  Record: (found peak position) - (target peak
  // position)
  int numpeaksfitted = offsetresult.numpeaksfitted;
  if (numpeaksfitted > 0) {
    // Not all peaks in peakOffsetTable are in
    // tofitpeakpositions/fittedpeakpositions
    std::vector<bool> haspeakvec(offsetresult.numpeakstofit, false);
    std::vector<double> deltavec(offsetresult.numpeakstofit, 0.0);

    // to calculate deviation from peak centre
    double sumdelta1 = 0.0;
    double sumdelta2 = 0.0;
    for (int i = 0; i < numpeaksfitted; ++i) {
      double peakcentre = tofitpeakpositions[i];
      int index =
          static_cast<int>(std::lower_bound(m_peakPositions.begin(),
                                            m_peakPositions.end(), peakcentre) -
                           m_peakPositions.begin());
      if (index > 0 && (m_peakPositions[index] - peakcentre >
                        peakcentre - m_peakPositions[index - 1])) {
        --index;
      }
      haspeakvec[index] = true;
      deltavec[index] = peakcentre - fittedpeakpositions[i];

      sumdelta1 += deltavec[index] / tofitpeakpositions[i];
      sumdelta2 += deltavec[index] * deltavec[index] /
                   (tofitpeakpositions[i] * tofitpeakpositions[i]);
    }

    double numdelta = static_cast<double>(numpeaksfitted);
    double stddev = 0.;
    if (numpeaksfitted > 1.)
      stddev = sqrt(sumdelta2 / numdelta -
                    (sumdelta1 / numdelta) * (sumdelta1 / numdelta));

    // Set the peak positions to workspace and
    for (int i = 0; i < offsetresult.numpeakstofit; ++i) {
      if (haspeakvec[i]) {
        std::stringstream ss;
        ss << deltavec[i];

        int icol = i + 1;
        m_peakOffsetTableWS->cell<std::string>(wi, icol) = ss.str();
      }
    }

    // Final statistic
    size_t icol = m_peakOffsetTableWS->columnCount() - 1;
    m_peakOffsetTableWS->cell<double>(wi, icol) = stddev;
  }
}

//----------------------------------------------------------------------------------------------
/** Clean peak offset table workspace
  */
void CalculateDetOffsetsMultiPeaks::removeEmptyRowsFromPeakOffsetTable() {
  size_t numrows = m_infoTableWS->rowCount();
  if (m_peakOffsetTableWS->rowCount() != numrows) {
    g_log.warning(
        "Peak position offset workspace has different number of rows to "
        "that of offset fitting information workspace. "
        "No row will be removed from peak position offset table workspace. ");
    return;
  }

  size_t icurrow = 0;
  for (size_t i = 0; i < numrows; ++i) {
    // Criteria 1 dev is not equal to zero
    bool removerow = false;
    int numpeakfitted = m_infoTableWS->cell<int>(i, 1);
    if (numpeakfitted == 0) {
      removerow = true;
    }

    // Remove row
    if (removerow) {
      m_peakOffsetTableWS->removeRow(icurrow);
    } else {
      // advance to next row
      ++icurrow;
    }
  }
}

} // namespace Algorithm
} // namespace Mantid
