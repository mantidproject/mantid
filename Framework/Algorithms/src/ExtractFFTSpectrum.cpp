// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAlgorithms/ExtractFFTSpectrum.h"
#include "MantidAPI/Axis.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidDataObjects/WorkspaceCreation.h"
#include "MantidKernel/BoundedValidator.h"
#include "MantidKernel/MultiThreaded.h"
#include "MantidKernel/Unit.h"
#include "MantidKernel/UnitFactory.h"

namespace Mantid::Algorithms {

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(ExtractFFTSpectrum)

using namespace Kernel;
using namespace API;
using namespace DataObjects;

void ExtractFFTSpectrum::init() {
  declareProperty(std::make_unique<WorkspaceProperty<>>("InputWorkspace", "", Direction::Input),
                  "The input workspace.");
  // if desired, provide the imaginary part in a separate workspace.
  declareProperty(
      std::make_unique<WorkspaceProperty<>>("InputImagWorkspace", "", Direction::Input, PropertyMode::Optional),
      "The optional input workspace for the imaginary part.");
  declareProperty("FFTPart", 2, std::make_shared<BoundedValidator<int>>(0, 5),
                  "Spectrum number, one of the six possible spectra output by "
                  "the FFT algorithm");
  declareProperty(std::make_unique<WorkspaceProperty<>>("OutputWorkspace", "", Direction::Output),
                  "The output workspace.");
  declareProperty("Shift", 0.0,
                  "Apply an extra phase equal to this quantity "
                  "times 2*pi to the transform");
  declareProperty("AutoShift", false,
                  "Automatically calculate and apply phase shift. Zero on the "
                  "X axis is assumed to be in the centre - if it is not, "
                  "setting this property will automatically correct for this.");
  declareProperty("AcceptXRoundingErrors", false, "Continue to process the data even if X values are not evenly spaced",
                  Direction::Input);
}

void ExtractFFTSpectrum::exec() {
  MatrixWorkspace_sptr inputWS = getProperty("InputWorkspace");
  MatrixWorkspace_sptr inputImagWS = getProperty("InputImagWorkspace");
  const double shift = getProperty("Shift");
  const bool autoShift = getProperty("AutoShift");
  const bool xRoundingErrs = getProperty("AcceptXRoundingErrors");
  const int fftPart = getProperty("FFTPart");
  const auto numHists = static_cast<int>(inputWS->getNumberHistograms());
  MatrixWorkspace_sptr outputWS = create<MatrixWorkspace>(*inputWS);

  Progress prog(this, 0.0, 1.0, numHists);

  PARALLEL_FOR_IF(Kernel::threadSafe(*outputWS))
  for (int i = 0; i < numHists; i++) {
    PARALLEL_START_INTERRUPT_REGION

    auto childFFT = createChildAlgorithm("FFT");
    childFFT->setProperty<MatrixWorkspace_sptr>("InputWorkspace", inputWS);
    childFFT->setProperty<int>("Real", i);
    if (inputImagWS) {
      childFFT->setProperty<MatrixWorkspace_sptr>("InputImagWorkspace", inputImagWS);
      childFFT->setProperty<int>("Imaginary", i);
    }
    childFFT->setProperty<double>("Shift", shift);
    childFFT->setProperty<bool>("AutoShift", autoShift);
    childFFT->setProperty<bool>("AcceptXRoundingErrors", xRoundingErrs);
    childFFT->execute();
    MatrixWorkspace_const_sptr fftTemp = childFFT->getProperty("OutputWorkspace");
    if (i == 0) {
      outputWS->getAxis(0)->unit() = fftTemp->getAxis(0)->unit();
    }
    outputWS->setHistogram(i, fftTemp->histogram(fftPart));

    prog.report();

    PARALLEL_END_INTERRUPT_REGION
  }
  PARALLEL_CHECK_INTERRUPT_REGION

  if (!inputImagWS && fftPart <= 2) {
    // In this case, trim half of the workspace, as these are just zeros.
    const double xMax = outputWS->x(0)[(outputWS->x(0).size() / 2) - 1];

    auto extractSpectra = createChildAlgorithm("ExtractSpectra");
    extractSpectra->setProperty<MatrixWorkspace_sptr>("InputWorkspace", outputWS);
    extractSpectra->setProperty<MatrixWorkspace_sptr>("OutputWorkspace", outputWS);
    extractSpectra->setProperty("XMax", xMax);
    extractSpectra->execute();
  }

  setProperty("OutputWorkspace", outputWS);
}
} // namespace Mantid::Algorithms
