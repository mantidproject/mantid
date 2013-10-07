/*WIKI*

This algorithm retrieves the files associated to selected investigation from the information catalog and saves the file search results to mantid workspace.

*WIKI*/

#include "MantidICat/CatalogGetDataFiles.h"
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
    using std::size_t;

    DECLARE_ALGORITHM(CatalogGetDataFiles)

    /// Sets documentation strings for this algorithm
    void CatalogGetDataFiles::initDocs()
    {
      this->setWikiSummary("Gets the files associated to the selected investigation.");
      this->setOptionalMessage("Gets the files associated to the selected investigation.");
    }

    /// Initialising the algorithm
    void CatalogGetDataFiles::init()
    {
      auto mustBePositive = boost::make_shared<BoundedValidator<int64_t> >();
      mustBePositive->setLower(0);
      declareProperty<int64_t>("InvestigationId",-1,mustBePositive,"Id of the selected investigation");

      declareProperty(new WorkspaceProperty<API::ITableWorkspace> ("OutputWorkspace", "", Direction::Output),
          "The name of the workspace to store the file data search details");
      declareProperty("FilterLogFiles",false,"Use this boolean option to filter log files from the list of files associated to the investigation.\n"
          "The default option is set to false and loads all the files assocaited to the selected investigation.");
    }

    //execute the algorithm
    void CatalogGetDataFiles::exec()
    {
      ICatalog_sptr catalog_sptr;
      try
      {
        catalog_sptr=CatalogFactory::Instance().create(ConfigService::Instance().getFacility().catalogInfo().catalogName());
      }
      catch(Kernel::Exception::NotFoundError&)
      {
        throw std::runtime_error("Error when getting the catalog information from the Facilities.xml file.");
      }
      if(!catalog_sptr)
      {
        throw std::runtime_error("Error when getting the catalog information from the Facilities.xml file");
      }

      int64_t investigationId = getProperty("InvestigationId");
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
    void CatalogGetDataFiles::filterLogFiles( API::ITableWorkspace_sptr& ws_sptr)
    {
      if(!ws_sptr)
      {
        return;
      }
      /// now filter log files
      for( size_t row=0;row<ws_sptr->rowCount();)
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
    bool CatalogGetDataFiles::isDataFile(const std::string& fileName)
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
