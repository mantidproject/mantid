// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2015 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/Algorithm.h"
#include "MantidAPI/DataProcessorAlgorithm.h"
#include "MantidAPI/IMDWorkspace.h"
#include "MantidMDAlgorithms/DllConfig.h"

namespace Mantid {
namespace MDAlgorithms {

std::vector<std::string> DLLExport findOriginalQUnits(const Mantid::API::IMDWorkspace_const_sptr &inws,
                                                      Mantid::Kernel::Logger &logger);

/** CutMD : Slices multidimensional workspaces.

  @date 2015-03-20
*/
class MANTID_MDALGORITHMS_DLL CutMD : public API::DataProcessorAlgorithm {
public:
  const std::string name() const override { return "CutMD"; }
  int version() const override { return 1; }
  const std::vector<std::string> seeAlso() const override { return {"SliceMDHisto", "ProjectMD", "SliceMD", "BinMD"}; }
  const std::string summary() const override {
    return "Slices multidimensional workspaces using input projection "
           "information and binning limits.";
  }
  const std::string category() const override { return "MDAlgorithms\\Slicing"; }

  void init() override;
  void exec() override;

  static const std::string InvAngstromSymbol;
  static const std::string RLUSymbol;
  static const std::string AutoMethod;
  static const std::string RLUMethod;
  static const std::string InvAngstromMethod;
};

} // namespace MDAlgorithms
} // namespace Mantid
