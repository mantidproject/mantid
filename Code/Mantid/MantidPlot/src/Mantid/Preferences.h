#ifndef MANTIDPREFERENCES_H
#define MANTIDPREFERENCES_H

#include <QString>
#include <QMap>
#include <QList>

/** @class MantidPreferences

 Keeps the preferences for some Mantid-related UI settings.

 @author Roman Tolchenov, Tessella Support Services plc
 @date 13/02/2009

 Copyright &copy; 2007-8 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge National Laboratory & European Spallation Source

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

 File change history is stored at: <https://github.com/mantidproject/mantid>.
 Code Documentation is available at: <http://doxygen.mantidproject.org>
 */

class MantidPreferences
{
public:
    // ---  MantidMatrixSettings --- //

    static int MantidMatrixColumnWidthY();
    static int MantidMatrixColumnWidthX();
    static int MantidMatrixColumnWidthE();
    static void MantidMatrixColumnWidthY(int width);
    static void MantidMatrixColumnWidthX(int width);
    static void MantidMatrixColumnWidthE(int width);
    static void MantidMatrixColumnWidth(int width);

    static QChar MantidMatrixNumberFormatY();
    static QChar MantidMatrixNumberFormatX();
    static QChar MantidMatrixNumberFormatE();
    static void MantidMatrixNumberFormatY(const QChar& f);
    static void MantidMatrixNumberFormatX(const QChar& f);
    static void MantidMatrixNumberFormatE(const QChar& f);
    static void MantidMatrixNumberFormat(const QChar& f);

    static int MantidMatrixNumberPrecisionY();
    static int MantidMatrixNumberPrecisionX();
    static int MantidMatrixNumberPrecisionE();
    static void MantidMatrixNumberPrecisionY(int p);
    static void MantidMatrixNumberPrecisionX(int p);
    static void MantidMatrixNumberPrecisionE(int p);
    static void MantidMatrixNumberPrecision(int p);


private:
    /// Creation of non-static instances is not allowed.
    MantidPreferences();
    MantidPreferences(const MantidPreferences&);
};


#endif /* MANTIDPREFERENCES_H */
