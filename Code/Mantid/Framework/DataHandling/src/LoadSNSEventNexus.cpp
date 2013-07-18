/*WIKI* 


*WIKI*/
//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidDataHandling/LoadSNSEventNexus.h"

using namespace ::NeXus;
using namespace Mantid::Geometry;
using namespace Mantid::DataObjects;

namespace Mantid
{
namespace DataHandling
{

LoadSNSEventNexus::LoadSNSEventNexus()
{
  this->useAlgorithm("LoadEventNexus");
  this->deprecatedDate("2011-02-14");
}

int LoadSNSEventNexus::version() const
{
  return 1;
}

const std::string LoadSNSEventNexus::name() const
{
  return "LoadSNSEventNexus";
}

} // namespace DataHandling
} // namespace Mantid
