// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidAlgorithms/CreateLogTimeCorrection.h"
#include "MantidAPI/FileProperty.h"
#include "MantidAPI/InstrumentValidator.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/Run.h"
#include "MantidAPI/TableRow.h"
#include "MantidAPI/WorkspaceProperty.h"
#include "MantidGeometry/Instrument/DetectorInfo.h"

#include <fstream>

using namespace Mantid;
using namespace Mantid::API;
using namespace Mantid::DataObjects;
using namespace Mantid::Geometry;
using namespace Mantid::Kernel;

using namespace std;

namespace Mantid {
namespace Algorithms {

DECLARE_ALGORITHM(CreateLogTimeCorrection)

//----------------------------------------------------------------------------------------------
/** Declare properties
 */
void CreateLogTimeCorrection::init() {
  declareProperty(std::make_unique<WorkspaceProperty<MatrixWorkspace>>(
                      "InputWorkspace", "", Direction::Input,
                      boost::make_shared<InstrumentValidator>()),
                  "Name of the input workspace to generate log correct from.");

  declareProperty(std::make_unique<WorkspaceProperty<TableWorkspace>>(
                      "OutputWorkspace", "", Direction::Output),
                  "Name of the output workspace containing the corrections.");

  declareProperty(std::make_unique<FileProperty>("OutputFilename", "",
                                                    FileProperty::OptionalSave),
                  "Name of the output time correction file.");
}

//----------------------------------------------------------------------------------------------
/** Main execution body
 */
void CreateLogTimeCorrection::exec() {
  // 1. Process input
  MatrixWorkspace_sptr dataWS = getProperty("InputWorkspace");

  //   Check whether the output workspace name is same as input
  string outwsname = getPropertyValue("OutputWorkspace");
  if (outwsname == dataWS->getName()) {
    stringstream errmsg;
    errmsg << "It is not allowed to use the same name by both input matrix "
              "workspace and output table workspace.";
    g_log.error(errmsg.str());
    throw runtime_error(errmsg.str());
  }

  const auto &detectorInfo = dataWS->detectorInfo();

  // 2. Log some information
  logGeometryInformation(detectorInfo);

  // 3. Calculate log time correction
  auto corrections = calculateCorrections(detectorInfo);

  // 4. Output
  TableWorkspace_sptr outWS =
      generateCorrectionTable(detectorInfo, corrections);
  setProperty("OutputWorkspace", outWS);

  string filename = getProperty("OutputFilename");
  g_log.information() << "Output file name is " << filename << ".\n";
  if (!filename.empty()) {
    writeCorrectionToFile(filename, detectorInfo, corrections);
  }
}

//----------------------------------------------------------------------------------------------
/** Get instrument geometry setup including L2 for each detector and L1
 */
void CreateLogTimeCorrection::logGeometryInformation(
    const Geometry::DetectorInfo &detectorInfo) const {

  g_log.information() << "Sample position = " << detectorInfo.samplePosition()
                      << "; "
                      << "Source position = " << detectorInfo.sourcePosition()
                      << ", L1 = " << detectorInfo.l1() << "; "
                      << "Number of detector/pixels = " << detectorInfo.size()
                      << ".\n";
}

//----------------------------------------------------------------------------------------------
/** Calculate the log time correction for each pixel, i.e., correcton from event
 * time at detector
 * to time at sample
 */
std::vector<double> CreateLogTimeCorrection::calculateCorrections(
    const Geometry::DetectorInfo &detectorInfo) const {

  std::vector<double> corrections(detectorInfo.size());
  const double l1 = detectorInfo.l1();
  for (size_t detectorIndex = 0; detectorIndex < detectorInfo.size();
       ++detectorIndex) {

    double corrfactor = l1 / (l1 + detectorInfo.l2(detectorIndex));
    corrections[detectorIndex] = corrfactor;
  }
  return corrections;
}

//----------------------------------------------------------------------------------------------
/** Write L2 map and correction map to a TableWorkspace
 */
TableWorkspace_sptr CreateLogTimeCorrection::generateCorrectionTable(
    const Geometry::DetectorInfo &detectorInfo,
    const std::vector<double> &corrections) const {
  auto tablews = boost::make_shared<TableWorkspace>();

  tablews->addColumn("int", "DetectorID");
  tablews->addColumn("double", "Correction");
  tablews->addColumn("double", "L2");

  const auto &detectorIds = detectorInfo.detectorIDs();

  for (size_t detectorIndex = 0; detectorIndex < detectorInfo.size();
       ++detectorIndex) {

    if (!detectorInfo.isMonitor(detectorIndex)) {
      const detid_t detid = detectorIds[detectorIndex];
      const double correction = corrections[detectorIndex];
      const double l2 = detectorInfo.l2(detectorIndex);

      TableRow newrow = tablews->appendRow();
      newrow << detid << correction << l2;
    }
  }

  return tablews;
}
//----------------------------------------------------------------------------------------------
/** Write correction map to a text file
 */
void CreateLogTimeCorrection::writeCorrectionToFile(
    const string filename, const Geometry::DetectorInfo &detectorInfo,
    const std::vector<double> &corrections) const {
  ofstream ofile;
  ofile.open(filename.c_str());

  if (ofile.is_open()) {

    const auto &detectorIds = detectorInfo.detectorIDs();
    for (size_t detectorIndex = 0; detectorIndex < corrections.size();
         ++detectorIndex) {
      if (!detectorInfo.isMonitor(detectorIndex)) {
        ofile << detectorIds[detectorIndex] << "\t" << setw(20)
              << setprecision(5) << corrections[detectorIndex] << "\n";
      }
    }
    ofile.close();
  } else {
    g_log.error() << "Unable to open file " << filename << " to write!\n";
  }
}

} // namespace Algorithms
} // namespace Mantid
