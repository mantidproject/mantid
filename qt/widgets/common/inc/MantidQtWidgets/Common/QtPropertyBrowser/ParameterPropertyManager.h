// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2014 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef PARAMETERPROPERTYMANAGER_H
#define PARAMETERPROPERTYMANAGER_H

#include "qtpropertymanager.h"

#include <QMap>
#include <QSet>

/** ParameterPropertyManager : specialized version of QtDoublePropertyManager
   for fitting parameters.

    Is capable to store/display parameter errors in addition to value.
  */
class EXPORT_OPT_MANTIDQT_COMMON ParameterPropertyManager
    : public QtDoublePropertyManager {
  Q_OBJECT

public:
  ParameterPropertyManager(QObject *parent = nullptr,
                           bool hasGlobalOption = false);

  /// Get parameter error
  double error(const QtProperty *property) const;

  /// Get parameter description
  std::string description(const QtProperty *property) const;

  /// Checks if given property has error value set
  bool isErrorSet(const QtProperty *property) const;

  /// Returns errors enabled status
  bool areErrorsEnabled() const { return m_errorsEnabled; }

  bool isGlobal(const QtProperty *property) const;

public Q_SLOTS:
  /// Set property error
  void setError(QtProperty *property, const double &error);

  /// Set parameter description
  void setDescription(QtProperty *property, const std::string &description);

  /// Clears error of the property, if one was set
  void clearError(QtProperty *property);

  /// Clears errors from all properties, if set
  void clearErrors();

  /// Enabled/disables error display
  void setErrorsEnabled(bool enabled);

  /// Set parameter's global option
  void setGlobal(QtProperty *property, bool option);

protected:
  /// Text representation of the property
  QString valueText(const QtProperty *property) const override;

private Q_SLOTS:
  /// Updates the tooltip of the property
  void updateTooltip(QtProperty *property) const;

private:
  /// Text appended to parameter descr. tooltip if error is set
  static const QString ERROR_TOOLTIP;

  /// Parameter error values
  QMap<QtProperty *, double> m_errors;

  /// Parameter descriptions
  QMap<QtProperty *, std::string> m_descriptions;

  /// Errors enabled flag. When is false, errors can be set, but will not be
  /// displayed
  bool m_errorsEnabled;

  bool m_hasGlobalOption;
  QSet<QtProperty *> m_globals;
};

#endif // PARAMETERPROPERTYMANAGER_H
