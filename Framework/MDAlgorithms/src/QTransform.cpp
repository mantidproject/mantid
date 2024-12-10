// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2024 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +

#include "MantidMDAlgorithms/QTransform.h"
#include "MantidAPI/IMDEventWorkspace_fwd.h"
#include "MantidDataObjects/MDEventFactory.h"
#include "MantidKernel/SpecialCoordinateSystem.h"
#include "MantidMDAlgorithms/MDWSDescription.h"

using namespace Mantid::DataObjects;

namespace Mantid {
namespace MDAlgorithms {
using Mantid::API::IMDEventWorkspace;
using Mantid::API::IMDEventWorkspace_sptr;
using Mantid::API::WorkspaceProperty;
using Mantid::Kernel::Direction;

//----------------------------------------------------------------------------------------------
namespace { // anonymous
size_t getNumQDimensions(const IMDEventWorkspace_sptr ws) {
  // Check that the input workspace has units in Q_sample or Q_lab or |Q| frame
  // Check the assumption that the Q dimensions are the first
  if (ws->getSpecialCoordinateSystem() == Mantid::Kernel::SpecialCoordinateSystem::QSample ||
      ws->getSpecialCoordinateSystem() == Mantid::Kernel::SpecialCoordinateSystem::QLab) {
    // check that the first 3 dimensions are in Q
    if (ws->getNumDims() < 3)
      return 0;

    for (size_t i = 0; i < 3; ++i)
      if (!ws->getDimension(i)->getMDFrame().isQ())
        return 0;

    return 3;
  } else if (ws->getDimension(0)->getName() == "|Q|") {
    return 1;
  }

  return 0;
}
} // namespace
//----------------------------------------------------------------------------------------------
/** Initialize the algorithm's properties.
 */
void QTransform::init() {
  declareProperty(std::make_unique<WorkspaceProperty<IMDEventWorkspace>>("InputWorkspace", "", Direction::Input),
                  "n input MDEventWorkspace. Must be in Q_sample/Q_lab frame.");
  declareProperty(std::make_unique<WorkspaceProperty<IMDEventWorkspace>>("OutputWorkspace", "", Direction::Output),
                  "The output MDEventWorkspace with correction applied.");
}

std::map<std::string, std::string> QTransform::validateInputs() {
  std::map<std::string, std::string> result;
  // check that the input workspace has units in Q_sample or Q_lab frame
  API::IMDEventWorkspace_sptr input_ws = getProperty("InputWorkspace");
  if (getNumQDimensions(input_ws) == 0)
    result["InputWorkspace"] = "Input workspace must be in Q_sample or Q_lab frame. Either Q3D or Q1D with |Q|.";

  return result;
}
//----------------------------------------------------------------------------------------------
/** Execute the algorithm.
 */
void QTransform::exec() {
  IMDEventWorkspace_sptr input_ws = getProperty("InputWorkspace");
  IMDEventWorkspace_sptr output_ws = getProperty("OutputWorkspace");

  if (output_ws != input_ws)
    output_ws = input_ws->clone();

  m_numQDims = getNumQDimensions(output_ws);

  CALL_MDEVENT_FUNCTION(applyCorrection, output_ws);

  // refresh cache for MDBoxes: set correct Box signal
  output_ws->refreshCache();

  setProperty("OutputWorkspace", output_ws);
}

template <typename MDE, size_t nd>
void QTransform::applyCorrection(typename Mantid::DataObjects::MDEventWorkspace<MDE, nd>::sptr ws) {
  // Get Box from MDEventWorkspace
  MDBoxBase<MDE, nd> *box1 = ws->getBox();
  std::vector<API::IMDNode *> boxes;
  box1->getBoxes(boxes, 1000, true);
  auto numBoxes = int(boxes.size());

  PRAGMA_OMP(parallel for if (!ws->isFileBacked()))
  for (int i = 0; i < numBoxes; ++i) {
    PARALLEL_START_INTERRUPT_REGION
    auto *box = dynamic_cast<MDBox<MDE, nd> *>(boxes[i]);
    if (box && !box->getIsMasked()) {
      std::vector<MDE> &events = box->getEvents();
      for (auto it = events.begin(); it != events.end(); ++it) {

        double qsqr{0};
        for (size_t d = 0; d < m_numQDims; ++d)
          qsqr += it->getCenter(d) * it->getCenter(d);

        const auto correction = this->correction(qsqr);

        // apply the correction
        const auto intensity = it->getSignal() * correction;
        it->setSignal(static_cast<float>(intensity));
        const auto error2 = it->getErrorSquared() * correction * correction;
        it->setErrorSquared(static_cast<float>(error2));
      }
    }
    if (box) {
      box->releaseEvents();
    }
    PARALLEL_END_INTERRUPT_REGION
  }

  return;
}

} // namespace MDAlgorithms
} // namespace Mantid
