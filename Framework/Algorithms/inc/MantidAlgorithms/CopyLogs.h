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

namespace API {
class Run;
}

namespace Algorithms {

/** CopyLogs

  An algorithm to copy the sample logs from one workspace to another.
  This algorithm also provides several options for merging sample logs from one
  workspace to another.

  @author Samuel Jackson, STFC, RAL
  @date 11/10/2013
*/
class MANTID_ALGORITHMS_DLL CopyLogs : public API::Algorithm {
public:
  const std::string name() const override;
  /// Summary of algorithms purpose
  const std::string summary() const override { return "Copies the sample logs from one workspace to another."; }

  int version() const override;
  const std::vector<std::string> seeAlso() const override {
    return {"CreateLogPropertyTable", "CopyDetectorMapping", "CheckForSampleLogs", "CopySample"};
  }
  const std::string category() const override;

private:
  void init() override;
  void exec() override;

  /// appends new logs and overwrites existing logs.
  void mergeReplaceExisting(const std::vector<Kernel::Property *> &inputLogs, API::Run &outputRun);
  /// appends new logs but leaves exisitng logs untouched.
  void mergeKeepExisting(const std::vector<Kernel::Property *> &inputLogs, API::Run &outputRun);
  /// appends new logs and removes all existing logs.
  void wipeExisting(const std::vector<Kernel::Property *> &inputLogs, API::Run &outputRun);
};

} // namespace Algorithms
} // namespace Mantid
