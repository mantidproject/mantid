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

/**
 * Checks the file by opening it and reading few lines
 *  @param filePath :: name of the file inluding its path
 *  @return an integer value how much this algorithm can load the file
 */
int LoadSNSEventNexus::fileCheck(const std::string& filePath)
{
  int confidence(0);
  try
  {
    ::NeXus::File file = ::NeXus::File(filePath);
    // Open the base group called 'entry'
    file.openGroup("entry", "NXentry");
    // If all this succeeded then we'll assume this is an SNS Event NeXus file

    // Setting this low as this is a deprecated algorithm that we don't want to autorun.
    confidence = 10;
  }
  catch(::NeXus::Exception&)
  {
  }
  return confidence;
}



//------------------------------------------------------------------------------------------------
/** Executes the algorithm. Reading in the file and creating and populating
 *  the output workspace
 */
void LoadSNSEventNexus::exec()
{
  g_log.error() << "WARNING! LoadSNSEventNexus is deprecated! Please call LoadEventNexus from now on.\n";
  LoadEventNexus::exec();
  return;
}



} // namespace NeXus
} // namespace Mantid
