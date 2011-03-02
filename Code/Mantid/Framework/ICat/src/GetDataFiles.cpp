#include "MantidICat/GetDataFiles.h"
#include "MantidKernel/PropertyWithValue.h"
#include "MantidKernel/BoundedValidator.h"
#include "MantidAPI/WorkspaceProperty.h"

#include "MantidAPI/CatalogFactory.h"
#include "MantidKernel/ConfigService.h"
#include "MantidKernel/FacilityInfo.h"
#include "MantidAPI/ICatalog.h"

namespace Mantid
{
	namespace ICat
	{
		using namespace Kernel;
		using namespace API;

		DECLARE_ALGORITHM(CGetDataFiles)

		/// Sets documentation strings for this algorithm
		void CGetDataFiles::initDocs()
		{
		  this->setWikiSummary("Gets the files associated to the selected investigation . ");
		  this->setOptionalMessage("Gets the files associated to the selected investigation .");
		}

		/// Initialising the algorithm
		void CGetDataFiles::init()
		{
			BoundedValidator<long long>* mustBePositive = new BoundedValidator<long long>();
			mustBePositive->setLower(0);
			declareProperty<long long>("InvestigationId",-1,mustBePositive,"Id of the selected investigation");
		
			declareProperty(new WorkspaceProperty<API::ITableWorkspace> ("OutputWorkspace", "", Direction::Output),
                            "The name of the workspace to store the file data search details");
			declareProperty("FilterLogFiles",false,"Use this boolean option to filter log files from the list of files associated to the investigation.\n"
				"The default option is set to false and loads all the files assocaited to the selected investigation.");
		}

		//execute the algorithm
		void CGetDataFiles::exec()
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
			
			long long investigationId = getProperty("InvestigationId");
			bool bfiletrLog =getProperty("FilterLogFiles");

			API::ITableWorkspace_sptr ws_sptr = WorkspaceFactory::Instance().createTable("TableWorkspace");
			catalog_sptr->getDataFiles(investigationId,ws_sptr);

			if(bfiletrLog)
			{
				filterLogFiles(ws_sptr);
			}
			
			setProperty("OutputWorkspace",ws_sptr);
		}

	  /**This method filters log files from the workspace
	    *@param ws_sptr :: shared pointer to workspace
		*/
		void CGetDataFiles::filterLogFiles( API::ITableWorkspace_sptr& ws_sptr)
		{
			if(!ws_sptr)
			{
				return;
			}
			/// now filter log files
			for( int row=0;row<ws_sptr->rowCount();)
			{
				if(!isDataFile(ws_sptr->cell<std::string>(row,0)))
				{
					ws_sptr->removeRow(row);
 				}
        else
        {
          ++row;
        }
			}
		}

	 /**This checks the datafile boolean  selected
		 * @param fileName :: name of the  file 
		 * @return bool - returns true if it's a raw file or nexus file
		 */
		bool CGetDataFiles::isDataFile(const std::string& fileName)
		{			
			std::basic_string <char>::size_type dotIndex;
			//find the position of '.' in raw/nexus file name
			dotIndex = fileName.find_last_of (".");
			std::string fextn=fileName.substr(dotIndex+1,fileName.size()-dotIndex);
			std::transform(fextn.begin(),fextn.end(),fextn.begin(),tolower);
			return ((!fextn.compare("raw")|| !fextn.compare("nxs")) ? true : false);
		}
		
		
	}
}
