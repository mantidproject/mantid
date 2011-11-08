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

*WIKI*/

#include "MantidMDAlgorithms/DivideMD.h"
#include "MantidKernel/System.h"

using namespace Mantid::Kernel;
using namespace Mantid::API;

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
    if (m_lhs_event || m_rhs_event)
      throw std::runtime_error("Cannot divide a MDEventWorkspace at this time.");
  }

  //----------------------------------------------------------------------------------------------
  /// Run the algorithm with an MDEventWorkspace as output
  void DivideMD::execEvent()
  {
    throw std::runtime_error("Cannot divide a MDEventWorkspace at this time.");
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
