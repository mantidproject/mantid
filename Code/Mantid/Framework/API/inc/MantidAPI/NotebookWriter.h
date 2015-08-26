#ifndef MANTID_NOTEBOOKWRITER_H
#define MANTID_NOTEBOOKWRITER_H

/** @class NotebookWriter

    This class assists with writing ipython notebooks.

    @author Matthew D Jones, Tessella, ISIS, RAL
    @date 25/08/2015

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

#include <json/json.h>
#include <string>

class NotebookWriter {

public:

  NotebookWriter(){};
  virtual ~NotebookWriter(){};

  template<typename JsonString>
  Json::Value markdownCell(JsonString string_array);
  template<typename JsonString>
  Json::Value codeCell(JsonString string_code);
  Json::Value buildNotebook();
  void writeNotebook();

private:

  Json::Value headerComment();
  Json::Value headerCode();

};

#endif //MANTID_NOTEBOOKWRITER_H
