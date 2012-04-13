/*WIKI*
Divide two [[MDHistoWorkspace]]'s or a MDHistoWorkspace and a scalar.

The error of <math> f = a / b </math> is propagated with <math> df^2 = f^2 * (da^2 / a^2 + db^2 / b^2) </math>

* '''MDHistoWorkspace / MDHistoWorkspace'''
** The operation is performed element-by-element.
* '''MDHistoWorkspace / Scalar'''
** Every element of the MDHistoWorkspace is divided by the scalar.
* '''Scalar / MDHistoWorkspace'''
** This is not allowed.
* '''[[MDEventWorkspace]]'s'''
** This operation is not supported, as it is not clear what its meaning would be.

== Usage ==

 C = A / B
 C = A / 123.4
 A /= B
 A /= 123.4

See [[MDHistoWorkspace#Arithmetic_Operations|this page]] for examples on using arithmetic operations.

*WIKI*/

#include "MantidMDAlgorithms/DivideMD.h"
#include "MantidKernel/System.h"
#include "MantidMDEvents/MDEventFactory.h"
#include "MantidMDEvents/MDEventWorkspace.h"
#include "MantidMDEvents/MDBoxBase.h"
#include "MantidMDEvents/MDBox.h"

using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace Mantid::MDEvents;

namespace Mantid
{
namespace MDAlgorithms
{

  // Register the algorithm into the AlgorithmFactory
  DECLARE_ALGORITHM(DivideMD)
  

  //----------------------------------------------------------------------------------------------
  /** Constructor
   */
  DivideMD::DivideMD()
  {
  }
    
  //----------------------------------------------------------------------------------------------
  /** Destructor
   */
  DivideMD::~DivideMD()
  {
  }
  

  //----------------------------------------------------------------------------------------------
  /// Algorithm's name for identification. @see Algorithm::name
  const std::string DivideMD::name() const { return "DivideMD";};
  
  /// Algorithm's version for identification. @see Algorithm::version
  int DivideMD::version() const { return 1;};
  
  //----------------------------------------------------------------------------------------------
  /// Sets documentation strings for this algorithm
  void DivideMD::initDocs()
  {
    this->setWikiSummary("Divide [[MDHistoWorkspace]]'s");
    this->setOptionalMessage("Divide MDHistoWorkspace's");
  }


  //----------------------------------------------------------------------------------------------
  /// Is the operation commutative?
  bool DivideMD::commutative() const
  { return false; }

  //----------------------------------------------------------------------------------------------
  /// Check the inputs and throw if the algorithm cannot be run
  void DivideMD::checkInputs()
  {
    if (m_rhs_event)
      throw std::runtime_error("Cannot divide by a MDEventWorkspace on the RHS.");
    if (m_lhs_event && !m_rhs_scalar)
      throw std::runtime_error("A MDEventWorkspace can only be divided by a scalar.");
  }

  //----------------------------------------------------------------------------------------------
  /** Perform the operation with MDEventWorkpsace as LHS and a scalar as RHS
   * Will do "ws /= scalar"
   * @param ws ::  MDEventWorkspace being modified
   */
  template<typename MDE, size_t nd>
  void DivideMD::execEventScalar(typename MDEventWorkspace<MDE, nd>::sptr ws)
  {
    // Get the scalar multiplying
    float scalar = float(m_rhs_scalar->dataY(0)[0]);
    float scalarError = float(m_rhs_scalar->dataE(0)[0]);
    float scalarRelativeErrorSquared = (scalarError * scalarError) / (scalar * scalar);

    // Get all the MDBoxes contained
    MDBoxBase<MDE,nd> * parentBox = ws->getBox();
    std::vector<MDBoxBase<MDE,nd> *> boxes;
    parentBox->getBoxes(boxes, 1000, true);

    for (size_t i=0; i<boxes.size(); i++)
    {
      MDBox<MDE,nd> * box = dynamic_cast<MDBox<MDE,nd> *>(boxes[i]);
      if (box)
      {
        typename std::vector<MDE> & events = box->getEvents();
        typename std::vector<MDE>::iterator it = events.begin();
        typename std::vector<MDE>::iterator it_end = events.end();
        for (; it != it_end; it++)
        {
          // Multiply weight by a scalar, propagating error
          float oldSignal = it->getSignal();
          float signal = oldSignal / scalar;
          float errorSquared = signal * signal * (it->getErrorSquared() / (oldSignal * oldSignal) + scalarRelativeErrorSquared);
          it->setSignal(signal);
          it->setErrorSquared(errorSquared);
        }
        box->releaseEvents();
      }
    }
    // Recalculate the totals
    ws->refreshCache();
    // Mark file-backed workspace as dirty
    ws->setFileNeedsUpdating(true);
  }


  //----------------------------------------------------------------------------------------------
  /// Run the algorithm with an MDEventWorkspace as output
  void DivideMD::execEvent()
  {
    if (m_lhs_event && !m_rhs_scalar)
      throw std::runtime_error("A MDEventWorkspace can only be divided by a scalar.");
    if (!m_out_event)
      throw std::runtime_error("DivideMD::execEvent(): Error creating output MDEventWorkspace.");
    // Call the method to do the dividing
    CALL_MDEVENT_FUNCTION(this->execEventScalar, m_out_event);
  }

  //----------------------------------------------------------------------------------------------
  /// Run the algorithm with a MDHisotWorkspace as output and operand
  void DivideMD::execHistoHisto(Mantid::MDEvents::MDHistoWorkspace_sptr out, Mantid::MDEvents::MDHistoWorkspace_const_sptr operand)
  {
    out->divide(*operand);
  }

  //----------------------------------------------------------------------------------------------
  /// Run the algorithm with a MDHisotWorkspace as output, scalar and operand
  void DivideMD::execHistoScalar(Mantid::MDEvents::MDHistoWorkspace_sptr out, Mantid::DataObjects::WorkspaceSingleValue_const_sptr scalar)
  {
    out->divide(scalar->dataY(0)[0], scalar->dataE(0)[0]);
  }



} // namespace Mantid
} // namespace MDAlgorithms
