// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "../DllConfig.h"
#include "MantidQtWidgets/Common/UserInputValidator.h"

namespace IndirectDataValidationHelper {

enum DataType { Red, Sqw, Calib, Corrections };

MANTIDQT_INELASTIC_DLL bool validateDataIsOneOf(MantidQt::CustomInterfaces::UserInputValidator &uiv,
                                                MantidQt::MantidWidgets::DataSelector *dataSelector,
                                                std::string const &inputType, DataType const &primaryType,
                                                std::vector<DataType> const &otherTypes, bool silent = false);

MANTIDQT_INELASTIC_DLL bool validateDataIsOfType(MantidQt::CustomInterfaces::UserInputValidator &uiv,
                                                 MantidQt::MantidWidgets::DataSelector *dataSelector,
                                                 std::string const &inputType, DataType const &type,
                                                 bool silent = false);

MANTIDQT_INELASTIC_DLL bool validateDataIsAReducedFile(MantidQt::CustomInterfaces::UserInputValidator &uiv,
                                                       MantidQt::MantidWidgets::DataSelector *dataSelector,
                                                       std::string const &inputType, bool silent = false);

MANTIDQT_INELASTIC_DLL bool validateDataIsASqwFile(MantidQt::CustomInterfaces::UserInputValidator &uiv,
                                                   MantidQt::MantidWidgets::DataSelector *dataSelector,
                                                   std::string const &inputType, bool silent = false);

MANTIDQT_INELASTIC_DLL bool validateDataIsACalibrationFile(MantidQt::CustomInterfaces::UserInputValidator &uiv,
                                                           MantidQt::MantidWidgets::DataSelector *dataSelector,
                                                           std::string const &inputType, bool silent = false);

MANTIDQT_INELASTIC_DLL bool validateDataIsACorrectionsFile(MantidQt::CustomInterfaces::UserInputValidator &uiv,
                                                           MantidQt::MantidWidgets::DataSelector *dataSelector,
                                                           std::string const &inputType, bool silent = false);

} // namespace IndirectDataValidationHelper
