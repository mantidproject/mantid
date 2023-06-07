// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidDataHandling/GenerateGroupingPowder.h"
#include "MantidAPI/FileProperty.h"
#include "MantidAPI/InstrumentValidator.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/SpectrumInfo.h"
#include "MantidDataHandling/SaveDetectorsGrouping.h"
#include "MantidDataHandling/SavePAR.h"
#include "MantidDataObjects/GroupingWorkspace.h"
#include "MantidGeometry/Crystal/AngleUnits.h"
#include "MantidGeometry/Instrument.h"
#include "MantidGeometry/Instrument/DetectorGroup.h"
#include "MantidGeometry/Instrument/DetectorInfo.h"
#include "MantidKernel/BoundedValidator.h"
#include "MantidKernel/ListValidator.h"
#include "MantidKernel/System.h"

#include <Poco/DOM/AutoPtr.h>
#include <Poco/DOM/DOMWriter.h>
#include <Poco/DOM/Document.h>
#include <Poco/DOM/Element.h>
#include <Poco/DOM/Text.h>
#include <Poco/XML/XMLWriter.h>

#include <fstream>

using namespace Mantid::API;
using namespace Mantid::DataObjects;
using namespace Mantid::Kernel;
using namespace Mantid::Geometry;
using namespace Poco::XML;

namespace {
class Labelor {
protected:
  const double tt_step;
  const double aa_step;
  const double aa_start;

public:
  Labelor(double tt_step, double aa_step = 0.0, double aa_start = 0.0)
      : tt_step(tt_step), aa_step(aa_step), aa_start(aa_start){};
  virtual int operator()(SpectrumInfo const &spectrumInfo, int i) { return -1; };
};

class OldLabelor : public Labelor {
  /* Divides sphere into bands of size tt_step,
  which are then numbered sequentially, in order, with increasing twoTheta
  */
  // reduce flops by saving as mult. constant
  double inv_tt_step = Mantid::Geometry::rad2deg / tt_step;

public:
  OldLabelor(double tt_step) : Labelor(tt_step){};
  int operator()(SpectrumInfo const &spectrumInfo, int i) override {
    return static_cast<int>(spectrumInfo.twoTheta(i) * inv_tt_step);
  };
};

class PolarLabelor : public Labelor {
  /* uses twoTheta to label groups in order, with no empty groups
   */
  // a vector of the end points (in 2theta) for each group
  std::vector<std::pair<double, double>> group_tt;

public:
  PolarLabelor(double tt_step) : Labelor(tt_step), group_tt({}){};

  int operator()(SpectrumInfo const &spectrumInfo, int i) override {
    // find the group so that 2theta is inside its end points
    double tt = spectrumInfo.twoTheta(i);
    auto index = std::find_if(group_tt.begin(), group_tt.end(),
                              [tt](std::pair<double, double> x) { return x.first <= tt && tt < x.second; });
    // if no such group, make a new one
    if (index == group_tt.end()) {
      // check if 2theta is very close to an existing group
      auto ix = std::find_if(group_tt.begin(), group_tt.end(),
                             [tt, this](std::pair<double, double> x) { return x.first < tt + this->tt_step; });
      // if not close to existing group, make group starting here
      if (ix == group_tt.end())
        group_tt.push_back({tt, tt + tt_step});
      // if too close to an existing group, make group further back so no overlaps
      else
        group_tt.push_back({tt - tt_step, tt});
      index = group_tt.end() - 1;
    }
    return std::distance(group_tt.begin(), index) + 1;
  };
};

class PolarAzimuthalLabelor : public Labelor {
  /* uses twoTheta to label groups in order, with no empty groups
   */
  // a vector of the end points (in 2theta) for each group
  std::vector<std::pair<double, double>> group_tt;
  std::vector<std::pair<double, double>> group_aa;

public:
  PolarAzimuthalLabelor(double tt_step, double aa_step, double aa_start)
      : Labelor(tt_step, aa_step, aa_start), group_tt({}), group_aa({}){};

