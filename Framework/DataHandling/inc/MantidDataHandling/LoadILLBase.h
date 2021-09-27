// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/IFileLoader.h"
#include "MantidAPI/Workspace_fwd.h"
#include "MantidDataHandling/DllConfig.h"
#include "MantidDataHandling/LoadHelper.h"
#include "MantidDataHandling/NexusEntryProvider.h"
#include "MantidNexus/NexusClasses.h"

#include <type_traits>

namespace Mantid::DataHandling {

/** LoadILLBase : A common base class for ILL raw data nexus (numor) loaders.
 */

template <class W> class DLLExport LoadILLBase : public API::IFileLoader<Kernel::NexusDescriptor> {
  static_assert(std::is_base_of<API::Workspace, W>::value, "W must be a workspace type.");

protected:
  // protected getters
  std::unique_ptr<NeXus::NXRoot> &getNXRoot() const { return m_nxroot; }
  std::unique_ptr<NexusEntryProvider> &getNep() const { return m_nep; }
  std::unique_ptr<LoadHelper> &getHelper() const { return m_helper; }
  template <typename T> T getScalarMetadata(const std::string &key) { return m_nep->getScalarMetadata<T>(key); }
  template <typename T> std::vector<T> getVectorMetadata(const std::string &key) {
    return m_nep->getVectorMetadata<T>(key);
  }

  // pure virtual methods
  virtual std::shared_ptr<W> load() = 0; // the body of the actual loader

  // virtual methods
  virtual void declareExtraProperties() {} // declare additional properties if needed

  /**
   * @brief mandatoryKeys override to return a vector of all the mandatory metadata keys according to the protocols
   * Note, do not include the data entries here, as checking if data block exists is expensive. Furthermore, if it's
   * data that is missing, there is nothing one could do.
   * @return vector of mandatory keys
   */
  virtual std::vector<std::string> mandatoryKeys() { return std::vector<std::string>(); }

private:
  // non-virtual methods
  void init() override final;
  void exec() override final;
  void setup();
  void wrapup(std::shared_ptr<W>);
  void loadInstrument(std::shared_ptr<W>);
  void addStartTime(std::shared_ptr<W>);
  void addSampleLogs(std::shared_ptr<W>);
  void patchSampleLogs(std::shared_ptr<W>);
  void patchLogsForPatchedEntries(std::shared_ptr<W>);
  void setOutputWorkspace(std::shared_ptr<W>);
  std::string getStartTime();
  std::string resolveInstrument();
  std::string getInstrumentDefinitionFilePath(const std::string &);
  void validateMetadata() { m_nep->isValid(mandatoryKeys()); }

  // member variables
  std::unique_ptr<NeXus::NXRoot> m_nxroot;   // pointer to the root of the nexus tree
  std::unique_ptr<NexusEntryProvider> m_nep; // pointer to nexus entry provider
  std::unique_ptr<LoadHelper> m_helper;      // pointer to load helper
};
} // namespace Mantid::DataHandling
