// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidMDAlgorithms/ApplyDetailedBalanceMD.h"
#include "MantidDataObjects/MDBoxIterator.h"
#include "MantidDataObjects/MDEventFactory.h"
// #include "MantidAPI/IMDEventWorkspace.h"
#include "MantidAPI/MDGeometry.h"
#include "MantidGeometry/MDGeometry/IMDDimension.h"

using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace Mantid::Geometry;
using namespace Mantid::DataObjects;

namespace Mantid {
namespace MDAlgorithms {

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(ApplyDetailedBalanceMD)

//----------------------------------------------------------------------------------------------
/// Algorithm's name for identification. @see Algorithm::name
const std::string ApplyDetailedBalanceMD::name() const { return "ApplyDetailedBalanceMD"; }

/// Algorithm's version for identification. @see Algorithm::version
int ApplyDetailedBalanceMD::version() const { return 1; }

/// Summary
const std::string ApplyDetailedBalanceMD::summary() const { return "Apply detailed balance to MDEventWorkspace"; }

/// category
const std::string ApplyDetailedBalanceMD::category() const { return "MDAlgorithms"; }

void ApplyDetailedBalanceMD::init() {

  declareProperty(
      std::make_unique<WorkspaceProperty<API::IMDEventWorkspace>>("InputWorkspace", "", Kernel::Direction::Input),
      "An input MDEventWorkspace.  Must be in Q_sample/Q_lab frame.  Must have an axis as DeltaE");

  declareProperty(std::make_unique<PropertyWithValue<std::string>>("Temperature", "", Direction::Input),
                  "SampleLog variable name that contains the temperature");
  declareProperty(
      std::make_unique<WorkspaceProperty<API::IMDEventWorkspace>>("OutputWorkspace", "", Kernel::Direction::Output),
      "The output MDEventWorkspace with detailed balance applied");
}

void ApplyDetailedBalanceMD::exec() {
  // Process inputs
  // - input workspace
  API::IMDEventWorkspace_sptr input_ws = getProperty("InputWorkspace");
  if (input_ws == nullptr) {
    g_log.notice() << "Null pointer\n";
    return;
  }
  std::string output_ws_name = getPropertyValue("OutputWorkspace");

  API::IMDEventWorkspace_sptr output_ws(0);
  if (input_ws->getName() == output_ws_name) {
    g_log.notice() << "[DEVELOP] Calcualte in-place"
                   << "\n";
    output_ws = input_ws;
  } else {
    g_log.notice() << "[DEVELOP] Clone input workace to output workspace"
                   << "\n";
    output_ws = input_ws->clone();
  }

  // Understand dimension
  size_t numdims = output_ws->getNumDims();
  g_log.notice() << "[DEVELOP] num dims = " << numdims << "\n";
  for (size_t i = 0; i < numdims; ++i) {
    auto dim_i = output_ws->getDimension(i);
    std::string dim_name = dim_i->getName();
    g_log.notice() << "[DEVELOP] " << i << "-th dim: " << dim_name << "\n";
  }

  // Work with MDEvents
  CALL_MDEVENT_FUNCTION(applyDetailedBalance, output_ws);

  // Set output
  setProperty("OutputWorkspace", input_ws);
}

std::map<std::string, std::string> ApplyDetailedBalanceMD::validateInputs() {
  std::map<std::string, std::string> output;

  return output;
}

template <typename MDE, size_t nd>
void ApplyDetailedBalanceMD::applyDetailedBalance(typename Mantid::DataObjects::MDEventWorkspace<MDE, nd>::sptr ws) {
  // toy around
  g_log.notice() << "[DEVELOP] " << ws->getName() << "\n";
  uint64_t numevents = ws->getNEvents();
  g_log.notice() << "[DEVELOP] number of events = " << numevents << "\n";

  // Get Box from MDEventWorkspace
  MDBoxBase<MDE, nd> *box1 = ws->getBox();
  std::vector<API::IMDNode *> boxes;
  box1->getBoxes(boxes, 1000, true);
  auto numBoxes = int(boxes.size());

  g_log.notice() << numBoxes << "\n";

  // auto num_boxes =

  // FIXME PRAGMA_OMP
  size_t count = 0;
  for (int i = 0; i < numBoxes; ++i) {
    // FIXME PARALLEL_START_INTERUPT_REGION
    auto *box = dynamic_cast<MDBox<MDE, nd> *>(boxes[i]);
    if (box && !box->getIsMasked()) {
      // Copy the events from WS2 and add them into WS1
      std::vector<MDE> &events = box->getEvents();
      // Add events, with bounds checking
      for (auto it = events.begin(); it != events.end(); ++it) {
        // Create the event
        // MDE newEvent(it->getSignal(), it->getErrorSquared(), it->getCenter());
        // std::cout << it->getSignal() << "\n";
        count += 1;
        it->setSignal(10.12);
      }
    }
    // FIXME PARALLEL_END_INTERUPT_REGION
  }
  // FIXME PARALLEL_CHECK_INTERUPT_REGION
  g_log.notice() << "[DEVELOP] traverse  events = " << count << "\n";

  for (int i = 0; i < numBoxes; ++i) {
    // FIXME PARALLEL_START_INTERUPT_REGION
    auto *box = dynamic_cast<MDBox<MDE, nd> *>(boxes[i]);
    if (box && !box->getIsMasked()) {
      // Copy the events from WS2 and add them into WS1
      std::vector<MDE> &events = box->getEvents();
      // Add events, with bounds checking
      for (auto it = events.begin(); it != events.end(); ++it) {
        // Create the event
        // MDE newEvent(it->getSignal(), it->getErrorSquared(), it->getCenter());
        // std::cout << it->getSignal() << "\n";
        count += 1;
        // get the experiment info
        const uint16_t exp_index(it->getExpInfoIndex());
        // get the coordiate
        auto deltae = it->getCenter(3);

        g_log.notice() << "[DEVELOP] signal = " << it->getSignal() << ", exp info index = " << exp_index
                       << " DeltaE = " << deltae << "\n";
      }
    }
    // FIXME PARALLEL_END_INTERUPT_REGION
  }

  return;
}

} // namespace MDAlgorithms
} // namespace Mantid
