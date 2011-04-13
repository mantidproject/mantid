//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAlgorithms/Sort.h"
#include "MantidAPI/WorkspaceValidators.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidKernel/ArrayProperty.h"

namespace Mantid
{
  namespace Algorithms
  {

    // Register the class into the algorithm factory
    DECLARE_ALGORITHM(Sort)
    
    /// Sets documentation strings for this algorithm
    Sort::Sort()
    {
      this->useAlgorithm("SortEvents");
      this->deprecatedDate("2011-04-13");
    }
    

  } // namespace Algorithm
} // namespace Mantid
