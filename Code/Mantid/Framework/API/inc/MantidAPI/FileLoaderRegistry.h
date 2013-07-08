#ifndef MANTID_API_FILELOADERREGISTRY_H_
#define MANTID_API_FILELOADERREGISTRY_H_

#include "MantidAPI/AlgorithmFactory.h"
#include "MantidAPI/IFileLoader.h"
#include "MantidAPI/IHDFFileLoader.h"
#include "MantidKernel/SingletonHolder.h"

#include <boost/type_traits/is_base_of.hpp>

#include <map>
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
    // Forward declaration
    class IAlgorithm;

    /**
    Keeps a registry of algorithm's that are file loading algorithms to allow them to be searched
    to find the correct one to load a particular file.

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
    class MANTID_API_DLL FileLoaderRegistryImpl
    {
    public:

      /// Defines types of possible file
      enum LoaderFormat { NonHDF, HDF };

    public:
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
        SubscriptionValidator<Type>::check(format);
        const auto nameVersion = AlgorithmFactory::Instance().subscribe<Type>();
        // If the factory didn't throw then the name is valid
        m_names[format].insert(nameVersion);
        m_totalSize += 1;
        m_log.debug() << "Registered '" << nameVersion.first << "' version '" << nameVersion.second << "' as file loader\n";
      }

      /// Returns the name of an Algorithm that can load the given filename
      const boost::shared_ptr<IAlgorithm> chooseLoader(const std::string &filename) const;
      /// Checks whether the given algorithm can load the file
      bool canLoad(const std::string & algorithmName,const std::string & filename) const;

    private:
      /// Friend so that CreateUsingNew
      friend struct Mantid::Kernel::CreateUsingNew<FileLoaderRegistryImpl>;

      /// Default constructor (for singleton)
      FileLoaderRegistryImpl();
      /// Destructor
      ~FileLoaderRegistryImpl();

      /// Helper for subscribe to check base class
      template <typename T>
      struct SubscriptionValidator
      {
        static void check(LoaderFormat format)
        {
          switch(format)
          {
          case HDF:
            if(!boost::is_base_of<IHDFFileLoader,T>::value)
            {
              throw std::runtime_error(std::string("FileLoaderRegistryImpl::subscribe - Class '") + typeid(T).name() +
                                       "' registered as HDF loader but it does not inherit from Mantid::API::IHDFFileLoader");
            }
            break;
          case NonHDF:
            if(!boost::is_base_of<IFileLoader,T>::value)
            {
              throw std::runtime_error(std::string("FileLoaderRegistryImpl::subscribe - Class '") + typeid(T).name() +
                "' registered as Non-HDF loader but it does not inherit from Mantid::API::IFileLoader");
            }
          break;
            default: throw std::runtime_error("Invalid LoaderFormat given");
          }
        }
      };

      /// The list of names. The index pointed to by LoaderFormat defines a set for that format
      std::vector<std::multimap<std::string,int> > m_names;
      /// Total number of names registered
      size_t m_totalSize;

      /// Reference to a logger
      Kernel::Logger & m_log;
    };

    ///Forward declaration of a specialisation of SingletonHolder for FileLoaderRegistryImpl (needed for dllexport/dllimport) and a typedef for it.
    #ifdef _WIN32
      // this breaks new namespace declaration rules; need to find a better fix
      template class MANTID_API_DLL Mantid::Kernel::SingletonHolder<FileLoaderRegistryImpl>;
    #endif /* _WIN32 */

    /// Type for the actual singleton instance
    typedef MANTID_API_DLL Mantid::Kernel::SingletonHolder<FileLoaderRegistryImpl> FileLoaderRegistry;

  } // namespace API
} // namespace Mantid

#endif  /* MANTID_API_FILELOADERREGISTRY_H_ */
