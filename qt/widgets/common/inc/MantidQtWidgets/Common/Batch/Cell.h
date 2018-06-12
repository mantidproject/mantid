/**
See the developer documentation for Batch Widget at
developer.mantidproject.org/BatchWidget/index.html

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

File change history is stored at: <https://github.com/mantidproject/mantid>.
Code Documentation is available at: <http://doxygen.mantidproject.org>
*/
#ifndef MANTIDQTMANTIDWIDGETS_CELL_H_
#define MANTIDQTMANTIDWIDGETS_CELL_H_
#include "MantidQtWidgets/Common/DllOption.h"
#include <string>
#include <vector>

namespace MantidQt {
namespace MantidWidgets {
namespace Batch {

class EXPORT_OPT_MANTIDQT_COMMON Cell {
public:
  Cell(std::string const &contentText);
  Cell(std::string const &contentText, std::string const &backgroundColor,
       int borderThickness, std::string const &color, int borderOpacity,
       bool isEditable);

  void setContentText(std::string const &contentText);
  std::string const &contentText() const;

  void setIconFilePath(std::string const &iconPath);
  std::string const &iconFilePath() const;

  void setBorderColor(std::string const &borderColor);
  std::string const &borderColor() const;

  void setBackgroundColor(std::string const &backgroundColor);
  std::string const &backgroundColor() const;

  void setBorderOpacity(int transparency);
  int borderOpacity() const;

  int borderThickness() const;
  void setBorderThickness(int borderThickness);

  bool isEditable() const;
  void setEditable(bool isEditable);
  void disableEditing();
  void enableEditing();

private:
  std::string m_contentText;
  std::string m_backgroundColor;
  int m_borderThickness;
  int m_borderOpacity;
  std::string m_borderColor;
  std::string m_iconFilePath;
  bool m_isEditable;
};

EXPORT_OPT_MANTIDQT_COMMON std::ostream &operator<<(std::ostream &os,
                                                    Cell const &cell);
EXPORT_OPT_MANTIDQT_COMMON bool operator==(Cell const &lhs, Cell const &rhs);
EXPORT_OPT_MANTIDQT_COMMON bool operator!=(Cell const &lhs, Cell const &rhs);
EXPORT_OPT_MANTIDQT_COMMON std::vector<Cell>
paddedCellsToWidth(std::vector<Cell> const &cells, Cell const &paddingCell,
                   int paddedWidth);
}
}
}
#endif // MANTIDQTMANTIDWIDGETS_CELL_H_
