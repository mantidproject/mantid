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
using std::vector;

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(PDCalibration)

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
  declareProperty( // TODO fix this mess
      new WorkspaceProperty<MatrixWorkspace>("UncalibratedWorkspace", "", Direction::Output),
      "");


  this->declareProperty(
      new FileProperty("SignalFile", "", FileProperty::Load,
                       {"_event.nxs", ".nxs.h5", ".nxs"}),
      "Calibration measurement");
  this->declareProperty(
      new FileProperty("BackgroundFile", "", FileProperty::OptionalLoad,
                       {"_event.nxs", ".nxs.h5", ".nxs"}),
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

  declareProperty(
      new ArrayProperty<double>("TofBinning",
                                boost::make_shared<RebinParamsValidator>()),
        "Min, Step, and Max of time-of-flight bins. " \
        "Logarithmic binning is used if Step is negative.");

  // TODO should be optional
  declareProperty(
      new FileProperty("PreviousCalibration", "", FileProperty::Load, // FileProperty::OptionalLoad,
                       {".h5", ".cal"}),
      "Calibration measurement");

  auto mustBePosArr =
      boost::make_shared<Kernel::ArrayBoundedValidator<double>>();
  mustBePosArr->setLower(0.0);
  declareProperty(
        new ArrayProperty<double>("PeakPositions", mustBePosArr),
        "Comma delimited d-space positions of reference peaks.");

//  declareProperty(new WorkspaceProperty<API::ITableWorkspace>(
//                    "RefTOFTable", "", Direction::Output),
//                  "");
  declareProperty(new WorkspaceProperty<API::ITableWorkspace>(
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

  loadAndBin();
  auto uncalibratedEWS = boost::dynamic_pointer_cast<EventWorkspace>(m_uncalibratedWS);
  bool isEvent = bool(uncalibratedEWS);

  loadOldCalibration();

  const std::size_t NUMHIST = m_uncalibratedWS->getNumberHistograms();
  for (std::size_t wkspIndex = 0; wkspIndex < NUMHIST; ++wkspIndex) {
       const auto spectrum = m_uncalibratedWS->getSpectrum(wkspIndex);
       const auto detIds = spectrum->getDetectorIDs();
       if (detIds.size() != 1) {
         throw std::runtime_error("Summed pixels is not currently supported");
       }
       const detid_t detid = *(detIds.begin());

       if (isEvent && uncalibratedEWS->getEventList(wkspIndex).empty()) {
         // TODO copy old results
         //std::cout << "Empty event list at wkspIndex = " << wkspIndex << std::endl;
         continue;
       }


       auto tofs = dSpacingToTof(m_peaksInDspacing, detid);

       std::cout << "****************************** detid = " << detid
                 << " wkspIndex = " << wkspIndex << std::endl;

       auto alg = createChildAlgorithm("FindPeaks");
       alg->setProperty("InputWorkspace", m_uncalibratedWS);
       alg->setProperty("WorkspaceIndex", static_cast<int>(wkspIndex));
       alg->setProperty("PeakPositions", tofs);
       alg->setProperty("FWHM", 7); // TODO default
       alg->setProperty("Tolerance", 4); // TODO default
       alg->setProperty("PeakFunction", "Gaussian"); // TODO configurable?
       alg->setProperty("BackgroundType","Linear"); // TODO configurable?
       alg->setProperty("HighBackground", true); // TODO configurable?
       alg->setProperty("MinGuessedPeakWidth", 4); // TODO configurable?
       alg->setProperty("MaxGuessedPeakWidth", 4); // TODO configurable?
       alg->setProperty("MinimumPeakHeight", 2.); // TODO configurable?
       alg->setProperty("StartFromObservedPeakCentre", true); // TODO configurable?
       alg->executeAsChildAlg();
       API::ITableWorkspace_sptr fittedTable = alg->getProperty("PeaksList");
       std::cout << "fitted rowcount " << fittedTable->rowCount() << std::endl;

       std::cout << "old: ";
       for (auto tof: tofs) {
         std::cout << tof << " ";
       }
       std::cout << std::endl;

       std::cout << "------------------------------" << std::endl;
       double difc_cumm = 0.;
       size_t difc_count = 0;
       for (size_t i = 0; i < fittedTable->rowCount(); ++i) {
         // Get peak value
         double centre = fittedTable->getRef<double>("centre", i);
         double width = fittedTable->getRef<double>("width", i);
         double height = fittedTable->getRef<double>("height", i);
         double chi2 = fittedTable->getRef<double>("chi2", i);

         std::cout << "d=" << m_peaksInDspacing[i]<< " centre old=" << tofs[i];
         if (chi2 > 1.e10) {
           std::cout << " failed to fit" << std::endl;
         } else {
           double difc = centre / m_peaksInDspacing[i];
           difc_cumm += difc;
           difc_count += 1;

           std::cout << " new=" << centre << " width=" << width << " height=" << height
                     << " chi2=" << chi2 << " difc=" << difc << std::endl;
         }
       }
       std::cout << "avg difc = " << (difc_cumm/static_cast<double>(difc_count)) << std::endl;

       break;
  }
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

vector<double> PDCalibration::dSpacingToTof(const vector<double> &dSpacing,
                                  const detid_t detid) {
  auto rowNum = m_detidToRow[detid];

  const double difa = m_calibrationTableOld->getRef<double>("difa", rowNum);
  const double difc = m_calibrationTableOld->getRef<double>("difc", rowNum);
  const double tzero = m_calibrationTableOld->getRef<double>("tzero", rowNum);
  std::cout << ">>>>> difc=" << difc << " difa=" << difa << " tzero=" << tzero << std::endl;
  auto toTof = d_to_tof(difc, difa, tzero);

  vector<double> tof(dSpacing.size());
  std::transform(dSpacing.begin(), dSpacing.end(), tof.begin(), toTof);

  return tof;
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

void PDCalibration::loadAndBin() {
  m_uncalibratedWS = getProperty("UncalibratedWorkspace");

  if (bool(m_uncalibratedWS)) {
    m_uncalibratedWS = rebin(m_uncalibratedWS);

    setProperty("UncalibratedWorkspace", m_uncalibratedWS);

    return;
  }

  const std::string signalFile = getProperty("Signalfile");
  g_log.information() << "Loading signal file \""
                      << signalFile << "\"\n";
  auto signalWS = load(getProperty("SignalFile"));

  const std::string backFile = getProperty("Backgroundfile");
  if (!backFile.empty()) {
    g_log.information() << "Loading background file \""
                        << backFile << "\"\n";
    auto backWS = load(backFile);

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
    DataObjects::EventWorkspace_sptr compressResult
      = algCompress->getProperty("OutputWorkspace");
    signalWS = boost::dynamic_pointer_cast<MatrixWorkspace>(compressResult);
  }

  m_uncalibratedWS = rebin(signalWS);

  setProperty("UncalibratedWorkspace", m_uncalibratedWS);
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
    return (std::find(columnNames.begin(), columnNames.end(), std::string("dasid")) != columnNames.end());
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
    m_calibrationTableOld = alg->getProperty("OutputCalWorkspace");

    m_hasDasIds = hasDasIDs(m_calibrationTableOld);

    // generate the map of detid -> row
    API::ColumnVector<int> detIDs = m_calibrationTableOld->getVector("detid");
    const size_t numDets = detIDs.size();
    for (size_t i = 0; i < numDets; ++i) {
      m_detidToRow[static_cast<detid_t>(detIDs[i])] = i;
    }

    // create a new workspace
    m_calibrationTableNew = boost::make_shared<DataObjects::TableWorkspace>();
    // TODO m_calibrationTable->setTitle("");
    m_calibrationTableNew->addColumn("int", "detid");
    m_calibrationTableNew->addColumn("double", "difc");
    m_calibrationTableNew->addColumn("double", "difa");
    m_calibrationTableNew->addColumn("double", "tzero");
    if (m_hasDasIds)
      m_calibrationTableNew->addColumn("int", "dasid");
    m_calibrationTableNew->addColumn("double", "tofmin");
    m_calibrationTableNew->addColumn("double", "tofmax");
    setProperty("OutputCalibrationTable", m_calibrationTableNew);

    // copy over the values
//    for (std::size_t i = 0; i < oldCalibrationTable->rowCount(); ++i) {
//        API::TableRow newRow = m_calibrationTable->appendRow();
//        newRow << oldCalibrationTable->getRef<int>("detid", i);

//    }
}

} // namespace Algorithms
} // namespace Mantid
