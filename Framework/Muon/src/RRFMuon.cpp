// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidMuon/RRFMuon.h"
#include "MantidAPI/Axis.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidKernel/Unit.h"

namespace Mantid {
namespace Algorithms {

using namespace Kernel;

// Register the class into the algorithm factory
DECLARE_ALGORITHM(RRFMuon)

/** Initialisation method. Declares properties to be used in algorithm.
 *
 */
void RRFMuon::init() {

  declareProperty(
      make_unique<API::WorkspaceProperty<API::MatrixWorkspace>>(
          "InputWorkspace", "", Direction::Input),
      "Name of the input workspace containing the spectra in the lab frame");

  declareProperty(
      make_unique<API::WorkspaceProperty<API::MatrixWorkspace>>(
          "OutputWorkspace", "", Direction::Output),
      "Name of the output workspace containing the spectra in the RRF");

  declareProperty(
      make_unique<PropertyWithValue<double>>("Frequency", 0, Direction::Input),
      "Frequency of the oscillations");

  std::vector<std::string> unitOptions{"MHz", "Gauss", "Mrad/s"};
  declareProperty("FrequencyUnits", "MHz",
                  boost::make_shared<StringListValidator>(unitOptions),
                  "The frequency units");

  declareProperty(
      make_unique<PropertyWithValue<double>>("Phase", 0, Direction::Input),
      "Phase accounting for any misalignment of the counters");
}

/** Executes the algorithm
 *
 */
void RRFMuon::exec() {
  // Get input workspace containing polarization in the lab-frame
  API::MatrixWorkspace_sptr inputWs = getProperty("InputWorkspace");
  // Get frequency
  double freq = getProperty("Frequency");
  // Get units
  std::string units = getProperty("FrequencyUnits");
  // Convert frequency to input workspace X units
  double factor =
      unitConversionFactor(inputWs->getAxis(0)->unit()->label().ascii(), units);
  // Get phase
  double phase = getProperty("Phase");
  // Get number of histograms
  const size_t nHisto = inputWs->getNumberHistograms();
  if (nHisto != 2) {
    throw std::runtime_error("Invalid number of spectra in input workspace");
  }
  // Set number of data points
  const size_t nData = inputWs->blocksize();

  // Compute the RRF polarization
  const double twoPiFreq = 2. * M_PI * freq * factor;
  const auto &time = inputWs->x(0);  // X axis: time
  const auto &labRe = inputWs->y(0); // Lab-frame polarization (real part)
  const auto &labIm = inputWs->y(1); // Lab-frame polarization (imaginary part)

  MantidVec rrfRe(nData),
      rrfIm(nData); // Rotating Reference frame (RRF) polarizations
  for (size_t t = 0; t < nData; t++) {
    rrfRe[t] = labRe[t] * cos(twoPiFreq * time[t] + phase) +
               labIm[t] * sin(twoPiFreq * time[t] + phase);
    rrfIm[t] = -labRe[t] * sin(twoPiFreq * time[t] + phase) +
               labIm[t] * cos(twoPiFreq * time[t] + phase);
  }

  // Create output workspace to put results into
  API::MatrixWorkspace_sptr outputWs =
      boost::dynamic_pointer_cast<API::MatrixWorkspace>(
          API::WorkspaceFactory::Instance().create("Workspace2D", nHisto,
                                                   nData + 1, nData));
  outputWs->getAxis(0)->unit() = inputWs->getAxis(0)->unit();

  // Put results into output workspace
  // Real RRF polarization
  outputWs->setSharedX(0, inputWs->sharedX(0));
  outputWs->mutableY(0) = std::move(rrfRe);
  // Imaginary RRF polarization
  outputWs->setSharedX(1, inputWs->sharedX(1));
  outputWs->mutableY(1) = std::move(rrfIm);

  // Set output workspace
  setProperty("OutputWorkspace", outputWs);
}

/** Gets factor to convert frequency units to input workspace units, typically
 * in microseconds
 *  @param uin :: [input] input workspace units
 *  @param uuser :: [input] units selected by user
 */
double RRFMuon::unitConversionFactor(std::string uin, std::string uuser) {

  if ((uin == "microsecond")) {

    if (uuser == "MHz") {
      return 1.0;
    } else if (uuser == "Gauss") {
      // Factor = 2 * PI * MU
      // where MU is the muon gyromagnetic ratio:
      // MU = 135.538817 MHz/T, 1T = 10000 Gauss
      return 2.0 * M_PI * 135.538817 * 0.0001;
    } else if (uuser == "Mrad/s") {
      // Factor = 2 * PI
      return 2.0 * M_PI;
    } else {
      throw std::runtime_error("Could not find units");
    }

  } else {
    throw std::runtime_error("X units must be in microseconds");
  }
}
} // namespace Algorithms
} // namespace Mantid
