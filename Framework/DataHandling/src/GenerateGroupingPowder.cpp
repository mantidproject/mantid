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
#include "MantidDataObjects/GroupingWorkspace.h"
#include "MantidGeometry/Crystal/AngleUnits.h"
#include "MantidGeometry/Instrument/DetectorGroup.h"
#include "MantidGeometry/Instrument/DetectorInfo.h"
#include "MantidKernel/BoundedValidator.h"
#include "MantidKernel/ListValidator.h"

#include <Poco/DOM/AutoPtr.h>
#include <Poco/DOM/DOMWriter.h>
#include <Poco/DOM/Document.h>
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
  Labelor(double t_step, double a_step = 0.0, double a_start = 0.0)
      : tt_step(t_step * Mantid::Geometry::deg2rad), aa_step(a_step * Mantid::Geometry::deg2rad),
        aa_start(a_start * Mantid::Geometry::deg2rad) {};
  virtual ~Labelor() {};
  virtual size_t operator()(SpectrumInfo const &spectrumInfo, size_t i) = 0;
};

class CircularSectorLabelor : public Labelor {
  /** Divides sphere into bands of size tt_step,
   * which are then numbered sequentially, in order, with increasing twoTheta.
   * The label must be increased by 1 from original so that there are no zero labels, to be
   * consistent with GroupingWorkspace's use of 0 to signal an invalid or unused detector.
   */

  // reduce flops by saving as mult. constant
  const double inv_tt_step;

public:
  CircularSectorLabelor(double t_step) : Labelor(t_step), inv_tt_step(1.0 / tt_step) {};
  size_t operator()(SpectrumInfo const &spectrumInfo, size_t i) override {
    return static_cast<size_t>(spectrumInfo.twoTheta(i) * inv_tt_step) + 1;
  };
};

class SphericalSectorLabelor : public Labelor {
  /**
   * Divides sphere into spherical sectors in 2theta and phi, similarly to above.
   * Ordering goes by increasing 2theta, then by increasing phi
   * Able to distinguish left/right
   */

  // reduce flops by saving as mult. constant
  const double inv_tt_step;
  const double inv_aa_step;
  const int num_aa_step;

public:
  SphericalSectorLabelor(double t_step, double a_step, double a_start)
      : Labelor(t_step, a_step, a_start), inv_tt_step(1.0 / tt_step), inv_aa_step(1.0 / aa_step),
        num_aa_step(int(std::ceil(360.0 / a_step))) {};
  size_t operator()(SpectrumInfo const &spectrumInfo, size_t i) override {
    return static_cast<size_t>(spectrumInfo.twoTheta(i) * inv_tt_step) * num_aa_step +
           static_cast<size_t>(std::floor((spectrumInfo.azimuthal(i) - aa_start) * inv_aa_step)) % num_aa_step + 1;
  };
};

class CircularOrderedLabelor : public Labelor {
  /**
   * Labels using unsigned twoTheta, with no gaps in labeld groups
   * Groups are labeled in order of twoTheta
   * First, angles are sorted then sphere is divided into bands of width tt_step.
   * Within each band, at most two possibilities for label
   * The map `groups` assigns to each band the options for group label
   * The map `divs` assigns to each band the value that splits the groups in that band
   * These maps are pre-populated when the labelor is created.
   */
  std::unordered_map<size_t, double> divs;
  std::unordered_map<size_t, std::vector<size_t>> groups;

  const double inv_tt_step;
  size_t currentGroup;

  inline size_t getBand(const double x) { return static_cast<int>(x * inv_tt_step); };

public:
  CircularOrderedLabelor(double t_step, SpectrumInfo const &spectrumInfo)
      : Labelor(t_step), inv_tt_step(1.0 / tt_step), currentGroup(0) {

    std::vector<double> tt(spectrumInfo.size());
    std::transform(spectrumInfo.cbegin(), spectrumInfo.cend(), tt.begin(), [](auto x) {
      return (x.isMonitor() || x.isMasked() || !x.hasDetectors() ? -1.0 : M_PI - x.twoTheta());
    });
    auto stopit = std::remove(tt.begin(), tt.end(), -1.0);
    tt.erase(stopit, tt.end());
    std::sort(tt.begin(), tt.end());

    auto it = tt.begin();
    while (it != tt.end()) {
      size_t band = getBand(*it);
      divs[band] = *it;
      groups[band].push_back(++currentGroup);
      groups[band + 1].push_back(currentGroup);
      double next = *it + tt_step;
      // cppcheck-suppress derefInvalidIterator
      while ((*it) <= next && it != tt.end())
        it++;
    }
  };

