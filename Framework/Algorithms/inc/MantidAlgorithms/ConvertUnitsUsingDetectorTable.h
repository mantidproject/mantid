// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2015 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/DeprecatedAlgorithm.h"
#include "MantidAlgorithms/ConvertUnits.h"
#include "MantidAlgorithms/DllConfig.h"
#include "MantidKernel/Unit.h"

namespace Mantid {
namespace Algorithms {

/** ConvertUnitsUsingDetectorTable : Converts the units in which a workspace is
  represented, this variant of ConvertUnits uses a supplied table of geometry
  values
  rather than those given by the instrument geometry.
*/
class MANTID_ALGORITHMS_DLL ConvertUnitsUsingDetectorTable : public ConvertUnits, public API::DeprecatedAlgorithm {
public:
  const std::string name() const override;
  int version() const override;
  const std::string category() const override;
  const std::string summary() const override;

protected:
  const std::string workspaceMethodName() const override { return ""; }
  const std::string workspaceMethodInputProperty() const override { return ""; }

private:
  void init() override;

  void storeEModeOnWorkspace(API::MatrixWorkspace_sptr outputWS) override;

  /// Convert the workspace units using TOF as an intermediate step in the
  /// conversion
  API::MatrixWorkspace_sptr convertViaTOF(Kernel::Unit_const_sptr fromUnit,
                                          API::MatrixWorkspace_const_sptr inputWS) override;
};

} // namespace Algorithms
} // namespace Mantid
