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
#include "MantidHistogramData/Slice.h"

namespace Mantid {
namespace Algorithms {

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(ExtractFFTSpectrum)

using namespace Kernel;
using namespace API;
using namespace DataObjects;

void ExtractFFTSpectrum::init() {
  declareProperty(std::make_unique<WorkspaceProperty<>>("InputWorkspace", "",
                                                        Direction::Input),
                  "The input workspace.");
  // if desired, provide the imaginary part in a separate workspace.
  declareProperty(std::make_unique<WorkspaceProperty<>>("InputImagWorkspace",
                                                        "", Direction::Input,
                                                        PropertyMode::Optional),
                  "The optional input workspace for the imaginary part.");
  declareProperty("FFTPart", 2, std::make_shared<BoundedValidator<int>>(0, 5),
                  "Spectrum number, one of the six possible spectra output by "
                  "the FFT algorithm");
  declareProperty(std::make_unique<WorkspaceProperty<>>("OutputWorkspace", "",
                                                        Direction::Output),
                  "The output workspace.");
  declareProperty("Shift", 0.0,
                  "Apply an extra phase equal to this quantity "
                  "times 2*pi to the transform");
  declareProperty("AutoShift", false,
                  "Automatically calculate and apply phase shift. Zero on the "
                  "X axis is assumed to be in the centre - if it is not, "
                  "setting this property will automatically correct for this.");
  declareProperty("AcceptXRoundingErrors", false,
                  "Continue to process the data even if X values are not evenly spaced",
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
  std::vector<int> chopIndex(numHists, 0);  // we use this to chop tail zeros if necessary

  PARALLEL_FOR_IF(Kernel::threadSafe(*outputWS))
  for (int i = 0; i < numHists; i++) {
    PARALLEL_START_INTERUPT_REGION

    IAlgorithm_sptr childFFT = createChildAlgorithm("FFT");
    childFFT->setProperty<MatrixWorkspace_sptr>("InputWorkspace", inputWS);
    childFFT->setProperty<int>("Real", i);
    if (inputImagWS) {
      childFFT->setProperty<MatrixWorkspace_sptr>("InputImagWorkspace",
                                                  inputImagWS);
      childFFT->setProperty<int>("Imaginary", i);
    }
    childFFT->setProperty<double>("Shift", shift);
    childFFT->setProperty<bool>("AutoShift", autoShift);
    childFFT->setProperty<bool>("AcceptXRoundingErrors", xRoundingErrs);
    childFFT->execute();
    MatrixWorkspace_const_sptr fftTemp =
        childFFT->getProperty("OutputWorkspace");

    auto &histo = fftTemp->histogram(fftPart);

    if (!inputImagWS) {
      // with only real input, the tail of the histo is just zeros
      // so we need to trim them 
      const auto &yData = histo.y();
      auto results = std::find_if(yData.rbegin(), yData.rend(),
                                  [](double v) { return v != 0; });
      // for histogram i, the data after and including chopIndex[i] are zeros.
      chopIndex[i] = std::distance(yData.begin(), results.base());
    }
    outputWS->setHistogram(i, histo);

    prog.report();

    PARALLEL_END_INTERUPT_REGION
  }
  PARALLEL_CHECK_INTERUPT_REGION
  
  // chop all tail zeros where it is safe to do so - we don't want to remove actual data
  // maxChopIter is the maximum 'safe' x value to chop (will only remove 0's)
  const auto maxChopIter = std::max_element(chopIndex.cbegin(), chopIndex.cend());
  if (*maxChopIter != 0) {  // if the input had imaginary component, we won't chop.
    const int wsIndex = std::distance(chopIndex.cbegin(), maxChopIter); // the row where our max value is
    // now we get the x value at our max value
    const double xMax = outputWS->x(wsIndex)[*maxChopIter - 1];

    IAlgorithm_sptr extractSpectra = createChildAlgorithm("ExtractSpectra");
    extractSpectra->setProperty<MatrixWorkspace_sptr>("InputWorkspace", outputWS);
    extractSpectra->setProperty<MatrixWorkspace_sptr>("OutputWorkspace", outputWS);
    extractSpectra->setProperty("XMax", xMax);
    extractSpectra->execute();
  }

  std::shared_ptr<Kernel::Units::Label> lblUnit =
      std::dynamic_pointer_cast<Kernel::Units::Label>(
          UnitFactory::Instance().create("Label"));
  lblUnit->setLabel("Time", "ns");
  outputWS->getAxis(0)->unit() = lblUnit;

  setProperty("OutputWorkspace", outputWS);
}
} // namespace Algorithms
} // namespace Mantid
