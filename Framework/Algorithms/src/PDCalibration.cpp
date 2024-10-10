// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidAlgorithms/PDCalibration.h"
#include "MantidAPI/FileProperty.h"
#include "MantidAPI/FuncMinimizerFactory.h"
#include "MantidAPI/FunctionFactory.h"
#include "MantidAPI/IEventList.h"
#include "MantidAPI/IPeakFunction.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/Run.h"
#include "MantidAPI/SpectrumInfo.h"
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
#include "MantidKernel/ArrayLengthValidator.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/BoundedValidator.h"
#include "MantidKernel/CompositeValidator.h"
#include "MantidKernel/EnumeratedString.h"
#include "MantidKernel/EnumeratedStringProperty.h"
#include "MantidKernel/ListValidator.h"
#include "MantidKernel/MandatoryValidator.h"
#include "MantidKernel/RebinParamsValidator.h"
#include "MantidKernel/Unit.h"

#include <algorithm>
#include <gsl/gsl_multifit_nlin.h>
#include <gsl/gsl_multimin.h>
#include <limits>
#include <numeric>

namespace Mantid::Algorithms {

using namespace Mantid::DataObjects;
using namespace Mantid::HistogramData;
using Mantid::API::AnalysisDataService;
using Mantid::API::FileProperty;
using Mantid::API::MatrixWorkspace;
using Mantid::API::MatrixWorkspace_const_sptr;
using Mantid::API::MatrixWorkspace_sptr;
using Mantid::API::PropertyMode;
using Mantid::API::WorkspaceProperty;
using Mantid::DataObjects::EventWorkspace;
using Mantid::DataObjects::MaskWorkspace;
using Mantid::DataObjects::MaskWorkspace_const_sptr;
using Mantid::DataObjects::MaskWorkspace_sptr;
using Mantid::Geometry::Instrument_const_sptr;
using Mantid::Kernel::ArrayBoundedValidator;
using Mantid::Kernel::ArrayProperty;
using Mantid::Kernel::BoundedValidator;
using Mantid::Kernel::CompositeValidator;
using Mantid::Kernel::Direction;
using Mantid::Kernel::Logger;
using Mantid::Kernel::MandatoryValidator;
using Mantid::Kernel::RebinParamsValidator;
using Mantid::Kernel::StringListValidator;
using std::vector;

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(PDCalibration)

namespace { // anonymous
const auto isNonZero = [](const double value) { return value != 0.; };

// properties about peak positions to fit
const std::vector<std::string> peakTypesNames{"BackToBackExponential", "Gaussian", "Lorentzian", "PseudoVoigt",
                                              "IkedaCarpenterPV"};
enum class PeakMode { BackToBackExponential, Gaussian, Lorentzian, PseudoVoigt, IkedaCarpenterPV, enum_count };
typedef Mantid::Kernel::EnumeratedString<PeakMode, &peakTypesNames> PEAKMODE;

const vector<std::string> backgroundTypesNames{"Flat", "Linear", "Quadratic"};
enum class BackgroundMode { FLAT, LINEAR, QUADRATIC, enum_count };
typedef Mantid::Kernel::EnumeratedString<BackgroundMode, &backgroundTypesNames> BACKGROUND_MODE;

} // namespace

/// private inner class
class PDCalibration::FittedPeaks {
public:
  /**
   * Find the bins with non-zero counts and with minimum and maximum TOF
   *
   * @param wksp :: Input signal workspace
   * @param wkspIndex :: workspace index
   *
   * @throws runtime_error :: more than one detector is associated to the
   * workspace index
   */
  FittedPeaks(const API::MatrixWorkspace_const_sptr &wksp, const std::size_t wkspIndex) {
    this->wkspIndex = wkspIndex;

    // convert workspace index into detector id
    const auto &spectrum = wksp->getSpectrum(wkspIndex);
    this->detid = spectrum.getDetectorIDs();

    const auto &X = spectrum.x();
    const auto &Y = spectrum.y();
    tofMin = X.front();
    tofMax = X.back();

    // determine tof min supported by the workspace (non-zero counts)
    size_t minIndex = 0; // want to store value
    for (; minIndex < Y.size(); ++minIndex) {
      if (isNonZero(Y[minIndex])) {
        tofMin = X[minIndex];
        break; // we found the first bin with non-zero counts
      }
    }

    // determine tof max supported by the workspace (non-zero counts)
    size_t maxIndex = Y.size() - 1;
    for (; maxIndex > minIndex; --maxIndex) {
      if (isNonZero(Y[maxIndex])) {
        tofMax = X[maxIndex];
        break; // we found the last bin with non-zero counts
      }
    }
  }

  /**
   * Store the positions of the peak centers, as well as the left and right
   * fit ranges for each peak, in TOF units
   *
   * @param peaksInD :: peak centers, in d-spacing
   * @param peaksInDWindows :: left and right fit ranges for each peak
   * @param difa :: difa diffractometer constant (quadratic term)
   * @param difc :: difc diffractometer constant (linear term)
   * @param tzero :: tzero diffractometer constant (constant term)
   */
  void setPositions(const std::vector<double> &peaksInD, const std::vector<double> &peaksInDWindows, const double difa,
                    const double difc, const double tzero) {
    // clear out old values
    inDPos.clear();
    inTofPos.clear();
    inTofWindows.clear();

    // assign things
    inDPos.assign(peaksInD.begin(), peaksInD.end());
    inTofPos.assign(peaksInD.begin(), peaksInD.end());
    inTofWindows.assign(peaksInDWindows.begin(), peaksInDWindows.end());

    // convert the bits that matter to TOF
    Kernel::Units::dSpacing dSpacingUnit;
    std::vector<double> yunused;
    dSpacingUnit.toTOF(
        inTofPos, yunused, -1, 0,
        {{Kernel::UnitParams::difa, difa}, {Kernel::UnitParams::difc, difc}, {Kernel::UnitParams::tzero, tzero}});
    dSpacingUnit.toTOF(
        inTofWindows, yunused, -1, 0,
        {{Kernel::UnitParams::difa, difa}, {Kernel::UnitParams::difc, difc}, {Kernel::UnitParams::tzero, tzero}});
  }

