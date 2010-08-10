#include "MantidICat/SearchByRunNumber.h"
#include"MantidICat/Session.h"
#include "MantidKernel/PropertyWithValue.h"
#include "MantidKernel/BoundedValidator.h"
#include "MantidAPI/WorkspaceProperty.h"
#include"MantidICat/SearchHelper.h"

#include <time.h>
#include <iomanip>


namespace Mantid
{
	namespace ICat
	{
		using namespace Kernel;
		using namespace API;

		DECLARE_ALGORITHM(CSearchByRunNumber)
		/// Initialisation method.
		void CSearchByRunNumber::init()
		{
			BoundedValidator<double>* mustBePositive = new BoundedValidator<double>();
			mustBePositive->setLower(0.0);

			declareProperty("StartRun",0.0,"The start run number for the range of investigations to be searched.");
			declareProperty("EndRun",0.0,"The end run number for the range of investigations to be searched.");
			
			declareProperty("Instrument","","The list of instruments used in ISIS nuetron scattering experiments.");
			declareProperty("StartDate","","The start date for the range of investigations to be searched.The format is DD/MM/YYYY.");
			declareProperty("EndDate","","The end date for the range of investigations to be searched.The format is DD/MM/YYYY.");
			declareProperty("Keywords","","An option to search investigations data");
			declareProperty("Case Sensitive", false, "Boolean option to do case sensitive ICat investigations search.");

			declareProperty(new WorkspaceProperty<API::ITableWorkspace> ("OutputWorkspace", "", Direction::Output),
				"The name of the workspace that will be created to store the ICat investigations search result.");
		}
		/// Execution method.
		void CSearchByRunNumber::exec()
		{			
			API::ITableWorkspace_sptr ws_sptr=doSearchByRunNumber();
			setProperty("OutputWorkspace",ws_sptr);
		}

		/* This method does search by run number and instrument name.
		 * @returns shared pointer to table workspace which stores the data
		 */
		API::ITableWorkspace_sptr  CSearchByRunNumber::doSearchByRunNumber()
		{	
			CSearchInput inputs;
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
			if(!instrument.empty())
			{
				inputs.setInstrument(instrument);
			}

			std::string date = getPropertyValue("StartDate");
			time_t startDate = getTimevalue(date);
			if(startDate==-1)
			{
				throw std::runtime_error("Invalid date.Enter a valid date in DD/MM/YYYY format");
			}
			
			inputs.setStartDate(startDate);
		
			date = getPropertyValue("EndDate");
			time_t endDate = getTimevalue(date);
			if(endDate==-1)
			{
				throw std::runtime_error("Invalid date.Enter a valid date in DD/MM/YYYY format");
			}
			inputs.setEndDate(endDate);
			
			std::string keyWords=getPropertyValue("Keywords");
			inputs.setKeywords(keyWords);
			
			bool bCase=getProperty("Case Sensitive");
			inputs.setCaseSensitive(bCase);
			
			inputs.setInvestigationInclude(ns1__investigationInclude__DATASETS_USCOREAND_USCOREDATAFILES);
	
			API::ITableWorkspace_sptr outputws = WorkspaceFactory::Instance().createTable("TableWorkspace"); 
			CSearchHelper searchobj;
			//int ret_advsearch=searchobj.doSearchByRunNumber(dstartRun,dendRun,bCase,instrument,
			//	ns1__investigationInclude__DATASETS_USCOREAND_USCOREDATAFILES,outputws);
			int ret_search=searchobj.doIsisSearch(inputs,outputws);
			return outputws;
		
		}
		/**This method saves the date components to C library struct tm
		  *@param sDate
		  *@return time_t 
		*/
		time_t CSearchByRunNumber::getTimevalue(const std::string& sDate)
		{
			if((!sDate.compare("DD/MM/YYYY")) || (!sDate.compare("dd/mm/yyyy")) )
			{
				return 0;
			}
			if(!sDate.compare(""))
			{
				return 0;
			}
			//struct tm * timeinfo =new tm;;
			std::basic_string <char>::size_type index,off=0;
			int day,month,year;
		
			//timeinfo
			struct tm  timeinfo;
		    day=atoi(sDate.substr(off,2).c_str());
			timeinfo.tm_mday=day;
			
			index=sDate.find('/',off);
			if(index!=std::string::npos)
			{
			off=index+1;
			}
			else
			{
				throw std::runtime_error("Invalid Date:date format must be DD/MM/YYYY");
			}
			month=atoi(sDate.substr(off,2).c_str());
			timeinfo.tm_mon=month-1;
			
			index=sDate.find('/',off);
			if(index!=std::string::npos)
			{
				off=index+1;
			}
			else
			{
				throw std::runtime_error("Invalid Date:date format must be DD/MM/YYYY");
			}
			year=atoi(sDate.substr(off,4).c_str());
						
			timeinfo.tm_year=year-1900;
			validateTimeFormat(timeinfo);
			timeinfo.tm_min=0;
			timeinfo.tm_sec=0;
			timeinfo.tm_hour=0;
			//timeinfo->tm_isdst=-1;
			 return std::mktime (&timeinfo );
		}
		/**This method validates the date properties
		  *@param timeinfo
		 */
		void CSearchByRunNumber::validateTimeFormat(const struct tm &timeinfo)
		{
			if (timeinfo.tm_mday<1 || timeinfo.tm_mday>31)
			{
				throw std::runtime_error("Invalid Date:Day part of search parameter Date must be between 1 and 31 ");
			}
			if (timeinfo.tm_mon<0 || timeinfo.tm_mon>11)
			{
				throw std::runtime_error("Invalid Date:Month part of search parameter Date must be between 1 and 12 ");
			}
		}

	
	}
}

