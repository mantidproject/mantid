// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2007 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +

#include "MantidNexus/NexusPath.h"

namespace NeXus {

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

NexusPath::NexusPath(std::filesystem::path const &path) : m_path(path.lexically_normal()) {}

NexusPath::NexusPath(std::string const &path) : m_path(cleanup(path)) {}

NexusPath::NexusPath(char const *const path) : NexusPath(std::string(path)) {}

NexusPath::NexusPath() : m_path(nxroot) {}

NexusPath &NexusPath::operator=(std::string const &s) {
  m_path = cleanup(s);
  return *this;
}

bool NexusPath::operator==(NexusPath const &p) const { return m_path == p.m_path; }

bool NexusPath::operator==(std::string const &s) const { return m_path.string() == s; }

bool NexusPath::operator==(char const *const s) const { return m_path.string() == std::string(s); }

bool NexusPath::operator!=(NexusPath const &p) const { return m_path != p.m_path; }

bool NexusPath::operator!=(std::string const &s) const { return m_path.string() != s; }

bool NexusPath::operator!=(char const *const s) const { return m_path.string() != std::string(s); }

NexusPath NexusPath::operator/(std::string const &s) const { return NexusPath(m_path / s); }

NexusPath NexusPath::operator/(char const *const s) const { return *this / std::string(s); }

NexusPath NexusPath::operator/(NexusPath const &p) const { return NexusPath(m_path / p.m_path); }

NexusPath &NexusPath::operator/=(std::string const &s) {
  m_path /= s;
  return *this;
}

NexusPath &NexusPath::operator/=(char const *const s) { return *this /= std::string(s); }

NexusPath &NexusPath::operator/=(NexusPath const &p) {
  m_path /= p.m_path;
  return *this;
}

bool NexusPath::isAbsolute() const { return *m_path.begin() == nxroot; }

bool NexusPath::isRoot() const { return (m_path == nxroot); }

NexusPath NexusPath::parent_path() const { return NexusPath(m_path.parent_path()); }

NexusPath NexusPath::fromRoot() const { return NexusPath(nxroot / m_path); }

NexusPath NexusPath::stem() const { return NexusPath(m_path.stem()); }

NexusPath NexusPath::root() { return NexusPath(nxroot); }

std::string NexusPath::operator+(std::string const &s) { return m_path.string() + s; }

} // namespace NeXus

bool operator==(std::string const &s, NeXus::NexusPath const &p) { return s == std::string(p); }

bool operator!=(std::string const &s, NeXus::NexusPath const &p) { return s != std::string(p); }

std::string operator+(std::string const &s, NeXus::NexusPath const &p) { return s + std::string(p); }

std::ostream &operator<<(std::ostream &os, NeXus::NexusPath const &p) {
  os << std::string(p);
  return os;
}
