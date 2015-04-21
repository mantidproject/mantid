#include "MantidDataHandling/SaveNXSPE.h"
#include "MantidAPI/FileProperty.h"
#include "MantidKernel/ConfigService.h"
#include "MantidKernel/MantidVersion.h"
#include "MantidAPI/WorkspaceValidators.h"
#include "MantidAPI/WorkspaceOpOverloads.h"
#include "MantidGeometry/Instrument/Detector.h"
#include "MantidGeometry/Instrument/ObjComponent.h"
#include "MantidDataHandling/FindDetectorsPar.h"

#include <Poco/File.h>
#include <Poco/Path.h>
#include <limits>

namespace Mantid {
namespace DataHandling {

// Register the algorithm into the algorithm factory
DECLARE_ALGORITHM(SaveNXSPE)

using namespace Kernel;
using namespace API;

const double SaveNXSPE::MASK_FLAG = std::numeric_limits<double>::quiet_NaN();
const double SaveNXSPE::MASK_ERROR = 0.0;
// works fine but there were cases that some compilers crush on this (VS2008 in
// mixed .net environment ?)
const std::string SaveNXSPE::NXSPE_VER = "1.2";

SaveNXSPE::SaveNXSPE() : API::Algorithm() {}

/**
 * Initialise the algorithm
 */
void SaveNXSPE::init() {
  auto wsValidator = boost::make_shared<CompositeValidator>();
  wsValidator->add(boost::make_shared<API::WorkspaceUnitValidator>("DeltaE"));
  wsValidator->add<API::CommonBinsValidator>();
  wsValidator->add<API::HistogramValidator>();

  declareProperty(new WorkspaceProperty<MatrixWorkspace>(
                      "InputWorkspace", "", Direction::Input, wsValidator),
                  "The name of the workspace to save.");

  declareProperty(
      new API::FileProperty("Filename", "", FileProperty::Save,
                            std::vector<std::string>(1, ".nxspe")),
      "The name of the NXSPE file to write, as a full or relative path");

  declareProperty("Efixed", EMPTY_DBL(),
                  "Value of the fixed energy to write into NXSPE file.");
  declareProperty("Psi", EMPTY_DBL(), "Value of PSI to write into NXSPE file.");
  declareProperty(
      "KiOverKfScaling", true,
      "Flags in the file whether Ki/Kf scaling has been done or not.");

  // optional par or phx file
  std::vector<std::string> fileExts(2);
  fileExts[0] = ".par";
  fileExts[1] = ".phx";
  declareProperty(
      new FileProperty("ParFile", "not_used.par", FileProperty::OptionalLoad,
                       fileExts),
      "If provided, will replace detectors parameters in resulting nxspe file with the values taken from the file. \n\
        Should be used only if the parameters, calculated by the [[FindDetectorsPar]] algorithm are not suitable for some reason. \n\
        See [[FindDetectorsPar]] description for the details.");
}

/**
 * Execute the algorithm
 */
void SaveNXSPE::exec() {
  // Retrieve the input workspace
  MatrixWorkspace_sptr inputWS = getProperty("InputWorkspace");

  // Do the full check for common binning
  if (!WorkspaceHelpers::commonBoundaries(inputWS)) {
    g_log.error("The input workspace must have common bins");
    throw std::invalid_argument("The input workspace must have common bins");
  }

  // Number of spectra
  const int nHist = static_cast<int>(inputWS->getNumberHistograms());
  // Number of energy bins
  this->nBins = inputWS->blocksize();

  // Get a pointer to the sample
  Geometry::IComponent_const_sptr sample =
      inputWS->getInstrument()->getSample();

  // Retrieve the filename from the properties
  this->filename = getPropertyValue("Filename");

  // Create the file.
  ::NeXus::File nxFile(this->filename, NXACC_CREATE5);

  // Make the top level entry (and open it)
  std::string entryName = getPropertyValue("InputWorkspace");
  if(entryName.empty()) {
    entryName = "mantid_workspace";
  }
  nxFile.makeGroup(entryName, "NXentry", true);

  // Definition name and version
  nxFile.writeData("definition", "NXSPE");
  nxFile.openData("definition");
  nxFile.putAttr("version", NXSPE_VER);
  nxFile.closeData();

  // Program name and version
  nxFile.writeData("program_name", "mantid");
  nxFile.openData("program_name");
  nxFile.putAttr("version", Mantid::Kernel::MantidVersion::version());
  nxFile.closeData();

  // Create NXSPE_info
  nxFile.makeGroup("NXSPE_info", "NXcollection", true);

  // Get the value out of the property first
  double efixed = getProperty("Efixed");
  if (isEmpty(efixed))
    efixed = MASK_FLAG;
  // Now lets check to see if the workspace nows better.
  // TODO: Check that this is the way round we want to do it.
  const API::Run &run = inputWS->run();
  if (run.hasProperty("Ei")) {
    Kernel::Property *propEi = run.getProperty("Ei");
    efixed = boost::lexical_cast<double, std::string>(propEi->value());
  }
  nxFile.writeData("fixed_energy", efixed);
  nxFile.openData("fixed_energy");
  nxFile.putAttr("units", "meV");
  nxFile.closeData();

  double psi = getProperty("Psi");
  if (isEmpty(psi))
    psi = MASK_FLAG;
  nxFile.writeData("psi", psi);
  nxFile.openData("psi");
  nxFile.putAttr("units", "degrees");
  nxFile.closeData();

  bool kikfScaling = getProperty("KiOverKfScaling");
  if (kikfScaling) {
    nxFile.writeData("ki_over_kf_scaling", 1);
  } else {
    nxFile.writeData("ki_over_kf_scaling", 0);
  }

  nxFile.closeGroup(); // NXSPE_info

  // NXinstrument
  nxFile.makeGroup("instrument", "NXinstrument", true);
  // Write the instrument name
  nxFile.writeData("name", inputWS->getInstrument()->getName());
  // and the short name
  nxFile.openData("name");
  // TODO: Get the instrument short name
  nxFile.putAttr("short_name", inputWS->getInstrument()->getName());
  nxFile.closeData();

  // NXfermi_chopper
  nxFile.makeGroup("fermi", "NXfermi_chopper", true);

  nxFile.writeData("energy", efixed);
  nxFile.closeGroup(); // NXfermi_chopper

  nxFile.closeGroup(); // NXinstrument

  // NXsample
  nxFile.makeGroup("sample", "NXsample", true);
  // TODO: Write sample info
  //      nxFile.writeData("rotation_angle", 0.0);
  //      nxFile.writeData("seblock", "NONE");
  //      nxFile.writeData("temperature", 300.0);

  nxFile.closeGroup(); // NXsample

  // Make the NXdata group
  nxFile.makeGroup("data", "NXdata", true);

  // Energy bins
  // Get the Energy Axis (X) of the first spectra (they are all the same -
  // checked above)
  const MantidVec &X = inputWS->readX(0);
  nxFile.writeData("energy", X);
  nxFile.openData("energy");
  nxFile.putAttr("units", "meV");
  nxFile.closeData();

  // let's create some blank arrays in the nexus file

  std::vector<int> array_dims;
  array_dims.push_back((int)nHist);
  array_dims.push_back((int)nBins);

  nxFile.makeData("data", ::NeXus::FLOAT64, array_dims, false);
  nxFile.makeData("error", ::NeXus::FLOAT64, array_dims, false);

  // Add the axes attributes to the data
  nxFile.openData("data");
  nxFile.putAttr("signal", 1);
  nxFile.putAttr("axes", "polar:energy");
  nxFile.closeData();

  std::vector<int64_t> slab_start;
  std::vector<int64_t> slab_size;

  // What size slabs are we going to write...
  slab_size.push_back(1);
  slab_size.push_back((int64_t)nBins);

  // And let's start at the beginning
  slab_start.push_back(0);
  slab_start.push_back(0);

  // define the data and error vectors for masked detectors
  std::vector<double> masked_data(nBins, MASK_FLAG);
  std::vector<double> masked_error(nBins, MASK_ERROR);

  // Create a progress reporting object
  Progress progress(this, 0, 1, 100);
  const int progStep = (int)(ceil(nHist / 100.0));
  Geometry::IDetector_const_sptr det;
  // Loop over spectra
  for (int i = 0; i < nHist; i++) {
    try { // detector exist
      det = inputWS->getDetector(i);
      // Check that we aren't writing a monitor...
      if (!det->isMonitor()) {
        Geometry::IDetector_const_sptr det = inputWS->getDetector(i);

        if (!det->isMasked()) {
          // no masking...
          // Open the data
          nxFile.openData("data");
          slab_start[0] = i;
          nxFile.putSlab(const_cast<MantidVec &>(inputWS->readY(i)), slab_start,
                         slab_size);
          // Close the data
          nxFile.closeData();

          // Open the error
          nxFile.openData("error");
          // MantidVec& tmparr = const_cast<MantidVec&>(inputWS->dataE(i));
          // nxFile.putSlab((void*)(&(tmparr[0])), slab_start, slab_size);
          nxFile.putSlab(const_cast<MantidVec &>(inputWS->readE(i)), slab_start,
                         slab_size);
          // Close the error
          nxFile.closeData();
        } else {
          // Write a masked value...
          // Open the data
          nxFile.openData("data");
          slab_start[0] = i;
          nxFile.putSlab(masked_data, slab_start, slab_size);
          // Close the data
          nxFile.closeData();

          // Open the error
          nxFile.openData("error");
          nxFile.putSlab(masked_error, slab_start, slab_size);
          // Close the error
          nxFile.closeData();
        }
      }
    } catch (Exception::NotFoundError &) {
      // Catch if no detector. Next line tests whether this happened - test
      // placed
      // outside here because Mac Intel compiler doesn't like 'continue' in a
      // catch
      // in an openmp block.
    }
    // If no detector found, skip onto the next spectrum
    if (!det)
      continue;

    // make regular progress reports and check for canceling the algorithm
    if (i % progStep == 0) {
      progress.report();
    }
  }
  // execute the ChildAlgorithm to calculate the detector's parameters;
  IAlgorithm_sptr spCalcDetPar =
      this->createChildAlgorithm("FindDetectorsPar", 0, 1, true, 1);

  spCalcDetPar->initialize();
  spCalcDetPar->setProperty("InputWorkspace", inputWS);
  std::string parFileName = this->getPropertyValue("ParFile");
  if (!(parFileName.empty() || parFileName == "not_used.par")) {
    spCalcDetPar->setPropertyValue("ParFile", parFileName);
  }
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

  // Write the Polar (2Theta) angles
  nxFile.writeData("polar", polar);

  // Write the Azimuthal (phi) angles
  nxFile.writeData("azimuthal", azimuthal);

  // Now the widths...
  nxFile.writeData("polar_width", polar_width);
  nxFile.writeData("azimuthal_width", azimuthal_width);

  // Secondary flight path
  nxFile.writeData("distance", secondary_flightpath);

  nxFile.closeGroup(); // NXdata

  nxFile.closeGroup(); // Top level NXentry
}

} // namespace DataHandling
} // namespace Mantid
