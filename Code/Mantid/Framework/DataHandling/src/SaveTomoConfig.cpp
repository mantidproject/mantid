#include "MantidAPI/FileProperty.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidDataHandling/SaveTomoConfig.h"
#include "MantidDataObjects/TableWorkspace.h"
#include <Poco/File.h>

#include <nexus/NeXusException.hpp>
#include <nexus/NeXusFile.hpp>


namespace Mantid
{
  namespace DataHandling
  {    
    // Register the algorithm into the algorithm factory
    DECLARE_ALGORITHM(SaveTomoConfig)

    using namespace Kernel;
    using namespace API;
    using namespace DataObjects;

    SaveTomoConfig::SaveTomoConfig() :  API::Algorithm()
    {
      m_pluginInfoCount = 4;
    }

    /**
     * Initialise the algorithm
     */
    void SaveTomoConfig::init()
    {
      // Get a list of table workspaces which contain the plugin information
      declareProperty(new ArrayProperty<std::string>("InputWorkspaces",  ""), 
        "The names of the table workspaces containing plugin information.");

      declareProperty(new API::FileProperty("Filename", "", FileProperty::Save, std::vector<std::string>(1,".nxs")),
        "The name of the tomographic config file to write, as a full or relative path. This will overwrite existing files.");      
    }

    /**
     * Execute the algorithm
     */
    void SaveTomoConfig::exec()
    {
      // Prepare properties for writing to file
      std::string fileName = getPropertyValue("Filename");

      std::vector<std::string> workspaces = getProperty("InputWorkspaces");
      std::vector<TableWorkspace_sptr> wsPtrs;

      for(auto it=workspaces.begin();it!=workspaces.end();++it)
      {
        if(AnalysisDataService::Instance().doesExist(*it))
        {
          TableWorkspace_sptr table = AnalysisDataService::Instance().retrieveWS<TableWorkspace>(*it);
          // Check it's valid
          if(table && table->columnCount() == m_pluginInfoCount)
          {
            wsPtrs.push_back(table);
          }
          else
          {
            throw std::runtime_error("Invalid workspaces entered, requires table with correct plugin information"); 
          }
        }
        else
        {
          throw std::runtime_error("One or more specified table workspaces don't exist.");
        }
      }

      // Ensure it has a .nxs extension
      if(!boost::ends_with(fileName, ".nxs"))
        fileName = fileName + ".nxs"; 

      // If file exists, delete it.
      Poco::File file(fileName);
      if(file.exists())
        file.remove();

      // Create the file handle
      NXhandle fileHandle;
      NXstatus status = NXopen(fileName.c_str(), NXACC_CREATE5, &fileHandle);
      
      if(status==NX_ERROR)
        throw std::runtime_error("Unable to create file.");   
      
      ::NeXus::File nxFile(fileHandle);    
   
      // Make the top level entry (and open it)
      nxFile.makeGroup("entry1", "NXentry", true);
     
      nxFile.makeGroup("processing", "NXsubentry", true);

      // Iterate through all plugin entries (number sub groups 0....n)
      for(size_t i=0;i<wsPtrs.size();++i)
      {
        // Column info order is [ID / Params {as json string} / name {description} / citation info]
        std::string id = wsPtrs[i]->cell<std::string>(0,0);
        std::string params = wsPtrs[i]->cell<std::string>(0,1);
        std::string name = wsPtrs[i]->cell<std::string>(0,2);
        std::string cite = wsPtrs[i]->cell<std::string>(0,3);
                
        nxFile.makeGroup(boost::lexical_cast<std::string>(i), "NXsubentry", true);

        nxFile.writeData("id", id);
        nxFile.writeData("params", params);
        nxFile.writeData("name",name);
        nxFile.writeData("cite",cite);

        nxFile.closeGroup();
      }
                 
      nxFile.closeGroup(); // processing sub-group

      nxFile.makeGroup("intermediate", "NXsubEntry", false);
      nxFile.makeGroup("raw_data", "NXsubEntry", false);

      nxFile.closeGroup(); // Top level NXentry      

      nxFile.close();
    }


  } // namespace DataHandling
} // namespace Mantid


