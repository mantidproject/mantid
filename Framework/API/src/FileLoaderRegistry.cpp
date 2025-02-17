// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidAPI/FileLoaderRegistry.h"
#include "MantidAPI/IFileLoader.h"
#include "MantidAPI/NexusFileLoader.h"

#include <H5Cpp.h>
#include <Poco/File.h>

namespace Mantid::API {
namespace {
//----------------------------------------------------------------------------------------------
// Anonymous namespace helpers
//----------------------------------------------------------------------------------------------
/// @cond
template <typename T> struct DescriptorCallback {
  void apply(std::shared_ptr<T> & /*unused*/) {} // general one does nothing
};
template <> struct DescriptorCallback<Kernel::FileDescriptor> {
  void apply(std::shared_ptr<Kernel::FileDescriptor> &descriptor) { descriptor->resetStreamToStart(); }
};
/// @endcond

/// @cond
template <typename T> struct DescriptorSetter {
  // general one does nothing
  void apply(std::shared_ptr<NexusFileLoader> & /*unused*/, std::shared_ptr<T> & /*unused*/) {}
};
template <> struct DescriptorSetter<Kernel::NexusHDF5Descriptor> {
  void apply(std::shared_ptr<NexusFileLoader> &loader, const std::shared_ptr<Kernel::NexusHDF5Descriptor> &descriptor) {
    loader->setFileInfo(descriptor);
  }
};
/// @endcond

/**
 * @param filename A string giving a filename
 * @param names The collection of names to search through
 * @param logger A reference to a Mantid Logger object
 * @return  A string containing the name of an algorithm to load the file, or an
 * empty string if nothing
 * was found
 */
template <typename DescriptorType, typename FileLoaderType>
std::pair<IAlgorithm_sptr, int> searchForLoader(const std::string &filename,
                                                const std::multimap<std::string, int> &names, Kernel::Logger &logger) {
  const auto &factory = AlgorithmFactory::Instance();
  IAlgorithm_sptr bestLoader;
  int maxConfidence(0);
  auto descriptor = std::make_shared<DescriptorType>(filename);
  DescriptorCallback<DescriptorType> callback;
  DescriptorSetter<DescriptorType> setdescriptor;

  auto iend = names.end();
  for (auto it = names.begin(); it != iend; ++it) {
    const std::string &name = it->first;
    const int version = it->second;
    logger.debug() << "Checking " << name << " version " << version << '\n';

    // Use static cast for speed. Checks have been done at registration to check
    // the types
    auto alg = std::static_pointer_cast<FileLoaderType>(factory.create(name, version)); // highest version
    try {
      const int confidence = alg->confidence(*(descriptor.get()));
      logger.debug() << name << " returned with confidence=" << confidence << '\n';
      if (confidence > maxConfidence) // strictly greater
      {
        bestLoader = alg;
        maxConfidence = confidence;
      }
    } catch (std::exception &exc) {
      logger.warning() << "Checking loader '" << name << "' raised an error: '" << exc.what() << "'. Loader skipped.\n";
    }
    callback.apply(descriptor);
  }

  auto nxsLoader = std::dynamic_pointer_cast<NexusFileLoader>(bestLoader);
  if (nxsLoader)
    setdescriptor.apply(nxsLoader, descriptor);

  return {bestLoader, maxConfidence};
}
} // namespace

//----------------------------------------------------------------------------------------------
// Public members
//----------------------------------------------------------------------------------------------

/**
 * If the name does not exist then it does nothing
 * @param name Name of the algorithm to remove from the search list
 * @param version An optional version to remove. -1 indicates remove all
 * (Default=-1)
 */
void FileLoaderRegistryImpl::unsubscribe(const std::string &name, const int version) {
  auto iend = m_names.end();
  for (auto it = m_names.begin(); it != iend; ++it) {
    removeAlgorithm(name, version, *it);
  }
}

/**
 * Queries each registered algorithm and asks it how confident it is that it can
 * load the given file. The name of the one with the highest confidence is
 * returned.
 * @param filename A full file path pointing to an existing file
 * @return A string containing the name of an algorithm to load the file
 * @throws Exception::NotFoundError if an algorithm cannot be found
 */
const std::shared_ptr<IAlgorithm> FileLoaderRegistryImpl::chooseLoader(const std::string &filename) const {
  using Kernel::FileDescriptor;
  using Kernel::NexusDescriptor;
  using Kernel::NexusHDF5Descriptor;
  m_log.debug() << "Trying to find loader for '" << filename << "'\n";

  IAlgorithm_sptr bestLoader;

  if (H5::H5File::isHdf5(filename)) {
    std::pair<IAlgorithm_sptr, int> HDF5result =
        searchForLoader<NexusHDF5Descriptor, IFileLoader<NexusHDF5Descriptor>>(filename, m_names[NexusHDF5], m_log);

    // must also try NexusDescriptor algorithms because LoadMuonNexus can load both HDF4 and HDF5 files
    std::pair<IAlgorithm_sptr, int> HDF4result =
        searchForLoader<NexusDescriptor, IFileLoader<NexusDescriptor>>(filename, m_names[Nexus], m_log);

    if (HDF5result.second > HDF4result.second)
      bestLoader = HDF5result.first;
    else
      bestLoader = HDF4result.first;
  } else {
    try {
      bestLoader =
          searchForLoader<NexusDescriptor, IFileLoader<NexusDescriptor>>(filename, m_names[Nexus], m_log).first;
    } catch (std::exception const &e) {
      m_log.debug() << "Error in looking for NeXus files: " << e.what() << '\n';
    }
  }

  if (!bestLoader)
    bestLoader = searchForLoader<FileDescriptor, IFileLoader<FileDescriptor>>(filename, m_names[Generic], m_log).first;

  if (!bestLoader) {
    throw Kernel::Exception::NotFoundError(filename, "Unable to find loader");
  }
  m_log.debug() << "Found loader " << bestLoader->name() << " for file '" << filename << "'\n";
  return bestLoader;
}

/**
 * Perform a check that that the given algorithm can load the file
 * @param algorithmName The name of the algorithm to check
 * @param filename The name of file to check
 * @returns True if the algorithm can load the file, false otherwise
 * @throws std::invalid_argument if the loader does not exist
 */
bool FileLoaderRegistryImpl::canLoad(const std::string &algorithmName, const std::string &filename) const {
  using Kernel::FileDescriptor;
  using Kernel::NexusDescriptor;
  using Kernel::NexusHDF5Descriptor;

  // Check if it is in one of our lists
  const bool nexus = (m_names[Nexus].find(algorithmName) != m_names[Nexus].end());
  const bool nexusHDF5 = (m_names[NexusHDF5].find(algorithmName) != m_names[NexusHDF5].end());
  const bool nonHDF = (m_names[Generic].find(algorithmName) != m_names[Generic].end());

  if (!(nexus || nexusHDF5 || nonHDF))
    throw std::invalid_argument("FileLoaderRegistryImpl::canLoad - Algorithm '" + algorithmName +
                                "' is not registered as a loader.");

  std::multimap<std::string, int> names{{algorithmName, -1}};
  IAlgorithm_sptr loader;
  if (nexus) {
    try {
      loader = searchForLoader<NexusDescriptor, IFileLoader<NexusDescriptor>>(filename, names, m_log).first;
    } catch (std::exception const &e) {
      m_log.debug() << "Error in looking for NeXus files: " << e.what() << '\n';
    }
  } else if (nexusHDF5) {
    if (H5::H5File::isHdf5(filename)) {
      try {
        loader = searchForLoader<NexusHDF5Descriptor, IFileLoader<NexusHDF5Descriptor>>(filename, names, m_log).first;
      } catch (const std::invalid_argument &e) {
        m_log.debug() << "Error in looking for HDF5 based NeXus files: " << e.what() << '\n';
      }
    }
  } else if (nonHDF) {
    loader = searchForLoader<FileDescriptor, IFileLoader<FileDescriptor>>(filename, names, m_log).first;
  }
  return static_cast<bool>(loader);
}

//----------------------------------------------------------------------------------------------
// Private members
//----------------------------------------------------------------------------------------------
/**
 * Creates an empty registry
 *
 * m_names is initialized in the header
 */
FileLoaderRegistryImpl::FileLoaderRegistryImpl() : m_totalSize(0), m_log("FileLoaderRegistry") {}

FileLoaderRegistryImpl::~FileLoaderRegistryImpl() = default;

/**
 * @param name A string containing the algorithm name
 * @param version The version to remove. -1 indicates all instances
 * @param typedLoaders A map of names to version numbers
 **/
void FileLoaderRegistryImpl::removeAlgorithm(const std::string &name, const int version,
                                             std::multimap<std::string, int> &typedLoaders) {
  if (version == -1) // remove all
  {
    typedLoaders.erase(name);
  } else // find the right version
  {
    auto range = typedLoaders.equal_range(name);
    for (auto ritr = range.first; ritr != range.second; ++ritr) {
      if (ritr->second == version) {
        typedLoaders.erase(ritr);
        break;
      }
    }
  }
}

} // namespace Mantid::API
