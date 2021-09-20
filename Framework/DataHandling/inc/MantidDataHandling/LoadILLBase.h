// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/IFileLoader.h"
#include "MantidAPI/Workspace.h"
#include "MantidDataHandling/DllConfig.h"
#include "MantidDataHandling/LoadHelper.h"
#include "MantidDataHandling/NexusEntryProvider.h"
#include "MantidNexus/NexusClasses.h"

namespace Mantid::DataHandling {

/** LoadILLBase : A common base class for ILL raw data (numor) loaders.
 */

class DLLExport LoadILLBase : public API::IFileLoader<Kernel::NexusDescriptor> {
private:
  // pure virtual methods
  virtual void buildWorkspace() = 0;
  virtual void loadAndFillData() = 0;

  // virtual methods
  virtual void declareExtraProperties() {}
  virtual void configureBeamline() {}
  virtual std::vector<std::string> mandatoryKeys() { return std::vector<std::string>(); }
  virtual bool isOutputGroup() { return false; }
  virtual std::string resolveVariant() { return ""; }
  virtual std::string resolveAcqMode() { return ""; }

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
  void setOutputWorkspace() { setProperty<API::Workspace_sptr>("OutputWorkspace", m_workspace); }

  // private getters
  std::shared_ptr<NeXus::NXRoot> getNXRoot() { return m_nxroot; }
  std::shared_ptr<NexusEntryProvider> getNep() { return m_nep; }
  std::shared_ptr<LoadHelper> getHelper() { return m_helper; }
  std::shared_ptr<API::Workspace> getOutput() { return m_workspace; }
  std::string getAcqMode() { return m_acqMode; }
  std::string getInstrument() { return m_instrumentName; }

  // member variables
  std::shared_ptr<NeXus::NXRoot> m_nxroot;
  std::shared_ptr<NexusEntryProvider> m_nep;
  std::shared_ptr<LoadHelper> m_helper;
  std::shared_ptr<API::Workspace> m_workspace;
  std::string m_acqMode;
  std::string m_instrumentName;
};
} // namespace Mantid::DataHandling
