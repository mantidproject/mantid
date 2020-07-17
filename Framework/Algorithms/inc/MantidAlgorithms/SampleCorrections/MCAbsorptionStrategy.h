// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2016 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/ISpectrum.h"
#include "MantidAlgorithms/DllConfig.h"
#include "MantidAlgorithms/SampleCorrections/MCInteractionStatistics.h"
#include "MantidAlgorithms/SampleCorrections/MCInteractionVolume.h"
#include "MantidHistogramData/Histogram.h"
#include "MantidKernel/DeltaEMode.h"
#include <tuple>

namespace Mantid {
namespace API {
class Sample;
}
namespace Kernel {
class PseudoRandomNumberGenerator;
class V3D;
class Logger;
} // namespace Kernel
namespace Algorithms {
class IBeamProfile;
class MonteCarloAbsorption;

/**
  Implements the algorithm for calculating the correction factor for
  self-attenuation and single wavelength using a Monte Carlo. A single
  instance has a fixed nominal source position, nominal sample
  position & sample + containers shapes.

  The error on all points is defined to be \f$\frac{1}{\sqrt{N}}\f$, where N is
  the number of events generated.
*/
class MANTID_ALGORITHMS_DLL MCAbsorptionStrategy {
public:
  MCAbsorptionStrategy(IMCInteractionVolume &interactionVolume,
                       const IBeamProfile &beamProfile,
                       Kernel::DeltaEMode::Type EMode, const size_t nevents,
                       const size_t maxScatterPtAttempts,
                       const bool regenerateTracksForEachLambda);
  virtual void calculate(Kernel::PseudoRandomNumberGenerator &rng,
                 const Kernel::V3D &finalPos,
                 const std::vector<double> &lambdas, const double lambdaFixed,
                 std::vector<double> &attenuationFactors,
                 std::vector<double> &attFactorErrors,
                 MCInteractionStatistics &stats);

private:
  const IBeamProfile &m_beamProfile;
  const IMCInteractionVolume &m_scatterVol;
  const size_t m_nevents;
  const size_t m_maxScatterAttempts;
  const double m_error;
  const Kernel::DeltaEMode::Type m_EMode;
  const bool m_regenerateTracksForEachLambda;
};

} // namespace Algorithms
} // namespace Mantid
