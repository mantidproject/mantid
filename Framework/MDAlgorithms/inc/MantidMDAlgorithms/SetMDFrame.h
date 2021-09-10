// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2015 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/Algorithm.h"
#include "MantidGeometry/MDGeometry/MDFrame.h"
#include "MantidMDAlgorithms/DllConfig.h"

namespace Mantid {
namespace MDAlgorithms {

/** SetMDFrame : This algorithm changes the MDFrame stored alongside the
    dimension of MDWorkspaes.The algorithm should primarily be used to
    introduce the correct MDFrame type to workspaces based on legacy files.
*/
class MANTID_MDALGORITHMS_DLL SetMDFrame : public API::Algorithm {
public:
  static const std::string mdFrameSpecifier;

  const std::string name() const override;
  int version() const override;
  const std::string category() const override;
  const std::string summary() const override;
  std::map<std::string, std::string> validateInputs() override;

private:
  void init() override;
  void exec() override;
  Mantid::Geometry::MDFrame_uptr createMDFrame(const std::string &frameSelection,
                                               const Mantid::Geometry::MDFrame &oldFrame) const;
};

} // namespace MDAlgorithms
} // namespace Mantid
