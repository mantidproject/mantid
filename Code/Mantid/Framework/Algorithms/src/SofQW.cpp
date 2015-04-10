//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include <stdexcept>

#include "MantidAlgorithms/SofQW.h"
#include "MantidDataObjects/Histogram1D.h"
#include "MantidAPI/BinEdgeAxis.h"
#include "MantidAPI/SpectrumDetectorMapping.h"
#include "MantidAPI/WorkspaceValidators.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/RebinParamsValidator.h"
#include "MantidKernel/VectorHelper.h"
#include "MantidKernel/UnitFactory.h"
#include "MantidGeometry/Instrument/DetectorGroup.h"
#include "MantidKernel/BoundedValidator.h"
#include "MantidKernel/ListValidator.h"

namespace Mantid {
namespace Algorithms {

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(SofQW)

/// Energy to K constant
double SofQW::energyToK() {
  static const double energyToK = 8.0 * M_PI * M_PI *
                                  PhysicalConstants::NeutronMass *
                                  PhysicalConstants::meV * 1e-20 /
                                  (PhysicalConstants::h * PhysicalConstants::h);
  return energyToK;
}

using namespace Kernel;
using namespace API;

/**
 * @return A summary of the algorithm
 */
const std::string SofQW::summary() const {
  return "Computes S(Q,w) using a either centre point or parallel-piped "
         "rebinning.\n"
         "The output from each method is:\n"
         "CentrePoint - centre-point rebin that takes no account of pixel "
         "curvature or area overlap\n\n"
         "Polygon - parallel-piped rebin, outputting a weighted-sum of "
         "overlapping polygons\n\n"
         "NormalisedPolygon - parallel-piped rebin, outputting a weighted-sum of "
         "overlapping polygons normalised by the fractional area of each overlap";
}

/**
 * Create the input properties
 */
void SofQW::init() { createInputProperties(*this); }

/**
 * Create the given algorithm's input properties
 * @param alg An algorithm object
 */
void SofQW::createInputProperties(API::Algorithm &alg) {
  auto wsValidator = boost::make_shared<CompositeValidator>();
  wsValidator->add<WorkspaceUnitValidator>("DeltaE");
  wsValidator->add<SpectraAxisValidator>();
  wsValidator->add<CommonBinsValidator>();
  wsValidator->add<HistogramValidator>();
  wsValidator->add<InstrumentValidator>();
  alg.declareProperty(new WorkspaceProperty<>("InputWorkspace", "",
                                              Direction::Input, wsValidator),
                      "Reduced data in units of energy transfer DeltaE.\nThe "
                      "workspace must contain histogram data and have common "
                      "bins across all spectra.");
  alg.declareProperty(
      new WorkspaceProperty<>("OutputWorkspace", "", Direction::Output),
      "The name to use for the q-omega workspace.");
  alg.declareProperty(
      new ArrayProperty<double>("QAxisBinning",
                                boost::make_shared<RebinParamsValidator>()),
      "The bin parameters to use for the q axis (in the format used by the "
      ":ref:`algm-Rebin` algorithm).");

  std::vector<std::string> propOptions;
  propOptions.push_back("Direct");
  propOptions.push_back("Indirect");
  alg.declareProperty("EMode", "",
                      boost::make_shared<StringListValidator>(propOptions),
                      "The energy transfer analysis mode (Direct/Indirect)");
  auto mustBePositive = boost::make_shared<BoundedValidator<double>>();
  mustBePositive->setLower(0.0);
  alg.declareProperty("EFixed", 0.0, mustBePositive,
                      "The value of fixed energy: :math:`E_i` (EMode=Direct) "
                      "or :math:`E_f` (EMode=Indirect) (meV).\nMust be set "
                      "here if not available in the instrument definition.");
}

void SofQW::exec() {
  using namespace Geometry;

  MatrixWorkspace_const_sptr inputWorkspace = getProperty("InputWorkspace");

  // Do the full check for common binning
  if (!WorkspaceHelpers::commonBoundaries(inputWorkspace)) {
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
  std::vector<specid_t> specNumberMapping;
  std::vector<detid_t> detIDMapping;

  m_EmodeProperties.initCachedValues(inputWorkspace, this);
  int emode = m_EmodeProperties.m_emode;

  // Get a pointer to the instrument contained in the workspace
  Instrument_const_sptr instrument = inputWorkspace->getInstrument();

  // Get the distance between the source and the sample (assume in metres)
  IComponent_const_sptr source = instrument->getSource();
  IComponent_const_sptr sample = instrument->getSample();
  V3D beamDir = sample->getPos() - source->getPos();
  beamDir.normalize();

  try {
    double l1 = source->getDistance(*sample);
    g_log.debug() << "Source-sample distance: " << l1 << std::endl;
  } catch (Exception::NotFoundError &) {
    g_log.error("Unable to calculate source-sample distance");
    throw Exception::InstrumentDefinitionError(
        "Unable to calculate source-sample distance",
        inputWorkspace->getTitle());
  }

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
    try {
      // Now get the detector object for this histogram
      IDetector_const_sptr spectrumDet = inputWorkspace->getDetector(i);
      if (spectrumDet->isMonitor())
        continue;

      const double efixed = m_EmodeProperties.getEFixed(spectrumDet);

      // For inelastic scattering the simple relationship q=4*pi*sinTheta/lambda
      // does not hold. In order to
      // be completely general we must calculate the momentum transfer by
      // calculating the incident and final
      // wave vectors and then use |q| = sqrt[(ki - kf)*(ki - kf)]
      DetectorGroup_const_sptr detGroup =
          boost::dynamic_pointer_cast<const DetectorGroup>(spectrumDet);
      std::vector<IDetector_const_sptr> detectors;
      if (detGroup) {
        detectors = detGroup->getDetectors();
      } else {
        detectors.push_back(spectrumDet);
      }

      const size_t numDets = detectors.size();
      const double numDets_d = static_cast<double>(
          numDets); // cache to reduce number of static casts
      const MantidVec &Y = inputWorkspace->readY(i);
      const MantidVec &E = inputWorkspace->readE(i);
      const MantidVec &X = inputWorkspace->readX(i);

      // Loop over the detectors and for each bin calculate Q
      for (size_t idet = 0; idet < numDets; ++idet) {
        IDetector_const_sptr det = detectors[idet];
        // Calculate kf vector direction and then Q for each energy bin
        V3D scatterDir = (det->getPos() - sample->getPos());
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
                  boost::lexical_cast<std::string>(idet) + " bin No " +
                  boost::lexical_cast<std::string>(j) + " with Ei=" +
                  boost::lexical_cast<std::string>(efixed) +
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
                  boost::lexical_cast<std::string>(idet) + " bin No " +
                  boost::lexical_cast<std::string>(j) + " with efied=" +
                  boost::lexical_cast<std::string>(efixed) +
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
              outputWorkspace->getSpectrum(qIndex)->getSpectrumNo());
          detIDMapping.push_back(det->getID());

          // And add the data and it's error to that bin, taking into account
          // the number of detectors contributing to this bin
          outputWorkspace->dataY(qIndex)[j] += Y[j] / numDets_d;
          // Standard error on the average
          outputWorkspace->dataE(qIndex)[j] =
              sqrt((pow(outputWorkspace->readE(qIndex)[j], 2) + pow(E[j], 2)) /
                   numDets_d);
        }
      }

    } catch (Exception::NotFoundError &) {
      // Get to here if exception thrown when calculating distance to detector
      // Presumably, if we get to here the spectrum will be all zeroes anyway
      // (from conversion to E)
      continue;
    }
    prog.report();
  }

