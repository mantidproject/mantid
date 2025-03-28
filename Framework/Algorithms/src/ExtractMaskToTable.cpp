// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidAlgorithms/ExtractMaskToTable.h"
#include "MantidAPI/TableRow.h"
#include "MantidAPI/WorkspaceProperty.h"
#include "MantidDataObjects/MaskWorkspace.h"
#include "MantidGeometry/Instrument.h"
#include "MantidGeometry/Instrument/DetectorInfo.h"
#include "MantidKernel/ArrayProperty.h"

namespace Mantid::Algorithms {

using namespace Mantid::API;
using namespace Mantid::DataObjects;
using namespace Mantid::Geometry;
using namespace Mantid::Kernel;

DECLARE_ALGORITHM(ExtractMaskToTable)

//----------------------------------------------------------------------------------------------
/** Declare properties
 */
void ExtractMaskToTable::init() {
  declareProperty(std::make_unique<WorkspaceProperty<MatrixWorkspace>>("InputWorkspace", "", Direction::Input),
                  "A workspace whose masking is to be extracted or a MaskWorkspace. ");

  declareProperty(std::make_unique<WorkspaceProperty<TableWorkspace>>("MaskTableWorkspace", "", Direction::Input,
                                                                      PropertyMode::Optional),
                  "A mask table workspace containing 3 columns: "
                  "XMin, XMax and DetectorIDsList. ");

  declareProperty(std::make_unique<WorkspaceProperty<TableWorkspace>>("OutputWorkspace", "", Direction::Output),
                  "A comma separated list or array containing a "
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
    throw std::runtime_error("InputWorkspace cannot be cast to a MatrixWorkspace.");
  MaskWorkspace_const_sptr maskws = std::dynamic_pointer_cast<const MaskWorkspace>(m_dataWS);

  bool m_inputIsMask = false;
  if (maskws) {
    g_log.notice() << "InputWorkspace " << m_dataWS->getName() << " is a MaskWorkspace.\n";
    m_inputIsMask = true;
  } else {
    m_inputIsMask = false;
  }

  m_inputTableWS = getProperty("MaskTableWorkspace");

  double xmin = getProperty("XMin");
  double xmax = getProperty("XMax");
  if (xmin == EMPTY_DBL() || xmax == EMPTY_DBL() || xmin >= xmax)
    throw std::runtime_error("XMin or XMax cannot be empty.  XMin must be less than XMax.");

  // Create and set up output workspace
  auto outws = std::make_shared<TableWorkspace>();
  outws->addColumn("double", "XMin");
  outws->addColumn("double", "XMax");
  outws->addColumn("str", "DetectorIDsList");
  setProperty("OutputWorkspace", outws);

  // Optionally import the input table workspace
  std::vector<detid_t> prevmaskeddetids;
  if (m_inputTableWS) {
    g_log.notice("Parse input masking table workspace.");
    prevmaskeddetids = parseMaskTable(m_inputTableWS);
  } else {
    g_log.notice("No input workspace to parse.");
  }

  // Extract mask
  std::vector<detid_t> maskeddetids;
  if (m_inputIsMask)
    maskeddetids = extractMaskFromMaskWorkspace();
  else
    maskeddetids = extractMaskFromMatrixWorkspace();
  g_log.debug() << "[DB] Number of masked detectors = " << maskeddetids.size() << ".\n";

  // Write out
  if (m_inputTableWS) {
    g_log.notice() << "About to copying input table workspace content to output workspace."
                   << ".\n";
    copyTableWorkspaceContent(m_inputTableWS, outws);
  } else {
    g_log.notice() << "There is no input workspace information to copy to "
                      "output workspace."
                   << ".\n";
  }

  addToTableWorkspace(outws, maskeddetids, xmin, xmax, prevmaskeddetids);
}

//----------------------------------------------------------------------------------------------
/** Parse input TableWorkspace to get a list of detectors IDs of which detector
 * are already masked
 * @param masktablews :: TableWorkspace containing masking information
 * @returns :: vector of detector IDs that are masked
 */
std::vector<detid_t> ExtractMaskToTable::parseMaskTable(const DataObjects::TableWorkspace_sptr &masktablews) {
  // Output vector
  std::vector<detid_t> maskeddetectorids;

  // Check format of mask table workspace
  if (masktablews->columnCount() != 3) {
    g_log.error("Mask table workspace must have more than 3 columns.  First 3 "
                "must be Xmin, Xmax and Spectrum List.");
    return maskeddetectorids;
  } else {
    std::vector<std::string> colnames = masktablews->getColumnNames();
    std::vector<std::string> chkcolumans(3);
    chkcolumans[0] = "XMin";
    chkcolumans[1] = "XMax";
    chkcolumans[2] = "DetectorIDsList";
    for (int i = 0; i < 3; ++i) {
      if (colnames[i] != chkcolumans[i]) {
        g_log.error() << "Mask table workspace " << masktablews->getName() << "'s " << i << "-th column name is "
                      << colnames[i] << ", while it should be " << chkcolumans[i] << ". MaskWorkspace is invalid"
                      << " and thus not used.\n";
        return maskeddetectorids;
      }
    }
  }

  // Parse each row
  size_t numrows = masktablews->rowCount();
  double xmin, xmax;
  std::string specliststr;
  for (size_t i = 0; i < numrows; ++i) {
    TableRow tmprow = masktablews->getRow(i);
    tmprow >> xmin >> xmax >> specliststr;

    std::vector<detid_t> tmpdetidvec = parseStringToVector(specliststr);
    maskeddetectorids.insert(maskeddetectorids.end(), tmpdetidvec.begin(), tmpdetidvec.end());
  }

  return maskeddetectorids;
}

//----------------------------------------------------------------------------------------------
/** Parse a string containing list in format (x, xx-yy, x, x, ...) to a vector
 * of detid_t
 * @param liststr :: string containing list to parse
 * @returns :: vector genrated from input string containing the list
 */
std::vector<detid_t> ExtractMaskToTable::parseStringToVector(const std::string &liststr) {
  std::vector<detid_t> detidvec;

  // Use ArrayProperty to parse the list
  ArrayProperty<int> detlist("i", liststr);
  if (!detlist.isValid().empty()) {
    std::stringstream errss;
    errss << "String '" << liststr << "' is unable to be converted to a list of detectors IDs. "
          << "Validation mesage: " << detlist.isValid();
    g_log.error(errss.str());
    throw std::runtime_error(errss.str());
  }

  // Convert from ArrayProperty to detectors list
  size_t numdetids = detlist.size();
  detidvec.reserve(numdetids);
  for (size_t i = 0; i < numdetids; ++i) {
    int tmpid = detlist.operator()()[i];
    detidvec.emplace_back(tmpid);
    g_log.debug() << "[DB] Add detector ID: " << tmpid << ".\n";
  }

  return detidvec;
}

//----------------------------------------------------------------------------------------------
/** Extract mask information from a workspace containing instrument
 * @return vector of detector IDs of detectors that are masked
 */
std::vector<detid_t> ExtractMaskToTable::extractMaskFromMatrixWorkspace() {
  // Clear input
  std::vector<detid_t> maskeddetids;

  // Get on hold of instrument
  const auto &detectorInfo = m_dataWS->detectorInfo();
  if (detectorInfo.size() == 0)
    throw std::runtime_error("There is no instrument in input workspace.");

  // Extract
  const std::vector<detid_t> &detids = detectorInfo.detectorIDs();

  for (size_t i = 0; i < detectorInfo.size(); ++i) {
    bool masked = detectorInfo.isMasked(i);
    if (masked) {
      maskeddetids.emplace_back(detids[i]);
    }
    g_log.debug() << "[DB] Detector No. " << i << ":  ID = " << detids[i] << ", Masked = " << masked << ".\n";
  }

  g_log.notice() << "Extract mask:  There are " << maskeddetids.size()
                 << " detectors that"
                    " are masked."
                 << ".\n";

  return maskeddetids;
}

//----------------------------------------------------------------------------------------------
/** Extract masked detectors from a MaskWorkspace
 * @return vector of detector IDs of the detectors that are
 * masked
 */
std::vector<detid_t> ExtractMaskToTable::extractMaskFromMaskWorkspace() {
  // output vector
  std::vector<detid_t> maskeddetids;

  // Go through all spectra to find masked workspace
  MaskWorkspace_const_sptr maskws = std::dynamic_pointer_cast<const MaskWorkspace>(m_dataWS);
  size_t numhist = maskws->getNumberHistograms();
  for (size_t i = 0; i < numhist; ++i) {
    // Rule out the spectrum without mask
    if (maskws->readY(i)[0] < 1.0E-9)
      continue;

    const auto &detidset = maskws->getSpectrum(i).getDetectorIDs();
    std::copy(detidset.cbegin(), detidset.cend(), std::inserter(maskeddetids, maskeddetids.end()));
  }
  return maskeddetids;
}

//----------------------------------------------------------------------------------------------
/** Copy table workspace content from one workspace to another
 * @param sourceWS :: table workspace from which the content is copied;
 * @param targetWS :: table workspace to which the content is copied;
 */
void ExtractMaskToTable::copyTableWorkspaceContent(const TableWorkspace_sptr &sourceWS,
                                                   const TableWorkspace_sptr &targetWS) {
  // Compare the column names.  They must be exactly the same
  std::vector<std::string> sourcecolnames = sourceWS->getColumnNames();
  std::vector<std::string> targetcolnames = targetWS->getColumnNames();
  if (sourcecolnames.size() != targetcolnames.size()) {
    std::stringstream errmsg;
    errmsg << "Soruce table workspace " << sourceWS->getName() << " has different number of columns ("
           << sourcecolnames.size() << ") than target table workspace's (" << targetcolnames.size() << ")";
    throw std::runtime_error(errmsg.str());
  }
  for (size_t i = 0; i < sourcecolnames.size(); ++i) {
    if (sourcecolnames[i] != targetcolnames[i]) {
      std::stringstream errss;
      errss << "Source and target have incompatible column name at column " << i << ". "
            << "Column name of source is " << sourcecolnames[i] << "; "
            << "Column name of target is " << targetcolnames[i];
      throw std::runtime_error(errss.str());
    }
  }

  // Copy over the content
  size_t numrows = sourceWS->rowCount();
  for (size_t i = 0; i < numrows; ++i) {
    double xmin, xmax;
    std::string speclist;
    TableRow tmprow = sourceWS->getRow(i);
    tmprow >> xmin >> xmax >> speclist;

    TableRow newrow = targetWS->appendRow();
    newrow << xmin << xmax << speclist;
  }
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
void ExtractMaskToTable::addToTableWorkspace(const TableWorkspace_sptr &outws, std::vector<detid_t> maskeddetids,
                                             double xmin, double xmax, std::vector<detid_t> prevmaskedids) {
  // Sort vector of detectors ID
  size_t numdetids = maskeddetids.size();
  if (numdetids == 0) {
    std::stringstream warnss;
    warnss << "Attempting to add an empty vector of masked detectors IDs to "
              "output workspace.  Operation failed.";
    g_log.warning(warnss.str());
    return;
  } else {
    sort(maskeddetids.begin(), maskeddetids.end());
  }

  // Exclude previously masked detectors IDs from masked detectors IDs
  if (!prevmaskedids.empty()) {
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
  std::stringstream spectralist;
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
      g_log.error() << "Current ID = " << tmpid << ", Previous ID = " << previd << ", Head ID = " << headid << ".\n";
      throw std::runtime_error("Impossible!  Programming logic error!");
    }
  } // ENDFOR (i)

  // Last one
  if (previd == headid)
    spectralist << " " << headid;
  else
    spectralist << " " << headid << "-" << previd;

  // Add to table workspace
  std::string specliststr = spectralist.str();
  TableRow newrow = outws->appendRow();
  newrow << xmin << xmax << specliststr;
}

//----------------------------------------------------------------------------------------------
/** Remove the detector IDs of one vector that appear in another vector
 * @param minuend :: vector with items to be removed from
 * @param subtrahend :: vector containing the items to be removed from minuend
 */
std::vector<detid_t> ExtractMaskToTable::subtractVector(const std::vector<detid_t> &minuend,
                                                        std::vector<detid_t> &subtrahend) {
  // Define some variables
  std::vector<detid_t>::iterator firstsubiter, fiter;
  firstsubiter = subtrahend.begin();

  // Returned
  std::vector<detid_t> diff;
  size_t numminend = minuend.size();

  for (size_t i = 0; i < numminend; ++i) {
    detid_t tmpid = minuend[i];
    fiter = lower_bound(firstsubiter, subtrahend.end(), tmpid);
    bool exist(false);
    if (fiter != subtrahend.end())
      exist = *fiter == tmpid;
    if (!exist) {
      diff.emplace_back(tmpid);
    }
    firstsubiter = fiter;
  }

  return diff;
}

} // namespace Mantid::Algorithms
