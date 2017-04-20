#include "MantidAlgorithms/PDCalibration.h"
#include "MantidAPI/FileProperty.h"
#include "MantidAPI/IEventList.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/Run.h"
#include "MantidAPI/TableRow.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidDataObjects/MaskWorkspace.h"
#include "MantidDataObjects/TableWorkspace.h"
#include "MantidGeometry/Instrument.h"
#include "MantidKernel/ArrayBoundedValidator.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/BoundedValidator.h"
#include "MantidKernel/CompositeValidator.h"
#include "MantidKernel/Diffraction.h"
#include "MantidKernel/ListValidator.h"
#include "MantidKernel/MandatoryValidator.h"
#include "MantidKernel/RebinParamsValidator.h"
#include "MantidKernel/make_unique.h"
#include <cassert>
#include <limits>

namespace Mantid {
namespace Algorithms {

using Mantid::API::FileProperty;
using Mantid::API::MatrixWorkspace;
using Mantid::API::MatrixWorkspace_sptr;
using Mantid::API::WorkspaceProperty;
using Mantid::DataObjects::EventWorkspace;
using Mantid::DataObjects::MaskWorkspace_sptr;
using Mantid::Kernel::ArrayProperty;
using Mantid::Kernel::ArrayBoundedValidator;
using Mantid::Kernel::BoundedValidator;
using Mantid::Kernel::CompositeValidator;
using Mantid::Kernel::Direction;
using Mantid::Kernel::MandatoryValidator;
using Mantid::Kernel::RebinParamsValidator;
using Mantid::Kernel::StringListValidator;
using Mantid::Geometry::Instrument_const_sptr;
using std::vector;

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(PDCalibration)

namespace { // anonymous
const auto isNonZero = [](const double value) { return value != 0.; };
const double CHISQ_BAD = 1.e9; // hopefully much worse than possible
}

/// private inner class
class PDCalibration::FittedPeaks {
public:
  FittedPeaks(API::MatrixWorkspace_const_sptr wksp,
              const std::size_t wkspIndex) {
    this->wkspIndex = wkspIndex;

    // convert workspace index into detector id
    const auto &spectrum = wksp->getSpectrum(wkspIndex);
    const auto detIds = spectrum.getDetectorIDs();
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

    // determin tof max supported by the workspace
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

    const std::size_t numOrig = peaksInD.size();
    for (std::size_t i = 0; i < numOrig; ++i) {
      const double centre = toTof(peaksInD[i]);
      if (centre < tofMax && centre > tofMin) {
        inDPos.push_back(peaksInD[i]);
        inTofPos.push_back(peaksInD[i]);
        inTofWindows.push_back(peaksInDWindows[2 * i]);
        inTofWindows.push_back(peaksInDWindows[2 * i + 1]);
      }
    }
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
                      "SignalWorkspace", "", Direction::InOut),
                  "Input signal workspace.\nIf the workspace does not exist it "
                  "will read it from the SignalFile into this workspace.");

  const std::vector<std::string> exts{"_event.nxs", ".nxs.h5", ".nxs"};
  declareProperty(
      Kernel::make_unique<FileProperty>(
          "SignalFile", "", FileProperty::FileAction::OptionalLoad, exts),
      "Calibration measurement");
  declareProperty(Kernel::make_unique<FileProperty>(
                      "BackgroundFile", "", FileProperty::OptionalLoad, exts),
                  "Calibration background");

  auto mustBePositive = boost::make_shared<BoundedValidator<double>>();
  mustBePositive->setLower(0.0);
  declareProperty("MaxChunkSize", EMPTY_DBL(), mustBePositive,
                  "Get chunking strategy for chunks with this number of "
                  "Gbytes. File will not be loaded if this option is set.");

  auto range = boost::make_shared<BoundedValidator<double>>();
  range->setBounds(0., 100.);
  declareProperty("FilterBadPulses", 95., range,
                  "The percentage of the average to use as the lower bound");

