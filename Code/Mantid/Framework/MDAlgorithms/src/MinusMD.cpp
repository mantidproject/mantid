#include "MantidMDAlgorithms/MinusMD.h"
#include "MantidKernel/System.h"
#include "MantidMDEvents/MDEventFactory.h"
#include "MantidMDEvents/MDEventWorkspace.h"
#include "MantidMDEvents/MDBoxIterator.h"
#include "MantidMDEvents/MDBox.h"

using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace Mantid::MDEvents;

namespace Mantid {
namespace MDAlgorithms {

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(MinusMD)

//----------------------------------------------------------------------------------------------
/** Constructor
 */
MinusMD::MinusMD() {}

//----------------------------------------------------------------------------------------------
/** Destructor
 */
MinusMD::~MinusMD() {}

//----------------------------------------------------------------------------------------------
/// Algorithm's name for identification. @see Algorithm::name
const std::string MinusMD::name() const { return "MinusMD"; }

/// Algorithm's version for identification. @see Algorithm::version
int MinusMD::version() const { return 1; }

//----------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------
/// Is the operation commutative?
bool MinusMD::commutative() const { return false; }

//----------------------------------------------------------------------------------------------
/// Check the inputs and throw if the algorithm cannot be run
void MinusMD::checkInputs() {
  if (m_lhs_event || m_rhs_event) {
    if (m_lhs_histo || m_rhs_histo)
      throw std::runtime_error("Cannot subtract a MDHistoWorkspace and a "
                               "MDEventWorkspace (only MDEventWorkspace - "
                               "MDEventWorkspace is allowed).");
    if (m_lhs_scalar || m_rhs_scalar)
      throw std::runtime_error("Cannot subtract a MDEventWorkspace and a "
                               "scalar (only MDEventWorkspace - "
                               "MDEventWorkspace is allowed).");
  }
}

//----------------------------------------------------------------------------------------------
/** Perform the subtraction.
 *
 * Will do m_out_event -= m_operand_event
 *
 * @param ws ::  MDEventWorkspace being added to
 */
template <typename MDE, size_t nd>
void MinusMD::doMinus(typename MDEventWorkspace<MDE, nd>::sptr ws) {
  typename MDEventWorkspace<MDE, nd>::sptr ws1 = ws;
  typename MDEventWorkspace<MDE, nd>::sptr ws2 =
      boost::dynamic_pointer_cast<MDEventWorkspace<MDE, nd>>(m_operand_event);
  if (!ws1 || !ws2)
    throw std::runtime_error("Incompatible workspace types passed to MinusMD.");

  MDBoxBase<MDE, nd> *box1 = ws1->getBox();
  MDBoxBase<MDE, nd> *box2 = ws2->getBox();

  Progress prog(this, 0.0, 0.4, box2->getBoxController()->getTotalNumMDBoxes());

  // How many events you started with
  size_t initial_numEvents = ws1->getNPoints();

  // Make a leaf-only iterator through all boxes with events in the RHS
  // workspace
  MDBoxIterator<MDE, nd> it2(box2, 1000, true);
  do {
    MDBox<MDE, nd> *box = dynamic_cast<MDBox<MDE, nd> *>(it2.getBox());
    if (box) {
      // Copy the events from WS2 and add them into WS1
      const std::vector<MDE> &events = box->getConstEvents();

      // Perform a copy while flipping the signal
      std::vector<MDE> eventsCopy;
      eventsCopy.reserve(events.size());
      for (auto it = events.begin(); it != events.end(); it++) {
        MDE eventCopy(*it);
        eventCopy.setSignal(-eventCopy.getSignal());
        eventsCopy.push_back(eventCopy);
      }
      // Add events, with bounds checking
      box1->addEvents(eventsCopy);
      box->releaseEvents();
    }
    prog.report("Substracting Events");
  } while (it2.next());

  this->progress(0.41, "Splitting Boxes");
  Progress *prog2 = new Progress(this, 0.4, 0.9, 100);
  ThreadScheduler *ts = new ThreadSchedulerFIFO();
  ThreadPool tp(ts, 0, prog2);
  ws1->splitAllIfNeeded(ts);
  prog2->resetNumSteps(ts->size(), 0.4, 0.6);
  tp.joinAll();

  this->progress(0.95, "Refreshing cache");
  ws1->refreshCache();

  // Set a marker that the file-back-end needs updating if the # of events
  // changed.
  if (ws1->getNPoints() != initial_numEvents)
    ws1->setFileNeedsUpdating(true);
}

//----------------------------------------------------------------------------------------------
/// Run the algorithm with an MDEventWorkspace as output
void MinusMD::execEvent() {
  // Now we add m_operand_event into m_out_event.
  CALL_MDEVENT_FUNCTION(this->doMinus, m_out_event);

  // Set to the output
  setProperty("OutputWorkspace", m_out_event);
}

//----------------------------------------------------------------------------------------------
/// Run the algorithm with a MDHisotWorkspace as output and operand
void
MinusMD::execHistoHisto(Mantid::MDEvents::MDHistoWorkspace_sptr out,
                        Mantid::MDEvents::MDHistoWorkspace_const_sptr operand) {
  out->subtract(*operand);
}

//----------------------------------------------------------------------------------------------
/// Run the algorithm with a MDHisotWorkspace as output, scalar and operand
void MinusMD::execHistoScalar(
    Mantid::MDEvents::MDHistoWorkspace_sptr out,
    Mantid::DataObjects::WorkspaceSingleValue_const_sptr scalar) {
  out->subtract(scalar->dataY(0)[0], scalar->dataE(0)[0]);
}

} // namespace Mantid
} // namespace MDAlgorithms
