/*WIKI* 



*WIKI*/
//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidDataHandling/LoadSNSNexus.h"

namespace Mantid
{
namespace DataHandling
{
// Register the algorithm into the algorithm factory
DECLARE_ALGORITHM(LoadSNSNexus);

LoadSNSNexus::LoadSNSNexus()
{
  this->useAlgorithm("LoadTOFRawNexus");
  this->deprecatedDate("2011-09-27");
}

int LoadSNSNexus::version() const {return 1;}

const std::string LoadSNSNexus::name() const {return "LoadSNSNexus";}

} // namespace DataHandling
} // namespace Mantid
