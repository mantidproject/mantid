#ifndef MANTID_MANTIDWIDGETS_IFUNCTIONBROWSER_H_
#define MANTID_MANTIDWIDGETS_IFUNCTIONBROWSER_H_

#include "WidgetDllOption.h"
#include "MantidAPI/IFunction.h"
#include <QString>

namespace MantidQt {
namespace MantidWidgets {

/** IFunctionBrowser: interface for FunctionBrowser

  Abstract base class to be implemented

  Copyright &copy; 2016 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
  National Laboratory & European Spallation Source

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
class EXPORT_OPT_MANTIDQT_MANTIDWIDGETS IFunctionBrowser {
public:
  virtual ~IFunctionBrowser() {}
  virtual QString getFunctionString() = 0;
  virtual void functionStructureChanged() = 0;
  virtual void parameterChanged(const QString &funcIndex,
                                const QString &paramName) = 0;
  virtual void updateParameters(const Mantid::API::IFunction &fun) = 0;
  virtual void clear() = 0;
  virtual void setErrorsEnabled(bool enabled) = 0;
  virtual void clearErrors() = 0;
  virtual void setFunction(const QString &funStr) = 0;
  virtual void setNumberOfDatasets(int n) = 0;
  virtual Mantid::API::IFunction_sptr getGlobalFunction() = 0;
  virtual void
  updateMultiDatasetParameters(const Mantid::API::IFunction &fun) = 0;
  virtual bool isLocalParameterFixed(const QString &parName, int i) const = 0;
  virtual double getLocalParameterValue(const QString &parName,
                                        int i) const = 0;
  virtual QString getLocalParameterTie(const QString &parName, int i) const = 0;
  virtual int getNumberOfDatasets() const = 0;
  virtual void setLocalParameterValue(const QString &parName, int i,
                                      double value) = 0;
  virtual void setLocalParameterFixed(const QString &parName, int i,
                                      bool fixed) = 0;
  virtual void setLocalParameterTie(const QString &parName, int i,
                                    QString tie) = 0;
  virtual void setCurrentDataset(int i) = 0;
};

} // namespace MantidWidgets
} // namespace MantidQt

#endif // MANTID_MANTIDWIDGETS_IFUNCTIONBROWSER_H