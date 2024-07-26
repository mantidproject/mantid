// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidDataHandling/SavePAR.h"
#include "MantidDataHandling/FindDetectorsPar.h"

#include "MantidAPI/FileProperty.h"
#include "MantidAPI/InstrumentValidator.h"
#include "MantidAPI/MatrixWorkspace.h"

namespace Mantid::DataHandling {

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(SavePAR)

using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace Mantid::Geometry;

// A reference to the logger is provided by the base class, it is called g_log.
// It is used to print out information,

void SavePAR::init() {
  declareProperty(std::make_unique<WorkspaceProperty<>>("InputWorkspace", "", Direction::Input,
                                                        std::make_shared<InstrumentValidator>()),
                  "The name of the workspace to save.");
  declareProperty(std::make_unique<FileProperty>("Filename", "", FileProperty::Save),
                  "The name to give to the saved file.");
}

void SavePAR::exec() {
  // Get the input workspace
  MatrixWorkspace_sptr inputWorkspace = getProperty("InputWorkspace");

  // Retrieve the filename from the properties
  const std::string filename = getProperty("Filename");

  // execute the ChildAlgorithm to calculate the detector's parameters;
  auto spCalcDetPar = createChildAlgorithm("FindDetectorsPar", 0, 1, true, 1);
  spCalcDetPar->initialize();
  spCalcDetPar->setPropertyValue("InputWorkspace", inputWorkspace->getName());
  // calculate linear rather then angular detector's sizes;
  spCalcDetPar->setPropertyValue("ReturnLinearRanges", "1");
  // in test mode, request the ChildAlgortithm to create output workspace and
  // add it to dataservice
  if (!det_par_ws_name.empty()) {
    spCalcDetPar->setPropertyValue("OutputParTable", det_par_ws_name);
  }

  spCalcDetPar->execute();
  //
  const auto *pCalcDetPar = dynamic_cast<FindDetectorsPar *>(spCalcDetPar.get());
  if (!pCalcDetPar) { // "can not get pointer to FindDetectorsPar algorithm"
    throw(std::bad_cast());
  }
  const std::vector<double> &azimuthal = pCalcDetPar->getAzimuthal();
  const std::vector<double> &polar = pCalcDetPar->getPolar();
  const std::vector<double> &azimuthal_width = pCalcDetPar->getAzimWidth();
  const std::vector<double> &polar_width = pCalcDetPar->getPolarWidth();
  const std::vector<double> &secondary_flightpath = pCalcDetPar->getFlightPath();
  const std::vector<size_t> &det_ID = pCalcDetPar->getDetID();

  size_t nDetectors = pCalcDetPar->getNDetectors();

  writePAR(filename, azimuthal, polar, azimuthal_width, polar_width, secondary_flightpath, det_ID, nDetectors);
}

void SavePAR::writePAR(const std::string &filename, const std::vector<double> &azimuthal,
                       const std::vector<double> &polar, const std::vector<double> &azimuthal_width,
                       const std::vector<double> &polar_width, const std::vector<double> &secondary_flightpath,
                       const std::vector<size_t> &det_ID, const size_t nDetectors) {
  std::ofstream outPAR_file(filename.c_str());

  if (!outPAR_file) {
    throw Kernel::Exception::FileError("Failed to open (PAR) file:", filename);
  }
  // Write the number of detectors to the file.
  outPAR_file << " " << nDetectors << '\n';

  for (size_t i = 0; i < nDetectors; ++i) {
    // verify if no detector defined;
    volatile double NanID = azimuthal[i];
    if (NanID != azimuthal[i])
      continue; // skip NaN -s

    // Now write all the detector info.
    outPAR_file << std::fixed << std::setprecision(3);
    outPAR_file.width(10);
    outPAR_file << secondary_flightpath[i];
    outPAR_file.width(10);
    outPAR_file << polar[i];
    outPAR_file.width(10);
    outPAR_file << (-azimuthal[i]);
    outPAR_file.width(10);
    outPAR_file << polar_width[i];
    outPAR_file.width(10);
    outPAR_file << azimuthal_width[i];
    outPAR_file.width(10);
    outPAR_file << det_ID[i] << '\n';
  }

  // Close the file
  outPAR_file.close();
}

} // namespace Mantid::DataHandling
