// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2015 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef _MANTID_MDALGORITHMS_DISPLAYNORMALIZATION_SETTER_H
#define _MANTID_MDALGORITHMS_DISPLAYNORMALIZATION_SETTER_H
#include "MantidAPI/IMDWorkspace.h"
#include "MantidAPI/MatrixWorkspace_fwd.h"
#include "MantidKernel/DeltaEMode.h"
#include "MantidKernel/System.h"

namespace Mantid {
namespace MDAlgorithms {
/** DisplayNormalizationSetter: Sets the displaynormalization on a
    workspace based on several parameters such as workspace-type,
energy-transfer-mode
    and if we are dealing with Q3D.
*/
class DLLExport DisplayNormalizationSetter {
public:
  void operator()(Mantid::API::IMDWorkspace_sptr mdWorkspace,
                  const Mantid::API::MatrixWorkspace_sptr &underlyingWorkspace,
                  bool isQ = false,
                  const Mantid::Kernel::DeltaEMode::Type &mode =
                      Mantid::Kernel::DeltaEMode::Elastic);

private:
  void setNormalizationMDEvent(
      Mantid::API::IMDWorkspace_sptr mdWorkspace,
      const Mantid::API::MatrixWorkspace_sptr &underlyingWorkspace,
      bool isQ = false,
      const Mantid::Kernel::DeltaEMode::Type &mode =
          Mantid::Kernel::DeltaEMode::Elastic);

  void applyNormalizationMDEvent(
      Mantid::API::IMDWorkspace_sptr mdWorkspace,
      Mantid::API::MDNormalization displayNormalization,
      Mantid::API::MDNormalization displayNormalizationHisto);
};
} // namespace MDAlgorithms
} // namespace Mantid
#endif