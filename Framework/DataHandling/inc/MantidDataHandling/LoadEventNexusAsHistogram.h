// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/Algorithm.h"
#include "MantidAPI/MatrixWorkspace_fwd.h"
#include "MantidDataHandling/DllConfig.h"

#include <map>
#include <string>
#include <vector>

namespace Mantid::Nexus {
class NexusDescriptor;
}

namespace Mantid::DataHandling {

/** LoadEventNexusAsHistogram loads event NeXus data directly into a
 * histogram workspace without constructing an EventWorkspace.
 */
class MANTID_DATAHANDLING_DLL LoadEventNexusAsHistogram final : public API::Algorithm {
public:
  const std::string name() const override;
  int version() const override;
  const std::string category() const override;
  const std::string summary() const override;
  const std::vector<std::string> seeAlso() const override;

private:
  void init() override;
  std::map<std::string, std::string> validateInputs() override;
  void exec() override;
  API::MatrixWorkspace_sptr loadMetadata(const std::string &filename, const std::string &entryName,
                                         const Nexus::NexusDescriptor &descriptor);
};

} // namespace Mantid::DataHandling
