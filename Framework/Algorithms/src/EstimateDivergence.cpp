// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidAlgorithms/EstimateDivergence.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/SpectrumInfo.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidDataObjects/WorkspaceCreation.h"
#include "MantidGeometry/IDetector.h"
#include "MantidGeometry/Instrument/ComponentInfo.h"
#include "MantidGeometry/Instrument/DetectorInfo.h"
#include "MantidKernel/BoundedValidator.h"
#include "MantidKernel/V3D.h"
#include "MantidTypes/SpectrumDefinition.h"

namespace Mantid {
namespace Algorithms {

using namespace std;
using Mantid::API::WorkspaceProperty; // NOLINT
using Mantid::Kernel::Direction;      // NOLINT

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(EstimateDivergence)

//----------------------------------------------------------------------------------------------

/// Algorithms name for identification. @see Algorithm::name
const std::string EstimateDivergence::name() const {
  return "EstimateDivergence";
}

/// Algorithm's version for identification. @see Algorithm::version
int EstimateDivergence::version() const { return 1; }

/// Algorithm's category for identification. @see Algorithm::category
const std::string EstimateDivergence::category() const {
  return "Diffraction\\Utility";
}

/// Algorithm's summary for use in the GUI and help. @see Algorithm::summary
const std::string EstimateDivergence::summary() const {
  return "Estimate the divergence of each detector pixel";
}

//----------------------------------------------------------------------------------------------
/** Initialize the algorithm's properties.
 */
void EstimateDivergence::init() {
  declareProperty(std::make_unique<WorkspaceProperty<API::MatrixWorkspace>>(
                      "InputWorkspace", "", Direction::Input),
                  "Workspace to have divergence calculated from");

  auto positiveParameter =
      boost::make_shared<Kernel::BoundedValidator<double>>();
  positiveParameter->setLower(0.);
  positiveParameter->setLowerExclusive(false); // zero is allowed

  declareProperty("alpha", 0., positiveParameter,
                  "Vertical divergence parameter");
  declareProperty("beta0", 0., positiveParameter,
                  "Horizontal divergence parameter");
  declareProperty("beta1", 0., positiveParameter,
                  "Other horizontal divergence parameter");

  declareProperty(
      std::make_unique<WorkspaceProperty<API::MatrixWorkspace>>(
          "OutputWorkspace", "", Direction::Output),
      "Workspace containing the divergence of each detector/spectrum");
}

//----------------------------------------------------------------------------------------------
/** Execute the algorithm.
 */
void EstimateDivergence::exec() {
  // read input parameters
  API::MatrixWorkspace_sptr inputWS = getProperty("InputWorkspace");
  const double alpha = getProperty("alpha");
  const double beta0 = getProperty("beta0");
  const double beta1 = getProperty("beta1");

  // derived terms
  const double horizontal = alpha * alpha;
  const double vertical_numerator = 4. * (beta0 * beta0 + beta1 * beta1);

  // create output workspaces
  API::MatrixWorkspace_sptr divergenceWS =
      DataObjects::create<DataObjects::Workspace2D>(*inputWS,
                                                    HistogramData::Points(1));

  // do the math
  const auto &spectrumInfo = inputWS->spectrumInfo();
  const auto samplepos = spectrumInfo.samplePosition();
  const auto &componentInfo = inputWS->componentInfo();
  const auto &detectorInfo = inputWS->detectorInfo();
  size_t numspec = inputWS->getNumberHistograms();
  double solidangletotal = 0.;
  for (size_t i = 0; i < numspec; ++i) {
    // angle
    const double twotheta =
        spectrumInfo.isMonitor(i) ? 0.0 : spectrumInfo.twoTheta(i);
    const double sintwotheta = sin(twotheta);
    // vertical term
    const double vertical = vertical_numerator / (sintwotheta * sintwotheta);

    // solid angle
    auto &spectrumDefinition = spectrumInfo.spectrumDefinition(i);
    // No scanning support for solidAngle currently, use only first component
    // of index, ignore time index
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
    solidangletotal += solidangle;
    const double deltatwotheta = sqrt(solidangle);

    // put it all together and set it in the output workspace
    const double divergence =
        .5 * sqrt(deltatwotheta * deltatwotheta + horizontal + vertical);
    divergenceWS->mutableX(i)[0] = static_cast<double>(i);
    if (spectrumInfo.isMonitor(i))
      divergenceWS->mutableY(i)[0] = 0.;
    else
      divergenceWS->mutableY(i)[0] = divergence;
  }
  g_log.notice() << "total solid angle " << solidangletotal << "\n";

  setProperty("OutputWorkspace", divergenceWS);
}

} // namespace Algorithms
} // namespace Mantid
