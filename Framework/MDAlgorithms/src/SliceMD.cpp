#include "MantidMDAlgorithms/SliceMD.h"
#include "MantidAPI/FileProperty.h"
#include "MantidAPI/IMDEventWorkspace.h"
#include "MantidDataObjects/MDEventFactory.h"
#include "MantidGeometry/MDGeometry/MDHistoDimension.h"
#include "MantidGeometry/MDGeometry/MDImplicitFunction.h"
#include "MantidKernel/BoundedValidator.h"
#include "MantidKernel/EnabledWhenProperty.h"
#include "MantidKernel/System.h"
#include "MantidKernel/ThreadPool.h"
#include "MantidKernel/ThreadScheduler.h"

using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace Mantid::Geometry;
using namespace Mantid::DataObjects;

namespace Mantid {
namespace MDAlgorithms {

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(SliceMD)

//----------------------------------------------------------------------------------------------
/** Initialize the algorithm's properties.
 */
void SliceMD::init() {
  declareProperty(make_unique<WorkspaceProperty<IMDWorkspace>>(
                      "InputWorkspace", "", Direction::Input),
                  "An input MDWorkspace.");

  // Properties for specifying the slice to perform.
  this->initSlicingProps();

  declareProperty(make_unique<WorkspaceProperty<Workspace>>(
                      "OutputWorkspace", "", Direction::Output),
                  "Name of the output MDEventWorkspace.");

  declareProperty(
      make_unique<FileProperty>("OutputFilename", "",
                                FileProperty::OptionalSave, ".nxs"),
      "Optional: Specify a NeXus file to write if you want the output "
      "workspace to be file-backed.");

  declareProperty(
      make_unique<PropertyWithValue<int>>("Memory", -1),
      "If OutputFilename is specified to use a file back end:\n"
      "  The amount of memory (in MB) to allocate to the in-memory cache.\n"
      "  If not specified, a default of 40% of free physical memory is used.");

  declareProperty("TakeMaxRecursionDepthFromInput", true,
                  "Copy the maximum recursion depth from the input workspace.");

  auto mustBePositiveInteger = boost::make_shared<BoundedValidator<int>>();
  mustBePositiveInteger->setLower(0);

  declareProperty("MaxRecursionDepth", 1000, mustBePositiveInteger,
                  "Sets the maximum recursion depth to use. Can be used to "
                  "constrain the workspaces internal structure");
  setPropertySettings("MaxRecursionDepth",
                      make_unique<EnabledWhenProperty>(
                          "TakeMaxRecursionDepthFromInput", IS_EQUAL_TO, "0"));

  setPropertyGroup("OutputFilename", "File Back-End");
  setPropertyGroup("Memory", "File Back-End");
}

//----------------------------------------------------------------------------------------------
/** Copy the extra data (not signal, error or coordinates) from one event to
 *another
 * with different numbers of dimensions
 *
 * @param srcEvent :: the source event, being copied
 * @param newEvent :: the destination event
 */
template <size_t nd, size_t ond>
inline void copyEvent(const MDLeanEvent<nd> &srcEvent,
                      MDLeanEvent<ond> &newEvent) {
  // Nothing extra copy - this is no-op
  UNUSED_ARG(srcEvent);
  UNUSED_ARG(newEvent);
}

//----------------------------------------------------------------------------------------------
/** Copy the extra data (not signal, error or coordinates) from one event to
 *another
 * with different numbers of dimensions
 *
 * @param srcEvent :: the source event, being copied
 * @param newEvent :: the destination event
 */
template <size_t nd, size_t ond>
inline void copyEvent(const MDEvent<nd> &srcEvent, MDEvent<ond> &newEvent) {
  newEvent.setDetectorId(srcEvent.getDetectorID());
  newEvent.setRunIndex(srcEvent.getRunIndex());
}

//----------------------------------------------------------------------------------------------
/** Perform the slice from nd input dimensions to ond output dimensions
 *
 * @param ws :: input workspace with nd dimensions
 * @tparam OMDE :: MDEvent type for the OUTPUT workspace
 * @tparam ond :: number of dimensions in the OUTPUT workspace
 */
template <typename MDE, size_t nd, typename OMDE, size_t ond>
void SliceMD::slice(typename MDEventWorkspace<MDE, nd>::sptr ws) {
  // Create the ouput workspace
  typename MDEventWorkspace<OMDE, ond>::sptr outWS(
      new MDEventWorkspace<OMDE, ond>());
  for (auto &binDimension : m_binDimensions) {
    outWS->addDimension(binDimension);
  }
  outWS->setCoordinateSystem(ws->getSpecialCoordinateSystem());
  outWS->initialize();
  // Copy settings from the original box controller
  BoxController_sptr bc = ws->getBoxController();

  // store wrute buffer size for the future
  // uint64_t writeBufSize =
  // bc->getFileIO()getDiskBuffer().getWriteBufferSize();
  // and disable write buffer (if any) for input MD Events for this algorithm
  // purposes;
  // bc->setCacheParameters(1,0);

  BoxController_sptr obc = outWS->getBoxController();
  // Use the "number of bins" as the "split into" parameter for the top level
  for (size_t od = 0; od < m_binDimensions.size(); od++) {
    obc->setSplitTopInto(od, m_binDimensions[od]->getNBins());
    obc->setSplitInto(od, bc->getSplitInto(od));
  }
  obc->setSplitThreshold(bc->getSplitThreshold());

  bool bTakeDepthFromInputWorkspace =
      getProperty("TakeMaxRecursionDepthFromInput");
  int tempDepth = getProperty("MaxRecursionDepth");
  size_t maxDepth =
      bTakeDepthFromInputWorkspace ? bc->getMaxDepth() : size_t(tempDepth);
  obc->setMaxDepth(maxDepth);

  // size_t outputSize = writeBufSize;
  // obc->setCacheParameters(sizeof(OMDE),outputSize);

  obc->resetNumBoxes();
  // Perform the first box splitting
  outWS->splitBox();
  size_t lastNumBoxes = obc->getTotalNumMDBoxes();

  // --- File back end ? ----------------
  std::string filename = getProperty("OutputFilename");
  if (!filename.empty()) {

    // First save to the NXS file
    g_log.notice() << "Running SaveMD to create file back-end\n";
    IAlgorithm_sptr alg = createChildAlgorithm("SaveMD");
    alg->setPropertyValue("Filename", filename);
    alg->setProperty("InputWorkspace", outWS);
    alg->setProperty("MakeFileBacked", true);
    alg->executeAsChildAlg();

    if (!obc->isFileBacked())
      throw std::runtime_error("SliceMD with file-backed output: Can not set "
                               "up file-backed output workspace ");

    auto IOptr = obc->getFileIO();
    size_t outBufSize = IOptr->getWriteBufferSize();
    // the buffer size for resulting workspace; reasonable size is at least 10
    // data chunk sizes (nice to verify)
    if (outBufSize < 10 * IOptr->getDataChunk()) {
      outBufSize = 10 * IOptr->getDataChunk();
      IOptr->setWriteBufferSize(outBufSize);
    }
  }

  // Function defining which events (in the input dimensions) to place in the
  // output
  MDImplicitFunction *function =
      this->getImplicitFunctionForChunk(nullptr, nullptr);

  std::vector<API::IMDNode *> boxes;
  // Leaf-only; no depth limit; with the implicit function passed to it.
  ws->getBox()->getBoxes(boxes, 1000, true, function);
  // Sort boxes by file position IF file backed. This reduces seeking time,
  // hopefully.
  bool fileBackedWS = bc->isFileBacked();
  if (fileBackedWS)
    API::IMDNode::sortObjByID(boxes);

  auto prog = make_unique<Progress>(this, 0.0, 1.0, boxes.size());

  // The root of the output workspace
  MDBoxBase<OMDE, ond> *outRootBox = outWS->getBox();

  // if target workspace has events, we should count them as added
  uint64_t totalAdded = outWS->getNEvents();
  uint64_t numSinceSplit = 0;

  // Go through every box for this chunk.
  // PARALLEL_FOR_IF( !bc->isFileBacked() )
  for (int i = 0; i < int(boxes.size()); i++) {
    MDBox<MDE, nd> *box = dynamic_cast<MDBox<MDE, nd> *>(boxes[i]);
    // Perform the binning in this separate method.
    if (box && !box->getIsMasked()) {
      // An array to hold the rotated/transformed coordinates
      coord_t outCenter[ond];

      const std::vector<MDE> &events = box->getConstEvents();

      for (auto it = events.cbegin(); it != events.cend(); ++it) {
        // Cache the center of the event (again for speed)
        const coord_t *inCenter = it->getCenter();

        if (function->isPointContained(inCenter)) {
          // Now transform to the output dimensions
          m_transformFromOriginal->apply(inCenter, outCenter);

          // Create the event
          OMDE newEvent(it->getSignal(), it->getErrorSquared(), outCenter);
          // Copy extra data, if any
          copyEvent(*it, newEvent);
          // Add it to the workspace
          if (outRootBox->addEvent(newEvent))
            numSinceSplit++;
        }
      }
      box->releaseEvents();

      // Ask BC if one needs to split boxes
      if (obc->shouldSplitBoxes(totalAdded, numSinceSplit, lastNumBoxes))
      // if (numSinceSplit > 20000000 || (i == int(boxes.size()-1)))
      {
        // This splits up all the boxes according to split thresholds and sizes.
        Kernel::ThreadScheduler *ts = new ThreadSchedulerFIFO();
        ThreadPool tp(ts);
        outWS->splitAllIfNeeded(ts);
        tp.joinAll();
        // Accumulate stats
        totalAdded += numSinceSplit;
        numSinceSplit = 0;
        lastNumBoxes = obc->getTotalNumMDBoxes();
        // Progress reporting
        if (!fileBackedWS)
          prog->report(i);
      }
      if (fileBackedWS) {
        if (!(i % 10))
          prog->report(i);
      }
    } // is box

  } // for each box in the vector
  prog->report();

  outWS->splitAllIfNeeded(nullptr);
  // Refresh all cache.
  outWS->refreshCache();

  // Account for events that were added after the last split
  totalAdded += numSinceSplit;
  g_log.notice() << totalAdded << " " << OMDE::getTypeName()
                 << "s added to the output workspace.\n";

  if (outWS->isFileBacked()) {
    // Update the file-back-end
    g_log.notice() << "Running SaveMD\n";
    IAlgorithm_sptr alg = createChildAlgorithm("SaveMD");
    alg->setProperty("UpdateFileBackEnd", true);
    alg->setProperty("InputWorkspace", outWS);
    alg->executeAsChildAlg();
  }

  try {
    outWS->copyExperimentInfos(*ws);
  } catch (std::runtime_error &) {
    g_log.warning()
        << this->name()
        << " was not able to copy experiment info to output workspace "
        << outWS->getName() << '\n';
  }

  // Pass on the display normalization from the input event workspace to the
  // output event workspace
  IMDEventWorkspace_sptr outEvent =
      boost::dynamic_pointer_cast<IMDEventWorkspace>(outWS);
  outEvent->setDisplayNormalization(ws->displayNormalization());
  outEvent->setDisplayNormalizationHisto(ws->displayNormalizationHisto());
  // return the size of the input workspace write buffer to its initial value
  // bc->setCacheParameters(sizeof(MDE),writeBufSize);
  this->setProperty("OutputWorkspace", outEvent);
}

//----------------------------------------------------------------------------------------------
/// Helper method
template <typename MDE, size_t nd>
void SliceMD::doExec(typename MDEventWorkspace<MDE, nd>::sptr ws) {
  if (m_outD == 0)
    throw std::runtime_error("No output dimensions specified!");

  // Templated method needs to call another templated method depending on the #
  // of output dimensions.
  if (MDE::getTypeName() == "MDLeanEvent") {
    if (m_outD == 1)
      this->slice<MDE, nd, MDLeanEvent<1>, 1>(ws);
    else if (m_outD == 2)
      this->slice<MDE, nd, MDLeanEvent<2>, 2>(ws);
    else if (m_outD == 3)
      this->slice<MDE, nd, MDLeanEvent<3>, 3>(ws);
    else if (m_outD == 4)
      this->slice<MDE, nd, MDLeanEvent<4>, 4>(ws);
    else
      throw std::runtime_error(
          "Number of output dimensions > 4. This is not currently handled.");
  } else if (MDE::getTypeName() == "MDEvent") {
    if (m_outD == 1)
      this->slice<MDE, nd, MDEvent<1>, 1>(ws);
    else if (m_outD == 2)
      this->slice<MDE, nd, MDEvent<2>, 2>(ws);
    else if (m_outD == 3)
      this->slice<MDE, nd, MDEvent<3>, 3>(ws);
    else if (m_outD == 4)
      this->slice<MDE, nd, MDEvent<4>, 4>(ws);
    else
      throw std::runtime_error(
          "Number of output dimensions > 4. This is not currently handled.");
  } else
    throw std::runtime_error("Unexpected MDEvent type '" + MDE::getTypeName() +
                             "'. This is not currently handled.");
}

//----------------------------------------------------------------------------------------------
/** Execute the algorithm.
 */
void SliceMD::exec() {
  // Input MDEventWorkspace
  m_inWS = getProperty("InputWorkspace");

  // Run through the properties to create the transform you need
  createTransform();

  CALL_MDEVENT_FUNCTION(this->doExec, m_inWS);
}

} // namespace MDAlgorithms
} // namespace Mantid
