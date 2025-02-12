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
