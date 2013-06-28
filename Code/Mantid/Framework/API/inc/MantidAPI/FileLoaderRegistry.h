#ifndef MANTID_API_FILELOADERREGISTRY_H_
#define MANTID_API_FILELOADERREGISTRY_H_

#include "MantidAPI/DllConfig.h"
#include "MantidAPI/AlgorithmFactory.h"

#include <set>
#include <string>
#include <vector>

namespace Mantid
{
  // Forward declaration
  namespace Kernel
  {
    class Logger;
  }
  namespace API
  {

    /**
    Keeps a registry of algorithm's that are file loading algorithms to allow them to be searched
    to find the correct one to load a particular file. Uses FileLoaderPicker to do the most of the work

    A macro, DECLARE_FILELOADER_ALGORITHM is defined in RegisterFileLoader.h. Use this in place of the standard
    DECLARE_ALGORITHM macro

    Copyright &copy; 2013 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

    This file is part of Mantid.

    Mantid is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 3 of the License, or
    (at your option) any later version.

    Mantid is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.

    File change history is stored at: <https://github.com/mantidproject/mantid>
    Code Documentation is available at: <http://doxygen.mantidproject.org>
     */
    class MANTID_API_DLL FileLoaderRegistry
    {
    public:

      /// Defines types of possible file
      enum LoaderFormat { NonHDF, HDF };

    public:
      /// Default constructor
      FileLoaderRegistry();

      /// @returns the number of entries in the registry
      inline size_t size() const { return m_totalSize; }

      /**
       * Registers a loader whose format is one of the known formats given in LoaderFormat. It
       * also passes this registration on to the AlgorithmFactory so that it can be created.
       * The template type should be the class being registered. The name is taken from the string
       * returned by the name() method on the object.
       * @param format The type of loader being subscribed, see LoaderFormat
       * @throws std::invalid_argument if an entry with this name already exists
       */
      template<typename Type>
      void subscribe(LoaderFormat format)
      {
        const std::string name = AlgorithmFactory::Instance().subscribe<Type>();
        // If the factory didn't throw then the name is valid
        m_names[format].insert(name);
        m_totalSize += 1;
        m_log.debug() << "Registered '" << name << "' as file loader\n";
      }

      /// Returns the name of an Algorithm that can load the given filename
      const std::string chooseLoader(const std::string &filename) const;

    private:
      /// The list of names. The index pointed to by LoaderFormat defines a set for that format
      std::vector<std::set<std::string> > m_names;
      /// Total number of names registered
      size_t m_totalSize;

      /// Reference to a logger
      Kernel::Logger & m_log;
    };

  } // namespace API
} // namespace Mantid

#endif  /* MANTID_API_FILELOADERREGISTRY_H_ */
