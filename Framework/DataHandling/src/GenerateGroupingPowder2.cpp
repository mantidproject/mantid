// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2024 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidDataHandling/GenerateGroupingPowder2.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/SpectrumInfo.h"
#include "MantidDataObjects/GroupingWorkspace.h"
#include "MantidGeometry/Crystal/AngleUnits.h"
#include "MantidGeometry/Instrument/DetectorGroup.h"
#include "MantidGeometry/Instrument/DetectorInfo.h"

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

namespace Mantid::DataHandling {

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(GenerateGroupingPowder2)

/// Algorithm's version for identification. @see Algorithm::version
int GenerateGroupingPowder2::version() const { return 2; }

/** Execute the algorithm.
 */
void GenerateGroupingPowder2::exec() {
  createGroups();
  saveGroups();
}

void GenerateGroupingPowder2::saveGroups() {

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
void GenerateGroupingPowder2::saveAsXML() {
  const std::string filename = this->getProperty("GroupingFilename");
  auto saveDetectorsGrouping = createChildAlgorithm("SaveDetectorsGrouping");
  saveDetectorsGrouping->setProperty("InputWorkspace", this->m_groupWS);
  saveDetectorsGrouping->setProperty("OutputFile", filename);
  saveDetectorsGrouping->setProperty("SaveUngroupedDetectors", false);
  saveDetectorsGrouping->executeAsChildAlg();
}

// PAR file
void GenerateGroupingPowder2::saveAsPAR() {
  std::string PARfilename = getPropertyValue("GroupingFilename");
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
} // namespace Mantid::DataHandling
