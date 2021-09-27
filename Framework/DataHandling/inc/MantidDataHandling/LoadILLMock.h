// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/MatrixWorkspace.h"
#include "MantidDataHandling/DllConfig.h"
#include "MantidDataHandling/LoadILLBase.h"

namespace Mantid {
namespace DataHandling {

/** LoadILLMock : This is a fake loader to enable testing of the abstract base class functionality.
 * It is a part of the main build (and not the tests) in order to allow for manual tests too, using the workbench.
 */
class MANTID_DATAHANDLING_DLL LoadILLMock final : public LoadILLBase<API::MatrixWorkspace> {
public:
  int confidence(Kernel::NexusDescriptor &) const override;
  const std::string name() const override;
  int version() const override;
  const std::string category() const override;
  const std::string summary() const override;
  std::shared_ptr<API::MatrixWorkspace> load() override;
  std::vector<std::string> mandatoryKeys() override;
};

} // namespace DataHandling
} // namespace Mantid
