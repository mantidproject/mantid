/***************************************************************************
    File                 : pixmaps.h
    Project              : QtiPlot
    --------------------------------------------------------------------
    Copyright            : (C) 2006 by Ion Vasilief, Tilman Hoener zu
 Siederdissen
    Email (use @ for *)  : ion_vasilief*yahoo.fr, thzs*gmx.net
    Description          : Various pixmaps

 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *  This program is free software; you can redistribute it and/or modify   *
 *  it under the terms of the GNU General Public License as published by   *
 *  the Free Software Foundation; either version 2 of the License, or      *
 *  (at your option) any later version.                                    *
 *                                                                         *
 *  This program is distributed in the hope that it will be useful,        *
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of         *
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the          *
 *  GNU General Public License for more details.                           *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the Free Software           *
 *   Foundation, Inc., 51 Franklin Street, Fifth Floor,                    *
 *   Boston, MA  02110-1301  USA                                           *
 *                                                                         *
 ***************************************************************************/

#ifndef MANTIDQT_API_PIXMAPS_H
#define MANTIDQT_API_PIXMAPS_H

#include "qpixmap.h"
#include <MantidQtWidgets/Common/DllOption.h>

namespace MantidQt {
namespace API {
/** Function that returns a QPixmap given a string name. */
EXPORT_OPT_MANTIDQT_COMMON QPixmap getQPixmap(const std::string &name);
} // namespace API
} // namespace MantidQt

#endif // MANTIDQT_API_PIXMAPS_H
