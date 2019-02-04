// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidAlgorithms/PDCalibration.h"
#include "MantidAPI/FileProperty.h"
#include "MantidAPI/FuncMinimizerFactory.h"
#include "MantidAPI/IEventList.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/Run.h"
#include "MantidAPI/TableRow.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidDataObjects/MaskWorkspace.h"
#include "MantidDataObjects/SpecialWorkspace2D.h"
#include "MantidDataObjects/TableWorkspace.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidDataObjects/WorkspaceCreation.h"
#include "MantidGeometry/IDetector.h"
#include "MantidGeometry/Instrument.h"
#include "MantidGeometry/Instrument/DetectorInfo.h"
#include "MantidHistogramData/Histogram.h"
#include "MantidKernel/ArrayBoundedValidator.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/BoundedValidator.h"
#include "MantidKernel/CompositeValidator.h"
#include "MantidKernel/Diffraction.h"
#include "MantidKernel/ListValidator.h"
#include "MantidKernel/MandatoryValidator.h"
#include "MantidKernel/RebinParamsValidator.h"
#include "MantidKernel/make_unique.h"

#include <algorithm>
#include <cassert>
#include <gsl/gsl_multifit_nlin.h>
#include <gsl/gsl_multimin.h>
#include <limits>
#include <numeric>

namespace Mantid {
namespace Algorithms {

using namespace Mantid::DataObjects;
using namespace Mantid::HistogramData;
using Mantid::API::FileProperty;
using Mantid::API::MatrixWorkspace;
using Mantid::API::MatrixWorkspace_sptr;
using Mantid::API::WorkspaceProperty;
using Mantid::DataObjects::EventWorkspace;
using Mantid::DataObjects::MaskWorkspace_sptr;
using Mantid::Geometry::Instrument_const_sptr;
using Mantid::Kernel::ArrayBoundedValidator;
using Mantid::Kernel::ArrayProperty;
using Mantid::Kernel::BoundedValidator;
using Mantid::Kernel::CompositeValidator;
using Mantid::Kernel::Direction;
using Mantid::Kernel::MandatoryValidator;
using Mantid::Kernel::RebinParamsValidator;
using Mantid::Kernel::StringListValidator;
using std::vector;

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(PDCalibration)

namespace { // anonymous
const auto isNonZero = [](const double value) { return value != 0.; };
}

/// private inner class
class PDCalibration::FittedPeaks {
public:
  FittedPeaks(API::MatrixWorkspace_const_sptr wksp,
              const std::size_t wkspIndex) {
    this->wkspIndex = wkspIndex;

    // convert workspace index into detector id
    const auto &spectrum = wksp->getSpectrum(wkspIndex);
    const auto &detIds = spectrum.getDetectorIDs();
    if (detIds.size() != 1) {
      throw std::runtime_error("Summed pixels is not currently supported");
    }
    this->detid = *(detIds.begin());

    const auto &X = spectrum.x();
    const auto &Y = spectrum.y();
    tofMin = X.front();
    tofMax = X.back();

    // determine tof min supported by the workspace
    size_t minIndex = 0; // want to store value
    for (; minIndex < Y.size(); ++minIndex) {
      if (isNonZero(Y[minIndex])) {
        tofMin = X[minIndex];
        break;
      }
    }

    // determine tof max supported by the workspace
    size_t maxIndex = Y.size() - 1;
    for (; maxIndex > minIndex; --maxIndex) {
      if (isNonZero(Y[maxIndex])) {
        tofMax = X[maxIndex];
        break;
      }
    }
  }

  void setPositions(const std::vector<double> &peaksInD,
                    const std::vector<double> &peaksInDWindows,
                    std::function<double(double)> toTof) {
    // clear out old values
    inDPos.clear();
    inTofPos.clear();
    inTofWindows.clear();

    // assign things
    inDPos.assign(peaksInD.begin(), peaksInD.end());
    inTofPos.assign(peaksInD.begin(), peaksInD.end());
    inTofWindows.assign(peaksInDWindows.begin(), peaksInDWindows.end());

    // convert the bits that matter to TOF
    std::transform(inTofPos.begin(), inTofPos.end(), inTofPos.begin(), toTof);
    std::transform(inTofWindows.begin(), inTofWindows.end(),
                   inTofWindows.begin(), toTof);
  }

