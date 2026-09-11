// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2007 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidNexus/DllConfig.h"

#include <memory>
#include <string>
#include <vector>

namespace Mantid::Nexus {

/**
 * This simple class encapsulates some methods for working with paths inside a Nexus file.
 * Paths are stored as plain std::string; the root is always "/".
 * Trailing slashes are stripped on construction (except for the root itself).
 */
class MANTID_NEXUS_DLL NexusAddress {

public:
  NexusAddress(std::string const &p);

  NexusAddress(char const *const p);

  NexusAddress();

  NexusAddress &operator=(NexusAddress const &nd) = default;

  NexusAddress &operator=(std::string const &s);

  NexusAddress(NexusAddress const &nd) = default;

  ~NexusAddress() = default;

  // comparison operators

  bool operator==(NexusAddress const &p) const;

  bool operator==(std::string const &s) const;

  bool operator==(char const *const s) const;

  bool operator!=(NexusAddress const &p) const;

  bool operator!=(std::string const &s) const;

  bool operator!=(char const *const s) const;

  // concatenation

  NexusAddress operator/(std::string const &s) const;

  NexusAddress operator/(char const *const s) const;

  NexusAddress operator/(NexusAddress const &p) const;

  NexusAddress &operator/=(std::string const &s);

  NexusAddress &operator/=(char const *const s);

  NexusAddress &operator/=(NexusAddress const &p);

  bool hasChild(std::string const &p) const;

  // In-place mutation — no heap allocation when string capacity is sufficient.
  void appendComponent(std::string const &name);
  void popComponent();

  // access

  bool isAbsolute() const;

  bool isRoot() const;

  NexusAddress parent_path() const;

  NexusAddress fromRoot() const;

  NexusAddress stem() const;

  static NexusAddress root();

  std::vector<std::string> parts() const;

  // printing

  std::string operator+(std::string const &s) const;

  std::string operator+(char const s[]) const;

  operator std::string() const { return m_path; }

  std::string const &string() const { return m_path; }

  char const *c_str() const { return m_path.c_str(); }

private:
  std::string m_path;
};

} // namespace Mantid::Nexus

MANTID_NEXUS_DLL bool operator==(std::string const &s, Mantid::Nexus::NexusAddress const &p);

MANTID_NEXUS_DLL bool operator!=(std::string const &s, Mantid::Nexus::NexusAddress const &p);

MANTID_NEXUS_DLL std::string operator+(std::string const &s, Mantid::Nexus::NexusAddress const &p);

MANTID_NEXUS_DLL std::string operator+(char const s[], Mantid::Nexus::NexusAddress const &p);

MANTID_NEXUS_DLL std::ostream &operator<<(std::ostream &os, Mantid::Nexus::NexusAddress const &p);
