// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2025 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +

#include "MantidMDAlgorithms/SpectralMomentMD.h"
#include "MantidDataObjects/MDBoxIterator.h"
#include "MantidDataObjects/MDEventFactory.h"
#include "MantidKernel/PropertyWithValue.h"
#include <cmath>

namespace Mantid {
namespace MDAlgorithms {
using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace Mantid::DataObjects;
// using Mantid::API::WorkspaceProperty;
// using Mantid::Kernel::Direction;

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(SpectralMomentMD)

//----------------------------------------------------------------------------------------------

/// Algorithms name for identification. @see Algorithm::name
const std::string SpectralMomentMD::name() const { return "SpectralMomentMD"; }

/// Algorithm's version for identification. @see Algorithm::version
int SpectralMomentMD::version() const { return 1; }

/// Algorithm's category for identification. @see Algorithm::category
const std::string SpectralMomentMD::category() const { return "MDAlgorithms"; }

/// Algorithm's summary for use in the GUI and help. @see Algorithm::summary
const std::string SpectralMomentMD::summary() const { return "Multiply MD events by DeltaE^n"; }

//----------------------------------------------------------------------------------------------
/** Initialize the algorithm's properties.
 */
void SpectralMomentMD::init() {
  declareProperty(
      std::make_unique<WorkspaceProperty<API::IMDEventWorkspace>>("InputWorkspace", "", Kernel::Direction::Input),
      "An input MDEventWorkspace. Must have an axis as DeltaE");

  declareProperty(std::make_unique<PropertyWithValue<int>>("Moment", 1),
                  "The integer exponent of energy transfer (default: 1)");
  declareProperty(
      std::make_unique<WorkspaceProperty<API::IMDEventWorkspace>>("OutputWorkspace", "", Kernel::Direction::Output),
      "The output MDEventWorkspace with events scaled by DeltaE^Moment");
}

//---------------------------------------------------------------------------------------------------------
/**
 * @brief Validate inputs
 * Input MDEventWorkspace - one of the dimensions must be DeltaE
 */
std::map<std::string, std::string> SpectralMomentMD::validateInputs() {
  std::map<std::string, std::string> output;

  // Get input workspace
  API::IMDEventWorkspace_sptr input_ws = getProperty("InputWorkspace");

  // initialize error string
  std::string error_str("No dimension containing energy transfer was found.");

  // loop over dimensions
  for (std::size_t dim_num = 0; dim_num < input_ws->getNumDims(); dim_num++) {
    if (input_ws->getDimension(dim_num)->getName() == "DeltaE") {
      error_str = "";
      // cache DeltaE index
      mDeltaEIndex = dim_num;
    }
  }

  if (error_str != "") {
    output["InputWorkspace"] = error_str;
  }

  return output;
}

//----------------------------------------------------------------------------------------------
/** Execute the algorithm.
 */
void SpectralMomentMD::exec() {
  // Get input workspace
  API::IMDEventWorkspace_sptr input_ws = getProperty("InputWorkspace");

  // Process input workspace and create output workspace
  std::string output_ws_name = getPropertyValue("OutputWorkspace");

  API::IMDEventWorkspace_sptr output_ws(0);
  if (input_ws->getName() == output_ws_name) {
    // Calcualte in-place
    output_ws = input_ws;
  } else {
    // Clone input workace to output workspace
    output_ws = input_ws->clone();
  }

  // Cache moment
  mExponent = getProperty("Moment");

  g_log.debug() << "Moment: " << mExponent << " DeltaE index: " << mDeltaEIndex << "\n";

  // Apply scaling to MDEvents
  CALL_MDEVENT_FUNCTION(applyScaling, output_ws);

  // refresh cache for MDBoxes: set correct Box signal
  output_ws->refreshCache();

  // Clear masking (box flags) from the output workspace
  output_ws->clearMDMasking();

  // Set output
  setProperty("OutputWorkspace", output_ws);
}

//---------------------------------------------------------------------------------------------
/**
 * @brief Apply scaling to each MDEvent in MDEventWorkspace
 */
template <typename MDE, size_t nd>
void SpectralMomentMD::applyScaling(typename Mantid::DataObjects::MDEventWorkspace<MDE, nd>::sptr ws) {
  // Get Box from MDEventWorkspace
  MDBoxBase<MDE, nd> *box1 = ws->getBox();
  std::vector<API::IMDNode *> boxes;
  box1->getBoxes(boxes, 1000, true);
  auto numBoxes = int(boxes.size());

  PRAGMA_OMP( parallel for if (!ws->isFileBacked()))
  for (int i = 0; i < numBoxes; ++i) {
    PARALLEL_START_INTERRUPT_REGION
    auto *box = dynamic_cast<MDBox<MDE, nd> *>(boxes[i]);
    if (box && !box->getIsMasked()) {
      // get the MEEvents from box
      std::vector<MDE> &events = box->getEvents();
      // Add events, with bounds checking
      for (auto it = events.begin(); it != events.end(); ++it) {
        // Create the event
        // do calculattion
        float factor = static_cast<float>(std::pow(it->getCenter(mDeltaEIndex), mExponent));

        // calcalate and set intesity
        auto intensity = it->getSignal() * factor;
        it->setSignal(intensity);

        // calculate and set error
        auto error2 = it->getErrorSquared() * factor * factor;
        // error2 *= factor * factor;
        it->setErrorSquared(error2);
      }
    }
    if (box) {
      box->releaseEvents();
    }
    PARALLEL_END_INTERRUPT_REGION
  }
  PARALLEL_CHECK_INTERRUPT_REGION

  return;
}

} // namespace MDAlgorithms
} // namespace Mantid
