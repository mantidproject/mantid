#include "MantidAlgorithms/VesuvioL1ThetaResolution.h"

#include "MantidAPI/AlgorithmManager.h"
#include "MantidKernel/BoundedValidator.h"
#include "MantidKernel/Statistics.h"

#include <boost/make_shared.hpp>
#include <boost/random/variate_generator.hpp>
#include <boost/random/uniform_real.hpp>

namespace Mantid {
namespace Algorithms {

using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace Mantid::Geometry;

namespace {
  Mantid::Kernel::Logger g_log("VesuvioL1ThetaResolution");
}

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(VesuvioL1ThetaResolution)

//----------------------------------------------------------------------------------------------
/** Constructor
 */
VesuvioL1ThetaResolution::VesuvioL1ThetaResolution() {}

//----------------------------------------------------------------------------------------------
/** Destructor
 */
VesuvioL1ThetaResolution::~VesuvioL1ThetaResolution() {}

//----------------------------------------------------------------------------------------------

/// Algorithms name for identification. @see Algorithm::name
const std::string VesuvioL1ThetaResolution::name() const { return "VesuvioL1ThetaResolution"; }

/// Algorithm's version for identification. @see Algorithm::version
int VesuvioL1ThetaResolution::version() const { return 1; }

/// Algorithm's category for identification. @see Algorithm::category
const std::string VesuvioL1ThetaResolution::category() const {
  return "CorrectionFunctions";
}

/// Algorithm's summary for use in the GUI and help. @see Algorithm::summary
const std::string VesuvioL1ThetaResolution::summary() const {
  return "Calculates resolution of l1 and theta";
}

//----------------------------------------------------------------------------------------------
/** Initialize the algorithm's properties.
 */
void VesuvioL1ThetaResolution::init() {
  auto positiveInt = boost::make_shared<Kernel::BoundedValidator<int>>();
  positiveInt->setLower(1);

  declareProperty("SpectrumMin", 3,
                  "Index of minimum spectrum");
  declareProperty("SpectrumMax", 198,
                  "Index of maximum spectrum");

  declareProperty("NumEvents", 10000,
                  "Number of scattering events");
  declareProperty("Seed", 123456789,
                  "Seed for random number generator");

  declareProperty("L1BinWidth", 0.01,
                  "Bin width for L1 distribution.");
  declareProperty("ThetaBinWidth", 0.0001,
                  "Bin width for theta distribution.");

  declareProperty(
      new WorkspaceProperty<>("L1Distribution", "", Direction::Output, PropertyMode::Optional),
      "Distribution of lengths of the final flight path.");

  declareProperty(
      new WorkspaceProperty<>("ThetaDistribution", "", Direction::Output, PropertyMode::Optional),
      "Distribution of scattering angles.");
}

//----------------------------------------------------------------------------------------------
/** Execute the algorithm.
 */
void VesuvioL1ThetaResolution::exec() {
  // Set up random number generator
  m_generator.seed(static_cast<boost::mt19937::result_type>(static_cast<int>(getProperty("Seed"))));

  // Load the instrument workspace
  loadInstrument();

  const std::string l1DistributionWsName = getPropertyValue("L1Distribution");
  const std::string thetaDistributionWsName = getPropertyValue("ThetaDistribution");
  const size_t numHist = m_instWorkspace->getNumberHistograms();
  const int numEvents = getProperty("NumEvents");

  // Create output workspaces for distributions if required
  if(!l1DistributionWsName.empty())
    m_l1DistributionWs = WorkspaceFactory::Instance().create("Workspace2D", numHist, numEvents, numEvents);
  if(!thetaDistributionWsName.empty())
    m_thetaDistributionWs = WorkspaceFactory::Instance().create("Workspace2D", numHist, numEvents, numEvents);

  // Set up progress reporting
  Progress prog(this, 0.0, 1.0, numHist);

  // Loop for all detectors
  for(size_t i = 0; i < numHist; i++) {
    std::vector<double> l1;
    std::vector<double> theta;
    IDetector_const_sptr det = m_instWorkspace->getDetector(i);

    // Report progress
    std::stringstream report;
    report << "Detector " << det->getID();
    prog.report(report.str());

    // Do simulation
    calculateDetector(det, l1, theta);

    // Calculate statistics for L1 and theta
    Statistics l1Stats = getStatistics(l1);
    Statistics thetaStats = getStatistics(theta);

    g_log.information() << "Detector ID: " << det->getID() << std::endl
                        << "l0: mean=" << l1Stats.mean << ", std.dev.="
                        << l1Stats.standard_deviation << std::endl
                        << "theta: mean=" << thetaStats.mean << ", std.dev.="
                        << thetaStats.standard_deviation << std::endl;

    // Process data for L1 distribution
    if(m_l1DistributionWs) {
      std::vector<double>& x = m_l1DistributionWs->dataX(i);
      std::vector<double> y(numEvents, 1.0);

      std::sort(l1.begin(), l1.end());
      std::copy(l1.begin(), l1.end(), x.begin());

      m_l1DistributionWs->dataY(i) = y;
    }

    // Process data for theta distribution
    if(m_thetaDistributionWs) {
      std::vector<double>& x = m_thetaDistributionWs->dataX(i);
      std::vector<double> y(numEvents, 1.0);

      std::sort(theta.begin(), theta.end());
      std::copy(theta.begin(), theta.end(), x.begin());

      m_thetaDistributionWs->dataY(i) = y;
    }
  }

  // Process the L1 distribution workspace
  if(m_l1DistributionWs) {
    const double binWidth = getProperty("L1BinWidth");
    setProperty("L1Distribution", processDistribution(m_l1DistributionWs, binWidth));
  }

  // Process the theta distribution workspace
  if(m_thetaDistributionWs) {
    const double binWidth = getProperty("ThetaBinWidth");
    setProperty("ThetaDistribution", processDistribution(m_thetaDistributionWs, binWidth));
  }

}

//----------------------------------------------------------------------------------------------
/** Loads the instrument into a workspace.
 */
void VesuvioL1ThetaResolution::loadInstrument() {
  MatrixWorkspace_sptr tempWS = WorkspaceFactory::Instance().create("Workspace2D", 1, 1, 1);
  const std::string vesuvioIPF = tempWS->getInstrumentFilename("VESUVIO");

  IAlgorithm_sptr loadInst = AlgorithmManager::Instance().create("LoadEmptyInstrument");
  loadInst->initialize();
  loadInst->setChild(true);
  loadInst->setLogging(false);
  loadInst->setProperty("OutputWorkspace", "__evs");
  loadInst->setProperty("Filename", vesuvioIPF);
  loadInst->execute();
  m_instWorkspace = loadInst->getProperty("OutputWorkspace");

  //TODO: load par file

  const int specIdxMin = static_cast<int>(m_instWorkspace->getIndexFromSpectrumNumber(getProperty("SpectrumMin")));
  const int specIdxMax = static_cast<int>(m_instWorkspace->getIndexFromSpectrumNumber(getProperty("SpectrumMax")));

  IAlgorithm_sptr crop = AlgorithmManager::Instance().create("CropWorkspace");
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
void VesuvioL1ThetaResolution::calculateDetector(IDetector_const_sptr detector, std::vector<double>& l1Values, std::vector<double>& thetaValues) {
  const int numEvents = getProperty("NumEvents");
  l1Values.reserve(numEvents);
  thetaValues.reserve(numEvents);

  //TODO
  const double detHeight = 25.0;
  const double detWidth = 2.5;
  const double beamWidth = 3.0;

  // Scattering angle in rad
  const double theta = m_instWorkspace->detectorSignedTwoTheta(detector);
  if(theta == 0.0)
    return;

  // Final flight path in cm
  const double l1av = detector->getDistance(*m_sample) * 100.0;

  const double x0 = l1av * sin(theta);
  const double y0 = l1av * cos(theta);

  // Get as many events as defined by NumEvents
  // This loop is not iteration limited but highly unlikely to ever become infinate
  while(l1Values.size() < static_cast<size_t>(numEvents)) {
    const double xs = -beamWidth/2 + beamWidth*random();
    const double ys = 0.0;
    const double zs = -beamWidth/2 + beamWidth*random();
    const double rs = sqrt(pow(xs, 2) + pow(xs, 2));

    if(rs <= beamWidth/2) {
      const double a = -detWidth/2 + detWidth*random();
      const double xd = x0 - a*cos(theta);
      const double yd = y0 + a*sin(theta);
      const double zd = -detHeight/2 + detHeight*random();

      const double l1 = sqrt(pow(xd-xs, 2) + pow(yd-ys, 2) + pow(zd-zs, 2));
      double angle = acos(yd / l1);

      if(xd < 0.0)
        angle *= -1;

      //TODO: convert angle to degrees

      l1Values.push_back(l1);
      thetaValues.push_back(angle);
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
  for(size_t i = 0; i < numHist; i++) {
    const std::vector<double> x = ws->readX(i);
    if(x[0] < xMin)
      xMin = x[0];
    if(x[x.size()-1] > xMax)
      xMax = x[x.size()-1];
  }

  std::stringstream binParams;
  binParams << xMin << "," << binWidth << "," << xMax;

  IAlgorithm_sptr rebin = AlgorithmManager::Instance().create("Rebin");
  rebin->initialize();
  rebin->setChild(true);
  rebin->setLogging(false);
  rebin->setProperty("InputWorkspace", ws);
  rebin->setProperty("OutputWorkspace", "__rebin");
  rebin->setProperty("Params", binParams.str());
  rebin->execute();
  ws = rebin->getProperty("OutputWorkspace");

  for(size_t i = 0; i < numHist; i++) {
    const std::vector<double> y = ws->readY(i);
    std::vector<double>& e = ws->dataE(i);

    std::transform(y.begin(), y.end(), e.begin(), sqrt);
  }

  return ws;
}

//----------------------------------------------------------------------------------------------
/** Generates a random number.
 */
double VesuvioL1ThetaResolution::random() {
  typedef boost::uniform_real<double> uniform_double;
  return boost::variate_generator<boost::mt19937 &, uniform_double>(m_generator, uniform_double(0.0, 1.0))();
}

} // namespace Algorithms
} // namespace Mantid
