// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2012 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidMDAlgorithms/DllConfig.h"
#include "MantidMDAlgorithms/ImportMDHistoWorkspaceBase.h"

namespace Mantid {
namespace MDAlgorithms {

/** CreateMDHistoWorkspace

  @date 2012-06-21
*/
class MANTID_MDALGORITHMS_DLL CreateMDHistoWorkspace : public MDAlgorithms::ImportMDHistoWorkspaceBase {
public:
  const std::string name() const override;
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Creates an MDHistoWorkspace from supplied lists of signal and "
           "error values.";
  }

  int version() const override;
  const std::vector<std::string> seeAlso() const override { return {"ConvertToMD", "CreateMDWorkspace"}; }
  const std::string category() const override;

  std::map<std::string, std::string> validateInputs() override;

private:
  void init() override;
  void exec() override;
};

} // namespace MDAlgorithms
} // namespace Mantid
