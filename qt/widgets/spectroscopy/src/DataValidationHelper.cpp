// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidQtWidgets/Spectroscopy/DataValidationHelper.h"

#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/WorkspaceGroup.h"

using namespace Mantid::API;
using namespace MantidQt::MantidWidgets;
using namespace MantidQt::CustomInterfaces;

namespace DataValidationHelper {

/**
 * Validates that the data selector contains data which is of one of the types
 * specified.
 *
 * @param uiv The UserInputValidator used to generate any error messages.
 * @param dataSelector The DataSelector containing the data.
 * @param inputType The type of the input (e.g. Sample or Container).
 * @param types The types the data is allowed to be (e.g. red or sqw).
 * @param primaryType The most important type.
 * @return True if the data is valid.
 */
bool validateDataIsOneOf(IUserInputValidator *uiv, DataSelector *dataSelector, std::string const &inputType,
                         DataType const &primaryType, std::vector<DataType> const &otherTypes, bool silent) {
  if (std::any_of(otherTypes.cbegin(), otherTypes.cend(),
                  [&](auto const &type) { return validateDataIsOfType(uiv, dataSelector, inputType, type, true); })) {
    return true;
  }

  return validateDataIsOfType(uiv, dataSelector, inputType, primaryType, silent);
}

/**
 * Validates that the data selector contains data of the type specified.
 *
 * @param uiv The UserInputValidator used to generate any error messages.
 * @param dataSelector The DataSelector containing the data.
 * @param inputType The type of the input (e.g. Sample or Container).
 * @param type The type of the data (e.g. red or sqw).
 * @return True if the data is valid.
 */
bool validateDataIsOfType(IUserInputValidator *uiv, DataSelector *dataSelector, std::string const &inputType,
                          DataType const &type, bool silent) {
  switch (type) {
  case DataType::Red:
    return validateDataIsAReducedFile(uiv, dataSelector, inputType, silent);
  case DataType::Sqw:
    return validateDataIsASqwFile(uiv, dataSelector, inputType, silent);
  case DataType::Calib:
    return validateDataIsACalibrationFile(uiv, dataSelector, inputType, silent);
  case DataType::Corrections:
    return validateDataIsACorrectionsFile(uiv, dataSelector, inputType, silent);
  default:
    return false;
  }
}

/**
 * Validates that the data selector is holding a reduced file or workspace.
 *
 * @param uiv The UserInputValidator used to generate any error
 * messages.
 * @param dataSelector The DataSelector containing the data.
 * @param inputType The type of the input (e.g. Sample or
 * Container).
 * @return True if the data is valid.
 */
bool validateDataIsAReducedFile(IUserInputValidator *uiv, DataSelector *dataSelector, std::string const &inputType,
                                bool silent) {
  auto const dataName = dataSelector->getCurrentDataName();
  uiv->checkDataSelectorIsValid(QString::fromStdString(inputType), dataSelector, silent);
  uiv->checkWorkspaceType<MatrixWorkspace>(dataName, QString::fromStdString(inputType), "MatrixWorkspace", silent);
  // TODO :: check the axis labels for the data units
  return uiv->isAllInputValid();
}

/**
 * Validates that the data selector is holding a S(Q,w) file or workspace.
 *
 * @param uiv The UserInputValidator used to generate any error messages.
 * @param dataSelector The DataSelector containing the data.
 * @param inputType The type of the input (e.g. Sample or Container).
 * @return True if the data is valid.
 */
bool validateDataIsASqwFile(IUserInputValidator *uiv, DataSelector *dataSelector, std::string const &inputType,
                            bool silent) {
  auto const dataName = dataSelector->getCurrentDataName();
  uiv->checkDataSelectorIsValid(QString::fromStdString(inputType), dataSelector, silent);
  uiv->checkWorkspaceType<MatrixWorkspace>(dataName, QString::fromStdString(inputType), "MatrixWorkspace", silent);
  // TODO :: check the axis labels for the data units
  return uiv->isAllInputValid();
}

/**
 * Validates that the data selector is holding a S(Q,w) file or workspace.
 *
 * @param uiv The UserInputValidator used to generate any error messages.
 * @param dataSelector The DataSelector containing the data.
 * @param inputType The type of the input (e.g. Sample or Container).
 * @return True if the data is valid.
 */
bool validateDataIsACalibrationFile(IUserInputValidator *uiv, DataSelector *dataSelector, std::string const &inputType,
                                    bool silent) {
  auto const dataName = dataSelector->getCurrentDataName();
  uiv->checkDataSelectorIsValid(QString::fromStdString(inputType), dataSelector, silent);
  uiv->checkWorkspaceType<MatrixWorkspace>(dataName, QString::fromStdString(inputType), "MatrixWorkspace", silent);
  // TODO :: check the axis labels for the data units
  return uiv->isAllInputValid();
}

/**
 * Validates that the data selector is holding a corrections file or workspace.
 *
 * @param uiv The UserInputValidator used to generate any error messages.
 * @param dataSelector The DataSelector containing the data.
 * @param inputType The type of the input (e.g. Sample or Container).
 * @return True if the data is valid.
 */
bool validateDataIsACorrectionsFile(IUserInputValidator *uiv, DataSelector *dataSelector, std::string const &inputType,
                                    bool silent) {
  auto const dataName = dataSelector->getCurrentDataName();
  uiv->checkDataSelectorIsValid(QString::fromStdString(inputType), dataSelector, silent);
  uiv->checkWorkspaceType<WorkspaceGroup>(dataName, QString::fromStdString(inputType), "WorkspaceGroup", silent);
  uiv->checkWorkspaceGroupIsValid(dataName, QString::fromStdString(inputType), silent);
  // TODO :: check the axis labels for the data units
  return uiv->isAllInputValid();
}

} // namespace DataValidationHelper
