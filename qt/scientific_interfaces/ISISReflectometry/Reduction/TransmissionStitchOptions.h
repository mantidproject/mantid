// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once
#include "Common/DllConfig.h"
#include "ProcessingInstructions.h"
#include "RangeInLambda.h"
#include <boost/optional.hpp>
#include <optional>
#include <string>
namespace MantidQt {
namespace CustomInterfaces {
namespace ISISReflectometry {

// For now just pass rebin params as a string but in future we may want to
// expand this
using RebinParameters = std::string;

/** @class TransmissionStitchOptions

    The TransmissionStitchOptions model holds information about stitching
   properties that should be applied during reduction for the transmission
   runs, if both the first and second transmission runs are specified.
 */
class MANTIDQT_ISISREFLECTOMETRY_DLL TransmissionStitchOptions {
public:
  TransmissionStitchOptions();
  TransmissionStitchOptions(std::optional<RangeInLambda> overlapRange, RebinParameters rebinParameters, bool scaleRHS);

  std::optional<RangeInLambda> overlapRange() const;
  RebinParameters rebinParameters() const;
  bool scaleRHS() const;

private:
  std::optional<RangeInLambda> m_overlapRange;
  RebinParameters m_rebinParameters;
  bool m_scaleRHS;
};

MANTIDQT_ISISREFLECTOMETRY_DLL bool operator==(TransmissionStitchOptions const &lhs,
                                               TransmissionStitchOptions const &rhs);
MANTIDQT_ISISREFLECTOMETRY_DLL bool operator!=(TransmissionStitchOptions const &lhs,
                                               TransmissionStitchOptions const &rhs);
} // namespace ISISReflectometry
} // namespace CustomInterfaces
} // namespace MantidQt
