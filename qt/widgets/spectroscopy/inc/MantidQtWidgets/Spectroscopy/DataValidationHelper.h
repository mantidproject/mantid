// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "DllConfig.h"
#include "MantidQtWidgets/Common/UserInputValidator.h"

namespace DataValidationHelper {

enum DataType { Red, Sqw, Calib, Corrections };

MANTID_SPECTROSCOPY_DLL bool validateDataIsOneOf(MantidQt::CustomInterfaces::IUserInputValidator *uiv,
                                                 MantidQt::MantidWidgets::DataSelector *dataSelector,
                                                 std::string const &inputType, DataType const &primaryType,
                                                 std::vector<DataType> const &otherTypes, bool const silent = false,
                                                 bool const autoLoad = true);

MANTID_SPECTROSCOPY_DLL bool validateDataIsOfType(MantidQt::CustomInterfaces::IUserInputValidator *uiv,
                                                  MantidQt::MantidWidgets::DataSelector *dataSelector,
                                                  std::string const &inputType, DataType const &type,
                                                  bool const silent = false, bool const autoLoad = true);

MANTID_SPECTROSCOPY_DLL bool validateDataIsAReducedFile(MantidQt::CustomInterfaces::IUserInputValidator *uiv,
                                                        MantidQt::MantidWidgets::DataSelector *dataSelector,
                                                        std::string const &inputType, bool const silent = false,
                                                        bool const autoLoad = true);

MANTID_SPECTROSCOPY_DLL bool validateDataIsASqwFile(MantidQt::CustomInterfaces::IUserInputValidator *uiv,
                                                    MantidQt::MantidWidgets::DataSelector *dataSelector,
                                                    std::string const &inputType, bool const silent = false,
                                                    bool const autoLoad = true);

MANTID_SPECTROSCOPY_DLL bool validateDataIsACalibrationFile(MantidQt::CustomInterfaces::IUserInputValidator *uiv,
                                                            MantidQt::MantidWidgets::DataSelector *dataSelector,
                                                            std::string const &inputType, bool const silent = false,
                                                            bool const autoLoad = true);

MANTID_SPECTROSCOPY_DLL bool validateDataIsACorrectionsFile(MantidQt::CustomInterfaces::IUserInputValidator *uiv,
                                                            MantidQt::MantidWidgets::DataSelector *dataSelector,
                                                            std::string const &inputType, bool const silent = false,
                                                            bool const autoLoad = true);

} // namespace DataValidationHelper
