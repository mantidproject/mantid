// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_CUSTOMINTERFACES_INDIRECTDATAVALIDATIONHELPER_H_
#define MANTID_CUSTOMINTERFACES_INDIRECTDATAVALIDATIONHELPER_H_

#include "MantidQtWidgets/Common/UserInputValidator.h"

namespace MantidQt {
namespace CustomInterfaces {

enum DataType { Red, Sqw, Calib, Corrections } const;

bool validateDataIsOneOf(UserInputValidator &uiv,
                         MantidWidgets::DataSelector *dataSelector,
                         std::string const &inputType,
                         DataType const &primaryType,
                         std::vector<DataType> const &otherTypes,
                         bool silent = false);

bool validateDataIsOfType(UserInputValidator &uiv,
                          MantidWidgets::DataSelector *dataSelector,
                          std::string const &inputType, DataType const &type,
                          bool silent = false);

bool validateDataIsAReducedFile(UserInputValidator &uiv,
                                MantidWidgets::DataSelector *dataSelector,
                                std::string const &inputType,
                                bool silent = false);

bool validateDataIsASqwFile(UserInputValidator &uiv,
                            MantidWidgets::DataSelector *dataSelector,
                            std::string const &inputType, bool silent = false);

bool validateDataIsACalibrationFile(UserInputValidator &uiv,
                                    MantidWidgets::DataSelector *dataSelector,
                                    std::string const &inputType,
                                    bool silent = false);

bool validateDataIsACorrectionsFile(UserInputValidator &uiv,
                                    MantidWidgets::DataSelector *dataSelector,
                                    std::string const &inputType,
                                    bool silent = false);

} // namespace CustomInterfaces
} // namespace MantidQt

#endif /* MANTID_CUSTOMINTERFACES_INDIRECTDATAVALIDATIONHELPER_H_ */
