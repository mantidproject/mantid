// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidAlgorithms/EstimateResolutionDiffraction.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/Run.h"
#include "MantidAPI/SpectrumInfo.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidAPI/WorkspaceProperty.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidDataObjects/WorkspaceCreation.h"
#include "MantidGeometry/Crystal/AngleUnits.h"
#include "MantidGeometry/IDetector.h"
#include "MantidGeometry/Instrument/ComponentInfo.h"
#include "MantidGeometry/Instrument/Detector.h"
#include "MantidGeometry/Instrument/DetectorInfo.h"
#include "MantidKernel/BoundedValidator.h"
#include "MantidKernel/PhysicalConstants.h"
#include "MantidKernel/TimeSeriesProperty.h"
#include "MantidKernel/V3D.h"
#include "MantidTypes/SpectrumDefinition.h"

#include <cmath>

using namespace Mantid;
using namespace Mantid::API;
using namespace Mantid::Geometry;
using namespace Mantid::Kernel;
using namespace std;

namespace Mantid::Algorithms {

DECLARE_ALGORITHM(EstimateResolutionDiffraction)

namespace { // hide these constants
///
const double MICROSEC_TO_SEC = 1.0E-6;
///
const double WAVELENGTH_TO_VELOCITY = 1.0E10 * PhysicalConstants::h / PhysicalConstants::NeutronMass;
/// This is an absurd number for even ultra cold neutrons
const double WAVELENGTH_MAX = 1000.;

namespace PropertyNames {
const std::string INPUT_WKSP("InputWorkspace");
const std::string OUTPUT_WKSP("OutputWorkspace");
const std::string PARTIALS_WKSP_GRP("PartialResolutionWorkspaces");
const std::string DIVERGENCE_WKSP("DivergenceWorkspace");
const std::string WAVELENGTH("Wavelength");
const std::string DELTA_T("DeltaTOF");
const std::string DELTA_T_OVER_T("DeltaTOFOverTOF");
const std::string SOURCE_DELTA_L("SourceDeltaL");
const std::string SOURCE_DELTA_THETA("SourceDeltaTheta");
} // namespace PropertyNames
} // namespace

const std::string EstimateResolutionDiffraction::name() const { return "EstimateResolutionDiffraction"; }

const std::string EstimateResolutionDiffraction::alias() const { return "EstimatePDDetectorResolution"; }

const std::string EstimateResolutionDiffraction::summary() const {
  return "Estimate the resolution of each detector pixel for a powder "
         "diffractometer";
}

int EstimateResolutionDiffraction::version() const { return 1; }

const std::string EstimateResolutionDiffraction::category() const { return "Diffraction\\Utility"; }

void EstimateResolutionDiffraction::init() {
  declareProperty(std::make_unique<WorkspaceProperty<MatrixWorkspace>>(PropertyNames::INPUT_WKSP, "", Direction::Input),
                  "Name of the workspace to have detector resolution calculated");
  declareProperty(std::make_unique<WorkspaceProperty<MatrixWorkspace>>(PropertyNames::DIVERGENCE_WKSP, "",
                                                                       Direction::Input, PropertyMode::Optional),
                  "Workspace containing the divergence");
  declareProperty(
      std::make_unique<WorkspaceProperty<MatrixWorkspace>>(PropertyNames::OUTPUT_WKSP, "", Direction::Output),
      "Name of the output workspace containing delta(d)/d of each "
      "detector/spectrum");

  auto positiveOrZero = std::make_shared<BoundedValidator<double>>();
  positiveOrZero->setLower(0.);
  positiveOrZero->setLowerExclusive(false);
  declareProperty(
      std::make_unique<PropertyWithValue<double>>(PropertyNames::DELTA_T, 0., positiveOrZero, PropertyMode::Optional),
      "DeltaT as the resolution of TOF with unit microsecond");

  declareProperty(PropertyNames::DELTA_T_OVER_T, EMPTY_DBL(), positiveOrZero,
                  "DeltaT/T as the full term in the equation");

  declareProperty(PropertyNames::SOURCE_DELTA_L, 0., positiveOrZero,
                  "Uncertainty in the path length due to the source in unit of meters");
  declareProperty(PropertyNames::SOURCE_DELTA_THETA, 0., positiveOrZero,
                  "Uncertainty in angle due to the source in unit of radians");

  auto positiveWavelength = std::make_shared<BoundedValidator<double>>();
  positiveWavelength->setLower(0.);
  positiveWavelength->setLowerExclusive(true);
  declareProperty(PropertyNames::WAVELENGTH, EMPTY_DBL(), positiveWavelength,
                  "Wavelength setting in Angstroms. This overrides what is in "
                  "the dataset.");

  declareProperty(
      std::make_unique<WorkspaceProperty<WorkspaceGroup>>(PropertyNames::PARTIALS_WKSP_GRP, "", Direction::Output),
      "Workspaces created showing the various resolution terms");
}

std::map<std::string, std::string> EstimateResolutionDiffraction::validateInputs() {
  std::map<std::string, std::string> errors;

  // cannot specify both deltaTOF AND deltaTOF/TOF
  const double deltaT = getProperty(PropertyNames::DELTA_T);
  const bool hasDeltaT = (deltaT > 0.);
  const bool hasDeltaTOverT = (!isDefault(PropertyNames::DELTA_T_OVER_T));
  std::string msg;
  if (hasDeltaT && hasDeltaTOverT) {
    msg = "Cannot specify both " + PropertyNames::DELTA_T + " and " + PropertyNames::DELTA_T_OVER_T;
  } else if ((!hasDeltaT) && (!hasDeltaTOverT)) {
    msg = "Must specify either " + PropertyNames::DELTA_T + " or " + PropertyNames::DELTA_T_OVER_T;
  }
  if (!msg.empty()) {
    errors[PropertyNames::DELTA_T] = msg;
    errors[PropertyNames::DELTA_T_OVER_T] = msg;
  }

  return errors;
}

void EstimateResolutionDiffraction::exec() {
  processAlgProperties();

  // create all of the output workspaces
  std::string partials_prefix = getPropertyValue(PropertyNames::PARTIALS_WKSP_GRP);
  m_resTof = DataObjects::create<DataObjects::Workspace2D>(*m_inputWS, HistogramData::Points(1));
  m_resPathLength = DataObjects::create<DataObjects::Workspace2D>(*m_inputWS, HistogramData::Points(1));
  m_resAngle = DataObjects::create<DataObjects::Workspace2D>(*m_inputWS, HistogramData::Points(1));
  m_outputWS = DataObjects::create<DataObjects::Workspace2D>(*m_inputWS, HistogramData::Points(1));

  estimateDetectorResolution();

  setProperty(PropertyNames::OUTPUT_WKSP, m_outputWS);

  // put together the output group
  auto partialsGroup = std::make_shared<WorkspaceGroup>();
  API::AnalysisDataService::Instance().addOrReplace(partials_prefix + "_tof", m_resTof);
  API::AnalysisDataService::Instance().addOrReplace(partials_prefix + "_length", m_resPathLength);
  API::AnalysisDataService::Instance().addOrReplace(partials_prefix + "_angle", m_resAngle);
  partialsGroup->addWorkspace(m_resTof);
  partialsGroup->addWorkspace(m_resPathLength);
  partialsGroup->addWorkspace(m_resAngle);
  setProperty(PropertyNames::PARTIALS_WKSP_GRP, partialsGroup);
}

void EstimateResolutionDiffraction::processAlgProperties() {
  m_inputWS = getProperty(PropertyNames::INPUT_WKSP);
  m_divergenceWS = getProperty(PropertyNames::DIVERGENCE_WKSP);

  // get source delta-L value and square it
  m_sourceDeltaLMetersSq = getProperty(PropertyNames::SOURCE_DELTA_L);
  m_sourceDeltaLMetersSq = m_sourceDeltaLMetersSq * m_sourceDeltaLMetersSq;

  // get source delta-theta value and square it
  m_sourceDeltaThetaRadiansSq = getProperty(PropertyNames::SOURCE_DELTA_THETA);
  m_sourceDeltaThetaRadiansSq = m_sourceDeltaThetaRadiansSq * m_sourceDeltaThetaRadiansSq;

  if (isDefault(PropertyNames::DELTA_T_OVER_T)) {
    m_deltaT = getProperty(PropertyNames::DELTA_T);
    m_deltaT *= MICROSEC_TO_SEC; // convert to seconds

    calcCentreVelocity();
  } else {
    m_deltaTOverTOF = getProperty(PropertyNames::DELTA_T_OVER_T);
  }
}

double EstimateResolutionDiffraction::getWavelength() {
  double wavelength = getProperty(PropertyNames::WAVELENGTH);
  if (!isEmpty(wavelength)) {
    return wavelength;
  }

  Property *cwlproperty = m_inputWS->run().getProperty("LambdaRequest");
  if (!cwlproperty)
    throw runtime_error("Unable to locate property LambdaRequest as central wavelength. ");

  auto const *cwltimeseries = dynamic_cast<TimeSeriesProperty<double> *>(cwlproperty);

  if (!cwltimeseries)
    throw runtime_error("LambdaReqeust is not a TimeSeriesProperty in double. ");

  string unit = cwltimeseries->units();
  if ((unit != "Angstrom") && (unit != "A")) {
    throw runtime_error("Unit is not recognized: " + unit);
  }

  return m_inputWS->run().getTimeAveragedValue("LambdaRequest");
}

void EstimateResolutionDiffraction::calcCentreVelocity() {
  double centrewavelength = getWavelength();
  g_log.notice() << "Centre wavelength = " << centrewavelength << " Angstrom\n";
  if (centrewavelength > WAVELENGTH_MAX) {
    throw runtime_error("unphysical wavelength used");
  }

  // Calculate centre neutron velocity
  m_centreVelocity = WAVELENGTH_TO_VELOCITY / centrewavelength;
  g_log.notice() << "Centre neutron velocity = " << m_centreVelocity << "\n";
}

void EstimateResolutionDiffraction::estimateDetectorResolution() {
  const auto &spectrumInfo = m_inputWS->spectrumInfo();
  const auto l1 = spectrumInfo.l1();
  const auto &componentInfo = m_inputWS->componentInfo();
  const auto &detectorInfo = m_inputWS->detectorInfo();
  g_log.notice() << "L1 = " << l1 << "\n";
  const auto samplepos = spectrumInfo.samplePosition();

  const size_t numspec = m_inputWS->getNumberHistograms();

  double mintwotheta = 2. * M_PI; // a bit more than 2*pi
  double maxtwotheta = 0.;
  for (std::size_t i = 0; i < detectorInfo.size(); ++i) {
    if (!(detectorInfo.isMasked(i) || detectorInfo.isMonitor(i))) {
      // update overall range for two-theta and term3
      const auto twotheta = detectorInfo.twoTheta(i);
      mintwotheta = std::min(twotheta, mintwotheta);
      maxtwotheta = std::max(twotheta, maxtwotheta);
    }
  }

  double minTerm3Sq = 1.;
  double maxTerm3Sq = 0.;

  g_log.information() << "Source terms: deltaL=" << sqrt(m_sourceDeltaLMetersSq)
                      << " deltaTheta=" << sqrt(m_sourceDeltaThetaRadiansSq) << "\n";

  // if a spectrum has multiple pixels, add them up in quadrature
  for (size_t specNum = 0; specNum < numspec; ++specNum) {
    // which of the detectors are part of this spectrum
    const auto &spectrumDefinition = spectrumInfo.spectrumDefinition(specNum);
    // number of spectra as a double
    const double numDet = double(spectrumDefinition.size());

    // resolution in time
    double term1Sq = m_deltaTOverTOF * m_deltaTOverTOF;
    if (term1Sq == 0.) { // calculate per pixel
      const auto centreVel = m_centreVelocity;
      const auto deltaT = m_deltaT;
      term1Sq = std::accumulate(spectrumDefinition.cbegin(), spectrumDefinition.cend(), 0.,
                                [&detectorInfo, &centreVel, &l1, &deltaT](const auto sum, const auto &index) {
                                  if (detectorInfo.isMasked(index.first) || detectorInfo.isMonitor(index.first)) {
                                    return sum;
                                  } else {
                                    // Get the distance from detector to source
                                    const double l2 = detectorInfo.l2(index.first);
                                    const double centraltof = (l1 + l2) / centreVel;
                                    const double term = deltaT / centraltof;
                                    return sum + (term * term);
                                  }
                                });
      term1Sq = term1Sq / numDet;
    }

    // resolution in length
    double term2Sq;
    { // reduce scope
      const double sourceTerm = m_sourceDeltaLMetersSq;
      term2Sq = std::accumulate(spectrumDefinition.cbegin(), spectrumDefinition.cend(), 0.,
                                [&detectorInfo, &sourceTerm, &l1](const auto sum, const auto &index) {
                                  if (detectorInfo.isMasked(index.first) || detectorInfo.isMonitor(index.first)) {
                                    return sum;
                                  } else {
                                    const auto &detector = detectorInfo.detector(index.first);
                                    const auto realdet = dynamic_cast<const Detector *>(&detector);
                                    if (realdet) {
                                      const auto l2 = detectorInfo.l2(index.first);
                                      const auto l_total = (l1 + l2);
                                      realdet->shape();
                                      const double dx = realdet->getWidth();
                                      const double dy = realdet->getHeight();
                                      const double dz = realdet->getDepth();
                                      // not sure why divide by 4, but it has always been there
                                      const double detdimSq = (dx * dx + dy * dy + dz * dz) * 0.25;
                                      const double term = (detdimSq + sourceTerm) / (l_total * l_total);
                                      return sum + term;
                                    } else {
                                      return sum;
                                    }
                                  }
                                });
      term2Sq = term2Sq / numDet;
    }

    // resolution in angle - everything is in radians
    double term3Sq;
    if (m_divergenceWS) {
      double deltathetaSq = m_divergenceWS->y(specNum)[0];
      deltathetaSq = deltathetaSq * deltathetaSq;
      // use the average angle for the spectrum - not right for focussed data
      const double theta = spectrumInfo.isMonitor(specNum) ? 0.0 : 0.5 * spectrumInfo.twoTheta(specNum);
      const double tan_theta = tan(theta);
      term3Sq = (deltathetaSq + m_sourceDeltaThetaRadiansSq) / (tan_theta * tan_theta);
    } else {
      const double sourceTerm = m_sourceDeltaThetaRadiansSq;
      const double solidAngle = std::accumulate(
          spectrumDefinition.cbegin(), spectrumDefinition.cend(), 0.,
          [&componentInfo, &samplepos, &detectorInfo, &sourceTerm](const auto sum, const auto &index) {
            if (detectorInfo.isMasked(index.first) || detectorInfo.isMonitor(index.first)) {
              return sum;
            } else {
              const double theta = 0.5 * detectorInfo.twoTheta(index.first); // angle of the pixel
              const double cot_theta = 1. / tan(theta);
              return sum + ((componentInfo.solidAngle(index.first, samplepos) + sourceTerm) * cot_theta * cot_theta);
            }
          });
      term3Sq = solidAngle / numDet;
    }

    // add information to outputs
    if (spectrumInfo.isMonitor(specNum)) {
      m_resTof->mutableY(specNum) = 0.;
      m_resPathLength->mutableY(specNum) = 0.;
      m_resAngle->mutableY(specNum) = 0.;
      m_outputWS->mutableY(specNum) = 0.;
    } else { // not a monitor
      const double resolution = sqrt(term1Sq + term2Sq + term3Sq);
      m_resTof->mutableY(specNum) = sqrt(term1Sq);
      m_resPathLength->mutableY(specNum) = sqrt(term2Sq);
      m_resAngle->mutableY(specNum) = sqrt(term3Sq);
      m_outputWS->mutableY(specNum) = resolution;
    }

    m_resTof->mutableX(specNum) = static_cast<double>(specNum);
    m_resPathLength->mutableX(specNum) = static_cast<double>(specNum);
    m_resAngle->mutableX(specNum) = static_cast<double>(specNum);
    m_outputWS->mutableX(specNum) = static_cast<double>(specNum);

    // update overall range for term3
    minTerm3Sq = std::min(term3Sq, minTerm3Sq);
    maxTerm3Sq = std::max(term3Sq, maxTerm3Sq);

    // log extra information if level is debug (7) or greater - converting to strings is expensive
    if (g_log.getLevelOffset() > 6) {
      // central two-theta
      const double twotheta = spectrumInfo.isMonitor(specNum) ? 0.0 : spectrumInfo.twoTheta(specNum);
      const auto &det = spectrumInfo.detector(specNum);

      g_log.debug() << det.type() << " " << specNum << "\t\t" << twotheta << "\t\tdT/T = " << sqrt(term1Sq)
                    << "\t\tdL/L = " << sqrt(term2Sq) << "\t\tdTheta*cotTheta = " << sqrt(term3Sq) << "\n";
    }
  }

  g_log.notice() << "2theta range: " << (mintwotheta * Geometry::rad2deg) << ", " << (maxtwotheta * Geometry::rad2deg)
                 << "\n";
  g_log.notice() << "t3 range: " << sqrt(minTerm3Sq) << ", " << sqrt(maxTerm3Sq) << "\n";
}

} // namespace Mantid::Algorithms
