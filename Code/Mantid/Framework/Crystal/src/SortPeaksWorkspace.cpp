/*WIKI*
TODO: Enter a full wiki-markup description of your algorithm here. You can then use the Build/wiki_maker.py script to generate your full wiki page.
*WIKI*/

#include "MantidCrystal/SortPeaksWorkspace.h"
#include "MantidKernel/PropertyWithValue.h"
#include "MantidKernel/MandatoryValidator.h"
#include "MantidAPI/WorkspaceProperty.h"
#include "MantidAPI/IPeaksWorkspace.h"
#include <boost/make_shared.hpp>

using namespace Mantid::Kernel;
using namespace Mantid::API;

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
  }

  //----------------------------------------------------------------------------------------------
  /** Execute the algorithm.
   */
  void SortPeaksWorkspace::exec()
  {
    // TODO Auto-generated execute stub

  }



} // namespace Crystal
} // namespace Mantid