  // If the input workspace was a distribution, need to divide by q bin width
  if (inputWorkspace->isDistribution())
    this->makeDistribution(outputWorkspace, verticalAxis);

  // Set the output spectrum-detector mapping
  SpectrumDetectorMapping outputDetectorMap(specNumberMapping, detIDMapping);
  outputWorkspace->updateSpectraUsing(outputDetectorMap);
}

/** Creates the output workspace, setting the axes according to the input
 * binning parameters
 *  @param[in]  inputWorkspace The input workspace
 *  @param[in]  binParams The bin parameters from the user
 *  @param[out] newAxis        The 'vertical' axis defined by the given
 * parameters
 *  @return A pointer to the newly-created workspace
 */
API::MatrixWorkspace_sptr
SofQW::setUpOutputWorkspace(API::MatrixWorkspace_const_sptr inputWorkspace,
                            const std::vector<double> &binParams,
                            std::vector<double> &newAxis) {
  // Create vector to hold the new X axis values
  MantidVecPtr xAxis;
  xAxis.access() = inputWorkspace->readX(0);
  const int xLength = static_cast<int>(xAxis->size());
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
    outputWorkspace->setX(i, xAxis);
  }

  // Set the axis units
  verticalAxis->unit() = UnitFactory::Instance().create("MomentumTransfer");
  verticalAxis->title() = "|Q|";

  // Set the X axis title (for conversion to MD)
  outputWorkspace->getAxis(0)->title() = "Energy transfer";

  return outputWorkspace;
}

/** Divide each bin by the width of its q bin.
 *  @param outputWS :: The output workspace
 *  @param qAxis ::    A vector of the q bin boundaries
 */
void SofQW::makeDistribution(API::MatrixWorkspace_sptr outputWS,
                             const std::vector<double> qAxis) {
  std::vector<double> widths(qAxis.size());
  std::adjacent_difference(qAxis.begin(), qAxis.end(), widths.begin());

  const size_t numQBins = outputWS->getNumberHistograms();
  for (size_t i = 0; i < numQBins; ++i) {
    MantidVec &Y = outputWS->dataY(i);
    MantidVec &E = outputWS->dataE(i);
    std::transform(Y.begin(), Y.end(), Y.begin(),
                   std::bind2nd(std::divides<double>(), widths[i + 1]));
    std::transform(E.begin(), E.end(), E.begin(),
                   std::bind2nd(std::divides<double>(), widths[i + 1]));
  }
}

} // namespace Algorithms
} // namespace Mantid
