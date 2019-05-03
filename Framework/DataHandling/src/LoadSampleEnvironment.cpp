// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidDataHandling/LoadSampleEnvironment.h"
#include "MantidDataHandling/LoadAsciiStl.h"
#include "MantidDataHandling/LoadBinaryStl.h"
#include "MantidDataHandling/ReadMaterial.h"
#include "MantidGeometry/Instrument/Container.h"
#include "MantidGeometry/Instrument/SampleEnvironment.h"
#include "MantidGeometry/Objects/MeshObject.h"

#include "MantidAPI/FileProperty.h"
#include "MantidAPI/InstrumentValidator.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/Sample.h"

#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/BoundedValidator.h"
#include "MantidKernel/EnabledWhenProperty.h"
#include "MantidKernel/Exception.h"
#include "MantidKernel/ListValidator.h"
#include "MantidKernel/MandatoryValidator.h"

#include <Poco/File.h>
#include <boost/algorithm/string.hpp>
#include <cmath>
#include <fstream>

namespace Mantid {
namespace DataHandling {

namespace {
double DegreesToRadians(double angle) { return angle * M_PI / 180; }
} // namespace

// Register the algorithm into the algorithm factory
DECLARE_ALGORITHM(LoadSampleEnvironment)

using namespace Kernel;
using namespace API;
using namespace Geometry;

void LoadSampleEnvironment::init() {
  auto wsValidator = boost::make_shared<InstrumentValidator>();
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

  // scale to use for stl
  declareProperty("Scale", "cm", "The scale of the stl: m, cm, or mm");

  // Output workspace
  declareProperty(make_unique<WorkspaceProperty<>>("OutputWorkspace", "",
                                                   Direction::Output),
                  "The name of the workspace that will contain the loaded "
                  "Environment of the sample");

  // Environment Name
  declareProperty("EnvironmentName", "Environment");

  // New Can or Add
  declareProperty("Add", false);

  // Vector to translate mesh
  declareProperty(
      make_unique<ArrayProperty<double>>("TranslationVector", "0,0,0"),
      "Vector by which to translate the loaded environment");

  // Rotation angles
  declareProperty("XDegrees", 0.0, "The degrees to rotate on the x axis by");
  declareProperty("YDegrees", 0.0, "The degrees to rotate on the y axis by");
  declareProperty("ZDegrees", 0.0, "The degrees to rotate on the z axis by");

  declareProperty("SetMaterial", false);

  // properties for SetMaterial

  declareProperty("ChemicalFormula", "",
                  "The chemical formula, see examples in documentation");

  declareProperty("AtomicNumber", 0, "The atomic number");
  declareProperty("MassNumber", 0,
                  "Mass number if ion (use 0 for default mass sensity)");
  auto mustBePositive = boost::make_shared<BoundedValidator<double>>();
  mustBePositive->setLower(0.0);
  declareProperty("SampleNumberDensity", EMPTY_DBL(), mustBePositive,
                  "This number density of the sample in number of "
                  "atoms per cubic angstrom will be used instead of "
                  "calculated");
  declareProperty("ZParameter", EMPTY_DBL(), mustBePositive,
                  "Number of formula units in unit cell");
  declareProperty("UnitCellVolume", EMPTY_DBL(), mustBePositive,
                  "Unit cell volume in Angstoms^3. Will be calculated from the "
                  "OrientedLattice if not supplied.");
  declareProperty("CoherentXSection", EMPTY_DBL(), mustBePositive,
                  "Optional:  This coherent cross-section for the sample "
                  "material in barns will be used instead of tabulated");
  declareProperty("IncoherentXSection", EMPTY_DBL(), mustBePositive,
                  "Optional:  This incoherent cross-section for the sample "
                  "material in barns will be used instead of tabulated");
  declareProperty("AttenuationXSection", EMPTY_DBL(), mustBePositive,
                  "Optional:  This absorption cross-section for the sample "
                  "material in barns will be used instead of tabulated");
  declareProperty("ScatteringXSection", EMPTY_DBL(), mustBePositive,
                  "Optional:  This total scattering cross-section (coherent + "
                  "incoherent) for the sample material in barns will be used "
                  "instead of tabulated");
  declareProperty("SampleMassDensity", EMPTY_DBL(), mustBePositive,
                  "Measured mass density in g/cubic cm of the sample "
                  "to be used to calculate the number density.");
  const std::vector<std::string> units({"Atoms", "Formula Units"});
  declareProperty("NumberDensityUnit", units.front(),
                  boost::make_shared<StringListValidator>(units),
                  "Choose which units SampleNumberDensity referes to.");

  // Perform Group Associations.
  std::string formulaGrp("By Formula or Atomic Number");
  setPropertyGroup("ChemicalFormula", formulaGrp);
  setPropertyGroup("AtomicNumber", formulaGrp);
  setPropertyGroup("MassNumber", formulaGrp);
  setPropertySettings("ChemicalFormula", make_unique<EnabledWhenProperty>(
                                             "SetMaterial", IS_NOT_DEFAULT));
  setPropertySettings("AtomicNumber", make_unique<EnabledWhenProperty>(
                                          "SetMaterial", IS_NOT_DEFAULT));
  setPropertySettings("MassNumber", make_unique<EnabledWhenProperty>(
                                        "SetMaterial", IS_NOT_DEFAULT));

  std::string densityGrp("Sample Density");
  setPropertyGroup("SampleNumberDensity", densityGrp);
  setPropertyGroup("NumberDensityUnit", densityGrp);
  setPropertyGroup("ZParameter", densityGrp);
  setPropertyGroup("UnitCellVolume", densityGrp);
  setPropertyGroup("SampleMassDensity", densityGrp);
  setPropertySettings(
      "SampleNumberDensity",
      make_unique<EnabledWhenProperty>("SetMaterial", IS_NOT_DEFAULT));
  setPropertySettings("ZParameter", make_unique<EnabledWhenProperty>(
                                        "SetMaterial", IS_NOT_DEFAULT));
  setPropertySettings("UnitCellVolume", make_unique<EnabledWhenProperty>(
                                            "SetMaterial", IS_NOT_DEFAULT));
  setPropertySettings("SampleMassDensity", make_unique<EnabledWhenProperty>(
                                               "SetMaterial", IS_NOT_DEFAULT));
  setPropertySettings(
      "NumberDensityUnit",
      make_unique<EnabledWhenProperty>("SampleNumberDensity", IS_NOT_DEFAULT));

  std::string specificValuesGrp("Override Cross Section Values");
  setPropertyGroup("CoherentXSection", specificValuesGrp);
  setPropertyGroup("IncoherentXSection", specificValuesGrp);
  setPropertyGroup("AttenuationXSection", specificValuesGrp);
  setPropertyGroup("ScatteringXSection", specificValuesGrp);
  setPropertySettings("CoherentXSection", make_unique<EnabledWhenProperty>(
                                              "SetMaterial", IS_NOT_DEFAULT));
  setPropertySettings("IncoherentXSection", make_unique<EnabledWhenProperty>(
                                                "SetMaterial", IS_NOT_DEFAULT));
  setPropertySettings(
      "AttenuationXSection",
      make_unique<EnabledWhenProperty>("SetMaterial", IS_NOT_DEFAULT));
  setPropertySettings("ScatteringXSection", make_unique<EnabledWhenProperty>(
                                                "SetMaterial", IS_NOT_DEFAULT));
}

std::map<std::string, std::string> LoadSampleEnvironment::validateInputs() {
  std::map<std::string, std::string> result;
  if (getProperty("SetMaterial")) {
    ReadMaterial::MaterialParameters params;
    params.chemicalSymbol = getPropertyValue("ChemicalFormula");
    params.atomicNumber = getProperty("AtomicNumber");
    params.massNumber = getProperty("MassNumber");
    params.sampleNumberDensity = getProperty("SampleNumberDensity");
    params.zParameter = getProperty("ZParameter");
    params.unitCellVolume = getProperty("UnitCellVolume");
    params.sampleMassDensity = getProperty("SampleMassDensity");
    result = ReadMaterial::validateInputs(params);
  }
  return result;
}

void LoadSampleEnvironment::exec() {

  MatrixWorkspace_const_sptr inputWS = getProperty("InputWorkspace");
  MatrixWorkspace_sptr outputWS = getProperty("OutputWorkspace");

  if (inputWS != outputWS) {
    outputWS = inputWS->clone();
  }

  const std::string filename = getProperty("Filename");
  const std::ifstream file(filename.c_str());
  if (!file) {
    g_log.error("Unable to open file: " + filename);
    throw Exception::FileError("Unable to open file: ", filename);
  }

  boost::shared_ptr<MeshObject> environmentMesh = nullptr;

  std::unique_ptr<LoadAsciiStl> asciiStlReader = nullptr;
  std::unique_ptr<LoadBinaryStl> binaryStlReader = nullptr;
  const std::string scaleProperty = getPropertyValue("Scale");
  const ScaleUnits scaleType = getScaleType(scaleProperty);

  if (getProperty("SetMaterial")) {
    ReadMaterial::MaterialParameters params;
    params.chemicalSymbol = getPropertyValue("ChemicalFormula");
    params.atomicNumber = getProperty("AtomicNumber");
    params.massNumber = getProperty("MassNumber");
    params.sampleNumberDensity = getProperty("SampleNumberDensity");
    params.zParameter = getProperty("ZParameter");
    params.unitCellVolume = getProperty("UnitCellVolume");
    params.sampleMassDensity = getProperty("SampleMassDensity");
    params.coherentXSection = getProperty("CoherentXSection");
    params.incoherentXSection = getProperty("IncoherentXSection");
    params.attenuationXSection = getProperty("AttenuationXSection");
    params.scatteringXSection = getProperty("ScatteringXSection");
    const std::string numberDensityUnit = getProperty("NumberDensityUnit");
    if (numberDensityUnit == "Atoms") {
      params.numberDensityUnit = MaterialBuilder::NumberDensityUnit::Atoms;
    } else {
      params.numberDensityUnit =
          MaterialBuilder::NumberDensityUnit::FormulaUnits;
    }
    binaryStlReader =
        std::make_unique<LoadBinaryStl>(filename, scaleType, params);
    asciiStlReader =
        std::make_unique<LoadAsciiStl>(filename, scaleType, params);
  } else {
    binaryStlReader = std::make_unique<LoadBinaryStl>(filename, scaleType);
    asciiStlReader = std::make_unique<LoadAsciiStl>(filename, scaleType);
  }

  if (binaryStlReader->isBinarySTL(filename)) {
    environmentMesh = binaryStlReader->readStl();
  } else if (asciiStlReader->isAsciiSTL(filename)) {
    environmentMesh = asciiStlReader->readStl();
  } else {
    throw Exception::ParseError(
        "Could not read file, did not match either STL Format", filename, 0);
  }
  environmentMesh = translate(environmentMesh);
  environmentMesh = rotate(environmentMesh);

  std::string name = getProperty("EnvironmentName");
  const bool add = getProperty("Add");
  Sample &sample = outputWS->mutableSample();
  std::unique_ptr<SampleEnvironment> environment = nullptr;
  if (add) {
    environment = std::make_unique<SampleEnvironment>(sample.getEnvironment());
    environment->add(environmentMesh);
  } else {
    auto can = boost::make_shared<Container>(environmentMesh);
    environment = std::make_unique<SampleEnvironment>(name, can);
  }
  // Put Environment into sample.

  std::string debugString =
      "Environment has: " + std::to_string(environment->nelements()) +
      " elements.";

  sample.setEnvironment(std::move(environment));

  auto translatedVertices = environmentMesh->getVertices();
  if (g_log.is(Logger::Priority::PRIO_DEBUG)) {
    int i = 0;
    for (double vertex : translatedVertices) {
      i++;
      g_log.debug(std::to_string(vertex));
      if (i % 3 == 0) {
        g_log.debug("\n");
      }
    }
  }
  // get the material name and number density for debug
  const auto outMaterial =
      outputWS->sample().getEnvironment().getContainer().material();
  debugString += "\n"
                 "Environment Material: " +
                 outMaterial.name();
  debugString += "\n"
                 "Environment Material Number Density: " +
                 std::to_string(outMaterial.numberDensity());
  // Set output workspace
  setProperty("OutputWorkspace", outputWS);
  g_log.debug(debugString);
}

/**
 * translates the environment by a provided matrix
 * @param environmentMesh The environment to translate
 * @returns a shared pointer to the newly translated environment
 */
boost::shared_ptr<MeshObject> LoadSampleEnvironment::translate(
    boost::shared_ptr<MeshObject> environmentMesh) {
  const std::vector<double> translationVector =
      getProperty("TranslationVector");
  std::vector<double> checkVector = std::vector<double>(3, 0.0);
  if (translationVector != checkVector) {
    if (translationVector.size() != 3) {
      throw std::invalid_argument(
          "Invalid Translation vector, must have exactly 3 dimensions");
    }
    V3D translate =
        V3D(translationVector[0], translationVector[1], translationVector[2]);
    environmentMesh->translate(translate);
  }
  return environmentMesh;
}

/**
 * Rotates the environment by a generated matrix
 * @param environmentMesh The environment to rotate
 * @returns a shared pointer to the newly rotated environment
 */
boost::shared_ptr<MeshObject>
LoadSampleEnvironment::rotate(boost::shared_ptr<MeshObject> environmentMesh) {
  const std::vector<double> rotationMatrix = generateMatrix();
  environmentMesh->rotate(rotationMatrix);
  return environmentMesh;
}

/**
 * Generates a rotation Matrix applying the x rotation then y rotation, then z rotation
 * @returns a matrix of doubles to use as the rotation matrix
 */
Matrix<double> LoadSampleEnvironment::generateMatrix() {
  Kernel::Matrix<double> xMatrix = generateXRotation();
  Kernel::Matrix<double> yMatrix = generateYRotation();
  Kernel::Matrix<double> zMatrix = generateZRotation();

  return zMatrix * yMatrix * xMatrix;
}

/**
 * Generates the x component of the rotation matrix
 * using the xDegrees Property.
 * @returns a matrix of doubles to use as the x axis rotation matrix
 */
Matrix<double> LoadSampleEnvironment::generateXRotation() {
  const double xRotation = getProperty("xDegrees");
  const double sinX = sin(DegreesToRadians(xRotation));
  const double cosX = cos(DegreesToRadians(xRotation));
  std::vector<double> matrixList = {1, 0, 0, 0, cosX, -sinX, 0, sinX, cosX};
  return Kernel::Matrix<double>(matrixList);
}

/**
 * Generates the y component of the rotation matrix
 * using the yDegrees Property.
 * @returns a matrix of doubles to use as the y axis rotation matrix
 */
Matrix<double> LoadSampleEnvironment::generateYRotation() {
  const double yRotation = getProperty("yDegrees");
  const double sinY = sin(DegreesToRadians(yRotation));
  const double cosY = cos(DegreesToRadians(yRotation));
  std::vector<double> matrixList = {cosY, 0, sinY, 0, 1, 0, -sinY, 0, cosY};
  return Kernel::Matrix<double>(matrixList);
}

/**
 * Generates the z component of the rotation matrix
 * using the zDegrees Property.
 * @returns a matrix of doubles to use as the z axis rotation matrix
 */
Matrix<double> LoadSampleEnvironment::generateZRotation() {
  const double zRotation = getProperty("zDegrees");
  const double sinZ = sin(DegreesToRadians(zRotation));
  const double cosZ = cos(DegreesToRadians(zRotation));
  std::vector<double> matrixList = {cosZ, -sinZ, 0, sinZ, cosZ, 0, 0, 0, 1};
  return Kernel::Matrix<double>(matrixList);
}

} // namespace DataHandling
} // namespace Mantid
