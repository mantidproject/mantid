/*WIKI* 
*WIKI*/
//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------

#include "MantidDataHandling/SaveReflTBL.h"
#include "MantidAPI/FileProperty.h"

#include <set>
#include <fstream>
#include <boost/tokenizer.hpp>

namespace Mantid
{
  namespace DataHandling
  {
    // Register the algorithm into the algorithm factory
    DECLARE_ALGORITHM(SaveReflTBL)
    
    /// Sets documentation strings for this algorithm
    void SaveReflTBL::initDocs()
    {
      this->setWikiSummary("Saves a table [[workspace]] to a reflectometry tbl format ascii file. ");
      this->setOptionalMessage("Saves a table workspace to a reflectometry tbl format ascii file.");
    }
    
    using namespace Kernel;
    using namespace API;

    // Initialise the logger
    Logger& SaveReflTBL::g_log = Logger::get("SaveReflTBL");

    /// Empty constructor
    SaveReflTBL::SaveReflTBL() : m_separatorIndex()
    {
    }

    /// Initialisation method.
    void SaveReflTBL::init()
    {
      std::vector<std::string> exts;
      exts.push_back(".tbl");

      declareProperty(new FileProperty("Filename", "", FileProperty::Load, exts),
        "The filename of the output TBL file.");

      declareProperty(new WorkspaceProperty<ITableWorkspace>("InputWorkspace",
        "",Direction::Input), "The name of the workspace containing the data you want to save to a TBL file.");
    }

    /** 
     *   Executes the algorithm.
     */
    void SaveReflTBL::exec()
    {
    }

  } // namespace DataHandling
} // namespace Mantid
