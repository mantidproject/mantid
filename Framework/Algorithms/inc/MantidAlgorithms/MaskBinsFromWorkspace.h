// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/DistributedAlgorithm.h"
#include "MantidAlgorithms/DllConfig.h"
#include "MantidIndexing/SpectrumIndexSet.h"

namespace Mantid {

namespace Histogram {
class BinEdges;
}
namespace Algorithms {
class MANTID_ALGORITHMS_DLL MaskBinsFromWorkspace : public API::DistributedAlgorithm {
public:
  /// Algorithm's name
  const std::string name() const override { return "MaskBinsFromWorkspace"; }
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Copies over masked bins from a workspace to another workspace.";
  }

  /// Algorithm's version
  int version() const override { return (1); }
  const std::vector<std::string> seeAlso() const override { return {"MaskBinsFromTable"}; }
  /// Algorithm's category for identification
  const std::string category() const override { return "Transforms\\Masking"; }

private:
  /// Initialisation code
  void init() override;
  /// Execution code
  void exec() override;
  Indexing::SpectrumIndexSet m_indexSet; ///< the list of Spectra (workspace index) to load
};

} // namespace Algorithms
} // namespace Mantid
