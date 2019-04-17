// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidAlgorithms/ManualProgressReporter.h"
#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/Progress.h"
#include "MantidKernel/Unit.h"
#include "MantidKernel/UnitFactory.h"
#include <thread>
namespace Mantid {
namespace Algorithms {

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(ManualProgressReporter)

//----------------------------------------------------------------------------------------------

/// Algorithms name for identification. @see Algorithm::name
const std::string ManualProgressReporter::name() const {
  return "ManualProgressReporter";
}

/// Algorithm's version for identification. @see Algorithm::version
int ManualProgressReporter::version() const { return 1; }

/// Algorithm's category for identification. @see Algorithm::category
const std::string ManualProgressReporter::category() const {
  return "Utility\\Development";
}

/// Algorithm's summary for use in the GUI and help. @see Algorithm::summary
const std::string ManualProgressReporter::summary() const {
  return "Warning: This algorithm just reports progress a few times.";
}

/** Initialize the algorithm's properties. */
void ManualProgressReporter::init() {
  declareProperty(std::make_unique<Kernel::PropertyWithValue<int>>(
                      "NumberOfProgressReports", 10),
                  "The number of times the progress will be reported.");
  declareProperty(std::make_unique<Kernel::PropertyWithValue<bool>>(
                      "StartAnotherAlgorithm", false),
                  "The algorithm will start a child algorithm on every "
                  "iteration, before reporting progress.");
}

/** Execute the algorithm. */
void ManualProgressReporter::exec() {
  using namespace std::chrono_literals;
  int numberOfReports = getProperty("NumberOfProgressReports");
  bool startAnotherAlg = getProperty("StartAnotherAlgorithm");
  auto m_progress =
      std::make_unique<API::Progress>(this, 0.0, 1.0, numberOfReports);
  for (int i = 0; i < numberOfReports; ++i) {
    if (startAnotherAlg) {
      auto alg =
          API::AlgorithmManager::Instance().create("ManualProgressReporter");

      alg->initialize();
      alg->setProperty("NumberOfProgressReports", numberOfReports);
      alg->setChild(false);
      alg->execute();
    }
    m_progress->report();
  }
}

} // namespace Algorithms
} // namespace Mantid
