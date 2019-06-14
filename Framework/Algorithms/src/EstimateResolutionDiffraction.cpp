// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidAlgorithms/EstimateResolutionDiffraction.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/Run.h"
#include "MantidAPI/SpectrumInfo.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidAPI/WorkspaceProperty.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidDataObjects/WorkspaceCreation.h"
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

namespace Mantid {
namespace Algorithms {

DECLARE_ALGORITHM(EstimateResolutionDiffraction)

namespace { // hide these constants
///
const double MICROSEC_TO_SEC = 1.0E-6;
///
const double WAVELENGTH_TO_VELOCITY =
    1.0E10 * PhysicalConstants::h / PhysicalConstants::NeutronMass;
/// This is an absurd number for even ultra cold neutrons
const double WAVELENGTH_MAX = 1000.;
} // namespace

const std::string EstimateResolutionDiffraction::name() const {
  return "EstimateResolutionDiffraction";
}

const std::string EstimateResolutionDiffraction::alias() const {
  return "EstimatePDDetectorResolution";
}

const std::string EstimateResolutionDiffraction::summary() const {
  return "Estimate the resolution of each detector pixel for a powder "
         "diffractometer";
}

int EstimateResolutionDiffraction::version() const { return 1; }

const std::string EstimateResolutionDiffraction::category() const {
  return "Diffraction\\Utility";
}

void EstimateResolutionDiffraction::init() {
  declareProperty(
      std::make_unique<WorkspaceProperty<MatrixWorkspace>>("InputWorkspace", "",
                                                           Direction::Input),
      "Name of the workspace to have detector resolution calculated");
  declareProperty(
      std::make_unique<WorkspaceProperty<MatrixWorkspace>>(
          "DivergenceWorkspace", "", Direction::Input, PropertyMode::Optional),
      "Workspace containing the divergence");
  declareProperty(std::make_unique<WorkspaceProperty<MatrixWorkspace>>(
                      "OutputWorkspace", "", Direction::Output),
                  "Name of the output workspace containing delta(d)/d of each "
                  "detector/spectrum");

  auto positiveDeltaTOF = boost::make_shared<BoundedValidator<double>>();
  positiveDeltaTOF->setLower(0.);
  positiveDeltaTOF->setLowerExclusive(true);
  declareProperty("DeltaTOF", 0., positiveDeltaTOF,
                  "DeltaT as the resolution of TOF with unit microsecond");

  auto positiveWavelength = boost::make_shared<BoundedValidator<double>>();
  positiveWavelength->setLower(0.);
  positiveWavelength->setLowerExclusive(true);
  declareProperty("Wavelength", EMPTY_DBL(), positiveWavelength,
                  "Wavelength setting in Angstroms. This overrides what is in "
                  "the dataset.");

  declareProperty(std::make_unique<WorkspaceProperty<WorkspaceGroup>>(
                      "PartialResolutionWorkspaces", "", Direction::Output),
                  "Workspaces created showing the various resolution terms");
}

/**
 */
void EstimateResolutionDiffraction::exec() {
  processAlgProperties();

  retrieveInstrumentParameters();

  // create all of the output workspaces
  std::string partials_prefix = getPropertyValue("PartialResolutionWorkspaces");
  m_resTof = DataObjects::create<DataObjects::Workspace2D>(
      *m_inputWS, HistogramData::Points(1));
  m_resPathLength = DataObjects::create<DataObjects::Workspace2D>(
      *m_inputWS, HistogramData::Points(1));
  m_resAngle = DataObjects::create<DataObjects::Workspace2D>(
      *m_inputWS, HistogramData::Points(1));
  m_outputWS = DataObjects::create<DataObjects::Workspace2D>(
      *m_inputWS, HistogramData::Points(1));

  estimateDetectorResolution();

  setProperty("OutputWorkspace", m_outputWS);

  // put together the output group
  auto partialsGroup = boost::make_shared<WorkspaceGroup>();
  API::AnalysisDataService::Instance().addOrReplace(partials_prefix + "_tof",
                                                    m_resTof);
  API::AnalysisDataService::Instance().addOrReplace(partials_prefix + "_length",
                                                    m_resPathLength);
  API::AnalysisDataService::Instance().addOrReplace(partials_prefix + "_angle",
                                                    m_resAngle);
  partialsGroup->addWorkspace(m_resTof);
  partialsGroup->addWorkspace(m_resPathLength);
  partialsGroup->addWorkspace(m_resAngle);
  setProperty("PartialResolutionWorkspaces", partialsGroup);
}

/**
 */
void EstimateResolutionDiffraction::processAlgProperties() {
  m_inputWS = getProperty("InputWorkspace");
  m_divergenceWS = getProperty("DivergenceWorkspace");

  m_deltaT = getProperty("DeltaTOF");
  m_deltaT *= MICROSEC_TO_SEC; // convert to meter
}

double EstimateResolutionDiffraction::getWavelength() {
  double wavelength = getProperty("Wavelength");
  if (!isEmpty(wavelength)) {
    return wavelength;
  }

  Property *cwlproperty = m_inputWS->run().getProperty("LambdaRequest");
  if (!cwlproperty)
    throw runtime_error(
        "Unable to locate property LambdaRequest as central wavelength. ");

  TimeSeriesProperty<double> *cwltimeseries =
      dynamic_cast<TimeSeriesProperty<double> *>(cwlproperty);

  if (!cwltimeseries)
    throw runtime_error(
        "LambdaReqeust is not a TimeSeriesProperty in double. ");

  string unit = cwltimeseries->units();
  if ((unit != "Angstrom") && (unit != "A")) {
    throw runtime_error("Unit is not recognized: " + unit);
  }

  return cwltimeseries->timeAverageValue();
}

/**
 */
void EstimateResolutionDiffraction::retrieveInstrumentParameters() {
  double centrewavelength = getWavelength();
  g_log.notice() << "Centre wavelength = " << centrewavelength << " Angstrom\n";
  if (centrewavelength > WAVELENGTH_MAX) {
    throw runtime_error("unphysical wavelength used");
  }

  // Calculate centre neutron velocity
  m_centreVelocity = WAVELENGTH_TO_VELOCITY / centrewavelength;
  g_log.notice() << "Centre neutron velocity = " << m_centreVelocity << "\n";
}

/**
 */
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

