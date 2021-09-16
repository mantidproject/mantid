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
        std::ostringstream ss;
        ss << "Unable to retrieve a mandatory entry " << entryName << "\n";
        ss << "Please contact support to get the root cause fixed.\n";
        ss << "In the meantime, consider overriding the value for the missing key.";
        throw std::runtime_error(ss.str());
      }
    }
  };

private:
  NeXus::NXRoot &m_nxroot;
  Kernel::PropertyManager &m_entriesToPatch;
};

} // namespace Mantid::DataHandling
