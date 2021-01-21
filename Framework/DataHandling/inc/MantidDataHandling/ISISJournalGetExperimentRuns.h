// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/Algorithm.h"
#include "MantidAPI/ITableWorkspace_fwd.h"
#include "MantidDataHandling/DllConfig.h"

namespace Mantid {
namespace API {
class IJournal;
}

namespace DataHandling {
/**
ISISJournalGetExperimentRuns obtains a list of runs and related information for
an investigation and cycle.
 */
class DLLExport ISISJournalGetExperimentRuns : public API::Algorithm {
public:
  ISISJournalGetExperimentRuns() : API::Algorithm() {}
  ~ISISJournalGetExperimentRuns() override = default;

  const std::string name() const override { return "ISISJournalGetExperimentRuns"; }
  const std::string summary() const override {
    return "Obtains information of the runs associated with a specific "
           "investigation.";
  }
  int version() const override { return 1; }
  const std::vector<std::string> seeAlso() const override { return {"CatalogGetDataFiles"}; }
  const std::string category() const override { return "DataHandling"; }

protected:
  /// Construct a journal; can be overridden by tests to return a mock.
  virtual std::unique_ptr<API::IJournal> makeJournal(std::string const &instrument,
                                                     std::string const &cycle = std::string());

private:
  void init() override;
  void exec() override;
};
} // namespace DataHandling
} // namespace Mantid
