#include "MantidICat/AdvancedSearch.h"
#include "MantidKernel/PropertyWithValue.h"
#include "MantidKernel/BoundedValidator.h"
#include "MantidAPI/WorkspaceProperty.h"
#include "MantidICat/Session.h"
#include "MantidKernel/DateValidator.h"


namespace Mantid
{
	namespace ICat
	{
		using namespace Kernel;
		using namespace API;
		DECLARE_ALGORITHM(CAdvancedSearch)
				
		/// Initialisation method.
		void CAdvancedSearch::init()
		{
			BoundedValidator<double>* mustBePositive = new BoundedValidator<double>();
			mustBePositive->setLower(0.0);
	
			declareProperty("StartRun",0.0,mustBePositive,"The start run number for the range of investigations to be searched.");
			declareProperty("EndRun",0.0,mustBePositive->clone(),"The end run number for the range of investigations to be searched.");
			
			declareProperty("Instrument","","The list of instruments used in ISIS nuetron scattering experiments.");
			declareProperty("StartDate","",new DateValidator(),"The start date for the range of investigations to be searched.The format is DD/MM/YYYY.");
			declareProperty("EndDate","",new DateValidator(),"The end date for the range of investigations to be searched.The format is DD/MM/YYYY.");
			declareProperty("Keywords","","The keywords used for the  investigations search");
			declareProperty("Case Sensitive",false,"Bolean option to do case senistive investigations search.");
			
			declareProperty("Investigation Name", "", "The name of the investigation to search.");
			declareProperty("Investigation Type", "", "The type  of the investigation to search.");
			declareProperty("Investigation Abstract", "", "The abstract of the investigation to search.");
			declareProperty("Sample Name", "", "The name of the sample used in the investigation to search.");
			declareProperty("Investigator SurName", "", "The sur name of the investigation to search.");
			declareProperty("DataFile Name","", "The name of the data file to search.");

			declareProperty(new WorkspaceProperty<API::ITableWorkspace> ("OutputWorkspace", "", Direction::Output),
				"The name of the workspace that will be created to store the ICat investigations search result.");
			
		}
		/// Execution method.
		void CAdvancedSearch::exec()
		{	
			if(Session::Instance().getSessionId().empty())
			{
				throw std::runtime_error("Please login to ICat using the ICat:Login menu provided to access ICat data.");
			}
			ITableWorkspace_sptr ws_sptr = WorkspaceFactory::Instance().createTable("TableWorkspace"); 
			doAdvancedSearch(ws_sptr);
			setProperty("OutputWorkspace",ws_sptr);
			
		}
		/** This method does advanced search by calling icat api
		  *@param outputws shared pointer to output workspace
		*/
		void CAdvancedSearch::doAdvancedSearch(ITableWorkspace_sptr& outputws)
		{
			CSearchInput inputs;CICatHelper searchobj;
			getInputProperties(searchobj,inputs);
			
			searchobj.doAdvancedSearch(inputs,outputws);
		}

		/** This method gets the input properties of the algorithm
		  *@param inputs reference to searchinput class.
		*/
		void CAdvancedSearch::getInputProperties(CICatHelper& helper,CSearchInput& inputs)
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
			inputs.setRunStart(dstartRun);
			inputs.setRunEnd(dendRun);

			std::string instrument = getPropertyValue("Instrument");
			// as ICat API is expecting instrument name in uppercase 
			std::transform(instrument.begin(),instrument.end(),instrument.begin(),toupper);
			if(!instrument.empty())
			{
				inputs.setInstrument(instrument);
			}

			std::string date = getPropertyValue("StartDate");
			time_t startDate = helper.getTimevalue(date);
			if(startDate==-1)
			{
				throw std::runtime_error("Invalid date.Enter a valid date in DD/MM/YYYY format");
			}
			

			date = getPropertyValue("EndDate");
			time_t endDate = helper.getTimevalue(date);
			if(endDate==-1)
			{
				throw std::runtime_error("Invalid date.Enter a valid date in DD/MM/YYYY format");
			}
			
			if(startDate>endDate)
			{
				throw std::runtime_error("End date cannot be lower than Start date");
			}
			
			inputs.setEndDate(endDate);
			inputs.setStartDate(startDate);

			std::string keyWords=getPropertyValue("Keywords");
			inputs.setKeywords(keyWords);

			bool bCase=getProperty("Case Sensitive");
			inputs.setCaseSensitive(bCase);


			std::string invstName=getPropertyValue("Investigation Name");
			inputs.setInvestigationName(invstName);

			std::string invstType=getPropertyValue("Investigation Type");
			inputs.setInvestigationType(invstType);

			std::string invstAbstarct=getPropertyValue("Investigation Abstract");
			inputs.setInvestigationAbstract(invstAbstarct);

			std::string sampleName=getPropertyValue("Sample Name");
			inputs.setSampleName(sampleName);

			std::string invstSurname=getPropertyValue("Investigator SurName");
			inputs.setInvestigatorSurName(invstSurname);

			std::string dataFileName=getPropertyValue("DataFile Name");
			inputs.setDatafileName(dataFileName);

			inputs.setInvestigationInclude(ns1__investigationInclude__INVESTIGATORS_USCORESHIFTS_USCOREAND_USCORESAMPLES);
		}

	}
}
