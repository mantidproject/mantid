#include "MantidAlgorithms/ExtractMasking.h"
#include "MantidKernel/System.h"

using namespace Mantid::Kernel;
using namespace Mantid::API;

namespace Mantid
{
namespace Algorithms
{

  DECLARE_ALGORITHM(ExtractMasking)


  //----------------------------------------------------------------------------------------------
  /** Constructor
   */
  ExtractMasking::ExtractMasking()
  {
    this->useAlgorithm("ExtractMask");
    this->deprecatedDate("2012-02-14");
  }
    
  //----------------------------------------------------------------------------------------------
  /** Destructor
   */
  ExtractMasking::~ExtractMasking()
  {
  }
  


} // namespace Mantid
} // namespace Algorithms
