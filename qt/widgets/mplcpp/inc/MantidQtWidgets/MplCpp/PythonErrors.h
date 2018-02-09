#ifndef PYTHONERRORS_H
#define PYTHONERRORS_H
/*
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
#include "MantidQtWidgets/MplCpp/DllOption.h"
#include <stdexcept>
#include <string>

namespace MantidQt {
namespace Widgets {
namespace MplCpp {

//-----------------------------------------------------------------------------
// Exceptions
//-----------------------------------------------------------------------------
/**
 * @brief The PythonError class
 *
 * A custom exception type to indicate a python error occurred.
 */
class EXPORT_OPT_MANTIDQT_MPLCPP PythonError : public std::exception {
public:
  explicit PythonError(bool withTrace = true);
  const char *what() const noexcept override { return m_msg.c_str(); }

private:
  std::string m_msg;
};

//-----------------------------------------------------------------------------
// Error Handling Utilities
//-----------------------------------------------------------------------------
/// Convert the current error indicator to a string and clear it
std::string errorToString(bool withTrace);
}
}
}
#endif // PYTHONERRORS_H
