// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2016 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_ALGORITHMS_MCABSORPTIONSTRATEGY_H_
#define MANTID_ALGORITHMS_MCABSORPTIONSTRATEGY_H_

#include "MantidAPI/ISpectrum.h"
#include "MantidAlgorithms/DllConfig.h"
#include "MantidAlgorithms/InterpolationOption.h"
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
  MCAbsorptionStrategy(const IBeamProfile &beamProfile,
                       const API::Sample &sample,
                       Kernel::DeltaEMode::Type EMode, const size_t nevents,
                       const int nlambda, const size_t maxScatterPtAttempts,
                       const bool useSparseInstrument,
                       const InterpolationOption &interpolateOpt,
                       const bool regenerateTracksForEachLambda,
                       Kernel::Logger &logger);
  void calculate(Kernel::PseudoRandomNumberGenerator &rng,
                 const Kernel::V3D &finalPos,
                 Mantid::HistogramData::Points lambdas, double lambdaFixed,
                 Mantid::API::ISpectrum &attenuationFactorsSpectrum);

private:
  const IBeamProfile &m_beamProfile;
  MCInteractionVolume m_scatterVol;
  const size_t m_nevents;
  const size_t m_maxScatterAttempts;
  const double m_error;
  const Kernel::DeltaEMode::Type m_EMode;
  const int m_nlambda;
  const bool m_useSparseInstrument;
  const InterpolationOption &m_interpolateOpt;
  const bool m_regenerateTracksForEachLambda;
};

} // namespace Algorithms
} // namespace Mantid

#endif /* MANTID_ALGORITHMS_MCABSORPTIONSTRATEGY_H_ */
