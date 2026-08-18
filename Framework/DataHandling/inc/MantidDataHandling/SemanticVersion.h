// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2008 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidDataHandling/DllConfig.h"

#include <compare>
#include <cstdint>
#include <regex>
#include <string>

namespace Mantid {

namespace DataHandling {

namespace ILLNexus {

/**
 * @class SemanticVersion
 * @brief Implementation of a SemVer parser.
 *
 * It supports th following formats :
 * - X
 * - X.Y
 * - X.Y.Z
 * - X.Y.Z-identifier
 *
 * The implementation is not strict in the sense that:
 * - minor = 0 if absent
 * - patch = 0 if absent
 * - identifier = "" if absent
 *
 * so e.g.:
 * - 3 --> major = 3, minor = 0, patch = 0, identifier = ""
 * - 3.12 --> major = 3, minor = 12, patch = 0, identifier = ""
 * - 3.12.3 --> major = 3, minor = 12, patch = 3, identifier = ""
 * - 3.12.3-myid --> major = 3, minor = 12, patch = 3, identifier = "myid"
 */
class MANTID_DATAHANDLING_DLL SemanticVersion {

public:
  // The SemVer regex
  static std::regex version_regex;

  // Default constructor
  SemanticVersion() = delete;

  // Copy constructor
  SemanticVersion(const SemanticVersion &other) = default;

  // Constructor from string
  SemanticVersion(const std::string &);

  // Constructor from major, minor, patch and identifier;
  SemanticVersion(std::uint32_t major = 0, std::uint32_t minor = 0, std::uint32_t patch = 0,
                  const std::string &prerelease = "", const std::string &build = "");

  // Destructor
  ~SemanticVersion() = default;

  // Assignment operators
  SemanticVersion &operator=(const SemanticVersion &other) = default;

  // Equality operator
  bool operator==(const SemanticVersion &other) const;

  // Comparison operators
  std::strong_ordering operator<=>(const SemanticVersion &other) const;
  std::strong_ordering operator<=>(const std::string &versionStr) const;

  // Returns the build
  const std::string &getBuild() const;

  // Returns the major version
  std::uint32_t getMajor() const;

  // Returns the minor version
  std::uint32_t getMinor() const;

  // Returns the patch version
  std::uint32_t getPatch() const;

  // Returns the prerelease
  const std::string &getPrerelease() const;

  // Returns the version
  const std::string &getVersion() const;

private:
  // Proceeds to the internal validation of the version
  void parse_version(const std::string &);

  // The full version
  std::string m_version = "";

  // The major number
  std::uint32_t m_major = 0;

  // The minor number
  std::uint32_t m_minor = 0;

  // The patch number
  std::uint32_t m_patch = 0;

  // The prerelease
  std::string m_prerelease = "";

  // The build
  std::string m_build = "";
};

} // end namespace ILLNexus

} // end namespace DataHandling

} // end namespace Mantid
