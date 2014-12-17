#include "MantidAlgorithms/ExtractMaskToTable.h"
#include "MantidAPI/WorkspaceProperty.h"
#include "MantidAPI/TableRow.h"
#include "MantidDataObjects/MaskWorkspace.h"
#include "MantidGeometry/Instrument.h"
#include "MantidKernel/ArrayProperty.h"

namespace Mantid {
namespace Algorithms {

using namespace Mantid::API;
using namespace Mantid::DataObjects;
using namespace Mantid::Geometry;
using namespace Mantid::Kernel;

using namespace std;

DECLARE_ALGORITHM(ExtractMaskToTable)

//----------------------------------------------------------------------------------------------
/** Constructor
 */
ExtractMaskToTable::ExtractMaskToTable() {}

//----------------------------------------------------------------------------------------------
/** Destructor
 */
ExtractMaskToTable::~ExtractMaskToTable() {}

//----------------------------------------------------------------------------------------------
/** Declare properties
  */
void ExtractMaskToTable::init() {
  auto inwsprop = new WorkspaceProperty<MatrixWorkspace>("InputWorkspace", "",
                                                         Direction::Input);
  declareProperty(
      inwsprop,
      "A workspace whose masking is to be extracted or a MaskWorkspace. ");

  auto intblprop = new WorkspaceProperty<TableWorkspace>(
      "MaskTableWorkspace", "", Direction::Input, PropertyMode::Optional);
  declareProperty(intblprop, "A mask table workspace containing 3 columns: "
                             "XMin, XMax and DetectorIDsList. ");

  auto outwsprop = new WorkspaceProperty<TableWorkspace>("OutputWorkspace", "",
                                                         Direction::Output);
  declareProperty(outwsprop, "A comma separated list or array containing a "
                             "list of masked detector ID's ");

  declareProperty("Xmin", EMPTY_DBL(), "Minimum of X-value.");

  declareProperty("Xmax", EMPTY_DBL(), "Maximum of X-value.");
}

//----------------------------------------------------------------------------------------------
/** Main execution body
  */
void ExtractMaskToTable::exec() {
  // Get input properties
  m_dataWS = getProperty("InputWorkspace");
  if (!m_dataWS)
    throw runtime_error("InputWorkspace cannot be cast to a MatrixWorkspace.");
  MaskWorkspace_const_sptr maskws =
      boost::dynamic_pointer_cast<const MaskWorkspace>(m_dataWS);
  if (maskws) {
    g_log.notice() << "InputWorkspace " << m_dataWS->name()
                   << " is a MaskWorkspace.\n";
    m_inputIsMask = true;
  } else {
    m_inputIsMask = false;
  }

  m_inputTableWS = getProperty("MaskTableWorkspace");

  double xmin = getProperty("XMin");
  double xmax = getProperty("XMax");
  if (xmin == EMPTY_DBL() || xmax == EMPTY_DBL() || xmin >= xmax)
    throw runtime_error(
        "XMin or XMax cannot be empty.  XMin must be less than XMax.");

  // Create and set up output workspace
  TableWorkspace_sptr outws(new TableWorkspace());
  outws->addColumn("double", "XMin");
  outws->addColumn("double", "XMax");
  outws->addColumn("str", "DetectorIDsList");
  setProperty("OutputWorkspace", outws);

  // Optionally import the input table workspace
  vector<detid_t> prevmaskeddetids;
  if (m_inputTableWS) {
    g_log.notice("Parse input masking table workspace.");
    parseMaskTable(m_inputTableWS, prevmaskeddetids);
  } else {
    g_log.notice("No input workspace to parse.");
  }

  // Extract mask
  vector<detid_t> maskeddetids;
  if (m_inputIsMask)
    extractMaskFromMaskWorkspace(maskeddetids);
  else
    extractMaskFromMatrixWorkspace(maskeddetids);
  g_log.debug() << "[DB] Number of masked detectors = " << maskeddetids.size()
                << ".\n";

  // Write out
  if (m_inputTableWS) {
    g_log.notice()
        << "About to copying input table workspace content to output workspace."
        << ".\n";
    copyTableWorkspaceContent(m_inputTableWS, outws);
  } else {
    g_log.notice() << "There is no input workspace information to copy to "
                      "output workspace."
                   << ".\n";
  }

  addToTableWorkspace(outws, maskeddetids, xmin, xmax, prevmaskeddetids);

  return;
}

//----------------------------------------------------------------------------------------------
/** Parse input TableWorkspace to get a list of detectors IDs of which detector
 * are already masked
  * @param masktablews :: TableWorkspace containing masking information
  * @param maskeddetectorids :: (output) vector of detector IDs that are masked
  */
void
ExtractMaskToTable::parseMaskTable(DataObjects::TableWorkspace_sptr masktablews,
                                   std::vector<detid_t> &maskeddetectorids) {
  // Clear input
  maskeddetectorids.clear();

  // Check format of mask table workspace
  if (masktablews->columnCount() != 3) {
    g_log.error("Mask table workspace must have more than 3 columns.  First 3 "
                "must be Xmin, Xmax and Spectrum List.");
    return;
  } else {
    vector<string> colnames = masktablews->getColumnNames();
    vector<string> chkcolumans(3);
    chkcolumans[0] = "XMin";
    chkcolumans[1] = "XMax";
    chkcolumans[2] = "DetectorIDsList";
    for (int i = 0; i < 3; ++i) {
      if (colnames[i] != chkcolumans[i]) {
        g_log.error() << "Mask table workspace " << masktablews->name() << "'s "
                      << i << "-th column name is " << colnames[i]
                      << ", while it should be " << chkcolumans[i]
                      << ". MaskWorkspace is invalid"
                      << " and thus not used.\n";
        return;
      }
    }
  }

  // Parse each row
  size_t numrows = masktablews->rowCount();
  double xmin, xmax;
  string specliststr;
  for (size_t i = 0; i < numrows; ++i) {
    TableRow tmprow = masktablews->getRow(i);
    tmprow >> xmin >> xmax >> specliststr;

    vector<detid_t> tmpdetidvec;
    parseStringToVector(specliststr, tmpdetidvec);
    maskeddetectorids.insert(maskeddetectorids.end(), tmpdetidvec.begin(),
                             tmpdetidvec.end());
  }

  return;
}

//----------------------------------------------------------------------------------------------
/** Parse a string containing list in format (x, xx-yy, x, x, ...) to a vector
 * of detid_t
  * @param liststr :: string containing list to parse
  * @param detidvec :: vector genrated from input string containing the list
  */
void ExtractMaskToTable::parseStringToVector(std::string liststr,
                                             vector<detid_t> &detidvec) {
  detidvec.clear();

  // Use ArrayProperty to parse the list
  ArrayProperty<int> detlist("i", liststr);
  if (detlist.isValid().compare("")) {
    stringstream errss;
    errss << "String '" << liststr
          << "' is unable to be converted to a list of detectors IDs. "
          << "Validation mesage: " << detlist.isValid();
    g_log.error(errss.str());
    throw runtime_error(errss.str());
  }

  // Convert from ArrayProperty to detectors list
  size_t numdetids = detlist.size();
  detidvec.reserve(numdetids);
  for (size_t i = 0; i < numdetids; ++i) {
    int tmpid = detlist.operator()()[i];
    detidvec.push_back(tmpid);
    g_log.debug() << "[DB] Add detector ID: " << tmpid << ".\n";
  }

  return;
}

//----------------------------------------------------------------------------------------------
/** Extract mask information from a workspace containing instrument
  * @param maskeddetids :: vector of detector IDs of detectors that are masked
  */
void ExtractMaskToTable::extractMaskFromMatrixWorkspace(
    std::vector<detid_t> &maskeddetids) {
  // Clear input
  maskeddetids.clear();

  // Get on hold of instrument
  Instrument_const_sptr instrument = m_dataWS->getInstrument();
  if (!instrument)
    throw runtime_error("There is no instrument in input workspace.");

  // Extract
  size_t numdets = instrument->getNumberDetectors();
  vector<detid_t> detids = instrument->getDetectorIDs();

  for (size_t i = 0; i < numdets; ++i) {
    detid_t tmpdetid = detids[i];
    IDetector_const_sptr tmpdetector = instrument->getDetector(tmpdetid);
    bool masked = tmpdetector->isMasked();
    if (masked) {
      maskeddetids.push_back(tmpdetid);
    }
    g_log.debug() << "[DB] Detector No. " << i << ":  ID = " << detids[i]
                  << ", Masked = " << masked << ".\n";
  }

  g_log.notice() << "Extract mask:  There are " << maskeddetids.size()
                 << " detectors that"
                    " are masked."
                 << ".\n";

  return;
}

//----------------------------------------------------------------------------------------------
/** Extract masked detectors from a MaskWorkspace
  * @param maskeddetids :: vector of detector IDs of the detectors that are
 * masked
  */
void ExtractMaskToTable::extractMaskFromMaskWorkspace(
    std::vector<detid_t> &maskeddetids) {
  // Clear input
  maskeddetids.clear();

  // Go through all spectra to find masked workspace
  MaskWorkspace_const_sptr maskws =
      boost::dynamic_pointer_cast<const MaskWorkspace>(m_dataWS);
  size_t numhist = maskws->getNumberHistograms();
  for (size_t i = 0; i < numhist; ++i) {
    // Rule out the spectrum without mask
    if (maskws->readY(i)[0] < 1.0E-9)
      continue;

    // Get spectrum
    const API::ISpectrum *spec = maskws->getSpectrum(i);
    if (!spec)
      throw runtime_error(
          "Unable to get spectrum reference from mask workspace.");

    const set<detid_t> detidset = spec->getDetectorIDs();
    for (set<detid_t>::const_iterator sit = detidset.begin();
         sit != detidset.end(); ++sit) {
      detid_t tmpdetid = *sit;
      maskeddetids.push_back(tmpdetid);
    }
  }

  return;
}

//----------------------------------------------------------------------------------------------
/** Copy table workspace content from one workspace to another
  * @param sourceWS :: table workspace from which the content is copied;
  * @param targetWS :: table workspace to which the content is copied;
  */
void
ExtractMaskToTable::copyTableWorkspaceContent(TableWorkspace_sptr sourceWS,
                                              TableWorkspace_sptr targetWS) {
  // Compare the column names.  They must be exactly the same
  vector<string> sourcecolnames = sourceWS->getColumnNames();
  vector<string> targetcolnames = targetWS->getColumnNames();
  if (sourcecolnames.size() != targetcolnames.size()) {
    stringstream errmsg;
    errmsg << "Soruce table workspace " << sourceWS->name()
           << " has different number of columns (" << sourcecolnames.size()
           << ") than target table workspace's (" << targetcolnames.size()
           << ")";
    throw runtime_error(errmsg.str());
  }
  for (size_t i = 0; i < sourcecolnames.size(); ++i) {
    if (sourcecolnames[i].compare(targetcolnames[i])) {
      stringstream errss;
      errss << "Source and target have incompatible column name at column " << i
            << ". "
            << "Column name of source is " << sourcecolnames[i] << "; "
            << "Column name of target is " << targetcolnames[i];
      throw runtime_error(errss.str());
    }
  }

  // Copy over the content
  size_t numrows = sourceWS->rowCount();
  for (size_t i = 0; i < numrows; ++i) {
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
/** Add a list of spectra (detector IDs) to the output table workspace.
  * If a detector is masked in input MaskTableWorkspace, then it will not be
 * added to a new row
  * @param outws :: table workspace to write
  * @param maskeddetids :: vector of detector IDs of which detectors masked
  * @param xmin :: minumim x
  * @param xmax :: maximum x
  * @param prevmaskedids :: vector of previous masked detector IDs
  */
void ExtractMaskToTable::addToTableWorkspace(TableWorkspace_sptr outws,
                                             vector<detid_t> maskeddetids,
                                             double xmin, double xmax,
                                             vector<detid_t> prevmaskedids) {
  // Sort vector of detectors ID
  size_t numdetids = maskeddetids.size();
  if (numdetids == 0) {
    stringstream warnss;
    warnss << "Attempting to add an empty vector of masked detectors IDs to "
              "output workspace.  Operation failed.";
    g_log.warning(warnss.str());
    return;
  } else {
    sort(maskeddetids.begin(), maskeddetids.end());
  }

  // Exclude previously masked detectors IDs from masked detectors IDs
  if (prevmaskedids.size() > 0) {
    sort(prevmaskedids.begin(), prevmaskedids.end());
    maskeddetids = subtractVector(maskeddetids, prevmaskedids);
    numdetids = maskeddetids.size();
  } else {
    g_log.debug() << "[DB] There is no previously masked detectors."
                  << ".\n";
  }

  if (numdetids == 0) {
    // I don't know what should be done here
    throw std::runtime_error("Empty detector ID list");
  }

  // Convert vector to string
  stringstream spectralist;
  detid_t previd = maskeddetids[0];
  detid_t headid = maskeddetids[0];
  for (size_t i = 1; i < numdetids; ++i) {
    detid_t tmpid = maskeddetids[i];
    if (tmpid == previd + 1) {
      // Continuous ID
      previd = tmpid;
    } else if (tmpid > previd + 1) {
      // Skipped ID: make a pair
      if (previd == headid) {
        // Single item
        spectralist << " " << headid << ", ";
      } else {
        // Multiple items
        spectralist << " " << headid << "-" << previd << ", ";
      }

      // New head
      headid = tmpid;
      previd = tmpid;
    } else {
      g_log.error() << "Current ID = " << tmpid << ", Previous ID = " << previd
                    << ", Head ID = " << headid << ".\n";
      throw runtime_error("Impossible!  Programming logic error!");
    }
  } // ENDFOR (i)

  // Last one
  if (previd == headid)
    spectralist << " " << headid;
  else
    spectralist << " " << headid << "-" << previd;

  // Add to table workspace
  string specliststr = spectralist.str();
  TableRow newrow = outws->appendRow();
  newrow << xmin << xmax << specliststr;

  return;
}

//----------------------------------------------------------------------------------------------
/** Remove the detector IDs of one vector that appear in another vector
  * @param minuend :: vector with items to be removed from
  * @param subtrahend :: vector containing the items to be removed from minuend
  */
std::vector<detid_t>
ExtractMaskToTable::subtractVector(std::vector<detid_t> minuend,
                                   std::vector<detid_t> subtrahend) {
  // Define some variables
  vector<detid_t>::iterator firstsubiter, fiter;
  firstsubiter = subtrahend.begin();

  // Returned
  vector<detid_t> diff;
  size_t numminend = minuend.size();

  for (size_t i = 0; i < numminend; ++i) {
    detid_t tmpid = minuend[i];
    fiter = lower_bound(firstsubiter, subtrahend.end(), tmpid);
    bool exist(false);
    if (fiter != subtrahend.end())
      exist = *fiter == tmpid;
    if (!exist) {
      diff.push_back(tmpid);
    }
    firstsubiter = fiter;
  }

  return diff;
}

} // namespace Algorithms
} // namespace Mantid
