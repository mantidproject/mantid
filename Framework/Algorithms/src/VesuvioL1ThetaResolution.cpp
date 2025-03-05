// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidAlgorithms/VesuvioL1ThetaResolution.h"

#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/FileProperty.h"
#include "MantidAPI/InstrumentFileFinder.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/SpectrumInfo.h"
#include "MantidAPI/TextAxis.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidGeometry/Instrument.h"
#include "MantidGeometry/Objects/IObject.h"
#include "MantidKernel/BoundedValidator.h"
#include "MantidKernel/Statistics.h"
#include "MantidKernel/Unit.h"

#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/trim.hpp>
#include <memory>

#include <fstream>
#include <random>

namespace Mantid::Algorithms {

using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace Mantid::Geometry;

namespace {
Mantid::Kernel::Logger g_log("VesuvioL1ThetaResolution");
}

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(VesuvioL1ThetaResolution)

/// Algorithms name for identification. @see Algorithm::name
const std::string VesuvioL1ThetaResolution::name() const { return "VesuvioL1ThetaResolution"; }

/// Algorithm's version for identification. @see Algorithm::version
int VesuvioL1ThetaResolution::version() const { return 1; }

/// Algorithm's category for identification. @see Algorithm::category
const std::string VesuvioL1ThetaResolution::category() const { return "CorrectionFunctions\\SpecialCorrections"; }

/// Algorithm's summary for use in the GUI and help. @see Algorithm::summary
const std::string VesuvioL1ThetaResolution::summary() const { return "Calculates resolution of l1 and theta"; }

//----------------------------------------------------------------------------------------------
/** Initialize the algorithm's properties.
 */
void VesuvioL1ThetaResolution::init() {
  auto positiveInt = std::make_shared<Kernel::BoundedValidator<int>>();
  positiveInt->setLower(1);
  auto positiveDouble = std::make_shared<Kernel::BoundedValidator<double>>();
  positiveDouble->setLower(DBL_EPSILON);

  const std::vector<std::string> exts{".par", ".dat"};
  declareProperty(
      std::make_unique<FileProperty>("PARFile", "", FileProperty::FileAction::OptionalLoad, exts, Direction::Input),
      "PAR file containing calibrated detector positions.");

  declareProperty("SampleWidth", 3.0, positiveDouble, "With of sample in cm.");

  declareProperty("SpectrumMin", 3, "Index of minimum spectrum");
  declareProperty("SpectrumMax", 198, "Index of maximum spectrum");

  declareProperty("NumEvents", 1000000, positiveInt, "Number of scattering events");
  declareProperty("Seed", 123456789, positiveInt, "Seed for random number generator");

  declareProperty("L1BinWidth", 0.05, positiveDouble, "Bin width for L1 distribution.");
  declareProperty("ThetaBinWidth", 0.05, positiveDouble, "Bin width for theta distribution.");

  declareProperty(std::make_unique<WorkspaceProperty<>>("OutputWorkspace", "", Direction::Output),
                  "Output workspace containing mean and standard deviation of resolution "
                  "per detector.");

  declareProperty(
      std::make_unique<WorkspaceProperty<>>("L1Distribution", "", Direction::Output, PropertyMode::Optional),
      "Distribution of lengths of the final flight path.");

  declareProperty(
      std::make_unique<WorkspaceProperty<>>("ThetaDistribution", "", Direction::Output, PropertyMode::Optional),
      "Distribution of scattering angles.");
}

//----------------------------------------------------------------------------------------------
/** Execute the algorithm.
 */
void VesuvioL1ThetaResolution::exec() {
  // Load the instrument workspace
  loadInstrument();

  const std::string l1DistributionWsName = getPropertyValue("L1Distribution");
  const std::string thetaDistributionWsName = getPropertyValue("ThetaDistribution");
  const size_t numHist = m_instWorkspace->getNumberHistograms();
  const int numEvents = getProperty("NumEvents");

  // Create output workspace of resolution
  m_outputWorkspace = WorkspaceFactory::Instance().create("Workspace2D", 4, numHist, numHist);

  // Set vertical axis to statistic labels
  auto specAxis = std::make_unique<TextAxis>(4);
  specAxis->setLabel(0, "l1_Mean");
  specAxis->setLabel(1, "l1_StdDev");
  specAxis->setLabel(2, "theta_Mean");
  specAxis->setLabel(3, "theta_StdDev");
  m_outputWorkspace->replaceAxis(1, std::move(specAxis));

  // Set X axis to spectrum numbers
  m_outputWorkspace->getAxis(0)->setUnit("Label");
  auto xAxis = std::dynamic_pointer_cast<Units::Label>(m_outputWorkspace->getAxis(0)->unit());
  if (xAxis)
    xAxis->setLabel("Spectrum Number");

  // Create output workspaces for distributions if required
  if (!l1DistributionWsName.empty()) {
    m_l1DistributionWs = WorkspaceFactory::Instance().create(m_instWorkspace, numHist, numEvents, numEvents);

    // Set Y axis
    m_l1DistributionWs->setYUnitLabel("Events");

    // Set X axis
    auto distributionXAxis = m_l1DistributionWs->getAxis(0);
    distributionXAxis->setUnit("Label");
    auto labelUnit = std::dynamic_pointer_cast<Units::Label>(distributionXAxis->unit());
    if (labelUnit)
      labelUnit->setLabel("l1");
  }

  if (!thetaDistributionWsName.empty()) {
    m_thetaDistributionWs = WorkspaceFactory::Instance().create(m_instWorkspace, numHist, numEvents, numEvents);

    // Set Y axis
    m_thetaDistributionWs->setYUnitLabel("Events");

    // Set X axis
    auto distributionXAxis = m_thetaDistributionWs->getAxis(0);
    distributionXAxis->setUnit("Label");
    auto labelUnit = std::dynamic_pointer_cast<Units::Label>(distributionXAxis->unit());
    if (labelUnit)
      labelUnit->setLabel("theta");
  }

  // Set up progress reporting
  Progress prog(this, 0.0, 1.0, numHist);
  const int seed(getProperty("Seed"));
  std::mt19937 randEngine(static_cast<std::mt19937::result_type>(seed));
  std::uniform_real_distribution<> flatDistrib(0.0, 1.0);
  std::function<double()> flatVariateGen([&randEngine, &flatDistrib]() { return flatDistrib(randEngine); });

  const auto &spectrumInfo = m_instWorkspace->spectrumInfo();
  // Loop for all detectors
  for (size_t i = 0; i < numHist; i++) {
    std::vector<double> l1;
    std::vector<double> theta;
    const auto &det = spectrumInfo.detector(i);

    // Report progress
    std::stringstream report;
    report << "Detector " << det.getID();
    prog.report(report.str());
    g_log.information() << "Detector ID " << det.getID() << '\n';

    // Do simulation
    calculateDetector(det, flatVariateGen, l1, theta);

    // Calculate statistics for L1 and theta
    Statistics l1Stats = getStatistics(l1);
    Statistics thetaStats = getStatistics(theta);

    g_log.information() << "l0: mean=" << l1Stats.mean << ", std.dev.=" << l1Stats.standard_deviation
                        << "\ntheta: mean=" << thetaStats.mean << ", std.dev.=" << thetaStats.standard_deviation
                        << '\n';

    // Set values in output workspace
    const int specNo = m_instWorkspace->getSpectrum(i).getSpectrumNo();
    m_outputWorkspace->mutableX(0)[i] = specNo;
    m_outputWorkspace->mutableX(1)[i] = specNo;
    m_outputWorkspace->mutableX(2)[i] = specNo;
    m_outputWorkspace->mutableX(3)[i] = specNo;
    m_outputWorkspace->mutableY(0)[i] = l1Stats.mean;
    m_outputWorkspace->mutableY(1)[i] = l1Stats.standard_deviation;
    m_outputWorkspace->mutableY(2)[i] = thetaStats.mean;
    m_outputWorkspace->mutableY(3)[i] = thetaStats.standard_deviation;

    // Process data for L1 distribution
    if (m_l1DistributionWs) {
      auto &x = m_l1DistributionWs->mutableX(i);

      std::sort(l1.begin(), l1.end());
      std::copy(l1.begin(), l1.end(), x.begin());

      m_l1DistributionWs->mutableY(i) = 1.0;

      auto &spec = m_l1DistributionWs->getSpectrum(i);
      spec.setSpectrumNo(specNo);
      spec.addDetectorID(det.getID());
    }

    // Process data for theta distribution
    if (m_thetaDistributionWs) {
      auto &x = m_thetaDistributionWs->mutableX(i);

      std::sort(theta.begin(), theta.end());
      std::copy(theta.begin(), theta.end(), x.begin());

      m_thetaDistributionWs->mutableY(i) = 1.0;

      auto &spec = m_thetaDistributionWs->getSpectrum(i);
      spec.setSpectrumNo(specNo);
      spec.addDetectorID(det.getID());
    }
  }

  // Process the L1 distribution workspace
  if (m_l1DistributionWs) {
    const double binWidth = getProperty("L1BinWidth");
    setProperty("L1Distribution", processDistribution(m_l1DistributionWs, binWidth));
  }

  // Process the theta distribution workspace
  if (m_thetaDistributionWs) {
    const double binWidth = getProperty("ThetaBinWidth");
    setProperty("ThetaDistribution", processDistribution(m_thetaDistributionWs, binWidth));
  }

  setProperty("OutputWorkspace", m_outputWorkspace);
}

//----------------------------------------------------------------------------------------------
/** Loads the instrument into a workspace.
 */
void VesuvioL1ThetaResolution::loadInstrument() {
  // Get the filename for the VESUVIO IDF
  const std::string vesuvioIPF = InstrumentFileFinder::getInstrumentFilename("VESUVIO");

  // Load an empty VESUVIO instrument workspace
  auto loadInst = AlgorithmManager::Instance().create("LoadEmptyInstrument");
  loadInst->initialize();
  loadInst->setChild(true);
  loadInst->setLogging(false);
  loadInst->setProperty("OutputWorkspace", "__evs");
  loadInst->setProperty("Filename", vesuvioIPF);
  loadInst->execute();
  m_instWorkspace = loadInst->getProperty("OutputWorkspace");

  // Load the PAR file if provided
  const std::string parFilename = getPropertyValue("PARFile");
  if (!parFilename.empty()) {
    g_log.information() << "Loading PAR file: " << parFilename << '\n';

    // Get header format
    std::map<size_t, std::string> headerFormats;
    headerFormats[5] = "spectrum,theta,t0,-,R";
    headerFormats[6] = "spectrum,-,theta,t0,-,R";

    std::ifstream parFile(parFilename);
    if (!parFile) {
      throw std::runtime_error("Cannot open PAR file");
    }
    std::string header;
    getline(parFile, header);
    g_log.debug() << "PAR file header: " << header << '\n';
    boost::trim(header);
    std::vector<std::string> headers;
    boost::split(headers, header, boost::is_any_of("\t "), boost::token_compress_on);
    size_t numCols = headers.size();
    g_log.debug() << "PAR file columns: " << numCols << '\n';

    std::string headerFormat = headerFormats[numCols];
    if (headerFormat.empty()) {
      std::stringstream error;
      error << "Unrecognised PAR file header. Number of colums: " << numCols << " (expected either 5 or 6.";
      throw std::runtime_error(error.str());
    }
    g_log.debug() << "PAR file header format: " << headerFormat << '\n';

    // Update instrument
    auto updateInst = AlgorithmManager::Instance().create("UpdateInstrumentFromFile");
    updateInst->initialize();
    updateInst->setChild(true);
    updateInst->setLogging(false);
    updateInst->setProperty("Workspace", m_instWorkspace);
    updateInst->setProperty("Filename", parFilename);
    updateInst->setProperty("MoveMonitors", false);
    updateInst->setProperty("IgnorePhi", true);
    updateInst->setProperty("AsciiHeader", headerFormat);
    updateInst->execute();
    m_instWorkspace = updateInst->getProperty("Workspace");
  }

  const int specIdxMin = static_cast<int>(m_instWorkspace->getIndexFromSpectrumNumber(getProperty("SpectrumMin")));
  const int specIdxMax = static_cast<int>(m_instWorkspace->getIndexFromSpectrumNumber(getProperty("SpectrumMax")));

  // Crop the workspace to just the detectors we are interested in
  auto crop = AlgorithmManager::Instance().create("CropWorkspace");
  crop->initialize();
  crop->setChild(true);
  crop->setLogging(false);
  crop->setProperty("InputWorkspace", m_instWorkspace);
  crop->setProperty("OutputWorkspace", "__evs");
  crop->setProperty("StartWorkspaceIndex", specIdxMin);
  crop->setProperty("EndWorkspaceIndex", specIdxMax);
  crop->execute();
  m_instWorkspace = crop->getProperty("OutputWorkspace");

  m_sample = m_instWorkspace->getInstrument()->getSample();
}

//----------------------------------------------------------------------------------------------
/** Loads the instrument into a workspace.
 */
void VesuvioL1ThetaResolution::calculateDetector(const IDetector &detector,
                                                 const std::function<double()> &flatRandomVariateGen,
                                                 std::vector<double> &l1Values, std::vector<double> &thetaValues) {
  const int numEvents = getProperty("NumEvents");
  l1Values.reserve(numEvents);
  thetaValues.reserve(numEvents);

  double sampleWidth = getProperty("SampleWidth");
  // If the sample is large fix the width to the approximate beam width
  if (sampleWidth > 4.0)
    sampleWidth = 4.0;

  // Get detector dimensions
  Geometry::IObject_const_sptr pixelShape = detector.shape();
  if (!pixelShape || !pixelShape->hasValidShape()) {
    throw std::invalid_argument("Detector pixel has no defined shape!");
  }
  Geometry::BoundingBox detBounds = pixelShape->getBoundingBox();
  V3D detBoxWidth = detBounds.width();
  const double detWidth = detBoxWidth.X() * 100;
  const double detHeight = detBoxWidth.Y() * 100;

  g_log.debug() << "detWidth=" << detWidth << "\ndetHeight=" << detHeight << '\n';

  // Scattering angle in rad
  const double theta = m_instWorkspace->detectorTwoTheta(detector);
  if (theta == 0.0)
    return;

  // Final flight path in cm
  const double l1av = detector.getDistance(*m_sample) * 100.0;

  const double x0 = l1av * sin(theta);
  const double y0 = l1av * cos(theta);

  // Get as many events as defined by NumEvents
  // This loop is not iteration limited but highly unlikely to ever become
  // infinate
  while (l1Values.size() < static_cast<size_t>(numEvents)) {
    const double xs = -sampleWidth / 2 + sampleWidth * flatRandomVariateGen();
    const double ys = 0.0;
    const double zs = -sampleWidth / 2 + sampleWidth * flatRandomVariateGen();
    const double rs = sqrt(pow(xs, 2) + pow(zs, 2));

    if (rs <= sampleWidth / 2) {
      const double a = -detWidth / 2 + detWidth * flatRandomVariateGen();
      const double xd = x0 - a * cos(theta);
      const double yd = y0 + a * sin(theta);
      const double zd = -detHeight / 2 + detHeight * flatRandomVariateGen();

      const double l1 = sqrt(pow(xd - xs, 2) + pow(yd - ys, 2) + pow(zd - zs, 2));
      double angle = acos(yd / l1);

      if (xd < 0.0)
        angle *= -1;

      // Convert angle to degrees
      angle *= 180.0 / M_PI;

      l1Values.emplace_back(l1);
      thetaValues.emplace_back(angle);
    }

    interruption_point();
  }
}

//----------------------------------------------------------------------------------------------
/** Rebins the distributions and sets error values.
 */
MatrixWorkspace_sptr VesuvioL1ThetaResolution::processDistribution(MatrixWorkspace_sptr ws, const double binWidth) {
  const size_t numHist = ws->getNumberHistograms();

  double xMin(DBL_MAX);
  double xMax(DBL_MIN);
  for (size_t i = 0; i < numHist; i++) {
    auto &x = ws->x(i);
    xMin = std::min(xMin, x.front());
    xMax = std::max(xMax, x.back());
  }

  std::stringstream binParams;
  binParams << xMin << "," << binWidth << "," << xMax;

  auto rebin = AlgorithmManager::Instance().create("Rebin");
  rebin->initialize();
  rebin->setChild(true);
  rebin->setLogging(false);
  rebin->setProperty("InputWorkspace", ws);
  rebin->setProperty("OutputWorkspace", "__rebin");
  rebin->setProperty("Params", binParams.str());
  rebin->execute();
  ws = rebin->getProperty("OutputWorkspace");

  for (size_t i = 0; i < numHist; i++) {
    auto &y = ws->y(i);
    auto &e = ws->mutableE(i);
    std::transform(y.begin(), y.end(), e.begin(), [](double x) { return sqrt(x); });
  }

  return ws;
}

} // namespace Mantid::Algorithms
