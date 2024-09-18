// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2007 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/MatrixWorkspace_fwd.h"
#include "MantidQtWidgets/Common/DataSelector.h"
#include "MantidQtWidgets/Common/FileFinderWidget.h"
#include "MantidQtWidgets/Common/WorkspaceSelector.h"
#include "MantidQtWidgets/Common/WorkspaceUtils.h"

using MantidQt::API::FileFinderWidget;
using MantidQt::MantidWidgets::DataSelector;
using MantidQt::MantidWidgets::WorkspaceSelector;

class QLineEdit;
class QLabel;
class QString;
class QStringList;

namespace MantidQt {
namespace CustomInterfaces {

class DLLExport IUserInputValidator {
public:
  virtual ~IUserInputValidator() = default;

  virtual bool checkFieldIsNotEmpty(const QString &name, QLineEdit *field, QLabel *errorLabel = nullptr) = 0;
  virtual bool checkFieldIsValid(const QString &errorMessage, QLineEdit *field, QLabel *errorLabel = nullptr) = 0;
  virtual bool checkWorkspaceSelectorIsNotEmpty(const QString &name, WorkspaceSelector *workspaceSelector) = 0;
  virtual bool checkFileFinderWidgetIsValid(const QString &name, const FileFinderWidget *widget) = 0;
  virtual bool checkDataSelectorIsValid(const QString &name, DataSelector *widget, bool silent = false) = 0;
  virtual bool checkWorkspaceGroupIsValid(QString const &groupName, QString const &inputType, bool silent = false) = 0;
  virtual bool checkWorkspaceExists(QString const &workspaceName, bool silent = false) = 0;
  virtual bool checkValidRange(const QString &name, std::pair<double, double> range) = 0;
  virtual bool checkRangesDontOverlap(std::pair<double, double> rangeA, std::pair<double, double> rangeB) = 0;
  virtual bool checkRangeIsEnclosed(const QString &outerName, std::pair<double, double> outer, const QString &innerName,
                                    std::pair<double, double> inner) = 0;
  virtual bool checkBins(double lower, double binWidth, double upper, double tolerance = 0.00000001) = 0;
  template <typename T = Mantid::API::MatrixWorkspace>
  bool checkWorkspaceType(QString const &workspaceName, QString const &inputType, QString const &validType,
                          bool silent = false);

  virtual void setErrorLabel(QLabel *errorLabel, bool valid) = 0;

  virtual void addErrorMessage(const std::string &message, bool const silent = false) = 0;

  virtual std::string generateErrorMessage() const = 0;
  virtual bool isAllInputValid() const = 0;
};

/**
 * A class to try and get rid of some of the boiler-plate code surrounding input
 * validation, and hopefully as a result make it more readable.
 *
 * It has as its state a QStringList, which are the accumulated error messages
 * after multiple calls to its "check[...]" member-functions.
 *
 *
 */
class DLLExport UserInputValidator final : public IUserInputValidator {
public:
  /// Default Constructor.
  UserInputValidator();
  ~UserInputValidator() override;

  /// Check that the given QLineEdit field is not empty.
  bool checkFieldIsNotEmpty(const QString &name, QLineEdit *field, QLabel *errorLabel = nullptr) override;
  /// Check that the given QLineEdit field is valid as per any validators it might have.
  bool checkFieldIsValid(const QString &errorMessage, QLineEdit *field, QLabel *errorLabel = nullptr) override;
  /// Check that the given WorkspaceSelector is not empty.
  bool checkWorkspaceSelectorIsNotEmpty(const QString &name, WorkspaceSelector *workspaceSelector) override;
  /// Check that the given FileFinderWidget widget has valid files.
  bool checkFileFinderWidgetIsValid(const QString &name, const FileFinderWidget *widget) override;
  /// Check that the given DataSelector widget has valid input.
  bool checkDataSelectorIsValid(const QString &name, DataSelector *widget, bool silent = false) override;
  /// Check that the given start and end range is valid.
  bool checkValidRange(const QString &name, std::pair<double, double> range) override;
  /// Check that the given ranges dont overlap.
  bool checkRangesDontOverlap(std::pair<double, double> rangeA, std::pair<double, double> rangeB) override;
  /// Check that the given "outer" range completely encloses the given "inner" range.
  bool checkRangeIsEnclosed(const QString &outerName, std::pair<double, double> outer, const QString &innerName,
                            std::pair<double, double> inner) override;
  /// Check that the given range can be split evenly into bins of the given width.
  bool checkBins(double lower, double binWidth, double upper, double tolerance = 0.00000001) override;
  /// Checks two values are not equal
  bool checkNotEqual(const QString &name, double x, double y = 0.0, double tolerance = 0.00000001);

  /// Checks that a workspace exists in the ADS
  bool checkWorkspaceExists(QString const &workspaceName, bool silent = false) override;
  /// Checks the number of histograms in a workspace
  bool checkWorkspaceNumberOfHistograms(QString const &workspaceName, std::size_t const &validSize);
  bool checkWorkspaceNumberOfHistograms(const Mantid::API::MatrixWorkspace_sptr &, std::size_t const &validSize);
  /// Checks the number of bins in a workspace
  bool checkWorkspaceNumberOfBins(QString const &workspaceName, std::size_t const &validSize);
  bool checkWorkspaceNumberOfBins(const Mantid::API::MatrixWorkspace_sptr &, std::size_t const &validSize);
  /// Checks that a workspace group contains valid matrix workspace's
  bool checkWorkspaceGroupIsValid(QString const &groupName, QString const &inputType, bool silent = false) override;

  /// Add a custom error message to the list.
  void addErrorMessage(const std::string &message, bool const silent = false) override;

  /// Sets a validation label
  void setErrorLabel(QLabel *errorLabel, bool valid) override;

  /// Returns an error message which contains all the error messages raised by
  /// the check functions.
  std::string generateErrorMessage() const override;
  /// Checks to see if all input is valid
  bool isAllInputValid() const override;

private:
  /// Any raised error messages.
  QStringList m_errorMessages;
  /// True if there has been an error.
  bool m_error;
};

/**
 * Checks if the workspace has the correct type.
 *
 * @param workspaceName The name of the workspace
 * @param inputType What the workspace is used for (e.g. Sample)
 * @param validType The type which is valid
 * @param silent True if an error should not be added to the validator.
 * @return True if the workspace has the correct type
 */
template <typename T>
bool IUserInputValidator::checkWorkspaceType(QString const &workspaceName, QString const &inputType,
                                             QString const &validType, bool silent) {
  if (checkWorkspaceExists(workspaceName, silent)) {
    if (!MantidWidgets::WorkspaceUtils::getADSWorkspace<T>(workspaceName.toStdString())) {
      addErrorMessage("The " + inputType.toStdString() + " workspace is not a " + validType.toStdString() + ".",
                      silent);
      return false;
    } else
      return true;
  }
  return false;
}

} // namespace CustomInterfaces
} // namespace MantidQt