  declareProperty(Kernel::make_unique<ArrayProperty<double>>(
                      "TofBinning", boost::make_shared<RebinParamsValidator>()),
                  "Min, Step, and Max of time-of-flight bins. "
                  "Logarithmic binning is used if Step is negative.");

  const std::vector<std::string> exts2{".h5", ".cal"};

  declareProperty(Kernel::make_unique<FileProperty>("PreviousCalibration", "",
                                                    FileProperty::OptionalLoad,
                                                    exts2),
                  "Calibration measurement");

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

  declareProperty(
      "PeakWindow", 0.1, mustBePositive,
      "The maximum window (in d space) around peak to look for peak.");
  std::vector<std::string> modes{"DIFC", "DIFC+TZERO", "DIFC+TZERO+DIFA"};

  // copy FindPeaks properties
  auto min = boost::make_shared<BoundedValidator<int>>();
  min->setLower(1);
  declareProperty(
      "FWHM", 7, min,
      "Estimated number of points covered by the fwhm of a peak (default 7)");
  declareProperty("Tolerance", 4, min, "A measure of the strictness desired in "
                                       "meeting the condition on peak "
                                       "candidates,\n Mariscotti recommends 2 "
                                       "(default 4)");
  std::vector<std::string> peaktypes{"BackToBackExponential", "Gaussian",
                                     "Lorentzian"};
  declareProperty("PeakFunction", "Gaussian",
                  boost::make_shared<StringListValidator>(peaktypes));
  std::vector<std::string> bkgdtypes{"Flat", "Linear", "Quadratic"};
  declareProperty("BackgroundType", "Linear",
                  boost::make_shared<StringListValidator>(bkgdtypes),
                  "Type of Background.");
  declareProperty("HighBackground", true,
                  "Relatively weak peak in high background");
  declareProperty(
      "MinGuessedPeakWidth", 4, min,
      "Minimum guessed peak width for fit. It is in unit of number of pixels.");
  declareProperty(
      "MaxGuessedPeakWidth", 4, min,
      "Maximum guessed peak width for fit. It is in unit of number of pixels.");
  declareProperty("MinimumPeakHeight", 2., "Minimum allowed peak height. ");
  declareProperty(
      "MaxChiSq", 100.,
      "Maximum chisq value for individual peak fit allowed. (Default: 100)");
  declareProperty("StartFromObservedPeakCentre", true,
                  "Use observed value as the starting value of peak centre. ");

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

  // make group for Input properties
  std::string inputGroup("Input Options");
  setPropertyGroup("SignalWorkspace", inputGroup);
  setPropertyGroup("SignalFile", inputGroup);
  setPropertyGroup("BackgroundFile", inputGroup);
  setPropertyGroup("MaxChunkSize", inputGroup);
  setPropertyGroup("FilterBadPulses", inputGroup);
  setPropertyGroup("TofBinning", inputGroup);
  setPropertyGroup("PreviousCalibration", inputGroup);

  // make group for FindPeak properties
  std::string findPeaksGroup("Peak finding properties");
  setPropertyGroup("PeakPositions", findPeaksGroup);
  setPropertyGroup("PeakWindow", findPeaksGroup);
  setPropertyGroup("FWHM", findPeaksGroup);
  setPropertyGroup("Tolerance", findPeaksGroup);
  setPropertyGroup("PeakFunction", findPeaksGroup);
  setPropertyGroup("BackgroundType", findPeaksGroup);
  setPropertyGroup("HighBackground", findPeaksGroup);
  setPropertyGroup("MinGuessedPeakWidth", findPeaksGroup);
  setPropertyGroup("MaxGuessedPeakWidth", findPeaksGroup);
  setPropertyGroup("MinimumPeakHeight", findPeaksGroup);
  setPropertyGroup("MaxChiSq", findPeaksGroup);
  setPropertyGroup("StartFromObservedPeakCentre", findPeaksGroup);
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
  const auto windowsInDSpacing =
      dSpacingWindows(m_peaksInDspacing, peakWindowMaxInDSpacing);

