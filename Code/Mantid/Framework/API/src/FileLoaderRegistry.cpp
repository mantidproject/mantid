#include "MantidAPI/FileLoaderRegistry.h"
#include "MantidAPI/IFileLoader.h"

#include <Poco/File.h>

namespace Mantid {
namespace API {
namespace {
//----------------------------------------------------------------------------------------------
// Anonymous namespace helpers
//----------------------------------------------------------------------------------------------
/// @cond
template <typename T> struct DescriptorCallback {
  void apply(T &) {} // general one does nothing
};
template <> struct DescriptorCallback<Kernel::FileDescriptor> {
  void apply(Kernel::FileDescriptor &descriptor) {
    descriptor.resetStreamToStart();
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
const IAlgorithm_sptr
searchForLoader(const std::string &filename,
                const std::multimap<std::string, int> &names,
                Kernel::Logger &logger) {
  const auto &factory = AlgorithmFactory::Instance();
  IAlgorithm_sptr bestLoader;
  int maxConfidence(0);
  DescriptorType descriptor(filename);
  DescriptorCallback<DescriptorType> callback;

  auto iend = names.end();
  for (auto it = names.begin(); it != iend; ++it) {
    const std::string &name = it->first;
    const int version = it->second;
    logger.debug() << "Checking " << name << " version " << version
                   << std::endl;

    // Use static cast for speed. Checks have been done at registration to check
    // the types
    auto alg = boost::static_pointer_cast<FileLoaderType>(
        factory.create(name, version)); // highest version
    try {
      const int confidence = alg->confidence(descriptor);
      logger.debug() << name << " returned with confidence=" << confidence
                     << std::endl;
      if (confidence > maxConfidence) // strictly greater
      {
        bestLoader = alg;
        maxConfidence = confidence;
      }
    } catch (std::exception &exc) {
      logger.warning() << "Checking loader '" << name << "' raised an error: '"
                       << exc.what() << "'. Loader skipped." << std::endl;
    }
    callback.apply(descriptor);
  }
  return bestLoader;
}
} // end anonymous

//----------------------------------------------------------------------------------------------
// Public members
//----------------------------------------------------------------------------------------------

/**
 * If the name does not exist then it does nothing
 * @param name Name of the algorithm to remove from the search list
 * @param version An optional version to remove. -1 indicates remove all
 * (Default=-1)
 */
void FileLoaderRegistryImpl::unsubscribe(const std::string &name,
                                         const int version) {
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
const boost::shared_ptr<IAlgorithm>
FileLoaderRegistryImpl::chooseLoader(const std::string &filename) const {
  using Kernel::FileDescriptor;
  using Kernel::NexusDescriptor;

  m_log.debug() << "Trying to find loader for '" << filename << "'"
                << std::endl;

  IAlgorithm_sptr bestLoader;
  if (NexusDescriptor::isHDF(filename)) {
    m_log.debug()
        << filename
        << " looks like a Nexus file. Checking registered Nexus loaders\n";
    bestLoader = searchForLoader<NexusDescriptor, IFileLoader<NexusDescriptor>>(
        filename, m_names[Nexus], m_log);
  } else {
    m_log.debug() << "Checking registered non-HDF loaders\n";
    bestLoader = searchForLoader<FileDescriptor, IFileLoader<FileDescriptor>>(
        filename, m_names[Generic], m_log);
  }

  if (!bestLoader) {
    throw Kernel::Exception::NotFoundError(filename, "Unable to find loader");
  }
  m_log.debug() << "Found loader " << bestLoader->name() << " for file '"
                << filename << "'" << std::endl;
  return bestLoader;
}

/**
 * Perform a check that that the given algorithm can load the file
 * @param algorithmName The name of the algorithm to check
 * @param filename The name of file to check
 * @returns True if the algorithm can load the file, false otherwise
 * @throws std::invalid_argument if the loader does not exist
 */
bool FileLoaderRegistryImpl::canLoad(const std::string &algorithmName,
                                     const std::string &filename) const {
  using Kernel::FileDescriptor;
  using Kernel::NexusDescriptor;

  // Check if it is in one of our lists
  bool nexus(false), nonHDF(false);
  if (m_names[Nexus].find(algorithmName) != m_names[Nexus].end())
    nexus = true;
  else if (m_names[Generic].find(algorithmName) != m_names[Generic].end())
    nonHDF = true;

  if (!nexus && !nonHDF)
    throw std::invalid_argument(
        "FileLoaderRegistryImpl::canLoad - Algorithm '" + algorithmName +
        "' is not registered as a loader.");

  std::multimap<std::string, int> names;
  names.insert(std::make_pair(algorithmName, -1));
  IAlgorithm_sptr loader;
  if (nexus && NexusDescriptor::isHDF(filename)) {
    loader = searchForLoader<NexusDescriptor, IFileLoader<NexusDescriptor>>(
        filename, names, m_log);
  } else {
    loader = searchForLoader<FileDescriptor, IFileLoader<FileDescriptor>>(
        filename, names, m_log);
  }
  if (loader)
    return true;
  else
    return false;
}

//----------------------------------------------------------------------------------------------
// Private members
//----------------------------------------------------------------------------------------------
/**
 * Creates an empty registry
 */
FileLoaderRegistryImpl::FileLoaderRegistryImpl()
    : m_names(2, std::multimap<std::string, int>()), m_totalSize(0),
      m_log("FileLoaderRegistry") {}

/**
 */
FileLoaderRegistryImpl::~FileLoaderRegistryImpl() {}

/**
 * @param name A string containing the algorithm name
 * @param version The version to remove. -1 indicates all instances
 * @param typedLoaders A map of names to version numbers
 **/
void FileLoaderRegistryImpl::removeAlgorithm(
    const std::string &name, const int version,
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

} // namespace API
} // namespace Mantid
