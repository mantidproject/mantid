#ifndef MANTID_API_TEXTPROPERTYWIDGET_H_
#define MANTID_API_TEXTPROPERTYWIDGET_H_

#include "MantidKernel/System.h"
#include "MantidQtWidgets/Common/PropertyWidget.h"
#include <QLabel>
#include <QLineEdit>

namespace MantidQt {
namespace API {

/** The most generic widgets for Property's that are only
 * a simple string.

  @date 2012-02-16

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
  Code Documentation is available at: <http://doxygen.mantidproject.org>
*/
class DLLExport TextPropertyWidget : public PropertyWidget {
  Q_OBJECT

public:
  TextPropertyWidget(Mantid::Kernel::Property *prop, QWidget *parent = nullptr,
                     QGridLayout *layout = nullptr, int row = -1);
  ~TextPropertyWidget() override;
  QString getValue() const override;
  void setValueImpl(const QString &value) override;

  ///@return the main widget of this combo of widgets
  QWidget *getMainWidget() override { return m_textbox; }

protected:
  /// Label (name of the property)
  QLabel *m_label;

  /// The text box to edit
  QLineEdit *m_textbox;
};

} // namespace API
} // namespace MantidQt

#endif /* MANTID_API_TEXTPROPERTYWIDGET_H_ */