  size_t operator()(SpectrumInfo const &spectrumInfo, size_t i) override {
    double tt = M_PI - spectrumInfo.twoTheta(i);
    size_t band = getBand(tt);
    size_t index = groups[band].back();
    if (tt < divs[band])
      index = groups[band].front();
    return index;
  }
};

class SplitCircularOrderedLabelor : public Labelor {
  /**
   * As CircularOrderedLabelor but splitting left/right
   */
  const double inv_tt_step;

  std::unordered_map<size_t, double> divs;
  std::unordered_map<size_t, std::vector<size_t>> groups;
  size_t currentGroup;

  inline size_t getBand(const double tt) { return static_cast<size_t>(tt * inv_tt_step); };

public:
  SplitCircularOrderedLabelor(double t_step, SpectrumInfo const &spectrumInfo)
      : Labelor(t_step), inv_tt_step(1.0 / tt_step), currentGroup(0) {
    std::vector<double> xx(spectrumInfo.size());
    std::transform(spectrumInfo.cbegin(), spectrumInfo.cend(), xx.begin(), [](auto x) {
      return (x.isMonitor() || x.isMasked() || !x.hasDetectors() ? -1.0 : M_PI - x.signedTwoTheta());
    });
    auto stopit = std::remove(xx.begin(), xx.end(), -1.0);
    xx.erase(stopit, xx.end());
    std::transform(xx.cbegin(), xx.cend(), xx.begin(), [](double x) { return (x <= M_PI ? x : 3. * M_PI - x); });
    std::sort(xx.begin(), xx.end());

    auto it = xx.begin();
    while (it != xx.end()) {
      size_t band = getBand(*it);
      divs[band] = *it;
      groups[band].push_back(++currentGroup);
      groups[band + 1].push_back(currentGroup);
      double next = (*it) + tt_step;
      // cppcheck-suppress derefInvalidIterator
      while ((*it) <= next && it != xx.end())
        it++;
    }
  };

  size_t operator()(SpectrumInfo const &spectrumInfo, size_t i) override {
    double tt = spectrumInfo.signedTwoTheta(i);
    tt = (tt < 0 ? 2. * M_PI + tt : M_PI - tt);
    size_t band = getBand(tt);
    size_t index = groups[band].back();
    if (tt < divs[band])
      index = groups[band].front();
    return (index);
  }
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
  declareProperty(std::make_unique<WorkspaceProperty<GroupingWorkspace>>("GroupingWorkspace", "", Direction::Output,
                                                                         PropertyMode::Optional),
                  "The grouping workspace created");

  auto positiveDouble = std::make_shared<BoundedValidator<double>>();
  positiveDouble->setLower(0.0);
  declareProperty("AngleStep", -1.0, positiveDouble, "The polar angle step for grouping, in degrees.");

  auto withinCircle = std::make_shared<BoundedValidator<double>>();
  withinCircle->setLower(0.0);
  withinCircle->setUpper(360.0);
  withinCircle->setLowerExclusive(true);
  declareProperty("AzimuthalStep", 360.0, withinCircle, "The azimuthal angle step for grouping, in degrees.");
  auto withinDoubleCircle = std::make_shared<BoundedValidator<double>>();
  withinDoubleCircle->setLower(-360.0);
  withinDoubleCircle->setUpper(360.0);
  withinDoubleCircle->setExclusive(true);
  declareProperty("AzimuthalStart", 0.0, withinDoubleCircle, "The azimuthal angle start location, in degrees.");

  declareProperty("NumberByAngle", true,
                  "If true, divide sphere into groups labeled by angular sector."
                  "Empty parts of the instrument will effectively have group numbers that do not exist in the output."
                  "If false, labels will be assigned in order of highest angle.");

  // allow saving as either xml or hdf5 formats
  auto fileExtensionXMLorHDF5 = std::make_shared<ListValidator<std::string>>();
  fileExtensionXMLorHDF5->addAllowedValue(std::string("xml"));
  fileExtensionXMLorHDF5->addAllowedValue(std::string("nxs"));
  fileExtensionXMLorHDF5->addAllowedValue(std::string("nx5"));
  declareProperty("FileFormat", std::string("xml"), fileExtensionXMLorHDF5,
                  "File extension/format for saving output: either xml (default) or nxs/nx5.");
  declareProperty(std::make_unique<FileProperty>("GroupingFilename", "", FileProperty::OptionalSave,
                                                 std::vector<std::string>{"xml", "nxs", "nx5"}),
                  "A grouping file that will be created.");
  declareProperty("GenerateParFile", true,
                  "If true, a par file with a corresponding name to the "
                  "grouping file will be generated. Ignored if grouping file is not specified.");
}