  std::size_t wkspIndex;
  std::set<detid_t> detid;
  double tofMin;                // TOF of bin with minimum TOF and non-zero counts
  double tofMax;                // TOF of bin with maximum TOF and non-zero counts
  std::vector<double> inTofPos; // peak centers, in TOF
  // left and right fit ranges for each peak center, in TOF
  std::vector<double> inTofWindows;
  std::vector<double> inDPos; // peak centers, in d-spacing
};

//----------------------------------------------------------------------------------------------
/** Constructor
 */
PDCalibration::PDCalibration() = default;

//----------------------------------------------------------------------------------------------
/** Destructor
 */
PDCalibration::~PDCalibration() = default;

//----------------------------------------------------------------------------------------------

/// Algorithms name for identification. @see Algorithm::name
const std::string PDCalibration::name() const { return "PDCalibration"; }

/// Algorithm's version for identification. @see Algorithm::version
int PDCalibration::version() const { return 1; }

/// Algorithm's category for identification. @see Algorithm::category
const std::string PDCalibration::category() const { return "Diffraction\\Calibration"; }

/// Algorithm's summary for use in the GUI and help. @see Algorithm::summary
const std::string PDCalibration::summary() const {
  return "This algorithm determines the diffractometer constants that convert diffraction spectra "
         "from time-of-flight (TOF) to d-spacing. The results are output to a calibration table.";
}

//----------------------------------------------------------------------------------------------
/** Initialize the algorithm's properties.
 */
void PDCalibration::init() {
  declareProperty(std::make_unique<WorkspaceProperty<MatrixWorkspace>>("InputWorkspace", "", Direction::Input),
                  "Input workspace containing spectra as a function of TOF measured on a standard sample.");

  auto mustBePositive = std::make_shared<BoundedValidator<int>>();
  mustBePositive->setLower(0);
  declareProperty("StartWorkspaceIndex", 0, mustBePositive, "Starting workspace index for fit");
  declareProperty(
      "StopWorkspaceIndex", EMPTY_INT(),
      "Last workspace index for fit is the smaller of this value and the workspace index of last spectrum.");

  declareProperty(
      std::make_unique<ArrayProperty<double>>("TofBinning", std::make_shared<RebinParamsValidator>()),
      "Min, Step, and Max of TOF bins. "
      "Logarithmic binning is used if Step is negative. Chosen binning should ensure sufficient datapoints across "
      "the peaks to be fitted, considering the number of parameters required by PeakFunction and BackgroundType.");

  const std::vector<std::string> exts2{".h5", ".cal"};
  declareProperty(std::make_unique<FileProperty>("PreviousCalibrationFile", "", FileProperty::OptionalLoad, exts2),
                  "An existent file to be loaded into a calibration table, which is used for converting PeakPositions "
                  "from d-spacing to TOF.");
  declareProperty(std::make_unique<WorkspaceProperty<API::ITableWorkspace>>(
                      "PreviousCalibrationTable", "", Direction::Input, API::PropertyMode::Optional),
                  "An existent calibration table used for converting PeakPositions from d-spacing to TOF. "
                  "This property has precedence over PreviousCalibrationFile.");

  // properties about peak positions to fit
  declareProperty(std::make_unique<Mantid::Kernel::EnumeratedStringProperty<PeakMode, &peakTypesNames>>("PeakFunction"),
                  "Function to fit input peaks.");
  setProperty("PeakFunction", "Gaussian");
  declareProperty(std::make_unique<Mantid::Kernel::EnumeratedStringProperty<BackgroundMode, &backgroundTypesNames>>(
                      "BackgroundType"),
                  "Function to fit input peaks background.");
  setProperty("BackgroundType", "Linear");

  auto peaksValidator = std::make_shared<CompositeValidator>();
  auto mustBePosArr = std::make_shared<Kernel::ArrayBoundedValidator<double>>();
  mustBePosArr->setLower(0.0);
  peaksValidator->add(mustBePosArr);
  peaksValidator->add(std::make_shared<MandatoryValidator<std::vector<double>>>());
  declareProperty(std::make_unique<ArrayProperty<double>>("PeakPositions", peaksValidator),
                  "Comma-delimited positions (d-spacing) of reference peaks. Care should be taken to avoid using peaks "
                  "whose predicted positions are too close considering their peak windows.");

  auto windowValidator = std::make_shared<CompositeValidator>();
  windowValidator->add(mustBePosArr);
  auto lengthValidator = std::make_shared<Kernel::ArrayLengthValidator<double>>();
  lengthValidator->setLengthMin(1);
  windowValidator->add(lengthValidator);
  windowValidator->add(std::make_shared<MandatoryValidator<std::vector<double>>>());
  declareProperty(std::make_unique<ArrayProperty<double>>("PeakWindow", "0.1", windowValidator),
                  "Width of the window (d-spacing) over which to fit a peak. If a single value is supplied, it will be "
                  "used as half "
                  "the window width for all peaks. Otherwise, the expected input is a comma-delimited list of 2*N "
                  "window boundaries, "
                  "where N is the number of values in PeakPositions. The order of the window boundaries should match "
                  "the order in PeakPositions.");

  auto min = std::make_shared<BoundedValidator<double>>();
  min->setLower(1e-3);
  declareProperty("PeakWidthPercent", EMPTY_DBL(), min,
                  "Used for estimating peak width (an initial parameter for peak fitting) by multiplying peak's value "
                  "in PeakPositions by this factor.");

  declareProperty("MinimumPeakHeight", 2.,
                  "Used for validating peaks before and after fitting. If a peak's observed/estimated or "
                  "fitted height is under this value, the peak will be marked as an error.");

  declareProperty("MaxChiSq", 100.,
                  "Used for validating peaks after fitting. If the chi-squared value is higher than this value, "
                  "the peak will be excluded from calibration. The recommended value is between 2 and 10.");

  declareProperty("ConstrainPeakPositions", false,
                  "If true, a peak center being fit will be constrained by the estimated peak position "
                  "+/- 0.5 * \"estimated peak width\", where the estimated peak position is the highest Y-value "
                  "position in the window, "
                  " and the estimated peak width is FWHM calculated over the window data.");

  declareProperty("HighBackground", false,
                  "Flag whether the input data has high background compared to peaks' heights. "
                  "This option is recommended for data with peak-to-background ratios under ~5.");

  declareProperty("MinimumSignalToNoiseRatio", 0.,
                  "Used for validating peaks before fitting. If the signal-to-noise ratio is under this value, "
                  "the peak will be excluded from fitting and calibration. This check does not apply to peaks for "
                  "which the noise cannot be estimated. "
                  "The minimum recommended value is 12.");

  std::vector<std::string> modes{"DIFC", "DIFC+TZERO", "DIFC+TZERO+DIFA"};
  declareProperty("CalibrationParameters", "DIFC", std::make_shared<StringListValidator>(modes),
                  "Select which diffractometer constants (GSAS convention) to determine.");

  declareProperty(std::make_unique<ArrayProperty<double>>("TZEROrange"),
                  "Range for allowable calibrated TZERO value. Default: no restriction.");

  declareProperty(std::make_unique<ArrayProperty<double>>("DIFArange"),
                  "Range for allowable calibrated DIFA value. Default: no restriction.");

  declareProperty("UseChiSq", false,
                  "Defines the weighting scheme used in the least-squares fit of the extracted peak centers "
                  "that determines the diffractometer constants. If true, the peak weight will be "
                  "the inverse square of the error on the fitted peak center. If false, the peak weight will be "
                  "the square of the fitted peak height.");

  declareProperty(
      std::make_unique<WorkspaceProperty<API::ITableWorkspace>>("OutputCalibrationTable", "", Direction::Output),
      "Output table workspace containing the calibration.");

  // Mantid's python API _requires_ a non empty-string name for any Output workspace, even when 'PropertyMode::Optional'
  // is specified.
  declareProperty(std::make_unique<WorkspaceProperty<MaskWorkspace>>("MaskWorkspace", "_empty_", Direction::Output,
                                                                     PropertyMode::Optional),
                  "Mask workspace (optional input / output workspace):"
                  "  when specified, if the workspace already exists, any incoming masked detectors will be combined"
                  "  with any additional outgoing masked detectors detected by the algorithm");

  declareProperty(
      std::make_unique<WorkspaceProperty<API::WorkspaceGroup>>("DiagnosticWorkspaces", "", Direction::Output),
      "Auxiliary workspaces containing extended information on the calibration results.");

  declareProperty("MinimumPeakTotalCount", EMPTY_DBL(),
                  "Used for validating peaks before fitting. If the total peak window Y-value count "
                  "is under this value, the peak will be excluded from fitting and calibration.");

  declareProperty("MinimumSignalToSigmaRatio", 0.,
                  "Used for validating peaks after fitting. If the signal-to-sigma ratio is under this value, "
                  "the peak will be excluded from fitting and calibration.");

  // make group for Input properties
  std::string inputGroup("Input Options");
  setPropertyGroup("InputWorkspace", inputGroup);
  setPropertyGroup("StartWorkspaceIndex", inputGroup);
  setPropertyGroup("StopWorkspaceIndex", inputGroup);
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
  setPropertyGroup("MinimumSignalToNoiseRatio", fitPeaksGroup);
  setPropertyGroup("MinimumPeakTotalCount", fitPeaksGroup);
  setPropertyGroup("MinimumSignalToSigmaRatio", fitPeaksGroup);
  setPropertyGroup("HighBackground", fitPeaksGroup);
  setPropertyGroup("MaxChiSq", fitPeaksGroup);
  setPropertyGroup("ConstrainPeakPositions", fitPeaksGroup);

  // make group for type of calibration
  std::string calGroup("Calibration Type");
  setPropertyGroup("CalibrationParameters", calGroup);
  setPropertyGroup("TZEROrange", calGroup);
  setPropertyGroup("DIFArange", calGroup);
  setPropertyGroup("UseChiSq", calGroup);
}

std::map<std::string, std::string> PDCalibration::validateInputs() {
  std::map<std::string, std::string> messages;

  if (MaskWorkspace_const_sptr maskWS = getProperty("MaskWorkspace")) {
    MatrixWorkspace_const_sptr inputWS = getProperty("InputWorkspace");

    // detectors which are monitors are not included in the mask
    if (maskWS->getInstrument()->getNumberDetectors(true) != inputWS->getInstrument()->getNumberDetectors(true)) {
      messages["MaskWorkspace"] = "incoming mask workspace must have the same instrument as the input workspace";
    } else if (maskWS->getNumberHistograms() != inputWS->getInstrument()->getNumberDetectors(true)) {
      messages["MaskWorkspace"] = "incoming mask workspace must have one spectrum per detector";
    }
  }

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

  vector<double> peakWindow = getProperty("PeakWindow");
  vector<double> peakCentres = getProperty("PeakPositions");
  if (peakWindow.size() > 1 && peakWindow.size() != 2 * peakCentres.size()) {
    messages["PeakWindow"] = "PeakWindow must be a vector with either 1 element (interpreted as half the width of "
                             "the window) or twice the number of peak centres provided.";
  }

  return messages;
}

namespace {

/// @return ``true`` if an input table contains a column termed "dasid"
bool hasDasIDs(const API::ITableWorkspace_const_sptr &table) {
  const auto columnNames = table->getColumnNames();
  return (std::find(columnNames.begin(), columnNames.end(), std::string("dasid")) != columnNames.end());
}

/**
 * Conversion factor between fit-function width and FWHM. Factors are calculated
 * for "Gaussian" and "Lorentzian" functions. For other functions, a value
 * of 1.0 is returned
 *
 * @param peakshape :: name of the fitting function
 */
double getWidthToFWHM(const std::string &peakshape) {
  // could we use actual peak function here?
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
  std::vector<double> peakWindow = getProperty("PeakWindow");
  const std::size_t NUMPEAKS = m_peaksInDspacing.size();
  // if peak windows given for each peak center, sort all by peak center
  if (peakWindow.size() > 1) {
    // load windows into ordered map keyed by peak center
    // this preserves association of centers and window edges
    std::map<double, std::pair<double, double>> peakEdgeAndCenter;
    for (std::size_t i = 0; i < m_peaksInDspacing.size(); i++) {
      peakEdgeAndCenter[m_peaksInDspacing[i]] = {peakWindow[2 * i], peakWindow[2 * i + 1]};
    }
    m_peaksInDspacing.clear();
    m_peaksInDspacing.reserve(NUMPEAKS);
    peakWindow.clear();
    peakWindow.reserve(2 * NUMPEAKS);
    // retrieve ordered center, windows
    for (auto it = peakEdgeAndCenter.begin(); it != peakEdgeAndCenter.end(); it++) {
      m_peaksInDspacing.push_back(it->first);
      peakWindow.push_back(it->second.first);
      peakWindow.push_back(it->second.second);
    }
  } else {
    // Sort peak positions, required for correct peak window calculations
    std::sort(m_peaksInDspacing.begin(), m_peaksInDspacing.end());
  }

  const double minPeakHeight = getProperty("MinimumPeakHeight");
  const double minPeakTotalCount = getProperty("MinimumPeakTotalCount");
  const double minSignalToNoiseRatio = getProperty("MinimumSignalToNoiseRatio");
  const double minSignalToSigmaRatio = getProperty("MinimumSignalToSigmaRatio");
  const double maxChiSquared = getProperty("MaxChiSq");

  const std::string calParams = getPropertyValue("CalibrationParameters");
  if (calParams == std::string("DIFC"))
    m_numberMaxParams = 1;
  else if (calParams == std::string("DIFC+TZERO"))
    m_numberMaxParams = 2;
  else if (calParams == std::string("DIFC+TZERO+DIFA"))
    m_numberMaxParams = 3;
  else
    throw std::runtime_error("Encountered impossible CalibrationParameters value");

  m_uncalibratedWS = loadAndBin();
  setProperty("InputWorkspace", m_uncalibratedWS);

  m_startWorkspaceIndex = getProperty("StartWorkspaceIndex");
  m_stopWorkspaceIndex = isDefault("StopWorkspaceIndex") ? static_cast<int>(m_uncalibratedWS->getNumberHistograms() - 1)
                                                         : getProperty("StopWorkspaceIndex");

  auto uncalibratedEWS = std::dynamic_pointer_cast<EventWorkspace>(m_uncalibratedWS);
  auto isEvent = bool(uncalibratedEWS);

  // Load Previous Calibration or create calibration table from signal file
  if ((!static_cast<std::string>(getProperty("PreviousCalibrationFile")).empty()) ||
      (!getPropertyValue("PreviousCalibrationTable").empty())) { //"PreviousCalibrationTable"
    createCalTableFromExisting();
  } else {
    createCalTableNew(); // calculates "difc" values from instrument geometry
  }
  createInformationWorkspaces();

  // Use the incoming mask workspace, or start a new one if the workspace does not exist.
  MaskWorkspace_sptr maskWS;
  if (!isDefault("MaskWorkspace")) {
    maskWS = getProperty("MaskWorkspace");
  }
  if (!maskWS) {
    g_log.debug() << "[PDCalibration]: CREATING new MaskWorkspace.\n";
    // A new mask is completely cleared at creation.
    maskWS = std::make_shared<MaskWorkspace>(m_uncalibratedWS->getInstrument());
  } else {
    g_log.debug() << "[PDCalibration]: Using EXISTING MaskWorkspace.\n";
  }
  // Include any incoming masked detector flags in the mask-workspace values.
  maskWS->combineFromDetectorMasks(m_uncalibratedWS->detectorInfo());

  const std::string peakFunction = getProperty("PeakFunction");
  const double WIDTH_TO_FWHM = getWidthToFWHM(peakFunction);
  if (WIDTH_TO_FWHM == 1.) {
    g_log.notice() << "Unknown conversion for \"" << peakFunction
                   << "\", found peak widths and resolution should not be "
                      "directly compared to delta-d/d";
  }
  auto NUMHIST = m_stopWorkspaceIndex - m_startWorkspaceIndex + 1;

  // Create a pair of workspaces, one containing the nominal peak centers in TOF units,
  // the other containing the left and right fitting ranges around each nominal
  // peak center, also in TOF units. This for each pixel of the instrument
  auto matrix_pair = createTOFPeakCenterFitWindowWorkspaces(m_uncalibratedWS, peakWindow);
  API::MatrixWorkspace_sptr tof_peak_center_ws = matrix_pair.first;
  API::MatrixWorkspace_sptr tof_peak_window_ws = matrix_pair.second;

  double peak_width_percent = getProperty("PeakWidthPercent");

  const std::string diagnostic_prefix = getPropertyValue("DiagnosticWorkspaces");

  // Refine the position of the peak centers starting from the nominal peak
  // centers and fitting them against a peak fit function (e.g. a Gaussian)
  auto algFitPeaks = createChildAlgorithm("FitPeaks", .2, .7);
  algFitPeaks->setLoggingOffset(3);

  algFitPeaks->setProperty("InputWorkspace", m_uncalibratedWS);

  // limit the spectra to fit
  algFitPeaks->setProperty("StartWorkspaceIndex", static_cast<int>(m_startWorkspaceIndex));
  algFitPeaks->setProperty("StopWorkspaceIndex", static_cast<int>(m_stopWorkspaceIndex));

  // theoretical peak center
  algFitPeaks->setProperty("PeakCentersWorkspace", tof_peak_center_ws);

  // peak and background functions
  algFitPeaks->setProperty<std::string>("PeakFunction", peakFunction);
  algFitPeaks->setProperty<std::string>("BackgroundType", getProperty("BackgroundType"));
  // peak range setup
  algFitPeaks->setProperty("FitPeakWindowWorkspace", tof_peak_window_ws);
  algFitPeaks->setProperty("PeakWidthPercent", peak_width_percent);
  algFitPeaks->setProperty("MinimumPeakHeight", minPeakHeight);
  algFitPeaks->setProperty("MinimumPeakTotalCount", minPeakTotalCount);
  algFitPeaks->setProperty("MinimumSignalToNoiseRatio", minSignalToNoiseRatio);
  algFitPeaks->setProperty("MinimumSignalToSigmaRatio", minSignalToSigmaRatio);
  // some fitting strategy
  algFitPeaks->setProperty("FitFromRight", true);
  const bool highBackground = getProperty("HighBackground");
  algFitPeaks->setProperty("HighBackground", highBackground);
  bool constrainPeakPosition = getProperty("ConstrainPeakPositions");
  algFitPeaks->setProperty("ConstrainPeakPositions",
                           constrainPeakPosition); // TODO Pete: need to test this option
  //  optimization setup // TODO : need to test LM or LM-MD
  algFitPeaks->setProperty("Minimizer", "Levenberg-Marquardt");
  algFitPeaks->setProperty("CostFunction", "Least squares");

  // FitPeaks will abstract the peak parameters if you ask (if using chisq then
  // need FitPeaks to output fitted params rather than height, width)
  const bool useChiSq = getProperty("UseChiSq");
  algFitPeaks->setProperty("RawPeakParameters", useChiSq);

  // Analysis output
  // If using a Gaussian peak shape plus a constant background, then
  // OutputPeakParametersWorkspace is a table with columns:
  // wsindex_0 peakindex_0  centre width height intensity A0 chi2
  //   ...
  // wsindex_0 peakindex_N  centre width height intensity A0 chi2
  //   ...
  //   ...
  // wsindex_M peakindex_N  centre width height intensity
  algFitPeaks->setPropertyValue("OutputPeakParametersWorkspace", diagnostic_prefix + "_fitparam");
  // contains the same intensities as input m_uncalibratedWS except within
  // the fitting range of each successfully fitted peak. Within this range,
  // the actual intensities are replaced with the values resulting from
  // evaluating the peak function (e.g. a Gaussian peak function)
  algFitPeaks->setPropertyValue("FittedPeaksWorkspace", diagnostic_prefix + "_fitted");
  if (useChiSq) {
    algFitPeaks->setPropertyValue("OutputParameterFitErrorsWorkspace", diagnostic_prefix + "_fiterrors");
  }

  // run and get the result
  algFitPeaks->executeAsChildAlg();
  g_log.information("finished FitPeaks");

  // get the fit result
  API::ITableWorkspace_sptr fittedTable = algFitPeaks->getProperty("OutputPeakParametersWorkspace");
  API::MatrixWorkspace_sptr calculatedWS = algFitPeaks->getProperty("FittedPeaksWorkspace");
  API::ITableWorkspace_sptr errorTable; // or nullptr as in FitPeaks L1997
  if (useChiSq) {
    errorTable = algFitPeaks->getProperty("OutputParameterFitErrorsWorkspace");
  }

  // check : for Pete
  if (!fittedTable)
    throw std::runtime_error("FitPeaks does not have output OutputPeakParametersWorkspace.");
  if (fittedTable->rowCount() != NUMHIST * m_peaksInDspacing.size())
    throw std::runtime_error("The number of rows in OutputPeakParametersWorkspace is not correct!");

  // END-OF (FitPeaks)
  const std::string backgroundType = getPropertyValue("BackgroundType");

  API::Progress prog(this, 0.7, 1.0, NUMHIST);

  // calculate fitting ranges to the left and right of each nominal peak
  // center, in d-spacing units
  const auto windowsInDSpacing = dSpacingWindows(m_peaksInDspacing, peakWindow);

  // get spectrum info to check workspace index correpsonds to a valid spectrum
  const auto &spectrumInfo = m_uncalibratedWS->spectrumInfo();

  // Scan the table containing the fit parameters for every peak, retrieve the
  // parameters for peaks that were successfully fitting, then use this info
  // to obtain difc, difa, and tzero for each pixel

  PRAGMA_OMP(parallel for schedule(dynamic, 1))
  for (int wkspIndex = m_startWorkspaceIndex; wkspIndex <= m_stopWorkspaceIndex; ++wkspIndex) {
    PARALLEL_START_INTERRUPT_REGION
    if ((isEvent && uncalibratedEWS->getSpectrum(wkspIndex).empty()) || !spectrumInfo.hasDetectors(wkspIndex) ||
        spectrumInfo.isMonitor(wkspIndex) ||
        maskWS->isMasked(m_uncalibratedWS->getSpectrum(wkspIndex).getDetectorIDs())) {
      prog.report();

      if (spectrumInfo.hasDetectors(wkspIndex) && !spectrumInfo.isMonitor(wkspIndex)) {
        if (isEvent && uncalibratedEWS->getSpectrum(wkspIndex).empty()) {
          maskWS->setMasked(m_uncalibratedWS->getSpectrum(wkspIndex).getDetectorIDs(), true);
          g_log.debug() << "FULLY masked spectrum, index: " << wkspIndex << "\n";
        }
      }

      continue;
    }

    // object to hold  information about the peak positions, detid, and workspace index
    PDCalibration::FittedPeaks peaks(m_uncalibratedWS, wkspIndex);
    const auto [dif_c, dif_a, tzero] =
        getDSpacingToTof(peaks.detid); // doesn't matter which one - all have same difc etc.
    peaks.setPositions(m_peaksInDspacing, windowsInDSpacing, dif_a, dif_c, tzero);

    // includes peaks that aren't used in the fit
    // The following data structures will hold information for the peaks
    // found in the current spectrum
    const size_t numPeaks = m_peaksInDspacing.size();
    // TOF of fitted peak centers, default `nan` for failed fitted peaks
    std::vector<double> tof_vec_full(numPeaks, std::nan(""));
    std::vector<double> d_vec;   // nominal peak centers of fitted peaks
    std::vector<double> tof_vec; // TOF of fitted peak centers only
    // width of fitted peak centers, default `nan` for failed fitted peaks
    std::vector<double> width_vec_full(numPeaks, std::nan(""));
    // height of fitted peak centers, default `nan` for failed fitted peaks
    std::vector<double> height_vec_full(numPeaks, std::nan(""));
    std::vector<double> weights; // weights for diff const fits
    // row where first peak occurs
    const size_t rowNumInFitTableOffset = (wkspIndex - m_startWorkspaceIndex) * numPeaks;
    // We assumed that the current spectrum contains peaks near the nominal
    // peak centers. Now we check how many peaks we actually found
    for (size_t peakIndex = 0; peakIndex < numPeaks; ++peakIndex) {
      size_t rowIndexInFitTable = rowNumInFitTableOffset + peakIndex;

      // check indices in PeaksTable
      if (fittedTable->getRef<int>("wsindex", rowIndexInFitTable) != wkspIndex)
        throw std::runtime_error("workspace index mismatch!");
      if (fittedTable->getRef<int>("peakindex", rowIndexInFitTable) != static_cast<int>(peakIndex))
        throw std::runtime_error("peak index mismatch but workspace index matched");

      const double chi2 = fittedTable->getRef<double>("chi2", rowIndexInFitTable);
      double centre = 0.0;
      double centre_error = 0.0; // only used if useChiSq true
      double width = 0.0;
      double height = 0.0;
      if (!useChiSq) {
        // get the effective peak parameters from FitPeaks output
        centre = fittedTable->getRef<double>("centre", rowIndexInFitTable);
        width = fittedTable->getRef<double>("width", rowIndexInFitTable);
        height = fittedTable->getRef<double>("height", rowIndexInFitTable);
      } else {
        // FitPeaks outputs actual fitting parameters
        // extract these from the peak function (not efficient)
        auto peakfunc = std::dynamic_pointer_cast<API::IPeakFunction>(
            API::FunctionFactory::Instance().createFunction(peakFunction));
        // set peak functio nparameters from fit
        for (size_t ipar = 0; ipar < peakfunc->nParams(); ipar++) {
          peakfunc->setParameter(ipar, fittedTable->getRef<double>(peakfunc->parameterName(ipar), rowIndexInFitTable));
        }
        centre = peakfunc->centre();
        width = peakfunc->fwhm();
        height = peakfunc->height();
        centre_error = errorTable->getRef<double>(peakfunc->getCentreParameterName(), rowIndexInFitTable);
      }

      // check chi-square
      if (chi2 > maxChiSquared || chi2 < 0.) {
        g_log.debug("failure to fit: chi2 > maximum");
        continue; // peak fit deemed as failure
      }

      // rule out of peak with wrong position. `centre` should be within its
      // left and right window ranges
      if (peaks.inTofWindows[2 * peakIndex] >= centre || peaks.inTofWindows[2 * peakIndex + 1] <= centre) {
        g_log.debug("failure to fit: peak center is out-of-range");
        continue; // peak fit deemed as failure
      }

      // check height: make sure 0 is smaller than 0
      if (height < minPeakHeight + 1.E-15) {
        g_log.debug("failure to fit: peak height is less than minimum");
        continue; // peak fit deemed as failure
      }

      // the peak fit was a success. Collect info
      g_log.getLogStream(Logger::Priority::PRIO_TRACE) << "successful fit: peak centered at " << centre << "\n";

      d_vec.emplace_back(m_peaksInDspacing[peakIndex]);
      tof_vec.emplace_back(centre);
      if (!useChiSq) {
        weights.emplace_back(height * height);
      } else {
        weights.emplace_back(1 / (centre_error * centre_error));
      }
      tof_vec_full[peakIndex] = centre;
      width_vec_full[peakIndex] = width;
      height_vec_full[peakIndex] = height;
    }

    if (d_vec.size() < 2) {
      // If less than two peaks were fitted successfully, indicate failure by
      //   masking all of the detectors contributing to the spectrum.
      maskWS->setMasked(peaks.detid, true);

      g_log.debug() << "MASKING:\n";
      for (const auto &det : peaks.detid) {
        g_log.debug() << "  " << det << "\n";
      }
      g_log.debug() << "\n";

      continue;
    } else {
      // obtain difc, difa, and t0 by fitting the nominal peak center
      // positions, in d-spacing against the fitted peak center positions, in
      // TOF units.
      double difc = 0., t0 = 0., difa = 0.;
      fitDIFCtZeroDIFA_LM(d_vec, tof_vec, weights, difc, t0, difa);
      for (auto iter = peaks.detid.begin(); iter != peaks.detid.end(); ++iter) {
        auto det = *iter;
        const auto rowIndexOutputPeaks = m_detidToRow[det];
        // chisq represent the deviations between the nominal peak positions
        // and the peak positions using the GSAS formula with optimized difc,
        // difa, and tzero
        double chisq = 0.;
        Mantid::Kernel::Units::dSpacing dSpacingUnit;
        dSpacingUnit.initialize(-1., 0,
                                Kernel::UnitParametersMap{{Kernel::UnitParams::difa, difa},
                                                          {Kernel::UnitParams::difc, difc},
                                                          {Kernel::UnitParams::tzero, t0}});
        for (std::size_t i = 0; i < numPeaks; ++i) {
          if (std::isnan(tof_vec_full[i]))
            continue;
          // Find d-spacing using the GSAS formula with optimized difc, difa,
          // t0 for the TOF of the current peak's center.
          const double dspacing = dSpacingUnit.singleFromTOF(tof_vec_full[i]);
          // `temp` is residual between the nominal position in d-spacing for
          // the current peak, and the fitted position in d-spacing
          const double temp = m_peaksInDspacing[i] - dspacing;
          chisq += (temp * temp);
          m_peakPositionTable->cell<double>(rowIndexOutputPeaks, i + 1) = dspacing;
          m_peakWidthTable->cell<double>(rowIndexOutputPeaks, i + 1) =
              WIDTH_TO_FWHM * (width_vec_full[i] / (2 * difa * dspacing + difc));
          m_peakHeightTable->cell<double>(rowIndexOutputPeaks, i + 1) = height_vec_full[i];
        }
        m_peakPositionTable->cell<double>(rowIndexOutputPeaks, m_peaksInDspacing.size() + 1) = chisq;
        m_peakPositionTable->cell<double>(rowIndexOutputPeaks, m_peaksInDspacing.size() + 2) =
            chisq / static_cast<double>(numPeaks - 1);

        setCalibrationValues(det, difc, difa, t0);
      }
    }
    prog.report();

    PARALLEL_END_INTERRUPT_REGION
  }
  PARALLEL_CHECK_INTERRUPT_REGION

  // sort the calibration tables by increasing detector ID
  m_calibrationTable = sortTableWorkspace(m_calibrationTable);
  setProperty("OutputCalibrationTable", m_calibrationTable);

  // Return the mask workspace only if it was specified as a parameter.
  if (!isDefault("MaskWorkspace")) {
    // Align the detector mask flags of the mask workspace with the workspace values:
    maskWS->combineToDetectorMasks();
    setProperty("MaskWorkspace", maskWS);
  }

  // fix-up the diagnostic workspaces
  m_peakPositionTable = sortTableWorkspace(m_peakPositionTable);
  m_peakWidthTable = sortTableWorkspace(m_peakWidthTable);
  m_peakHeightTable = sortTableWorkspace(m_peakHeightTable);

  // a derived table from the position and width
  auto resolutionWksp = calculateResolutionTable();

  // set the diagnostic workspaces out
  auto diagnosticGroup = std::make_shared<API::WorkspaceGroup>();
  // add workspaces calculated by FitPeaks
  API::AnalysisDataService::Instance().addOrReplace(diagnostic_prefix + "_fitparam", fittedTable);
  diagnosticGroup->addWorkspace(fittedTable);
  API::AnalysisDataService::Instance().addOrReplace(diagnostic_prefix + "_fitted", calculatedWS);
  diagnosticGroup->addWorkspace(calculatedWS);
  if (useChiSq) {
    API::AnalysisDataService::Instance().addOrReplace(diagnostic_prefix + "_fiterror", errorTable);
    diagnosticGroup->addWorkspace(errorTable);
  }

  // add workspaces calculated by PDCalibration
  API::AnalysisDataService::Instance().addOrReplace(diagnostic_prefix + "_dspacing", m_peakPositionTable);
  diagnosticGroup->addWorkspace(m_peakPositionTable);
  API::AnalysisDataService::Instance().addOrReplace(diagnostic_prefix + "_width", m_peakWidthTable);
  diagnosticGroup->addWorkspace(m_peakWidthTable);
  API::AnalysisDataService::Instance().addOrReplace(diagnostic_prefix + "_height", m_peakHeightTable);
  diagnosticGroup->addWorkspace(m_peakHeightTable);
  API::AnalysisDataService::Instance().addOrReplace(diagnostic_prefix + "_resolution", resolutionWksp);
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
  // vector<weights>]
  // index as      [0,        1,         2,         , 2+n           , 2+2n]
  const std::vector<double> *peakVec = reinterpret_cast<std::vector<double> *>(peaks);
  // number of peaks being fit
  const auto numPeaks = static_cast<size_t>(peakVec->at(0));
  // number of parameters
  const auto numParams = static_cast<size_t>(peakVec->at(1));

  // isn't strictly necessary, but makes reading the code much easier
  const std::vector<double> tofObs(peakVec->begin() + 2, peakVec->begin() + 2 + numPeaks);
  const std::vector<double> dspace(peakVec->begin() + (2 + numPeaks), peakVec->begin() + (2 + 2 * numPeaks));
  const std::vector<double> weights(peakVec->begin() + (2 + 2 * numPeaks), peakVec->begin() + (2 + 3 * numPeaks));

  // create the function to convert tof to dspacing
  double difc = gsl_vector_get(v, 0);
  double tzero = 0.;
  double difa = 0.;
  if (numParams > 1) {
    tzero = gsl_vector_get(v, 1);
    if (numParams > 2)
      difa = gsl_vector_get(v, 2);
  }
  Mantid::Kernel::Units::dSpacing dSpacingUnit;
  dSpacingUnit.initialize(-1., 0,
                          Kernel::UnitParametersMap{{Kernel::UnitParams::difa, difa},
                                                    {Kernel::UnitParams::difc, difc},
                                                    {Kernel::UnitParams::tzero, tzero}});

  // calculate the sum of the residuals from observed peaks
  double errsum = 0.0;
  for (size_t i = 0; i < numPeaks; ++i) {
    const double tofCalib = dSpacingUnit.singleToTOF(dspace[i]);
    const double errsum_i = std::pow(tofObs[i] - tofCalib, 2) * weights[i];
    errsum += errsum_i;
  }

  return errsum;
}

/**
 * Linear regression of  the nominal peak center positions, in d-spacing
 * against the fitted peak center positions, in TOF units.
 *
 * The equation to carry out the fit depends on the number of fit parameter
 * selected:
 * one fit parameter: TOF = DIFC * d
 * two fit parameter: TOF = DIFC * d + TZERO
 * three fit parameters: TOF = DIFC * d + TZERO + DIFA * d^2
 *
 * @param peaks :: array with structure (numpeaks, numfitparms, tof_1,
 * ...tof_numpeaks, d1,...d_numpeaks, weight_1,...weight_numpeaks)
 * @param difc :: will store optimized DIFC fit parameter
 * @param t0 :: will store optimized TZERO fit parameter
 * @param difa :: will store optimized DIFA fit parameter

 * @return cummulative error of the fit, zero if the fit fails
 */
double fitDIFCtZeroDIFA(std::vector<double> &peaks, double &difc, double &t0, double &difa) {
  const auto numParams = static_cast<size_t>(peaks[1]);

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
  const gsl_multimin_fminimizer_type *minimizerType = gsl_multimin_fminimizer_nmsimplex2;
  gsl_multimin_fminimizer *minimizer = gsl_multimin_fminimizer_alloc(minimizerType, numParams);
  gsl_multimin_fminimizer_set(minimizer, &minex_func, fitParams, stepSizes);

  // Finally do the fitting
  size_t iter = 0; // number of iterations
  const size_t MAX_ITER = 75 * numParams;
  int status = 0;
  do {
    iter++;
    status = gsl_multimin_fminimizer_iterate(minimizer);
    if (status)
      break;

    double size = gsl_multimin_fminimizer_size(minimizer);
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

/**
 * Fit the nominal peak center positions, in d-spacing against the fitted
 * peak center positions, in TOF units. We use the GSAS formula:
 *     TOF = DIFC∗d + DIFA∗d^2 + TZERO
 *
 * No fitting is performed if the number of peaks is less than two.
 *
 * @param d  :: nominal peak center positions, in d-spacing units
 * @param tof :: fitted peak center positions, in TOF units
 * @param weights :: weights for leastsq fit
 * @param difc :: output optimized DIFC parameter
 * @param t0 :: output optimized TZERO parameter
 * @param difa :: output optimized DIFA parameter
 */
void PDCalibration::fitDIFCtZeroDIFA_LM(const std::vector<double> &d, const std::vector<double> &tof,
                                        const std::vector<double> &weights, double &difc, double &t0, double &difa) {
  const size_t numPeaks = d.size();
  if (numPeaks <= 1) {
    return; // don't do anything
  }
  // number of fit parameters 1=[DIFC], 2=[DIFC,TZERO], 3=[DIFC,TZERO,DIFA]
  // set the maximum number of parameters that will be used
  // statistics doesn't support having too few peaks
  size_t maxParams = std::min<size_t>(numPeaks - 1, m_numberMaxParams);

  // this must have the same layout as the unpacking in gsl_costFunction above
  // `peaks` has the following structure for a fit session with three peaks
  // and two fit parameters:
  //     3, 2, tof1, tof2, tof3, d1, d2, d3, h21, h22, h23
  std::vector<double> peaks(numPeaks * 3 + 2, 0.);
  peaks[0] = static_cast<double>(d.size());
  peaks[1] = 1.; // number of parameters to fit. Initialize to just one
  for (size_t i = 0; i < numPeaks; ++i) {
    peaks[i + 2] = tof[i];
    peaks[i + 2 + numPeaks] = d[i];
    peaks[i + 2 + 2 * numPeaks] = weights[i];
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
  double best_difc = difc_start;
  double best_t0 = 0.;
  double best_difa = 0.;

  // loop over possible number of parameters, doing up to three sequential fits.
  // We first start with equation TOF = DIFC * d and obtain
  // optimized DIFC which serves as initial guess for next fit with equation
  // TOF = DIFC * d + TZERO, obtaining optimized DIFC and TZERO which serve as
  // initial guess for final fit TOF = DIFC * d + DIFA * d^2 + TZERO.
  for (size_t numParams = 1; numParams <= maxParams; ++numParams) {
    peaks[1] = static_cast<double>(numParams);
    double difc_local = best_difc;
    double t0_local = best_t0;
    double difa_local = best_difa;
    double errsum = fitDIFCtZeroDIFA(peaks, difc_local, t0_local, difa_local);
    if (errsum > 0.) {
      // normalize by degrees of freedom
      errsum = errsum / static_cast<double>(numPeaks - numParams);
      // save the best and forget the rest
      // the selected (DIFC, DIFA, TZERO) correspond to those of the fit that
      // minimizes errsum. It doesn't have to be the last fit.
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

  difc = best_difc;
  // check that something actually fit and set to the best result
  if (best_difc > 0. && best_errsum < std::numeric_limits<double>::max()) {
    t0 = best_t0;
    difa = best_difa;
  }
}

/**
 * Fitting ranges to the left and right of peak center (the window cannot exceed half the distance to the adjacent peaks
 * in either direction)
 * @param centres :: peak centers, in d-spacing
 * @param windows_in :: A vector of boundaries in d-sapcing (if only one element this is half the width of the window
 * for all peaks)
 * @return array containing left and right ranges for first peak, left and right
 * for second peak, and so on.
 */
vector<double> PDCalibration::dSpacingWindows(const std::vector<double> &centres,
                                              const std::vector<double> &windows_in) {

  if (!(windows_in.size() == 1 || windows_in.size() / 2 == centres.size()))
    throw std::logic_error("the peak-window vector must contain either a single peak-width value, or a pair of values "
                           "for each peak center specified");

  const std::size_t numPeaks = centres.size();

  // assumes distance between peaks can be used for window sizes
  if (!(numPeaks >= 2))
    throw std::logic_error("at least two peak centres must be specified: the distance between these centres will be "
                           "used to estimate the peak widths");

  vector<double> windows_out(2 * numPeaks);
  double left;
  double right;
  for (std::size_t i = 0; i < numPeaks; ++i) {
    if (windows_in.size() == 1) {
      left = centres[i] - windows_in[0];
      right = centres[i] + windows_in[0];
    } else {
      left = windows_in[2 * i];
      right = windows_in[2 * i + 1];
    }
    // check boundaries don't exceed half dist to adjacent peaks
    if (i > 0) {
      left = std::max(left, centres[i] - 0.5 * (centres[i] - centres[i - 1]));
    }
    if (i < numPeaks - 1) {
      right = std::min(right, centres[i] + 0.5 * (centres[i + 1] - centres[i]));
    }
    // set the windows
    windows_out[2 * i] = left;
    windows_out[2 * i + 1] = right;
  }
  return windows_out;
}

/**
 * Return a function that converts from d-spacing to TOF units for a particular
 * pixel, evaulating the GSAS conversion formula:
 *                  TOF = DIFC∗d + DIFA∗d^2 + TZERO
 *
 * @param detIds :: set of detector IDs
 */
std::tuple<double, double, double> PDCalibration::getDSpacingToTof(const std::set<detid_t> &detIds) {
  // to start this is the old calibration values
  double difc = 0.;
  double difa = 0.;
  double tzero = 0.;
  for (auto detId : detIds) {
    auto rowNum = m_detidToRow[detId];
    difc += m_calibrationTable->getRef<double>("difc", rowNum);
    difa += m_calibrationTable->getRef<double>("difa", rowNum);
    tzero += m_calibrationTable->getRef<double>("tzero", rowNum);
  }
  if (detIds.size() > 1) {
    double norm = 1. / static_cast<double>(detIds.size());
    difc = norm * difc;
    difa = norm * difa;
    tzero = norm * tzero;
  }

  return {difc, difa, tzero};
}

void PDCalibration::setCalibrationValues(const detid_t detid, const double difc, const double difa,
                                         const double tzero) {
  // don't set values that aren't in the table
  const auto rowIter = m_detidToRow.find(detid);
  if (rowIter == m_detidToRow.end())
    return;

  // get the row number
  auto rowNum = rowIter->second;

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

/**
 * Adjustment of TofMin and TofMax values, to ensure positive values of
 * d-spacing when converting from TOF to d-spacing using the GSAS equation
 *               TOF = DIFC∗d + DIFA∗d^2 + TZERO
 * See calcTofMin and calcTofMax for adjustments to TofMin and TofMax.
 *
 * @return two-item array containing adjusted TofMin and TofMax values
 */
vector<double> PDCalibration::getTOFminmax(const double difc, const double difa, const double tzero) {
  vector<double> tofminmax(2);

  Kernel::Units::dSpacing dSpacingUnit;
  tofminmax[0] = dSpacingUnit.calcTofMin(difc, difa, tzero, m_tofMin);
  tofminmax[1] = dSpacingUnit.calcTofMax(difc, difa, tzero, m_tofMax);

  return tofminmax;
}
MatrixWorkspace_sptr PDCalibration::load(const std::string &filename) {
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

  return std::dynamic_pointer_cast<MatrixWorkspace>(workspace);
}

/// load input workspace and rebin with parameters "TofBinning" provided by User
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

std::set<detid_t> PDCalibration::detIdsForTable() {
  std::set<detid_t> detids;

  // return early since everything is being used
  if (isDefault("StartWorkspaceIndex") && isDefault("StopWorkspaceIndex"))
    return detids;

  // get the indices to loop over
  std::size_t startIndex = static_cast<std::size_t>(m_startWorkspaceIndex);
  std::size_t stopIndex = static_cast<std::size_t>(m_stopWorkspaceIndex);

  for (std::size_t i = startIndex; i <= stopIndex; ++i) {
    const auto detidsForSpectrum = m_uncalibratedWS->getSpectrum(i).getDetectorIDs();
    for (const auto &detid : detidsForSpectrum) {
      detids.emplace(detid);
    }
  }
  return detids;
}

void PDCalibration::createCalTableHeader() {
  // create a new workspace
  m_calibrationTable = std::make_shared<DataObjects::TableWorkspace>();
  // TODO m_calibrationTable->setTitle("");
  m_calibrationTable->addColumn("int", "detid");
  m_calibrationTable->addColumn("double", "difc");
  m_calibrationTable->addColumn("double", "difa");
  m_calibrationTable->addColumn("double", "tzero");
  if (m_hasDasIds)
    m_calibrationTable->addColumn("int", "dasid");
  m_calibrationTable->addColumn("double", "tofmin");
  m_calibrationTable->addColumn("double", "tofmax");
}

/**
 * Read a calibration table workspace provided by user, or load from a file
 * provided by User
 *
 * Every pixel may need adjustment of TofMin and TofMax values, to ensure
 * positive values of d-spacing when converting from TOF to d-spacing using
 * the GSAS equation TOF = DIFC∗d + DIFA∗d^2 + TZERO. See calcTofMin and
 * calcTofMax for adjustments
 * to TofMin and TofMax.
 *
 * The output calibration has columns "detid", "difc", "difa", "tzero",
 * "tofmin", and "tofmax", and possible additional columns "dasid" and "offset"
 */
void PDCalibration::createCalTableFromExisting() {
  API::ITableWorkspace_sptr calibrationTableOld = getProperty("PreviousCalibrationTable");
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

  // create a new workspace
  this->createCalTableHeader();

  const auto includedDetids = detIdsForTable();
  const bool useAllDetids = includedDetids.empty();

  // generate the map of detid -> row
  API::ColumnVector<int> detIDs = calibrationTableOld->getVector("detid");
  std::size_t rowNum = 0;
  for (std::size_t i = 0; i < detIDs.size(); ++i) {
    // only add rows for detids that exist in input workspace
    if ((useAllDetids) || (includedDetids.count(detIDs[i]) > 0)) {
      m_detidToRow[static_cast<detid_t>(detIDs[i])] = rowNum++;
      API::TableRow newRow = m_calibrationTable->appendRow();
      int detid = calibrationTableOld->getRef<int>("detid", i);
      double difc = calibrationTableOld->getRef<double>("difc", i);
      double difa = calibrationTableOld->getRef<double>("difa", i);
      double tzero = calibrationTableOld->getRef<double>("tzero", i);

      newRow << detid << difc << difa << tzero;
      if (m_hasDasIds)
        newRow << calibrationTableOld->getRef<int>("dasid", i);

      // adjust tofmin and tofmax for this pixel
      const auto tofMinMax = getTOFminmax(difc, difa, tzero);
      newRow << tofMinMax[0] << tofMinMax[1]; // tofmin/tofmax
    }
  }
}

/**
 * Initialize the calibration table workspace. The output calibration
 * has columns "detid", "difc", "difa", "tzero", "tofmin", and "tofmax".
 * "difc" from the instrument geometry: m_n * (L1 + L2)  * 2 * sin(theta) / h
 * "difa", "tzero", and "tofmin" set to zero, "tofmax" to DBL_MAX
 */
void PDCalibration::createCalTableNew() {
  // create new calibraion table for when an old one isn't loaded
  // using the signal workspace and CalculateDIFC
  auto alg = createChildAlgorithm("CalculateDIFC");
  alg->setLoggingOffset(1);
  alg->setProperty("InputWorkspace", m_uncalibratedWS);
  alg->executeAsChildAlg();
  API::MatrixWorkspace_const_sptr difcWS = alg->getProperty("OutputWorkspace");

  // create a new workspace
  this->createCalTableHeader();

  const detid2index_map allDetectors = difcWS->getDetectorIDToWorkspaceIndexMap(false);

  const auto includedDetids = detIdsForTable();
  const bool useAllDetids = includedDetids.empty();

  // copy over the values
  size_t i = 0;
  for (auto it = allDetectors.begin(); it != allDetectors.end(); ++it) {
    const detid_t detID = it->first;
    // only add rows for detids that exist in input workspace
    if (useAllDetids || (includedDetids.count(detID) > 0)) {
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
}

/**
 * Table workspaces where the first column is the detector ID and subsequent
 * columns are termed "@x.xxxxx" where x.xxxxx are the peak positions of the
 * "PeakPositions" input property array (the nominal peak center positions)
 */
void PDCalibration::createInformationWorkspaces() {
  // table for the fitted location of the various peaks, in d-spacing units
  m_peakPositionTable = std::make_shared<DataObjects::TableWorkspace>();
  m_peakWidthTable = std::make_shared<DataObjects::TableWorkspace>();
  m_peakHeightTable = std::make_shared<DataObjects::TableWorkspace>();

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
      std::make_shared<DataObjects::SpecialWorkspace2D>(m_uncalibratedWS->getInstrument());
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
    const auto detId = static_cast<detid_t>(m_peakPositionTable->Int(rowIndex, 0));
    for (size_t peakIndex = 1; peakIndex < numPeaks + 1; ++peakIndex) {
      const double pos = m_peakPositionTable->Double(rowIndex, peakIndex);
      if (std::isnormal(pos)) {
        resolution.emplace_back(m_peakWidthTable->Double(rowIndex, peakIndex) / pos);
      }
    }
    if (resolution.empty()) {
      resolutionWksp->setValue(detId, 0., 0.); // instview doesn't like nan
    } else {
      // calculate the mean
      const double mean =
          std::accumulate(resolution.begin(), resolution.end(), 0.) / static_cast<double>(resolution.size());
      double stddev = 0.;
      std::for_each(resolution.cbegin(), resolution.cend(),
                    [&stddev, mean](const auto value) { stddev += (value - mean) * (value * mean); });
      stddev = std::sqrt(stddev / static_cast<double>(resolution.size() - 1));
      resolutionWksp->setValue(detId, mean, stddev);
    }
  }

  return resolutionWksp;
}

/// sort the calibration table according increasing values in column "detid"
API::ITableWorkspace_sptr PDCalibration::sortTableWorkspace(API::ITableWorkspace_sptr &table) {
  auto alg = createChildAlgorithm("SortTableWorkspace");
  alg->setLoggingOffset(1);
  alg->setProperty("InputWorkspace", table);
  alg->setProperty("OutputWorkspace", table);
  alg->setProperty("Columns", "detid");
  alg->executeAsChildAlg();
  table = alg->getProperty("OutputWorkspace");

  return table;
}

/**
 *  Create a pair of workspaces, one containing the nominal peak centers in TOF units,
 *  the other containing the left and right fitting ranges around each nominal
 *  peak center, also in TOF units.
 *
 *  Because each pixel has a different set of difc, difa, and tzero values,
 *  the position of the nominal peak centers (and the fitting ranges)
 *  will be different when expressed in TOF units. Thus, they have to be
 *  calculated for each pixel.
 *
 *  @param dataws :: input signal workspace
 *  @param peakWindow:: A vector of boundaries in d-spacing (if the vector has only one element this is half the width
 * of the window for all peaks) left and right of the nominal peak center to look for the peak
 */
std::pair<API::MatrixWorkspace_sptr, API::MatrixWorkspace_sptr>
PDCalibration::createTOFPeakCenterFitWindowWorkspaces(const API::MatrixWorkspace_sptr &dataws,
                                                      const std::vector<double> &peakWindow) {

  // calculate fitting ranges to the left and right of each nominal peak
  // center, in d-spacing units
  const auto windowsInDSpacing = dSpacingWindows(m_peaksInDspacing, peakWindow);

  g_log.information() << "DSPACING WINDOWS\n";
  for (std::size_t i = 0; i < m_peaksInDspacing.size(); ++i) {
    g_log.information() << "[" << i << "] " << windowsInDSpacing[2 * i] << " < " << m_peaksInDspacing[i] << " < "
                        << windowsInDSpacing[2 * i + 1] << "\n";
  }

  // create workspaces for nominal peak centers and fit ranges
  size_t numspec = dataws->getNumberHistograms();
  size_t numpeaks = m_peaksInDspacing.size();
  MatrixWorkspace_sptr peak_pos_ws = create<Workspace2D>(numspec, Points(numpeaks));
  MatrixWorkspace_sptr peak_window_ws = create<Workspace2D>(numspec, Points(numpeaks * 2));

  const auto NUM_HIST = m_stopWorkspaceIndex - m_startWorkspaceIndex + 1;
  API::Progress prog(this, 0., .2, NUM_HIST);

  g_log.information() << "TOF WINDOWS\n";
  PRAGMA_OMP(parallel for schedule(dynamic, 1) )
  for (int64_t iiws = m_startWorkspaceIndex; iiws <= static_cast<int64_t>(m_stopWorkspaceIndex); iiws++) {
    PARALLEL_START_INTERRUPT_REGION
    std::size_t iws = static_cast<std::size_t>(iiws);
    // calculatePositionWindowInTOF
    PDCalibration::FittedPeaks peaks(dataws, iws);
    // toTof is a function that converts from d-spacing to TOF for a particular
    // pixel
    auto [difc, difa, tzero] = getDSpacingToTof(peaks.detid);
    // setpositions initializes peaks.inTofPos and peaks.inTofWindows
    peaks.setPositions(m_peaksInDspacing, windowsInDSpacing, difa, difc, tzero);
    peak_pos_ws->setPoints(iws, peaks.inTofPos);
    peak_window_ws->setPoints(iws, peaks.inTofWindows);
    prog.report();

    for (std::size_t i = 0; i < peaks.inTofPos.size(); i++) {
      g_log.information() << "[" << iws << "," << i << "] " << peaks.inTofWindows[2 * i] << " < " << peaks.inTofPos[i]
                          << " < " << peaks.inTofWindows[2 * i + 1] << "\n";
    }

    PARALLEL_END_INTERRUPT_REGION
  }
  PARALLEL_CHECK_INTERRUPT_REGION

  return std::make_pair(peak_pos_ws, peak_window_ws);
}

} // namespace Mantid::Algorithms
