#include "MantidAlgorithms/ExtractMaskToTable.h"
#include "MantidAPI/WorkspaceProperty.h"
#include "MantidAPI/TableRow.h"


namespace Mantid
{
namespace Algorithms
{

  using namespace Mantid::API;
  using namespace Mantid::DataObjects;
  using namespace Mantid::Kernel;

  using namespace std;

  DECLARE_ALGORITHM(ExtractMaskToTable)

  //----------------------------------------------------------------------------------------------
  /** Constructor
   */
  ExtractMaskToTable::ExtractMaskToTable()
  {
  }
    
  //----------------------------------------------------------------------------------------------
  /** Destructor
   */
  ExtractMaskToTable::~ExtractMaskToTable()
  {
  }

  //----------------------------------------------------------------------------------------------
  /** Documentation
    */
  void ExtractMaskToTable::initDocs()
  {
    setWikiSummary("Extract mask from a workspace to a TableWorkspace.");
    setOptionalMessage("The output TableWorkspace should be compatible to MaskBinsFromTable.");
  }

  //----------------------------------------------------------------------------------------------
  /** Declare properties
    */
  void ExtractMaskToTable::init()
  {
    auto inwsprop = new WorkspaceProperty<MatrixWorkspace>("InputWorkspace", "Anonymous", Direction::Input);
    declareProperty(inwsprop, "Name of a MatrixWorkspace from which the mask will be extracted.");

    auto intblprop = new WorkspaceProperty<TableWorkspace>("MaskTableWorkspace", "", Direction::Input, PropertyMode::Optional);
    declareProperty(intblprop, "Name of the TableWorkspace to append to.");

    auto outwsprop = new WorkspaceProperty<TableWorkspace>("OutputWorkspace", "_Hidden", Direction::Output);
    declareProperty(outwsprop, "Name of the output TableWorkspace containing the mask information.");

    declareProperty("Xmin", EMPTY_DBL(), "Minimum of X-value.");

    declareProperty("Xmax", EMPTY_DBL(), "Maximum of X-value.");

    declareProperty("ClearExistingTable", true, "If true, the existing table workspace will be cleared. ");
  }

  //----------------------------------------------------------------------------------------------
  /** Main execution body
    */
  void ExtractMaskToTable::exec()
  {
    // Get input properties
    m_dataWS = getProperty("InputWorkspace");
    if (!m_dataWS)
      throw runtime_error("InputWorkspace cannot be cast to a MatrixWorkspace.");

    m_inputTableWS = getProperty("MaskTableWorkspace");


    // Set up output workspace
    // TableWorkspace_sptr outws = m_inputTableWS;
    string outwsname = getPropertyValue("OutputWorkspace");
    if (outwsname.compare("_Hidden") == 0)
    {
      auto newprop = new WorkspaceProperty<TableWorkspace>("OutputWorkspace", m_inputTableWS->name(), Direction::Output);
      declareProperty(newprop, "");
    }

    TableWorkspace_sptr outws(new  TableWorkspace());
    outws->addColumn("double", "XMin");
    outws->addColumn("double", "XMax");
    outws->addColumn("str", "Spectra");

    TableRow newrow = outws->appendRow();
    newrow << 1.23 << 678.9 << "3-5";


    setProperty("OutputWorkspace", outws);

    return;
  }
  

} // namespace Algorithms
} // namespace Mantid





































