#ifndef MANTID_NOTEBOOKWRITER_H
#define MANTID_NOTEBOOKWRITER_H

/** @class NotebookWriter

    This class assists with writing ipython notebooks.

    Copyright &copy; 2007-8 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
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

    File change history is stored at: <https://github.com/mantidproject/mantid>.
    Code Documentation is available at: <http://doxygen.mantidproject.org>
    */

#include "MantidAPI/DllConfig.h"

#include <json/writer.h>
#include <string>

namespace Mantid {
namespace API {

class MANTID_API_DLL NotebookWriter {

public:
  NotebookWriter();
  virtual ~NotebookWriter() = default;
  std::string markdownCell(std::string string_text);
  std::string codeCell(std::string string_code);
  std::string writeNotebook();

private:
  void headerComment();
  void headerCode();

  Json::Value buildNotebook();

  void markdownCell(Json::Value string_array);
  void codeCell(Json::Value array_code);

  Json::Value m_cell_buffer;
};
} // namespace API
} // namespace Mantid

#endif // MANTID_NOTEBOOKWRITER_H
