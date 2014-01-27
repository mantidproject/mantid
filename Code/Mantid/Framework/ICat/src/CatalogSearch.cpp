/*WIKI*

This algorithm searches for the investigations and stores the search results in a table workspace.

*WIKI*/

#if GCC_VERSION >= 40800 // 4.8.0
    GCC_DIAG_OFF(literal-suffix)
#endif
#include "MantidICat/CatalogSearch.h"
#if GCC_VERSION >= 40800 // 4.8.0
    GCC_DIAG_ON(literal-suffix)
#endif

#include "MantidAPI/WorkspaceProperty.h"
#include "MantidKernel/BoundedValidator.h"
#include "MantidKernel/DateValidator.h"
#include "MantidKernel/PropertyWithValue.h"
#include "MantidICat/CatalogAlgorithmHelper.h"

#include <boost/algorithm/string/regex.hpp>
#include <limits>

namespace Mantid
{
  namespace ICat
  {
    using namespace Kernel;
    using namespace API;

    DECLARE_ALGORITHM(CatalogSearch)

    /// Sets documentation strings for this algorithm
    void CatalogSearch::initDocs()
    {
      this->setWikiSummary("Searches investigations");
      this->setOptionalMessage("Searches investigations");
    }

    /// Initialisation method.
    void CatalogSearch::init()
    {
      auto mustBePositive = boost::make_shared<BoundedValidator<double>>();
      auto isDate = boost::make_shared<DateValidator>();
      mustBePositive->setLower(0.0);

      declareProperty("InvestigationName", "", "The name of the investigation to search.");
      declareProperty("Instrument","","The name of the instrument used for investigation search.");
      declareProperty("RunRange","","The range of runs to search for related investigations.");
      declareProperty("StartDate","", isDate, "The start date for the range of investigations to be searched.The format is DD/MM/YYYY.");
      declareProperty("EndDate","", isDate, "The end date for the range of investigations to be searched.The format is DD/MM/YYYY.");
      declareProperty("Keywords","","An option to search investigations data");
      declareProperty("InvestigatorSurname", "", "The surname of the investigator associated to the investigation.");
      declareProperty("SampleName", "", "The name of the sample used in the investigation to search.");
      declareProperty("DataFileName","", "The name of the data file to search.");
      declareProperty("InvestigationType", "", "The type  of the investigation to search.");
      declareProperty("MyData",false, "Boolean option to do my data only search.");
      declareProperty(new WorkspaceProperty<API::ITableWorkspace> ("OutputWorkspace", "", Direction::Output),
          "The name of the workspace that will be created to store the ICat investigations search result.");

    }

    /// Execution method.
    void CatalogSearch::exec()
    {
      // Obtains the inputs from the search interface.
      CatalogSearchParam params;
      // Get the user input search terms to search for.
      getInputProperties(params);
      // Create output workspace.
      auto workspace = WorkspaceFactory::Instance().createTable("TableWorkspace");
      // Search for investigations in the archives.
      CatalogAlgorithmHelper().createCatalog()->search(params,workspace);
      // Search for investigations with user specific search inputs.
      setProperty("OutputWorkspace",workspace);
    }

    /**
     * This method gets the input properties for the algorithm.
     * @param params :: reference to inputs object.
     */
    void CatalogSearch::getInputProperties(CatalogSearchParam& params)
    {
      params.setInvestigationName(getPropertyValue("InvestigationName"));
      params.setInstrument(getPropertyValue("Instrument"));
      std::string runRange = getProperty("runRange");
      setRunRanges(runRange,params);
      params.setStartDate(params.getTimevalue(getPropertyValue("StartDate")));
      params.setEndDate(params.getTimevalue(getPropertyValue("EndDate")));
      params.setKeywords(getPropertyValue("Keywords"));
      params.setInvestigationName(getPropertyValue("InvestigationName"));
      params.setInvestigatorSurName(getPropertyValue("InvestigatorSurname"));
      params.setSampleName(getPropertyValue("SampleName"));
      params.setDatafileName(getPropertyValue("DataFileName"));
      params.setInvestigationType(getPropertyValue("InvestigationType"));
      params.setMyData(boost::lexical_cast<bool>(getPropertyValue("MyData")));
    }

    /**
     * Parse the run-range input and split it into start and end run, and then set the parameters.
     * @param runRange :: The input field to parse.
     * @param params   :: reference to inputs object.
     */
    void CatalogSearch::setRunRanges(std::string &runRange, CatalogSearchParam& params)
    {
      // A container to hold the range of run numbers.
      std::vector<std::string> runNumbers;
      // Split the input text by "-",":" or "," and add contents to runNumbers.
      boost::algorithm::split_regex(runNumbers, runRange, boost::regex("-|:"));

      double startRange = 0;
      double endRange   = 0;

      // If the user has only input a start range ("4444" or "4444-").
      if (!runNumbers.at(0).empty())
      {
        startRange = boost::lexical_cast<double>(runNumbers.at(0));
        // We set the end range to be equal now, so we do not have to do a check if it exists later.
        endRange   = boost::lexical_cast<double>(runNumbers.at(0));
      }

      // If the user has input a start and end range, or just an end range ("4444-4449" or "-4449").
      if (runNumbers.size() == 2)
      {
        // Has the user input an end range...
        if (!runNumbers.at(1).empty())
        {
          endRange = boost::lexical_cast<double>(runNumbers.at(1));

          // If they have not chosen a start range ("-4449");
          if (startRange == 0)
          {
            startRange = boost::lexical_cast<double>(runNumbers.at(1));
          }
        }
      }

      if (startRange > endRange)
      {
        throw std::runtime_error("Run end number cannot be lower than run start number.");
      }

      params.setRunStart(startRange);
      params.setRunEnd(endRange);
    }

  }
}

