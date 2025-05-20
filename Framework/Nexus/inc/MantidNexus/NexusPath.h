// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2007 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidNexus/DllConfig.h"

#include <filesystem>

namespace NeXus {

/**
 * This simple class encapsulates some methods for working with paths inside a NeXus file.
 * The base is std::filesystem::path, but the root will always resolve to "/" regardless of OS.
 * New paths are always cast to lexically normal during creation.
 */
class MANTID_NEXUS_DLL NexusPath {

public:
  NexusPath(std::filesystem::path const &p);

  NexusPath(std::string const &p);

  NexusPath(char const *const p);

  NexusPath();

  NexusPath &operator=(NexusPath const &nd) = default;

  NexusPath &operator=(std::string const &s);

  NexusPath(NexusPath const &nd) = default;

  ~NexusPath() = default;

  bool operator==(NexusPath const &p) const;

  bool operator==(std::string const &s) const;

  bool operator==(char const *const s) const;

  bool operator!=(NexusPath const &p) const;

  bool operator!=(std::string const &s) const;

  bool operator!=(char const *const s) const;

  NexusPath operator/(std::string const &s) const;

  NexusPath operator/(char const *const s) const;

  NexusPath operator/(NexusPath const &p) const;

  NexusPath &operator/=(std::string const &s);

  NexusPath &operator/=(char const *const s);

  NexusPath &operator/=(NexusPath const &p);

  bool isAbsolute() const;

  bool isRoot() const;

  NexusPath parent_path() const;

  NexusPath fromRoot() const;

  NexusPath stem() const;

  std::string operator+(std::string const &s);

  operator std::string() const { return m_path.string(); }

  std::string string() const { return m_path.string(); }

  static NexusPath root();

private:
  /** standard filesystem path */
  std::filesystem::path m_path;
};

} // namespace NeXus

MANTID_NEXUS_DLL bool operator==(std::string const &s, NeXus::NexusPath const &p);

MANTID_NEXUS_DLL bool operator!=(std::string const &s, NeXus::NexusPath const &p);

MANTID_NEXUS_DLL std::string operator+(std::string const &s, NeXus::NexusPath const &p);

MANTID_NEXUS_DLL std::ostream &operator<<(std::ostream &os, NeXus::NexusPath const &p);
