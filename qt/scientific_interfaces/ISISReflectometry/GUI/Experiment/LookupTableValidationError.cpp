// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "LookupTableValidationError.h"

#include <utility>

#include "LookupCriteriaError.h"
namespace MantidQt::CustomInterfaces::ISISReflectometry {

LookupTableValidationError::LookupTableValidationError(

    std::vector<InvalidLookupRowCells> validationErrors, std::optional<LookupCriteriaError> fullTableError)
    : m_validationErrors(std::move(validationErrors)), m_fullTableError(std::move(fullTableError)) {}

std::vector<InvalidLookupRowCells> const &LookupTableValidationError::errors() const { return m_validationErrors; }

std::optional<LookupCriteriaError> LookupTableValidationError::fullTableError() const { return m_fullTableError; }
} // namespace MantidQt::CustomInterfaces::ISISReflectometry
