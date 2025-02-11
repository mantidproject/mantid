// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2013 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidKernel/DllConfig.h"

#include <map>
#include <memory>
#include <string>
#include <unordered_set>
#include <utility>
#include <vector>

namespace Mantid::LegacyNexus {
class File;
}

namespace Mantid {
namespace Kernel {

/**
    Defines a wrapper around a file whose internal structure can be accessed
   using the NeXus API

    On construction the simple details about the layout of the file are cached
   for faster querying later.
 */
class MANTID_KERNEL_DLL NexusDescriptor {

public:
  /// Constructor accepting a filename
  NexusDescriptor(const std::string &filename);
  /// Destructor
  ~NexusDescriptor();

  /// Disable default constructor
  NexusDescriptor() = delete;

  /// Disable copy operator
  NexusDescriptor(const NexusDescriptor &) = delete;

  /// Disable assignment operator
  NexusDescriptor &operator=(const NexusDescriptor &) = delete;

  /**
   * Access the filename
   * @returns A reference to a const string containing the filename
   */
  inline const std::string &filename() const { return m_filename; }
  /**
   * Access the file extension. Defined as the string after and including the
   * last period character
   * @returns A reference to a const string containing the file extension
   */
  inline const std::string &extension() const { return m_extension; }
  /**
   * Access the open NeXus File object
   * @returns A reference to the open ::NeXus file object
   */
  inline Mantid::LegacyNexus::File &data() { return *m_file; }

  /// Returns the name & type of the first entry in the file
  const std::pair<std::string, std::string> &firstEntryNameType() const;
  /// Query if a path exists
  bool pathExists(const std::string &path) const;

private:
  /// Initialize object with filename
  void initialize(const std::string &filename);
  /// Walk the tree and cache the structure
  void walkFile(Mantid::LegacyNexus::File &file, const std::string &rootPath, const std::string &className,
                std::map<std::string, std::string> &pmap, int level);

  /// Full filename
  std::string m_filename;
  /// Extension
  std::string m_extension;
  /// First entry name/type
  std::pair<std::string, std::string> m_firstEntryNameType;
  /// Root attributes
  std::unordered_set<std::string> m_rootAttrs;
  /// Map of full path strings to types. Can check if path exists quickly
  std::map<std::string, std::string> m_pathsToTypes;

  /// Open NeXus handle
  std::unique_ptr<Mantid::LegacyNexus::File> m_file;
};

} // namespace Kernel
} // namespace Mantid
