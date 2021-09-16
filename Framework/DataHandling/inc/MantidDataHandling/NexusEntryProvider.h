// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidDataHandling/DllConfig.h"
#include "MantidKernel/PropertyManager.h"
#include "MantidNexus/NexusClasses.h"

namespace Mantid::DataHandling {

/** NexusEntryProvider : Wrapper around NXRoot providing metadata either from the tree or from the map of overridden
 * values.
 */
class MANTID_DATAHANDLING_DLL NexusEntryProvider {
public:
  NexusEntryProvider(NeXus::NXRoot &entry, Kernel::PropertyManager &entriesToPatch)
      : m_nxroot(entry), m_entriesToPatch(entriesToPatch){};

  template <typename T> T getNexusEntryValue(const std::string &entryName) {
    if (m_entriesToPatch.existsProperty(entryName)) {
      return m_entriesToPatch.getProperty(entryName);
    } else {
      try {
        return m_nxroot.getTyped<T>(entryName);
      } catch (std::runtime_error &) {
        throwMissingKeyError(entryName);
      }
    }
  }
  void isValid(const std::vector<std::string> &mandatoryKeys) {
    std::vector<std::string> missingKeys = missingMandatoryKeys(mandatoryKeys);
    for (const auto &key : missingKeys) {
      if (!m_entriesToPatch.existsProperty(key)) {
        throwMissingKeyError(key);
      }
    }
  }

private:
  void throwMissingKeyError(const std::string &key) {
    std::ostringstream ss;
    ss << "Unable to retrieve a mandatory entry " << key << " from the file\n";
    ss << "Please contact support to get the root cause fixed.\n";
    ss << "In the meantime, consider providing the value for the missing key.";
    throw std::runtime_error(ss.str());
  }

  bool keyExists(const std::string &entryName) { return m_nxroot.isValid(entryName); }

  std::vector<std::string> missingMandatoryKeys(const std::vector<std::string> &mandatoryKeys) {
    std::vector<std::string> missingKeys;
    for (const auto &key : mandatoryKeys) {
      if (!keyExists(key)) {
        missingKeys.emplace_back(key);
      }
    }
    return missingKeys;
  }

  NeXus::NXRoot &m_nxroot;
  Kernel::PropertyManager &m_entriesToPatch;
};

} // namespace Mantid::DataHandling