  double mint3 = 1.;
  double maxt3 = 0.;

  size_t count_nodetsize = 0;

  for (size_t i = 0; i < numspec; ++i) {
    const auto &det = spectrumInfo.detector(i);
    double detdim;
    const auto realdet = dynamic_cast<const Detector *>(&det);
    if (realdet) {
      const double dy = realdet->getHeight();
      const double dx = realdet->getWidth();
      detdim = sqrt(dx * dx + dy * dy) * 0.5;
    } else {
      // Use detector dimension as 0 as no-information
      detdim = 0;
      ++count_nodetsize;
    }

    // Get the distance from detector to source
    const double l2 = spectrumInfo.l2(i);

    // Calculate T
    const double centraltof = (l1 + l2) / m_centreVelocity;

    // Angle
    const double twotheta =
        spectrumInfo.isMonitor(i) ? 0.0 : spectrumInfo.twoTheta(i);
    const double theta = 0.5 * twotheta;

    double deltatheta = 0.;
    if (m_divergenceWS) {
      deltatheta = m_divergenceWS->y(i)[0];
    } else {
      auto &spectrumDefinition = spectrumInfo.spectrumDefinition(i);
      const double solidangle = std::accumulate(
          spectrumDefinition.cbegin(), spectrumDefinition.cend(), 0.,
          [&componentInfo, &detectorInfo, &samplepos](const auto sum,
                                                      const auto &index) {
            if (!detectorInfo.isMasked(index.first)) {
              return sum + componentInfo.solidAngle(index.first, samplepos);
            } else {
              return sum;
            }
          });
      deltatheta = sqrt(solidangle);
    }

    // Resolution
    const double t1 = m_deltaT / centraltof;
    const double t2 = detdim / (l1 + l2);
    const double t3 = deltatheta / tan(theta);

    if (spectrumInfo.isMonitor(i)) {
      m_resTof->mutableY(i) = 0.;
      m_resPathLength->mutableY(i) = 0.;
      m_resAngle->mutableY(i) = 0.;
      m_outputWS->mutableY(i) = 0.;
    } else { // not a monitor
      const double resolution = sqrt(t1 * t1 + t2 * t2 + t3 * t3);
      m_resTof->mutableY(i) = t1;
      m_resPathLength->mutableY(i) = t2;
      m_resAngle->mutableY(i) = t3;
      m_outputWS->mutableY(i) = resolution;
    }

    m_resTof->mutableX(i) = static_cast<double>(i);
    m_resPathLength->mutableX(i) = static_cast<double>(i);
    m_resAngle->mutableX(i) = static_cast<double>(i);
    m_outputWS->mutableX(i) = static_cast<double>(i);

    maxtwotheta = std::max(twotheta, maxtwotheta);
    mintwotheta = std::min(twotheta, mintwotheta);

    if (fabs(t3) < mint3)
      mint3 = fabs(t3);
    else if (fabs(t3) > maxt3)
      maxt3 = fabs(t3);

    g_log.debug() << det.type() << " " << i << "\t\t" << twotheta
                  << "\t\tdT/T = " << t1 * t1 << "\t\tdL/L = " << t2
                  << "\t\tdTheta*cotTheta = " << t3 << "\n";
  }

  g_log.notice() << "2theta range: " << mintwotheta << ", " << maxtwotheta
                 << "\n";
  g_log.notice() << "t3 range: " << mint3 << ", " << maxt3 << "\n";
  g_log.notice() << "Number of detector having NO size information = "
                 << count_nodetsize << "\n";
}

} // namespace Algorithms
} // namespace Mantid
