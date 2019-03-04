// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_ALGORITHMS_MERGELOGS_H_
#define MANTID_ALGORITHMS_MERGELOGS_H_

#include "MantidAPI/Algorithm.h"

namespace Mantid {
namespace Algorithms {

class DLLExport MergeLogs : public API::Algorithm {
public:
  /// Algorithm's name for identification
  const std::string name() const override { return "MergeLogs"; };
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Merge 2 TimeSeries logs in a given Workspace.";
  }
  /// Algorithm's version for identification
  int version() const override { return 1; };
  const std::vector<std::string> seeAlso() const override {
    return {"AddTimeSeriesLog"};
  }
  /// Algorithm's category for identification
  const std::string category() const override { return "DataHandling\\Logs"; }
  /// Cross-check properties with each other
  std::map<std::string, std::string> validateInputs() override;

private:
  void init() override;
  void exec() override;
  // Helper to validate TimeSeriesProperty
  std::string validateTSP(std::string const &propertyName);
};

} // namespace Algorithms
} // namespace Mantid

#endif /* MANTID_ALGORITHMS_MERGELOGS_H_ */
