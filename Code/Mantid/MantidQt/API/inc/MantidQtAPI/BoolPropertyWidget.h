#ifndef MANTID_API_BOOLPROPERTYWIDGET_H_
#define MANTID_API_BOOLPROPERTYWIDGET_H_

#include "MantidKernel/System.h"
#include "MantidKernel/PropertyWithValue.h"
#include "MantidQtAPI/PropertyWidget.h"
#include <qcheckbox.h>


namespace MantidQt
{
namespace API
{

  /** Set of widgets representing a PropertyWithValue<bool>.
   *
    
    @date 2012-02-16

    Copyright &copy; 2012 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge National Laboratory & European Spallation Source

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
  class DLLExport BoolPropertyWidget : public PropertyWidget
  {
    Q_OBJECT

  public:
    BoolPropertyWidget(Mantid::Kernel::PropertyWithValue<bool> * prop, QWidget * parent = NULL, QGridLayout * layout = NULL, int row=-1);
    virtual ~BoolPropertyWidget();
    QString getValue() const;
    virtual void setValueImpl(const QString & value);

    ///@return the main widget of this combo of widgets
    QWidget * getMainWidget() {return m_checkBox; }

  protected:
    /// Checkbox widget
    QCheckBox * m_checkBox;
  };


} // namespace API
} // namespace MantidQt

#endif  /* MANTID_API_BOOLPROPERTYWIDGET_H_ */
