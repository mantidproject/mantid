// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2012 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/Algorithm.h"
#include "MantidAPI/BoxControllerSettingsAlgorithm.h"
#include "MantidAPI/IMDEventWorkspace_fwd.h"
#include "MantidDataObjects/MDEventWorkspace.h"

namespace Mantid {
namespace MDAlgorithms {

/** Merge several MDWorkspaces into one.

  @date 2012-01-17
*/
class DLLExport MergeMD : public API::BoxControllerSettingsAlgorithm {
public:
  const std::string name() const override;
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Merge several MDWorkspaces into one.";
  }

  int version() const override;
  const std::vector<std::string> seeAlso() const override {
    return {"MergeMDFiles", "AccumulateMD"};
  }
  const std::string category() const override;

private:
  void init() override;
  void exec() override;
  void createOutputWorkspace(std::vector<std::string> &inputs);

  template <typename MDE, size_t nd>
  void doPlus(typename Mantid::DataObjects::MDEventWorkspace<MDE, nd>::sptr ws);

  /// Vector of input MDWorkspaces
  std::vector<Mantid::API::IMDEventWorkspace_sptr> m_workspaces;

  /// Vector of number of experimentalInfos for each input workspace
  std::vector<uint16_t> experimentInfoNo = {0};

  /// Output MDEventWorkspace
  Mantid::API::IMDEventWorkspace_sptr out;
};

} // namespace MDAlgorithms
} // namespace Mantid
