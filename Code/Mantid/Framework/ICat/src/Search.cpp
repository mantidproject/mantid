#include "MantidICat/Search.h"
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

		DECLARE_ALGORITHM(CSearch)

		/// Sets documentation strings for this algorithm
		void CSearch::initDocs()
		{
		  this->setWikiSummary("Searches investigations ");
		  this->setOptionalMessage("Searches investigations");
		}

		/// Initialisation method.
		void CSearch::init()
		{
			BoundedValidator<double>* mustBePositive = new BoundedValidator<double>();
			mustBePositive->setLower(0.0);


			
			declareProperty("StartRun",0.0,mustBePositive,"The start run number for the range of investigations to be searched.");
			declareProperty("EndRun",0.0,mustBePositive->clone(),"The end run number for the range of investigations to be searched.");
			declareProperty("Instrument","","The name of the insurument used for investigation search.");
			declareProperty("StartDate","",new DateValidator(),"The start date for the range of investigations to be searched.The format is DD/MM/YYYY.");
			declareProperty("EndDate","",new DateValidator(),"The end date for the range of investigations to be searched.The format is DD/MM/YYYY.");
			declareProperty("Keywords","","An option to search investigations data");
			declareProperty("Case Sensitive", false, "Boolean option to do case sensitive ICat investigations search.");

			declareProperty("Investigation Name", "", "The name of the investigation to search.");
			declareProperty("Investigation Type", "", "The type  of the investigation to search.");
			declareProperty("Investigation Abstract", "", "The abstract of the investigation to search.");
			declareProperty("Sample Name", "", "The name of the sample used in the investigation to search.");
			declareProperty("Investigator SurName", "", "The sur name of the investigator associated to the investigation.");
			declareProperty("DataFile Name","", "The name of the data file to search.");

			declareProperty(new WorkspaceProperty<API::ITableWorkspace> ("OutputWorkspace", "", Direction::Output),
				"The name of the workspace that will be created to store the ICat investigations search result.");
			
		}
		/// Execution method.
		void CSearch::exec()
		{				
			ICatalog_sptr catalog_sptr;
			try
			{			
			 catalog_sptr=CatalogFactory::Instance().create(ConfigService::Instance().Facility().catalogName());
			
			}
			catch(Kernel::Exception::NotFoundError&)
			{
				throw std::runtime_error("Error when getting the catalog information from the Facilities.xml file.");
			} 
			if(!catalog_sptr)
			{
				throw std::runtime_error("Error when getting the catalog information from the Facilities.xml file");
			}
			
			//get the inputs
			CSearchParam params;
			getInputProperties(params);
			//create output workspace
			ITableWorkspace_sptr ws_sptr = WorkspaceFactory::Instance().createTable("TableWorkspace"); 
			// search for investigations
			catalog_sptr->search(params,ws_sptr);
			//set output workspace
			setProperty("OutputWorkspace",ws_sptr);
			
		}
				
		/**This method gets the input properties for the algorithm.
		  * @param params :: reference to inputs object
		 */
		void CSearch::getInputProperties(CSearchParam& params)
		{
			double dstartRun=getProperty("StartRun");
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
			params.setRunStart(dstartRun);
			params.setRunEnd(dendRun);

			std::string instrument = getPropertyValue("Instrument");
			// as ICat API is expecting instrument name in uppercase 
			std::transform(instrument.begin(),instrument.end(),instrument.begin(),toupper);
			
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

			if(startDate>endDate)
			{
				throw std::runtime_error("End date cannot be lower than Start date");
			}
			
			params.setStartDate(startDate);

			params.setEndDate(endDate);

			std::string keyWords=getPropertyValue("Keywords");
			params.setKeywords(keyWords);

			bool bCase=getProperty("Case Sensitive");
			params.setCaseSensitive(bCase);

			std::string invstName=getPropertyValue("Investigation Name");
			params.setInvestigationName(invstName);

			std::string invstType=getPropertyValue("Investigation Type");
			params.setInvestigationType(invstType);

			std::string invstAbstarct=getPropertyValue("Investigation Abstract");
			params.setInvestigationAbstract(invstAbstarct);

			std::string sampleName=getPropertyValue("Sample Name");
			params.setSampleName(sampleName);

			std::string invstSurname=getPropertyValue("Investigator SurName");
			params.setInvestigatorSurName(invstSurname);

			std::string dataFileName=getPropertyValue("DataFile Name");
			params.setDatafileName(dataFileName);


		}

	
	}
}

