#include "MantidDataHandling/SavePHX.h"
#include "MantidDataHandling/FindDetectorsPar.h"
#include "MantidAPI/FileProperty.h"
#include "MantidGeometry/Instrument/Detector.h"
#include "MantidGeometry/Instrument/ObjComponent.h"
#include "MantidGeometry/Objects/Object.h"
#include "MantidAPI/WorkspaceValidators.h"

#include <cstdio>
#include <fstream>
#include <iomanip>

namespace Mantid {
namespace DataHandling {

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(SavePHX)

using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace Mantid::Geometry;

// A reference to the logger is provided by the base class, it is called g_log.
// It is used to print out information,

void SavePHX::init() {
  declareProperty(
      new WorkspaceProperty<>("InputWorkspace", "", Direction::Input,
                              boost::make_shared<InstrumentValidator>()),
      "The input workspace");
  declareProperty(new FileProperty("Filename", "", FileProperty::Save),
                  "The filename to use for the saved data");
}

void SavePHX::exec() {

  // Get the input workspace
  MatrixWorkspace_sptr inputWorkspace = getProperty("InputWorkspace");

  // Get the sample position
  const Kernel::V3D samplePos =
      inputWorkspace->getInstrument()->getSample()->getPos();

  // Retrieve the filename from the properties
  const std::string filename = getProperty("Filename");

  // Get a pointer to the sample
  IComponent_const_sptr sample = inputWorkspace->getInstrument()->getSample();

  std::ofstream outPHX_file(filename.c_str());

  if (!outPHX_file) {
    g_log.error("Failed to open (PHX) file:" + filename);
    throw Kernel::Exception::FileError("Failed to open (PHX) file:", filename);
  }

  // execute the ChildAlgorithm to calculate the detector's parameters;
  IAlgorithm_sptr spCalcDetPar =
      this->createChildAlgorithm("FindDetectorsPar", 0, 1, true, 1);
  spCalcDetPar->initialize();
  spCalcDetPar->setPropertyValue("InputWorkspace", inputWorkspace->getName());
  spCalcDetPar->setPropertyValue("ReturnLinearRanges", "0");
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
  outPHX_file << " " << nDetectors << std::endl;

  for (size_t i = 0; i < nDetectors; ++i) {
    // verify if no detector defined;
    volatile double NanID = azimuthal[i];
    if (NanID != azimuthal[i])
      continue; // skip NaN -s

    // Now write all the detector info.
    outPHX_file << std::fixed << std::setprecision(3);
    outPHX_file << " " << secondary_flightpath[i] << "\t 0 \t\t" << polar[i]
                << " \t" << azimuthal[i] << " \t" << polar_width[i] << " \t"
                << azimuthal_width[i] << " \t\t" << det_ID[i] << std::endl;
  }

  // Close the file
  outPHX_file.close();
}

} // namespace DataHandling
} // namespace Mantid
