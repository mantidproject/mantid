//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidNexus/LoadSNSEventNexus.h"
#include "MantidAPI/LoadAlgorithmFactory.h" // For the DECLARE_LOADALGORITHM macro

using namespace ::NeXus;
using namespace Mantid::Geometry;
using namespace Mantid::DataObjects;

namespace Mantid
{
namespace NeXus
{

DECLARE_ALGORITHM(LoadSNSEventNexus)
DECLARE_LOADALGORITHM(LoadSNSEventNexus)

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

/**
 * Checks the file by opening it and reading few lines
 *  @param filePath :: name of the file inluding its path
 *  @return an integer value how much this algorithm can load the file
 */
int LoadSNSEventNexus::fileCheck(const std::string& filePath)
{
  return 0;
}

} // namespace NeXus
} // namespace Mantid
