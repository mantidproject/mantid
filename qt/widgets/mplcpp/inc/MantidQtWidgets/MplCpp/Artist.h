#ifndef MPLCPP_ARTIST_H
#define MPLCPP_ARTIST_H
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
#include "MantidQtWidgets/MplCpp/DllConfig.h"
#include "MantidQtWidgets/MplCpp/Python/Object.h"

namespace MantidQt {
namespace Widgets {
namespace MplCpp {

/**
 * Wraps a matplotlib.artist object with a C++ interface
 */
class MANTID_MPLCPP_DLL Artist : Python::InstanceHolder {
public:
  // Holds a reference to the matplotlib artist object
  explicit Artist(Python::Object obj);

  // Remove the artist from the canvas
  void remove();
};

} // namespace MplCpp
} // namespace Widgets
} // namespace MantidQt

#endif // MPLCPP_ARTIST_H
