#if GCC_VERSION >= 40800 // 4.8.0
GCC_DIAG_OFF(literal - suffix)
#endif
#include "MantidICat/CatalogSearch.h"
#if GCC_VERSION >= 40800 // 4.8.0
GCC_DIAG_ON(literal - suffix)
#endif

#include "MantidAPI/CatalogManager.h"
#include "MantidAPI/WorkspaceProperty.h"
#include "MantidKernel/BoundedValidator.h"
#include "MantidKernel/DateValidator.h"
#include "MantidKernel/PropertyWithValue.h"

#include <boost/algorithm/string/regex.hpp>
#include <limits>

namespace Mantid {
namespace ICat {
DECLARE_ALGORITHM(CatalogSearch)

/// Initialisation method.
void CatalogSearch::init() {
  auto isDate = boost::make_shared<Kernel::DateValidator>();

  // Properties related to the search fields the user will fill in to improve
  // search.
  declareProperty("InvestigationName", "",
                  "The name of the investigation to search for.");
  declareProperty("Instrument", "",
                  "The name of the instrument used in the investigation.");
  declareProperty("RunRange", "", "The range of runs to search for related "
                                  "investigations. Must be in the format "
                                  "0000-0000 or 0000:0000.");
  declareProperty("StartDate", "", isDate, "The start date for the range of "
                                           "investigations to be searched. The "
                                           "format must be DD/MM/YYYY.");
  declareProperty("EndDate", "", isDate, "The end date for the range of "
                                         "investigations to be searched. The "
                                         "format must be DD/MM/YYYY.");
  declareProperty(
      "Keywords", "",
      "A comma separated list of words to search for in the investigation.");
  declareProperty("InvestigationId", "", "The ID of the investigation.");
  declareProperty(
      "InvestigatorSurname", "",
      "The surname of the investigator associated to the investigation.");
  declareProperty("SampleName", "",
                  "The name of the sample used in the investigation.");
  declareProperty("DataFileName", "",
                  "The name of the data file in the investigation.");
  declareProperty("InvestigationType", "", "The type of the investigation.");
  declareProperty("MyData", false, "If set to true, only search in "
                                   "investigations of which you are an "
                                   "investigator, e.g. 'My Data'.");

  // These are needed for paging on the interface, and to minimise the amount of
  // results returned by the query.
  declareProperty(
      "CountOnly", false,
      "Boolean option to perform COUNT search only. This is used for paging.");
  declareProperty<int>("Limit", 100, "The maximum amount of search results to "
                                     "return. Adds a LIMIT clause to the "
                                     "query. This is used for paging.");
  declareProperty<int>(
      "Offset", 0,
      "The location to begin returning results from. This is used for paging.");

  declareProperty("Session", "",
                  "The session information of the catalog search in.");

  declareProperty(new API::WorkspaceProperty<API::ITableWorkspace>(
                      "OutputWorkspace", "", Kernel::Direction::Output),
                  "The name of the workspace that will be created to store the "
                  "search results.");
  declareProperty<int64_t>("NumberOfSearchResults", 0,
                           "The number of search results returned for the "
                           "INPUT. Performs a COUNT query to determine this. "
                           "This is used for paging.",
                           Kernel::Direction::Output);
}

/// Execution method.
void CatalogSearch::exec() {
  // Obtains the inputs from the search interface.
  CatalogSearchParam params;
  // Get the user input search terms to search for.
  getInputProperties(params);
  // Create output workspace.
  auto workspace =
      API::WorkspaceFactory::Instance().createTable("TableWorkspace");
  // Obtain all the active catalogs.
  auto catalogs =
      API::CatalogManager::Instance().getCatalog(getPropertyValue("Session"));
  // Search for investigations with user specific search inputs.
  setProperty("OutputWorkspace", workspace);
  // Do not perform a full search if we only want a COUNT search.
  if (getProperty("CountOnly")) {
    // Set the related property needed for paging.
    setProperty("NumberOfSearchResults",
                catalogs->getNumberOfSearchResults(params));
    return;
  }
  // Search for investigations in the archives.
  catalogs->search(params, workspace, getProperty("Offset"),
                   getProperty("Limit"));
}

/**
 * This method gets the input properties for the algorithm.
 * @param params :: reference to inputs object.
 */
void CatalogSearch::getInputProperties(CatalogSearchParam &params) {
  params.setInvestigationName(getPropertyValue("InvestigationName"));
  params.setInstrument(getPropertyValue("Instrument"));
  std::string runRange = getProperty("RunRange");
  setRunRanges(runRange, params);
  params.setStartDate(params.getTimevalue(getPropertyValue("StartDate")));
  params.setEndDate(params.getTimevalue(getPropertyValue("EndDate")));
  params.setKeywords(getPropertyValue("Keywords"));
  params.setInvestigationId(getPropertyValue("InvestigationId"));
  params.setInvestigationName(getPropertyValue("InvestigationName"));
  params.setInvestigatorSurName(getPropertyValue("InvestigatorSurname"));
  params.setSampleName(getPropertyValue("SampleName"));
  params.setDatafileName(getPropertyValue("DataFileName"));
  params.setInvestigationType(getPropertyValue("InvestigationType"));
  params.setMyData(boost::lexical_cast<bool>(getPropertyValue("MyData")));
}

/**
 * Parse the run-range input and split it into start and end run, and then set
 * the parameters.
 * @param runRange :: The input field to parse.
 * @param params   :: reference to inputs object.
 */
void CatalogSearch::setRunRanges(std::string &runRange,
                                 CatalogSearchParam &params) {
  // A container to hold the range of run numbers.
  std::vector<std::string> runNumbers;
  // Split the input text by "-",":" or "," and add contents to runNumbers.
  boost::algorithm::split_regex(runNumbers, runRange, boost::regex("-|:"));

  double startRange = 0;
  double endRange = 0;

  // If the user has only input a start range ("4444" or "4444-").
  if (!runNumbers.at(0).empty()) {
    startRange = boost::lexical_cast<double>(runNumbers.at(0));
    // We set the end range to be equal now, so we do not have to do a check if
    // it exists later.
    endRange = boost::lexical_cast<double>(runNumbers.at(0));
  }

  // If the user has input a start and end range, or just an end range
  // ("4444-4449" or "-4449").
  if (runNumbers.size() == 2) {
    // Has the user input an end range...
    if (!runNumbers.at(1).empty()) {
      endRange = boost::lexical_cast<double>(runNumbers.at(1));

      // If they have not chosen a start range ("-4449");
      if (startRange == 0) {
        startRange = boost::lexical_cast<double>(runNumbers.at(1));
      }
    }
  }

  if (startRange > endRange) {
    throw std::runtime_error(
        "Run end number cannot be lower than run start number.");
  }

  params.setRunStart(startRange);
  params.setRunEnd(endRange);
}
}
}
