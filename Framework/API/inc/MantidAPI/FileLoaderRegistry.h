// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2013 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/AlgorithmFactory.h"
#include "MantidAPI/IFileLoader.h"
#include "MantidKernel/FileDescriptor.h"
#include "MantidKernel/LegacyNexusDescriptor.h"
#include "MantidKernel/NexusHDF5Descriptor.h"
#include "MantidKernel/SingletonHolder.h"

#ifndef Q_MOC_RUN
#include <type_traits>
#endif

#include <array>
#include <map>
#include <string>

namespace Mantid {
namespace Kernel {

class Logger;
}
namespace API {

class IAlgorithm;

/**
Keeps a registry of algorithm's that are file loading algorithms to allow them
to be searched
to find the correct one to load a particular file.

A macro, DECLARE_FILELOADER_ALGORITHM is defined in RegisterFileLoader.h. Use
this in place of the standard
DECLARE_ALGORITHM macro
 */
class MANTID_API_DLL FileLoaderRegistryImpl {
public:
  /// Defines types of possible file
  enum LoaderFormat { Nexus, Generic, NexusHDF5 };

public:
  /// @returns the number of entries in the registry
  inline size_t size() const { return m_totalSize; }

  /**
   * Registers a loader whose format is one of the known formats given in
   * LoaderFormat. It
   * also passes this registration on to the AlgorithmFactory so that it can be
   * created.
   * The template type should be the class being registered. The name is taken
   * from the string
   * returned by the name() method on the object.
   * @param format The type of loader being subscribed, see LoaderFormat
   * @throws std::invalid_argument if an entry with this name already exists
   */
  template <typename Type> void subscribe(LoaderFormat format) {
    SubscriptionValidator<Type>::check(format);
    const auto nameVersion = AlgorithmFactory::Instance().subscribe<Type>();
    // If the factory didn't throw then the name is valid
    m_names[format].insert(nameVersion);
    m_totalSize += 1;
    m_log.debug() << "Registered '" << nameVersion.first << "' version '" << nameVersion.second << "' as file loader\n";
  }

  /// Unsubscribe a named algorithm and version from the loader registration
  void unsubscribe(const std::string &name, const int version = -1);

  /// Returns the name of an Algorithm that can load the given filename
  const std::shared_ptr<IAlgorithm> chooseLoader(const std::string &filename) const;
  /// Checks whether the given algorithm can load the file
  bool canLoad(const std::string &algorithmName, const std::string &filename) const;

private:
  /// Friend so that CreateUsingNew
  friend struct Mantid::Kernel::CreateUsingNew<FileLoaderRegistryImpl>;

  /// Default constructor (for singleton)
  FileLoaderRegistryImpl();
  /// Destructor
  ~FileLoaderRegistryImpl();

  /// Helper for subscribe to check base class
  template <typename T> struct SubscriptionValidator {
    static void check(LoaderFormat format) {
      switch (format) {
      case Nexus:
        if (!std::is_base_of<IFileLoader<Kernel::LegacyNexusDescriptor>, T>::value) {
          throw std::runtime_error(std::string("FileLoaderRegistryImpl::subscribe - Class '") + typeid(T).name() +
                                   "' registered as Nexus loader but it does not "
                                   "inherit from "
                                   "API::IFileLoader<Kernel::NexusDescriptor>");
        }
        break;
      case NexusHDF5:
        if (!std::is_base_of<IFileLoader<Kernel::NexusHDF5Descriptor>, T>::value) {
          throw std::runtime_error(std::string("FileLoaderRegistryImpl::subscribe - Class '") + typeid(T).name() +
                                   "' registered as NexusHDF5 loader but it does not "
                                   "inherit from "
                                   "API::IFileLoader<Kernel::NexusHDF5Descriptor>");
        }
        break;
      case Generic:
        if (!std::is_base_of<IFileLoader<Kernel::FileDescriptor>, T>::value) {
          throw std::runtime_error(std::string("FileLoaderRegistryImpl::subscribe - Class '") + typeid(T).name() +
                                   "' registered as Generic loader but it does "
                                   "not inherit from "
                                   "API::IFileLoader<Kernel::FileDescriptor>");
        }
        break;
      default:
        throw std::runtime_error("Invalid LoaderFormat given");
      }
    }
  };

  /// Remove a named algorithm & version from the given map
  void removeAlgorithm(const std::string &name, const int version, std::multimap<std::string, int> &typedLoaders);

  /// The list of names. The index pointed to by LoaderFormat defines a set for
  /// that format. The length is equal to the length of the LoaderFormat enum
  std::array<std::multimap<std::string, int>, 3> m_names;
  /// Total number of names registered
  size_t m_totalSize;

  /// Reference to a logger
  mutable Kernel::Logger m_log;
};

/// Type for the actual singleton instance
using FileLoaderRegistry = Mantid::Kernel::SingletonHolder<FileLoaderRegistryImpl>;

} // namespace API
} // namespace Mantid

namespace Mantid {
namespace Kernel {
EXTERN_MANTID_API template class MANTID_API_DLL Mantid::Kernel::SingletonHolder<Mantid::API::FileLoaderRegistryImpl>;
}
} // namespace Mantid
