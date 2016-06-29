#include "MantidAlgorithms/PDCalibration.h"
#include "MantidAPI/FileProperty.h"
#include "MantidAPI/IEventList.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/TableRow.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidDataObjects/TableWorkspace.h"
#include "MantidKernel/ArrayBoundedValidator.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/BoundedValidator.h"
#include "MantidKernel/RebinParamsValidator.h"
#include <cassert>

namespace Mantid {
namespace Algorithms {

using Mantid::API::FileProperty;
using Mantid::API::MatrixWorkspace;
using Mantid::API::MatrixWorkspace_sptr;
using Mantid::API::WorkspaceProperty;
using Mantid::DataObjects::EventWorkspace;
using Mantid::Kernel::ArrayProperty;
using Mantid::Kernel::ArrayBoundedValidator;
using Mantid::Kernel::BoundedValidator;
using Mantid::Kernel::Direction;
using Mantid::Kernel::RebinParamsValidator;
using Mantid::Kernel::make_unique;
using std::vector;

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(PDCalibration)

namespace { // anonymous
const auto isNonZero = [](const double value) { return value != 0.; };
}

//----------------------------------------------------------------------------------------------
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

    const MantidVec &X = spectrum.readX();
    const MantidVec &Y = spectrum.readY();
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
  return "Calibrate the detector pixels and write a calibration file";
}

//----------------------------------------------------------------------------------------------
/** Initialize the algorithm's properties.
 */
void PDCalibration::init() {
  declareProperty(make_unique<WorkspaceProperty<MatrixWorkspace>>(
                      "UncalibratedWorkspace", "", Direction::Output),
                  "");

  const std::vector<std::string> exts{"_event.nxs", ".nxs.h5", ".nxs"};
  declareProperty(make_unique<FileProperty>(
                      "SignalFile", "", FileProperty::FileAction::Load, exts),
                  "Calibration measurement");
  declareProperty(make_unique<FileProperty>("BackgroundFile", "",
                                            FileProperty::OptionalLoad, exts),
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

  declareProperty(make_unique<ArrayProperty<double>>(
                      "TofBinning", boost::make_shared<RebinParamsValidator>()),
                  "Min, Step, and Max of time-of-flight bins. "
                  "Logarithmic binning is used if Step is negative.");

  const std::vector<std::string> exts2{".h5", ".cal"};
  // TODO should be optional
  declareProperty(make_unique<FileProperty>(
                      "PreviousCalibration", "",
                      FileProperty::Load, // FileProperty::OptionalLoad,
                      exts2),
                  "Calibration measurement");

  auto mustBePosArr =
      boost::make_shared<Kernel::ArrayBoundedValidator<double>>();
  mustBePosArr->setLower(0.0);
  declareProperty(
      make_unique<ArrayProperty<double>>("PeakPositions", mustBePosArr),
      "Comma delimited d-space positions of reference peaks.");

  //  declareProperty(new WorkspaceProperty<API::ITableWorkspace>(
  //                    "RefTOFTable", "", Direction::Output),
  //                  "");
  declareProperty(make_unique<WorkspaceProperty<API::ITableWorkspace>>(
                      "OutputCalibrationTable", "", Direction::Output),
                  "");
}

//----------------------------------------------------------------------------------------------
/** Execute the algorithm.
 */
void PDCalibration::exec() {
  vector<double> tofBinningParams = getProperty("TofBinning");
  m_tofMin = tofBinningParams.front();
  m_tofMax = tofBinningParams.back();

  m_peaksInDspacing = getProperty("PeakPositions");
  //Sort peak positions, requried for correct peak window calculations
  std::sort(m_peaksInDspacing.begin(), m_peaksInDspacing.end());

  const double peakWindowMaxInDSpacing = 0.1; // TODO configurable
  const auto windowsInDSpacing =
      dSpacingWindows(m_peaksInDspacing, peakWindowMaxInDSpacing);

  for (std::size_t i = 0; i < m_peaksInDspacing.size(); ++i) {
    std::cout << "[" << i << "] " << windowsInDSpacing[2 * i] << " < "
              << m_peaksInDspacing[i] << " < " << windowsInDSpacing[2 * i + 1]
              << std::endl;
  }

  m_uncalibratedWS = loadAndBin();
  setProperty("UncalibratedWorkspace", m_uncalibratedWS);

  auto uncalibratedEWS =
      boost::dynamic_pointer_cast<EventWorkspace>(m_uncalibratedWS);
  bool isEvent = bool(uncalibratedEWS);

  loadOldCalibration();

  const std::size_t NUMHIST = m_uncalibratedWS->getNumberHistograms();

  // cppcheck-suppress syntaxError
  PRAGMA_OMP(parallel for schedule(dynamic, 1) )
  for (std::size_t wkspIndex = 0; wkspIndex < NUMHIST; ++wkspIndex) {
    PARALLEL_START_INTERUPT_REGION
    if (isEvent && uncalibratedEWS->getSpectrum(wkspIndex).empty()) {
      // std::cout << "Empty event list at wkspIndex = " << wkspIndex <<
      // std::endl;
      continue;
    }

    PDCalibration::FittedPeaks peaks(m_uncalibratedWS, wkspIndex);
    auto toTof = getDSpacingToTof(peaks.detid);
    peaks.setPositions(m_peaksInDspacing, windowsInDSpacing, toTof);

    if (peaks.inTofPos.empty())
      continue;

    //       std::cout << "****************************** detid = " <<
    //       peaks.detid
    //                 << " wkspIndex = " << wkspIndex
    //                 << " numPeaks = " << peaks.inTofPos.size() << std::endl;
    //       std::cout << "--> TOFRANGE " << peaks.tofMin << " -> " <<
    //       peaks.tofMax << std::endl;

    auto alg = createChildAlgorithm("FindPeaks");
    alg->setProperty("InputWorkspace", m_uncalibratedWS);
    alg->setProperty("WorkspaceIndex", static_cast<int>(wkspIndex));
    alg->setProperty("PeakPositions", peaks.inTofPos);
    alg->setProperty("FitWindows", peaks.inTofWindows);
    alg->setProperty("FWHM", 7);                           // TODO default
    alg->setProperty("Tolerance", 4);                      // TODO default
    alg->setProperty("PeakFunction", "Gaussian");          // TODO configurable?
    alg->setProperty("BackgroundType", "Linear");          // TODO configurable?
    alg->setProperty("HighBackground", true);              // TODO configurable?
    alg->setProperty("MinGuessedPeakWidth", 4);            // TODO configurable?
    alg->setProperty("MaxGuessedPeakWidth", 4);            // TODO configurable?
    alg->setProperty("MinimumPeakHeight", 2.);             // TODO configurable?
    alg->setProperty("StartFromObservedPeakCentre", true); // TODO configurable?
    alg->executeAsChildAlg();
    API::ITableWorkspace_sptr fittedTable = alg->getProperty("PeaksList");
    //       std::cout << "fitted rowcount " << fittedTable->rowCount() <<
    //       std::endl;

    //       std::cout << "old: ";
    //       for (auto tof: peaks.inTofPos) {
    //         std::cout << tof << " ";
    //       }
    //       std::cout << std::endl;

    //       std::cout << "------------------------------" << std::endl;
    // double difc_cumm = 0.;
    // size_t difc_count = 0;
    std::vector<double> d_vec;
    std::vector<double> tof_vec;
    for (size_t i = 0; i < fittedTable->rowCount(); ++i) {
      // Get peak value
      double centre = fittedTable->getRef<double>("centre", i);
      // double width = fittedTable->getRef<double>("width", i);
      // double height = fittedTable->getRef<double>("height", i);
      double chi2 = fittedTable->getRef<double>("chi2", i);

      //         std::cout << "d=" << peaks.inDPos[i]<< " centre old=" <<
      //         peaks.inTofPos[i];
      if (chi2 > 1.e10) {
        //           std::cout << " failed to fit - chisq" << chi2 << std::endl;
      } else if (peaks.inTofWindows[2 * i] >= centre ||
                 peaks.inTofWindows[2 * i + 1] <= centre) {
        //           std::cout << " failed to fit - centre " <<
        //           peaks.inTofWindows[2*i] << " < " << centre
        //                     << " < " << peaks.inTofWindows[2*i+1] <<
        //                     std::endl;
      } else {
        // double difc = centre / peaks.inDPos[i];
        // difc_cumm += difc;
        // difc_count += 1;

        //           std::cout << " new=" << centre << " width=" << width << "
        //           height=" << height
        //                     << " chi2=" << chi2 << " difc=" << difc <<
        //                     std::endl;
        d_vec.push_back(peaks.inDPos[i]);
        tof_vec.push_back(centre);
      }
    }
    if (d_vec.size() > 0) {
      double difcc = 0, t0 = 0, difa = 0;
      fitDIFCtZeroDIFA(d_vec, tof_vec, difcc, t0, difa);
      setCalibrationValues(peaks.detid, difcc, difa, t0);

      //           std::cout << "avg difc = " <<
      //           (difc_cumm/static_cast<double>(difc_count)) << std::endl;
    } else {
      //           std::cout << "failed to fit - zero peaks found in " <<
      //           peaks.detid << std::endl;
    }

    // break;
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
  difc = 0;
  t0 = 0;
  difa = 0;

  double sum = 0;
  double sumX = 0;
  double sumY = 0;
  double sumX2 = 0;
  double sumXY = 0;
  double sumX2Y = 0;
  double sumX3 = 0;
  double sumX4 = 0;

  for (size_t i = 0; i < d.size(); ++i) {
    sum++;
    sumX += d[i];
    sumY += tof[i];
    sumX2 += d[i] * d[i];
    sumXY += d[i] * tof[i];
    sumX2Y += d[i] * d[i] * tof[i];
    sumX3 += d[i] * d[i] * d[i];
    sumX4 += d[i] * d[i] * d[i] * d[i];
  }

  // DIFC only
  double difc0 = sumXY / sumX2;
  // std::cout << "difc0 = " << difc0 << '\n';

  // DIFC and t0
  double determinant = sum * sumX2 - sumX * sumX;
  double difc1 = (sum * sumXY - sumX * sumY) / determinant;
  double tZero1 = sumY / sum - difc1 * sumX / sum;
  // std::cout << "difc1 = " << difc1 << '\n';
  // std::cout << "tZero1 = " << tZero1 << '\n';

  // DIFC, t0 and DIFA
  determinant = sum * sumX2 * sumX4 + sumX * sumX3 * sumX2 +
                sumX2 * sumX * sumX3 - sumX2 * sumX2 * sumX2 -
                sumX * sumX * sumX4 - sum * sumX3 * sumX3;
  double tZero2 =
      (sumY * sumX2 * sumX4 + sumX * sumX3 * sumX2Y + sumX2 * sumXY * sumX3 -
       sumX2 * sumX2 * sumX2Y - sumX * sumXY * sumX4 - sumY * sumX3 * sumX3) /
      determinant;
  double difc2 =
      (sum * sumXY * sumX4 + sumY * sumX3 * sumX2 + sumX2 * sumX * sumX2Y -
       sumX2 * sumXY * sumX2 - sumY * sumX * sumX4 - sum * sumX3 * sumX2Y) /
      determinant;
  double difa2 =
      (sum * sumX2 * sumX2Y + sumX * sumXY * sumX2 + sumY * sumX * sumX3 -
       sumY * sumX2 * sumX2 - sumX * sumX * sumX2Y - sum * sumXY * sumX3) /
      determinant;
  // std::cout << "difc2 = " << difc2 << '\n';
  // std::cout << "tZero2 = " << tZero2 << '\n';
  // std::cout << "difa2 = " << difa2 << '\n';

  // calculated reduced chi squared for each fit
  double chisq0 = 0;
  double chisq1 = 0;
  double chisq2 = 0;
  for (size_t i = 0; i < d.size(); ++i) {
    // difc chi-squared
    double temp = difc0 * d[i] - tof[i];
    chisq0 += (temp * temp);

    // difc and t0 chi-squared
    temp = tZero1 + difc1 * d[i] - tof[i];
    chisq1 += (temp * temp);

    // difc, t0 and difa chi-squared
    temp = tZero2 + difc2 * d[i] + difa2 * d[i] * d[i] - tof[i];
    chisq2 += (temp * temp);
  }

  chisq0 = chisq0 / (sum - 1);
  chisq1 = chisq1 / (sum - 2);
  chisq2 = chisq2 / (sum - 3);
  // std::cout << "chisq0 = " << chisq0 << '\n';
  // std::cout << "chisq1 = " << chisq1 << '\n';
  // std::cout << "chisq2 = " << chisq2 << '\n';

  // choose best one according to chi-squared
  if ((chisq0 < chisq1) && (chisq0 < chisq2)) {
    difc = difc0;
  } else if ((chisq1 < chisq0) && (chisq1 < chisq2)) {
    difc = difc1;
    t0 = tZero1;
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

  // TODO calculate values
  m_calibrationTable->cell<double>(rowNum, 4 + hasDasIdsOffset) = 0.; // tofmin
  m_calibrationTable->cell<double>(rowNum, 5 + hasDasIdsOffset) = 0.; // tofmax
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
  m_uncalibratedWS = getProperty("UncalibratedWorkspace");

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
    newRow << 0.; // tofmin   TODO
    newRow << 0.; // tofmax   TODO
  }
}

} // namespace Algorithms
} // namespace Mantid
