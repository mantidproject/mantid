// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2007 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +

#include "MantidNexus/NexusAddress.h"

#include <filesystem>
#include <string>

namespace Mantid::Nexus {

namespace {
std::filesystem::path const nxroot("/");

std::filesystem::path cleanup(std::string const &s) {
  std::filesystem::path ret(s);
  if (ret == nxroot) {
    // the root path is already clean
  } else if (s.back() == '/') {
    // make sure no entries end in "/" -- this confuses path location
    ret = s.substr(0, s.size() - 1);
  }
  if (s.starts_with("//")) {
    ret = s.substr(1);
  }
  // cast as lexically normal
  return ret.lexically_normal();
}

std::filesystem::path cleanup(std::filesystem::path const &p) { return cleanup(p.string()); }

} // namespace

NexusAddress::NexusAddress(std::filesystem::path const &path)
    : m_path(cleanup(path)), m_resolved_path(m_path.generic_string()) {}

NexusAddress::NexusAddress(std::string const &path) : m_path(cleanup(path)), m_resolved_path(m_path.generic_string()) {}

NexusAddress::NexusAddress(char const *const path) : NexusAddress(std::string(path)) {}

NexusAddress::NexusAddress() : m_path(nxroot), m_resolved_path(m_path.generic_string()) {}

NexusAddress &NexusAddress::operator=(std::string const &s) {
  m_path = cleanup(s);
  m_resolved_path = std::string(m_path.generic_string());
  return *this;
}

bool NexusAddress::operator==(NexusAddress const &p) const { return m_path == p.m_path; }

bool NexusAddress::operator==(std::string const &s) const { return m_resolved_path == s; }

bool NexusAddress::operator==(char const *const s) const { return m_resolved_path == std::string(s); }

bool NexusAddress::operator!=(NexusAddress const &p) const { return m_path != p.m_path; }

bool NexusAddress::operator!=(std::string const &s) const { return m_resolved_path != s; }

bool NexusAddress::operator!=(char const *const s) const { return m_resolved_path != std::string(s); }

NexusAddress NexusAddress::operator/(std::string const &s) const { return *this / NexusAddress(s); }

NexusAddress NexusAddress::operator/(char const *const s) const { return *this / NexusAddress(s); }

NexusAddress NexusAddress::operator/(NexusAddress const &p) const {
  return NexusAddress(m_path / p.m_path.relative_path());
}

NexusAddress &NexusAddress::operator/=(std::string const &s) { return *this /= NexusAddress(s); }

NexusAddress &NexusAddress::operator/=(char const *const s) { return *this /= NexusAddress(s); }

NexusAddress &NexusAddress::operator/=(NexusAddress const &p) {
  m_path = cleanup(m_path / p.m_path);
  m_resolved_path = std::string(m_path.generic_string());
  return *this;
}

bool NexusAddress::isAbsolute() const { return *m_path.begin() == nxroot; }

bool NexusAddress::isRoot() const { return (m_path == nxroot); }

NexusAddress NexusAddress::parent_path() const { return NexusAddress(m_path.parent_path()); }

NexusAddress NexusAddress::fromRoot() const { return NexusAddress(nxroot / m_path); }

NexusAddress NexusAddress::stem() const { return NexusAddress(m_path.filename()); }

NexusAddress NexusAddress::root() { return NexusAddress(nxroot); }

std::string NexusAddress::operator+(std::string const &s) const { return m_resolved_path + s; }

std::string NexusAddress::operator+(char const s[]) const { return m_resolved_path + s; }

std::vector<std::string> NexusAddress::parts() const {
  std::vector<std::string> names;
  for (auto it = m_path.begin(); it != m_path.end(); it++) {
    if (*it != nxroot) {
      names.push_back(it->generic_string());
    }
  }
  return names;
}

bool NexusAddress::hasChild(std::string const &child) const {
  // if child is empty
  if (child.empty() || m_resolved_path == child)
    return false;

  // if at root, must check specially
  if (isRoot()) {
    // child must be "/something" and not contain another '/'
    if (child.size() < 2 || child[0] != '/' || child.find('/', 1) != std::string::npos)
      return false;
    return true;
  }

  // parent must be a prefix, followed by a single '/'
  std::size_t const parent_size = m_resolved_path.size();
  if (child.size() <= parent_size + 1)
    return false;
  if (child.compare(0, parent_size, m_resolved_path) != 0)
    return false;
  if (child[parent_size] != '/')
    return false;

  // there must be no further '/' after the immediate child
  auto next_slash = child.find('/', parent_size + 1);
  return next_slash == std::string::npos;
}

} // namespace Mantid::Nexus

bool operator==(std::string const &s, Mantid::Nexus::NexusAddress const &p) { return s == p.string(); }

bool operator!=(std::string const &s, Mantid::Nexus::NexusAddress const &p) { return s != p.string(); }

std::string operator+(std::string const &s, Mantid::Nexus::NexusAddress const &p) { return s + p.string(); }

std::string operator+(char const s[], Mantid::Nexus::NexusAddress const &p) { return s + p.string(); }

std::ostream &operator<<(std::ostream &os, Mantid::Nexus::NexusAddress const &p) {
  os << p.string();
  return os;
}
