//------------------------------------------------------------------------------
// Includes
//------------------------------------------------------------------------------
#include "MantidPythonAPI/PythonScriptWriter.h"

#include "MantidKernel/Exception.h"

namespace Mantid
{
  namespace PythonAPI
  {
    
    DECLARE_SCRIPTWRITER(PythonScriptWriter)

    //------------------------------------------------------------------------------
    // Public methods
    //------------------------------------------------------------------------------
    /**
     * Generate the script from the history
     * @param history :: 
     */
    std::string PythonScriptWriter::write(const API::WorkspaceHistory & history) const
    {
      UNUSED_ARG(history);
      throw Kernel::Exception::NotImplementedError(" PythonScriptWriter::write");
    }
    
    
  }  // namespace PythonAPI
} // namespace Mantid

