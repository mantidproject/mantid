//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include <fstream>

#include "MantidAlgorithms/ReadGroupsFromFile.h"
#include "MantidAPI/FileProperty.h"
#include "MantidAPI/InstrumentDataService.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/WorkspaceProperty.h"
#include "MantidAPI/WorkspaceValidators.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidKernel/ConfigService.h"
#include "MantidKernel/ListValidator.h"
#include "MantidKernel/System.h"

// Poco XML Headers for Grouping File
#include <Poco/DOM/Document.h>
#include <Poco/DOM/DOMParser.h>
#include <Poco/DOM/Element.h>
#include <Poco/DOM/NodeList.h>

using namespace Mantid::API;
using namespace Mantid::Kernel;
using namespace Mantid::Geometry;
using namespace Mantid::DataObjects;

namespace Mantid {
namespace Algorithms {

// Register the class into the algorithm factory
DECLARE_ALGORITHM(ReadGroupsFromFile)

using namespace Kernel;
using API::WorkspaceProperty;
using API::MatrixWorkspace_sptr;
using API::MatrixWorkspace;
using API::FileProperty;

ReadGroupsFromFile::ReadGroupsFromFile() : API::Algorithm(), calibration() {}

//-----------------------------------------------------------------------------------------------
/** Initialisation method. Declares properties to be used in algorithm.
 *
 */
void ReadGroupsFromFile::init() {

  // The name of the instrument
  declareProperty(new WorkspaceProperty<MatrixWorkspace>(
                      "InstrumentWorkspace", "", Direction::Input,
                      boost::make_shared<InstrumentValidator>()),
                  "A workspace that refers to the instrument of interest. You "
                  "can use [[LoadEmptyInstrument]] to create such a "
                  "workspace.");

  // The calibration file that contains the grouping information
  std::vector<std::string> exts;
  exts.push_back(".cal");
  exts.push_back(".xml");
  declareProperty(
      new FileProperty("GroupingFileName", "", FileProperty::Load, exts),
      "Either as a XML grouping file (see [[GroupDetectors]]) or as a "
      "[[CalFile]] (.cal extension).");
  // Flag to consider unselected detectors in the cal file
  std::vector<std::string> select;
  select.push_back("True");
  select.push_back("False");
  declareProperty("ShowUnselected", "True",
                  boost::make_shared<StringListValidator>(select),
                  "Whether to show detectors that are not in any group");
  // The output workspace (2D) that will contain the group information
  declareProperty(new API::WorkspaceProperty<DataObjects::Workspace2D>(
                      "OutputWorkspace", "", Direction::Output),
                  "The name of the output workspace");
}

//-----------------------------------------------------------------------------------------------
/** Executes the algorithm
 *
 *  @throw Exception::FileError If the grouping file cannot be opened or read
 *successfully
 *  @throw runtime_error If unable to run one of the Child Algorithms
 *successfully
 */
void ReadGroupsFromFile::exec() {
  MatrixWorkspace_const_sptr ws = getProperty("InstrumentWorkspace");

  // Get the instrument.
  Instrument_const_sptr inst = ws->getInstrument();

  // Create a copy (without the data) of the workspace - it will contain the
  Workspace2D_sptr localWorkspace = boost::dynamic_pointer_cast<Workspace2D>(
      WorkspaceFactory::Instance().create(ws, ws->getNumberHistograms(), 2, 1));
  if (!localWorkspace)
    throw std::runtime_error(
        "Failed when creating a Workspace2D from the input!");

  const std::string groupfile = getProperty("GroupingFilename");

  if (!groupfile.empty()) {
    std::string filename(groupfile);
    std::transform(filename.begin(), filename.end(), filename.begin(), tolower);
    if (filename.find(".xml") != std::string::npos) {
      readXMLGroupingFile(groupfile);
    } else {
      readGroupingFile(groupfile);
    }
  }

  // Get the instrument.
  const int64_t nHist = localWorkspace->getNumberHistograms();

  // Determine whether the user wants to see unselected detectors or not
  const std::string su = getProperty("ShowUnselected");
  bool showunselected = (!su.compare("True"));
  bool success = false;

  for (int64_t i = 0; i < nHist; i++) {
    ISpectrum *spec = localWorkspace->getSpectrum(i);
    const std::set<detid_t> &dets = spec->getDetectorIDs();
    if (dets.empty()) // Nothing
    {
      spec->dataY()[0] = 0.0;
      continue;
    }
    // Find the first detector ID in the list
    calmap::const_iterator it = calibration.find(*dets.begin());
    if (it == calibration.end()) // Could not find the detector
    {
      spec->dataY()[0] = 0.0;
      continue;
    }
    if (showunselected) {
      if (((*it).second).second == 0)
        spec->dataY()[0] = 0.0;
      else
        spec->dataY()[0] = static_cast<double>(((*it).second).first);
    } else
      spec->dataY()[0] = static_cast<double>(((*it).second).first);
    if (!success)
      success = true; // At least one detector is found in the cal file
  }
  progress(1);

  calibration.clear();
  if (!success) // Do some cleanup
  {
    localWorkspace.reset();
    throw std::runtime_error("Fail to found a detector in " + groupfile +
                             " existing in instrument " + inst->getName());
  }
  setProperty("OutputWorkspace", localWorkspace);
  return;
}

//-----------------------------------------------------------------------------------------------
/** Load a grouping file into the algorithm.
 *
 * @param filename :: grouping (.cal) file.
 */
void ReadGroupsFromFile::readGroupingFile(const std::string &filename) {
  std::ifstream grFile(filename.c_str());
  if (!grFile.is_open()) {
    g_log.error() << "Unable to open grouping file " << filename << std::endl;
    throw Exception::FileError("Error reading .cal file", filename);
  }
  calibration.clear();
  std::string str;
  while (getline(grFile, str)) {
    // Comment, not read
    if (str.empty() || str[0] == '#')
      continue;
    std::istringstream istr(str);
    int n, udet, sel, group;
    double offset;
    istr >> n >> udet >> offset >> sel >> group;
    calibration[udet] = std::make_pair(group, sel);
  }
  grFile.close();
  progress(0.7);
  return;
}

//-----------------------------------------------------------------------------------------------
/**
 * Reads detctor ids for groups from an XML grouping file, such as one created
 * by the SpatialGrouping algorithm.
 * @param filename :: path and name of input file
 * @throw FileError is there is a problem with the XML file
 */
void ReadGroupsFromFile::readXMLGroupingFile(const std::string &filename) {
  Poco::XML::DOMParser xmlParser;
  Poco::AutoPtr<Poco::XML::Document> file;
  try {
    file = xmlParser.parse(filename);
  } catch (...) {
    throw Kernel::Exception::FileError("Unable to parse file: ", filename);
  }

  Poco::XML::Element *root = file->documentElement();

  if (!root->hasChildNodes()) {
    throw Kernel::Exception::FileError("No root element in XML grouping file: ",
                                       filename);
  }

  Poco::AutoPtr<Poco::XML::NodeList> groups =
      root->getElementsByTagName("group");

  if (groups->length() == 0) {
    throw Kernel::Exception::FileError(
        "XML group file contains no group elements:", filename);
  }

  unsigned int nGroups = static_cast<unsigned int>(groups->length());
  for (unsigned int i = 0; i < nGroups; i++) {
    // Get the "detids" element from the grouping file
    Poco::XML::Element *elem =
        static_cast<Poco::XML::Element *>(groups->item(i));
    Poco::XML::Element *group = elem->getChildElement("detids");

    if (!group) {
      throw Mantid::Kernel::Exception::FileError(
          "XML Group File, group contains no <detids> element:", filename);
    }

    std::string ids = group->getAttribute("val");

    Poco::StringTokenizer data(ids, ",", Poco::StringTokenizer::TOK_TRIM);

    if (data.begin() != data.end()) {
      for (Poco::StringTokenizer::Iterator it = data.begin(); it != data.end();
           ++it) {
        // cast the string to an int
        int detID;
        try {
          detID = boost::lexical_cast<int>(*it);
        } catch (boost::bad_lexical_cast &) {
          throw Mantid::Kernel::Exception::FileError(
              "Could cast string to integer in input XML file", filename);
        }

        if (calibration.find(detID) == calibration.end()) {
          // add detector to a group
          calibration[detID] = std::pair<int, int>(i + 1, 1);
        }
      }
    }
  }

  progress(0.7);
}

} // namespace Algorithm
} // namespace Mantid
