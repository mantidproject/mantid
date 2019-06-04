// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidAlgorithms/CopySample.h"
#include "MantidAPI/IMDEventWorkspace.h"
#include "MantidGeometry/Crystal/OrientedLattice.h"
#include "MantidGeometry/Instrument/SampleEnvironment.h"
#include "MantidKernel/EnabledWhenProperty.h"
#include "MantidKernel/Material.h"
#include "MantidKernel/System.h"
namespace Mantid {
namespace Algorithms {

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(CopySample)

using namespace Mantid::API;
using namespace Mantid::Kernel;
using Geometry::IObject;
using Geometry::SampleEnvironment;

//----------------------------------------------------------------------------------------------
/** Initialize the algorithm's properties.
 */
void CopySample::init() {
  declareProperty(std::make_unique<WorkspaceProperty<Workspace>>(
                      "InputWorkspace", "", Direction::Input),
                  "An input workspace from wich to copy sample information.");
  declareProperty(std::make_unique<WorkspaceProperty<Workspace>>(
                      "OutputWorkspace", "", Direction::InOut),
                  "An output workspace to wich to copy sample information..");
  declareProperty(
      std::make_unique<PropertyWithValue<bool>>("CopyName", true, Direction::Input),
      "Copy the name of the sample");
  declareProperty(std::make_unique<PropertyWithValue<bool>>("CopyMaterial", true,
                                                       Direction::Input),
                  "Copy the material of the sample");
  declareProperty(std::make_unique<PropertyWithValue<bool>>("CopyEnvironment", true,
                                                       Direction::Input),
                  "Copy the sample environment");
  declareProperty(
      std::make_unique<PropertyWithValue<bool>>("CopyShape", true, Direction::Input),
      "Copy the sample shape");
  declareProperty(std::make_unique<PropertyWithValue<bool>>("CopyLattice", true,
                                                       Direction::Input),
                  "Copy the sample oriented lattice");
  declareProperty(std::make_unique<PropertyWithValue<bool>>("CopyOrientationOnly",
                                                       false, Direction::Input),
                  "Copy the U matrix only, if both origin and destination have "
                  "oriented lattices");
  setPropertySettings("CopyOrientationOnly",
                      std::make_unique<Kernel::EnabledWhenProperty>(
                          "CopyLattice", IS_EQUAL_TO, "1"));
  declareProperty(
      std::make_unique<PropertyWithValue<int>>("MDInputSampleNumber", 0,
                                          Direction::Input),
      "The number of the sample to be copied from, for an MD workspace "
      "(starting from 0)");
  declareProperty(std::make_unique<PropertyWithValue<int>>(
                      "MDOutputSampleNumber", EMPTY_INT(), Direction::Input),
                  "The number of the sample to be copied to for an MD "
                  "workspace (starting from 0). No number, or negative number, "
                  "means that it will copy to all samples");
}

std::map<std::string, std::string> CopySample::validateInputs() {
  std::map<std::string, std::string> result;
  const bool copyLattice = getProperty("CopyLattice");
  const bool copyOrientationOnly = getProperty("CopyOrientationOnly");
  if (copyOrientationOnly && !copyLattice) {
    result["CopyLattice"] =
        "Need to check CopyLattice if CopyOrientationOnly is checked";
  }
  return result;
}

//----------------------------------------------------------------------------------------------
/** Execute the algorithm.
 */
void CopySample::exec() {
  Workspace_sptr inWS = this->getProperty("InputWorkspace");
  Workspace_sptr outWS = this->getProperty("OutputWorkspace");

  Sample sample;
  // get input sample
  MultipleExperimentInfos_sptr inMDWS =
      boost::dynamic_pointer_cast<MultipleExperimentInfos>(inWS);
  if (inMDWS != nullptr) // it is an MD workspace
  {
    int inputSampleNumber = getProperty("MDInputSampleNumber");
    if (inputSampleNumber < 0) {
      g_log.warning()
          << "Number less then 0. Will use sample number 0 instead\n";
      inputSampleNumber = 0;
    }
    if (static_cast<uint16_t>(inputSampleNumber) >
        (inMDWS->getNumExperimentInfo() - 1)) {
      g_log.warning()
          << "Number greater than the number of last sample in the workspace ("
          << (inMDWS->getNumExperimentInfo() - 1)
          << "). Will use sample number 0 instead\n";
      inputSampleNumber = 0;
    }
    sample = inMDWS->getExperimentInfo(static_cast<uint16_t>(inputSampleNumber))
                 ->sample();
  } else // peaks workspace or matrix workspace
  {
    ExperimentInfo_sptr ei = boost::dynamic_pointer_cast<ExperimentInfo>(inWS);
    if (!ei)
      throw std::invalid_argument("Wrong type of input workspace");
    sample = ei->sample();
  }

  bool copyName = getProperty("CopyName");
  bool copyMaterial = getProperty("CopyMaterial");
  bool copyEnvironment = getProperty("CopyEnvironment");
  bool copyShape = getProperty("CopyShape");
  bool copyLattice = getProperty("CopyLattice");
  bool copyOrientation = getProperty("CopyOrientationOnly");

  // Sample copy;
  MultipleExperimentInfos_sptr outMDWS =
      boost::dynamic_pointer_cast<MultipleExperimentInfos>(outWS);
  if (outMDWS != nullptr) {
    int outputSampleNumber = getProperty("MDOutputSampleNumber");
    if ((outputSampleNumber == EMPTY_INT()) ||
        (outputSampleNumber < 0)) // copy to all samples
    {
      for (uint16_t i = 0; i < outMDWS->getNumExperimentInfo(); i++)
        copyParameters(sample, outMDWS->getExperimentInfo(i)->mutableSample(),
                       copyName, copyMaterial, copyEnvironment, copyShape,
                       copyLattice, copyOrientation);
    } else // copy to a single sample
    {
      if (static_cast<uint16_t>(outputSampleNumber) >
          (outMDWS->getNumExperimentInfo() - 1)) {
        g_log.warning() << "Number greater than the number of last sample in "
                           "the workspace ("
                        << (outMDWS->getNumExperimentInfo() - 1)
                        << "). Will use sample number 0 instead\n";
        outputSampleNumber = 0;
      }
      copyParameters(
          sample,
          outMDWS->getExperimentInfo(static_cast<uint16_t>(outputSampleNumber))
              ->mutableSample(),
          copyName, copyMaterial, copyEnvironment, copyShape, copyLattice,
          copyOrientation);
    }
  } else // peaks workspace or matrix workspace
  {
    ExperimentInfo_sptr ei = boost::dynamic_pointer_cast<ExperimentInfo>(outWS);
    if (!ei)
      throw std::invalid_argument("Wrong type of output workspace");
    copyParameters(sample, ei->mutableSample(), copyName, copyMaterial,
                   copyEnvironment, copyShape, copyLattice, copyOrientation);
  }
  this->setProperty("OutputWorkspace", outWS);
}

void CopySample::copyParameters(Sample &from, Sample &to, bool nameFlag,
                                bool materialFlag, bool environmentFlag,
                                bool shapeFlag, bool latticeFlag,
                                bool orientationOnlyFlag) {
  if (nameFlag)
    to.setName(from.getName());
  if (environmentFlag) {
    to.setEnvironment(
        std::make_unique<SampleEnvironment>(from.getEnvironment()));
  }
  if (shapeFlag) {
    Material rhsMaterial;
    if (materialFlag) {
      rhsMaterial = from.getMaterial();
    } else {
      // Reset to lhs material
      rhsMaterial = to.getMaterial();
    }
    auto rhsObject = boost::shared_ptr<IObject>(
        from.getShape().cloneWithMaterial(rhsMaterial));
    to.setShape(rhsObject);
    to.setGeometryFlag(from.getGeometryFlag());
    to.setHeight(from.getHeight());
    to.setThickness(from.getThickness());
    to.setWidth(from.getWidth());
  } else if (materialFlag) {
    auto lhsObject = boost::shared_ptr<IObject>(
        to.getShape().cloneWithMaterial(from.getMaterial()));
    to.setShape(lhsObject);
  }

  if ((latticeFlag) && from.hasOrientedLattice()) {
    if (to.hasOrientedLattice() && orientationOnlyFlag) {
      to.getOrientedLattice().setU(from.getOrientedLattice().getU());
    } else {
      to.setOrientedLattice(&from.getOrientedLattice());
    }
  }
}

} // namespace Algorithms
} // namespace Mantid
