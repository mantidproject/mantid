#ifndef MANTID_PYTHONINTERFACE_ERRORHANDLING_H
#define MANTID_PYTHONINTERFACE_ERRORHANDLING_H
/**
    Copyright &copy; 2012 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
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
#include "MantidKernel/System.h"
#include <stdexcept>
#include <string>

/**
 * This file defines error handling code that transforms
 * a Python error state to C++ exceptions.
 */
namespace Mantid {
namespace PythonInterface {
namespace Environment {

/**
 * Exception type that captures the current Python error state
 * as a generic C++ exception for any general Python exception
 */
class DLLExport PythonException : public std::exception {
public:
  PythonException(bool withTrace = true);

  const char *what() const noexcept override { return m_msg.c_str(); }

private:
  std::string m_msg;
};

/// Exception type that captures the current Python error state
/// as a C++ std::runtime exception
class DLLExport PythonRuntimeError : public std::runtime_error {
public:
  PythonRuntimeError(bool withTrace = true);
};
} // namespace Environment
} // namespace PythonInterface
} // namespace Mantid

#endif /* MANTID_PYTHONINTERFACE_ERRORHANDLING_H_ */
