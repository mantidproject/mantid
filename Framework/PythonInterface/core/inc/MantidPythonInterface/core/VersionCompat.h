#ifndef MANTID_PYTHONINTERFACE_CORE_PYTHONCOMPAT_H
#define MANTID_PYTHONINTERFACE_CORE_PYTHONCOMPAT_H
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
  Code Documentation is available at: <http://doxygen.mantidproject.org>
*/
#include <boost/python/detail/wrap_python.hpp>

// Macros for 2/3 compatability
#if PY_VERSION_HEX >= 0x03000000
#define IS_PY3K
#define INT_CHECK PyLong_Check
#define TO_LONG PyLong_AsLong
#define FROM_LONG PyLong_FromLong
#define STR_CHECK PyUnicode_Check
#define TO_CSTRING _PyUnicode_AsString
#define FROM_CSTRING PyUnicode_FromString
#define CODE_OBJECT(x) x
#else
#define INT_CHECK PyInt_Check
#define TO_LONG PyInt_AsLong
#define STR_CHECK PyString_Check
#define TO_CSTRING PyString_AsString
#define FROM_CSTRING PyString_FromString
#define CODE_OBJECT(x) (PyCodeObject *)x
#define FROM_LONG PyInt_FromLong
#endif

#endif // MANTID_PYTHONINTERFACE_CORE_PYTHONCOMPAT_H
