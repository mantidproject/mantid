// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2015 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_ALGORITHMS_EXTRACTSPECTRA_H_
#define MANTID_ALGORITHMS_EXTRACTSPECTRA_H_

#include "MantidAPI/DistributedAlgorithm.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidKernel/System.h"

namespace Mantid {
namespace Algorithms {

/** Extracts specified spectra from a workspace and places them in a new
  workspace.
*/
class DLLExport ExtractSpectra : public API::DistributedAlgorithm {
public:
  const std::string name() const override;
  int version() const override;
  const std::vector<std::string> seeAlso() const override {
    return {"CropWorkspace", "ExtractSingleSpectrum", "ExtractUnmaskedSpectra",
            "PerformIndexOperations"};
  }
  const std::string category() const override;
  const std::string summary() const override;

private:
  void init() override;
  void exec() override;
  void execHistogram();
  void execEvent();

  void propagateBinMasking(API::MatrixWorkspace &workspace, const int i) const;
  void checkProperties();
  std::size_t getXMin(const size_t wsIndex = 0);
  std::size_t getXMax(const size_t wsIndex = 0);
  void cropRagged(API::MatrixWorkspace &workspace, int index);

  /// The input workspace
  API::MatrixWorkspace_sptr m_inputWorkspace;
  DataObjects::EventWorkspace_sptr eventW;
  /// The bin index to start the cropped workspace from
  std::size_t m_minX = 0;
  /// The bin index to end the cropped workspace at
  std::size_t m_maxX = 0;
  /// Flag indicating whether the input workspace has common boundaries
  bool m_commonBoundaries = false;
  /// Flag indicating whether we're dealing with histogram data
  bool m_histogram = false;
  /// Flag indicating whether XMin and/or XMax has been set
  bool m_croppingInX = false;
  /// The list of workspaces to extract.
  std::vector<size_t> m_workspaceIndexList;
};

} // namespace Algorithms
} // namespace Mantid

#endif /* MANTID_ALGORITHMS_EXTRACTSPECTRA_H_ */
