#include "MantidAlgorithms/SofQWCentre.h"
#include "MantidDataObjects/Histogram1D.h"
#include "MantidAPI/BinEdgeAxis.h"
#include "MantidAPI/CommonBinsValidator.h"
#include "MantidAPI/DetectorInfo.h"
#include "MantidAPI/HistogramValidator.h"
#include "MantidAPI/InstrumentValidator.h"
#include "MantidAPI/SpectraAxisValidator.h"
#include "MantidAPI/SpectrumDetectorMapping.h"
#include "MantidAPI/SpectrumInfo.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidAPI/WorkspaceUnitValidator.h"
#include "MantidGeometry/Instrument.h"
#include "MantidGeometry/Instrument/DetectorGroup.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/BoundedValidator.h"
#include "MantidKernel/CompositeValidator.h"
#include "MantidKernel/ListValidator.h"
#include "MantidKernel/PhysicalConstants.h"
#include "MantidKernel/RebinParamsValidator.h"
#include "MantidKernel/UnitFactory.h"
#include "MantidKernel/VectorHelper.h"

#include <numeric>
#include <stdexcept>

namespace Mantid {
namespace Algorithms {

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(SofQWCentre)

/// Energy to K constant
double SofQWCentre::energyToK() {
  static const double energyToK = 8.0 * M_PI * M_PI *
                                  PhysicalConstants::NeutronMass *
                                  PhysicalConstants::meV * 1e-20 /
                                  (PhysicalConstants::h * PhysicalConstants::h);
  return energyToK;
}

using namespace Kernel;
using namespace API;

/**
 * Create the input properties
 */
void SofQWCentre::init() { createInputProperties(*this); }

/**
 * Create the given algorithm's input properties
 * @param alg An algorithm object
 */
void SofQWCentre::createInputProperties(API::Algorithm &alg) {
  auto wsValidator = boost::make_shared<CompositeValidator>();
  wsValidator->add<WorkspaceUnitValidator>("DeltaE");
  wsValidator->add<SpectraAxisValidator>();
  wsValidator->add<CommonBinsValidator>();
  wsValidator->add<HistogramValidator>();
  wsValidator->add<InstrumentValidator>();
  alg.declareProperty(make_unique<WorkspaceProperty<>>(
                          "InputWorkspace", "", Direction::Input, wsValidator),
                      "Reduced data in units of energy transfer DeltaE.\nThe "
                      "workspace must contain histogram data and have common "
                      "bins across all spectra.");
  alg.declareProperty(make_unique<WorkspaceProperty<>>("OutputWorkspace", "",
                                                       Direction::Output),
                      "The name to use for the q-omega workspace.");
  alg.declareProperty(
      make_unique<ArrayProperty<double>>(
          "QAxisBinning", boost::make_shared<RebinParamsValidator>()),
      "The bin parameters to use for the q axis (in the format used by the "
      ":ref:`algm-Rebin` algorithm).");

  std::vector<std::string> propOptions{"Direct", "Indirect"};
  alg.declareProperty("EMode", "",
                      boost::make_shared<StringListValidator>(propOptions),
                      "The energy transfer analysis mode (Direct/Indirect)");
  auto mustBePositive = boost::make_shared<BoundedValidator<double>>();
  mustBePositive->setLower(0.0);
  alg.declareProperty("EFixed", 0.0, mustBePositive,
                      "The value of fixed energy: :math:`E_i` (EMode=Direct) "
                      "or :math:`E_f` (EMode=Indirect) (meV).\nMust be set "
                      "here if not available in the instrument definition.");
  alg.declareProperty("ReplaceNaNs", false,
                      "If true, replaces all NaNs in output workspace with "
                      "zeroes.",
                      Direction::Input);
}

void SofQWCentre::exec() {
  using namespace Geometry;

  MatrixWorkspace_const_sptr inputWorkspace = getProperty("InputWorkspace");

  // Do the full check for common binning
  if (!WorkspaceHelpers::commonBoundaries(*inputWorkspace)) {
    g_log.error(
        "The input workspace must have common binning across all spectra");
    throw std::invalid_argument(
        "The input workspace must have common binning across all spectra");
  }

  std::vector<double> verticalAxis;
  MatrixWorkspace_sptr outputWorkspace = setUpOutputWorkspace(
      inputWorkspace, getProperty("QAxisBinning"), verticalAxis);
  setProperty("OutputWorkspace", outputWorkspace);

  // Holds the spectrum-detector mapping
  std::vector<specnum_t> specNumberMapping;
  std::vector<detid_t> detIDMapping;

  m_EmodeProperties.initCachedValues(*inputWorkspace, this);
  int emode = m_EmodeProperties.m_emode;

  const auto &detectorInfo = inputWorkspace->detectorInfo();
  const auto &spectrumInfo = inputWorkspace->spectrumInfo();
  V3D beamDir = detectorInfo.samplePosition() - detectorInfo.sourcePosition();
  beamDir.normalize();
  double l1 = detectorInfo.l1();
  g_log.debug() << "Source-sample distance: " << l1 << '\n';

  // Conversion constant for E->k. k(A^-1) = sqrt(energyToK*E(meV))
  const double energyToK = 8.0 * M_PI * M_PI * PhysicalConstants::NeutronMass *
                           PhysicalConstants::meV * 1e-20 /
                           (PhysicalConstants::h * PhysicalConstants::h);

  // Loop over input workspace bins, reassigning data to correct bin in output
  // qw workspace
  const size_t numHists = inputWorkspace->getNumberHistograms();
  const size_t numBins = inputWorkspace->blocksize();
  Progress prog(this, 0.0, 1.0, numHists);
  for (int64_t i = 0; i < int64_t(numHists); ++i) {
    if (!spectrumInfo.hasDetectors(i) || spectrumInfo.isMonitor(i))
      continue;

    const auto &spectrumDet = spectrumInfo.detector(i);
    const double efixed = m_EmodeProperties.getEFixed(spectrumDet);

    // For inelastic scattering the simple relationship q=4*pi*sinTheta/lambda
    // does not hold. In order to
    // be completely general we must calculate the momentum transfer by
    // calculating the incident and final
    // wave vectors and then use |q| = sqrt[(ki - kf)*(ki - kf)]

    const auto &detIDs = inputWorkspace->getSpectrum(i).getDetectorIDs();
    double numDets_d = static_cast<double>(detIDs.size());
    const auto &Y = inputWorkspace->y(i);
    const auto &E = inputWorkspace->e(i);
    const auto &X = inputWorkspace->x(i);

    // Loop over the detectors and for each bin calculate Q
    for (const auto detID : detIDs) {
      try {
        size_t idet = detectorInfo.indexOf(detID);
        // Calculate kf vector direction and then Q for each energy bin
        V3D scatterDir =
            (detectorInfo.position(idet) - detectorInfo.samplePosition());
        scatterDir.normalize();
        for (size_t j = 0; j < numBins; ++j) {
          const double deltaE = 0.5 * (X[j] + X[j + 1]);
          // Compute ki and kf wave vectors and therefore q = ki - kf
          double ei(0.0), ef(0.0);
          if (emode == 1) {
            ei = efixed;
            ef = efixed - deltaE;
            if (ef < 0) {
              std::string mess =
                  "Energy transfer requested in Direct mode exceeds incident "
                  "energy.\n Found for det ID: " +
                  std::to_string(idet) + " bin No " + std::to_string(j) +
                  " with Ei=" + boost::lexical_cast<std::string>(efixed) +
                  " and energy transfer: " +
                  boost::lexical_cast<std::string>(deltaE);
              throw std::runtime_error(mess);
            }
          } else {
            ei = efixed + deltaE;
            ef = efixed;
            if (ef < 0) {
              std::string mess =
                  "Incident energy of a neutron is negative. Are you trying to "
                  "process Direct data in Indirect mode?\n Found for det ID: " +
                  std::to_string(idet) + " bin No " + std::to_string(j) +
                  " with efied=" + boost::lexical_cast<std::string>(efixed) +
                  " and energy transfer: " +
                  boost::lexical_cast<std::string>(deltaE);
              throw std::runtime_error(mess);
            }
          }

          if (ei < 0)
            throw std::runtime_error(
                "Negative incident energy. Check binning.");

          const V3D ki = beamDir * sqrt(energyToK * ei);
          const V3D kf = scatterDir * (sqrt(energyToK * (ef)));
          const double q = (ki - kf).norm();

          // Test whether it's in range of the Q axis
          if (q < verticalAxis.front() || q > verticalAxis.back())
            continue;
          // Find which q bin this point lies in
          const MantidVec::difference_type qIndex =
              std::upper_bound(verticalAxis.begin(), verticalAxis.end(), q) -
              verticalAxis.begin() - 1;

          // Add this spectra-detector pair to the mapping
          specNumberMapping.push_back(
              outputWorkspace->getSpectrum(qIndex).getSpectrumNo());
          detIDMapping.push_back(detID);

          // And add the data and it's error to that bin, taking into account
          // the number of detectors contributing to this bin
          outputWorkspace->mutableY(qIndex)[j] += Y[j] / numDets_d;
          // Standard error on the average
          outputWorkspace->mutableE(qIndex)[j] =
              sqrt((pow(outputWorkspace->e(qIndex)[j], 2) + pow(E[j], 2)) /
                   numDets_d);
        }
      } catch (std::out_of_range &) {
        // Skip invalid detector IDs
        numDets_d -= 1.0;
        continue;
      }
    }
    prog.report();
  }

  // If the input workspace was a distribution, need to divide by q bin width
  if (inputWorkspace->isDistribution())
    this->makeDistribution(outputWorkspace, verticalAxis);

  // Set the output spectrum-detector mapping
  SpectrumDetectorMapping outputDetectorMap(specNumberMapping, detIDMapping);
  outputWorkspace->updateSpectraUsing(outputDetectorMap);

  // Replace any NaNs in outputWorkspace with zeroes
  if (this->getProperty("ReplaceNaNs")) {
    auto replaceNans = this->createChildAlgorithm("ReplaceSpecialValues");
    replaceNans->setChild(true);
    replaceNans->initialize();
    replaceNans->setProperty("InputWorkspace", outputWorkspace);
    replaceNans->setProperty("OutputWorkspace", outputWorkspace);
    replaceNans->setProperty("NaNValue", 0.0);
    replaceNans->setProperty("InfinityValue", 0.0);
    replaceNans->setProperty("BigNumberThreshold", DBL_MAX);
    replaceNans->execute();
  }
}

/** Creates the output workspace, setting the axes according to the input
 * binning parameters
 *  @param[in]  inputWorkspace The input workspace
 *  @param[in]  binParams The bin parameters from the user
 *  @param[out] newAxis        The 'vertical' axis defined by the given
 * parameters
 *  @return A pointer to the newly-created workspace
 */
API::MatrixWorkspace_sptr SofQWCentre::setUpOutputWorkspace(
    API::MatrixWorkspace_const_sptr inputWorkspace,
    const std::vector<double> &binParams, std::vector<double> &newAxis) {
  // Create vector to hold the new X axis values
  HistogramData::BinEdges xAxis(inputWorkspace->sharedX(0));
  const int xLength = static_cast<int>(xAxis.size());
  // Create a vector to temporarily hold the vertical ('y') axis and populate
  // that
  const int yLength = static_cast<int>(
      VectorHelper::createAxisFromRebinParams(binParams, newAxis));

  // Create the output workspace
  MatrixWorkspace_sptr outputWorkspace = WorkspaceFactory::Instance().create(
      inputWorkspace, yLength - 1, xLength, xLength - 1);
  // Create a numeric axis to replace the default vertical one
  Axis *const verticalAxis = new BinEdgeAxis(newAxis);
  outputWorkspace->replaceAxis(1, verticalAxis);

  // Now set the axis values
  for (int i = 0; i < yLength - 1; ++i) {
    outputWorkspace->setBinEdges(i, xAxis);
  }

  // Set the axis units
  verticalAxis->unit() = UnitFactory::Instance().create("MomentumTransfer");
  verticalAxis->title() = "|Q|";

  // Set the X axis title (for conversion to MD)
  outputWorkspace->getAxis(0)->title() = "Energy transfer";

  outputWorkspace->setYUnit("");
  outputWorkspace->setYUnitLabel("Intensity");

  return outputWorkspace;
}

/** Divide each bin by the width of its q bin.
 *  @param outputWS :: The output workspace
 *  @param qAxis ::    A vector of the q bin boundaries
 */
void SofQWCentre::makeDistribution(API::MatrixWorkspace_sptr outputWS,
                                   const std::vector<double> qAxis) {
  std::vector<double> widths(qAxis.size());
  std::adjacent_difference(qAxis.begin(), qAxis.end(), widths.begin());

  const size_t numQBins = outputWS->getNumberHistograms();
  for (size_t i = 0; i < numQBins; ++i) {
    auto &Y = outputWS->mutableY(i);
    auto &E = outputWS->mutableE(i);
    std::transform(Y.begin(), Y.end(), Y.begin(),
                   std::bind2nd(std::divides<double>(), widths[i + 1]));
    std::transform(E.begin(), E.end(), E.begin(),
                   std::bind2nd(std::divides<double>(), widths[i + 1]));
  }
}

} // namespace Algorithms
} // namespace Mantid
