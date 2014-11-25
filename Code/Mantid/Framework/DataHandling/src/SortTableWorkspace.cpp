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
  const std::string SortTableWorkspace::category() const { return "Utility";}

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

    if ( columns.empty() )
    {
      throw std::invalid_argument("No column names given.");
    }

    // by default sort all columns in ascending order
    if ( ascending.empty() )
    {
      ascending.push_back(1);
    }

    // if "Ascending" contains a single value - it's common for all columns.
    if ( ascending.size() == 1 )
    {
      int commonValue = ascending.front();
      ascending.resize( columns.size(), commonValue );
    }
    else if ( ascending.size() != columns.size() )
    {
      throw std::invalid_argument("Number of sorting options is different form number of columns.");
    }

    std::vector< std::pair<std::string, bool> > criteria(columns.size());
    auto col = columns.begin();
    auto asc = ascending.begin();
    for(auto crt = criteria.begin(); crt != criteria.end(); ++crt, ++col, ++asc)
    {
      crt->first = *col;
      crt->second = (*asc) != 0;
    }

    API::ITableWorkspace_sptr outputWS(ws->clone());
    outputWS->sort( criteria );
    setProperty("OutputWorkspace", outputWS);
    
  }



} // namespace DataHandling
} // namespace Mantid