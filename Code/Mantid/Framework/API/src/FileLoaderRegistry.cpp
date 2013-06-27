#include "MantidAPI/FileLoaderRegistry.h"
#include "MantidKernel/Exception.h"
#include "MantidKernel/Logger.h"

#include <Poco/File.h>

namespace Mantid
{
  namespace API
  {
    //----------------------------------------------------------------------------------------------
    // Public members
    //----------------------------------------------------------------------------------------------
    /**
     * Creates an empty registry
     */
    FileLoaderRegistry::FileLoaderRegistry() :
        m_names(2, std::set<std::string>()), m_totalSize(0),
        m_log(Kernel::Logger::get("FileLoaderRegistry"))
    {
    }

    //----------------------------------------------------------------------------------------------
    // Private members
    //----------------------------------------------------------------------------------------------

  } // namespace API
} // namespace Mantid
