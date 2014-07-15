#include "MantidDataHandling/SortTableWorkspace.h"
#include "MantidAPI/ITableWorkspace.h"
#include "MantidKernel/ArrayProperty.h"

namespace Mantid
{
namespace DataHandling
{

  using Mantid::Kernel::Direction;
  using Mantid::API::WorkspaceProperty;

  // Register the algorithm into the AlgorithmFactory
  DECLARE_ALGORITHM(SortTableWorkspace)
  


  //----------------------------------------------------------------------------------------------
  /** Constructor
   */
  SortTableWorkspace::SortTableWorkspace()
  {
  }
    
  //----------------------------------------------------------------------------------------------
  /** Destructor
   */
  SortTableWorkspace::~SortTableWorkspace()
  {
  }
  

  //----------------------------------------------------------------------------------------------

  
  /// Algorithm's version for identification. @see Algorithm::version
  int SortTableWorkspace::version() const { return 1;};
  
  /// Algorithm's category for identification. @see Algorithm::category
  const std::string SortTableWorkspace::category() const { return "General";}

  /// Algorithm's summary for use in the GUI and help. @see Algorithm::summary
  const std::string SortTableWorkspace::summary() const { return "Sort a TableWorkspace.";};

  //----------------------------------------------------------------------------------------------
  /** Initialize the algorithm's properties.
   */
  void SortTableWorkspace::init()
  {
    declareProperty(new WorkspaceProperty<API::ITableWorkspace>("InputWorkspace","",Direction::Input), "An input workspace.");
    declareProperty(new WorkspaceProperty<API::ITableWorkspace>("OutputWorkspace","",Direction::Output), "An output workspace.");
    declareProperty(new Kernel::ArrayProperty<std::string>("Columns"), "Column names to sort by.");
    declareProperty(new Kernel::ArrayProperty<int>("Ascending"), "List of bools for each column: true for ascending order, false for descending. "
      "If contains a single value it applies to all columns.");
  }

  //----------------------------------------------------------------------------------------------
  /** Execute the algorithm.
   */
  void SortTableWorkspace::exec()
  {
    API::ITableWorkspace_sptr ws = getProperty("InputWorkspace");
    std::vector<std::string> columns = getProperty("Columns");
    std::vector<int> ascending = getProperty("Ascending");
    std::vector< std::pair<std::string,bool> > sortingOptions;

    // if "Ascending" contains a single value - it's common for all columns.
    if ( ascending.size() == 1 )
    {
    }
    else if ( ascending.size() != columns.size() )
    {
      throw std::invalid_argument("Number of sorting options is different form number of columns.");
    }
  }



} // namespace DataHandling
} // namespace Mantid