// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidMDAlgorithms/PlusMD.h"
#include "MantidAPI/IMDEventWorkspace.h"
#include "MantidDataObjects/MDBoxBase.h"
#include "MantidDataObjects/MDBoxIterator.h"
#include "MantidDataObjects/MDEventFactory.h"
#include "MantidKernel/ThreadPool.h"
#include "MantidKernel/ThreadScheduler.h"

using namespace Mantid::Kernel;
using namespace Mantid::DataObjects;
using namespace Mantid::API;

namespace Mantid::MDAlgorithms {

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(PlusMD)

//----------------------------------------------------------------------------------------------
/** Perform the adding.
 *
 * Will do m_out_event += m_operand_event
 *
 * @param ws ::  MDEventWorkspace being added to
 */
template <typename MDE, size_t nd> void PlusMD::doPlus(typename MDEventWorkspace<MDE, nd>::sptr ws1) {
  typename MDEventWorkspace<MDE, nd>::sptr ws2 = std::dynamic_pointer_cast<MDEventWorkspace<MDE, nd>>(m_operand_event);
  if (!ws1 || !ws2)
    throw std::runtime_error("Incompatible workspace types passed to PlusMD.");

  MDBoxBase<MDE, nd> *box1 = ws1->getBox();
  MDBoxBase<MDE, nd> *box2 = ws2->getBox();

  Progress prog(this, 0.0, 0.4, box2->getBoxController()->getTotalNumMDBoxes());

  // How many events you started with
  size_t initial_numEvents = ws1->getNPoints();

  // Make a leaf-only iterator through all boxes with events in the RHS
  // workspace
  // TODO:    OMP
  MDBoxIterator<MDE, nd> it2(box2, 1000, true);
  do {
    auto *box = dynamic_cast<MDBox<MDE, nd> *>(it2.getBox());
    if (box && !box->getIsMasked()) {
      // Copy the events from WS2 and add them into WS1
      const std::vector<MDE> &events = box->getConstEvents();
      // Add events, with bounds checking
      box1->addEvents(events);
      box->releaseEvents();
    }
    prog.report("Adding Events");
  } while (it2.next());

  this->progress(0.41, "Splitting Boxes");
  // This is freed in the destructor of the ThreadPool class,
  // it should not be a memory leak
  auto prog2 = new Progress(this, 0.4, 0.9, 100);
  ThreadScheduler *ts = new ThreadSchedulerFIFO();
  ThreadPool tp(ts, 0, prog2);
  ws1->splitAllIfNeeded(ts);
  prog2->resetNumSteps(ts->size(), 0.4, 0.6);
  tp.joinAll();

  //// Now we need to save all the data that was not saved before.
  // if (ws1->isFileBacked())
  //{
  //    // flusush disk kernel buffer and save all still in memory
  //    ws1->getBoxController()->getFileIO()->flushCache();
  //  // Flush the data writes to disk from nexus IO buffer
  //    ws1->getBoxController()->getFileIO()->flushData();
  //}
  // if(ws2->isFileBacked())
  //{
  //    // flusush disk kernel buffer and save all still in memory
  //    ws2->getBoxController()->getFileIO()->flushCache();
  //  // Flush the data writes to disk from nexus IO buffer
  //    ws2->getBoxController()->getFileIO()->flushData();

  //  //// Flush anything else in the to-write buffer
  //  //BoxController_sptr bc = ws1->getBoxController();

  //  //prog.resetNumSteps(bc->getTotalNumMDBoxes(), 0.6, 1.0);
  //  //MDBoxIterator<MDE,nd> it1(box1, 1000, true);
  //  //while (true)
  //  //{
  //  //  MDBox<MDE,nd> * box = dynamic_cast<MDBox<MDE,nd> *>(it1.getBox());
  //  //  if (box)
  //  //  {
  //  //    // Something was maybe added to this box
  //  //    if (box->getEventVectorSize() > 0)
  //  //    {
  //  //      // By getting the events, this will merge the newly added and the
  //  cached events.
  //  //      box->getEvents();
  //  //      // The MRU to-write cache will optimize writes by reducing seek
  //  times
  //  //      box->releaseEvents();
  //  //    }
  //  //  }
  //  //  prog.report("Saving");
  //  //  if (!it1.next()) break;
  //  //}
  //  //bc->getDiskBuffer().flushCache();

  //}

  this->progress(0.95, "Refreshing cache");
  ws1->refreshCache();

  // Set a marker that the file-back-end needs updating if the # of events
  // changed.
  if (ws1->getNPoints() != initial_numEvents)
    ws1->setFileNeedsUpdating(true);
}

//----------------------------------------------------------------------------------------------
/// Is the operation commutative?
bool PlusMD::commutative() const { return true; }

//----------------------------------------------------------------------------------------------
/// Check the inputs and throw if the algorithm cannot be run
void PlusMD::checkInputs() {
  if (m_lhs_event || m_rhs_event) {
    if (m_lhs_histo || m_rhs_histo)
      throw std::runtime_error("Cannot sum a MDHistoWorkspace and a "
                               "MDEventWorkspace (only MDEventWorkspace + "
                               "MDEventWorkspace is allowed).");
    if (m_lhs_scalar || m_rhs_scalar)
      throw std::runtime_error("Cannot sum a MDEventWorkspace and a scalar "
                               "(only MDEventWorkspace + MDEventWorkspace is "
                               "allowed).");
  }
}

//----------------------------------------------------------------------------------------------
/// Run the algorithm with a MDHisotWorkspace as output and operand
void PlusMD::execHistoHisto(Mantid::DataObjects::MDHistoWorkspace_sptr out,
                            Mantid::DataObjects::MDHistoWorkspace_const_sptr operand) {
  out->add(*operand);
}

//----------------------------------------------------------------------------------------------
/// Run the algorithm with a MDHisotWorkspace as output, scalar and operand
void PlusMD::execHistoScalar(Mantid::DataObjects::MDHistoWorkspace_sptr out,
                             Mantid::DataObjects::WorkspaceSingleValue_const_sptr scalar) {
  out->add(scalar->y(0)[0], scalar->e(0)[0]);
}

//----------------------------------------------------------------------------------------------
/** Execute the algorithm with an MDEventWorkspace as output
 */
void PlusMD::execEvent() {
  // Now we add m_operand_event into m_out_event.
  CALL_MDEVENT_FUNCTION(this->doPlus, m_out_event);

  // Clear masking (box flags) from the output workspace
  m_out_event->clearMDMasking();

  // Set to the output
  setProperty("OutputWorkspace", m_out_event);
}

} // namespace Mantid::MDAlgorithms
