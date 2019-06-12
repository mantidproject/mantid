// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidAlgorithms/SolidAngle.h"
#include "MantidAPI/InstrumentValidator.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/SpectrumInfo.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidGeometry/IComponent.h"
#include "MantidGeometry/IDetector.h"
#include "MantidGeometry/Instrument.h"
#include "MantidGeometry/Instrument/DetectorInfo.h"
#include "MantidKernel/BoundedValidator.h"
#include "MantidKernel/UnitFactory.h"

#include <cfloat>

namespace Mantid {
namespace Algorithms {

// Register with the algorithm factory
DECLARE_ALGORITHM(SolidAngle)

using namespace Kernel;
using namespace API;

/// Initialisation method
void SolidAngle::init() {
  declareProperty(std::make_unique<WorkspaceProperty<API::MatrixWorkspace>>(
                      "InputWorkspace", "", Direction::Input,
                      boost::make_shared<InstrumentValidator>()),
                  "This workspace is used to identify the instrument to use "
                  "and also which\n"
                  "spectra to create a solid angle for. If the Max and Min "
                  "spectra values are\n"
                  "not provided one solid angle will be created for each "
                  "spectra in the input\n"
                  "workspace");
  declareProperty(std::make_unique<WorkspaceProperty<API::MatrixWorkspace>>(
                      "OutputWorkspace", "", Direction::Output),
                  "The name of the workspace to be created as the output of "
                  "the algorithm.  A workspace of this name will be created "
                  "and stored in the Analysis Data Service.");

  auto mustBePositive = boost::make_shared<BoundedValidator<int>>();
  mustBePositive->setLower(0);
  declareProperty("StartWorkspaceIndex", 0, mustBePositive,
                  "The index number of the first spectrum for which to find "
                  "the solid angle\n"
                  "(default: 0)");
  declareProperty("EndWorkspaceIndex", EMPTY_INT(), mustBePositive,
                  "The index of the last spectrum whose solid angle is to be "
                  "found (default: the\n"
                  "last spectrum in the workspace)");
}

/** Executes the algorithm
 */
void SolidAngle::exec() {
  // Get the workspaces
  API::MatrixWorkspace_const_sptr inputWS = getProperty("InputWorkspace");
  int m_MinSpec = getProperty("StartWorkspaceIndex");
  int m_MaxSpec = getProperty("EndWorkspaceIndex");

  const int numberOfSpectra = static_cast<int>(inputWS->getNumberHistograms());

  // Check 'StartSpectrum' is in range 0-numberOfSpectra
  if (m_MinSpec > numberOfSpectra) {
    g_log.warning("StartWorkspaceIndex out of range! Set to 0.");
    m_MinSpec = 0;
  }
  if (isEmpty(m_MaxSpec))
    m_MaxSpec = numberOfSpectra - 1;
  if (m_MaxSpec > numberOfSpectra - 1 || m_MaxSpec < m_MinSpec) {
    g_log.warning("EndWorkspaceIndex out of range! Set to max detector number");
    m_MaxSpec = numberOfSpectra - 1;
  }

  API::MatrixWorkspace_sptr outputWS = WorkspaceFactory::Instance().create(
      inputWS, m_MaxSpec - m_MinSpec + 1, 2, 1);
  // The result of this will be a distribution
  outputWS->setDistribution(true);
  outputWS->setYUnit("");
  outputWS->setYUnitLabel("Steradian");
  setProperty("OutputWorkspace", outputWS);

  const auto &spectrumInfo = inputWS->spectrumInfo();
  const auto &detectorInfo = inputWS->detectorInfo();
  const Kernel::V3D samplePos = spectrumInfo.samplePosition();
  g_log.debug() << "Sample position is " << samplePos << '\n';

  const int loopIterations = m_MaxSpec - m_MinSpec;
  int failCount = 0;
  Progress prog(this, 0.0, 1.0, numberOfSpectra);

  // Loop over the histograms (detector spectra)
  PARALLEL_FOR_IF(Kernel::threadSafe(*outputWS, *inputWS))
  for (int j = 0; j <= loopIterations; ++j) {
    PARALLEL_START_INTERUPT_REGION
    const int i = j + m_MinSpec;
    if (spectrumInfo.hasDetectors(i)) {
      // Copy over the spectrum number & detector IDs
      outputWS->getSpectrum(j).copyInfoFrom(inputWS->getSpectrum(i));
      double solidAngle = 0.0;
      for (const auto detID : inputWS->getSpectrum(i).getDetectorIDs()) {
        const auto index = detectorInfo.indexOf(detID);
        if (!detectorInfo.isMasked(index))
          solidAngle += detectorInfo.detector(index).solidAngle(samplePos);
      }

      outputWS->mutableX(j)[0] = inputWS->x(i).front();
      outputWS->mutableX(j)[1] = inputWS->x(i).back();
      outputWS->mutableY(j)[0] = solidAngle;
      outputWS->mutableE(j)[0] = 0;
    } else {
      failCount++;
      outputWS->mutableX(j) = 0;
      outputWS->mutableY(j) = 0;
      outputWS->mutableE(j) = 0;
    }

    prog.report();
    PARALLEL_END_INTERUPT_REGION
  } // loop over spectra
  PARALLEL_CHECK_INTERUPT_REGION

  if (failCount != 0) {
    g_log.information() << "Unable to calculate solid angle for " << failCount
                        << " spectra. Zeroing these spectra.\n";
  }
}

} // namespace Algorithms
} // namespace Mantid
