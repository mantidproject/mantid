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
#include "MantidDataHandling/LoadHelper_fwd.h"
#include "MantidDataHandling/NexusEntryProvider.h"
#include "MantidNexus/NexusClasses.h"

namespace Mantid::DataHandling {

/** LoadILLBase : A common base class for ILL raw data nexus (numor) loaders.
 */

class DLLExport LoadILLBase : public API::IFileLoader<Kernel::NexusDescriptor> {
protected:
  // protected getters
  std::unique_ptr<NeXus::NXRoot> &getNXRoot() { return m_nxroot; }
  std::unique_ptr<NexusEntryProvider> &getNep() { return m_nep; }
  std::shared_ptr<LoadHelper> getHelper() { return m_helper; }
  std::shared_ptr<API::Workspace> getOutput() { return m_workspace; }
  std::string getAcqMode() { return m_mode; }
  std::string getInstrument() { return m_instrument; }
  template <typename T> T getScalarMetadata(const std::string &key) { return m_nep->getScalarMetadata<T>(key); }
  template <typename T> std::vector<T> getVectorMetadata(const std::string &key) {
    return m_nep->getVectorMetadata<T>(key);
  }

  // pure virtual methods
  virtual API::Workspace_sptr buildWorkspace() = 0; // initialize the workspace
  virtual void loadAndFillData() = 0;               // load data from the file and fill into workspace

  // virtual methods
  virtual void declareExtraProperties() {}            // declare additional properties if needed
  virtual void configureBeamline() {}                 // configure the beamline components, place the detector, etc.
  virtual bool isOutputGroup() { return false; }      // override if the output must be a workspace group
  virtual std::string resolveVariant() { return ""; } // override to return the variant of the instrument, if any
  virtual std::string resolveAcqMode() { return ""; } // resolve the acquisition mode as string for future queries

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
  void bootstrap();
  void loadInstrument();
  void resolveStartTime();
  std::string resolveInstrument();
  void addSampleLogs();
  void patchSampleLogs();
  std::string getInstrumentDefinitionFilePath();
  void validateMetadata() { m_nep->isValid(mandatoryKeys()); }
  void setOutputWorkspace();

  // member variables
  std::unique_ptr<NeXus::NXRoot> m_nxroot;     // pointer to the root of the nexus tree
  std::unique_ptr<NexusEntryProvider> m_nep;   // pointer to nexus entry provider
  std::shared_ptr<LoadHelper> m_helper;        // pointer to load helper
  std::shared_ptr<API::Workspace> m_workspace; // pointer to output workspace
  std::string m_mode;                          // acquisition mode
  std::string m_instrument;                    // instrument name
  std::string m_timestamp;                     // start time in ISO format
};
} // namespace Mantid::DataHandling
