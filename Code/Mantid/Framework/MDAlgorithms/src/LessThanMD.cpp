/*WIKI*

Perform the < (less-than) boolean operation on two MDHistoWorkspaces or a MDHistoWorkspace and a scalar.
The output workspace has a signal of 0.0 to mean "false" and a signal of 1.0 to mean "true". Errors are 0.

For two MDHistoWorkspaces, the operation is performed element-by-element.

For a MDHistoWorkspace and a scalar, the operation is performed on each element of the output.

*WIKI*/

#include "MantidMDAlgorithms/LessThanMD.h"
#include "MantidKernel/System.h"

using namespace Mantid::Kernel;
using namespace Mantid::API;

namespace Mantid
{
namespace MDAlgorithms
{

  // Register the algorithm into the AlgorithmFactory
  DECLARE_ALGORITHM(LessThanMD)
  
  //----------------------------------------------------------------------------------------------
  /** Constructor
   */
  LessThanMD::LessThanMD()
  {  }
    
  //----------------------------------------------------------------------------------------------
  /** Destructor
   */
  LessThanMD::~LessThanMD()
  {  }
  
  //----------------------------------------------------------------------------------------------
  /// Algorithm's name for identification. @see Algorithm::name
  const std::string LessThanMD::name() const { return "LessThanMD";};
  
  /// Algorithm's version for identification. @see Algorithm::version
  int LessThanMD::version() const { return 1;};
  
  //----------------------------------------------------------------------------------------------
  /// Run the algorithm with a MDHisotWorkspace as output and operand
  void LessThanMD::execHistoHisto(Mantid::MDEvents::MDHistoWorkspace_sptr out, Mantid::MDEvents::MDHistoWorkspace_const_sptr operand)
  {
    out->lessThan(*operand);
  }

  //----------------------------------------------------------------------------------------------
  /// Run the algorithm with a MDHisotWorkspace as output and a scalar on the RHS
  void LessThanMD::execHistoScalar(Mantid::MDEvents::MDHistoWorkspace_sptr out, Mantid::DataObjects::WorkspaceSingleValue_const_sptr scalar)
  {
    out->lessThan(scalar->dataY(0)[0]);
  }


} // namespace Mantid
} // namespace MDAlgorithms