  for (std::size_t i = 0; i < m_peaksInDspacing.size(); ++i) {
    g_log.information() << "[" << i << "] " << windowsInDSpacing[2 * i] << " < "
                        << m_peaksInDspacing[i] << " < "
                        << windowsInDSpacing[2 * i + 1] << std::endl;
  }

  double minPeakHeight = getProperty("MinimumPeakHeight");
  double maxChiSquared = getProperty("MaxChiSq");

  calParams = getPropertyValue("CalibrationParameters");

  m_uncalibratedWS = loadAndBin();
  setProperty("SignalWorkspace", m_uncalibratedWS);

  auto uncalibratedEWS =
      boost::dynamic_pointer_cast<EventWorkspace>(m_uncalibratedWS);
  bool isEvent = bool(uncalibratedEWS);

  // Load Previous Calibration or create calibration table from signal file
  if (!static_cast<std::string>(getProperty("PreviousCalibration")).empty()) {
    loadOldCalibration();
  } else {
    createNewCalTable();
  }

  std::string maskWSName = getPropertyValue("OutputCalibrationTable");
  maskWSName += "_mask";
  declareProperty(Kernel::make_unique<WorkspaceProperty<>>(
                      "MaskWorkspace", maskWSName, Direction::Output),
                  "An output workspace containing the mask");

  MaskWorkspace_sptr maskWS = boost::make_shared<DataObjects::MaskWorkspace>(
      m_uncalibratedWS->getInstrument());
  setProperty("MaskWorkspace", maskWS);

  int NUMHIST = static_cast<int>(m_uncalibratedWS->getNumberHistograms());
  API::Progress prog(this, 0, 1.0, NUMHIST);

  // cppcheck-suppress syntaxError
  PRAGMA_OMP(parallel for schedule(dynamic, 1) )
  for (int wkspIndex = 0; wkspIndex < NUMHIST; ++wkspIndex) {
    PARALLEL_START_INTERUPT_REGION
    if (isEvent && uncalibratedEWS->getSpectrum(wkspIndex).empty()) {
      prog.report();
      continue;
    }

    PDCalibration::FittedPeaks peaks(m_uncalibratedWS, wkspIndex);
    auto toTof = getDSpacingToTof(peaks.detid);
    peaks.setPositions(m_peaksInDspacing, windowsInDSpacing, toTof);

    if (peaks.inTofPos.empty())
      continue;

    auto alg = createChildAlgorithm("FindPeaks");
    alg->setLoggingOffset(2);
    alg->setProperty("InputWorkspace", m_uncalibratedWS);
    alg->setProperty("WorkspaceIndex", static_cast<int>(wkspIndex));
    alg->setProperty("PeakPositions", peaks.inTofPos);
    alg->setProperty("FitWindows", peaks.inTofWindows);
    alg->setProperty<int>("FWHM", getProperty("FWHM"));
    alg->setProperty<int>("Tolerance", getProperty("Tolerance"));
    alg->setProperty<std::string>("PeakFunction", getProperty("PeakFunction"));
    alg->setProperty<std::string>("BackgroundType",
                                  getProperty("BackgroundType"));
    alg->setProperty<bool>("HighBackground", getProperty("HighBackground"));
    alg->setProperty<int>("MinGuessedPeakWidth",
                          getProperty("MinGuessedPeakWidth"));
    alg->setProperty<int>("MaxGuessedPeakWidth",
                          getProperty("MaxGuessedPeakWidth"));
    alg->setProperty<double>("MinimumPeakHeight",
                             getProperty("MinimumPeakHeight"));
    alg->setProperty<bool>("StartFromObservedPeakCentre",
                           getProperty("StartFromObservedPeakCentre"));
    alg->executeAsChildAlg();
    API::ITableWorkspace_sptr fittedTable = alg->getProperty("PeaksList");

    std::vector<double> d_vec;
    std::vector<double> tof_vec;
    for (size_t i = 0; i < fittedTable->rowCount(); ++i) {
      // Get peak value
      double centre = fittedTable->getRef<double>("centre", i);
      double height = fittedTable->getRef<double>("height", i);
      double chi2 = fittedTable->getRef<double>("chi2", i);

      // check chi-square
      if (chi2 > maxChiSquared || chi2 < 0) {
        continue;
      }

      // rule out of peak with wrong position
      if (peaks.inTofWindows[2 * i] >= centre ||
          peaks.inTofWindows[2 * i + 1] <= centre) {
        continue;
      }

      // check height
      if (height < minPeakHeight) {
        continue;
      }

      // background value
      double back_intercept =
          fittedTable->getRef<double>("backgroundintercept", i);
      double back_slope = fittedTable->getRef<double>("backgroundslope", i);
      double back_quad = fittedTable->getRef<double>("A2", i);
      double background =
          back_intercept + back_slope * centre + back_quad * centre * centre;

      // ban peaks that are not outside of error bars for the background
      if (height < 0.5 * std::sqrt(height + background))
        continue;

      d_vec.push_back(peaks.inDPos[i]);
      tof_vec.push_back(centre);
    }

    if (d_vec.empty()) {
      maskWS->setMaskedIndex(wkspIndex, true);
      continue;
    } else {
      double difc = 0., t0 = 0., difa = 0.;
      fitDIFCtZeroDIFA(d_vec, tof_vec, difc, t0, difa);
      setCalibrationValues(peaks.detid, difc, difa, t0);
    }
    prog.report();

    PARALLEL_END_INTERUPT_REGION
  }
  PARALLEL_CHECK_INTERUPT_REGION
}

namespace {
struct d_to_tof {
  d_to_tof(const double difc, const double difa, const double tzero) {
    this->difc = difc;
    this->difa = difa;
    this->tzero = tzero;
  }

