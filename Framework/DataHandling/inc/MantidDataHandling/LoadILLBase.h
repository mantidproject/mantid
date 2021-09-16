// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/IFileLoader.h"
#include "MantidDataHandling/DllConfig.h"
#include "MantidDataHandling/LoadHelper.h"
#include "MantidDataHandling/NexusEntryProvider.h"
#include "MantidKernel/NexusDescriptor.h"
#include "MantidNexus/NexusClasses.h"

namespace Mantid::DataHandling {

/** LoadILLBase : A common base class for ILL raw data (numor) loaders.
 */

class DLLExport LoadILLBase : public API::IFileLoader<Kernel::NexusDescriptor> {
public:
  virtual void declareExtraProperties() {}
  virtual std::vector<std::string> mandatoryKeys() { return std::vector<std::string>(); }
  virtual void load() = 0;

private:
  void init() override;
  void exec() override;
  void validateProtocol() { m_nep->isValid(mandatoryKeys()); }

  std::unique_ptr<NeXus::NXRoot> m_root;
  std::unique_ptr<NexusEntryProvider> m_nep;
  LoadHelper m_helper;
};
} // namespace Mantid::DataHandling
