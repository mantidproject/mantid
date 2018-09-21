#ifndef MPLCPP_LINE2D_H
#define MPLCPP_LINE2D_H
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
*/
#include "MantidQtWidgets/MplCpp/Artist.h"
#include "MantidQtWidgets/MplCpp/DllConfig.h"

namespace MantidQt {
namespace Widgets {
namespace MplCpp {

/**
 * @brief Line2D holds an instance of a matplotlib Line2D type.
 * This type is designed to hold an existing Line2D instance that contains
 * data in numpy arrays that do not own their data but have a view on to an
 * existing vector of data. This object keeps the data alive.
 */
class MANTID_MPLCPP_DLL Line2D : public Artist {
public:
  Line2D(Python::Object obj, std::vector<double> xdataOwner,
         std::vector<double> ydataOwner);
  // not copyable
  Line2D(const Line2D &) = delete;
  Line2D &operator=(const Line2D &) = delete;
  // movable
  Line2D(Line2D &&) = default;
  Line2D &operator=(Line2D &&) = default;

private:
  // Containers that own the data making up the line
  std::vector<double> m_xOwner, m_yOwner;
};

} // namespace MplCpp
} // namespace Widgets
} // namespace MantidQt

#endif // MPLCPP_LINE2D_H
