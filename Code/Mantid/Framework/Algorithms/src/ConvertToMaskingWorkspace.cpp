/*WIKI*

Convert a Workspace2D to a MaskWorkspace

*WIKI*/
#include "MantidAlgorithms/ConvertToMaskingWorkspace.h"
#include "MantidKernel/System.h"


using namespace Mantid::Kernel;
using namespace Mantid::API;

namespace Mantid
{
namespace Algorithms
{

  DECLARE_ALGORITHM(ConvertToMaskingWorkspace)

  //----------------------------------------------------------------------------------------------
  /** Constructor
   */
  ConvertToMaskingWorkspace::ConvertToMaskingWorkspace()
  {
      this->useAlgorithm("ConvertToMaskWorkspace");
      this->deprecatedDate("2012-04-25");
  }
    
  //----------------------------------------------------------------------------------------------
  /** Destructor
   */
  ConvertToMaskingWorkspace::~ConvertToMaskingWorkspace()
  {
  }


} // namespace Mantid
} // namespace Algorithms