  std::size_t wkspIndex;
  detid_t detid;
  double tofMin;
  double tofMax;
  std::vector<double> inTofPos;
  std::vector<double> inTofWindows;
  std::vector<double> inDPos;
};

//----------------------------------------------------------------------------------------------
/** Constructor
 */
PDCalibration::PDCalibration() {}

//----------------------------------------------------------------------------------------------
/** Destructor
 */
PDCalibration::~PDCalibration() {}

//----------------------------------------------------------------------------------------------

/// Algorithms name for identification. @see Algorithm::name
const std::string PDCalibration::name() const { return "PDCalibration"; }

/// Algorithm's version for identification. @see Algorithm::version
int PDCalibration::version() const { return 1; }

/// Algorithm's category for identification. @see Algorithm::category
const std::string PDCalibration::category() const {
  return "Diffraction\\Calibration";
}

/// Algorithm's summary for use in the GUI and help. @see Algorithm::summary
const std::string PDCalibration::summary() const {
  return "Calibrate the detector pixels and create a calibration table";
}

//----------------------------------------------------------------------------------------------
/** Initialize the algorithm's properties.
 */
void PDCalibration::init() {
  declareProperty(Kernel::make_unique<WorkspaceProperty<MatrixWorkspace>>(
                      "InputWorkspace", "", Direction::InOut),
                  "Input signal workspace");

  declareProperty(Kernel::make_unique<ArrayProperty<double>>(
                      "TofBinning", boost::make_shared<RebinParamsValidator>()),
                  "Min, Step, and Max of time-of-flight bins. "
                  "Logarithmic binning is used if Step is negative.");

  const std::vector<std::string> exts2{".h5", ".cal"};
  declareProperty(
      Kernel::make_unique<FileProperty>("PreviousCalibrationFile", "",
                                        FileProperty::OptionalLoad, exts2),
      "Previous calibration file");
  declareProperty(
      Kernel::make_unique<WorkspaceProperty<API::ITableWorkspace>>(
          "PreviousCalibrationTable", "", Direction::Input,
          API::PropertyMode::Optional),
      "Previous calibration table. This overrides results from previous file.");

  // properties about peak positions to fit
  std::vector<std::string> peaktypes{"BackToBackExponential", "Gaussian",
                                     "Lorentzian", "PseudoVoigt"};
  declareProperty("PeakFunction", "Gaussian",
                  boost::make_shared<StringListValidator>(peaktypes));
  vector<std::string> bkgdtypes{"Flat", "Linear", "Quadratic"};
  declareProperty("BackgroundType", "Linear",
                  boost::make_shared<StringListValidator>(bkgdtypes),
                  "Type of Background.");

  auto peaksValidator = boost::make_shared<CompositeValidator>();
  auto mustBePosArr =
      boost::make_shared<Kernel::ArrayBoundedValidator<double>>();
  mustBePosArr->setLower(0.0);
  peaksValidator->add(mustBePosArr);
  peaksValidator->add(
      boost::make_shared<MandatoryValidator<std::vector<double>>>());
  declareProperty(Kernel::make_unique<ArrayProperty<double>>("PeakPositions",
                                                             peaksValidator),
                  "Comma delimited d-space positions of reference peaks.");

  auto mustBePositive = boost::make_shared<BoundedValidator<double>>();
  mustBePositive->setLower(0.0);
  declareProperty(
      "PeakWindow", 0.1, mustBePositive,
      "The maximum window (in d space) around peak to look for peak.");
  std::vector<std::string> modes{"DIFC", "DIFC+TZERO", "DIFC+TZERO+DIFA"};

  auto min = boost::make_shared<BoundedValidator<double>>();
  min->setLower(1e-3);
  declareProperty("PeakWidthPercent", EMPTY_DBL(), min,
                  "The estimated peak width as a "
                  "percentage of the d-spacing "
                  "of the center of the peak. This is the same as the width in "
                  "time-of-flight.");

  declareProperty("MinimumPeakHeight", 2.,
                  "Minimum peak height such that all the fitted peaks with "
                  "height under this value will be excluded.");

  declareProperty(
      "MaxChiSq", 100.,
      "Maximum chisq value for individual peak fit allowed. (Default: 100)");

  declareProperty(
      "ConstrainPeakPositions", false,
      "If true peak position will be constrained by estimated positions "
      "(highest Y value position) and "
      "the peak width either estimted by observation or calculate.");

  declareProperty("CalibrationParameters", "DIFC",
                  boost::make_shared<StringListValidator>(modes),
                  "Select calibration parameters to fit.");

  declareProperty(
      Kernel::make_unique<ArrayProperty<double>>("TZEROrange"),
      "Range for allowable TZERO from calibration (default is all)");
  declareProperty(Kernel::make_unique<ArrayProperty<double>>("DIFArange"),
                  "Range for allowable DIFA from calibration (default is all)");

  declareProperty(Kernel::make_unique<WorkspaceProperty<API::ITableWorkspace>>(
                      "OutputCalibrationTable", "", Direction::Output),
                  "An output workspace containing the Calibration Table");

  declareProperty(Kernel::make_unique<WorkspaceProperty<API::WorkspaceGroup>>(
                      "DiagnosticWorkspaces", "", Direction::Output),
                  "Workspaces to promote understanding of calibration results");

  // make group for Input properties
  std::string inputGroup("Input Options");
  setPropertyGroup("InputWorkspace", inputGroup);
  setPropertyGroup("TofBinning", inputGroup);
  setPropertyGroup("PreviousCalibrationFile", inputGroup);
  setPropertyGroup("PreviousCalibrationTable", inputGroup);

  std::string funcgroup("Function Types");
  setPropertyGroup("PeakFunction", funcgroup);
  setPropertyGroup("BackgroundType", funcgroup);

  // make group for FitPeaks properties
  std::string fitPeaksGroup("Peak Fitting");
  setPropertyGroup("PeakPositions", fitPeaksGroup);
  setPropertyGroup("PeakWindow", fitPeaksGroup);
  setPropertyGroup("PeakWidthPercent", fitPeaksGroup);
  setPropertyGroup("MinimumPeakHeight", fitPeaksGroup);
  setPropertyGroup("MaxChiSq", fitPeaksGroup);
  setPropertyGroup("ConstrainPeakPositions", fitPeaksGroup);

  // make group for type of calibration
  std::string calGroup("Calibration Type");
  setPropertyGroup("CalibrationParameters", calGroup);
  setPropertyGroup("TZEROrange", calGroup);
  setPropertyGroup("DIFArange", calGroup);
}

std::map<std::string, std::string> PDCalibration::validateInputs() {
  std::map<std::string, std::string> messages;

  vector<double> tzeroRange = getProperty("TZEROrange");
  if (!tzeroRange.empty()) {
    if (tzeroRange.size() != 2) {
      messages["TZEROrange"] = "Require two values [min,max]";
    } else if (tzeroRange[0] >= tzeroRange[1]) {
      messages["TZEROrange"] = "min must be less than max";
    }
  }

  vector<double> difaRange = getProperty("DIFArange");
  if (!difaRange.empty()) {
    if (difaRange.size() != 2) {
      messages["DIFArange"] = "Require two values [min,max]";
    } else if (difaRange[0] >= difaRange[1]) {
      messages["DIFArange"] = "min must be less than max";
    }
  }

  return messages;
}

namespace {

bool hasDasIDs(API::ITableWorkspace_const_sptr table) {
  const auto columnNames = table->getColumnNames();
  return (std::find(columnNames.begin(), columnNames.end(),
                    std::string("dasid")) != columnNames.end());
}

/// @return Conversion factor or 1. if it is unknown
double getWidthToFWHM(const std::string &peakshape) {
  if (peakshape == "Gaussian") {
    return 2 * std::sqrt(2. * std::log(2.));
  } else if (peakshape == "Lorentzian") {
    return 2.;
  } else if (peakshape == "BackToBackExponential") {
    return 1.; // TODO the conversion isn't document in the function
  } else {
    return 1.;
  }
}

} //  end of anonymous namespace

//----------------------------------------------------------------------------------------------
/** Execute the algorithm.
 */
void PDCalibration::exec() {
  vector<double> tofBinningParams = getProperty("TofBinning");
  m_tofMin = tofBinningParams.front();
  m_tofMax = tofBinningParams.back();

  vector<double> tzeroRange = getProperty("TZEROrange");
  if (tzeroRange.size() == 2) {
    m_tzeroMin = tzeroRange[0];
    m_tzeroMax = tzeroRange[1];

    std::stringstream msg;
    msg << "Using tzero range of " << m_tzeroMin << " <= "
        << "TZERO <= " << m_tzeroMax;
    g_log.information(msg.str());
  } else {
    g_log.information("Using all TZERO values");

    m_tzeroMin = std::numeric_limits<double>::lowest();
    m_tzeroMax = std::numeric_limits<double>::max();
  }

  vector<double> difaRange = getProperty("DIFArange");
  if (difaRange.size() == 2) {
    m_difaMin = difaRange[0];
    m_difaMax = difaRange[1];

    std::stringstream msg;
    msg << "Using difa range of " << m_difaMin << " <= "
        << "DIFA <= " << m_difaMax;
    g_log.information(msg.str());
  } else {
    g_log.information("Using all DIFA values");

    m_difaMin = std::numeric_limits<double>::lowest();
    m_difaMax = std::numeric_limits<double>::max();
  }

  m_peaksInDspacing = getProperty("PeakPositions");
  // Sort peak positions, requried for correct peak window calculations
  std::sort(m_peaksInDspacing.begin(), m_peaksInDspacing.end());

  const double peakWindowMaxInDSpacing = getProperty("PeakWindow");
  const double minPeakHeight = getProperty("MinimumPeakHeight");
  const double maxChiSquared = getProperty("MaxChiSq");

  const std::string calParams = getPropertyValue("CalibrationParameters");
  if (calParams == std::string("DIFC"))
    m_numberMaxParams = 1;
  else if (calParams == std::string("DIFC+TZERO"))
    m_numberMaxParams = 2;
  else if (calParams == std::string("DIFC+TZERO+DIFA"))
    m_numberMaxParams = 3;
  else
    throw std::runtime_error(
        "Encountered impossible CalibrationParameters value");

  m_uncalibratedWS = loadAndBin();
  setProperty("InputWorkspace", m_uncalibratedWS);

  auto uncalibratedEWS =
      boost::dynamic_pointer_cast<EventWorkspace>(m_uncalibratedWS);
  bool isEvent = bool(uncalibratedEWS);

  // Load Previous Calibration or create calibration table from signal file
  if ((!static_cast<std::string>(getProperty("PreviousCalibrationFile"))
            .empty()) ||
      (!getPropertyValue("PreviousCalibrationTable")
            .empty())) { //"PreviousCalibrationTable"
    createCalTableFromExisting();
  } else {
    createCalTableNew();
  }
  createInformationWorkspaces();

  std::string maskWSName = getPropertyValue("OutputCalibrationTable");
  maskWSName += "_mask";
  declareProperty(Kernel::make_unique<WorkspaceProperty<>>(
                      "MaskWorkspace", maskWSName, Direction::Output),
                  "An output workspace containing the mask");

  MaskWorkspace_sptr maskWS = boost::make_shared<DataObjects::MaskWorkspace>(
      m_uncalibratedWS->getInstrument());
  for (size_t i = 0; i < maskWS->getNumberHistograms(); ++i) // REMOVE
    maskWS->setMaskedIndex(i, true); // mask everything to start
  setProperty("MaskWorkspace", maskWS);

  const std::string peakFunction = getProperty("PeakFunction");
  const double WIDTH_TO_FWHM = getWidthToFWHM(peakFunction);
  if (WIDTH_TO_FWHM == 1.) {
    g_log.notice() << "Unknown conversion for \"" << peakFunction
                   << "\", found peak widths and resolution should not be "
                      "directly compared to delta-d/d";
  }
  int NUMHIST = static_cast<int>(m_uncalibratedWS->getNumberHistograms());

  // create TOF peak centers workspace
  auto matrix_pair = createTOFPeakCenterFitWindowWorkspaces(
      m_uncalibratedWS, peakWindowMaxInDSpacing);
  API::MatrixWorkspace_sptr tof_peak_center_ws = matrix_pair.first;
  API::MatrixWorkspace_sptr tof_peak_window_ws = matrix_pair.second;
  //  API::MatrixWorkspace_sptr peak_window_ws =
  //      createTOFPeakFitWindowWorkspace(m_uncalibratedWS, windowsInDSpacing);

  double peak_width_percent = getProperty("PeakWidthPercent");

  const std::string diagnostic_prefix =
      getPropertyValue("DiagnosticWorkspaces");

  auto algFitPeaks = createChildAlgorithm("FitPeaks", .2, .7);
  algFitPeaks->setLoggingOffset(3);

  algFitPeaks->setProperty("InputWorkspace", m_uncalibratedWS);
  // theoretical peak center
  algFitPeaks->setProperty("PeakCentersWorkspace", tof_peak_center_ws);

  // peak and background functions
  algFitPeaks->setProperty<std::string>("PeakFunction", peakFunction);
  algFitPeaks->setProperty<std::string>("BackgroundType",
                                        getProperty("BackgroundType"));
  // peak range setup
  algFitPeaks->setProperty("FitPeakWindowWorkspace", tof_peak_window_ws);
  algFitPeaks->setProperty("PeakWidthPercent", peak_width_percent);
  algFitPeaks->setProperty("MinimumPeakHeight", minPeakHeight);
  // some fitting strategy
  algFitPeaks->setProperty("FitFromRight", true);
  algFitPeaks->setProperty("HighBackground", false);
  bool constrainPeakPosition = getProperty("ConstrainPeakPositions");
  algFitPeaks->setProperty(
      "ConstrainPeakPositions",
      constrainPeakPosition); // TODO Pete: need to test this option
  //  optimization setup // TODO : need to test LM or LM-MD
  algFitPeaks->setProperty("Minimizer", "Levenberg-Marquardt");
  algFitPeaks->setProperty("CostFunction", "Least squares");

  // FitPeaks will abstract the peak parameters if you ask
  algFitPeaks->setProperty("RawPeakParameters", false);

  // Analysis output
  algFitPeaks->setPropertyValue("OutputPeakParametersWorkspace",
                                diagnostic_prefix + "_fitparam");
  algFitPeaks->setPropertyValue("FittedPeaksWorkspace",
                                diagnostic_prefix + "_fitted");

  // run and get the result
  algFitPeaks->executeAsChildAlg();
  g_log.information("finished FitPeaks");

  // get the fit result
  API::ITableWorkspace_sptr fittedTable =
      algFitPeaks->getProperty("OutputPeakParametersWorkspace");
  API::MatrixWorkspace_sptr calculatedWS =
      algFitPeaks->getProperty("FittedPeaksWorkspace");

  // check : for Pete
  if (!fittedTable)
    throw std::runtime_error(
        "FitPeaks does not have output OutputPeakParametersWorkspace.");
  if (fittedTable->rowCount() != NUMHIST * m_peaksInDspacing.size())
    throw std::runtime_error(
        "The number of rows in OutputPeakParametersWorkspace is not correct!");

  // END-OF (FitPeaks)
  const std::string backgroundType = getPropertyValue("BackgroundType");

  API::Progress prog(this, 0.7, 1.0, NUMHIST);

  const auto windowsInDSpacing =
      dSpacingWindows(m_peaksInDspacing, peakWindowMaxInDSpacing);

  // cppcheck-suppress syntaxError
  PRAGMA_OMP(parallel for schedule(dynamic, 1) )
  for (int wkspIndex = 0; wkspIndex < NUMHIST; ++wkspIndex) {
    PARALLEL_START_INTERUPT_REGION
    if (isEvent && uncalibratedEWS->getSpectrum(wkspIndex).empty()) {
      prog.report();
      continue;
    }

    // object to hold the information about the peak positions, detid, and wksp
    // index
    PDCalibration::FittedPeaks peaks(m_uncalibratedWS, wkspIndex);
    auto toTof = getDSpacingToTof(peaks.detid);
    peaks.setPositions(m_peaksInDspacing, windowsInDSpacing, toTof);

    // includes peaks that aren't used in the fit
    const size_t numPeaks = m_peaksInDspacing.size();
    std::vector<double> tof_vec_full(numPeaks, std::nan(""));
    std::vector<double> d_vec;
    std::vector<double> tof_vec;
    std::vector<double> width_vec_full(numPeaks, std::nan(""));
    std::vector<double> height_vec_full(numPeaks, std::nan(""));
    std::vector<double> height2; // the square of the peak height
    // for (size_t i = 0; i < fittedTable->rowCount(); ++i) {
    const size_t rowNumInFitTableOffset = wkspIndex * numPeaks;
    for (size_t peakIndex = 0; peakIndex < numPeaks; ++peakIndex) {
      size_t rowIndexInFitTable = rowNumInFitTableOffset + peakIndex;

      // check indices in PeaksTable
      if (fittedTable->getRef<int>("wsindex", rowIndexInFitTable) != wkspIndex)
        throw std::runtime_error("workspace index mismatch!");
      if (fittedTable->getRef<int>("peakindex", rowIndexInFitTable) !=
          static_cast<int>(peakIndex))
        throw std::runtime_error(
            "peak index mismatch but workspace index matched");

      // get the effective peak parameters
      const double centre =
          fittedTable->getRef<double>("centre", rowIndexInFitTable);
      const double width =
          fittedTable->getRef<double>("width", rowIndexInFitTable);
      const double height =
          fittedTable->getRef<double>("height", rowIndexInFitTable);
      const double chi2 =
          fittedTable->getRef<double>("chi2", rowIndexInFitTable);

      // check chi-square
      if (chi2 > maxChiSquared || chi2 < 0.) {
        continue;
      }

      // rule out of peak with wrong position
      if (peaks.inTofWindows[2 * peakIndex] >= centre ||
          peaks.inTofWindows[2 * peakIndex + 1] <= centre) {
        continue;
      }

      // check height: make sure 0 is smaller than 0
      if (height < minPeakHeight + 1.E-15) {
        continue;
      }

      // background value
      double back_intercept =
          fittedTable->getRef<double>("A0", rowIndexInFitTable);
      double back_slope = 0.;
      double back_quad = 0.;
      switch (backgroundType[0]) {
      case 'Q': // Quadratic
        back_quad = fittedTable->getRef<double>(
            "A2", rowIndexInFitTable); // fall through
      case 'L':                        // Linear
        back_slope = fittedTable->getRef<double>("A1", rowIndexInFitTable);
      }
      double background =
          back_intercept + back_slope * centre + back_quad * centre * centre;

      // ban peaks that are not outside of error bars for the background
      if (height < 0.5 * std::sqrt(height + background)) {
        continue;
      }
      d_vec.push_back(m_peaksInDspacing[peakIndex]);
      tof_vec.push_back(centre);
      height2.push_back(height * height);
      tof_vec_full[peakIndex] = centre;
      width_vec_full[peakIndex] = width;
      height_vec_full[peakIndex] = height;
    }

    maskWS->setMasked(peaks.detid, d_vec.size() < 2);
    if (d_vec.size() < 2) { // not enough peaks were found
      continue;
    } else {
      double difc = 0., t0 = 0., difa = 0.;
      fitDIFCtZeroDIFA_LM(d_vec, tof_vec, height2, difc, t0, difa);

      const auto rowIndexOutputPeaks = m_detidToRow[peaks.detid];
      double chisq = 0.;
      auto converter =
          Kernel::Diffraction::getTofToDConversionFunc(difc, difa, t0);
      for (std::size_t i = 0; i < numPeaks; ++i) {
        if (std::isnan(tof_vec_full[i]))
          continue;
        const double dspacing = converter(tof_vec_full[i]);
        const double temp = m_peaksInDspacing[i] - dspacing;
        chisq += (temp * temp);
        m_peakPositionTable->cell<double>(rowIndexOutputPeaks, i + 1) =
            dspacing;
        m_peakWidthTable->cell<double>(rowIndexOutputPeaks, i + 1) =
            WIDTH_TO_FWHM * converter(width_vec_full[i]);
        m_peakHeightTable->cell<double>(rowIndexOutputPeaks, i + 1) =
            height_vec_full[i];
      }
      m_peakPositionTable->cell<double>(rowIndexOutputPeaks,
                                        m_peaksInDspacing.size() + 1) = chisq;
      m_peakPositionTable->cell<double>(rowIndexOutputPeaks,
                                        m_peaksInDspacing.size() + 2) =
          chisq / static_cast<double>(numPeaks - 1);

      setCalibrationValues(peaks.detid, difc, difa, t0);
    }
    prog.report();

    PARALLEL_END_INTERUPT_REGION
  }
  PARALLEL_CHECK_INTERUPT_REGION

  // sort the calibration workspaces
  m_calibrationTable = sortTableWorkspace(m_calibrationTable);
  setProperty("OutputCalibrationTable", m_calibrationTable);

  // fix-up the diagnostic workspaces
  m_calibrationTable = sortTableWorkspace(m_peakPositionTable);
  m_calibrationTable = sortTableWorkspace(m_peakWidthTable);
  m_calibrationTable = sortTableWorkspace(m_peakHeightTable);

  // a derived table from the position and width
  auto resolutionWksp = calculateResolutionTable();

  // set the diagnostic workspaces out
  auto diagnosticGroup = boost::make_shared<API::WorkspaceGroup>();
  // add workspaces calculated by FitPeaks
  API::AnalysisDataService::Instance().addOrReplace(
      diagnostic_prefix + "_fitparam", fittedTable);
  diagnosticGroup->addWorkspace(fittedTable);
  API::AnalysisDataService::Instance().addOrReplace(
      diagnostic_prefix + "_fitted", calculatedWS);
  diagnosticGroup->addWorkspace(calculatedWS);

  // add workspaces calculated by PDCalibration
  API::AnalysisDataService::Instance().addOrReplace(
      diagnostic_prefix + "_dspacing", m_peakPositionTable);
  diagnosticGroup->addWorkspace(m_peakPositionTable);
  API::AnalysisDataService::Instance().addOrReplace(
      diagnostic_prefix + "_width", m_peakWidthTable);
  diagnosticGroup->addWorkspace(m_peakWidthTable);
  API::AnalysisDataService::Instance().addOrReplace(
      diagnostic_prefix + "_height", m_peakHeightTable);
  diagnosticGroup->addWorkspace(m_peakHeightTable);
  API::AnalysisDataService::Instance().addOrReplace(
      diagnostic_prefix + "_resolution", resolutionWksp);
  diagnosticGroup->addWorkspace(resolutionWksp);
  setProperty("DiagnosticWorkspaces", diagnosticGroup);
}

namespace { // anonymous namespace
            /**
             * Helper function for calculating costs in gsl.
             * @param v vector of parameters that are being modified by gsl (difc, tzero,
             * difa)
             * @param params The parameters being used for the fit
             * @return Sum of the errors
             */
double gsl_costFunction(const gsl_vector *v, void *peaks) {
  // this array is [numPeaks, numParams, vector<tof>, vector<dspace>,
  // vector<height^2>]
  // index as      [0,        1,         2,         , 2+n           , 2+2n]
  const std::vector<double> *peakVec =
      reinterpret_cast<std::vector<double> *>(peaks);
  // number of peaks being fit
  const size_t numPeaks = static_cast<size_t>(peakVec->at(0));
  // number of parameters
  const size_t numParams = static_cast<size_t>(peakVec->at(1));

  // isn't strictly necessary, but makes reading the code much easier
  const std::vector<double> tofObs(peakVec->begin() + 2,
                                   peakVec->begin() + 2 + numPeaks);
  const std::vector<double> dspace(peakVec->begin() + (2 + numPeaks),
                                   peakVec->begin() + (2 + 2 * numPeaks));
  const std::vector<double> height2(peakVec->begin() + (2 + 2 * numPeaks),
                                    peakVec->begin() + (2 + 3 * numPeaks));

  // create the function to convert tof to dspacing
  double difc = gsl_vector_get(v, 0);
  double tzero = 0.;
  double difa = 0.;
  if (numParams > 1) {
    tzero = gsl_vector_get(v, 1);
    if (numParams > 2)
      difa = gsl_vector_get(v, 2);
  }
  auto converter =
      Kernel::Diffraction::getDToTofConversionFunc(difc, difa, tzero);

  // calculate the sum of the residuals from observed peaks
  double errsum = 0.0;
  for (size_t i = 0; i < numPeaks; ++i) {
    const double tofCalib = converter(dspace[i]);
    const double errsum_i = std::fabs(tofObs[i] - tofCalib) * height2[i];
    errsum += errsum_i;
  }

  return errsum;
}

// returns the errsum, the conversion parameters are done by in/out parameters
// to the function
// if the fit fails it returns 0.
double fitDIFCtZeroDIFA(std::vector<double> &peaks, double &difc, double &t0,
                        double &difa) {
  const size_t numParams = static_cast<size_t>(peaks[1]);

  // initial starting point as [DIFC, 0, 0]
  gsl_vector *fitParams = gsl_vector_alloc(numParams);
  gsl_vector_set_all(fitParams, 0.0); // set all parameters to zero
  gsl_vector_set(fitParams, 0, difc);
  if (numParams > 1) {
    gsl_vector_set(fitParams, 1, t0);
    if (numParams > 2) {
      gsl_vector_set(fitParams, 2, difa);
    }
  }

  // Set initial step sizes
  gsl_vector *stepSizes = gsl_vector_alloc(numParams);
  gsl_vector_set_all(stepSizes, 0.1);
  gsl_vector_set(stepSizes, 0, 0.01);

  // Initialize method and iterate
  gsl_multimin_function minex_func;
  minex_func.n = numParams;
  minex_func.f = &gsl_costFunction;
  minex_func.params = &peaks;

  // Set up GSL minimzer - simplex is overkill
  const gsl_multimin_fminimizer_type *minimizerType =
      gsl_multimin_fminimizer_nmsimplex;
  gsl_multimin_fminimizer *minimizer =
      gsl_multimin_fminimizer_alloc(minimizerType, numParams);
  gsl_multimin_fminimizer_set(minimizer, &minex_func, fitParams, stepSizes);

  // Finally do the fitting
  size_t iter = 0; // number of iterations
  const size_t MAX_ITER = 75 * numParams;
  int status = 0;
  double size;
  do {
    iter++;
    status = gsl_multimin_fminimizer_iterate(minimizer);
    if (status)
      break;

    size = gsl_multimin_fminimizer_size(minimizer);
    status = gsl_multimin_test_size(size, 1e-4);

  } while (status == GSL_CONTINUE && iter < MAX_ITER);

  // only update calibration values on successful fit
  double errsum = 0.; // return 0. if fit didn't work
  std::string status_msg = gsl_strerror(status);
  if (status_msg == "success") {
    difc = gsl_vector_get(minimizer->x, 0);
    if (numParams > 1) {
      t0 = gsl_vector_get(minimizer->x, 1);
      if (numParams > 2) {
        difa = gsl_vector_get(minimizer->x, 2);
      }
    }
    // return from gsl_costFunction can be accessed as fval
    errsum = minimizer->fval;
  }

  // free memory
  gsl_vector_free(fitParams);
  gsl_vector_free(stepSizes);
  gsl_multimin_fminimizer_free(minimizer);

  return errsum;
}

} // end of anonymous namespace

void PDCalibration::fitDIFCtZeroDIFA_LM(const std::vector<double> &d,
                                        const std::vector<double> &tof,
                                        const std::vector<double> &height2,
                                        double &difc, double &t0,
                                        double &difa) {
  const size_t numPeaks = d.size();
  if (numPeaks <= 1) {
    return; // don't do anything
  }
  // number of fit parameters 1=[DIFC], 2=[DIFC,TZERO], 3=[DIFC,TZERO,DIFA]
  // set the maximum number of parameters that will be used
  // statistics doesn't support having too few peaks
  size_t maxParams = std::min<size_t>(numPeaks - 1, m_numberMaxParams);

  // this must have the same layout as the unpacking in gsl_costFunction above
  std::vector<double> peaks(numPeaks * 3 + 2, 0.);
  peaks[0] = static_cast<double>(d.size());
  peaks[1] = 1.; // number of parameters to fit
  for (size_t i = 0; i < numPeaks; ++i) {
    peaks[i + 2] = tof[i];
    peaks[i + 2 + numPeaks] = d[i];
    peaks[i + 2 + 2 * numPeaks] = height2[i];
  }

  // calculate a starting DIFC
  double difc_start = difc;
  if (difc_start == 0.) {
    const double d_sum = std::accumulate(d.begin(), d.end(), 0.);
    const double tof_sum = std::accumulate(tof.begin(), tof.end(), 0.);
    difc_start = tof_sum / d_sum; // number of peaks falls out of division
  }

  // save the best values so far
  double best_errsum = std::numeric_limits<double>::max();
  double best_difc = 0.;
  double best_t0 = 0.;
  double best_difa = 0.;

  // loop over possible number of parameters
  for (size_t numParams = 1; numParams <= maxParams; ++numParams) {
    peaks[1] = static_cast<double>(numParams);
    double difc_local = difc_start;
    double t0_local = 0.;
    double difa_local = 0.;
    double errsum = fitDIFCtZeroDIFA(peaks, difc_local, t0_local, difa_local);
    if (errsum > 0.) {
      // normalize by degrees of freedom
      errsum = errsum / static_cast<double>(numPeaks - numParams);
      // save the best and forget the rest
      if (errsum < best_errsum) {
        if (difa_local > m_difaMax || difa_local < m_difaMin)
          continue; // unphysical fit
        if (t0_local > m_tzeroMax || t0_local < m_tzeroMin)
          continue; // unphysical fit
        best_errsum = errsum;
        best_difc = difc_local;
        best_t0 = t0_local;
        best_difa = difa_local;
      }
    }
  }

  // check that something actually fit and set to the best result
  if (best_difc > 0. && best_errsum < std::numeric_limits<double>::max()) {
    difc = best_difc;
    t0 = best_t0;
    difa = best_difa;
  }
}

vector<double>
PDCalibration::dSpacingWindows(const std::vector<double> &centres,
                               const double widthMax) {
  if (widthMax <= 0. || isEmpty(widthMax)) {
    return vector<double>(); // option is turned off
  }

  const std::size_t numPeaks = centres.size();

  // assumes distance between peaks can be used for window sizes
  assert(numPeaks >= 2);

  vector<double> windows(2 * numPeaks);
  double widthLeft;
  double widthRight;
  for (std::size_t i = 0; i < centres.size(); ++i) {
    // calculate left
    if (i == 0)
      widthLeft = .5 * (centres[1] - centres[0]);
    else
      widthLeft = .5 * (centres[i] - centres[i - 1]);
    widthLeft = std::min(widthLeft, widthMax);

    // calculate right
    if (i + 1 == numPeaks)
      widthRight = .5 * (centres[numPeaks - 1] - centres[numPeaks - 2]);
    else
      widthRight = .5 * (centres[i + 1] - centres[i]);
    widthRight = std::min(widthRight, widthMax);

    // set the windows
    windows[2 * i] = centres[i] - widthLeft;
    windows[2 * i + 1] = centres[i] + widthRight;
  }
  return windows;
}

std::function<double(double)>
PDCalibration::getDSpacingToTof(const detid_t detid) {
  auto rowNum = m_detidToRow[detid];

  // to start this is the old calibration values
  const double difa = m_calibrationTable->getRef<double>("difa", rowNum);
  const double difc = m_calibrationTable->getRef<double>("difc", rowNum);
  const double tzero = m_calibrationTable->getRef<double>("tzero", rowNum);

  return Kernel::Diffraction::getDToTofConversionFunc(difc, difa, tzero);
}

void PDCalibration::setCalibrationValues(const detid_t detid, const double difc,
                                         const double difa,
                                         const double tzero) {
  auto rowNum = m_detidToRow[detid];

  // detid is already there
  m_calibrationTable->cell<double>(rowNum, 1) = difc;
  m_calibrationTable->cell<double>(rowNum, 2) = difa;
  m_calibrationTable->cell<double>(rowNum, 3) = tzero;

  size_t hasDasIdsOffset = 0; // because it adds a column
  if (m_hasDasIds)
    hasDasIdsOffset++;

  const auto tofMinMax = getTOFminmax(difc, difa, tzero);
  m_calibrationTable->cell<double>(rowNum, 4 + hasDasIdsOffset) = tofMinMax[0];
  m_calibrationTable->cell<double>(rowNum, 5 + hasDasIdsOffset) = tofMinMax[1];
}

vector<double> PDCalibration::getTOFminmax(const double difc, const double difa,
                                           const double tzero) {
  vector<double> tofminmax(2);

  tofminmax[0] = Kernel::Diffraction::calcTofMin(difc, difa, tzero, m_tofMin);
  tofminmax[1] = Kernel::Diffraction::calcTofMax(difc, difa, tzero, m_tofMax);

  return tofminmax;
}
MatrixWorkspace_sptr PDCalibration::load(const std::string filename) {
  // TODO this assumes that all files are event-based
  const double maxChunkSize = getProperty("MaxChunkSize");
  const double filterBadPulses = getProperty("FilterBadPulses");

  auto alg = createChildAlgorithm("LoadEventAndCompress");
  alg->setLoggingOffset(1);
  alg->setProperty("Filename", filename);
  alg->setProperty("MaxChunkSize", maxChunkSize);
  alg->setProperty("FilterByTofMin", m_tofMin);
  alg->setProperty("FilterByTofMax", m_tofMax);
  alg->setProperty("FilterBadPulses", filterBadPulses);
  alg->setProperty("LoadMonitors", false);
  alg->executeAsChildAlg();
  API::Workspace_sptr workspace = alg->getProperty("OutputWorkspace");

  return boost::dynamic_pointer_cast<MatrixWorkspace>(workspace);
}

MatrixWorkspace_sptr PDCalibration::loadAndBin() {
  m_uncalibratedWS = getProperty("InputWorkspace");
  return rebin(m_uncalibratedWS);
}

API::MatrixWorkspace_sptr PDCalibration::rebin(API::MatrixWorkspace_sptr wksp) {
  g_log.information("Binning data in time-of-flight");
  auto rebin = createChildAlgorithm("Rebin");
  rebin->setLoggingOffset(1);
  rebin->setProperty("InputWorkspace", wksp);
  rebin->setProperty("OutputWorkspace", wksp);
  rebin->setProperty("Params", getPropertyValue("TofBinning"));
  rebin->setProperty("PreserveEvents", true);
  rebin->executeAsChildAlg();
  wksp = rebin->getProperty("OutputWorkspace");

  return wksp;
}

void PDCalibration::createCalTableFromExisting() {
  API::ITableWorkspace_sptr calibrationTableOld =
      getProperty("PreviousCalibrationTable");
  if (calibrationTableOld == nullptr) {
    // load from file
    std::string filename = getProperty("PreviousCalibrationFile");
    auto alg = createChildAlgorithm("LoadDiffCal");
    alg->setLoggingOffset(1);
    alg->setProperty("Filename", filename);
    alg->setProperty("WorkspaceName", "NOMold"); // TODO
    alg->setProperty("MakeGroupingWorkspace", false);
    alg->setProperty("MakeMaskWorkspace", false);
    alg->setProperty("TofMin", m_tofMin);
    alg->setProperty("TofMax", m_tofMax);
    alg->executeAsChildAlg();
    calibrationTableOld = alg->getProperty("OutputCalWorkspace");
  }

  m_hasDasIds = hasDasIDs(calibrationTableOld);

  // generate the map of detid -> row
  API::ColumnVector<int> detIDs = calibrationTableOld->getVector("detid");
  const size_t numDets = detIDs.size();
  for (size_t i = 0; i < numDets; ++i) {
    m_detidToRow[static_cast<detid_t>(detIDs[i])] = i;
  }

  // create a new workspace
  m_calibrationTable = boost::make_shared<DataObjects::TableWorkspace>();
  // TODO m_calibrationTable->setTitle("");
  m_calibrationTable->addColumn("int", "detid");
  m_calibrationTable->addColumn("double", "difc");
  m_calibrationTable->addColumn("double", "difa");
  m_calibrationTable->addColumn("double", "tzero");
  if (m_hasDasIds)
    m_calibrationTable->addColumn("int", "dasid");
  m_calibrationTable->addColumn("double", "tofmin");
  m_calibrationTable->addColumn("double", "tofmax");

  // copy over the values
  for (std::size_t rowNum = 0; rowNum < calibrationTableOld->rowCount();
       ++rowNum) {
    API::TableRow newRow = m_calibrationTable->appendRow();

    newRow << calibrationTableOld->getRef<int>("detid", rowNum);
    newRow << calibrationTableOld->getRef<double>("difc", rowNum);
    newRow << calibrationTableOld->getRef<double>("difa", rowNum);
    newRow << calibrationTableOld->getRef<double>("tzero", rowNum);
    if (m_hasDasIds)
      newRow << calibrationTableOld->getRef<int>("dasid", rowNum);

    const auto tofMinMax =
        getTOFminmax(calibrationTableOld->getRef<double>("difc", rowNum),
                     calibrationTableOld->getRef<double>("difa", rowNum),
                     calibrationTableOld->getRef<double>("tzero", rowNum));
    newRow << tofMinMax[0]; // tofmin
    newRow << tofMinMax[1]; // tofmax
  }
}

void PDCalibration::createCalTableNew() {
  // create new calibraion table for when an old one isn't loaded
  // using the signal workspace and CalculateDIFC
  auto alg = createChildAlgorithm("CalculateDIFC");
  alg->setLoggingOffset(1);
  alg->setProperty("InputWorkspace", m_uncalibratedWS);
  alg->executeAsChildAlg();
  API::MatrixWorkspace_const_sptr difcWS = alg->getProperty("OutputWorkspace");

  // create a new workspace
  m_calibrationTable = boost::make_shared<DataObjects::TableWorkspace>();
  // TODO m_calibrationTable->setTitle("");
  m_calibrationTable->addColumn("int", "detid");
  m_calibrationTable->addColumn("double", "difc");
  m_calibrationTable->addColumn("double", "difa");
  m_calibrationTable->addColumn("double", "tzero");
  m_hasDasIds = false;
  m_calibrationTable->addColumn("double", "tofmin");
  m_calibrationTable->addColumn("double", "tofmax");
  setProperty("OutputCalibrationTable", m_calibrationTable);

  const detid2index_map allDetectors =
      difcWS->getDetectorIDToWorkspaceIndexMap(true);

  // copy over the values
  detid2index_map::const_iterator it = allDetectors.begin();
  size_t i = 0;
  for (; it != allDetectors.end(); ++it) {
    const detid_t detID = it->first;
    m_detidToRow[detID] = i++;
    const size_t wi = it->second;
    API::TableRow newRow = m_calibrationTable->appendRow();
    newRow << detID;
    newRow << difcWS->y(wi)[0];
    newRow << 0.;      // difa
    newRow << 0.;      // tzero
    newRow << 0.;      // tofmin
    newRow << DBL_MAX; // tofmax
  }
}

void PDCalibration::createInformationWorkspaces() {
  // table for the fitted location of the various peaks
  m_peakPositionTable = boost::make_shared<DataObjects::TableWorkspace>();
  m_peakWidthTable = boost::make_shared<DataObjects::TableWorkspace>();
  m_peakHeightTable = boost::make_shared<DataObjects::TableWorkspace>();

  m_peakPositionTable->addColumn("int", "detid");
  m_peakWidthTable->addColumn("int", "detid");
  m_peakHeightTable->addColumn("int", "detid");

  for (double dSpacing : m_peaksInDspacing) {
    std::stringstream namess;
    namess << "@" << std::setprecision(5) << dSpacing;
    m_peakPositionTable->addColumn("double", namess.str());
    m_peakWidthTable->addColumn("double", namess.str());
    m_peakHeightTable->addColumn("double", namess.str());
  }
  m_peakPositionTable->addColumn("double", "chisq");
  m_peakPositionTable->addColumn("double", "normchisq");
  // residuals aren't needed for FWHM or height

  // convert the map of m_detidToRow to be a vector of detector ids
  std::vector<detid_t> detIds(m_detidToRow.size());
  for (const auto &it : m_detidToRow) {
    detIds[it.second] = it.first;
  }

  // copy the detector ids from the main table and add lots of NaNs
  for (const auto &detId : detIds) {
    API::TableRow newPosRow = m_peakPositionTable->appendRow();
    API::TableRow newWidthRow = m_peakWidthTable->appendRow();
    API::TableRow newHeightRow = m_peakHeightTable->appendRow();

    newPosRow << detId;
    newWidthRow << detId;
    newHeightRow << detId;

    for (double dSpacing : m_peaksInDspacing) {
      UNUSED_ARG(dSpacing);
      newPosRow << std::nan("");
      newWidthRow << std::nan("");
      newHeightRow << std::nan("");
    }
  }
}

API::MatrixWorkspace_sptr PDCalibration::calculateResolutionTable() {
  DataObjects::SpecialWorkspace2D_sptr resolutionWksp =
      boost::make_shared<DataObjects::SpecialWorkspace2D>(
          m_uncalibratedWS->getInstrument());
  resolutionWksp->setTitle("average width/height");

  // assume both tables have the same number of rows b/c the algorithm created
  // both
  // they are also in the same order
  // accessing cells is done by (row, col)
  const size_t numRows = m_peakPositionTable->rowCount();
  const size_t numPeaks = m_peaksInDspacing.size();
  std::vector<double> resolution; // vector of non-nan resolutions
  for (size_t rowIndex = 0; rowIndex < numRows; ++rowIndex) {
    resolution.clear();
    // first column is detid
    const detid_t detId =
        static_cast<detid_t>(m_peakPositionTable->Int(rowIndex, 0));
    for (size_t peakIndex = 1; peakIndex < numPeaks + 1; ++peakIndex) {
      const double pos = m_peakPositionTable->Double(rowIndex, peakIndex);
      if (std::isnormal(pos)) {
        resolution.push_back(m_peakWidthTable->Double(rowIndex, peakIndex) /
                             pos);
      }
    }

    if (resolution.empty()) {
      resolutionWksp->setValue(detId, 0.,
                               0.); // instrument view doesn't like nan
    } else {
      // calculate the mean
      const double mean =
          std::accumulate(resolution.begin(), resolution.end(), 0.) /
          static_cast<double>(resolution.size());
      double stddev = 0.;
      for (const auto value : resolution) {
        stddev += (value - mean) * (value * mean);
      }
      stddev = std::sqrt(stddev / static_cast<double>(resolution.size() - 1));
      resolutionWksp->setValue(detId, mean, stddev);
    }
  }

  return resolutionWksp;
}

API::ITableWorkspace_sptr
PDCalibration::sortTableWorkspace(API::ITableWorkspace_sptr &table) {
  auto alg = createChildAlgorithm("SortTableWorkspace");
  alg->setLoggingOffset(1);
  alg->setProperty("InputWorkspace", table);
  alg->setProperty("OutputWorkspace", table);
  alg->setProperty("Columns", "detid");
  alg->executeAsChildAlg();
  table = alg->getProperty("OutputWorkspace");

  return table;
}

/// NEW: convert peak positions in dSpacing to peak centers workspace
std::pair<API::MatrixWorkspace_sptr, API::MatrixWorkspace_sptr>
PDCalibration::createTOFPeakCenterFitWindowWorkspaces(
    API::MatrixWorkspace_sptr dataws, const double peakWindowMaxInDSpacing) {

  // calculate from peaks in dpsacing to peak fit window in dspacing
  const auto windowsInDSpacing =
      dSpacingWindows(m_peaksInDspacing, peakWindowMaxInDSpacing);

  for (std::size_t i = 0; i < m_peaksInDspacing.size(); ++i) {
    g_log.information() << "[" << i << "] " << windowsInDSpacing[2 * i] << " < "
                        << m_peaksInDspacing[i] << " < "
                        << windowsInDSpacing[2 * i + 1] << std::endl;
  }

  // create workspaces
  size_t numspec = dataws->getNumberHistograms();
  size_t numpeaks = m_peaksInDspacing.size();
  MatrixWorkspace_sptr peak_pos_ws =
      create<Workspace2D>(numspec, Points(numpeaks));
  MatrixWorkspace_sptr peak_window_ws =
      create<Workspace2D>(numspec, Points(numpeaks * 2));

  const int64_t NUM_HIST = static_cast<int64_t>(dataws->getNumberHistograms());
  API::Progress prog(this, 0., .2, NUM_HIST);

  PRAGMA_OMP(parallel for schedule(dynamic, 1) )
  for (int64_t iws = 0; iws < static_cast<int64_t>(NUM_HIST); ++iws) {
    PARALLEL_START_INTERUPT_REGION
    // calculatePositionWindowInTOF
    PDCalibration::FittedPeaks peaks(dataws, static_cast<size_t>(iws));
    auto toTof = getDSpacingToTof(peaks.detid);
    peaks.setPositions(m_peaksInDspacing, windowsInDSpacing, toTof);
    peak_pos_ws->setPoints(iws, peaks.inTofPos);
    peak_window_ws->setPoints(iws, peaks.inTofWindows);
    prog.report();

    PARALLEL_END_INTERUPT_REGION
  }
  PARALLEL_CHECK_INTERUPT_REGION

  return std::make_pair(peak_pos_ws, peak_window_ws);
}

} // namespace Algorithms
} // namespace Mantid