  double operator()(const double dspacing) const {
    return difc * dspacing + difa * dspacing * dspacing + tzero;
  }

  double difc;
  double difa;
  double tzero;
};
}

void PDCalibration::fitDIFCtZeroDIFA(const std::vector<double> &d,
                                     const std::vector<double> &tof,
                                     double &difc, double &t0, double &difa) {
  double num_peaks = static_cast<double>(d.size());
  double sumX = 0.;
  double sumY = 0.;
  double sumX2 = 0.;
  double sumXY = 0.;
  double sumX2Y = 0.;
  double sumX3 = 0.;
  double sumX4 = 0.;

  for (size_t i = 0; i < d.size(); ++i) {
    sumX2 += d[i] * d[i];
    sumXY += d[i] * tof[i];
  }

  // DIFC only
  double difc0 = sumXY / sumX2;
  // Get out early if only DIFC is needed.
  if (calParams == "DIFC" || num_peaks < 3) {
    difc = difc0;
    return;
  }

  // DIFC and t0
  for (size_t i = 0; i < d.size(); ++i) {
    sumX += d[i];
    sumY += tof[i];
  }

  double difc1 = 0;
  double tZero1 = 0;
  double determinant = num_peaks * sumX2 - sumX * sumX;
  if (determinant != 0) {
    difc1 = (num_peaks * sumXY - sumX * sumY) / determinant;
    tZero1 = sumY / num_peaks - difc1 * sumX / num_peaks;
  }

  // calculated chi squared for each fit
  double chisq0 = 0;
  double chisq1 = 0;
  for (size_t i = 0; i < d.size(); ++i) {
    // difc chi-squared
    double temp = difc0 * d[i] - tof[i];
    chisq0 += (temp * temp);

    // difc and t0 chi-squared
    temp = tZero1 + difc1 * d[i] - tof[i];
    chisq1 += (temp * temp);
  }

  // Get reduced chi-squared
  chisq0 = chisq0 / (num_peaks - 1);
  chisq1 = chisq1 / (num_peaks - 2);

  // check that the value is reasonable, only need to check minimum
  // side since difa is not in play - shift to a higher minimum
  // means something went wrong
  if (m_tofMin < Kernel::Diffraction::calcTofMin(difc1, 0., tZero1, m_tofMin) ||
      difc1 <= 0. || tZero1 < m_tzeroMin || tZero1 > m_tzeroMax) {
    difc1 = 0;
    tZero1 = 0;
    chisq1 = CHISQ_BAD;
  }

  // Select difc, t0 depending on CalibrationParameters chosen and
  // number of peaks fitted.
  if (calParams == "DIFC+TZERO" || num_peaks == 3) {
    // choose best one according to chi-squared
    if (chisq0 < chisq1) {
      difc = difc0;
    } else {
      difc = difc1;
      t0 = tZero1;
    }
    return;
  }

  // DIFC, t0 and DIFA
  for (size_t i = 0; i < d.size(); ++i) {
    sumX2Y += d[i] * d[i] * tof[i];
    sumX3 += d[i] * d[i] * d[i];
    sumX4 += d[i] * d[i] * d[i] * d[i];
  }

  double tZero2 = 0;
  double difc2 = 0;
  double difa2 = 0;
  determinant = num_peaks * sumX2 * sumX4 + sumX * sumX3 * sumX2 +
                sumX2 * sumX * sumX3 - sumX2 * sumX2 * sumX2 -
                sumX * sumX * sumX4 - num_peaks * sumX3 * sumX3;
  if (determinant != 0) {
    tZero2 =
        (sumY * sumX2 * sumX4 + sumX * sumX3 * sumX2Y + sumX2 * sumXY * sumX3 -
         sumX2 * sumX2 * sumX2Y - sumX * sumXY * sumX4 - sumY * sumX3 * sumX3) /
        determinant;
    difc2 = (num_peaks * sumXY * sumX4 + sumY * sumX3 * sumX2 +
             sumX2 * sumX * sumX2Y - sumX2 * sumXY * sumX2 -
             sumY * sumX * sumX4 - num_peaks * sumX3 * sumX2Y) /
            determinant;
    difa2 = (num_peaks * sumX2 * sumX2Y + sumX * sumXY * sumX2 +
             sumY * sumX * sumX3 - sumY * sumX2 * sumX2 - sumX * sumX * sumX2Y -
             num_peaks * sumXY * sumX3) /
            determinant;
  }

  // calculated reduced chi squared for each fit
  double chisq2 = 0;
  for (size_t i = 0; i < d.size(); ++i) {
    double temp = tZero2 + difc2 * d[i] + difa2 * d[i] * d[i] - tof[i];
    chisq2 += (temp * temp);
  }
  chisq2 = chisq2 / (num_peaks - 3);

  // check that the value is reasonable, only need to check minimum
  // side since difa is not in play - shift to a higher minimum
  // or a lower maximum means something went wrong
  if (m_tofMin <
          Kernel::Diffraction::calcTofMin(difc2, difa2, tZero2, m_tofMin) ||
      m_tofMax <
          Kernel::Diffraction::calcTofMax(difc2, difa2, tZero2, m_tofMax) ||
      difc2 <= 0. || tZero2 < m_tzeroMin || tZero2 > m_tzeroMax ||
      difa2 < m_difaMin || difa2 > m_difaMax) {
    tZero2 = 0;
    difc2 = 0;
    difa2 = 0;
    chisq2 = CHISQ_BAD;
  }

  // Select difc, t0 and difa depending on CalibrationParameters chosen and
  // number of peaks fitted.
  if ((chisq0 < chisq1) && (chisq0 < chisq2)) {
    difc = difc0;
    t0 = 0.;
    difa = 0.;
  } else if ((chisq1 < chisq0) && (chisq1 < chisq2)) {
    difc = difc1;
    t0 = tZero1;
    difa = 0.;
  } else {
    difc = difc2;
    t0 = tZero2;
    difa = difa2;
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

  return d_to_tof(difc, difa, tzero);
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
  m_uncalibratedWS = getProperty("SignalWorkspace");

  if (bool(m_uncalibratedWS)) {
    return rebin(m_uncalibratedWS);
  }

  const std::string signalFile = getProperty("Signalfile");
  g_log.information() << "Loading signal file \"" << signalFile << "\"\n";
  auto signalWS = load(getProperty("SignalFile"));

  const std::string backFile = getProperty("Backgroundfile");
  if (!backFile.empty()) {
    g_log.information() << "Loading background file \"" << backFile << "\"\n";
    auto backWS = load(backFile);

    double signalPcharge = signalWS->run().getProtonCharge();
    double backPcharge = backWS->run().getProtonCharge();
    backWS *= (signalPcharge / backPcharge); // scale background by charge

    g_log.information("Subtracting background");
    auto algMinus = createChildAlgorithm("Minus");
    algMinus->setProperty("LHSWorkspace", signalWS);
    algMinus->setProperty("RHSWorkspace", backWS);
    algMinus->setProperty("OutputWorkspace", signalWS);
    algMinus->setProperty("ClearRHSWorkspace", true); // only works for events
    algMinus->executeAsChildAlg();
    signalWS = algMinus->getProperty("OutputWorkspace");

    g_log.information("Compressing data");
    auto algCompress = createChildAlgorithm("CompressEvents");
    algCompress->setProperty("InputWorkspace", signalWS);
    algCompress->setProperty("OutputWorkspace", signalWS);
    algCompress->executeAsChildAlg();
    DataObjects::EventWorkspace_sptr compressResult =
        algCompress->getProperty("OutputWorkspace");
    signalWS = boost::dynamic_pointer_cast<MatrixWorkspace>(compressResult);
  }

  return rebin(signalWS);
}

API::MatrixWorkspace_sptr PDCalibration::rebin(API::MatrixWorkspace_sptr wksp) {
  g_log.information("Binning data in time-of-flight");
  auto rebin = createChildAlgorithm("Rebin");
  rebin->setProperty("InputWorkspace", wksp);
  rebin->setProperty("OutputWorkspace", wksp);
  rebin->setProperty("Params", getPropertyValue("TofBinning"));
  rebin->setProperty("PreserveEvents", true);
  rebin->executeAsChildAlg();
  wksp = rebin->getProperty("OutputWorkspace");

  return wksp;
}

namespace {

bool hasDasIDs(API::ITableWorkspace_const_sptr table) {
  const auto columnNames = table->getColumnNames();
  return (std::find(columnNames.begin(), columnNames.end(),
                    std::string("dasid")) != columnNames.end());
}
}

void PDCalibration::loadOldCalibration() {
  // load the old one
  std::string filename = getProperty("PreviousCalibration");
  auto alg = createChildAlgorithm("LoadDiffCal");
  alg->setProperty("Filename", filename);
  alg->setProperty("WorkspaceName", "NOMold"); // TODO
  alg->setProperty("MakeGroupingWorkspace", false);
  alg->setProperty("MakeMaskWorkspace", false);
  alg->setProperty("TofMin", m_tofMin);
  alg->setProperty("TofMax", m_tofMax);
  alg->executeAsChildAlg();
  API::ITableWorkspace_sptr calibrationTableOld =
      alg->getProperty("OutputCalWorkspace");

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
  setProperty("OutputCalibrationTable", m_calibrationTable);

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

void PDCalibration::createNewCalTable() {
  // create new calibraion table for when an old one isn't loaded
  // using the signal workspace and CalculateDIFC
  auto alg = createChildAlgorithm("CalculateDIFC");
  alg->setProperty("InputWorkspace", m_uncalibratedWS);
  alg->executeAsChildAlg();
  API::MatrixWorkspace_sptr difcWS = alg->getProperty("OutputWorkspace");

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

} // namespace Algorithms
} // namespace Mantid
