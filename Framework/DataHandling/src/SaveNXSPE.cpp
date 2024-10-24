// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidDataHandling/SaveNXSPE.h"

#include "MantidAPI/CommonBinsValidator.h"
#include "MantidAPI/FileProperty.h"
#include "MantidAPI/HistogramValidator.h"
#include "MantidAPI/Run.h"
#include "MantidAPI/SpectrumInfo.h"
#include "MantidAPI/WorkspaceUnitValidator.h"

#include "MantidDataHandling/FindDetectorsPar.h"
#include "MantidGeometry/Instrument.h"

#include "MantidKernel/CompositeValidator.h"
#include "MantidKernel/MantidVersion.h"
#include "MantidKernel/Unit.h"

#include <Poco/File.h>
#include <Poco/Path.h>
#include <boost/scoped_array.hpp>
#include <limits>
#include <nexus/NeXusFile.hpp>

namespace Mantid::DataHandling {

// Register the algorithm into the algorithm factory
DECLARE_ALGORITHM(SaveNXSPE)

using namespace Kernel;
using namespace API;

const double SaveNXSPE::MASK_FLAG = std::numeric_limits<double>::quiet_NaN();
const double SaveNXSPE::MASK_ERROR = 0.0;
// works fine but there were cases that some compilers crush on this (VS2008 in
// mixed .net environment ?)
const std::string SaveNXSPE::NXSPE_VER = "1.3";
// 4MB chunk size
const size_t SaveNXSPE::MAX_CHUNK_SIZE = 4194304;

SaveNXSPE::SaveNXSPE() : API::Algorithm() {}

/**
 * Initialise the algorithm
 */
void SaveNXSPE::init() {
  auto wsValidator = std::make_shared<CompositeValidator>();
  wsValidator->add(std::make_shared<API::WorkspaceUnitValidator>("DeltaE"));
  wsValidator->add<API::CommonBinsValidator>();
  wsValidator->add<API::HistogramValidator>();

  declareProperty(
      std::make_unique<WorkspaceProperty<MatrixWorkspace>>("InputWorkspace", "", Direction::Input, wsValidator),
      "The name of the workspace to save.");

  declareProperty(
      std::make_unique<API::FileProperty>("Filename", "", FileProperty::Save, std::vector<std::string>(1, ".nxspe")),
      "The name of the NXSPE file to write, as a full or relative path");

  declareProperty("Efixed", EMPTY_DBL(), "Value of the fixed energy to write into NXSPE file.");
  declareProperty("Psi", EMPTY_DBL(), "Value of PSI to write into NXSPE file.");
  declareProperty("KiOverKfScaling", true, "Flags in the file whether Ki/Kf scaling has been done or not.");

  // optional par or phx file
  std::vector<std::string> fileExts{".par", ".phx"};
  declareProperty(
      std::make_unique<FileProperty>("ParFile", "not_used.par", FileProperty::OptionalLoad, fileExts),
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

  // Number of spectra
  const auto nHist = static_cast<int64_t>(inputWS->getNumberHistograms());
  // Number of energy bins
  const auto nBins = static_cast<int64_t>(inputWS->blocksize());

  // Retrieve the filename from the properties
  std::string filename = getPropertyValue("Filename");

  // Create the file.
  ::NeXus::File nxFile(filename, NXACC_CREATE5);

  // Make the top level entry (and open it)
  std::string entryName = getPropertyValue("InputWorkspace");
  if (entryName.empty()) {
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
  bool efixed_provided(!isEmpty(efixed));
  if (!efixed_provided) {
    efixed = MASK_FLAG;
  }
  auto eMode = inputWS->getEMode();
  bool write_single_energy(true);
  if (!efixed_provided) {
    // efixed identified differently for different types of inelastic instruments
    // Now lets check to see if we can retrieve energy from workspace.
    switch (eMode) {
    case (Kernel::DeltaEMode::Indirect): {
      // here workspace detectors should always have the energy
      auto detEfixed = getIndirectEfixed(inputWS);
      if (detEfixed.size() == 1) {
        efixed = detEfixed[0];
      } else if (detEfixed.size() > 1) {
        write_single_energy = false; // do not write single energy, write array of energies here
        nxFile.writeData("fixed_energy", detEfixed);
      }
      if (!write_single_energy || (detEfixed[0] > std::numeric_limits<float>::epsilon()))
        // this is generally incorrect, but following historical practice
        break; // assume that indirect instrument without energy attached to detectors
               // may have energy specified in Ei log. Some user scripts may depend on it.
               // so here we break only if Ei is defined on detectors and look for
               // Ei log otherwise
    }
    case (Kernel::DeltaEMode::Elastic):   // no efixed for elastic,
                                          // whatever retrieved from the property should remain unchanged.
                                          // Despite it is incorrect, previous tests were often using Instrument in
                                          // Elastic mode as the source of data for Direct inelastic. Despite correct
                                          // action would be no efixed for elastic, to keep tests happy here we try to
                                          // derive efixed from Ei log similarly to Direct inelastic case if no external
                                          // efixed provided to the algorithm.
    case (Kernel::DeltaEMode::Undefined): // This should not happen
      eMode = Kernel::DeltaEMode::Direct; // but to keep cpp-check happy and code consistent, set up Direct
                                          // instrument in this case.
    case (Kernel::DeltaEMode::Direct): {
      const API::Run &run = inputWS->run();
      if (run.hasProperty("Ei")) {
        efixed = run.getPropertyValueAsType<double>("Ei");
      }
      break;
    }
    }
  }
  if (write_single_energy) // if multiple energies, they have already been written
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
  nxFile.writeData("run_number", inputWS->getRunNumber());

  if (eMode == Kernel::DeltaEMode::Direct ||
      eMode == Kernel::DeltaEMode::Elastic) { // Elastic is also not entirely correct but
    // to maintain consistency with previous code, assume Direct instrument in Elastic mode.
    // NXfermi_chopper
    nxFile.makeGroup("fermi", "NXfermi_chopper", true);
    nxFile.writeData("energy", efixed);
    nxFile.closeGroup(); // NXfermi_chopper
  } // TODO: Do not know what indirect people want for their instrument,
    // but they certainly do not have Fermi chopper.
    // This is for them to decide what(if) they want here in a future.

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
  const auto &X = inputWS->x(0);
  nxFile.writeData("energy", X.rawData());
  nxFile.openData("energy");
  nxFile.putAttr("units", "meV");
  nxFile.closeData();

  // let's create some blank arrays in the nexus file
  using Dimensions = std::vector<int64_t>;
  Dimensions arrayDims(2);
  arrayDims[0] = nHist;
  arrayDims[1] = nBins;
  nxFile.makeData("data", ::NeXus::FLOAT64, arrayDims, false);
  nxFile.makeData("error", ::NeXus::FLOAT64, arrayDims, false);

  // Add the axes attributes to the data
  nxFile.openData("data");
  nxFile.putAttr("signal", 1);
  nxFile.putAttr("axes", "polar:energy");
  nxFile.closeData();

  // What size slabs are we going to write...
  // Use an intermediate in-memory buffer to reduce the number
  // of calls to putslab, i.e disk writes but still write whole rows
  Dimensions slabStart(2, 0), slabSize(2, 0);
  auto chunkRows = static_cast<Dimensions::value_type>(MAX_CHUNK_SIZE / 8 / nBins);
  if (nHist < chunkRows) {
    chunkRows = nHist;
  }
  // slab size for all but final write
  slabSize[0] = chunkRows;
  slabSize[1] = nBins;

  // Allocate the temporary buffers for the signal and errors
  using Buffer = boost::scoped_array<double>;
  const size_t bufferSize(slabSize[0] * slabSize[1]);
  Buffer signalBuffer(new double[bufferSize]);
  Buffer errorBuffer(new double[bufferSize]);

  // Write the data
  Progress progress(this, 0.0, 1.0, nHist);
  int64_t bufferCounter(0);
  const auto &spectrumInfo = inputWS->spectrumInfo();
  for (int64_t i = 0; i < nHist; ++i) {
    progress.report();

    double *signalBufferStart = signalBuffer.get() + bufferCounter * nBins;
    double *errorBufferStart = errorBuffer.get() + bufferCounter * nBins;
    if (spectrumInfo.hasDetectors(i) && !spectrumInfo.isMonitor(i)) {
      // a detector but not a monitor
      if (!spectrumInfo.isMasked(i)) {
        std::copy(inputWS->y(i).cbegin(), inputWS->y(i).cend(), signalBufferStart);
        std::copy(inputWS->e(i).cbegin(), inputWS->e(i).cend(), errorBufferStart);
      } else {
        std::fill_n(signalBufferStart, nBins, MASK_FLAG);
        std::fill_n(errorBufferStart, nBins, MASK_ERROR);
      }
    } else {
      // no detector gets zeroes.
      std::fill_n(signalBufferStart, nBins, 0.0);
      std::fill_n(errorBufferStart, nBins, 0.0);
    }
    ++bufferCounter;

    // Do we need to flush the buffer. Either we have filled the buffer
    // or we have reached the end of the workspace and not completely filled
    // the buffer
    if (bufferCounter == chunkRows || i == nHist - 1) {
      // techincally only need to update for the very last slab but
      // this is always correct and avoids an if statement
      slabSize[0] = bufferCounter;
      nxFile.openData("data");
      nxFile.putSlab(signalBuffer.get(), slabStart, slabSize);
      nxFile.closeData();

      // Open the error
      nxFile.openData("error");
      nxFile.putSlab(errorBuffer.get(), slabStart, slabSize);
      nxFile.closeData();

      // Reset counters for next time
      slabStart[0] += bufferCounter;
      bufferCounter = 0;
    }
  }

  // execute the algorithm to calculate the detector's parameters;
  auto spCalcDetPar = createChildAlgorithm("FindDetectorsPar", 0, 1, true, 1);

  spCalcDetPar->initialize();
  spCalcDetPar->setProperty("InputWorkspace", inputWS);
  std::string parFileName = this->getPropertyValue("ParFile");
  if (!(parFileName.empty() || parFileName == "not_used.par")) {
    spCalcDetPar->setPropertyValue("ParFile", parFileName);
  }
  spCalcDetPar->execute();

  //
  auto *pCalcDetPar = dynamic_cast<FindDetectorsPar *>(spCalcDetPar.get());
  if (!pCalcDetPar) { // "can not get pointer to FindDetectorsPar algorithm"
    throw(std::bad_cast());
  }
  const std::vector<double> &azimuthal = pCalcDetPar->getAzimuthal();
  const std::vector<double> &polar = pCalcDetPar->getPolar();
  const std::vector<double> &azimuthal_width = pCalcDetPar->getAzimWidth();
  const std::vector<double> &polar_width = pCalcDetPar->getPolarWidth();
  const std::vector<double> &secondary_flightpath = pCalcDetPar->getFlightPath();

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

/**
 * Calculate fixed energy for indirect instrument. Depending on actual workspace, this may be the same energy for all
 * detectors or array of energies for all active detectors if some energies are different.
 *
 * @param inputWS pointer to matrix workspace with indirect instrument attached to it.
 *
 * @returns vector containing analyzer energies for each detector if the energies are different or single energy if they
 * are the same
 */
std::vector<double> SaveNXSPE::getIndirectEfixed(const MatrixWorkspace_sptr &inputWS) const {
  // Number of spectra
  const auto nHist = static_cast<int64_t>(inputWS->getNumberHistograms());
  const auto &spectrumInfo = inputWS->spectrumInfo();
  Mantid::MantidVec AllEnergies(nHist);
  size_t nDet(0);
  UnitParametersMap pmap;
  Units::Time inUnit;
  Units::DeltaE outUnit;
  for (int64_t i = 0; i < nHist; ++i) {
    if (spectrumInfo.hasDetectors(i) && !spectrumInfo.isMonitor(i)) {
      // a detector but not a monitor and not masked for indirect instrument should have efixed
      spectrumInfo.getDetectorValues(inUnit, outUnit, DeltaEMode::Indirect, true, i, pmap);
      AllEnergies[nDet] = pmap[UnitParams::efixed];
      nDet++;
    }
  }
  AllEnergies.resize(nDet);
  if (nDet == 0)
    return AllEnergies; // Empty vector, no energies,

  if (std::all_of(AllEnergies.begin(), AllEnergies.end(), [&AllEnergies](double energy) {
        return std::abs(energy - AllEnergies[0]) < std::numeric_limits<float>::epsilon();
      })) {
    // all detectors have same energy
    AllEnergies.resize(1);
  }

  return AllEnergies;
}
} // namespace Mantid::DataHandling
