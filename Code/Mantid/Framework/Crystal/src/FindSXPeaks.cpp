//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidCrystal/FindSXPeaks.h"
#include "MantidAPI/WorkspaceValidators.h"
#include "MantidKernel/VectorHelper.h"
#include "MantidKernel/BoundedValidator.h"

using namespace Mantid::DataObjects;

namespace Mantid {
namespace Crystal {
// Register the class into the algorithm factory
DECLARE_ALGORITHM(FindSXPeaks)

using namespace Kernel;
using namespace API;

/** Initialisation method.
 *
 */
void FindSXPeaks::init() {
  declareProperty(
      new WorkspaceProperty<>("InputWorkspace", "", Direction::Input,
                              boost::make_shared<HistogramValidator>()),
      "The name of the Workspace2D to take as input");
  declareProperty("RangeLower", EMPTY_DBL(),
                  "The X value to search from (default 0)");
  declareProperty("RangeUpper", EMPTY_DBL(),
                  "The X value to search to (default FindSXPeaks)");
  auto mustBePositive = boost::make_shared<BoundedValidator<int>>();
  mustBePositive->setLower(0);
  declareProperty("StartWorkspaceIndex", 0, mustBePositive,
                  "Start spectrum number (default 0)");
  declareProperty("EndWorkspaceIndex", EMPTY_INT(), mustBePositive,
                  "End spectrum number  (default FindSXPeaks)");
  declareProperty("SignalBackground", 10.0,
                  "Multiplication factor for the signal background");
  declareProperty(
      "Resolution", 0.01,
      "Tolerance needed to avoid peak duplication in number of pixels");
  declareProperty(new WorkspaceProperty<PeaksWorkspace>("OutputWorkspace", "",
                                                        Direction::Output),
                  "The name of the PeaksWorkspace in which to store the list "
                  "of peaks found");

  // Create the output peaks workspace
  m_peaks.reset(new PeaksWorkspace);
}

/** Executes the algorithm
 *
 *  @throw runtime_error Thrown if algorithm cannot execute
 */
void FindSXPeaks::exec() {
  // Try and retrieve the optional properties
  m_MinRange = getProperty("RangeLower");
  m_MaxRange = getProperty("RangeUpper");

  int minSpec = getProperty("StartWorkspaceIndex");
  int maxSpec = getProperty("EndWorkspaceIndex");
  m_MinSpec = minSpec;
  m_MaxSpec = maxSpec;
  double SB = getProperty("SignalBackground");

  // Get the input workspace
  MatrixWorkspace_const_sptr localworkspace = getProperty("InputWorkspace");

  // copy the instrument accross. Cannot generate peaks without doing this
  // first.
  m_peaks->setInstrument(localworkspace->getInstrument());

  size_t numberOfSpectra = localworkspace->getNumberHistograms();

  // Check 'StartSpectrum' is in range 0-numberOfSpectra
  if (m_MinSpec > numberOfSpectra) {
    g_log.warning("StartSpectrum out of range! Set to 0.");
    m_MinSpec = 0;
  }
  if (m_MinSpec > m_MaxSpec) {
    throw std::invalid_argument(
        "Cannot have StartWorkspaceIndex > EndWorkspaceIndex");
  }
  if (isEmpty(m_MaxSpec))
    m_MaxSpec = numberOfSpectra - 1;
  if (m_MaxSpec > numberOfSpectra - 1 || m_MaxSpec < m_MinSpec) {
    g_log.warning("EndSpectrum out of range! Set to max detector number");
    m_MaxSpec = numberOfSpectra;
  }
  if (m_MinRange > m_MaxRange) {
    g_log.warning("Range_upper is less than Range_lower. Will integrate up to "
                  "frame maximum.");
    m_MaxRange = 0.0;
  }

  Progress progress(this, 0, 1, (m_MaxSpec - m_MinSpec + 1));

  // Calculate the primary flight path.
  Kernel::V3D sample = localworkspace->getInstrument()->getSample()->getPos();
  Kernel::V3D L1 =
      sample - localworkspace->getInstrument()->getSource()->getPos();

  double l1 = L1.norm();
  //

  peakvector entries;
  // Reserve 1000 peaks to make later push_back fast for first 1000 peaks, but
  // unlikely to have more than this.
  entries.reserve(1000);
  // Count the peaks so that we can resize the peakvector at the end.
  PARALLEL_FOR1(localworkspace)
  for (int i = static_cast<int>(m_MinSpec); i <= static_cast<int>(m_MaxSpec);
       ++i) {
    PARALLEL_START_INTERUPT_REGION
    // Retrieve the spectrum into a vector
    const MantidVec &X = localworkspace->readX(i);
    const MantidVec &Y = localworkspace->readY(i);

    // Find the range [min,max]
    MantidVec::const_iterator lowit, highit;

    if (m_MinRange == EMPTY_DBL())
      lowit = X.begin();
    else
      lowit = std::lower_bound(X.begin(), X.end(), m_MinRange);

    if (m_MaxRange == EMPTY_DBL())
      highit = X.end();
    else
      highit = std::find_if(lowit, X.end(),
                            std::bind2nd(std::greater<double>(), m_MaxRange));

    // If range specified doesn't overlap with this spectrum then bail out
    if (lowit == X.end() || highit == X.begin())
      continue;

    --highit; // Upper limit is the bin before, i.e. the last value smaller than
              // MaxRange

    MantidVec::difference_type distmin = std::distance(X.begin(), lowit);
    MantidVec::difference_type distmax = std::distance(X.begin(), highit);

    // Find the max element
    MantidVec::const_iterator maxY;
    if (Y.size() > 1) {
      maxY = std::max_element(Y.begin() + distmin, Y.begin() + distmax);
    } else {
      maxY = Y.begin();
    }
    double intensity = (*maxY);
    double background = 0.5 * (1.0 + Y.front() + Y.back());
    if (intensity < SB * background) // This is not a peak.
      continue;
    MantidVec::difference_type d = std::distance(Y.begin(), maxY);
    // t.o.f. of the peak
    double tof = 0.5 * (*(X.begin() + d) + *(X.begin() + d + 1));

    Geometry::IDetector_const_sptr det;
    try {
      det = localworkspace->getDetector(static_cast<size_t>(i));
    } catch (Mantid::Kernel::Exception::NotFoundError &) {
      // Catch if no detector. Next line tests whether this happened - test
      // placed
      // outside here because Mac Intel compiler doesn't like 'continue' in a
      // catch
      // in an openmp block.
    }
    // If no detector found, skip onto the next spectrum
    if (!det)
      continue;

    double phi = det->getPhi();
    if (phi < 0) {
      phi += 2.0 * M_PI;
    }

    double th2 = det->getTwoTheta(Mantid::Kernel::V3D(0, 0, 0),
                                  Mantid::Kernel::V3D(0, 0, 1));

    std::vector<int> specs(1, i);

    Mantid::Kernel::V3D L2 = det->getPos();
    L2 -= sample;
    // std::cout << "r,th,phi,t: " << L2.norm() << "," << th2*180/M_PI << "," <<
    // phi*180/M_PI << "," << tof << "\n";

    SXPeak peak(tof, th2, phi, *maxY, specs, l1 + L2.norm(), det->getID());
    PARALLEL_CRITICAL(entries) { entries.push_back(peak); }
    progress.report();
    PARALLEL_END_INTERUPT_REGION
  }
  PARALLEL_CHECK_INTERUPT_REGION

  // Now reduce the list with duplicate entries
  reducePeakList(entries);

  setProperty("OutputWorkspace", m_peaks);
  progress.report();
  return;
}

/**
Reduce the peak list by removing duplicates
then convert SXPeaks objects to PeakObjects and add them to the output workspace
@param pcv : current peak list containing potential duplicates
*/
void FindSXPeaks::reducePeakList(const peakvector &pcv) {
  double resol = getProperty("Resolution");
  peakvector finalv;
  bool found = false;
  for (std::size_t i = 0; i < pcv.size(); i++) {
    for (std::size_t j = 0; j < finalv.size(); j++) {
      if (pcv[i].compare(finalv[j], resol)) {
        finalv[j] += pcv[i];
        found = true;
        break;
      }
    }
    if (!found)
      finalv.push_back(pcv[i]);
    found = false;
  }

  for (std::size_t i = 0; i < finalv.size(); i++) {
    finalv[i].reduce();
    try {
      IPeak *peak = m_peaks->createPeak(finalv[i].getQ());
      if (peak) {
        peak->setIntensity(finalv[i].getIntensity());
        peak->setDetectorID(finalv[i].getDetectorId());
        m_peaks->addPeak(*peak);
        delete peak;
      }
    } catch (std::exception &e) {
      g_log.error() << e.what() << std::endl;
    }
  }
}
} // namespace Algorithms
} // namespace Mantid
