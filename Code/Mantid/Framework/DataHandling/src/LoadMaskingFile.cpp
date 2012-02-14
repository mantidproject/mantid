#include "MantidDataHandling/LoadMaskingFile.h"
#include "MantidKernel/System.h"

using namespace Mantid::Kernel;
using namespace Mantid::API;

namespace Mantid
{
namespace DataHandling
{

  DECLARE_ALGORITHM(LoadMaskingFile)

  //----------------------------------------------------------------------------------------------
  /** Constructor
   */
  LoadMaskingFile::LoadMaskingFile()
  {
    this->useAlgorithm("LoadMask");
    this->deprecatedDate("2012-02-14");
  }
    
  //----------------------------------------------------------------------------------------------
  /** Destructor
   */
  LoadMaskingFile::~LoadMaskingFile()
  {
  }
  


} // namespace Mantid
} // namespace DataHandling
