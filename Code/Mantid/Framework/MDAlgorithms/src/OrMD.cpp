/*WIKI*

Perform the Or boolean operation on two MDHistoWorkspaces.
The || operation is performed element-by-element.
A signal of 0.0 means "false" and any non-zero signal is "true".

== Usage ==

See [[MDHistoWorkspace#Boolean_Operations|this page]] for examples on using boolean operations.

*WIKI*/

#include "MantidMDAlgorithms/OrMD.h"
#include "MantidKernel/System.h"

using namespace Mantid::Kernel;
using namespace Mantid::API;

namespace Mantid
{
namespace MDAlgorithms
{

  // Register the algorithm into the AlgorithmFactory
  DECLARE_ALGORITHM(OrMD)
  
  //----------------------------------------------------------------------------------------------
  /** Constructor
   */
  OrMD::OrMD()
  {  }
    
  //----------------------------------------------------------------------------------------------
  /** Destructor
   */
  OrMD::~OrMD()
  {  }
  
  //----------------------------------------------------------------------------------------------
  /// Algorithm's name for identification. @see Algorithm::name
  const std::string OrMD::name() const { return "OrMD";};
  
  /// Algorithm's version for identification. @see Algorithm::version
  int OrMD::version() const { return 1;};
  
  //----------------------------------------------------------------------------------------------
  /// Run the algorithm with a MDHisotWorkspace as output and operand
  void OrMD::execHistoHisto(Mantid::MDEvents::MDHistoWorkspace_sptr out, Mantid::MDEvents::MDHistoWorkspace_const_sptr operand)
  {
    out->operator |=(*operand);
  }


} // namespace Mantid
} // namespace MDAlgorithms
