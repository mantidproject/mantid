// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidCrystal/NormaliseVanadium.h"
#include "MantidAPI/Axis.h"
#include "MantidAPI/InstrumentValidator.h"
#include "MantidAPI/SpectrumInfo.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidGeometry/Instrument.h"
#include "MantidKernel/BoundedValidator.h"
#include "MantidKernel/Fast_Exponential.h"
#include "MantidKernel/Unit.h"

/*  Following A.J.Schultz's anvred, scaling the vanadium spectra:
 */

namespace Mantid {
namespace Crystal {

// Register the class into the algorithm factory
DECLARE_ALGORITHM(NormaliseVanadium)

using namespace Kernel;
using namespace Geometry;
using namespace API;
using namespace DataObjects;

NormaliseVanadium::NormaliseVanadium() : API::Algorithm() {}

void NormaliseVanadium::init() {
  // The input workspace must have an instrument and units of wavelength
  auto wsValidator = boost::make_shared<InstrumentValidator>();

  declareProperty(std::make_unique<WorkspaceProperty<>>(
                      "InputWorkspace", "", Direction::Input, wsValidator),
                  "The X values for the input workspace must be in units of "
                  "wavelength or TOF");
  declareProperty(std::make_unique<WorkspaceProperty<>>("OutputWorkspace", "",
                                                        Direction::Output),
                  "Output workspace name");

  auto mustBePositive = boost::make_shared<BoundedValidator<double>>();
  mustBePositive->setLower(0.0);
  declareProperty("Wavelength", 1.0, mustBePositive,
                  "Normalizes spectra to this wavelength");
}

void NormaliseVanadium::exec() {
  // Retrieve the input workspace
  m_inputWS = getProperty("InputWorkspace");
  std::string unitStr = m_inputWS->getAxis(0)->unit()->unitID();

  // Get the input parameters
  double lambdanorm = getProperty("Wavelength"); // in 1/cm

  MatrixWorkspace_sptr correctionFactors =
      WorkspaceFactory::Instance().create(m_inputWS);

  const int64_t numHists =
      static_cast<int64_t>(m_inputWS->getNumberHistograms());
  const int64_t specSize = static_cast<int64_t>(m_inputWS->blocksize());

  // If sample not at origin, shift cached positions.
  const auto &spectrumInfo = m_inputWS->spectrumInfo();
  double L1 = spectrumInfo.l1();

  Progress prog(this, 0.0, 1.0, numHists);
  // Loop over the spectra
  PARALLEL_FOR_IF(Kernel::threadSafe(*m_inputWS, *correctionFactors))
  for (int64_t i = 0; i < int64_t(numHists); ++i) {
    //    PARALLEL_START_INTERUPT_REGION //FIXME: Restore

    // Get a reference to the Y's in the output WS for storing the factors
    auto &Y = correctionFactors->mutableY(i);
    auto &E = correctionFactors->mutableE(i);

    // Copy over bin boundaries
    const auto &inSpec = m_inputWS->getSpectrum(i);
    correctionFactors->setSharedX(i, inSpec.sharedX());
    const auto &Yin = inSpec.y();
    const auto &Ein = inSpec.e();

    // If no detector is found, skip onto the next spectrum
    if (!spectrumInfo.hasDetectors(i))
      continue;

    // This is the scattered beam direction
    double L2 = spectrumInfo.l2(i);
    // Two-theta = polar angle = scattering angle = between +Z vector and the
    // scattered beam
    double scattering = spectrumInfo.twoTheta(i);

    Mantid::Kernel::Units::Wavelength wl;
    auto timeflight = inSpec.points();
    if (unitStr == "TOF")
      wl.fromTOF(timeflight.mutableRawData(), timeflight.mutableRawData(), L1,
                 L2, scattering, 0, 0, 0);

    // Loop through the bins in the current spectrum
    double lambp = 0;
    double lambm = 0;
    double normp = 0;
    double normm = 0;
    for (int64_t j = 0; j < specSize; j++) {
      const double lambda = timeflight[j];
      if (lambda > lambdanorm) {
        lambp = lambda;
        normp = Yin[j];
        break;
      }
      lambm = lambda;
      normm = Yin[j];
    }
    double normvalue =
        normm + (lambdanorm - lambm) * (normp - normm) / (lambp - lambm);
    for (int64_t j = 0; j < specSize; j++) {
      Y[j] = Yin[j] / normvalue;
      E[j] = Ein[j] / normvalue;
    }

    prog.report();

    //    PARALLEL_END_INTERUPT_REGION
  }
  //  PARALLEL_CHECK_INTERUPT_REGION

  setProperty("OutputWorkspace", correctionFactors);
}

} // namespace Crystal
} // namespace Mantid
