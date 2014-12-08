#ifndef MANTIDQTWIDGETS_CHECKBOXHEADER_H_
#define MANTIDQTWIDGETS_CHECKBOXHEADER_H_

#include "WidgetDllOption.h"

#include <QHeaderView>
#include <QMouseEvent>
#include <QPainter>

namespace MantidQt
{
  namespace MantidWidgets
  {
    /**
      This class subclasses and overwrites QHeaderView methods to enable checkboxes to exist in the table header.

      @author Jay Rainey
      @date 15/10/2013

      Copyright &copy; 2013 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge National Laboratory & European Spallation Source

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

    class EXPORT_OPT_MANTIDQT_MANTIDWIDGETS CheckboxHeader : public QHeaderView
    {
      Q_OBJECT

    public:
      /// Override QHeaderView constructor.
      explicit CheckboxHeader(Qt::Orientation orientation, QWidget *parent = 0);
      void setChecked(bool checked);

    signals:
      void toggled(bool checked);

    protected:
      /// Overrides QHeaderView allowing checkbox functionality in the first column of the table.
      void paintSection(QPainter *painter, const QRect &rect, int logicalIndex) const;
      /// Set the checkbox to checked when it is clicked.
      void mousePressEvent(QMouseEvent *event);

    private:
      /// The area around the checkbox.
      QRect checkBoxRect(const QRect &sourceRect) const;
      /// The state of the checkbox in the column header.
      bool m_checked;
    };
  }
}
#endif // MANTIDQTWIDGETS_CHECKBOXHEADER_H_
