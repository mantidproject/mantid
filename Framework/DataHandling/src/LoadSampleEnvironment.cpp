// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidDataHandling/LoadSampleEnvironment.h"
#include "MantidDataHandling/LoadAsciiStl.h"
#include "MantidDataHandling/LoadBinaryStl.h"
#include "MantidGeometry/Instrument/Container.h"
#include "MantidGeometry/Instrument/SampleEnvironment.h"
#include "MantidGeometry/Objects/MeshObject.h"

#include "MantidAPI/FileProperty.h"
#include "MantidAPI/InstrumentValidator.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/Sample.h"

#include "MantidGeometry/Instrument.h"
#include "MantidKernel/Exception.h"

#include <Poco/File.h>
#include <boost/algorithm/string.hpp>
#include <fstream>
#include <stdio.h>
#include <string.h>

namespace Mantid {
namespace DataHandling {

// Register the algorithm into the algorithm factory
DECLARE_ALGORITHM(LoadSampleEnvironment)

using namespace Kernel;
using namespace API;
using namespace Geometry;

void LoadSampleEnvironment::init() {
  auto wsValidator = boost::make_shared<API::InstrumentValidator>();
  ;

  // input workspace
  declareProperty(make_unique<WorkspaceProperty<>>(
                      "InputWorkspace", "", Direction::Input, wsValidator),
                  "The name of the workspace containing the instrument to add "
                  "the Environment");

  // Environment file
  const std::vector<std::string> extensions{".stl"};
  declareProperty(
      make_unique<FileProperty>("Filename", "", FileProperty::Load, extensions),
      "The path name of the file containing the Environment");

  // Output workspace
  declareProperty(make_unique<WorkspaceProperty<>>("OutputWorkspace", "",
                                                   Direction::Output),
                  "The name of the workspace that will contain the loaded "
                  "Environment of the sample");

  // Environment Name
  declareProperty("EnvironmentName", "Environment");

  // New Can or Add
  declareProperty("Add", false);
}

void LoadSampleEnvironment::exec() {

  MatrixWorkspace_const_sptr inputWS = getProperty("InputWorkspace");
  MatrixWorkspace_sptr outputWS = getProperty("OutputWorkspace");

  if (inputWS != outputWS) {
    outputWS = inputWS->clone();
  }

  std::string filename = getProperty("Filename");
  std::ifstream file(filename.c_str());
  if (!file) {
    g_log.error("Unable to open file: " + filename);
    throw Exception::FileError("Unable to open file: ", filename);
  }


  boost::shared_ptr<MeshObject> environmentMesh = nullptr;

  auto asciiStlReader = LoadAsciiStl(filename);
  auto binaryStlReader = LoadBinaryStl(filename);
  if (binaryStlReader.isBinarySTL()) {
    environmentMesh = binaryStlReader.readStl();
  } else if (asciiStlReader.isAsciiSTL()) {
    environmentMesh = asciiStlReader.readStl();
  } else {
    throw Kernel::Exception::ParseError(
        "Could not read file, did not match either STL Format", filename, 0);
  }
  std::string name = getProperty("EnvironmentName");
  bool add = getProperty("Add");
  Sample &sample = outputWS->mutableSample();
  boost::shared_ptr<Geometry::SampleEnvironment> environment = nullptr;
  if (add) {
    environment = boost::make_shared<Geometry::SampleEnvironment>(
        sample.getEnvironment());
    environment->add(environmentMesh);
  } else {
    auto can = boost::make_shared<Container>(environmentMesh);
    environment = boost::make_shared<Geometry::SampleEnvironment>(name, can);
  }
  // Put Environment into sample.
  sample.setEnvironment(environment);

  // Set output workspace
  const std::string debugString =
      "Enviroment has: " + std::to_string(environment->nelements()) +
      " elements.";

  g_log.debug(debugString);
}

} // namespace DataHandling
} // namespace Mantid
