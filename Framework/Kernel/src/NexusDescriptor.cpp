// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidKernel/NexusDescriptor.h"

// clang-format off
#include <nexus/NeXusFile.hpp>
#include <nexus/NeXusException.hpp>
// clang-format on

#include <filesystem>
#include <Poco/Path.h>

#include <algorithm>
#include <cstring>
#include <string>

namespace Mantid::Kernel {
//---------------------------------------------------------------------------------------------------------------------------
// static NexusDescriptor constants
//---------------------------------------------------------------------------------------------------------------------------
/// Size of HDF magic number
const size_t NexusDescriptor::HDFMagicSize = 4;
/// HDF cookie that is stored in the first 4 bytes of the file.
const unsigned char NexusDescriptor::HDFMagic[4] = {'\016', '\003', '\023', '\001'}; // From HDF4::hfile.h

/// Size of HDF5 signature
size_t NexusDescriptor::HDF5SignatureSize = 8;
/// signature identifying a HDF5 file.
const unsigned char NexusDescriptor::HDF5Signature[8] = {137, 'H', 'D', 'F', '\r', '\n', '\032', '\n'};

namespace {
//---------------------------------------------------------------------------------------------------------------------------
// Anonymous helper methods to use isReadable methods to use an open file handle
//---------------------------------------------------------------------------------------------------------------------------

/**
 * Currently simply checks for the HDF signatures and returns true if one of
 * them is found
 * @param fileHandle A file handled opened and pointing at the start of the
 * file. On return the
 * fileHandle is left at the start of the file
 * @param version One of the NexusDescriptor::Version enumerations specifying
 * the required version
 * @return True if the file is considered hierarchical, false otherwise
 */
bool isHDFHandle(FILE *fileHandle, NexusDescriptor::Version version) {
  if (!fileHandle)
    throw std::invalid_argument("HierarchicalFileDescriptor::isHierarchical - Invalid file handle");

  bool result(false);

  // HDF4 check requires 4 bytes,  HDF5 check requires 8 bytes
  // Use same buffer and waste a few bytes if only checking HDF4
  unsigned char buffer[8] = {'0', '0', '0', '0', '0', '0', '0', '0'};
  if (NexusDescriptor::HDF5SignatureSize !=
      std::fread(static_cast<void *>(&buffer), sizeof(unsigned char), NexusDescriptor::HDF5SignatureSize, fileHandle)) {
    throw std::runtime_error("Error while reading file");
  }

  // Number of bytes read doesn't matter as if it is not enough then the memory
  // simply won't match
  // as the buffer has been "zeroed"
  if (version == NexusDescriptor::Version5 || version == NexusDescriptor::AnyVersion) {
    result = (std::memcmp(&buffer, &NexusDescriptor::HDF5Signature, NexusDescriptor::HDF5SignatureSize) == 0);
  }
  if (!result && (version == NexusDescriptor::Version4 || version == NexusDescriptor::AnyVersion)) {
    result = (std::memcmp(&buffer, &NexusDescriptor::HDFMagic, NexusDescriptor::HDFMagicSize) == 0);
  }

  // Return file stream to start of file
  std::rewind(fileHandle);
  return result;
}
} // namespace

//---------------------------------------------------------------------------------------------------------------------------
// static NexusDescriptor methods
//---------------------------------------------------------------------------------------------------------------------------

/**
 * Checks for the HDF signatures and returns true if one of them is found
 * @param filename A string filename to check
 * @param version One of the NexusDescriptor::Version enumerations specifying
 * the required version
 * @return True if the file is considered hierarchical, false otherwise
 */
bool NexusDescriptor::isReadable(const std::string &filename, const Version version) {
  FILE *fd = fopen(filename.c_str(), "rb");
  if (!fd) {
    throw std::invalid_argument("HierarchicalFileDescriptor::isHierarchical - Unable to open file '" + filename + "'");
  }
  const bool result = isHDFHandle(fd, version); // use anonymous helper
  fclose(fd);
  return result;
}

//---------------------------------------------------------------------------------------------------------------------------
// NexusDescriptor public methods
//---------------------------------------------------------------------------------------------------------------------------
/**
 * Constructs the wrapper
 * @param filename A string pointing to an existing file
 * @param init Whether or not to initialize the file
 * @throws std::invalid_argument if the file is not identified to be
 * hierarchical. This currently
 * involves simply checking for the signature if a HDF file at the start of the
 * file
 */
NexusDescriptor::NexusDescriptor(const std::string &filename, const bool init)
    : m_filename(), m_extension(), m_firstEntryNameType(), m_rootAttrs(), m_pathsToTypes(), m_file(nullptr) {
  if (filename.empty()) {
    throw std::invalid_argument("NexusDescriptor() - Empty filename '" + filename + "'");
  }
  if (!std::filesystem::exists(filename)) {
    throw std::invalid_argument("NexusDescriptor() - File '" + filename + "' does not exist");
  }

  if (init) {
    try {
      // this is very expesive as it walk the entire file
      initialize(filename);
    } catch (::NeXus::Exception &e) {
      throw std::invalid_argument("NexusDescriptor::initialize - File '" + filename +
                                  "' does not look like a HDF file.\n Error was: " + e.what());
    }
  }
}

NexusDescriptor::~NexusDescriptor() = default;

/// Returns the name & type of the first entry in the file
const std::pair<std::string, std::string> &NexusDescriptor::firstEntryNameType() const { return m_firstEntryNameType; }

/**
 * @param name The name of an attribute
 * @return True if the attribute exists, false otherwise
 */
bool NexusDescriptor::hasRootAttr(const std::string &name) const { return (m_rootAttrs.count(name) == 1); }

/**
 * @param path A string giving a path using UNIX-style path separators (/), e.g.
 * /raw_data_1, /entry/bank1
 * @return True if the path exists in the file, false otherwise
 */
bool NexusDescriptor::pathExists(const std::string &path) const {
  return (m_pathsToTypes.find(path) != m_pathsToTypes.end());
}

/**
 * @param path A string giving a path using UNIX-style path separators (/), e.g.
 * /raw_data_1, /entry/bank1
 * @param type A string specifying the required type
 * @return True if the path exists in the file, false otherwise
 */
bool NexusDescriptor::pathOfTypeExists(const std::string &path, const std::string &type) const {
  auto it = m_pathsToTypes.find(path);
  if (it != m_pathsToTypes.end()) {
    return (it->second == type);
  } else
    return false;
}

/**
 * @param type A string specifying the required type
 * @return path A string giving a path using UNIX-style path separators (/),
 * e.g. /raw_data_1, /entry/bank1
 */
std::string NexusDescriptor::pathOfType(const std::string &type) const {
  const auto it = std::find_if(m_pathsToTypes.cbegin(), m_pathsToTypes.cend(),
                               [&type](const auto &typeMap) { return type == typeMap.second; });
  if (it != m_pathsToTypes.cend()) {
    return it->first;
  }
  return "";
}

/**
 * @param type A string specifying the required type
 * @return path A vector of strings giving paths using UNIX-style path
 * separators (/), e.g. /raw_data_1, /entry/bank1
 */
std::vector<std::string> NexusDescriptor::allPathsOfType(const std::string &type) const {
  auto iend = m_pathsToTypes.end();
  std::vector<std::string> retval;
  for (auto it = m_pathsToTypes.begin(); it != iend; ++it) {
    if (type == it->second)
      retval.push_back(it->first);
  }
  return retval;
}

/**
 * @param classType A string name giving a class type
 * @return True if the type exists in the file, false otherwise
 */
bool NexusDescriptor::classTypeExists(const std::string &classType) const {
  return std::any_of(m_pathsToTypes.cbegin(), m_pathsToTypes.cend(),
                     [&classType](const auto typeMap) { return classType == typeMap.second; });
}

//---------------------------------------------------------------------------------------------------------------------------
// NexusDescriptor private methods
//---------------------------------------------------------------------------------------------------------------------------

/**
 * Creates the internal cached structure of the file as a tree of nodes
 */
void NexusDescriptor::initialize(const std::string &filename) {
  m_filename = filename;
  m_extension = "." + Poco::Path(filename).getExtension();

  m_file = std::make_unique<::NeXus::File>(this->filename());

  m_file->openPath("/");
  m_rootAttrs.clear();
  m_pathsToTypes.clear();
  walkFile(*m_file, "", "", m_pathsToTypes, 0);
}

/**
 * Cache the structure in the given maps
 * @param file An open NeXus File object
 * @param rootPath The current path that is open in the file
 * @param className The class of the current open path
 * @param pmap [Out] An output map filled with mappings of path->type
 * @param level An integer defining the current level in the file
 */
void NexusDescriptor::walkFile(::NeXus::File &file, const std::string &rootPath, const std::string &className,
                               std::map<std::string, std::string> &pmap, int level) {
  if (!rootPath.empty()) {
    pmap.emplace(rootPath, className);
  }
  if (level == 0) {
    auto attrInfos = file.getAttrInfos();
    for (auto &attrInfo : attrInfos) {
      m_rootAttrs.insert(attrInfo.name);
    }
  }

  auto dirents = file.getEntries();
  auto itend = dirents.end();
  for (auto it = dirents.begin(); it != itend; ++it) {
    const std::string &entryName = it->first;
    const std::string &entryClass = it->second;
    const std::string entryPath = std::string(rootPath).append("/").append(entryName);
    if (entryClass == "SDS" || entryClass == "ILL_data_scan_vars" || entryClass == "NXill_data_scan_vars") {
      pmap.emplace(entryPath, entryClass);
    } else if (entryClass == "CDF0.0") {
      // Do nothing with this
    } else {
      if (level == 0)
        m_firstEntryNameType = (*it); // copy first entry name & type
      if (!entryClass.empty()) {      // handles buggy files
        file.openGroup(entryName, entryClass);
        walkFile(file, entryPath, entryClass, pmap, level + 1);
      }
    }
  }
  file.closeGroup();
}

} // namespace Mantid::Kernel
