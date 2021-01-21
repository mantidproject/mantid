// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidDataHandling/ISISJournalGetExperimentRuns.h"
#include "MantidAPI/ITableWorkspace.h"
#include "MantidAPI/TableRow.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidAPI/WorkspaceProperty.h"
#include "MantidDataHandling/ISISJournal.h"
#include "MantidKernel/ConfigService.h"
#include "MantidKernel/FacilityInfo.h"
#include "MantidKernel/ListValidator.h"
#include "MantidKernel/MandatoryValidator.h"

namespace Mantid {
namespace DataHandling {
DECLARE_ALGORITHM(ISISJournalGetExperimentRuns)

using Mantid::API::IJournal;
using Mantid::API::ITableWorkspace_sptr;
using Mantid::API::TableRow;
using Mantid::DataHandling::ISISJournal;
using Mantid::Kernel::ConfigService;
using Mantid::Kernel::MandatoryValidator;
using Mantid::Kernel::StringListValidator;

namespace {
// These strings define the XML tags for the metadata we are going to return or
// filter by
static constexpr const char *INVESTIGATION_ID = "experiment_identifier";
static constexpr const char *RUN_NAME = "name";
static constexpr const char *RUN_NUMBER = "run_number";
static constexpr const char *RUN_TITLE = "title";

/** Get the run metadata for a particular investigation from an ISIS journal
 *
 * @param journal : a journal for the instrument and cycle of interest
 * @param investigationId : the experiment identifier for the investigation
 * @returns : a vector contining the data (name, number and title) for each run
 * @throws : if there is an error fetching the data
 */
std::vector<IJournal::RunData> getRuns(IJournal *journal, std::string const &investigationId) {
  static auto valuesToLookup = std::vector<std::string>{RUN_NUMBER, RUN_TITLE};
  auto filters = IJournal::RunData{{INVESTIGATION_ID, investigationId}};
  auto runs = journal->getRuns(valuesToLookup, filters);
  return runs;
}

/** Convert the run data to a table workspace
 * @param runs : a vector containing the data (name, number and title) for each
 * run
 * @returns : a table workspace containing one row for each run, with columns
 * for the data fields of interest (name, number and title)
 */
ITableWorkspace_sptr convertRunDataToTable(std::vector<IJournal::RunData> &runs) {
  auto workspace = API::WorkspaceFactory::Instance().createTable("TableWorkspace");

  workspace->addColumn("str", "Name");
  workspace->addColumn("str", "Run Number");
  workspace->addColumn("str", "Title");

  for (auto &run : runs) {
    TableRow row = workspace->appendRow();
    row << run[RUN_NAME] << run[RUN_NUMBER] << run[RUN_TITLE];
  }
  return workspace;
}

/** Get all instruments for ISIS
 * @returns : a list of instrument names
 */
std::vector<std::string> getInstruments() {
  auto instruments = std::vector<std::string>();
  auto &instrInfo = ConfigService::Instance().getFacility("ISIS").instruments();
  for (const auto &instrument : instrInfo)
    instruments.emplace_back(instrument.name());
  return instruments;
}

/** Get the default instrument if it is in the given list
 * Returns : the currently set default instrument, if it is in the given list,
 * or the first instrument in the list otherwise. Returns an empty string if the
 * list is empty
 */
std::string getDefaultInstrument(std::vector<std::string> const &instruments) {
  if (instruments.empty())
    return std::string();

  auto instrument = ConfigService::Instance().getInstrument().name();
  if (std::find(instruments.cbegin(), instruments.cend(), instrument) == instruments.cend()) {
    instrument = instruments.front();
  }
  return instrument;
}
} // namespace

void ISISJournalGetExperimentRuns::init() {
  auto const instruments = getInstruments();
  auto const instrument = getDefaultInstrument(instruments);

  declareProperty("Instrument", instrument, std::make_shared<StringListValidator>(instruments), "The instrument name");
  declareProperty("Cycle", "", std::make_shared<MandatoryValidator<std::string>>(), "The cycle name, for example 19_4");
  declareProperty("InvestigationId", "", std::make_shared<MandatoryValidator<std::string>>(),
                  "ID of the selected investigation");
  declareProperty(
      std::make_unique<API::WorkspaceProperty<API::ITableWorkspace>>("OutputWorkspace", "", Kernel::Direction::Output),
      "The name of the workspace to store the run details.");
}

void ISISJournalGetExperimentRuns::exec() {
  auto instrument = getPropertyValue("Instrument");
  auto cycle = getPropertyValue("Cycle");
  auto investigationId = getPropertyValue("InvestigationId");
  auto journal = makeJournal(instrument, cycle);

  auto runs = getRuns(journal.get(), investigationId);
  auto workspace = convertRunDataToTable(runs);
  setProperty("OutputWorkspace", workspace);
}

/** Construct a journal class for a particular journal file. This wrapper
 * function exists so that it can be overridden in the tests to return a mock
 * journal.
 * @param instrument : the instrument name (case insensitive), e.g. INTER
 * @param cycle : the cycle name, e.g. 19_4
 */
std::unique_ptr<IJournal> ISISJournalGetExperimentRuns::makeJournal(std::string const &instrument,
                                                                    std::string const &cycle) {
  return std::make_unique<ISISJournal>(instrument, cycle);
}
} // namespace DataHandling
} // namespace Mantid
