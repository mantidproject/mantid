#include "MantidDataHandling/SaveDetectorMasks.h"
#include "MantidKernel/System.h"

using namespace Mantid::Kernel;
using namespace Mantid::API;

namespace Mantid
{
namespace DataHandling
{

  DECLARE_ALGORITHM(SaveDetectorMasks)


  //----------------------------------------------------------------------------------------------
  /** Constructor
   */
  SaveDetectorMasks::SaveDetectorMasks()
  {
    this->useAlgorithm("SaveMask");
    this->deprecatedDate("2012-01-13");
  }
    
  //----------------------------------------------------------------------------------------------
  /** Destructor
   */
  SaveDetectorMasks::~SaveDetectorMasks()
  {
  }
  


} // namespace Mantid
} // namespace DataHandling
