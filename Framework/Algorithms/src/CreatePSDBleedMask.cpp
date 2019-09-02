// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidAlgorithms/CreatePSDBleedMask.h"
#include "MantidAPI/Run.h"
#include "MantidAPI/SpectrumInfo.h"
#include "MantidAPI/WorkspaceProperty.h"
#include "MantidGeometry/Instrument/DetectorGroup.h"
#include "MantidKernel/BoundedValidator.h"
#include "MantidKernel/MultiThreaded.h"
#include "MantidKernel/NullValidator.h"

#include <cfloat>
#include <iterator>
#include <list>
#include <map>

namespace Mantid {
namespace Algorithms {

// Register the class
DECLARE_ALGORITHM(CreatePSDBleedMask)

const std::string CreatePSDBleedMask::category() const { return "Diagnostics"; }

using API::MatrixWorkspace_const_sptr;
using API::MatrixWorkspace_sptr;
using DataObjects::MaskWorkspace_sptr;

/// Default constructor
CreatePSDBleedMask::CreatePSDBleedMask() {}

/// Initialize the algorithm properties
void CreatePSDBleedMask::init() {
  using API::WorkspaceProperty;
  using Kernel::BoundedValidator;
  using Kernel::Direction;

  declareProperty(std::make_unique<WorkspaceProperty<>>("InputWorkspace", "",
                                                        Direction::Input),
                  "The name of the input workspace.");
  declareProperty(
      std::make_unique<WorkspaceProperty<>>("OutputWorkspace", "",
                                            Direction::Output),
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
  double maxRate = maxFramerate * goodFrames;
  int numIgnoredPixels = getProperty("NIgnoredCentralPixels");

  // This algorithm assumes that the instrument geometry is tube based, i.e. the
  // parent CompAssembly
  // of the lowest detector in the tree is a "tube" and that all pixels in a
  // tube are consecutively ordered
  // with respect to spectra number
  const auto numSpectra =
      static_cast<int>(inputWorkspace->getNumberHistograms());
  // Keep track of a map of tubes to lists of indices
  using TubeIndex = std::map<Geometry::ComponentID, std::vector<int>>;
  TubeIndex tubeMap;

  API::Progress progress(this, 0.0, 1.0, numSpectra);

  const auto &spectrumInfo = inputWorkspace->spectrumInfo();

  // NOTE: This loop is intentionally left unparallelized as the majority of the
  // work requires a lock around it which actually slows down the loop.
  // Another benefit of keep it serial is losing the need for a call to 'sort'
  // when
  // performing the bleed test as the list of indices will already be in the
  // correct
  // order
  for (int i = 0; i < numSpectra; ++i) {
    if (!spectrumInfo.hasDetectors(i))
      continue;

    if (spectrumInfo.isMonitor(i))
      continue;

    auto &det = spectrumInfo.detector(i);
    boost::shared_ptr<const Geometry::IComponent> parent;

    if (!spectrumInfo.hasUniqueDetector(i)) {
      const auto &group = dynamic_cast<const Geometry::DetectorGroup &>(det);
      parent = group.getDetectors().front()->getParent();
    } else {
      parent = det.getParent();
    }

    if (!parent)
      continue;

    Geometry::ComponentID parentID = parent->getComponentID();
    // Already have this component
    if (tubeMap.find(parentID) != tubeMap.end()) {
      tubeMap[parentID].push_back(i);
    }
    // New tube
    else {
      tubeMap.emplace(parentID, TubeIndex::mapped_type(1, i));
    }

    progress.report();
  }

  // Now process the tubes in parallel
  const auto numTubes = static_cast<int>(tubeMap.size());
  g_log.information() << "Found " << numTubes << " tubes.\n";
  int numSpectraMasked(0), numTubesMasked(0);
  // Create a mask workspace for output
  MaskWorkspace_sptr outputWorkspace = this->generateEmptyMask(inputWorkspace);

  progress.resetNumSteps(numTubes, 0, 1);

  PARALLEL_FOR_IF(Kernel::threadSafe(*inputWorkspace, *outputWorkspace))
  for (int i = 0; i < numTubes; ++i) {
    PARALLEL_START_INTERUPT_REGION
    auto current = std::next(tubeMap.begin(), i);
    const TubeIndex::mapped_type tubeIndices = current->second;
    bool mask = performBleedTest(tubeIndices, inputWorkspace, maxRate,
                                 numIgnoredPixels);
    if (mask) {
      maskTube(tubeIndices, outputWorkspace);
      PARALLEL_ATOMIC
      numSpectraMasked += static_cast<int>(tubeIndices.size());
      PARALLEL_ATOMIC
      numTubesMasked += 1;
    }

    progress.report("Performing Bleed Test");

    PARALLEL_END_INTERUPT_REGION
  }
  PARALLEL_CHECK_INTERUPT_REGION

  g_log.information() << numTubesMasked << " tube(s) failed the bleed tests.";
  if (numTubesMasked > 0) {
    g_log.information()
        << " The " << numSpectraMasked
        << " spectra have been masked on the output workspace.\n";
  } else {
    g_log.information() << '\n';
  }

  setProperty("NumberOfFailures", numSpectraMasked);
  setProperty("OutputWorkspace", outputWorkspace);
}

/**
 * Process a tube whose indices are given
 * @param tubeIndices :: A list of workspace indices that point to members of a
 * single tube
 * @param inputWS :: The workspace containing the rates or counts for each bin
 * @param maxRate :: Maximum allowed rate
 * @param numIgnoredPixels :: Number of ignored pixels
 * @returns True if the tube is to be masked, false otherwise
 */
bool CreatePSDBleedMask::performBleedTest(
    const std::vector<int> &tubeIndices,
    API::MatrixWorkspace_const_sptr inputWS, double maxRate,
    int numIgnoredPixels) {

  // Require ordered pixels so that we can define the centre.
  // This of course assumes that the pixel IDs increase monotonically with the
  // workspace index
  // and that the above loop that searched for the tubes was NOT run in parallel
  const size_t numSpectra(tubeIndices.size());
  const size_t midIndex(numSpectra / 2);
  const size_t topEnd(midIndex - numIgnoredPixels / 2);
  const size_t bottomBegin(midIndex + numIgnoredPixels / 2);

  /// Is the input a distribution or raw counts. If true then bin width division
  /// is necessary when calculating the rate
  bool isRawCounts = !(inputWS->isDistribution());

  const auto numBins = static_cast<int>(inputWS->blocksize());
  std::vector<double> totalRate(numBins, 0.0);
  size_t top = 0, bot = bottomBegin;
  for (; top < topEnd; ++top, ++bot) {
    const int topIndex = tubeIndices[top];
    const int botIndex = tubeIndices[bot];
    auto &topY = inputWS->y(topIndex);
    auto &botY = inputWS->y(botIndex);
    auto &topX = inputWS->x(topIndex);
    auto &botX = inputWS->x(botIndex);
    for (int j = 0; j < numBins; ++j) {
      double topRate(topY[j]), botRate(botY[j]);
      if (isRawCounts) {
        topRate /= (topX[j + 1] - topX[j]);
        botRate /= (botX[j + 1] - botX[j]);
      }
      totalRate[j] += topRate + botRate;
      // If by now any have hit the allowed maximum then mark this to be masked
      if (totalRate[j] > maxRate) {
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
  for (auto tubeIndice : tubeIndices) {
    workspace->mutableY(tubeIndice)[0] = deadValue;
  }
}
} // namespace Algorithms
} // namespace Mantid
