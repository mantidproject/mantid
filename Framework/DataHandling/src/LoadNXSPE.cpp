// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidDataHandling/LoadNXSPE.h"
#include "MantidAPI/ExperimentInfo.h"
#include "MantidAPI/FileProperty.h"
#include "MantidAPI/InstrumentFileFinder.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/RegisterFileLoader.h"
#include "MantidAPI/Run.h"
#include "MantidAPI/SpectraAxis.h"
#include "MantidAPI/SpectrumInfo.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidGeometry/Instrument.h"
#include "MantidGeometry/Instrument/Detector.h"
#include "MantidGeometry/Instrument/Goniometer.h"
#include "MantidGeometry/Objects/ShapeFactory.h"
#include "MantidKernel/DeltaEMode.h"
#include "MantidKernel/UnitFactory.h"
#include "MantidNexus/NexusClasses.h"
#include "MantidNexusCpp/NeXusException.hpp"
#include "MantidNexusCpp/NeXusFile.hpp"

#include <boost/regex.hpp>

#include <Poco/File.h>

#include <map>
#include <sstream>
#include <string>
#include <vector>

namespace Mantid::DataHandling {

DECLARE_NEXUS_HDF5_FILELOADER_ALGORITHM(LoadNXSPE)

using namespace Mantid::Kernel;
using namespace Mantid::API;
using Mantid::HistogramData::BinEdges;

/**
 * Calculate the confidence in the string value. This is used for file
 * identification.
 * @param value
 * @return confidence 0 - 100%
 */
int LoadNXSPE::identiferConfidence(const std::string &value) {
  int identifierConfidence = 0;
  if (value == "NXSPE") {
    identifierConfidence = 99;
  } else {
    boost::regex re("^NXSP", boost::regex::icase);
    if (boost::regex_match(value, re)) {
      identifierConfidence = 95;
    }
  }
  return identifierConfidence;
}

/**
 * Return the confidence with with this algorithm can load the file
 * @param descriptor A descriptor for the file
 * @returns An integer specifying the confidence level. 0 indicates it will not
 * be used
 */
int LoadNXSPE::confidence(Kernel::NexusHDF5Descriptor &descriptor) const {
  int confidence(0);
  using string_map_t = std::map<std::string, std::string>;
  try {
    ::NeXus::File file = ::NeXus::File(descriptor.filename());
    string_map_t entries = file.getEntries();
    for (string_map_t::const_iterator it = entries.begin(); it != entries.end(); ++it) {
      if (it->second == "NXentry") {
        file.openGroup(it->first, it->second);
        file.openData("definition");
        const std::string value = file.getStrData();
        confidence = identiferConfidence(value);
      }
    }
  } catch (::NeXus::Exception &) {
  }
  return confidence;
}

/** Initialize the algorithm's properties.
 */
void LoadNXSPE::init() {
  const std::vector<std::string> exts{".nxspe", ""};
  declareProperty(std::make_unique<FileProperty>("Filename", "", FileProperty::Load, exts), "An NXSPE file");
  declareProperty(std::make_unique<WorkspaceProperty<>>("OutputWorkspace", "", Direction::Output),
                  "The name of the workspace that will be created.");
}

/** Execute the algorithm.
 */
void LoadNXSPE::exec() {
  std::string filename = getProperty("Filename");
  // quicly check if it's really nxspe
  try {
    ::NeXus::File file(filename);
    std::string mainEntry = (*(file.getEntries().begin())).first;
    file.openGroup(mainEntry, "NXentry");
    file.openData("definition");
    if (identiferConfidence(file.getStrData()) < 1) {
      throw std::invalid_argument("Not NXSPE");
    }
    file.close();
  } catch (...) {
    throw std::invalid_argument("Not NeXus or not NXSPE");
  }

  // Load the data
  ::NeXus::File file(filename);

  std::string mainEntry = (*(file.getEntries().begin())).first;
  file.openGroup(mainEntry, "NXentry");

  file.openGroup("NXSPE_info", "NXcollection");
  std::map<std::string, std::string> entries = file.getEntries();
  std::vector<double> temporary;
  double fixed_energy, psi = 0.;

  if (!entries.count("fixed_energy")) {
    throw std::invalid_argument("fixed_energy field was not found");
  }
  file.openData("fixed_energy");
  file.getData(temporary);
  fixed_energy = temporary.at(0);
  file.closeData();

  if (entries.count("psi")) {
    file.openData("psi");
    file.getData(temporary);
    psi = temporary.at(0);
    if (std::isnan(psi)) {
      psi = 0.;
      g_log.warning("Entry for PSI is empty, will use default of 0.0 instead.");
    }
    file.closeData();
  }

  int kikfscaling = 0;
  if (entries.count("ki_over_kf_scaling")) {
    file.openData("ki_over_kf_scaling");
    std::vector<int> temporaryint;
    file.getData(temporaryint);
    kikfscaling = temporaryint.at(0);
    file.closeData();
  }

  file.closeGroup(); // NXSPE_Info

  file.openGroup("data", "NXdata");
  entries = file.getEntries();

  if (!entries.count("data")) {
    throw std::invalid_argument("data field was not found");
  }
  file.openData("data");
  ::NeXus::Info info = file.getInfo();
  auto numSpectra = static_cast<std::size_t>(info.dims.at(0));
  auto numBins = static_cast<std::size_t>(info.dims.at(1));
  std::vector<double> data;
  file.getData(data);
  file.closeData();

  if (!entries.count("error")) {
    throw std::invalid_argument("error field was not found");
  }
  file.openData("error");
  std::vector<double> error;
  file.getData(error);
  file.closeData();

  if (!entries.count("energy")) {
    throw std::invalid_argument("energy field was not found");
  }
  file.openData("energy");
  std::vector<double> energies;
  file.getData(energies);
  file.closeData();

  if (!entries.count("azimuthal")) {
    throw std::invalid_argument("azimuthal field was not found");
  }
  file.openData("azimuthal");
  std::vector<double> azimuthal;
  file.getData(azimuthal);
  file.closeData();

  if (!entries.count("azimuthal_width")) {
    throw std::invalid_argument("azimuthal_width field was not found");
  }
  file.openData("azimuthal_width");
  std::vector<double> azimuthal_width;
  file.getData(azimuthal_width);
  file.closeData();

  if (!entries.count("polar")) {
    throw std::invalid_argument("polar field was not found");
  }
  file.openData("polar");
  std::vector<double> polar;
  file.getData(polar);
  file.closeData();

  if (!entries.count("polar_width")) {
    throw std::invalid_argument("polar_width field was not found");
  }
  file.openData("polar_width");
  std::vector<double> polar_width;
  file.getData(polar_width);
  file.closeData();

  // distance might not have been saved in all NXSPE files
  std::vector<double> distance;
  if (entries.count("distance")) {
    file.openData("distance");
    file.getData(distance);
    file.closeData();
  }

  file.closeGroup(); // data group

  file.openGroup("instrument", "NXinstrument");
  entries = file.getEntries();
  std::string instrument_name;
  if (entries.count("name")) {
    file.openData("name");
    instrument_name = file.getStrData();
    file.closeData();
  }
  file.closeGroup(); // instrument group

  file.closeGroup(); // Main entry
  file.close();

  // check if dimensions of the vectors are correct
  if ((error.size() != data.size()) || (azimuthal.size() != numSpectra) || (azimuthal_width.size() != numSpectra) ||
      (polar.size() != numSpectra) || (polar_width.size() != numSpectra) ||
      ((energies.size() != numBins) && (energies.size() != numBins + 1))) {
    throw std::invalid_argument("incompatible sizes of fields in the NXSPE file");
  }

  MatrixWorkspace_sptr outputWS = std::dynamic_pointer_cast<MatrixWorkspace>(
      WorkspaceFactory::Instance().create("Workspace2D", numSpectra, energies.size(), numBins));
  // Need to get hold of the parameter map
  outputWS->getAxis(0)->unit() = UnitFactory::Instance().create("DeltaE");
  outputWS->setYUnit("SpectraNumber");

  // add logs
  outputWS->mutableRun().addLogData(new PropertyWithValue<double>("Ei", fixed_energy));
  outputWS->mutableRun().addLogData(new PropertyWithValue<double>("psi", psi));
  outputWS->mutableRun().addLogData(
      new PropertyWithValue<std::string>("ki_over_kf_scaling", kikfscaling == 1 ? "true" : "false"));

  // Set Goniometer
  Geometry::Goniometer gm;
  gm.pushAxis("psi", 0, 1, 0, psi);
  outputWS->mutableRun().setGoniometer(gm, true);

  // generate instrument
  Geometry::Instrument_sptr instrument(new Geometry::Instrument(instrument_name.empty() ? "NXSPE" : instrument_name));

  Geometry::ObjComponent *source = new Geometry::ObjComponent("source");
  source->setPos(0.0, 0.0, -10.);
  instrument->add(source);
  instrument->markAsSource(source);
  Geometry::Component *sample = new Geometry::Component("sample");
  instrument->add(sample);
  instrument->markAsSamplePos(sample);

  constexpr double deg2rad = M_PI / 180.0;

  for (std::size_t i = 0; i < numSpectra; ++i) {
    double r = 1.0;
    if (!distance.empty()) {
      r = distance.at(i);
    }

    Kernel::V3D pos;
    pos.spherical(r, polar.at(i), azimuthal.at(i));
    // Define the size of the detector using the minimum of the polar or azimuthal width (with maximum of 2 deg)
    double rr =
        r * sin(std::min(std::min(std::abs(polar_width.at(i)), std::abs(azimuthal_width.at(i))), 2.0) * deg2rad / 2);

    const auto shape = Geometry::ShapeFactory().createSphere(V3D(0, 0, 0), std::max(rr, 0.01));

    Geometry::Detector *det = new Geometry::Detector("pixel", static_cast<int>(i + 1), sample);
    det->setPos(pos);
    det->setShape(shape);
    instrument->add(det);
    instrument->markAsDetector(det);
  }
  outputWS->setInstrument(instrument);

  std::vector<double>::iterator itdata = data.begin(), iterror = error.begin(), itdataend, iterrorend;
  auto &spectrumInfo = outputWS->mutableSpectrumInfo();
  API::Progress prog = API::Progress(this, 0.0, 0.9, numSpectra);
  BinEdges edges(std::move(energies));
  for (std::size_t i = 0; i < numSpectra; ++i) {
    itdataend = itdata + numBins;
    iterrorend = iterror + numBins;
    outputWS->getSpectrum(i).setDetectorID(static_cast<detid_t>(i + 1));
    outputWS->setBinEdges(i, edges);
    if ((!std::isfinite(*itdata)) || (*itdata <= -1e10)) // masked bin
    {
      spectrumInfo.setMasked(i, true);
    } else {
      outputWS->mutableY(i).assign(itdata, itdataend);
      outputWS->mutableE(i).assign(iterror, iterrorend);
    }
    itdata = (itdataend);
    iterror = (iterrorend);
    prog.report();
  }

  // If an instrument name is defined, load instrument parameter file for Emode
  // NB. LoadParameterFile must be used on a workspace with an instrument
  if (!instrument_name.empty() && instrument_name != "NXSPE") {
    std::string IDF_filename = InstrumentFileFinder::getInstrumentFilename(instrument_name);
    std::string instrument_parfile = IDF_filename.substr(0, IDF_filename.find("_Definition")) + "_Parameters.xml";
    if (Poco::File(instrument_parfile).exists()) {
      try {
        auto loadParamAlg = createChildAlgorithm("LoadParameterFile");
        loadParamAlg->setProperty("Filename", instrument_parfile);
        loadParamAlg->setProperty("Workspace", outputWS);
        loadParamAlg->execute();
      } catch (...) {
        g_log.information("Cannot load the instrument parameter file.");
      }
    }
  }
  // For NXSPE files generated by Mantid data is actually a distribution.
  outputWS->setDistribution(true);
  setProperty("OutputWorkspace", outputWS);
}

} // namespace Mantid::DataHandling
