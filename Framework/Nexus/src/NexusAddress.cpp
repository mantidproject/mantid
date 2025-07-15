// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2007 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +

#include "MantidNexus/NexusAddress.h"

namespace Mantid::Nexus {

namespace {
std::filesystem::path const nxroot("/");

std::filesystem::path cleanup(std::string const &s) {
  std::filesystem::path ret(s);
  std::string ns(s);
  if (ret == nxroot) {
    // the root path is already clean
  } else if (ns.back() == '/') {
    // make sure no entries end in "/" -- this confuses path location
    ns.pop_back();
    ret = ns;
  }
  // cast as lexically normal
  return ret.lexically_normal();
}
} // namespace

NexusAddress::NexusAddress(std::filesystem::path const &path)
    : m_path(path.lexically_normal()), m_resolved_path(m_path.generic_string()) {}

NexusAddress::NexusAddress(std::string const &path) : m_path(cleanup(path)), m_resolved_path(m_path.generic_string()) {}

NexusAddress::NexusAddress(char const *const path) : NexusAddress(std::string(path)) {}

NexusAddress::NexusAddress() : m_path(nxroot), m_resolved_path(m_path.generic_string()) {}

NexusAddress &NexusAddress::operator=(std::string const &s) {
  m_path = cleanup(s);
  m_resolved_path = m_path.generic_string();
  return *this;
}

bool NexusAddress::operator==(NexusAddress const &p) const { return m_path == p.m_path; }

bool NexusAddress::operator==(std::string const &s) const { return m_path.string() == s; }

bool NexusAddress::operator==(char const *const s) const { return m_path.string() == std::string(s); }

bool NexusAddress::operator!=(NexusAddress const &p) const { return m_path != p.m_path; }

bool NexusAddress::operator!=(std::string const &s) const { return m_path.string() != s; }

bool NexusAddress::operator!=(char const *const s) const { return m_path.string() != std::string(s); }

NexusAddress NexusAddress::operator/(std::string const &s) const { return NexusAddress(m_path / s); }

NexusAddress NexusAddress::operator/(char const *const s) const { return *this / std::string(s); }

NexusAddress NexusAddress::operator/(NexusAddress const &p) const { return NexusAddress(m_path / p.m_path); }

NexusAddress &NexusAddress::operator/=(std::string const &s) {
  m_path /= s;
  m_resolved_path = m_path.generic_string();
  return *this;
}

NexusAddress &NexusAddress::operator/=(char const *const s) { return *this /= std::string(s); }

NexusAddress &NexusAddress::operator/=(NexusAddress const &p) {
  m_path /= p.m_path;
  m_resolved_path = m_path.generic_string();
  return *this;
}

bool NexusAddress::isAbsolute() const { return *m_path.begin() == nxroot; }

bool NexusAddress::isRoot() const { return (m_path == nxroot); }

NexusAddress NexusAddress::parent_path() const { return NexusAddress(m_path.parent_path()); }

NexusAddress NexusAddress::fromRoot() const { return NexusAddress(nxroot / m_path); }

NexusAddress NexusAddress::stem() const { return NexusAddress(m_path.stem()); }

NexusAddress NexusAddress::root() { return NexusAddress(nxroot); }

std::string NexusAddress::operator+(std::string const &s) const { return m_path.string() + s; }

std::vector<std::string> NexusAddress::parts() const {
  std::vector<std::string> names;
  for (auto it = m_path.begin(); it != m_path.end(); it++) {
    if (*it != nxroot) {
      names.push_back(*it);
    }
  }
  return names;
}
} // namespace Mantid::Nexus

bool operator==(std::string const &s, Mantid::Nexus::NexusAddress const &p) { return s == std::string(p); }

bool operator!=(std::string const &s, Mantid::Nexus::NexusAddress const &p) { return s != std::string(p); }

std::string operator+(std::string const &s, Mantid::Nexus::NexusAddress const &p) { return s + std::string(p); }

std::ostream &operator<<(std::ostream &os, Mantid::Nexus::NexusAddress const &p) {
  os << std::string(p);
  return os;
}
