// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidKernel/NexusDescriptor.h"
#include "MantidNexusCpp/NeXusException.hpp"
#include "MantidNexusCpp/NeXusFile.hpp"

#include <algorithm>
#include <cstring>
#include <filesystem>
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
  if (version == NexusDescriptor::Version5) {
    result = (std::memcmp(&buffer, &NexusDescriptor::HDF5Signature, NexusDescriptor::HDF5SignatureSize) == 0);
  }
  if (!result && (version == NexusDescriptor::Version4)) {
    result = (std::memcmp(&buffer, &NexusDescriptor::HDFMagic, NexusDescriptor::HDFMagicSize) == 0);
  }

  // Return file stream to start of file
  std::rewind(fileHandle);
  return result;
}

NexusDescriptor::Version HDFversion(FILE *fileHandle) {
  if (!fileHandle)
    throw std::invalid_argument("HierarchicalFileDescriptor::isHierarchical - Invalid file handle");

  // HDF4 check requires 4 bytes,  HDF5 check requires 8 bytes
  // Use same buffer and waste a few bytes if only checking HDF4
  unsigned char buffer[8] = {'0', '0', '0', '0', '0', '0', '0', '0'};
  if (NexusDescriptor::HDF5SignatureSize !=
      std::fread(static_cast<void *>(&buffer), sizeof(unsigned char), NexusDescriptor::HDF5SignatureSize, fileHandle)) {
    throw std::runtime_error("Error while reading file");
  }

  NexusDescriptor::Version result = NexusDescriptor::None;

  // Number of bytes read doesn't matter as if it is not enough then the memory
  // simply won't match
  // as the buffer has been "zeroed"
  if (std::memcmp(&buffer, &NexusDescriptor::HDF5Signature, NexusDescriptor::HDF5SignatureSize) == 0)
    result = NexusDescriptor::Version5;
  else if (std::memcmp(&buffer, &NexusDescriptor::HDFMagic, NexusDescriptor::HDFMagicSize) == 0)
    result = NexusDescriptor::Version4;

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

NexusDescriptor::Version NexusDescriptor::getHDFVersion(const std::string &filename) {
  FILE *fd = fopen(filename.c_str(), "rb");
  if (!fd) {
    throw std::invalid_argument("HierarchicalFileDescriptor::isHierarchical - Unable to open file '" + filename + "'");
  }
  const Version result = HDFversion(fd); // use anonymous helper
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
 * @param path A string giving a path using UNIX-style path separators (/), e.g.
 * /raw_data_1, /entry/bank1
 * @return True if the path exists in the file, false otherwise
 */
bool NexusDescriptor::pathExists(const std::string &path) const {
  return (m_pathsToTypes.find(path) != m_pathsToTypes.end());
}

//---------------------------------------------------------------------------------------------------------------------------
// NexusDescriptor private methods
//---------------------------------------------------------------------------------------------------------------------------

/**
 * Creates the internal cached structure of the file as a tree of nodes
 */
void NexusDescriptor::initialize(const std::string &filename) {
  m_filename = filename;
  m_extension = std::filesystem::path(filename).extension().string();

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
