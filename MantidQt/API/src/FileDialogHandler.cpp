#include "MantidQtAPI/FileDialogHandler.h"

namespace MantidQt {
namespace API {
namespace FileDialogHandler {
/**
    Contains modifications to Qt functions where problems have been found
    on certain operating systems

    Copyright &copy; 2009-2010 ISIS Rutherford Appleton Laboratory, NScD Oak
   Ridge National Laboratory & European Spallation Source
    @date 17/09/2010

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
QString getExistingDirectory(QWidget *parent, const QString &caption,
                             const QString &dir, QFileDialog::Options options) {
  options = options | QFileDialog::DontUseNativeDialog;
  return QFileDialog::getExistingDirectory(parent, caption, dir, options);
}

QString getOpenFileName(QWidget *parent, const QString &caption,
                        const QString &dir, const QString &filter,
                        QString *selectedFilter, QFileDialog::Options options) {
  options = options | QFileDialog::DontUseNativeDialog;
  return QFileDialog::getOpenFileName(parent, caption, dir, filter,
                                      selectedFilter, options);
}

QStringList getOpenFileNames(QWidget *parent, const QString &caption,
                             const QString &dir, const QString &filter,
                             QString *selectedFilter,
                             QFileDialog::Options options) {
  options = options | QFileDialog::DontUseNativeDialog;
  return QFileDialog::getOpenFileNames(parent, caption, dir, filter,
                                       selectedFilter, options);
}
QString getSaveFileName(QWidget *parent, const QString &caption,
                        const QString &dir, const QString &filter,
                        QString *selectedFilter, QFileDialog::Options options) {
  options = options | QFileDialog::DontUseNativeDialog;
  return QFileDialog::getSaveFileName(parent, caption, dir, filter,
                                      selectedFilter, options);
}
}
}
}
