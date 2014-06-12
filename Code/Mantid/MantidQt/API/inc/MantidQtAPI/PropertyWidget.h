#ifndef MANTID_API_PROPERTYWIDGET_H_
#define MANTID_API_PROPERTYWIDGET_H_

#include "MantidKernel/System.h"
#include <QtGui/qwidget.h>
#include <qgridlayout.h>
#include "MantidKernel/Property.h"
#include "MantidQtAPI/PropertyInfoWidget.h"
#include <QLabel>
#include <QtCore/qstring.h>
#include <qpushbutton.h>
#include "DllOption.h"

class QLineEdit;

namespace MantidQt
{
namespace API
{

  /** Base class for widgets that will set
   * Mantid::Kernel::Property* types
    
    @date 2012-02-16

    Copyright &copy; 2012 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

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
  class EXPORT_OPT_MANTIDQT_API PropertyWidget : public QWidget
  {
    Q_OBJECT

  public:
    PropertyWidget(Mantid::Kernel::Property * prop, QWidget * parent = NULL, QGridLayout * layout = NULL, int row=-1);
    virtual ~PropertyWidget();
    
    bool inGrid() const;

    /// Return the value of the property given the GUI state.
    virtual QString getValue() const = 0;

    /// Set the value of the property given into the GUI state.
    void setValue(const QString & value);
    /// Set this widget's previously-entered value.
    void setPreviousValue(const QString & previousValue);

    virtual QWidget * getMainWidget() = 0;

    void setEnabled(bool val);
    void setVisible(bool val);

    /// @return the Layout object that these widget(s) are in.
    QGridLayout * getGridLayout()
    {return m_gridLayout; }

    /// @return the row of the widgets in the Layout
    int getGridRow()
    {return m_row; }

    void addReplaceWSButton();

    /// @return the property in the widget
    Mantid::Kernel::Property * getProperty()
    { return m_prop; }

    void setError(const QString & error);

  private:
    virtual void setValueImpl(const QString & value) = 0;
    void setRestoredStatus();

  public slots:
    void replaceWSButtonClicked();

    void valueChangedSlot();

  signals:
    /// Signal is emitted whenever the value (as entered by the user) in the GUI changes.
    void valueChanged(const QString & propName);

    /// Signal is emitted whenever someone clicks the replace WS button.
    void replaceWorkspaceName(const QString & propName);

  protected:
    /// Set the font of the given label based on the optional/required status of the given property.
    static void setLabelFont(Mantid::Kernel::Property * prop, QWidget * label);

    /// Set the placeholder text of the given field based on the default value of the given property.
    static void setFieldPlaceholderText(Mantid::Kernel::Property * prop, QLineEdit * field);

    /// Property being looked at. This is NOT owned by the widget
    Mantid::Kernel::Property * m_prop;

    /// Grid layout of the dialog to which we are adding widgets
    QGridLayout * m_gridLayout;

    /// Parent widget to add sub-widgets to.
    QWidget * m_parent;

    /// If using the GridLayout, this is the row where the widget was inserted.
    int m_row;

    /// Widget to display information about this property.
    PropertyInfoWidget * m_info;

    /// Documentation string (tooltip)
    QString m_doc;

    /// Button to "replace input workspace"
    QPushButton * m_replaceWSButton;

    /// All contained widgets
    QVector<QWidget*> m_widgets;

    /// Error message received when trying to set the value
    QString m_error;

    /// Whether or not the property is an output workspace.
    bool m_isOutputWsProp;

    QString m_previousValue;
  };


} // namespace API
} // namespace MantidQt

#endif  /* MANTID_API_PROPERTYWIDGET_H_ */
