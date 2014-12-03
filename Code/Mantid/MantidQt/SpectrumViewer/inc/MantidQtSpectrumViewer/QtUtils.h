#ifndef  QT_UTILS_H
#define  QT_UTILS_H

#include <QTableWidget>
#include <QLineEdit>

#include "MantidQtSpectrumViewer/DllOptionSV.h"

/**
    @class QtUtils

    This class has some static methods to simplify interaction with
    Qt and Qwt for the SpectrumView data viewer.

    @author Dennis Mikkelson
    @date   2012-04-03

    Copyright © 2012 ORNL, STFC Rutherford Appleton Laboratories

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

    Code Documentation is available at
                 <http://doxygen.mantidproject.org>
 */

namespace MantidQt
{
namespace SpectrumView
{


class EXPORT_OPT_MANTIDQT_SPECTRUMVIEWER QtUtils
{
  public:
  /// enter the specified string in the table
  static void SetTableEntry(       int           row,
                                   int           col,
                             const std::string & string,
                                   QTableWidget* table );

  /// enter the specified double, formatted, in the table
  static void SetTableEntry( int           row,
                             int           col,
                             int           width,
                             int           precision,
                             double        value,
                             QTableWidget* table );

  /// Set the specified string into the specified QLineEdit widget.
  static void SetText( const std::string & string,
                             QLineEdit*    lineEdit );

  /// enter the specified double, formatted, in the QLineEdit control
  static void SetText( int        width,
                       int        precision,
                       double     value,
                       QLineEdit* lineEdit );
};

} // namespace SpectrumView
} // namespace MantidQt

#endif   // QT_UTILS_H
