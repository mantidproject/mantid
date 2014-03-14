/*WIKI* 
SaveANSTOAscii is an export-only Ascii-based save format with no associated loader. It is based on a python script by Maximilian Skoda, written for the ISIS Reflectometry GUI
==== Limitations ====
While Files saved with SaveANSTOAscii can be loaded back into mantid using LoadAscii, the resulting workspaces won't be usful as the data written by SaveANSTOAscii is not in the normal X,Y,E,DX format.
*WIKI*/
//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidDataHandling/SaveANSTOAscii.h"
#include "MantidDataHandling/AsciiPointBase.h"

namespace Mantid
{
  namespace DataHandling
  {
    // Register the algorithm into the algorithm factory
    DECLARE_ALGORITHM(SaveANSTOAscii)
    using namespace Kernel;
    using namespace API;

    // Initialise the logger
    Logger& SaveANSTOAscii::g_log = Logger::get("SaveANSTOAscii");
    
    /// Sets documentation strings for this algorithm
    void SaveANSTOAscii::initDocs()
    {
      this->setWikiSummary("Saves a 2D [[workspace]] to a tab separated ascii file. ");
      this->setOptionalMessage("Saves a 2D workspace to a ascii file.");
    }
  } // namespace DataHandling
} // namespace Mantid
