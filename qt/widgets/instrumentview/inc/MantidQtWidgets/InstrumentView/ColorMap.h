#ifndef MANTIDQT_WIDGETS_INSTRUMENTVIEW_COLORMAP_H
#define MANTIDQT_WIDGETS_INSTRUMENTVIEW_COLORMAP_H
/*
Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
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
*/
#include <QtGlobal>

#if QT_VERSION < QT_VERSION_CHECK(5, 0, 0)
#include "MantidQtWidgets/LegacyQwt/MantidColorMap.h"

namespace MantidQt {
namespace MantidWidgets {
using ColorMap = MantidColorMap;
#else
#error No type defined for ColorMap for Qt >=5!
#endif

} // namespace MantidWidgets
} // namespace MantidQt

#endif // MANTIDQT_WIDGETS_INSTRUMENTVIEW_COLORMAP_H
