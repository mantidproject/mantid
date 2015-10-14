//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAlgorithms/CreatePSDBleedMask.h"
#include "MantidKernel/MultiThreaded.h"
#include "MantidAPI/WorkspaceProperty.h"
#include "MantidAPI/WorkspaceValidators.h"
#include "MantidKernel/BoundedValidator.h"
#include "MantidKernel/NullValidator.h"
#include "MantidGeometry/Instrument/DetectorGroup.h"

#include <map>
#include <list>
#include <cfloat>
#include <iterator>

namespace Mantid {
namespace Algorithms {

// Register the class
DECLARE_ALGORITHM(CreatePSDBleedMask)

const std::string CreatePSDBleedMask::category() const { return "Diagnostics"; }

using API::MatrixWorkspace_sptr;
using API::MatrixWorkspace_const_sptr;
using DataObjects::MaskWorkspace_sptr;

//----------------------------------------------------------------------
// Public methods
//----------------------------------------------------------------------

/// Default constructor
CreatePSDBleedMask::CreatePSDBleedMask()
    : m_maxRate(0.0), m_numIgnoredPixels(0), m_isRawCounts(false) {}

//----------------------------------------------------------------------
// Private methods
//----------------------------------------------------------------------

/// Initialize the algorithm properties
void CreatePSDBleedMask::init() {
  using API::WorkspaceProperty;
  using Kernel::Direction;
  using Kernel::BoundedValidator;

  declareProperty(
      new WorkspaceProperty<>("InputWorkspace", "", Direction::Input),
      "The name of the input workspace.");
  declareProperty(
      new WorkspaceProperty<>("OutputWorkspace", "", Direction::Output),
      "The name of the output MaskWorkspace which will contain the result "
      "masks.");
  auto mustBePosDbl = boost::make_shared<BoundedValidator<double>>();
  mustBePosDbl->setLower(0.0);
  declareProperty("MaxTubeFramerate", -1.0, mustBePosDbl,
                  "The maximum rate allowed for a tube in counts/us/frame.");
  auto mustBePosInt = boost::make_shared<BoundedValidator<int>>();
  mustBePosInt->setLower(0);
  declareProperty("NIgnoredCentralPixels", 80, mustBePosInt,
                  "The number of pixels about the centre to ignore.");
  declareProperty("NumberOfFailures", 0,
                  boost::make_shared<Kernel::NullValidator>(),
                  "An output property containing the number of masked tubes",
                  Direction::Output);
}

/// Execute the algorithm
void CreatePSDBleedMask::exec() {
  using Geometry::IDetector_const_sptr;

  MatrixWorkspace_const_sptr inputWorkspace = getProperty("InputWorkspace");
  // We require the number of good frames. Check that we have this
  if (!inputWorkspace->run().hasProperty("goodfrm")) {
    throw std::invalid_argument(
        "InputWorkspace does not contain the number of \"good frames\".\n"
        "(The sample log named: goodfrm with value, specifying number of good "
        "frames)");
  }
  Kernel::PropertyWithValue<int> *frameProp =
      dynamic_cast<Kernel::PropertyWithValue<int> *>(
          inputWorkspace->run().getProperty("goodfrm"));
  if (!frameProp) {
    throw std::invalid_argument("InputWorkspace has the number of \"good "
                                "frames\" property (goodfrm log value)"
                                "but this property value is not integer.");
  }

  int goodFrames = (*frameProp)();

  // Store the other properties
  double maxFramerate = getProperty("MaxTubeFramerate");
  // Multiply by the frames to save a division for each bin when we loop over
  // them later
  m_maxRate = maxFramerate * goodFrames;
  m_numIgnoredPixels = getProperty("NIgnoredCentralPixels");

  // Check the input for being a distribution
  m_isRawCounts = !(inputWorkspace->isDistribution());

  // This algorithm assumes that the instrument geometry is tube based, i.e. the
  // parent CompAssembly
  // of the lowest detector in the tree is a "tube" and that all pixels in a
  // tube are consecutively ordered
  // with respect to spectra number
  const int numSpectra =
      static_cast<int>(inputWorkspace->getNumberHistograms());
  // Keep track of a map of tubes to lists of indices
  typedef std::map<Geometry::ComponentID, std::vector<int>> TubeIndex;
  TubeIndex tubeMap;

  // NOTE: This loop is intentionally left unparallelized as the majority of the
  // work requires a lock around it which actually slows down the loop.
  // Another benefit of keep it serial is losing the need for a call to 'sort'
  // when
  // performing the bleed test as the list of indices will already be in the
  // correct
  // order
  for (int i = 0; i < numSpectra; ++i) {
    IDetector_const_sptr det;
    try {
      det = inputWorkspace->getDetector(i);
    } catch (Kernel::Exception::NotFoundError &) {
      continue;
    }
    if (det->isMonitor())
      continue;

    boost::shared_ptr<const Geometry::DetectorGroup> group =
        boost::dynamic_pointer_cast<const Geometry::DetectorGroup>(det);

    if (group) {
      det = group->getDetectors().front();
      if (!det)
        continue;
    }
    boost::shared_ptr<const Geometry::IComponent> parent = det->getParent();
    if (!parent)
      continue;

    Geometry::ComponentID parentID = parent->getComponentID();
    // Already have this component
    if (tubeMap.find(parentID) != tubeMap.end()) {
      tubeMap[parentID].push_back(i);
    }
    // New tube
    else {
      tubeMap.insert(std::pair<TubeIndex::key_type, TubeIndex::mapped_type>(
          parentID, TubeIndex::mapped_type(1, i)));
    }
  }

  // Now process the tubes in parallel
  const int numTubes = static_cast<int>(tubeMap.size());
  g_log.information() << "Found " << numTubes << " tubes.\n";
  int numSpectraMasked(0), numTubesMasked(0);
  // Create a mask workspace for output
  MaskWorkspace_sptr outputWorkspace = this->generateEmptyMask(inputWorkspace);

  PARALLEL_FOR2(inputWorkspace, outputWorkspace)
  for (int i = 0; i < numTubes; ++i) {
    PARALLEL_START_INTERUPT_REGION
    TubeIndex::iterator current = tubeMap.begin();
    std::advance(current, i);
    const TubeIndex::mapped_type tubeIndices = current->second;
    bool mask = performBleedTest(tubeIndices, inputWorkspace);
    if (mask) {
      maskTube(tubeIndices, outputWorkspace);
      PARALLEL_ATOMIC
      numSpectraMasked += static_cast<int>(tubeIndices.size());
      PARALLEL_ATOMIC
      numTubesMasked += 1;
    }

    PARALLEL_END_INTERUPT_REGION
  }
  PARALLEL_CHECK_INTERUPT_REGION

  g_log.information() << numTubesMasked << " tube(s) failed the bleed tests.";
  if (numTubesMasked > 0) {
    g_log.information()
        << " The " << numSpectraMasked
        << " spectra have been masked on the output workspace.\n";
  } else {
    g_log.information() << std::endl;
  }

  setProperty("NumberOfFailures", numSpectraMasked);
  setProperty("OutputWorkspace", outputWorkspace);
}

/**
 * Process a tube whose indices are given
 * @param tubeIndices :: A list of workspace indices that point to members of a
 * single tube
 * @param inputWS :: The workspace containing the rates or counts for each bin
 * @returns True if the tube is to be masked, false otherwise
 */
bool CreatePSDBleedMask::performBleedTest(
    const std::vector<int> &tubeIndices,
    API::MatrixWorkspace_const_sptr inputWS) {

  // Require ordered pixels so that we can define the centre.
  // This of course assumes that the pixel IDs increase monotonically with the
  // workspace index
  // and that the above loop that searched for the tubes was NOT run in parallel
  const size_t numSpectra(tubeIndices.size());
  const size_t midIndex(numSpectra / 2);
  const size_t topEnd(midIndex - m_numIgnoredPixels / 2);
  const size_t bottomBegin(midIndex + m_numIgnoredPixels / 2);

  const int numBins = static_cast<int>(inputWS->blocksize());
  std::vector<double> totalRate(numBins, 0.0);
  size_t top = 0, bot = bottomBegin;
  for (; top < topEnd; ++top, ++bot) {
    const int topIndex = tubeIndices[top];
    const int botIndex = tubeIndices[bot];
    const MantidVec &topY = inputWS->readY(topIndex);
    const MantidVec &botY = inputWS->readY(botIndex);
    const MantidVec &topX = inputWS->readX(topIndex);
    const MantidVec &botX = inputWS->readX(botIndex);
    for (int j = 0; j < numBins; ++j) {
      double topRate(topY[j]), botRate(botY[j]);
      if (m_isRawCounts) {
        topRate /= (topX[j + 1] - topX[j]);
        botRate /= (botX[j + 1] - botX[j]);
      }
      totalRate[j] += topRate + botRate;
      // If by now any have hit the allowed maximum then mark this to be masked
      if (totalRate[j] > m_maxRate) {
        return true;
      }
    }
  }

  if (top != topEnd) {
    g_log.error()
        << "Error in tube processing, loop variable has an unexpected value.\n";
    throw std::runtime_error(
        "top != topEnd in CreatePSDBleedMask::performBleedTest()");
  }
  if (bot != numSpectra) {
    g_log.error()
        << "Error in tube processing, loop variable has an unexpected value.\n";
    throw std::runtime_error(
        "bot != numSpectra  in CreatePSDBleedMask::performBleedTest()");
  }

  return false;
}

/**
 * Mask a tube with the given workspace indices
 * @param tubeIndices :: A list of the workspaces indices for the tube
 * @param workspace :: The workspace to accumulate the masking
 */
void CreatePSDBleedMask::maskTube(const std::vector<int> &tubeIndices,
                                  API::MatrixWorkspace_sptr workspace) {
  const double deadValue(1.0); // delete the data

  std::vector<int>::const_iterator cend = tubeIndices.end();
  for (std::vector<int>::const_iterator citr = tubeIndices.begin();
       citr != cend; ++citr) {
    workspace->dataY(*citr)[0] = deadValue;
  }
}
}
}
