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
#ifndef MANTIDQTMANTIDWIDGETS_CELLSTANDARDITEM_H_
#define MANTIDQTMANTIDWIDGETS_CELLSTANDARDITEM_H_
#include <QColor>
#include <QStandardItem>
#include "MantidQtWidgets/Common/Batch/Cell.h"

namespace MantidQt {
namespace MantidWidgets {
namespace Batch {

enum CellUserRoles {
  BorderColor = Qt::UserRole + 1,
  BorderThickness,
  IconFilePath
};

void setBorderThickness(QStandardItem &item, int borderThickness);
int getBorderThickness(QStandardItem const &item);

void setBorderColor(QStandardItem &item, std::string const &borderColor,
                    int alpha);
QColor getBorderColor(QStandardItem const &item);

std::string getIconFilePath(QStandardItem const &item);
void setIcon(QStandardItem &item, std::string const &iconFilePath);
void setIconFilePath(QStandardItem &item, QString const &iconFilePath);

std::string getBackgroundColor(QStandardItem const &item);
void setBackgroundColor(QStandardItem &item,
                        std::string const &backgroundColor);

void applyCellPropertiesToItem(Cell const &cell, QStandardItem &item);
Cell extractCellPropertiesFromItem(QStandardItem const &item);
}
}
}
#endif // MANTIDQTMANTIDWIDGETS_CELLSTANDARDITEM_H_
