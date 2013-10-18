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
#include "MantidKernel/PropertyWithValue.h"
#include "MantidKernel/BoundedValidator.h"
#include "MantidAPI/WorkspaceProperty.h"
#include "MantidKernel/DateValidator.h"
#include "MantidAPI/CatalogFactory.h"
#include "MantidKernel/ConfigService.h"
#include "MantidKernel/FacilityInfo.h"
#include "MantidAPI/ICatalog.h"


#include<limits>

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
      auto mustBePositive = boost::make_shared<BoundedValidator<double> >();
      auto isDate = boost::make_shared<DateValidator>();
      mustBePositive->setLower(0.0);

      declareProperty("InvestigationName", "", "The name of the investigation to search.");
      declareProperty("Instrument","","The name of the instrument used for investigation search.");
      declareProperty("RunRange","","The range of runs to search for related investigations.");
      declareProperty("StartDate","", isDate, "The start date for the range of investigations to be searched.The format is DD/MM/YYYY.");
      declareProperty("EndDate","", isDate, "The end date for the range of investigations to be searched.The format is DD/MM/YYYY.");
      declareProperty("Keywords","","An option to search investigations data");

      declareProperty("myData",false, "Boolean option to do my data only search.");

      // NOTE: This has changed in ICAT4 to simply "investigator's name". Will need fixed when ICAT3 is removed.
      declareProperty("InvestigatorSurname", "", "The surname of the investigator associated to the investigation.");

      declareProperty("SampleName", "", "The name of the sample used in the investigation to search.");
      declareProperty("InvestigationAbstract", "", "The abstract of the investigation to search.");
      declareProperty("InvestigationType", "", "The type  of the investigation to search.");

      // Note: These are ICAT3 specific, and can be removed when ICAT3 is.
      // Will need to update header documentation upon removal.
      // Moreover, their relevant getters/setters in CatalogSearchParam can to.
      declareProperty("StartRun",0.0,mustBePositive,"The start run number for the range of investigations to be searched.");
      declareProperty("EndRun",0.0,mustBePositive,"The end run number for the range of investigations to be searched.");
      declareProperty("CaseSensitive", false, "Boolean option to do case sensitive ICat investigations search.");
      declareProperty("DataFileName","", "The name of the data file to search.");

      declareProperty(new WorkspaceProperty<API::ITableWorkspace> ("OutputWorkspace", "", Direction::Output),
          "The name of the workspace that will be created to store the ICat investigations search result.");

    }
    /// Execution method.
    void CatalogSearch::exec()
    {
      ICatalog_sptr catalog;
      try
      {
        catalog = CatalogFactory::Instance().create(ConfigService::Instance().getFacility().catalogName());
      }
      catch(Kernel::Exception::NotFoundError&)
      {
        std::string facilityName = ConfigService::Instance().getFacility().name();
        std::stringstream ss;
        ss << "Your current Facility: " << facilityName << " does not have ICAT catalog information. " << std::endl;
        ss << "The facilities.xml file may need updating. Contact the Mantid Team for help." << std::endl;
        throw std::runtime_error(ss.str());
      }
      if(!catalog)
      {
        throw std::runtime_error("Error when getting the catalog information from the Facilities.xml file");
      }

      CatalogSearchParam params;
      // Get the user input search terms to search for.
      getInputProperties(params);
      // Create output workspace.
      ITableWorkspace_sptr workspace = WorkspaceFactory::Instance().createTable("TableWorkspace");
      // Search for investigations in the archives.
      catalog->search(params,workspace);
      // Search for investigations with user specific search inputs.
      setProperty("OutputWorkspace",workspace);

    }

    /**This method gets the input properties for the algorithm.
     * @param params :: reference to inputs object
     */
    void CatalogSearch::getInputProperties(CatalogSearchParam& params)
    {
      double dstartRun=getProperty("StartRun");
      // NOTE: This is for ICAT3 support, and can be removed when it's removed. (startRun, endRun, and lowercase
      if(dstartRun<0)
      {
        throw std::runtime_error("Invalid Start Run Number.Enter a valid run number to do investigations search");
      }
      double dendRun=getProperty("EndRun");
      if(dendRun<0)
      {
        throw std::runtime_error("Invalid End Run Number.Enter a valid run number to do investigations search");
      }
      if(dstartRun>dendRun)
      {
        throw std::runtime_error("Run end number cannot be lower than run start number");
      }

      // Need to set them here for ICAT3 support. They don't exist in ICAT4, and will therefore be empty.
      params.setRunStart(dstartRun);
      params.setRunEnd(dendRun);

      // Obtain the ICAT4 runRange input text.
      std::string runRange = getProperty("runRange");

      // A container to hold the range of run numbers.
      std::vector<std::string> runNumbers;
      // Split the input text by "-" and add contents to runNumbers.
      boost::split(runNumbers,runRange,boost::is_any_of("-"));

      // Has the user input a runRange?
      if (!runRange.empty())
      {
        params.setRunStart(boost::lexical_cast<double>(runNumbers.at(0)));
        params.setRunEnd( boost::lexical_cast<double>(runNumbers.at(1)));
      }

      std::string instrument = getPropertyValue("Instrument");
      // As ICat API is expecting instrument name in uppercase
      // NOTE: This is no longer needed for ICAT4, and can be removed when ICAT3 is.
      std::transform(instrument.begin(),instrument.end(),instrument.begin(),toupper);
      // NOTE: This is not needed in ICAT4 as our search query searches all instruments if empty.
      if(!instrument.empty())
      {
        params.setInstrument(instrument);
      }

      std::string date = getPropertyValue("StartDate");
      time_t startDate = params.getTimevalue(date);
      if(startDate==-1)
      {
        throw std::runtime_error("Invalid date.Enter a valid date in DD/MM/YYYY format");
      }
      date = getPropertyValue("EndDate");
      time_t endDate = params.getTimevalue(date);
      if(endDate==-1)
      {
        throw std::runtime_error("Invalid date.Enter a valid date in DD/MM/YYYY format");
      }

      params.setStartDate(startDate);

      params.setEndDate(endDate);

      std::string keyWords=getPropertyValue("Keywords");
      params.setKeywords(keyWords);

      bool bCase=getProperty("CaseSensitive");
      params.setCaseSensitive(bCase);

      std::string invstName=getPropertyValue("InvestigationName");
      params.setInvestigationName(invstName);

      std::string invstType=getPropertyValue("InvestigationType");
      params.setInvestigationType(invstType);

      std::string invstAbstarct=getPropertyValue("InvestigationAbstract");
      params.setInvestigationAbstract(invstAbstarct);

      std::string sampleName=getPropertyValue("SampleName");
      params.setSampleName(sampleName);

      std::string invstSurname=getPropertyValue("InvestigatorSurname");
      params.setInvestigatorSurName(invstSurname);

      std::string dataFileName=getPropertyValue("DataFileName");
      params.setDatafileName(dataFileName);

      bool mydata = boost::lexical_cast<bool>(getPropertyValue("myData"));
      params.setMyData(mydata);
    }


  }
}

