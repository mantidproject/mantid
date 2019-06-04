// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidDataHandling/SavePAR.h"
#include "MantidDataHandling/FindDetectorsPar.h"

#include "MantidAPI/FileProperty.h"
#include "MantidAPI/InstrumentValidator.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidGeometry/Instrument.h"
#include "MantidGeometry/Instrument/Detector.h"
#include "MantidGeometry/Instrument/ObjComponent.h"
#include "MantidGeometry/Objects/CSGObject.h"

#include <cstdio>
#include <fstream>

namespace Mantid {
namespace DataHandling {

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(SavePAR)

using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace Mantid::Geometry;

// A reference to the logger is provided by the base class, it is called g_log.
// It is used to print out information,

void SavePAR::init() {
  declareProperty(std::make_unique<WorkspaceProperty<>>(
                      "InputWorkspace", "", Direction::Input,
                      boost::make_shared<InstrumentValidator>()),
                  "The name of the workspace to save.");
  declareProperty(std::make_unique<FileProperty>("Filename", "", FileProperty::Save),
                  "The name to give to the saved file.");
}

void SavePAR::exec() {

  // Get the input workspace
  MatrixWorkspace_sptr inputWorkspace = getProperty("InputWorkspace");

  // Retrieve the filename from the properties
  const std::string filename = getProperty("Filename");

  // Get a pointer to the sample
  IComponent_const_sptr sample = inputWorkspace->getInstrument()->getSample();

  std::ofstream outPAR_file(filename.c_str());

  if (!outPAR_file) {
    g_log.error("Failed to open (PAR) file:" + filename);
    throw Kernel::Exception::FileError("Failed to open (PAR) file:", filename);
  }

  // execute the ChildAlgorithm to calculate the detector's parameters;
  IAlgorithm_sptr spCalcDetPar =
      this->createChildAlgorithm("FindDetectorsPar", 0, 1, true, 1);
  spCalcDetPar->initialize();
  spCalcDetPar->setPropertyValue("InputWorkspace", inputWorkspace->getName());
  // calculate linear rather then angular detector's sizes;
  spCalcDetPar->setPropertyValue("ReturnLinearRanges", "1");
  // in test mode, request the ChildAlgortithm to create output workspace and
  // add it to dataservice
  if (!det_par_ws_name.empty()) {
    spCalcDetPar->setPropertyValue("OutputParTable", det_par_ws_name);
  }

  // let's not do this for the time being
  /* std::string parFileName = this->getPropertyValue("ParFile");
      if(!(parFileName.empty()||parFileName=="not_used.par")){
                spCalcDetPar->setPropertyValue("ParFile",parFileName);
                    }*/
  spCalcDetPar->execute();
  //
  FindDetectorsPar *pCalcDetPar =
      dynamic_cast<FindDetectorsPar *>(spCalcDetPar.get());
  if (!pCalcDetPar) { // "can not get pointer to FindDetectorsPar algorithm"
    throw(std::bad_cast());
  }
  const std::vector<double> &azimuthal = pCalcDetPar->getAzimuthal();
  const std::vector<double> &polar = pCalcDetPar->getPolar();
  const std::vector<double> &azimuthal_width = pCalcDetPar->getAzimWidth();
  const std::vector<double> &polar_width = pCalcDetPar->getPolarWidth();
  const std::vector<double> &secondary_flightpath =
      pCalcDetPar->getFlightPath();
  const std::vector<size_t> &det_ID = pCalcDetPar->getDetID();

  size_t nDetectors = pCalcDetPar->getNDetectors();

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

} // namespace DataHandling
} // namespace Mantid
