// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidMDAlgorithms/MergeMD.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidDataObjects/MDBoxIterator.h"
#include "MantidDataObjects/MDEventFactory.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/CPUTimer.h"
#include "MantidKernel/MandatoryValidator.h"
#include "MantidKernel/Strings.h"

using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace Mantid::Geometry;
using namespace Mantid::DataObjects;

namespace Mantid {
namespace MDAlgorithms {

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(MergeMD)

/// Algorithm's name for identification. @see Algorithm::name
const std::string MergeMD::name() const { return "MergeMD"; }

/// Algorithm's version for identification. @see Algorithm::version
int MergeMD::version() const { return 1; }

/// Algorithm's category for identification. @see Algorithm::category
const std::string MergeMD::category() const { return "MDAlgorithms\\Creation"; }

/** Initialize the algorithm's properties.
 */
void MergeMD::init() {
  // declare arbitrary number of input m_workspaces as a list of strings at the
  // moment
  declareProperty(
      std::make_unique<ArrayProperty<std::string>>(
          "InputWorkspaces",
          boost::make_shared<MandatoryValidator<std::vector<std::string>>>()),
      "The names of the input MDWorkspaces as a comma-separated list");

  declareProperty(std::make_unique<WorkspaceProperty<IMDEventWorkspace>>(
                      "OutputWorkspace", "", Direction::Output),
                  "Name of the output MDWorkspace.");

  // Set the box controller properties
  this->initBoxControllerProps("2", 500, 16);
}

/** Create the output MDWorkspace from a list of input
 *
 * @param inputs :: list of names of input MDWorkspaces
 */
void MergeMD::createOutputWorkspace(std::vector<std::string> &inputs) {
  auto it = inputs.begin();
  for (; it != inputs.end(); it++) {
    IMDEventWorkspace_sptr ws = boost::dynamic_pointer_cast<IMDEventWorkspace>(
        AnalysisDataService::Instance().retrieve(*it));
    if (!ws)
      throw std::invalid_argument(
          "Workspace " + *it +
          " is not a MDEventWorkspace. Cannot merge this workspace.");
    else
      m_workspaces.push_back(ws);
  }
  if (m_workspaces.empty())
    throw std::invalid_argument("No valid m_workspaces specified.");

  // Number of dimensions, start with the 0th one. They all have to match # of
  // dims anyway
  IMDEventWorkspace_const_sptr ws0 = m_workspaces[0];
  size_t numDims = ws0->getNumDims();
  auto displNorm = ws0->displayNormalization();
  auto displNormH = ws0->displayNormalizationHisto();

  // Extents to create.
  std::vector<coord_t> dimMin(numDims, +1e30f);
  std::vector<coord_t> dimMax(numDims, -1e30f);

  // Validate each workspace
  for (auto &ws : m_workspaces) {
    if (ws->getNumDims() != numDims)
      throw std::invalid_argument(
          "Workspace " + ws->getName() +
          " does not match the number of dimensions of the others (" +
          Strings::toString(ws->getNumDims()) + ", expected " +
          Strings::toString(numDims) + ")");

    if (ws->getEventTypeName() != ws0->getEventTypeName())
      throw std::invalid_argument(
          "Workspace " + ws->getName() +
          " does not match the MDEvent type of the others (" +
          ws->getEventTypeName() + ", expected " + ws0->getEventTypeName() +
          ")");

    for (size_t d = 0; d < numDims; d++) {
      IMDDimension_const_sptr dim = ws->getDimension(d);
      IMDDimension_const_sptr dim0 = ws0->getDimension(d);
      if (dim->getName() != dim0->getName())
        throw std::invalid_argument("Workspace " + ws->getName() +
                                    " does not have the same dimension " +
                                    Strings::toString(d) + " as the others (" +
                                    dim->getName() + ", expected " +
                                    dim0->getName() + ")");

      // Find the extents
      if (dim->getMaximum() > dimMax[d])
        dimMax[d] = dim->getMaximum();
      if (dim->getMinimum() < dimMin[d])
        dimMin[d] = dim->getMinimum();
    }
  }

  // OK, now create the blank MDWorkspace

  // Have the factory create it
  out = MDEventFactory::CreateMDWorkspace(numDims, ws0->getEventTypeName());
  out->setDisplayNormalization(displNorm);
  out->setDisplayNormalizationHisto(displNormH);

  // Give all the dimensions
  for (size_t d = 0; d < numDims; d++) {
    IMDDimension_const_sptr dim0 = ws0->getDimension(d);
    MDHistoDimension *dim = new MDHistoDimension(
        dim0->getName(), dim0->getDimensionId(), dim0->getMDFrame(), dimMin[d],
        dimMax[d], dim0->getNBins());
    out->addDimension(MDHistoDimension_sptr(dim));
  }

  // Initialize it using the dimension
  out->initialize();

  // Set the box controller settings from the properties
  this->setBoxController(out->getBoxController());

  // Perform the initial box splitting
  out->splitBox();

  // copy experiment infos
  uint16_t nExperiments(0);
  if (m_workspaces.size() > std::numeric_limits<uint16_t>::max())
    throw std::invalid_argument(
        "currently we can not combine more then 65535 experiments");
  else
    nExperiments = static_cast<uint16_t>(m_workspaces.size());

  for (uint16_t i = 0; i < nExperiments; i++) {
    uint16_t nWSexperiments = m_workspaces[i]->getNumExperimentInfo();
    experimentInfoNo.push_back(nWSexperiments);
    for (uint16_t j = 0; j < nWSexperiments; j++) {
      API::ExperimentInfo_sptr ei = API::ExperimentInfo_sptr(
          m_workspaces[i]->getExperimentInfo(j)->cloneExperimentInfo());
      out->addExperimentInfo(ei);
    }
  }

  // Cumulative sum of number of experimentInfo and reverse order
  std::partial_sum(experimentInfoNo.begin(), experimentInfoNo.end(),
                   experimentInfoNo.begin());
  std::reverse(std::begin(experimentInfoNo), std::end(experimentInfoNo));
}

//----------------------------------------------------------------------------------------------
/** Copy the extra data (not signal, error or coordinates) from one event to
 * another with different numbers of dimensions
 *
 * @param srcEvent :: the source event, being copied
 * @param newEvent :: the destination event
 * @param runIndexOffset :: offset to be added to the runIndex
 */
template <size_t nd, size_t ond>
inline void copyEvent(const MDLeanEvent<nd> &srcEvent,
                      MDLeanEvent<ond> &newEvent,
                      const uint16_t runIndexOffset) {
  // Nothing extra copy - this is no-op
  UNUSED_ARG(srcEvent);
  UNUSED_ARG(newEvent);
  UNUSED_ARG(runIndexOffset);
}

//----------------------------------------------------------------------------------------------
/** Copy the extra data (not signal, error or coordinates) from one event to
 * another with different numbers of dimensions
 *
 * @param srcEvent :: the source event, being copied
 * @param newEvent :: the destination event
 * @param runIndexOffset :: offset to be added to the runIndex
 */
template <size_t nd, size_t ond>
inline void copyEvent(const MDEvent<nd> &srcEvent, MDEvent<ond> &newEvent,
                      const uint16_t runIndexOffset) {
  newEvent.setDetectorId(srcEvent.getDetectorID());
  newEvent.setRunIndex(
      static_cast<uint16_t>(srcEvent.getRunIndex() + runIndexOffset));
}

//----------------------------------------------------------------------------------------------
/** Perform the adding.
 * Will do out += ws
 *
 * @param ws ::  MDEventWorkspace to clone
 */
template <typename MDE, size_t nd>
void MergeMD::doPlus(typename MDEventWorkspace<MDE, nd>::sptr ws2) {
  // CPUTimer tim;
  typename MDEventWorkspace<MDE, nd>::sptr ws1 =
      boost::dynamic_pointer_cast<MDEventWorkspace<MDE, nd>>(out);
  if (!ws1 || !ws2)
    throw std::runtime_error("Incompatible workspace types passed to MergeMD.");

  MDBoxBase<MDE, nd> *box1 = ws1->getBox();
  MDBoxBase<MDE, nd> *box2 = ws2->getBox();

  uint16_t runIndexOffset = experimentInfoNo.back();
  experimentInfoNo.pop_back();

  // How many events you started with
  size_t initial_numEvents = ws1->getNPoints();

  // Make a leaf-only iterator through all boxes with events in the RHS
  // workspace
  std::vector<API::IMDNode *> boxes;
  box2->getBoxes(boxes, 1000, true);
  int numBoxes = int(boxes.size());

  bool fileBasedSource(false);
  if (ws2->isFileBacked())
    fileBasedSource = true;

  // Add the boxes in parallel. They should be spread out enough on each
  // core to avoid stepping on each other.
  // cppcheck-suppress syntaxError
    PRAGMA_OMP( parallel for if (!ws2->isFileBacked()) )
    for (int i = 0; i < numBoxes; i++) {
      PARALLEL_START_INTERUPT_REGION
      MDBox<MDE, nd> *box = dynamic_cast<MDBox<MDE, nd> *>(boxes[i]);
      if (box && !box->getIsMasked()) {
        // Copy the events from WS2 and add them into WS1
        const std::vector<MDE> &events = box->getConstEvents();
        // Add events, with bounds checking

        for (auto it = events.cbegin(); it != events.cend(); ++it) {
          // Create the event
          MDE newEvent(it->getSignal(), it->getErrorSquared(), it->getCenter());
          // Copy extra data, if any
          copyEvent(*it, newEvent, runIndexOffset);
          // Add it to the workspace
          box1->addEvent(newEvent);
        }
        if (fileBasedSource)
          box->clear();
        else
          box->releaseEvents();
      }
      PARALLEL_END_INTERUPT_REGION
    }
    PARALLEL_CHECK_INTERUPT_REGION

    // Progress * prog2 = new Progress(this, 0.4, 0.9, 100);
    Progress *prog2 = nullptr;
    ThreadScheduler *ts = new ThreadSchedulerFIFO();
    ThreadPool tp(ts, 0, prog2);
    ws1->splitAllIfNeeded(ts);
    // prog2->resetNumSteps( ts->size(), 0.4, 0.6);
    tp.joinAll();

    // Set a marker that the file-back-end needs updating if the # of events
    // changed.
    if (ws1->getNPoints() != initial_numEvents)
      ws1->setFileNeedsUpdating(true);
    //
    // std::cout << tim << " to add workspace " << ws2->name() << '\n';
}

//----------------------------------------------------------------------------------------------
/** Execute the algorithm.
 */
void MergeMD::exec() {
  CPUTimer tim;
  // Check that all input workspaces exist and match in certain important ways
  const std::vector<std::string> inputs_orig = getProperty("InputWorkspaces");

  // This will hold the inputs, with the groups separated off
  std::vector<std::string> inputs;
  for (const auto &input : inputs_orig) {
    WorkspaceGroup_sptr wsgroup =
        AnalysisDataService::Instance().retrieveWS<WorkspaceGroup>(input);
    if (wsgroup) { // Workspace group
      std::vector<std::string> group = wsgroup->getNames();
      inputs.insert(inputs.end(), group.begin(), group.end());
    } else {
      // Single workspace
      inputs.push_back(input);
    }
  }

  if (inputs.size() == 1) {
    g_log.error("Only one input workspace specified");
    throw std::invalid_argument("Only one input workspace specified");
  }

  // Create a blank output workspace
  this->createOutputWorkspace(inputs);

  // Run PlusMD on each of the input workspaces, in order.
  double progStep = 1.0 / double(m_workspaces.size());
  for (size_t i = 0; i < m_workspaces.size(); i++) {
    g_log.information() << "Adding workspace " << m_workspaces[i]->getName()
                        << '\n';
    progress(double(i) * progStep, m_workspaces[i]->getName());
    CALL_MDEVENT_FUNCTION(doPlus, m_workspaces[i]);
  }

  this->progress(0.95, "Refreshing cache");
  out->refreshCache();

  this->setProperty("OutputWorkspace", out);

  g_log.debug() << tim << " to merge all workspaces.\n";
}

} // namespace MDAlgorithms
} // namespace Mantid
