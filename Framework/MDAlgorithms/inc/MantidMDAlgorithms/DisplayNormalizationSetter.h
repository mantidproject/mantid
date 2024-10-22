// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2015 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once
#include "MantidAPI/IMDWorkspace.h"
#include "MantidAPI/MatrixWorkspace_fwd.h"
#include "MantidKernel/DeltaEMode.h"
#include "MantidMDAlgorithms/DllConfig.h"

namespace Mantid {
namespace MDAlgorithms {
/** DisplayNormalizationSetter: Sets the displaynormalization on a
    workspace based on several parameters such as workspace-type,
energy-transfer-mode
    and if we are dealing with Q3D.
*/
class MANTID_MDALGORITHMS_DLL DisplayNormalizationSetter {
public:
  void operator()(const Mantid::API::IMDWorkspace_sptr &mdWorkspace,
                  const Mantid::API::MatrixWorkspace_sptr &underlyingWorkspace, bool isQ = false,
                  const Mantid::Kernel::DeltaEMode::Type &mode = Mantid::Kernel::DeltaEMode::Elastic);

private:
  void setNormalizationMDEvent(const Mantid::API::IMDWorkspace_sptr &mdWorkspace,
                               const Mantid::API::MatrixWorkspace_sptr &underlyingWorkspace, bool isQ = false,
                               const Mantid::Kernel::DeltaEMode::Type &mode = Mantid::Kernel::DeltaEMode::Elastic);

  void applyNormalizationMDEvent(const Mantid::API::IMDWorkspace_sptr &mdWorkspace,
                                 Mantid::API::MDNormalization displayNormalization,
                                 Mantid::API::MDNormalization displayNormalizationHisto);
};
} // namespace MDAlgorithms
} // namespace Mantid
