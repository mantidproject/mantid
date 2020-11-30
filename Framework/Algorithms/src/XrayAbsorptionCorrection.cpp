// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidAlgorithms/XrayAbsorptionCorrection.h"
#include "MantidAPI/Axis.h"
#include "MantidAPI/Sample.h"
#include "MantidAPI/SpectrumInfo.h"
#include "MantidAPI/WorkspaceUnitValidator.h"
#include "MantidKernel/BoundedValidator.h"
#include "MantidKernel/CompositeValidator.h"
#include "MantidKernel/Material.h"
#include "MantidKernel/PhysicalConstants.h"
#include "MantidKernel/VectorHelper.h"
#include <iostream>
#include <math.h>
#include <numeric>

using namespace Mantid::API;
using namespace Mantid::Geometry;
using namespace Mantid::Kernel;
using Mantid::DataObjects::Workspace2D;
namespace PhysicalConstants = Mantid::PhysicalConstants;

/// @cond
namespace {

constexpr double DEFAULT_ANGLE = 45.0;
constexpr double DEFAULT_DETECTOR_DISTANCE = 10.0;

} // namespace
/// @endcond

namespace Mantid {
namespace Algorithms {

DECLARE_ALGORITHM(XrayAbsorptionCorrection)

/**
 * Initialize the algorithm
 */
void XrayAbsorptionCorrection::init() {

  auto wsValidator = std::make_shared<CompositeValidator>();

  declareProperty(std::make_unique<WorkspaceProperty<>>(
                      "InputWorkspace", "", Direction::Input, wsValidator),
                  "The name of the input workspace.");

  declareProperty(
      std::make_unique<WorkspaceProperty<>>("MuonImplantationProfile", "",
                                            Direction::Input, wsValidator),
      "The name of the Muon Implantation Profile.");

  declareProperty(std::make_unique<WorkspaceProperty<>>("OutputWorkspace", "",
                                                        Direction::Output),
                  "The name to use for the output workspace.");

  auto positiveDouble = std::make_shared<Kernel::BoundedValidator<double>>();
  positiveDouble->setLower(0);
  declareProperty("DetectorAngle", DEFAULT_ANGLE, positiveDouble,
                  "Angle between beam and Detector."
                  "Range of normal values for detectors are :  "
                  "Ge1 : 90-180 , Ge2 : 270-360 , Ge3 : 0 - 90 , Ge4 "
                  ": 180 -270.",
                  Direction::Input);

  declareProperty("DetectorDistance", DEFAULT_DETECTOR_DISTANCE, positiveDouble,
                  "Distance between detector and sample.", Direction::Input);
}

std::map<std::string, std::string> XrayAbsorptionCorrection::validateInputs() {
  MatrixWorkspace_sptr inputWS = getProperty("InputWorkspace");
  MatrixWorkspace_sptr muonProfile = getProperty("MuonImplantationProfile");
  std::map<std::string, std::string> issues;
  if (!inputWS->sample().getShape().hasValidShape()) {
    issues["InputWorkspace"] = "Input workspace does not have a Sample";
  } else {
    auto material = inputWS->sample().getShape().material();
    if (!material.hasValidxRayAttenuationOverride()) {
      issues["InputWorkspace"] =
          "Input workspace does not have a Xray Attenuation profile";
    } else {
      if (muonProfile->getNumberHistograms() != 1) {
        issues["MuonImplantationProfile"] =
            "Muon Implantation profile must have only one spectrum";
      }
    }
  }
  return issues;
}

/**
 * Converts angles in degrees to angles in radians.
 * @param degrees double of angle in degrees
 * @return A double of angle in radians
 */
double XrayAbsorptionCorrection::degreesToRadians(double degrees) {
  double radians = M_PI * (degrees / 180);
  return radians;
}

/**
 * Calculates the position of the detector.
 * @param detectorAngle double of angle of detector
 * @param detectorDistance distance betweeen sample and detector
 * @return V3D object of position of detector
 */
Kernel::V3D
XrayAbsorptionCorrection::calculateDetectorPos(double detectorAngle,
                                               double detectorDistance) {
  detectorDistance = detectorDistance / 100;
  double x = detectorDistance / std::tan(degreesToRadians(detectorAngle));
  if (detectorAngle > 180) {
    detectorDistance = -detectorDistance;
  }
  Kernel::V3D detectorPos = {x, detectorDistance, 0.};
  return detectorPos;
}

/**
 * normalise moun intensity to 1.
 * @param muonIntensity A MantidVec which contains the intensity of muons as a
 * function  of depth
 * @return A vector of doubles which contains the normalised muon intensity
 */
std::vector<double> XrayAbsorptionCorrection::normaliseMuonIntensity(
    MantidVec const &muonIntensity) {
  double sum_of_elems{0};
  for (double elem : muonIntensity) {
    sum_of_elems += elem;
  }

  std::vector<double> normalisedMuonIntensity;
  for (double x : muonIntensity) {
    normalisedMuonIntensity.push_back(x / sum_of_elems);
  }
  return normalisedMuonIntensity;
}

/**
 * Calculate the muon implantation position in the sample.
 * @param muonProfile A reference to the muon profile workspace
 * @param inputWS A reference to the input workspace
 * @param detectorDistance a double representing the distance from the sample to
 * detector
 * @return A vector of V3D objects that represent the position of muons
 */
std::vector<Kernel::V3D> XrayAbsorptionCorrection::calculateMuonPos(
    API::MatrixWorkspace_sptr &muonProfile, API::MatrixWorkspace_sptr inputWS,
    double detectorDistance) {
  const MantidVec muonDepth = muonProfile->readX(0);
  Kernel::V3D muonPoint = {0., detectorDistance, 0.};
  Kernel::V3D toStart = {0, -1, 0};
  const Geometry::IObject *shape = &inputWS->sample().getShape();
  Geometry::Track muonPath = Geometry::Track(muonPoint, toStart);
  shape->interceptSurface(muonPath);
  if (muonPath.count() == 0) {
    throw std::runtime_error("No valid solution, check shape parameters, Muon "
                             "depth profile and detector distance");
  }
  Kernel::V3D intersection = muonPath.cbegin()->entryPoint;
  double sampleDepth = intersection[1];
  std::vector<Kernel::V3D> muonPos;
  for (auto depth : muonDepth) {
    Kernel::V3D pos = {0., sampleDepth - (depth / 100), 0.};
    muonPos.push_back(pos);
  }
  return muonPos;
}

/**
 * Calculate the mid point of a bin in workspace.
 * @param energyVector a MantidVec which contains edges of bins
 * @return A vector of doubles of bin mid points
 */
std::vector<double> XrayAbsorptionCorrection::calculateMidPointOfBin(
    MantidVec const &energyVector) {
  std::vector<double> midPoint;
  for (size_t i = 0; i < energyVector.size() - 1; i++) {
    midPoint.push_back((energyVector[i] + energyVector[i + 1]) / 2);
  }
  return midPoint;
}
/**
 * Execution
 */
void XrayAbsorptionCorrection::exec() {
  MatrixWorkspace_sptr inputWS = getProperty("InputWorkspace");
  MatrixWorkspace_sptr outputWS = inputWS->clone();
  MatrixWorkspace_sptr muonProfile = getProperty("MuonImplantationProfile");
  MantidVec muonIntensity = muonProfile->readY(0);
  std::vector<double> normalisedMuonIntensity =
      normaliseMuonIntensity(muonIntensity);
  double detectorAngle = getProperty("DetectorAngle");
  double detectorDistance = getProperty("DetectorDistance");
  Kernel::V3D detectorPos =
      calculateDetectorPos(detectorAngle, detectorDistance);
  std::vector<Kernel::V3D> muonPos =
      calculateMuonPos(muonProfile, inputWS, detectorDistance);
  for (size_t j = 0; j < inputWS->getNumberHistograms(); j++) {
    std::vector<double> energyVector =
        calculateMidPointOfBin(inputWS->readX(j));
    auto &yData = outputWS->mutableY(j);
    for (size_t i = 0; i < energyVector.size(); i++) {
      double totalFactor{0};
      for (size_t k = 0; k < normalisedMuonIntensity.size(); k++) {
        Kernel::V3D pos = muonPos[k];
        Kernel::V3D detectorDirection = normalize(detectorPos - pos);
        Geometry::Track xrayPath = Geometry::Track(pos, detectorDirection);
        const Geometry::IObject *sampleShape = &inputWS->sample().getShape();
        sampleShape->interceptSurface(xrayPath);
        double factor{1.0};
        if (xrayPath.count() == 0) {
          throw std::runtime_error("No valid solution, check shape parameters "
                                   ", detector disatance and angle");
        }
        for (auto &link : xrayPath) {
          double distInObject = link.distInsideObject;
          factor = factor * link.object->material().xRayAttenuation(
                                distInObject, energyVector[i]);
        }
        totalFactor += (normalisedMuonIntensity[k] * factor);
      }
      yData[i] = totalFactor;
    }
  }
  setProperty("OutputWorkspace", std::move(outputWS));
}

} // namespace Algorithms
} // namespace Mantid