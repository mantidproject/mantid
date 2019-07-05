// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2007 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_CUSTOMINTERFACES_USERINPUTVALIDATOR_H_
#define MANTID_CUSTOMINTERFACES_USERINPUTVALIDATOR_H_

#include "MantidAPI/MatrixWorkspace_fwd.h"
#include "MantidQtWidgets/Common/DataSelector.h"
#include "MantidQtWidgets/Common/MWRunFiles.h"
#include "MantidQtWidgets/Common/WorkspaceSelector.h"

using MantidQt::API::MWRunFiles;
using MantidQt::MantidWidgets::DataSelector;
using MantidQt::MantidWidgets::WorkspaceSelector;

class QLineEdit;
class QLabel;
class QString;
class QStringList;

namespace MantidQt {
namespace CustomInterfaces {
/**
 * A class to try and get rid of some of the boiler-plate code surrounding input
 * validation, and hopefully as a result make it more readable.
 *
 * It has as its state a QStringList, which are the accumulated error messages
 * after multiple calls to its "check[...]" member-functions.
 *
 *
 */
class DLLExport UserInputValidator {
public:
  /// Default Constructor.
  UserInputValidator();

  /// Check that the given QLineEdit field is not empty.
  bool checkFieldIsNotEmpty(const QString &name, QLineEdit *field,
                            QLabel *errorLabel = nullptr);
  /// Check that the given QLineEdit field is valid as per any validators it
  /// might have.
  bool checkFieldIsValid(const QString &errorMessage, QLineEdit *field,
                         QLabel *errorLabel = nullptr);
  /// Check that the given WorkspaceSelector is not empty.
  bool checkWorkspaceSelectorIsNotEmpty(const QString &name,
                                        WorkspaceSelector *workspaceSelector);
  /// Check that the given MWRunFiles widget has valid files.
  bool checkMWRunFilesIsValid(const QString &name, MWRunFiles *widget);
  /// Check that the given DataSelector widget has valid input.
  bool checkDataSelectorIsValid(const QString &name, DataSelector *widget);
  /// Check that the given start and end range is valid.
  bool checkValidRange(const QString &name, std::pair<double, double> range);
  /// Check that the given ranges dont overlap.
  bool checkRangesDontOverlap(std::pair<double, double> rangeA,
                              std::pair<double, double> rangeB);
  /// Check that the given "outer" range completely encloses the given "inner"
  /// range.
  bool checkRangeIsEnclosed(const QString &outerName,
                            std::pair<double, double> outer,
                            const QString &innerName,
                            std::pair<double, double> inner);
  /// Check that the given range can be split evenly into bins of the given
  /// width.
  bool checkBins(double lower, double binWidth, double upper,
                 double tolerance = 0.00000001);
  /// Checks two values are not equal
  bool checkNotEqual(const QString &name, double x, double y = 0.0,
                     double tolerance = 0.00000001);

  /// Checks that a workspace has the correct workspace type
  template <typename T = Mantid::API::MatrixWorkspace,
            typename R = Mantid::API::MatrixWorkspace_sptr>
  bool checkWorkspaceType(QString const &workspaceName,
                          QString const &validType);
  /// Checks that a workspace exists in the ADS
  bool checkWorkspaceExists(QString const &workspaceName);
  /// Checks the number of histograms in a workspace
  bool checkWorkspaceNumberOfHistograms(QString const &workspaceName,
                                        std::size_t const &validSize);
  bool checkWorkspaceNumberOfHistograms(Mantid::API::MatrixWorkspace_sptr,
                                        std::size_t const &validSize);
  /// Checks the number of bins in a workspace
  bool checkWorkspaceNumberOfBins(QString const &workspaceName,
                                  std::size_t const &validSize);
  bool checkWorkspaceNumberOfBins(Mantid::API::MatrixWorkspace_sptr,
                                  std::size_t const &validSize);

  /// Add a custom error message to the list.
  void addErrorMessage(const QString &message);

  /// Sets a validation label
  void setErrorLabel(QLabel *errorLabel, bool valid);

  /// Returns an error message which contains all the error messages raised by
  /// the check functions.
  QString generateErrorMessage();
  /// Checks to see if all input is valid
  bool isAllInputValid();

private:
  template <typename T = Mantid::API::MatrixWorkspace,
            typename R = Mantid::API::MatrixWorkspace_sptr>
  R getADSWorkspace(std::string const &workspaceName);

  /// Any raised error messages.
  QStringList m_errorMessages;
};

/**
 * Checks if the workspace has the correct type.
 *
 * @param workspaceName The name of the workspace
 * @param validType The type which is valid
 * @return True if the workspace has the correct type
 */
template <typename T, typename R>
bool UserInputValidator::checkWorkspaceType(QString const &workspaceName,
                                            QString const &validType) {
  if (checkWorkspaceExists(workspaceName)) {
    if (!getADSWorkspace<T, R>(workspaceName.toStdString())) {
      addErrorMessage(workspaceName + " is not a " + validType + ".");
      return false;
    } else
      return true;
  }
  return false;
}

/**
 * Gets a workspace from the ADS.
 *
 * @param workspaceName The name of the workspace
 * @return The workspace
 */
template <typename T, typename R>
R UserInputValidator::getADSWorkspace(std::string const &workspaceName) {
  return AnalysisDataService::Instance().retrieveWS<T>(workspaceName);
}

} // namespace CustomInterfaces
} // namespace MantidQt

#endif // MANTID_CUSTOMINTERFACES_USERINPUTVALIDATOR_H_
