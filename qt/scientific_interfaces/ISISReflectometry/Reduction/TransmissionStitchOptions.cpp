// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "TransmissionStitchOptions.h"

namespace MantidQt {
namespace CustomInterfaces {
namespace ISISReflectometry {

TransmissionStitchOptions::TransmissionStitchOptions()
    : m_overlapRange(boost::none), m_rebinParameters(), m_scaleRHS(false) {}

TransmissionStitchOptions::TransmissionStitchOptions(boost::optional<RangeInLambda> overlapRange,
                                                     RebinParameters rebinParameters, bool scaleRHS)
    : m_overlapRange(std::move(overlapRange)), m_rebinParameters(std::move(rebinParameters)), m_scaleRHS(scaleRHS) {}

boost::optional<RangeInLambda> TransmissionStitchOptions::overlapRange() const { return m_overlapRange; }

RebinParameters TransmissionStitchOptions::rebinParameters() const { return m_rebinParameters; }

bool TransmissionStitchOptions::scaleRHS() const { return m_scaleRHS; }

bool operator==(TransmissionStitchOptions const &lhs, TransmissionStitchOptions const &rhs) {
  return lhs.overlapRange() == rhs.overlapRange() && lhs.rebinParameters() == rhs.rebinParameters() &&
         lhs.scaleRHS() == rhs.scaleRHS();
}

bool operator!=(TransmissionStitchOptions const &lhs, TransmissionStitchOptions const &rhs) { return !(lhs == rhs); }
} // namespace ISISReflectometry
} // namespace CustomInterfaces
} // namespace MantidQt
