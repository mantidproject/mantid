#include "MantidAlgorithms/ExtractMaskToTable.h"
#include "MantidAPI/WorkspaceProperty.h"
#include "MantidAPI/TableRow.h"
#include "MantidGeometry/Instrument.h"


namespace Mantid
{
namespace Algorithms
{

  using namespace Mantid::API;
  using namespace Mantid::DataObjects;
  using namespace Mantid::Geometry;
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

    double xmin = getProperty("XMin");
    double xmax = getProperty("XMax");
    if (xmin == EMPTY_DBL() || xmax == EMPTY_DBL() || xmin >= xmax)
      throw runtime_error("XMin or XMax cannot be empty.  XMin must be less than XMax.");

    // Create and set up output workspace
    TableWorkspace_sptr outws(new  TableWorkspace());
    outws->addColumn("double", "XMin");
    outws->addColumn("double", "XMax");
    outws->addColumn("str", "SpectraList");

    // Optionally import the input table workspace
    if (m_inputTableWS)
    {
    }

    // Extract mask
    vector<detid_t> maskeddetids;
    extractMaskFromMatrixWorkspace(maskeddetids);

    // Write out
    if (m_inputTableWS)
    {
      copyTableWorkspaceContent(m_inputTableWS, outws);
    }
    addToTableWorkspace(outws, maskeddetids, xmin, xmax);

    setProperty("OutputWorkspace", outws);

    return;
  }

  //----------------------------------------------------------------------------------------------
  /** Extract mask information from a workspace containing instrument
    * @param maskeddetids :: vector of detector IDs of detectors that are masked
    */
  void ExtractMaskToTable::extractMaskFromMatrixWorkspace(std::vector<detid_t>& maskeddetids)
  {
    // Clear input
    maskeddetids.clear();

    // Get on hold of instrument
    Instrument_const_sptr instrument = m_dataWS->getInstrument();
    if (!instrument)
      throw runtime_error("There is no instrument in input workspace.");

    // Extract
    size_t numdets = instrument->getNumberDetectors();
    vector<detid_t> detids = instrument->getDetectorIDs();

    for (size_t i = 0; i < numdets; ++i)
    {
      detid_t tmpdetid = detids[i];
      IDetector_const_sptr tmpdetector = instrument->getDetector(tmpdetid);
      bool masked = tmpdetector->isMasked();
      if (masked)
      {
        maskeddetids.push_back(tmpdetid);
      }
    }

    g_log.notice() << "Extract mask:  There are " << maskeddetids.size() << " detectors that"
                      " are masked." << ".\n";

    return;
  }

  //----------------------------------------------------------------------------------------------
  /** Copy table workspace content from one workspace to another
    * @param sourceWS :: table workspace from which the content is copied;
    * @param targetWS :: table workspace to which the content is copied;
    */
  void ExtractMaskToTable::copyTableWorkspaceContent(TableWorkspace_sptr sourceWS, TableWorkspace_sptr targetWS)
  {
    // Compare the column names.  They must be exactly the same
    vector<string> sourcecolnames = sourceWS->getColumnNames();
    vector<string> targetcolnames = targetWS->getColumnNames();
    if (sourcecolnames.size() != targetcolnames.size())
    {
      stringstream errmsg;
      errmsg << "Soruce table workspace " << sourceWS->name() << " has different number of columns ("
             << sourcecolnames.size() << ") than target table workspace's (" << targetcolnames.size()
             << ")";
      throw runtime_error(errmsg.str());
    }
    for (size_t i = 0; i < sourcecolnames.size(); ++i)
    {
      if (sourcecolnames[i].compare(targetcolnames[i]))
      {
        stringstream errss;
        errss << "Source and target have incompatible column name at column " << i << ". "
              << "Column name of source is " << sourcecolnames[i] << "; "
              << "Column name of target is " << targetcolnames[i];
        throw runtime_error(errss.str());
      }
    }

    // Copy over the content
    size_t numrows = sourceWS->rowCount();
    for (size_t i = 0; i < numrows; ++i)
    {
      double xmin, xmax;
      string speclist;
      TableRow tmprow = sourceWS->getRow(i);
      tmprow >> xmin >> xmax >> speclist;

      TableRow newrow = targetWS->appendRow();
      newrow << xmin << xmax << speclist;
    }

    return;
  }

  //----------------------------------------------------------------------------------------------
  /** Add a list of spectra (detector IDs) to the output table workspace
    */
  void ExtractMaskToTable::addToTableWorkspace(TableWorkspace_sptr outws, vector<detid_t> maskeddetids,
                                               double xmin, double xmax)
  {

  }



} // namespace Algorithms
} // namespace Mantid





































