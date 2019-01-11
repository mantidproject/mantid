// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_CRYSTAL_INDEX_PEAKS_H_
#define MANTID_CRYSTAL_INDEX_PEAKS_H_

#include "MantidAPI/Algorithm.h"
#include "MantidKernel/System.h"

namespace Mantid {
namespace Crystal {
/** IndexPeaks : Algorithm to use the UB saved in the sample associated
    with the specified PeaksWorkspace, to index the peaks in the workspace.

    @author Dennis Mikkelson
    @date   2011-08-17
  */
class DLLExport IndexPeaks : public API::Algorithm {
public:
  /// Algorithm's name for identification
  const std::string name() const override { return "IndexPeaks"; };
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Index the peaks using the UB from the sample.";
  }

  /// Algorithm's version for identification
  int version() const override { return 1; }
  const std::vector<std::string> seeAlso() const override {
    return {"IndexSXPeaks"};
  }

  /// Algorithm's category for identification
  const std::string category() const override { return "Crystal\\Peaks"; }

private:
  /// Initialise the properties
  void init() override;

  /// Run the algorithm
  void exec() override;
};

} // namespace Crystal
} // namespace Mantid

#endif /* MANTID_CRYSTAL_INDEX_PEAKS */
