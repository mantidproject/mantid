#ifndef PARAMETERPROPERTYMANAGER_H
#define PARAMETERPROPERTYMANAGER_H

#include "qtpropertymanager.h"

#include <QMap>

/** ParameterPropertyManager : specialized version of QtDoublePropertyManager for fitting parameters.

    Is capable to store/display parameter errors in addition to value.

    Copyright &copy; 2014 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge National Laboratory & European Spallation Source

    This file is part of Mantid.

    Mantid is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 3 of the License, or
    (at your option) any later version.

    Mantid is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.

    File change history is stored at: <https://github.com/mantidproject/mantid>
    Code Documentation is available at: <http://doxygen.mantidproject.org>
  */
class QT_QTPROPERTYBROWSER_EXPORT ParameterPropertyManager : public QtDoublePropertyManager
{
  Q_OBJECT

public:
  ParameterPropertyManager(QObject *parent = 0);

  /// Get parameter error
  double error(const QtProperty* property) const;

  /// Get parameter description
  std::string description(const QtProperty* property) const;

  /// Checks if given property has error value set
  bool isErrorSet(const QtProperty* property) const;

  /// Returns errors enabled status
  bool areErrorsEnabled() const { return m_errorsEnabled; }

public Q_SLOTS:
  /// Set property error
  void setError(QtProperty* property, const double& error);

  /// Set parameter description
  void setDescription(QtProperty* property, const std::string& description);

  /// Clears error of the property, if one was set
  void clearError(QtProperty* property);

  /// Enabled/disables error display
  void setErrorsEnabled(bool enabled);

protected:
  /// Text representation of the property
  virtual QString valueText(const QtProperty* property) const;

private Q_SLOTS:
  /// Updates the tooltip of the property
  void updateTooltip(QtProperty* property) const;

private:
  /// Text appended to parameter descr. tooltip if error is set
  static const QString ERROR_TOOLTIP;

  /// Parameter error values
  QMap<QtProperty*, double> m_errors;

  /// Parameter descriptions
  QMap<QtProperty*, std::string> m_descriptions;

  /// Errors enabled flag. When is false, errors can be set, but will not be displayed
  bool m_errorsEnabled;
};

#endif // PARAMETERPROPERTYMANAGER_H
