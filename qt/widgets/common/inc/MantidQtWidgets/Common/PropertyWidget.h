// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2012 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidKernel/Property.h"
#include "MantidKernel/System.h"

#include <QGridLayout>
#include <QLabel>
#include <QMap>
#include <QPushButton>
#include <QString>
#include <QWidget>

#include "DllOption.h"

class QLineEdit;

namespace MantidQt {
namespace API {
/**
 * A small extension to QLabel, so that it emits a signal when clicked.
 * Used for the information "icons" in PropertyWidget.
 */
class ClickableLabel : public QLabel {
  Q_OBJECT

public:
  /// Constructor
  ClickableLabel(QWidget *parent);
  /// Destructor
  ~ClickableLabel() override;

signals:
  /// Signal emitted when a user clicks the label.
  void clicked();

protected:
  /// Catches the mouse press event and emits the signal.
  void mousePressEvent(QMouseEvent *event) override;
};

/** Base class for widgets that will set
 * Mantid::Kernel::Property* types

  @date 2012-02-16
*/
class EXPORT_OPT_MANTIDQT_COMMON PropertyWidget : public QWidget {
  Q_OBJECT

public:
  /// Set the placeholder text of the given field based on the default value of
  /// the given property.
  static void setFieldPlaceholderText(Mantid::Kernel::Property *prop, QLineEdit *field);

  enum Info { INVALID, REPLACE, RESTORE };

  PropertyWidget(Mantid::Kernel::Property *prop, QWidget *parent = nullptr, QGridLayout *layout = nullptr,
                 int row = -1);
  ~PropertyWidget() override;

  bool inGrid() const;

  /// Return the value of the property given the GUI state.
  virtual QString getValue() const = 0;

  /// Set the value of the property given into the GUI state.
  void setValue(const QString &value);
  /// Set this widget's previously-entered value.
  void setPreviousValue(const QString &previousValue);
  /// Set the `isDynamicDefault` flag associated with the previously-entered value.
  void setPrevious_isDynamicDefault(bool flag);

  /// Transfer the history state from another `PropertyWidget`, possibly additionally depending
  /// on the history state of an upstream property.
  void transferHistoryState(const PropertyWidget *other, const PropertyWidget *upstream = nullptr);

  virtual QWidget *getMainWidget() = 0;

  void setEnabled(bool val);
  void setVisible(bool val) override;

  /// @return the Layout object that these widget(s) are in.
  QGridLayout *getGridLayout() { return m_gridLayout; }

  /// @return the row of the widgets in the Layout
  int getGridRow() const { return m_row; }

  void addReplaceWSButton();

  /// @return the property in the widget
  const Mantid::Kernel::Property *getProperty() const { return m_prop; }
  Mantid::Kernel::Property *getProperty() { return m_prop; }

  void setError(const QString &error);

private:
  virtual void setValueImpl(const QString &value) = 0;

public slots:
  /// Update which icons should be shown.
  void updateIconVisibility(const QString &error = "");

  /// Deal with the "replace workspace" button being clicked.
  void replaceWSButtonClicked();

  /// Emits a signal that the value of the property was changed.
  void valueChangedSlot();

  /// To be called when a user edits a property, as opposed to one being set
  /// programmatically.
  void userEditedProperty();

  /// Toggle whether or not to use the previously-entered value.
  void toggleUseHistory();

private:
  /// Sets the history on/off icons and the dynamic-default marker.
  void setUseHistoryIcon(bool useHistory, bool isDynamicDefault);

signals:
  /// Signal is emitted whenever the value (as entered by the user) in the GUI
  /// changes.
  void valueChanged(const QString &propName);

  /// Signal is emitted whenever someone clicks the replace WS button.
  void replaceWorkspaceName(const QString &propName);

  void userChangedProperty();

protected:
  /// Set the font of the given label based on the optional/required status of
  /// the given property.
  static void setLabelFont(Mantid::Kernel::Property *prop, QWidget *label);

  /// Property being looked at. This is NOT owned by the widget
  Mantid::Kernel::Property *m_prop;

  /// Grid layout of the dialog to which we are adding widgets
  QGridLayout *m_gridLayout;

  /// Parent widget to add sub-widgets to.
  QWidget *m_parent;

  /// If using the GridLayout, this is the row where the widget was inserted.
  int m_row;

  /// Documentation string (tooltip)
  QString m_doc;

  /// Button to "replace input workspace"
  QPushButton *m_replaceWSButton;

  /// All contained widgets
  QVector<QWidget *> m_widgets;

  /// Last modified value
  QString m_lastValue;

  /// Error message received when trying to set the value
  QString m_error;

  /// Whether or not the property is an output workspace.
  bool m_isOutputWsProp;

  /// Stores the previously entered value when this dialog was last open.
  QString m_previousValue;

  /// Stores the `isDynamicDefault` flag corresponding to the previously entered value.
  bool m_previous_isDynamicDefault;

  /// Stores the last value entered by the user.
  QString m_enteredValue;

  /// Stores the `isDynamicDefault` flag corresponding to the last value entered by the user.
  bool m_entered_isDynamicDefault;

  /// Allow icon access by Info enum.
  QMap<Info, ClickableLabel *> m_icons;

  /// History on/off flag.  Note this is different from whether or not
  /// the property has a previously-entered value to actually use.
  bool m_useHistory;
};

} // namespace API
} // namespace MantidQt
