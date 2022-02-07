// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2013 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/Algorithm.h"
#include "MantidAlgorithms/DllConfig.h"

namespace Mantid {
namespace Algorithms {

/** ClearMaskFlag : Delete the mask flag/bit on all spectra in a workspace
 */
class MANTID_ALGORITHMS_DLL ClearMaskFlag : public API::Algorithm {
public:
  const std::string name() const override;
  int version() const override;
  const std::vector<std::string> seeAlso() const override { return {"MaskDetectors"}; }
  const std::string category() const override;
  /// Algorithm's summary
  const std::string summary() const override { return "Delete the mask flag/bit on all spectra in a workspace."; }

private:
  void init() override;
  void exec() override;
};

} // namespace Algorithms
} // namespace Mantid
