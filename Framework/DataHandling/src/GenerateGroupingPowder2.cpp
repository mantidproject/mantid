// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2024 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidDataHandling/GenerateGroupingPowder2.h"

#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidDataHandling/FindDetectorsPar.h"
#include "MantidDataHandling/SavePAR.h"
#include "MantidGeometry/Crystal/AngleUnits.h"

using namespace Mantid::API;
using namespace Mantid::DataObjects;

namespace Mantid::DataHandling {

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(GenerateGroupingPowder2)

/// Algorithm's version for identification. @see Algorithm::version
int GenerateGroupingPowder2::version() const { return 2; }

/** Initialize the algorithm's properties.
 */
void GenerateGroupingPowder2::init() {
  GenerateGroupingPowder::init();

  // This version will determine the file format from the extension of the GroupingFilename property
  removeProperty("FileFormat");
}

/** Execute the algorithm.
 */
void GenerateGroupingPowder2::exec() {
  createGroups();
  saveGroups();
}

void GenerateGroupingPowder2::saveGroups() {

  // save if a filename was specified
  if (!isDefault("GroupingFilename")) {

    std::string filename = this->getProperty("GroupingFilename");
    std::string ext = filename.substr(filename.length() - 3);
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

  // create an empty workspace based on input to perform GroupDetectors with
  MatrixWorkspace_sptr inputWorkspace = getProperty("InputWorkspace");
  inputWorkspace = WorkspaceFactory::Instance().create(inputWorkspace, -1, 1, 1);

  auto groupDetectors = createChildAlgorithm("GroupDetectors", 0, 1, true, 2);
  groupDetectors->initialize();
  groupDetectors->setProperty("InputWorkspace", inputWorkspace);
  groupDetectors->setProperty("CopyGroupingFromWorkspace", this->m_groupWS);
  groupDetectors->execute();

  const MatrixWorkspace_sptr groupedWorkspace = groupDetectors->getProperty("OutputWorkspace");

  auto spCalcDetPar = createChildAlgorithm("FindDetectorsPar", 0, 1, true, 1);
  spCalcDetPar->initialize();
  spCalcDetPar->setProperty("InputWorkspace", groupedWorkspace);
  spCalcDetPar->setProperty("ReturnLinearRanges", true);
  spCalcDetPar->execute();

  auto *pCalcDetPar = dynamic_cast<FindDetectorsPar *>(spCalcDetPar.get());
  const size_t nDetectors = pCalcDetPar->getNDetectors();
  const std::vector<double> &secondary_flightpath = pCalcDetPar->getFlightPath();

  double stepSize = getProperty("AngleStep");
  stepSize *= Geometry::deg2rad;
  std::vector<double> polar_width;

  std::transform(secondary_flightpath.cbegin(), secondary_flightpath.cend(), std::back_inserter(polar_width),
                 [stepSize](const double r) { return r * stepSize; });

  SavePAR::writePAR(PARfilename, std::vector<double>(nDetectors, -0.), pCalcDetPar->getPolar(),
                    std::vector<double>(nDetectors, 0.01), polar_width, secondary_flightpath, pCalcDetPar->getDetID(),
                    nDetectors);
}
} // namespace Mantid::DataHandling
