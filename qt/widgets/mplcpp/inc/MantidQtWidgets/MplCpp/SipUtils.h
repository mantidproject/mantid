#ifndef SIPUTILS_H
#define SIPUTILS_H
/**
 Copyright &copy; 2017 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
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
#include "MantidQtWidgets/MplCpp/PythonObject.h"

namespace MantidQt {
namespace Widgets {
namespace MplCpp {

/**
 * A collection of utility functions to deal with sip wrapped
 * C++ objects.
 */
struct SipUtils {
  static void *unwrap(PyObject *obj_ptr);
};
}
}
}

#endif // SIPUTILS_H
