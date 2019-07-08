// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "IndirectDataValidationHelper.h"

#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/WorkspaceGroup.h"

using namespace Mantid::API;
using namespace MantidQt::MantidWidgets;

namespace MantidQt {
namespace CustomInterfaces {

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
bool validateDataIsOneOf(UserInputValidator &uiv, DataSelector *dataSelector,
                         std::string const &inputType,
                         DataType const &primaryType,
                         std::vector<DataType> const &otherTypes, bool silent) {
  for (auto const type : otherTypes)
    if (validateDataIsOfType(uiv, dataSelector, inputType, type, true))
      return true;

  return validateDataIsOfType(uiv, dataSelector, inputType, primaryType,
                              silent);
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
bool validateDataIsOfType(UserInputValidator &uiv, DataSelector *dataSelector,
                          std::string const &inputType, DataType const &type,
                          bool silent) {
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
bool validateDataIsAReducedFile(UserInputValidator &uiv,
                                DataSelector *dataSelector,
                                std::string const &inputType, bool silent) {
  auto const dataName = dataSelector->getCurrentDataName();
  uiv.checkDataSelectorIsValid(QString::fromStdString(inputType), dataSelector,
                               silent);
  uiv.checkWorkspaceType<MatrixWorkspace, MatrixWorkspace_sptr>(
      dataName, QString::fromStdString(inputType), "MatrixWorkspace", silent);

  return uiv.isAllInputValid();
};

/**
 * Validates that the data selector is holding a S(Q,w) file or workspace.
 *
 * @param uiv The UserInputValidator used to generate any error messages.
 * @param dataSelector The DataSelector containing the data.
 * @param inputType The type of the input (e.g. Sample or Container).
 * @return True if the data is valid.
 */
bool validateDataIsASqwFile(UserInputValidator &uiv, DataSelector *dataSelector,
                            std::string const &inputType, bool silent) {
  auto const dataName = dataSelector->getCurrentDataName();
  uiv.checkDataSelectorIsValid(QString::fromStdString(inputType), dataSelector,
                               silent);
  uiv.checkWorkspaceType<MatrixWorkspace, MatrixWorkspace_sptr>(
      dataName, QString::fromStdString(inputType), "MatrixWorkspace", silent);

  return uiv.isAllInputValid();
};

/**
 * Validates that the data selector is holding a S(Q,w) file or workspace.
 *
 * @param uiv The UserInputValidator used to generate any error messages.
 * @param dataSelector The DataSelector containing the data.
 * @param inputType The type of the input (e.g. Sample or Container).
 * @return True if the data is valid.
 */
bool validateDataIsACalibrationFile(UserInputValidator &uiv,
                                    DataSelector *dataSelector,
                                    std::string const &inputType, bool silent) {
  auto const dataName = dataSelector->getCurrentDataName();
  uiv.checkDataSelectorIsValid(QString::fromStdString(inputType), dataSelector,
                               silent);
  uiv.checkWorkspaceType<MatrixWorkspace, MatrixWorkspace_sptr>(
      dataName, QString::fromStdString(inputType), "MatrixWorkspace", silent);

  return uiv.isAllInputValid();
};

/**
 * Validates that the data selector is holding a corrections file or workspace.
 *
 * @param uiv The UserInputValidator used to generate any error messages.
 * @param dataSelector The DataSelector containing the data.
 * @param inputType The type of the input (e.g. Sample or Container).
 * @return True if the data is valid.
 */
bool validateDataIsACorrectionsFile(UserInputValidator &uiv,
                                    DataSelector *dataSelector,
                                    std::string const &inputType, bool silent) {
  auto const dataName = dataSelector->getCurrentDataName();
  uiv.checkDataSelectorIsValid(QString::fromStdString(inputType), dataSelector,
                               silent);
  uiv.checkWorkspaceType<WorkspaceGroup, WorkspaceGroup_sptr>(
      dataName, QString::fromStdString(inputType), "WorkspaceGroup", silent);
  uiv.checkWorkspaceGroupIsValid(dataName, QString::fromStdString(inputType),
                                 silent);

  return uiv.isAllInputValid();
};

// bool validateDataHasIdenticalDimensions(
//    UserInputValidator &uiv, DataAxis const &dataAxis,
//    std::map<std::string, DataSelector *> const &data) {
//  switch (dataAxis) {
//  case DataAxis::Histograms:
//    return validateDataHasSameNumberOfHistograms(uiv, data);
//  case DataAxis::Bins:
//    return validateDataHasSameNumberOfBins(uiv, data);
//  default:
//    return false;
//  }
//}
//
// bool validateDataHasSameNumberOfHistograms(
//    UserInputValidator &uiv,
//    std::map<std::string, DataSelector *> const &data) {
//
//  auto &ads = AnalysisDataService::Instance();
//  auto workspaceName =
//  data.begin()->second->getCurrentDataName().toStdString(); if
//  (ads.doesExist(workspaceName))
//    if (auto workspace = ads.retrieveWS<MatrixWorkspace>(workspaceName))
//      auto numberOfHistograms = workspace->getNumberHistograms();
//
//  for (auto iter = data.begin(); iter != data.end(); ++iter) {
//
//    uiv.checkWorkspaceNumberOfHistograms();
//  }
//}

} // namespace CustomInterfaces
} // namespace MantidQt
