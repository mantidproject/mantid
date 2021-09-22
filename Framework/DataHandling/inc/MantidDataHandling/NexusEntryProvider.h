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
 * values. The overridden values are stored in a PropertyManager; that is, type agnostic JSON-like dictionaries.
 * What is considered metadata are scalars (rank 0) or small 1D arrays (rank 1) of any standard type.
 * Anything with higher dimensionality is considered data, hence it is not possible to override as it does not make
 * sense.
 */
class MANTID_DATAHANDLING_DLL NexusEntryProvider {
public:
  NexusEntryProvider(const std::string &filename, const Kernel::PropertyManager &entriesToPatch)
      : m_nxroot(filename), m_entriesToPatch(entriesToPatch) {}

  /**
   * Retrieves scalar value of the given key from the map, if provided, or from the nexus otherwise
   * @param key : the full path of the entry in nexus
   * @return scalar value
   * @throws runtime_error if the key is missing
   */
  template <typename T> T getScalarMetadata(const std::string &key) {
    if (m_entriesToPatch.existsProperty(key)) {
      return m_entriesToPatch.getProperty(key);
    } else {
      try {
        return m_nxroot.getTypedScalar<T>(key);
      } catch (std::runtime_error &) {
        throwMissingKeyError(key);
      }
    }
    // shouldn't reach here
    return T();
  }

  /**
   * Retrieves vector value of the given key from the map, if provided, or from the nexus otherwise
   * @param key : the full path of the entry in nexus
   * @return 1D vector value
   * @throws runtime_error if the key is missing
   */
  template <typename T> std::vector<T> getVectorMetadata(const std::string &key) {
    if (m_entriesToPatch.existsProperty(key)) {
      return m_entriesToPatch.getProperty(key);
    } else {
      try {
        return m_nxroot.getTypedVector<T>(key);
      } catch (std::runtime_error &) {
        throwMissingKeyError(key);
      }
    }
    // shouldn't reach here
    return std::vector<T>();
  }

  /**
   * Checks if all the mandatory keys are present
   * @param mandatoryKeys : vector of all the mandatory keys
   * @return true if all provided
   * @throws runtim_error if one is missing
   */
  bool isValid(const std::vector<std::string> &mandatoryKeys) {
    for (const auto &key : mandatoryKeys) {
      if (!keyExists(key)) {
        throwMissingKeyError(key);
      }
    }
    return true;
  }

private:
  /**
   * Throws an error if the mandatory key is not found.
   * Gives hints what to do to work about it.
   * @param key : the full path of the entry in nexus
   * @throws runtime_error
   */
  [[noreturn]] void throwMissingKeyError(const std::string &key) {
    std::ostringstream ss;
    ss << "Numor does not conform to the protocols.\n";
    ss << "Unable to retrieve a mandatory entry " << key << " from the file.\n";
    ss << "Please contact instrument control service to get the root cause fixed.\n";
    ss << "In the meantime, consider providing the value for the missing key.\n";
    throw std::runtime_error(ss.str());
  }

  /**
   * Checks if the given key exists either in the map or in the tree
   * @param key : the full path of the entry in nexus
   * @return true if it exists
   */
  bool keyExists(const std::string &key) { return m_entriesToPatch.existsProperty(key) || m_nxroot.isValid(key); }

  NeXus::NXRoot m_nxroot;                   // root of the tree
  Kernel::PropertyManager m_entriesToPatch; // property manager
};

} // namespace Mantid::DataHandling
