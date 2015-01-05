#include "MantidDataHandling/GenerateGroupingPowder.h"
#include "MantidKernel/System.h"
#include "MantidAPI/FileProperty.h"
#include "MantidKernel/BoundedValidator.h"

#include "Poco/DOM/Document.h"
#include "Poco/DOM/Element.h"
#include "Poco/DOM/Text.h"
#include "Poco/DOM/AutoPtr.h"
#include "Poco/DOM/DOMWriter.h"

#ifdef _MSC_VER
// Disable a flood of warnings from Poco about inheriting from
// std::basic_istream
// See
// http://connect.microsoft.com/VisualStudio/feedback/details/733720/inheriting-from-std-fstream-produces-c4250-warning
#pragma warning(push)
#pragma warning(disable : 4250)
#endif

#include <Poco/XML/XMLWriter.h>

#ifdef _MSC_VER
#pragma warning(pop)
#endif

#include <fstream>

using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace Poco::XML;

namespace Mantid {
namespace DataHandling {

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(GenerateGroupingPowder)

//----------------------------------------------------------------------------------------------
/** Constructor
 */
GenerateGroupingPowder::GenerateGroupingPowder() {}

//----------------------------------------------------------------------------------------------
/** Destructor
 */
GenerateGroupingPowder::~GenerateGroupingPowder() {}

//----------------------------------------------------------------------------------------------
/// Algorithm's name for identification. @see Algorithm::name
const std::string GenerateGroupingPowder::name() const {
  return "GenerateGroupingPowder";
}

/// Algorithm's version for identification. @see Algorithm::version
int GenerateGroupingPowder::version() const { return 1; }

/// Algorithm's category for identification. @see Algorithm::category
const std::string GenerateGroupingPowder::category() const {
  return "DataHandling";
}

//----------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------
/** Initialize the algorithm's properties.
 */
void GenerateGroupingPowder::init() {
  declareProperty(
      new WorkspaceProperty<>("InputWorkspace", "", Direction::Input),
      "An input workspace.");
  auto positiveDouble = boost::make_shared<BoundedValidator<double>>();
  positiveDouble->setLower(0.0);
  declareProperty("AngleStep", -1.0, positiveDouble,
                  "The angle step for grouping");
  declareProperty(
      new FileProperty("GroupingFilename", "", FileProperty::Save, ".xml"),
      "A grouping file that will be created. The corresponding .par file will "
      "be created as well.");
}

//----------------------------------------------------------------------------------------------
/** Execute the algorithm.
 */
void GenerateGroupingPowder::exec() {
  MatrixWorkspace_const_sptr input_ws = getProperty("InputWorkspace");
  // check if workspace has an instrument. If not, throw an exception
  Mantid::Geometry::Instrument_const_sptr instrument =
      input_ws->getInstrument();
  std::vector<detid_t> dets = instrument->getDetectorIDs(true);
  if (dets.empty()) {
    throw std::invalid_argument("Workspace contains no detectors.");
  }

  double step = getProperty("AngleStep");
  size_t numSteps = static_cast<size_t>(180. / step + 1);

  std::vector<std::vector<detid_t>> groups(numSteps);
  std::vector<double> twoThetaAverage(numSteps, 0.), rAverage(numSteps, 0.);

  std::vector<detid_t>::iterator it;
  for (it = dets.begin(); it != dets.end(); ++it) {
    double tt = instrument->getDetector(*it)->getTwoTheta(
                    instrument->getSample()->getPos(), Kernel::V3D(0, 0, 1)) *
                Geometry::rad2deg;
    double r =
        instrument->getDetector(*it)->getDistance(*(instrument->getSample()));
    size_t where = static_cast<size_t>(tt / step);
    groups.at(where).push_back(*it);
    twoThetaAverage.at(where) += tt;
    rAverage.at(where) += r;
  }

  std::string XMLfilename = getProperty("GroupingFilename");
  std::string PARfilename = XMLfilename;
  PARfilename.replace(PARfilename.end() - 3, PARfilename.end(), "par");

  // XML
  AutoPtr<Document> pDoc = new Document;
  AutoPtr<Element> pRoot = pDoc->createElement("detector-grouping");
  pDoc->appendChild(pRoot);
  pRoot->setAttribute("instrument", instrument->getName());

  size_t goodGroups(0);
  for (size_t i = 0; i < numSteps; ++i) {
    size_t gSize = groups.at(i).size();
    if (gSize > 0) {
      ++goodGroups;
      std::stringstream spID, textvalue;
      spID << i;
      AutoPtr<Element> pChildGroup = pDoc->createElement("group");
      pChildGroup->setAttribute("ID", spID.str());
      pRoot->appendChild(pChildGroup);

      std::copy(groups.at(i).begin(), groups.at(i).end(),
                std::ostream_iterator<size_t>(textvalue, ","));
      std::string text = textvalue.str();
      size_t found = text.rfind(",");
      if (found != std::string::npos) {
        text.erase(found, 1); // erase the last comma
      }

      AutoPtr<Element> pDetid = pDoc->createElement("detids");
      AutoPtr<Text> pText1 = pDoc->createTextNode(text);
      pDetid->appendChild(pText1);
      pChildGroup->appendChild(pDetid);
    }
  }
  if (goodGroups == 0) {
    g_log.error("Something terrible has happened: I cannot find any detectors "
                "with scattering angle between 0 and 180 degrees");
    throw Exception::InstrumentDefinitionError(
        "Detector scattering angles not between 0 and 180 degrees");
  }

  DOMWriter writer;
  writer.setNewLine("\n");
  writer.setOptions(XMLWriter::PRETTY_PRINT);

  std::ofstream ofs(XMLfilename.c_str());
  if (!ofs) {
    g_log.error("Unable to create file: " + XMLfilename);
    throw Exception::FileError("Unable to create file: ", XMLfilename);
  }
  ofs << "<?xml version=\"1.0\"?>\n";

  writer.writeNode(ofs, pDoc);
  ofs.close();

  // PAR file
  std::ofstream outPAR_file(PARfilename.c_str());
  if (!outPAR_file) {
    g_log.error("Unable to create file: " + PARfilename);
    throw Exception::FileError("Unable to create file: ", PARfilename);
  }
  // Write the number of detectors to the file.
  outPAR_file << " " << goodGroups << std::endl;

  for (size_t i = 0; i < numSteps; ++i) {
    size_t gSize = groups.at(i).size();
    if (gSize > 0) {
      outPAR_file << std::fixed << std::setprecision(3);
      outPAR_file.width(10);
      outPAR_file << rAverage.at(i) / static_cast<double>(gSize);
      outPAR_file.width(10);
      outPAR_file << twoThetaAverage.at(i) / static_cast<double>(gSize);
      outPAR_file.width(10);
      outPAR_file << 0.;
      outPAR_file.width(10);
      outPAR_file << step *Geometry::deg2rad *rAverage.at(i) /
                         static_cast<double>(gSize);
      outPAR_file.width(10);
      outPAR_file << 0.01;
      outPAR_file.width(10);
      outPAR_file << (groups.at(i)).at(0) << std::endl;
    }
  }

  // Close the file
  outPAR_file.close();
}

} // namespace Mantid
} // namespace DataHandling
