/*WIKI* 

*WIKI*/
//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidDataHandling/LoadReflTBL.h"
#include "MantidDataObjects/TableWorkspace.h"
#include "MantidAPI/FileProperty.h"
#include "MantidAPI/RegisterFileLoader.h"
#include <fstream>

#include <boost/tokenizer.hpp>
#include <Poco/StringTokenizer.h>
// String utilities
#include <boost/algorithm/string.hpp>
#include <boost/regex.hpp>

namespace Mantid
{
  namespace DataHandling
  {
    DECLARE_FILELOADER_ALGORITHM(LoadReflTBL);

    /// Sets documentation strings for this algorithm
    void LoadReflTBL::initDocs()
    {
      this->setWikiSummary("Loads data from a reflectometry table file and stores it in a table [[workspace]] ([[TableWorkspace]] class). ");
      this->setOptionalMessage("Loads data from a reflectometry table file and stores it in a table workspace (TableWorkspace class).");
    }

    using namespace Kernel;
    using namespace API;

    /// Empty constructor
    LoadReflTBL::LoadReflTBL()
    {
    }

    /**
    * Return the confidence with with this algorithm can load the file
    * @param descriptor A descriptor for the file
    * @returns An integer specifying the confidence level. 0 indicates it will not be used
    */
    int LoadReflTBL::confidence(Kernel::FileDescriptor & descriptor) const
    {
      const std::string & filePath = descriptor.filename();
      const size_t filenameLength = filePath.size();

      // Avoid some known file types that have different loaders
      int confidence(0);
      if( filePath.compare(filenameLength - 12,12,"_runinfo.xml") == 0 ||
        filePath.compare(filenameLength - 6,6,".peaks") == 0 ||
        filePath.compare(filenameLength - 10,10,".integrate") == 0 )
      {
        confidence = 0;
      }
      else if(descriptor.isAscii())
      {
        if (true) //right ammount of columns
        {
          if (filePath.compare(filenameLength - 4,4,".tbl") == 0 )
          {
            confidence = 40;
          }
          else
          {
            confidence = 20;
          }
        }
        else //incorrect amount of columns
        {
          confidence = 0;
        }
      }
      return confidence;
    }

    //--------------------------------------------------------------------------
    // Private methods
    //--------------------------------------------------------------------------
    /// Initialisation method.
    void LoadReflTBL::init()
    {
      std::vector<std::string> exts;
      exts.push_back(".tbl");

      declareProperty(new FileProperty("Filename", "", FileProperty::Load, exts),
        "The name of the table file to read, including its full or relative path. The file extension must be .tbl");
      declareProperty(new WorkspaceProperty<ITableWorkspace>("OutputWorkspace",
        "",Direction::Output), "The name of the workspace that will be created, "
        "filled with the read-in data and stored in the [[Analysis Data Service]].");
    }

    /** 
    *   Executes the algorithm.
    */
    void LoadReflTBL::exec()
    {
    }
  } // namespace DataHandling
} // namespace Mantid