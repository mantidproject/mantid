/*WIKI* 



*WIKI*/
//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidDataHandling/LoadSNSNexus.h"
#include "MantidAPI/LoadAlgorithmFactory.h" // For the DECLARE_LOADALGORITHM macro

namespace Mantid
{
namespace DataHandling
{
// Register the algorithm into the algorithm factory
DECLARE_ALGORITHM(LoadSNSNexus)
DECLARE_LOADALGORITHM(LoadSNSNexus)

LoadSNSNexus::LoadSNSNexus()
{
  this->useAlgorithm("LoadTOFRawNexus");
  this->deprecatedDate("2011-09-27");
}

int LoadSNSNexus::version() const {return 1;}

const std::string LoadSNSNexus::name() const {return "LoadSNSNexus";}

/**
 * Checks the file by opening it and reading few lines
 *  @param filePath :: name of the file inluding its path
 *  @return an integer value how much this algorithm can load the file
 */
int LoadSNSNexus::fileCheck(const std::string& filePath)
{
  UNUSED_ARG(filePath);
  return 0;
}

} // namespace DataHandling
} // namespace Mantid
