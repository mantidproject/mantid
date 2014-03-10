#ifndef PARAMETERPROPERTYMANAGER_H
#define PARAMETERPROPERTYMANAGER_H

#include "qtpropertymanager.h"

#include <QMap>

/** ParameterPropertyManager : specialized version of QtDoublePropertyManager for fitting parameters.

    Is capable to store/display parameter errors in addition to value.

    Copyright &copy; 2014 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

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
class ParameterPropertyManager : public QtDoublePropertyManager
{
  Q_OBJECT

public:
  ParameterPropertyManager(QObject *parent = 0);

  /// Get property error
  double error(const QtProperty* property) const;

  /// Checks if given property has error value set
  bool isErrorSet(const QtProperty* property) const;

public Q_SLOTS:
  /// Set property error
  void setError(QtProperty* property, double error);

  /// Clears error of the property, if one was set
  void clearError(QtProperty* property);

protected:
  /// Text representation of the property
  virtual QString valueText(const QtProperty* property) const;

private:
  /// Property error values
  QMap<QtProperty*, double> m_errors;
};

#endif // PARAMETERPROPERTYMANAGER_H