  int operator()(SpectrumInfo const &spectrumInfo, int i) override {
    // find the group so that 2theta is inside its end points
    double tt = spectrumInfo.twoTheta(i);
    auto index = std::find_if(group_tt.begin(), group_tt.end(),
                              [tt](std::pair<double, double> x) { return x.first <= tt && tt < x.second; });
    // if no such group, make a new one
    if (index == group_tt.end()) {
      // check if 2theta is very close to an existing group
      auto ix = std::find_if(group_tt.begin(), group_tt.end(),
                             [tt, this](std::pair<double, double> x) { return x.first < tt + this->tt_step; });
      // if not close to existing group, make group starting here
      if (ix == group_tt.end())
        group_tt.push_back({tt, tt + tt_step});
      // if too close to an existing group, make group further back so no overlaps
      else
        group_tt.push_back({tt - tt_step, tt});
      index = group_tt.end() - 1;
    }
    return std::distance(group_tt.begin(), index) + 1;
  };
};

} // anonymous namespace

namespace Mantid::DataHandling {

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(GenerateGroupingPowder)

/// Algorithm's name for identification. @see Algorithm::name
const std::string GenerateGroupingPowder::name() const { return "GenerateGroupingPowder"; }

/// Algorithm's version for identification. @see Algorithm::version
int GenerateGroupingPowder::version() const { return 1; }

/// Algorithm's category for identification. @see Algorithm::category
const std::string GenerateGroupingPowder::category() const {
  return R"(DataHandling\Grouping;Transforms\Grouping;Diffraction\Utility)";
}

/** Initialize the algorithm's properties.
 */
void GenerateGroupingPowder::init() {
  declareProperty(std::make_unique<WorkspaceProperty<>>("InputWorkspace", "", Direction::Input,
                                                        std::make_shared<InstrumentValidator>()),
                  "A workspace from which to generate the grouping.");

  // allow saving as either xml or hdf5 formats
  auto fileExtensionXMLorHDF5 = std::make_shared<ListValidator<std::string>>();
  fileExtensionXMLorHDF5->addAllowedValue(std::string("xml"));
  fileExtensionXMLorHDF5->addAllowedValue(std::string("nxs"));
  fileExtensionXMLorHDF5->addAllowedValue(std::string("nx5"));
  declareProperty("FileFormat", std::string("xml"), fileExtensionXMLorHDF5,
                  "File extension/format for saving output: either xml (default) or nxs/nx5.");

  auto positiveDouble = std::make_shared<BoundedValidator<double>>();
  positiveDouble->setLower(0.0);
  declareProperty("AngleStep", -1.0, positiveDouble, "The angle step for grouping, in degrees.");

  auto withinCircle = std::make_shared<BoundedValidator<double>>();
  withinCircle->setLower(0.0);
  withinCircle->setUpper(360.0);
  withinCircle->setLowerExclusive(true);
  declareProperty("AzimuthalStep", 360.0, withinCircle, "The azimuthal angle step for grouping, in degrees.");
  auto withinDoubleCircle = std::make_shared<BoundedValidator<double>>();
  withinDoubleCircle->setLower(-360.0);
  withinDoubleCircle->setUpper(360.0);
  withinDoubleCircle->setExclusive(true);
  declareProperty("AzimuthalStart", 0.0, withinDoubleCircle, "The aimuthal angle start location, in degrees.");

  declareProperty(std::make_unique<FileProperty>("GroupingFilename", "", FileProperty::Save, "xml"),
                  "A grouping file that will be created.");
  declareProperty("GenerateParFile", true,
                  "If true, a par file with a corresponding name to the "
                  "grouping file will be generated.");

  declareProperty(std::make_unique<WorkspaceProperty<GroupingWorkspace>>("GroupingWorkspace", "", Direction::Output),
                  "The grouping workspace created");

  declareProperty("NumberByAngle", true,
                  "If true, divide sphere into groups by angle and step, number according to band."
                  "Empty parts of the instrument will effectively have group numbers that do not exist in the output.");
}

//----------------------------------------------------------------------------------------------
/**
 * @brief validate inputs
 *
 * @return std::map<std::string, std::string>
 */
std::map<std::string, std::string> GenerateGroupingPowder::validateInputs() {
  std::map<std::string, std::string> issues;
  const bool useAzimuthal = (getProperty("AzimuthalStep") < 360.0);
  const bool generateParFile = getProperty("GenerateParFile");
  if (useAzimuthal && generateParFile) {
    std::string noAzimuthInPar =
        "It is impossibleto save a PAR file while using azimuthal grouping." issues["EnergyMin"] = noAzimuthInPar;
    issues["EnergyMax"] = noAzimuthInPar;
  }
  return issues;
}

/** Execute the algorithm.
 */
void GenerateGroupingPowder::exec() {
  MatrixWorkspace_const_sptr input_ws = getProperty("InputWorkspace");
  const auto &spectrumInfo = input_ws->spectrumInfo();
  const auto &detectorInfo = input_ws->detectorInfo();
  const auto &detectorIDs = detectorInfo.detectorIDs();
  if (detectorIDs.empty())
    throw std::invalid_argument("Workspace contains no detectors.");

  const auto inst = input_ws->getInstrument();
  if (!inst) { // validator should make this impossible
    throw std::invalid_argument("Input Workspace has invalid Instrument\n");
  }

  auto groupWS = std::make_shared<GroupingWorkspace>(inst);
  this->setProperty("GroupingWorkspace", groupWS);

  const double step = getProperty("AngleStep");

  Labelor *label;
  OldLabelor oldlabel(step);
  PolarLabelor polarlabel(step);
  bool numberByAngle = getProperty("NumberByAngle");
  if (numberByAngle) {
    label = &oldlabel;
  } else {
    label = &polarlabel;
  }

  // const auto numSteps = static_cast<size_t>(180. / step + 1);

  for (size_t i = 0; i < spectrumInfo.size(); ++i) {
    if (!spectrumInfo.hasDetectors(i) || spectrumInfo.isMasked(i) || spectrumInfo.isMonitor(i)) {
      continue;
    }

    const auto &det = spectrumInfo.detector(i);
    // const double tt = spectrumInfo.twoTheta(i) * Geometry::rad2deg;
    const double groupId = (*label)(spectrumInfo, i);

    if (spectrumInfo.hasUniqueDetector(i)) {
      groupWS->setValue(det.getID(), groupId);
    } else {
      const auto &group = dynamic_cast<const DetectorGroup &>(det);
      const auto idv = group.getDetectorIDs();
      const auto ids = std::set<int>(idv.begin(), idv.end());
      groupWS->setValue(ids, groupId);
    }
  }

  std::string ext = this->getProperty("FileFormat");
  if (ext == "xml") {
    this->saveAsXML();
  } else if (ext == "nxs" || ext == "nx5") {
    this->saveAsNexus();
  } else {
    throw std::invalid_argument("that file format doesn't exist: must be xml, nxs, nx5\n");
  }

  if (getProperty("GenerateParFile")) {
    this->saveAsPAR();
  }
}

// XML file
void GenerateGroupingPowder::saveAsXML() {
  GroupingWorkspace_sptr groupWS = this->getProperty("GroupingWorkspace");
  const std::string XMLfilename = this->getProperty("GroupingFilename");
  // XML
  AutoPtr<Document> pDoc = new Document;
  AutoPtr<Element> pRoot = pDoc->createElement("detector-grouping");
  pDoc->appendChild(pRoot);
  pRoot->setAttribute("instrument", groupWS->getInstrument()->getName());

  const double step = getProperty("AngleStep");
  const auto numSteps = int(180. / step + 1);

  for (int i = 0; i < numSteps; ++i) {
    std::vector<detid_t> group = groupWS->getDetectorIDsOfGroup(i);
    size_t gSize = group.size(); // groups.at(i).size();
    if (gSize > 0) {
      std::stringstream spID, textvalue;
      spID << i;

      AutoPtr<Element> pChildGroup = pDoc->createElement("group");
      pChildGroup->setAttribute("ID", spID.str());
      pRoot->appendChild(pChildGroup);

      // std::copy(groups.at(i).begin(), groups.at(i).end(), std::ostream_iterator<size_t>(textvalue, ","));
      std::copy(group.begin(), group.end(), std::ostream_iterator<size_t>(textvalue, ","));
      std::string text = textvalue.str();
      const size_t found = text.rfind(',');
      if (found != std::string::npos) {
        text.erase(found, 1); // erase the last comma
      }

      AutoPtr<Element> pDetid = pDoc->createElement("detids");
      AutoPtr<Text> pText1 = pDoc->createTextNode(text);
      pDetid->appendChild(pText1);
      pChildGroup->appendChild(pDetid);
    }
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
}

// HDF5 file
void GenerateGroupingPowder::saveAsNexus() {
  GroupingWorkspace_sptr groupWS = this->getProperty("GroupingWorkspace");
  const std::string filename = this->getProperty("GroupingFilename");
  auto saveNexus = createChildAlgorithm("SaveNexusProcessed");
  saveNexus->setProperty("InputWorkspace", groupWS);
  saveNexus->setProperty("Filename", filename);
  saveNexus->executeAsChildAlg();
}

// PAR file
void GenerateGroupingPowder::saveAsPAR() {
  std::string PARfilename = getPropertyValue("GroupingFilename");
  std::string ext = getProperty("FileFormat");
  PARfilename = parFilenameFromXmlFilename(PARfilename);

  std::ofstream outPAR_file(PARfilename.c_str());
  if (!outPAR_file) {
    g_log.error("Unable to create file: " + PARfilename);
    throw Exception::FileError("Unable to create file: ", PARfilename);
  }
  MatrixWorkspace_const_sptr input_ws = getProperty("InputWorkspace");
  const auto &spectrumInfo = input_ws->spectrumInfo();

  const double step = getProperty("AngleStep");
  const auto numSteps = static_cast<size_t>(180. / step + 1);

  std::vector<std::vector<detid_t>> groups(numSteps);
  std::vector<double> twoThetaAverage(numSteps, 0.);
  std::vector<double> rAverage(numSteps, 0.);
  // run through spectrums
  for (size_t i = 0; i < spectrumInfo.size(); ++i) {
    // skip invalid cases
    if (!spectrumInfo.hasDetectors(i) || spectrumInfo.isMasked(i) || spectrumInfo.isMonitor(i)) {
      continue;
    }
    const auto &det = spectrumInfo.detector(i);
    // integer count angle slice
    const double tt = spectrumInfo.twoTheta(i) * Geometry::rad2deg;
    const auto where = static_cast<size_t>(tt / step);
    // create these averages at this slice?
    twoThetaAverage[where] += tt;
    rAverage[where] += spectrumInfo.l2(i);
    if (spectrumInfo.hasUniqueDetector(i)) {
      groups[where].emplace_back(det.getID());
    } else {
      const auto &group = dynamic_cast<const DetectorGroup &>(det);
      const auto idv = group.getDetectorIDs();
      groups[where].insert(groups[where].end(), idv.begin(), idv.end());
    }
  }

  size_t goodGroups(0);
  for (size_t i = 0; i < numSteps; ++i) {
    size_t gSize = groups.at(i).size();
    if (gSize > 0)
      ++goodGroups;
  }

  // Write the number of detectors to the file.
  outPAR_file << " " << goodGroups << '\n';

  for (size_t i = 0; i < numSteps; ++i) {
    const size_t gSize = groups.at(i).size();
    if (gSize > 0) {
      outPAR_file << std::fixed << std::setprecision(3);
      outPAR_file.width(10);
      outPAR_file << rAverage.at(i) / static_cast<double>(gSize);
      outPAR_file.width(10);
      outPAR_file << twoThetaAverage.at(i) / static_cast<double>(gSize);
      outPAR_file.width(10);
      outPAR_file << 0.;
      outPAR_file.width(10);
      outPAR_file << step * Geometry::deg2rad * rAverage.at(i) / static_cast<double>(gSize);
      outPAR_file.width(10);
      outPAR_file << 0.01;
      outPAR_file.width(10);
      outPAR_file << (groups.at(i)).at(0) << '\n';
    }
  }

  // Close the file
  outPAR_file.close();
}

// static function to convert output filename to parameter filename
// this assumes the file has a 3-letter extension so it works for
// nexus files as well
std::string GenerateGroupingPowder::parFilenameFromXmlFilename(const std::string &filename) {
  const std::size_t EXT_SIZE{3};
  std::string result(filename);
  result.replace(result.end() - EXT_SIZE, result.end(), "par");
  return result;
}

} // namespace Mantid::DataHandling
