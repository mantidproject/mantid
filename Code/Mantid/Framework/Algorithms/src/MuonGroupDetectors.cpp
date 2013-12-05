/*WIKI*
Applies detector grouping to a workspace. (Muon version).

TODO: Table format description
*WIKI*/

#include "MantidAlgorithms/MuonGroupDetectors.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidDataObjects/TableWorkspace.h"


namespace Mantid
{
namespace Algorithms
{

  using namespace Kernel;
  using namespace API;
  using namespace DataObjects;

  // Register the algorithm into the AlgorithmFactory
  DECLARE_ALGORITHM(MuonGroupDetectors)

  //----------------------------------------------------------------------------------------------
  /** Constructor
   */
  MuonGroupDetectors::MuonGroupDetectors()
  {
  }
    
  //----------------------------------------------------------------------------------------------
  /** Destructor
   */
  MuonGroupDetectors::~MuonGroupDetectors()
  {
  }
  

  //----------------------------------------------------------------------------------------------
  /// Algorithm's name for identification. @see Algorithm::name
  const std::string MuonGroupDetectors::name() const { return "MuonGroupDetectors";};
  
  /// Algorithm's version for identification. @see Algorithm::version
  int MuonGroupDetectors::version() const { return 1;};
  
  /// Algorithm's category for identification. @see Algorithm::category
  const std::string MuonGroupDetectors::category() const { return "Muon"; }

  //----------------------------------------------------------------------------------------------
  /// Sets documentation strings for this algorithm
  void MuonGroupDetectors::initDocs()
  {
    this->setWikiSummary("Applies detector grouping to a workspace. (Muon version).");
    this->setOptionalMessage("Applies detector grouping to a workspace. (Muon version).");
  }

  //----------------------------------------------------------------------------------------------
  /** Initialize the algorithm's properties.
   */
  void MuonGroupDetectors::init()
  {
    declareProperty(new WorkspaceProperty<MatrixWorkspace>("InputWorkspace","",Direction::Input), 
        "Workspace to apply grouping to.");

    declareProperty(new WorkspaceProperty<TableWorkspace>("DetectorGroupingTable","",Direction::Input), 
        "Table with detector grouping information. Check wiki page for table format expected.");

    declareProperty(new WorkspaceProperty<MatrixWorkspace>("OutputWorkspace","",Direction::Output), 
        "Workspace with detectors grouped.");
  }

  //----------------------------------------------------------------------------------------------
  /** Execute the algorithm.
   */
  void MuonGroupDetectors::exec()
  {
    TableWorkspace_sptr table = getProperty("DetectorGroupingTable");

    // Check that table does have expected format

    if ( table->columnCount() != 3 )
      throw std::invalid_argument("Detector Grouping Table should have 3 columns");

    if ( table->getColumn(0)->type() != "str" )
      throw std::invalid_argument("Invalid type of the first column. Should be string.");

    if ( table->getColumn(1)->type() != "str" )
      throw std::invalid_argument("Invalid type of the second column. Should be string.");

    if ( table->getColumn(2)->type() != "vector_int" )
      throw std::invalid_argument("Invalid type of the third column. Should be vector of ints.");

    size_t numGroups(0);

    // First pass to determine how many non-empty groups we have
    for ( size_t row = 0; row < table->rowCount(); ++row )
    {
      std::string& itemName = table->cell<std::string>(row, 0);
      std::vector<int>& elements = table->cell< std::vector<int> >(row, 2);

      if ( itemName  == "Group" && elements.size() != 0 )
      {
        numGroups++;
      }
    }

    if ( numGroups == 0 )
      throw std::invalid_argument("Detector Grouping Table doesn't contain any non-empty groups");

    MatrixWorkspace_sptr inWS = getProperty("InputWorkspace");

    // Create output workspace with all the same parameters as an input one except number of histograms
    MatrixWorkspace_sptr outWS = WorkspaceFactory::Instance().create(inWS, numGroups);

    setProperty("OutputWorkspace", outWS);
  }



} // namespace Algorithms
} // namespace Mantid
