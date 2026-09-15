// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidDataHandling/SemanticVersion.h"

#include <algorithm>
#include <cctype>
#include <charconv>
#include <cstddef>
#include <format>
#include <stdexcept>

namespace Mantid {

namespace DataHandling {

namespace ILLNexus {

namespace {

// trimming spaces
std::string trim(std::string s) {
  auto not_space = [](unsigned char c) { return !std::isspace(c); };

  s.erase(s.begin(), std::find_if(s.begin(), s.end(), not_space));
  s.erase(std::find_if(s.rbegin(), s.rend(), not_space).base(), s.end());

  return s;
}

} // namespace

std::regex
    SemanticVersion::version_regex(R"(^(\d+)(?:\.(\d+))?(?:\.(\d+))?(?:-([0-9A-Za-z.-]+))?(?:\+([0-9A-Za-z.-]+))?$)");

/** Constructor using a full version string
 *
 * The version string is parsed and separated into major, minor, patch, etc.
 *
 * @param version The full version string

 * @throw std::runtime_error if parsing fails
 */
SemanticVersion::SemanticVersion(const std::string &version) : m_version(version) { parse_version(version); }

/** Constructor using the full version string from the indidual major, minor, patch, etc. values
 *
 * @param major Major number of the release
 * @param minor Minor number of the release
 * @param patch patch number
 * @param prerelease pre-release string
 * @param build build id string
 */
SemanticVersion::SemanticVersion(std::uint32_t major, std::uint32_t minor, std::uint32_t patch,
                                 const std::string &prerelease, const std::string &build)
    : m_major(major), m_minor(minor), m_patch(patch), m_prerelease(trim(prerelease)), m_build(trim(build)) {
  // cppcheck-suppress useInitializationList
  m_version = std::format("{}.{}.{}", major, minor, patch);
  if (!m_prerelease.empty())
    m_version = std::format("{}-{}", m_version, m_prerelease);

  if (!m_build.empty())
    m_version = std::format("{}+{}", m_version, m_build);
}

/** Check if two versions are equivalent
 *
 * the build string is not considered
 */
bool SemanticVersion::operator==(const SemanticVersion &other) const {
  return (m_major == other.m_major) && (m_minor == other.m_minor) && (m_patch == other.m_patch) &&
         (m_prerelease == other.m_prerelease);
}

/** Comparison operator
 *
 * the build string is not considered
 *
 * @param other Other SemanticVersion class
 */
std::strong_ordering SemanticVersion::operator<=>(const SemanticVersion &other) const {
  if (auto cmp = m_major <=> other.m_major; cmp != 0)
    return cmp;

  if (auto cmp = m_minor <=> other.m_minor; cmp != 0)
    return cmp;

  if (auto cmp = m_patch <=> other.m_patch; cmp != 0)
    return cmp;

  // According SemVer the build is not used to differentiate two versions so the comparison is stopped at the prerelease
  return m_prerelease <=> other.m_prerelease;
}

/** Comparison operator
 *
 * the build string is not considered
 *
 * @param otherVersionStr other version string
 */
std::strong_ordering SemanticVersion::operator<=>(const std::string &otherVersionStr) const {
  return *this <=> SemanticVersion(otherVersionStr);
}

const std::string &SemanticVersion::getBuild() const { return m_build; }

std::uint32_t SemanticVersion::getMajor() const { return m_major; }

std::uint32_t SemanticVersion::getMinor() const { return m_minor; }

std::uint32_t SemanticVersion::getPatch() const { return m_patch; }

const std::string &SemanticVersion::getPrerelease() const { return m_prerelease; }

const std::string &SemanticVersion::getVersion() const { return m_version; }

/** Check and split the version string into major, minor, patch, etc.
 *
 * @param version full version string
 */
void SemanticVersion::parse_version(const std::string &version) {

  std::smatch m;
  if (!std::regex_match(version, m, version_regex))
    throw std::runtime_error(std::format("Invalid version: {}", version));

  std::uint32_t value = 0;

  std::string_view sv = m[1].str();
  {
    auto [ptr, ec] = std::from_chars(sv.data(), sv.data() + sv.size(), value);
    if (ec != std::errc())
      throw std::runtime_error(std::format("Invalid major revision number: {}", m[1].str()));
  }
  m_major = value;

  if (m[2].matched) {
    sv = m[2].str();
    {
      auto [ptr, ec] = std::from_chars(sv.data(), sv.data() + sv.size(), value);
      if (ec != std::errc())
        throw std::runtime_error(std::format("Invalid minor revision number: {}", m[2].str()));
    }
    m_minor = value;
  }

  if (m[3].matched) {
    sv = m[3].str();
    {
      auto [ptr, ec] = std::from_chars(sv.data(), sv.data() + sv.size(), value);
      if (ec != std::errc())
        throw std::runtime_error(std::format("Invalid minor revision number: {}", m[3].str()));
    }
    m_patch = value;
  }

  // No need to trim the match, covered by the regex
  if (m[4].matched)
    m_prerelease = m[4].str();

  // No need to trim the match, covered by the regex
  if (m[5].matched)
    m_build = m[5].str();
}

} // end namespace ILLNexus

} // end namespace DataHandling

} // end namespace Mantid
