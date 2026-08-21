// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2007 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +

#include "MantidNexus/NexusAddress.h"

#include <ostream>
#include <string>
#include <string_view>

namespace Mantid::Nexus {

namespace {

// Lexically normalises a NeXus path string:
//   - resolves ".." (pop last component) and skips "." and empty components
//   - strips trailing slashes
//   - always returns an absolute path when input was absolute, "/"  for root
std::string normalize(std::string const &s) {
  if (s.empty())
    return "";
  bool const is_absolute = (s[0] == '/');
  std::vector<std::string> components;

  std::size_t pos = is_absolute ? 1 : 0;
  while (pos < s.size()) {
    auto const slash = s.find('/', pos);
    auto const end = (slash == std::string::npos) ? s.size() : slash;
    auto const component = s.substr(pos, end - pos);
    pos = end + 1;

    if (component == "..") {
      if (!components.empty())
        components.pop_back();
    } else if (!component.empty() && component != ".") {
      components.push_back(component);
    }
  }

  if (components.empty())
    return is_absolute ? "/" : "";

  std::string result;
  result.reserve(s.size());
  for (auto const &c : components) {
    result += '/';
    result += c;
  }
  if (!is_absolute)
    result = result.substr(1); // relative path: strip the leading '/' we added
  return result;
}

} // namespace

NexusAddress::NexusAddress(std::string const &path) : m_path(normalize(path)) {}

NexusAddress::NexusAddress(char const *const path) : m_path(normalize(path)) {}

NexusAddress::NexusAddress() : m_path("/") {}

NexusAddress &NexusAddress::operator=(std::string const &s) {
  m_path = normalize(s);
  return *this;
}

bool NexusAddress::operator==(NexusAddress const &p) const { return m_path == p.m_path; }

bool NexusAddress::operator==(std::string const &s) const { return m_path == s; }

bool NexusAddress::operator==(char const *const s) const { return m_path == s; }

bool NexusAddress::operator!=(NexusAddress const &p) const { return m_path != p.m_path; }

bool NexusAddress::operator!=(std::string const &s) const { return m_path != s; }

bool NexusAddress::operator!=(char const *const s) const { return m_path != s; }

NexusAddress NexusAddress::operator/(std::string const &s) const { return *this / NexusAddress(s); }

NexusAddress NexusAddress::operator/(char const *const s) const { return *this / NexusAddress(s); }

NexusAddress NexusAddress::operator/(NexusAddress const &p) const {
  if (p.isRoot())
    return NexusAddress(m_path);
  // strip leading '/' from rhs so we can append cleanly
  std::string_view const rhs = p.isAbsolute() ? std::string_view(p.m_path).substr(1) : std::string_view(p.m_path);
  std::string result = (m_path == "/") ? "/" : m_path + "/";
  result.append(rhs);
  return NexusAddress(std::move(result));
}

NexusAddress &NexusAddress::operator/=(std::string const &s) { return *this /= NexusAddress(s); }

NexusAddress &NexusAddress::operator/=(char const *const s) { return *this /= NexusAddress(s); }

NexusAddress &NexusAddress::operator/=(NexusAddress const &p) {
  *this = *this / p;
  return *this;
}

bool NexusAddress::isAbsolute() const { return !m_path.empty() && m_path[0] == '/'; }

bool NexusAddress::isRoot() const { return m_path == "/"; }

NexusAddress NexusAddress::parent_path() const {
  auto const pos = m_path.rfind('/');
  if (pos == std::string::npos || pos == 0)
    return NexusAddress("/");
  return NexusAddress(m_path.substr(0, pos));
}

NexusAddress NexusAddress::fromRoot() const {
  if (isAbsolute())
    return NexusAddress(m_path);
  return NexusAddress("/" + m_path);
}

NexusAddress NexusAddress::stem() const {
  if (m_path == "/")
    return NexusAddress("");
  auto const pos = m_path.rfind('/');
  if (pos == std::string::npos)
    return NexusAddress(m_path);
  return NexusAddress(m_path.substr(pos + 1));
}

NexusAddress NexusAddress::root() { return NexusAddress("/"); }

std::vector<std::string> NexusAddress::parts() const {
  std::vector<std::string> result;
  std::size_t pos = (m_path[0] == '/') ? 1 : 0;
  while (pos < m_path.size()) {
    auto const next = m_path.find('/', pos);
    auto const end = (next == std::string::npos) ? m_path.size() : next;
    result.push_back(m_path.substr(pos, end - pos));
    pos = end + 1;
  }
  return result;
}

void NexusAddress::appendComponent(std::string const &name) {
  if (!m_path.empty() && m_path.back() != '/')
    m_path += '/';
  m_path += name;
}

void NexusAddress::popComponent() {
  auto const pos = m_path.rfind('/');
  if (pos == std::string::npos) {
    // no separator to pop back to -- leave a slash-free relative path empty
    m_path.clear();
  } else {
    m_path.resize(pos == 0 ? 1 : pos);
  }
}

bool NexusAddress::hasChild(std::string const &child) const {
  if (child.empty() || m_path == child)
    return false;

  if (isRoot()) {
    if (child.size() < 2 || child[0] != '/' || child.find('/', 1) != std::string::npos)
      return false;
    return true;
  }

  std::size_t const parent_size = m_path.size();
  if (child.size() <= parent_size + 1)
    return false;
  if (child.compare(0, parent_size, m_path) != 0)
    return false;
  if (child[parent_size] != '/')
    return false;
  return child.find('/', parent_size + 1) == std::string::npos;
}

std::string NexusAddress::operator+(std::string const &s) const { return m_path + s; }

std::string NexusAddress::operator+(char const s[]) const { return m_path + s; }

} // namespace Mantid::Nexus

bool operator==(std::string const &s, Mantid::Nexus::NexusAddress const &p) { return s == p.string(); }

bool operator!=(std::string const &s, Mantid::Nexus::NexusAddress const &p) { return s != p.string(); }

std::string operator+(std::string const &s, Mantid::Nexus::NexusAddress const &p) { return s + p.string(); }

std::string operator+(char const s[], Mantid::Nexus::NexusAddress const &p) { return s + p.string(); }

std::ostream &operator<<(std::ostream &os, Mantid::Nexus::NexusAddress const &p) {
  os << p.string();
  return os;
}
