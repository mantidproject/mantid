// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidDataHandling/DllConfig.h"
#include "MantidKernel/NexusDescriptor.h"
#include "MantidKernel/PropertyManager.h"
#include "MantidNexus/NexusClasses.h"

namespace Mantid::DataHandling {

/** NexusEntryProvider : Wrapper around NXRoot providing metadata either from the tree or from the map of overridden
 * values.
 */
class MANTID_DATAHANDLING_DLL NexusEntryProvider {
public:
  NexusEntryProvider(const std::string &filename, const Kernel::PropertyManager &entriesToPatch)
      : m_nxroot(filename), m_entriesToPatch(entriesToPatch) {}

  template <typename T> T getScalarMetadata(const std::string &entryName) {
    if (m_entriesToPatch.existsProperty(entryName)) {
      return m_entriesToPatch.getProperty(entryName);
    } else {
      try {
        return m_nxroot.getTypedScalar<T>(entryName);
      } catch (std::runtime_error &) {
        throwMissingKeyError(entryName);
      }
    }
    // shouldn't reach here
    return T();
  }

  template <typename T> std::vector<T> getVectorMetadata(const std::string &entryName) {
    if (m_entriesToPatch.existsProperty(entryName)) {
      return m_entriesToPatch.getProperty(entryName);
    } else {
      try {
        return m_nxroot.getTypedVector<T>(entryName);
      } catch (std::runtime_error &) {
        throwMissingKeyError(entryName);
      }
    }
    // shouldn't reach here
    return std::vector<T>();
  }

  bool isValid(const std::vector<std::string> &mandatoryKeys) {
    for (const auto &key : mandatoryKeys) {
      if (!keyExists(key)) {
        throwMissingKeyError(key);
      }
    }
    return true;
  }

private:
  [[noreturn]] void throwMissingKeyError(const std::string &key) {
    std::ostringstream ss;
    ss << "Numor does not conform to the protocols.\n";
    ss << "Unable to retrieve a mandatory entry " << key << " from the file.\n";
    ss << "Please contact instrument control service to get the root cause fixed.\n";
    ss << "In the meantime, consider providing the value for the missing key.\n";
    throw std::runtime_error(ss.str());
  }

  bool keyExists(const std::string &key) { return m_nxroot.isValid(key) || m_entriesToPatch.existsProperty(key); }

  NeXus::NXRoot m_nxroot;
  Kernel::PropertyManager m_entriesToPatch;
};

} // namespace Mantid::DataHandling
