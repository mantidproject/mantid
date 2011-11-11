/*WIKI*
Multiply two [[MDHistoWorkspace]]'s or a MDHistoWorkspace and a scalar.

The error of <math> f = a * b </math> is propagated with <math>df^2 = f^2 * (da^2 / a^2 + db^2 / b^2)</math>

* '''MDHistoWorkspace * MDHistoWorkspace'''
** The operation is performed element-by-element.
* '''MDHistoWorkspace * Scalar''' or '''Scalar * MDHistoWorkspace'''
** Every element of the MDHistoWorkspace is multiplied by the scalar.
* '''[[MDEventWorkspace]]'s'''
** This operation is not supported, as it is not clear what its meaning would be.

== Usage ==

 C = A * B
 C = A * 123.4
 A *= B
 A *= 123.4

See [[MDHistoWorkspace#Arithmetic_Operations|this page]] for examples on using arithmetic operations.

*WIKI*/

#include "MantidMDAlgorithms/MultiplyMD.h"
#include "MantidKernel/System.h"

using namespace Mantid::Kernel;
using namespace Mantid::API;

namespace Mantid
{
namespace MDAlgorithms
{

  // Register the algorithm into the AlgorithmFactory
  DECLARE_ALGORITHM(MultiplyMD)
  
  //----------------------------------------------------------------------------------------------
  /** Constructor
   */
  MultiplyMD::MultiplyMD()
  {  }
    
  //----------------------------------------------------------------------------------------------
  /** Destructor
   */
  MultiplyMD::~MultiplyMD()
  {  }
  
  //----------------------------------------------------------------------------------------------
  /// Algorithm's name for identification. @see Algorithm::name
  const std::string MultiplyMD::name() const { return "MultiplyMD";};
  
  /// Algorithm's version for identification. @see Algorithm::version
  int MultiplyMD::version() const { return 1;};
  
  //----------------------------------------------------------------------------------------------
  /// Sets documentation strings for this algorithm
  void MultiplyMD::initDocs()
  {
    this->setWikiSummary("Multiply a [[MDHistoWorkspace]] by another one or a scalar.");
    this->setOptionalMessage("Multiply a MDHistoWorkspace by another one or a scalar.");
  }


  //----------------------------------------------------------------------------------------------
  /// Is the operation commutative?
  bool MultiplyMD::commutative() const
  { return true; }

  //----------------------------------------------------------------------------------------------
  /// Check the inputs and throw if the algorithm cannot be run
  void MultiplyMD::checkInputs()
  {
    if (m_lhs_event || m_rhs_event)
      throw std::runtime_error("Cannot multiply a MDEventWorkspace at this time.");
  }

  //----------------------------------------------------------------------------------------------
  /// Run the algorithm with an MDEventWorkspace as output
  void MultiplyMD::execEvent()
  {
    throw std::runtime_error("Cannot multiply a MDEventWorkspace at this time.");
  }

  //----------------------------------------------------------------------------------------------
  /// Run the algorithm with a MDHisotWorkspace as output and operand
  void MultiplyMD::execHistoHisto(Mantid::MDEvents::MDHistoWorkspace_sptr out, Mantid::MDEvents::MDHistoWorkspace_const_sptr operand)
  {
    out->multiply(*operand);
  }

  //----------------------------------------------------------------------------------------------
  /// Run the algorithm with a MDHisotWorkspace as output, scalar and operand
  void MultiplyMD::execHistoScalar(Mantid::MDEvents::MDHistoWorkspace_sptr out, Mantid::DataObjects::WorkspaceSingleValue_const_sptr scalar)
  {
    out->multiply(scalar->dataY(0)[0], scalar->dataE(0)[0]);
  }



} // namespace Mantid
} // namespace MDAlgorithms
