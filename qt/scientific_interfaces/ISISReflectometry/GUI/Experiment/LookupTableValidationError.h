// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once
#include "../../Reduction/LookupRow.h"
#include "InvalidLookupRowCells.h"
#include "LookupCriteriaError.h"
#include <vector>

namespace MantidQt {
namespace CustomInterfaces {
namespace ISISReflectometry {

class MANTIDQT_ISISREFLECTOMETRY_DLL LookupTableValidationError {
public:
  LookupTableValidationError(std::vector<InvalidLookupRowCells> validationErrors,
                             std::optional<LookupCriteriaError> fullTableError);

  std::vector<InvalidLookupRowCells> const &errors() const;
  std::optional<LookupCriteriaError> fullTableError() const;

private:
  std::vector<InvalidLookupRowCells> m_validationErrors;
  std::optional<LookupCriteriaError> m_fullTableError;
};

} // namespace ISISReflectometry
} // namespace CustomInterfaces
} // namespace MantidQt
