/*WIKI*
Subtract two [[MDHistoWorkspace]]'s or a MDHistoWorkspace and a scalar.

* '''MDHistoWorkspace - MDHistoWorkspace'''
** The operation is performed element-by-element.
* '''MDHistoWorkspace - Scalar '''
** The scalar is subtracted from every element of the MDHistoWorkspace. The squares of errors are summed.
* '''Scalar - MDHistoWorkspace'''
** This is not allowed.
* '''[[MDEventWorkspace]]'s'''
** This is not currently supported, but it could be in the future.

== Usage ==

See [[MDHistoWorkspace#Arithmetic_Operations|this page]] for examples on using arithmetic operations.

*WIKI*/

#include "MantidMDAlgorithms/MinusMD.h"
#include "MantidKernel/System.h"

using namespace Mantid::Kernel;
using namespace Mantid::API;

namespace Mantid
{
namespace MDAlgorithms
{

  // Register the algorithm into the AlgorithmFactory
  DECLARE_ALGORITHM(MinusMD)

  //----------------------------------------------------------------------------------------------
  /** Constructor
   */
  MinusMD::MinusMD()
  {  }
    
  //----------------------------------------------------------------------------------------------
  /** Destructor
   */
  MinusMD::~MinusMD()
  {  }
  
  //----------------------------------------------------------------------------------------------
  /// Algorithm's name for identification. @see Algorithm::name
  const std::string MinusMD::name() const { return "MinusMD";};
  
  /// Algorithm's version for identification. @see Algorithm::version
  int MinusMD::version() const { return 1;};
  
  //----------------------------------------------------------------------------------------------
  /// Sets documentation strings for this algorithm
  void MinusMD::initDocs()
  {
    this->setWikiSummary("Subtract two [[MDHistoWorkspace]]s");
    this->setOptionalMessage("Subtract two MDHistoWorkspaces.");
  }


  //----------------------------------------------------------------------------------------------
  /// Is the operation commutative?
  bool MinusMD::commutative() const
  { return false; }

  //----------------------------------------------------------------------------------------------
  /// Check the inputs and throw if the algorithm cannot be run
  void MinusMD::checkInputs()
  {
    if (m_lhs_event || m_rhs_event)
      throw std::runtime_error("Cannot subtract a MDEventWorkspace at this time.");
  }

  //----------------------------------------------------------------------------------------------
  /// Run the algorithm with an MDEventWorkspace as output
  void MinusMD::execEvent()
  {
    throw std::runtime_error("Cannot subtract a MDEventWorkspace at this time.");
  }

  //----------------------------------------------------------------------------------------------
  /// Run the algorithm with a MDHisotWorkspace as output and operand
  void MinusMD::execHistoHisto(Mantid::MDEvents::MDHistoWorkspace_sptr out, Mantid::MDEvents::MDHistoWorkspace_const_sptr operand)
  {
    out->subtract(*operand);
  }

  //----------------------------------------------------------------------------------------------
  /// Run the algorithm with a MDHisotWorkspace as output, scalar and operand
  void MinusMD::execHistoScalar(Mantid::MDEvents::MDHistoWorkspace_sptr out, Mantid::DataObjects::WorkspaceSingleValue_const_sptr scalar)
  {
    out->subtract(scalar->dataY(0)[0], scalar->dataE(0)[0]);
  }



} // namespace Mantid
} // namespace MDAlgorithms
