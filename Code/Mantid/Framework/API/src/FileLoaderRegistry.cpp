#include "MantidAPI/FileLoaderRegistry.h"
#include "MantidKernel/Exception.h"

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
    FileLoaderRegistry::FileLoaderRegistry()
    {
    }

    /**
     * @param name The string name of the entry
     * @throws std::invalid_argument if an entry with this name already exists
     */
    void FileLoaderRegistry::subscribe(const std::string & name)
    {
      auto insertionResult = m_names.insert(name);
      if(!insertionResult.second)
      {
        throw std::invalid_argument("FileLoaderRegistry::subscribe - Cannot subscribe '"
            + name + "'. An entry with that name already exists");
      }
    }

    /**
     * Attempts to pick the best suited loader for the given file name from those in the registry
     * @param filename A string that should point to an existing file
     * @throws std::invalid_argument if the filename does not point to an existing file or an empty string is given
     * @throws Exception::NotFoundError if a loader could not be found
     */
    std::string FileLoaderRegistry::findLoader(const std::string & filename) const
    {
      if(filename.empty() || !Poco::File(filename).exists())
      {
        throw std::invalid_argument("FileLoaderRegistry::chooserLoader - Cannot open file '" + filename + "'");
      }
    }

    //----------------------------------------------------------------------------------------------
    // Private members
    //----------------------------------------------------------------------------------------------

  } // namespace API
} // namespace Mantid