//----------------------------------------------------------------------------------------------
/**
 * @brief validate inputs
 *
 * @return std::map<std::string, std::string>
 */
std::map<std::string, std::string> GenerateGroupingPowder::validateInputs() {
  std::map<std::string, std::string> issues;

  const bool useAzimuthal = (double(getProperty("AzimuthalStep")) < 360.0);
  const bool generateParFile = getProperty("GenerateParFile");
  if (useAzimuthal && generateParFile) {
    std::string noAzimuthInPar = "Cannot save a PAR file while using azimuthal grouping.";
    issues["AzimuthalStep"] = noAzimuthInPar;
    issues["GenerateParFile"] = noAzimuthInPar;
  }

  if (isDefault("GroupingWorkspace") && isDefault("GroupingFilename")) {
    std::string err = "At least one of {'GroupingWorkspace', 'GroupingFilename'} must be passed.";
    issues["GroupingWorkspace"] = err;
    issues["GroupingFilename"] = err;
  }

  return issues;
}

/** Execute the algorithm.
 */
void GenerateGroupingPowder::exec() {
  createGroups();
  saveGroups();
}

void GenerateGroupingPowder::createGroups() {
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

  this->m_groupWS = std::make_shared<GroupingWorkspace>(inst);
  if (!isDefault("GroupingWorkspace")) {
    this->setProperty("GroupingWorkspace", this->m_groupWS);
  }

  const double step = getProperty("AngleStep");

  std::unique_ptr<Labelor> label;
  bool numberByAngle = getProperty("NumberByAngle");
  double azimuthalStep = getProperty("AzimuthalStep");
  double azimuthalStart = getProperty("AzimuthalStart");
  if (numberByAngle) {
    if (azimuthalStep == 360.0)
      label = std::make_unique<CircularSectorLabelor>(step);
    else
      label = std::make_unique<SphericalSectorLabelor>(step, azimuthalStep, azimuthalStart);
  } else {
    if (azimuthalStep == 360.0)
      label = std::make_unique<CircularOrderedLabelor>(step, spectrumInfo);
    else
      label = std::make_unique<SplitCircularOrderedLabelor>(step, spectrumInfo);
  }

  for (size_t i = 0; i < spectrumInfo.size(); ++i) {
    if (!spectrumInfo.hasDetectors(i) || spectrumInfo.isMasked(i) || spectrumInfo.isMonitor(i)) {
      continue;
    }

    const auto &det = spectrumInfo.detector(i);
    const double groupId = (double)(*label)(spectrumInfo, i);

    if (spectrumInfo.hasUniqueDetector(i)) {
      this->m_groupWS->setValue(det.getID(), groupId);
    } else {
      const auto &group = dynamic_cast<const DetectorGroup &>(det);
      const auto idv = group.getDetectorIDs();
      const auto ids = std::set<detid_t>(idv.begin(), idv.end());
      this->m_groupWS->setValue(ids, groupId);
    }
  }
}

void GenerateGroupingPowder::saveGroups() {

  // save if a filename was specified
  if (!isDefault("GroupingFilename")) {

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
}

// XML file
void GenerateGroupingPowder::saveAsXML() {
  const std::string XMLfilename = this->getProperty("GroupingFilename");
  // XML
  AutoPtr<Document> pDoc = new Document;
  AutoPtr<Element> pRoot = pDoc->createElement("detector-grouping");
  pDoc->appendChild(pRoot);
  pRoot->setAttribute("instrument", this->m_groupWS->getInstrument()->getName());

  const double step = getProperty("AngleStep");
  const auto numSteps = int(180. / step + 1);

  for (int i = 0; i < numSteps; ++i) {
    std::vector<detid_t> group = this->m_groupWS->getDetectorIDsOfGroup(i);
    size_t gSize = group.size();
    if (gSize > 0) {
      std::stringstream spID, textvalue;
      // try to preserve original behavior of previous method, labeling starting at 0
      bool numberByAngle = this->getProperty("NumberByAngle");
      if (numberByAngle)
        spID << i - 1;
      else
        spID << i;

      AutoPtr<Element> pChildGroup = pDoc->createElement("group");
      pChildGroup->setAttribute("ID", spID.str());
      pRoot->appendChild(pChildGroup);

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
  const std::string filename = this->getProperty("GroupingFilename");
  auto saveNexus = createChildAlgorithm("SaveNexusProcessed");
  saveNexus->setProperty("InputWorkspace", this->m_groupWS);
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
