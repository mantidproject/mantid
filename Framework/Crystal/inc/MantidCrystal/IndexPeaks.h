// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_CRYSTAL_INDEX_PEAKS_H_
#define MANTID_CRYSTAL_INDEX_PEAKS_H_
#include "MantidAPI/Algorithm.h"

namespace Mantid {
namespace Crystal {

/**
 * Implements an algorithm for indexing main and satellites peaks
 * in single crystal peaks.
 */
class DLLExport IndexPeaks : public API::Algorithm {
public:
  const std::string name() const override { return "IndexPeaks"; }
  const std::string summary() const override {
    return "Index the peaks using the UB from the sample.";
  }
  int version() const override { return 1; }
  const std::vector<std::string> seeAlso() const override {
    return {"IndexSXPeaks"};
  }
  const std::string category() const override { return "Crystal\\Peaks"; }
  std::map<std::string, std::string> validateInputs() override;

protected:
  void init() override;
  void exec() override;
};

} // namespace Crystal
} // namespace Mantid

#endif /* MANTID_CRYSTAL_INDEX_PEAKS */
