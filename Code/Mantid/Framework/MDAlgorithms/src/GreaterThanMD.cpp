/*WIKI*

Perform the > (greater-than) boolean operation on two MDHistoWorkspaces or a MDHistoWorkspace and a scalar.
The output workspace has a signal of 0.0 to mean "false" and a signal of 1.0 to mean "true". Errors are 0.

For two MDHistoWorkspaces, the operation is performed element-by-element.

For a MDHistoWorkspace and a scalar, the operation is performed on each element of the output.

*WIKI*/

#include "MantidMDAlgorithms/GreaterThanMD.h"
#include "MantidKernel/System.h"

using namespace Mantid::Kernel;
using namespace Mantid::API;

namespace Mantid
{
namespace MDAlgorithms
{

  // Register the algorithm into the AlgorithmFactory
  DECLARE_ALGORITHM(GreaterThanMD)
  
  //----------------------------------------------------------------------------------------------
  /** Constructor
   */
  GreaterThanMD::GreaterThanMD()
  {  }
    
  //----------------------------------------------------------------------------------------------
  /** Destructor
   */
  GreaterThanMD::~GreaterThanMD()
  {  }
  
  //----------------------------------------------------------------------------------------------
  /// Algorithm's name for identification. @see Algorithm::name
  const std::string GreaterThanMD::name() const { return "GreaterThanMD";};
  
  /// Algorithm's version for identification. @see Algorithm::version
  int GreaterThanMD::version() const { return 1;};
  
  //----------------------------------------------------------------------------------------------
  /// Run the algorithm with a MDHisotWorkspace as output and operand
  void GreaterThanMD::execHistoHisto(Mantid::MDEvents::MDHistoWorkspace_sptr out, Mantid::MDEvents::MDHistoWorkspace_const_sptr operand)
  {
    out->greaterThan(*operand);
  }

  //----------------------------------------------------------------------------------------------
  /// Run the algorithm with a MDHisotWorkspace as output and a scalar on the RHS
  void GreaterThanMD::execHistoScalar(Mantid::MDEvents::MDHistoWorkspace_sptr out, Mantid::DataObjects::WorkspaceSingleValue_const_sptr scalar)
  {
    out->greaterThan(scalar->dataY(0)[0]);
  }


} // namespace Mantid
} // namespace MDAlgorithms
