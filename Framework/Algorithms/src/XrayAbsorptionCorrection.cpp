// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidAlgorithms/XrayAbsorptionCorrection.h"
#include "MantidAPI/Sample.h"
#include "MantidAPI/SpectrumInfo.h"
#include "MantidAPI/WorkspaceUnitValidator.h"
#include "MantidKernel/BoundedValidator.h"
#include "MantidKernel/CompositeValidator.h"
#include "MantidKernel/Material.h"
#include "MantidKernel/VectorHelper.h"
#include <math.h>
#include <numeric>

using namespace Mantid::API;
using namespace Mantid::Geometry;
using namespace Mantid::Kernel;
using Mantid::DataObjects::Workspace2D;

namespace {
// Angle in degrees
constexpr double DEFAULT_ANGLE = 45.0;
// distance in cm
constexpr double DEFAULT_DETECTOR_DISTANCE = 10.0;
constexpr double ConversionFrom_cm_to_m = 0.01;

} // namespace

namespace Mantid {
namespace Algorithms {

DECLARE_ALGORITHM(XrayAbsorptionCorrection)

/**
 * Initialize the algorithm
 */
void XrayAbsorptionCorrection::init() {

  auto wsValidator = std::make_shared<CompositeValidator>();

  declareProperty(std::make_unique<WorkspaceProperty<>>("InputWorkspace", "", Direction::Input, wsValidator),
                  "The name of the input workspace.");

  declareProperty(std::make_unique<WorkspaceProperty<>>("MuonImplantationProfile", "", Direction::Input, wsValidator),
                  "The name of the Muon Implantation Profile.");

  declareProperty(std::make_unique<WorkspaceProperty<>>("OutputWorkspace", "", Direction::Output),
                  "The name to use for the output workspace.");

  auto positiveDouble = std::make_shared<Kernel::BoundedValidator<double>>();
  positiveDouble->setLower(0);
  declareProperty("DetectorAngle", DEFAULT_ANGLE, positiveDouble,
                  "Angle in degrees between beam and Detector."
                  "Range of normal values for detectors are :  "
                  "Ge1 : 90-180 , Ge2 : 270-360 , Ge3 : 0 - 90 , Ge4 "
                  ": 180 -270.",
                  Direction::Input);

  declareProperty("DetectorDistance", DEFAULT_DETECTOR_DISTANCE, positiveDouble,
                  "Distance in cm between detector and sample.", Direction::Input);
}

std::map<std::string, std::string> XrayAbsorptionCorrection::validateInputs() {
  MatrixWorkspace_sptr inputWS = getProperty("InputWorkspace");
  MatrixWorkspace_sptr muonProfile = getProperty("MuonImplantationProfile");
  std::map<std::string, std::string> issues;
  if (!inputWS->sample().getShape().hasValidShape()) {
    issues["InputWorkspace"] = "Input workspace does not have a Sample";
  }
  auto material = inputWS->sample().getShape().material();
  if (!material.hasValidXRayAttenuationProfile()) {
    issues["InputWorkspace"] = "Input workspace does not have a Xray Attenuation profile";
  }
  if (muonProfile->getNumberHistograms() != 1) {
    issues["MuonImplantationProfile"] = "Muon Implantation profile must have only one spectrum";
  }
  return issues;
}

/**
 * Converts angles in degrees to angles in radians.
 * @param degrees double of angle in degrees
 * @return A double of angle in radians
 */
double XrayAbsorptionCorrection::degreesToRadians(double degrees) { return M_PI * (degrees / 180.0); }

/**
 * Calculates the position of the detector.
 * @param detectorAngle double of angle of detector
 * @param detectorDistance distance betweeen sample and detector
 * @return V3D object of position of detector
 */
Kernel::V3D XrayAbsorptionCorrection::calculateDetectorPos(double const detectorAngle, double detectorDistance) {
  detectorDistance = detectorDistance * ConversionFrom_cm_to_m;
  double x = detectorDistance / std::tan(degreesToRadians(detectorAngle));
  if (detectorAngle > 180.0) {
    detectorDistance = -detectorDistance;
  }
  Kernel::V3D detectorPos = {x, 0.0, detectorDistance};
  return detectorPos;
}

/**
 * normalise moun intensity to 1.
 * @param muonIntensity A MantidVec which contains the intensity of muons as a
 * function  of depth
 * @return A vector of doubles which contains the normalised muon intensity
 */
std::vector<double> XrayAbsorptionCorrection::normaliseMuonIntensity(MantidVec muonIntensity) {

  double sum_of_elems = std::accumulate(muonIntensity.begin(), muonIntensity.end(), 0.0);

  std::transform(muonIntensity.begin(), muonIntensity.end(), muonIntensity.begin(),
                 [sum_of_elems](double d) { return d / sum_of_elems; });
  return muonIntensity;
}

/**
 * Calculate the muon implantation position in the sample.
 * @param muonProfile A reference to the muon profile workspace
 * @param inputWS A reference to the input workspace
 * @param detectorDistance a double representing the distance from the sample to
 * detector
 * @return A vector of V3D objects that represent the position of muons
 */
std::vector<Kernel::V3D> XrayAbsorptionCorrection::calculateMuonPos(API::MatrixWorkspace_sptr &muonProfile,
                                                                    API::MatrixWorkspace_sptr inputWS,
                                                                    double detectorDistance) {
  const MantidVec muonDepth = muonProfile->readX(0);
  Kernel::V3D const muonPoint = {0.0, 0.0, detectorDistance};
  Kernel::V3D toStart = {0.0, 0.0, -1.0};
  const Geometry::IObject *shape = &inputWS->sample().getShape();
  Geometry::Track muonPath = Geometry::Track(muonPoint, toStart);
  shape->interceptSurface(muonPath);
  if (muonPath.count() == 0) {
    throw std::runtime_error("No valid solution, check shape parameters, Muon "
                             "depth profile and detector distance");
  }
  Kernel::V3D intersection = muonPath.cbegin()->entryPoint;
  double sampleDepth = intersection[2];
  std::vector<Kernel::V3D> muonPos;
  for (auto depth : muonDepth) {
    /* Muon implantation position are at x = 0 and y = 0 and z position is
    variable*/
    Kernel::V3D pos = {0.0, 0.0, sampleDepth - (depth * ConversionFrom_cm_to_m)};
    muonPos.push_back(pos);
  }
  return muonPos;
}

/**
 * Execution
 */
void XrayAbsorptionCorrection::exec() {
  MatrixWorkspace_sptr inputWS = getProperty("InputWorkspace");
  MatrixWorkspace_sptr outputWS = inputWS->clone();
  IAlgorithm_sptr convtoPoints = createChildAlgorithm("ConvertToPointData");
  convtoPoints->setProperty("InputWorkspace", inputWS);
  convtoPoints->execute();
  MatrixWorkspace_sptr pointDataWS = convtoPoints->getProperty("OutputWorkspace");

  MatrixWorkspace_sptr muonProfile = getProperty("MuonImplantationProfile");
  MantidVec muonIntensity = muonProfile->readY(0);
  std::vector<double> normalisedMuonIntensity = normaliseMuonIntensity(muonIntensity);
  double detectorAngle = getProperty("DetectorAngle");
  double detectorDistance = getProperty("DetectorDistance");
  Kernel::V3D detectorPos = calculateDetectorPos(detectorAngle, detectorDistance);
  std::vector<Kernel::V3D> muonPos = calculateMuonPos(muonProfile, inputWS, detectorDistance);

  for (size_t j = 0; j < inputWS->getNumberHistograms(); j++) {
    auto &yData = outputWS->mutableY(j);
    MantidVec xData = pointDataWS->readX(j);
    for (size_t i = 0; i < xData.size(); i++) {
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
          factor = factor * link.object->material().xRayAttenuation(distInObject, xData[i]);
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