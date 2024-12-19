// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2015 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/Algorithm.h"

static const std::string NAME_MANUALRPOGRESSREPORTER = "ManualProgressReporter";

namespace Mantid {
namespace Algorithms {

/** ManualProgressReporter : Development Algorithm that reports
 * to a progress tracker a bunch of times
 */
class ManualProgressReporter final : public API::Algorithm {
public:
  const std::string name() const override { return "ManualProgressReporter"; }
  int version() const override { return 1; };
  const std::string category() const override { return "Utility\\Development"; };
  const std::string summary() const override { return "Warning: This algorithm just reports progress a few times."; };

private:
  void init() override {
    declareProperty(std::make_unique<Kernel::PropertyWithValue<int>>("NumberOfProgressReports", 10),
                    "The number of times the progress will be reported.");
    declareProperty(std::make_unique<Kernel::PropertyWithValue<bool>>("StartAnotherAlgorithm", false),
                    "The algorithm will start a child algorithm on every "
                    "iteration, before reporting progress.");
  };
  void exec() override {
    int numberOfReports = getProperty("NumberOfProgressReports");
    bool startAnotherAlg = getProperty("StartAnotherAlgorithm");
    auto m_progress = std::make_unique<API::Progress>(this, 0.0, 1.0, numberOfReports);
    for (int i = 0; i < numberOfReports; ++i) {
      if (startAnotherAlg) {
        auto alg = API::AlgorithmManager::Instance().create("ManualProgressReporter");

        alg->initialize();
        alg->setProperty("NumberOfProgressReports", numberOfReports);
        alg->setChild(false);
        alg->execute();
      }
      m_progress->report();
    }
  };
};

} // namespace Algorithms
} // namespace Mantid
