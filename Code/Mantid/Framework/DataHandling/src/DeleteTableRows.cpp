//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidDataHandling/DeleteTableRows.h"
#include "MantidAPI/IWorkspaceProperty.h"
#include "MAntidAPI/ITableWorkspace.h"
#include "MantidKernel/ArrayProperty.h"

#include <set>
#include <functional>

namespace Mantid
{
  namespace DataHandling
  {
    // Register the algorithm into the algorithm factory
    DECLARE_ALGORITHM(DeleteTableRows);
    
    /// Sets documentation strings for this algorithm
    void DeleteTableRows::initDocs()
    {
      this->setWikiSummary("Deletes a row from a TableWorkspace.");
      this->setOptionalMessage("Deletes a row from a TableWorkspace.");
    }
    

    using namespace Kernel;
    using namespace API;

    /// Initialisation method.
    void DeleteTableRows::init()
    {
      declareProperty(new WorkspaceProperty<API::ITableWorkspace>("TableWorkspace", "",Direction::InOut), 
        "The name of the workspace that will be modified.");
      declareProperty(new ArrayProperty<size_t> ("Rows"),"The rows to delete.");
    }

    /** 
    *   Executes the algorithm.
    */
    void DeleteTableRows::exec()
    {
      API::ITableWorkspace_sptr tw = getProperty("TableWorkspace");
      std::vector<size_t> rows = getProperty("Rows");
      // sort the row indices in reverse order
      std::set<size_t,std::greater<size_t> > sortedRows(rows.begin(),rows.end());
      std::set<size_t,std::greater<size_t> >::iterator it = sortedRows.begin();
      for(; it != sortedRows.end(); ++it)
      {
        tw->removeRow(*it);
      }
      setProperty("TableWorkspace",tw);
    }

  } // namespace DataHandling
} // namespace Mantid
