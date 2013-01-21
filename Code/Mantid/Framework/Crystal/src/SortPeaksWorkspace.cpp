/*WIKI*
TODO: Enter a full wiki-markup description of your algorithm here. You can then use the Build/wiki_maker.py script to generate your full wiki page.
*WIKI*/

#include "MantidCrystal/SortPeaksWorkspace.h"
#include "MantidKernel/PropertyWithValue.h"
#include "MantidKernel/MandatoryValidator.h"
#include "MantidAPI/WorkspaceProperty.h"
#include "MantidAPI/IPeaksWorkspace.h"
#include "MantidDataObjects/PeaksWorkspace.h"
#include <boost/make_shared.hpp>

using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace Mantid::DataObjects;

namespace Mantid
{
namespace Crystal
{

  // Register the algorithm into the AlgorithmFactory
  DECLARE_ALGORITHM(SortPeaksWorkspace)
  


  //----------------------------------------------------------------------------------------------
  /** Constructor
   */
  SortPeaksWorkspace::SortPeaksWorkspace()
  {
  }
    
  //----------------------------------------------------------------------------------------------
  /** Destructor
   */
  SortPeaksWorkspace::~SortPeaksWorkspace()
  {
  }
  

  //----------------------------------------------------------------------------------------------
  /// Algorithm's name for identification. @see Algorithm::name
  const std::string SortPeaksWorkspace::name() const { return "SortPeaksWorkspace";};
  
  /// Algorithm's version for identification. @see Algorithm::version
  int SortPeaksWorkspace::version() const { return 1;};
  
  /// Algorithm's category for identification. @see Algorithm::category
  const std::string SortPeaksWorkspace::category() const { return "Crystal";}

  //----------------------------------------------------------------------------------------------
  /// Sets documentation strings for this algorithm
  void SortPeaksWorkspace::initDocs()
  {
    this->setWikiSummary("Sort a peaks workspace by a column");
    this->setOptionalMessage("Sort a peaks workspace by a column");
  }

  //----------------------------------------------------------------------------------------------
  /** Initialize the algorithm's properties.
   */
  void SortPeaksWorkspace::init()
  {
    declareProperty(new WorkspaceProperty<IPeaksWorkspace>("InputWorkspace","", Direction::Input), "An input workspace.");
    declareProperty(new WorkspaceProperty<IPeaksWorkspace>("OutputWorkspace","",Direction::Output), "An output workspace.");

    auto mustHave = boost::make_shared<MandatoryValidator<std::string> >();
    declareProperty("ColumnNameToSortBy", "", mustHave,
          "Column to sort by");

    declareProperty("SortAscending", true, "Sort the OutputWorkspace by the target column in a Ascending fashion.");
  }

  //----------------------------------------------------------------------------------------------
  /** Execute the algorithm.
   */
  void SortPeaksWorkspace::exec()
  {
     const std::string columnToSortBy = getProperty("ColumnNameToSortBy");
     const bool sortAscending = getProperty("SortAscending");
     IPeaksWorkspace_const_sptr temp = getProperty("InputWorkspace");
     PeaksWorkspace_const_sptr inputWS = boost::dynamic_pointer_cast<const PeaksWorkspace>(temp);
     if(inputWS == NULL)
     {
       throw std::invalid_argument("InputWorkspace is not a PeaksWorkspace.");
     }

     try
     {
       Column_const_sptr column = inputWS->getColumn(columnToSortBy);
       PeaksWorkspace_sptr outputWorkspace =  boost::shared_ptr<PeaksWorkspace>(inputWS->clone());

       // Perform the sorting.
       std::vector< std::pair<std::string, bool> > sortCriteria;
       sortCriteria.push_back( std::pair<std::string, bool>(columnToSortBy, sortAscending) );
       outputWorkspace->sort(sortCriteria);

       setProperty("OutputWorkspace", outputWorkspace);
     }
     catch(std::invalid_argument& ex)
     {
       this->g_log.error("Specified ColumnToSortBy does not exist");
       throw ex;
     }

  }



} // namespace Crystal
} // namespace Mantid
